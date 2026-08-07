/**
 * Copy from gatt-service.c
 */
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <pthread.h>

#include "bluetooth.h"
#include "l2cap.h"

#include "uni_log.h"
#include "tuya_gatt.h"

#define USE_BLUEZ
#ifdef USE_NO_BT_STACK

#define ATT_CID 4
#define ATT_PSM 31

// ATT 操作码
#define ATT_OP_ERROR                0x01
#define ATT_OP_MTU_REQ              0x02
#define ATT_OP_MTU_RESP             0x03
#define ATT_OP_FIND_INFO_REQ        0x04
#define ATT_OP_FIND_INFO_RESP       0x05
#define ATT_OP_READ_BY_TYPE_REQ     0x08
#define ATT_OP_READ_BY_TYPE_RESP    0x09
#define ATT_OP_READ_REQ             0x0A
#define ATT_OP_READ_RESP            0x0B
#define ATT_OP_READ_BLOB_REQ        0x0C
#define ATT_OP_READ_BLOB_RESP       0x0D
#define ATT_OP_READ_BY_GROUP_TYPE_REQ  0x10
#define ATT_OP_READ_BY_GROUP_TYPE_RESP 0x11
#define ATT_OP_WRITE_REQ            0x12
#define ATT_OP_WRITE_RESP           0x13
#define ATT_OP_READ_MULTI_REQ       0x16
#define ATT_OP_READ_MULTI_RESP      0x17
#define ATT_OP_PREPARE_WRITE_REQ    0x18
#define ATT_OP_PREPARE_WRITE_RESP   0x19

#define ATT_OP_HANDLE_NOTIFY        0x1B
#define ATT_OP_HANDLE_IND           0x1D
#define ATT_OP_WRITE_CMD            0x52

// ATT MTU configuration
#define ATT_DEFAULT_MTU             23
#define ATT_MIN_MTU                 23
#define ATT_MAX_MTU                 517

// GATT UUID
#define GATT_PRIM_SVC_UUID          0x2800
#define GATT_SEC_SVC_UUID           0x2801
#define GATT_INCLUDE_UUID           0x2802
#define GATT_CHARAC_UUID            0x2803
#define GATT_CLIENT_CHARAC_CFG_UUID 0x2902

typedef struct {
    uint16_t handle;
    gatt_uuid_t uuid;
    uint8_t properties;
    uint8_t permissions;
    uint16_t value_handle;
    uint8_t value[255];
    uint16_t value_len;
    uint16_t max_len;
} gatt_characteristic_t;

typedef struct
{
    uint16_t handle;
    gatt_uuid_t uuid;
    uint8_t permissions;
    uint16_t char_handle;
    uint8_t value[16];
    uint16_t value_len;
} gatt_descriptor_t;

typedef struct {
    uint16_t handle;
    gatt_uuid_t uuid;
    gatt_characteristic_t *characteristics;
    uint16_t char_count;
    gatt_descriptor_t *descriptors;
    uint16_t desc_count;
} gatt_service_t;

static int att_server_sock = -1;
static int att_client_sock = -1;
static pthread_t att_thread;
static bool att_running = false;
static uint16_t current_mtu = ATT_DEFAULT_MTU;  // 当前 MTU

static gatt_service_t *services = NULL;
static uint16_t service_count = 0;
static uint16_t next_handle = 1;

static void (*__gatt_connect_event)(int status) = NULL;
static void (*__gatt_write_request_event)(int handle, uint16_t uuid, uint8_t *data, uint8_t len) = NULL;

// UUID 比较函数
static bool uuid_cmp(const gatt_uuid_t *uuid1, const gatt_uuid_t *uuid2)
{
    if (uuid1->uuid_type != uuid2->uuid_type) {
        return false;
    }
    
    if (uuid1->uuid_type == 16) {
        return uuid1->uuid.uuid16 == uuid2->uuid.uuid16;
    } else if (uuid1->uuid_type == 32) {
        return uuid1->uuid.uuid32 == uuid2->uuid.uuid32;
    } else if (uuid1->uuid_type == 128) {
        return memcmp(uuid1->uuid.uuid128, uuid2->uuid.uuid128, 16) == 0;
    }
    
    return false;
}

// 从 uint16_t 创建 UUID
static void uuid_from_uint16(gatt_uuid_t *uuid, uint16_t val)
{
    uuid->uuid_type = 16;
    uuid->uuid.uuid16 = val;
}

// 从字符串创建 UUID
static int uuid_from_string(gatt_uuid_t *uuid, const char *str)
{
    size_t len = strlen(str);
    
    if (len == 4) {
        // 16-bit UUID
        uuid->uuid_type = 16;
        uuid->uuid.uuid16 = (uint16_t)strtol(str, NULL, 16);
        return 0;
    } else if (len == 8) {
        // 32-bit UUID
        uuid->uuid_type = 32;
        uuid->uuid.uuid32 = (uint32_t)strtol(str, NULL, 16);
        return 0;
    } else if (len == 36) {
        // 128-bit UUID (format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx)
        uuid->uuid_type = 128;
        
        // 移除连字符并解析
        char hex[33];
        int hex_idx = 0;
        for (int i = 0; i < len && hex_idx < 32; i++) {
            if (str[i] != '-') {
                hex[hex_idx++] = str[i];
            }
        }
        hex[hex_idx] = '\0';
        
        // 按小端序解析 UUID（蓝牙规范）
        for (int i = 0; i < 16; i++) {
            char byte_str[3] = {hex[30 - i * 2], hex[31 - i * 2], '\0'};
            uuid->uuid.uuid128[i] = (uint8_t)strtol(byte_str, NULL, 16);
        }
        return 0;
    }
    
    return -1;
}

// ATT 错误响应
static void send_error_resp(int sock, uint8_t opcode, uint16_t handle, uint8_t error)
{
    uint8_t pdu[5];
    pdu[0] = ATT_OP_ERROR;
    pdu[1] = opcode;
    pdu[2] = handle & 0xFF;
    pdu[3] = (handle >> 8) & 0xFF;
    pdu[4] = error;
    
    send(sock, pdu, sizeof(pdu), 0);
}

// 处理 MTU 请求
static void handle_mtu_req(int sock, uint8_t *pdu, int len)
{
    uint16_t client_mtu = pdu[1] | (pdu[2] << 8);
    
    // 协商 MTU（取双方最小值）
    current_mtu = (client_mtu < ATT_MAX_MTU) ? client_mtu : ATT_MAX_MTU;
    if (current_mtu < ATT_MIN_MTU) {
        current_mtu = ATT_MIN_MTU;
    }
    
    PR_DEBUG("MTU exchange: client=%d, negotiated=%d", client_mtu, current_mtu);
    
    uint8_t resp[3];
    resp[0] = ATT_OP_MTU_RESP;
    resp[1] = current_mtu & 0xFF;
    resp[2] = (current_mtu >> 8) & 0xFF;
    
    send(sock, resp, sizeof(resp), 0);
}

// 查找特征值
static gatt_characteristic_t* find_characteristic_by_handle(uint16_t handle)
{
    for (int i = 0; i < service_count; i++) {
        for (int j = 0; j < services[i].char_count; j++) {
            if (services[i].characteristics[j].handle == handle) {
                return &services[i].characteristics[j];
            }
        }
    }
    return NULL;
}

static gatt_characteristic_t* find_characteristic_by_value_handle(uint16_t handle)
{
    for (int i = 0; i < service_count; i++) {
        for (int j = 0; j < services[i].char_count; j++) {
            if (services[i].characteristics[j].value_handle == handle) {
                return &services[i].characteristics[j];
            }
        }
    }
    return NULL;
}

// 通过 UUID 查找特征
static gatt_characteristic_t* find_characteristic_by_uuid(const gatt_uuid_t *uuid)
{
    for (int i = 0; i < service_count; i++) {
        for (int j = 0; j < services[i].char_count; j++) {
            if (uuid_cmp(&services[i].characteristics[j].uuid, uuid)) {
                return &services[i].characteristics[j];
            }
        }
    }
    return NULL;
}

static gatt_descriptor_t* find_descriptor_by_handle(uint16_t handle)
{
    for (int i = 0; i < service_count; i++) {
        for (int j = 0; j < services[i].desc_count; j++) {
            if (services[i].descriptors[j].handle == handle) {
                return &services[i].descriptors[j];
            }
        }
    }
    return NULL;
}

static gatt_descriptor_t* find_descriptor_by_char_handle(uint16_t handle)
{
    for (int i = 0; i < service_count; i++) {
        for (int j = 0; j < services[i].desc_count; j++) {
            if (services[i].descriptors[j].char_handle == handle) {
                return &services[i].descriptors[j];
            }
        }
    }
    return NULL;
}

static uint16_t get_cccd_by_char_handle(uint16_t char_handle)
{
    gatt_descriptor_t *desc = find_descriptor_by_char_handle(char_handle);
    if (desc && uuid_cmp(&desc->uuid, &(gatt_uuid_t){.uuid_type=16, .uuid={.uuid16=GATT_CLIENT_CHARAC_CFG_UUID}})) {
        return desc->value[0] | (desc->value[1] << 8);
    }
    return -1;
}

// 处理读请求
static void handle_read_req(int sock, uint8_t *pdu, int len)
{
    uint16_t handle = pdu[1] | (pdu[2] << 8);
    PR_DEBUG("handle_read_req: handle=0x%04x", handle);
    gatt_characteristic_t *chr = find_characteristic_by_handle(handle-1);
    
    if (!chr) {
        PR_ERR("handle_read_req: characteristic not found");
        send_error_resp(sock, ATT_OP_READ_REQ, handle, 0x01); // Invalid handle
        return;
    }
    
    // 计算可用的 MTU 空间（减去操作码 1 字节）
    uint16_t max_data_len = current_mtu - 1;
    uint16_t actual_len = chr->value_len;
    
    // 如果数据长度超过 MTU，截断数据
    if (actual_len > max_data_len) {
        PR_WARN("Data length %d exceeds MTU %d, truncating to %d", 
                actual_len, current_mtu, max_data_len);
        actual_len = max_data_len;
    }
    
    uint8_t *resp = malloc(actual_len + 1);
    resp[0] = ATT_OP_READ_RESP;
    if (chr->value && actual_len > 0) {
        memcpy(resp + 1, chr->value, actual_len);
    }
    
    send(sock, resp, actual_len + 1, 0);
    free(resp);
}

// 处理 Read Blob 请求
static void handle_read_blob_req(int sock, uint8_t *pdu, int len)
{
    uint16_t handle = pdu[1] | (pdu[2] << 8);
    uint16_t offset = pdu[3] | (pdu[4] << 8);
    
    PR_DEBUG("handle_read_blob_req: handle=0x%04x, offset=%d", handle, offset);
    
    gatt_characteristic_t *chr = find_characteristic_by_handle(handle - 1);
    
    if (!chr) {
        PR_ERR("handle_read_blob_req: characteristic not found");
        send_error_resp(sock, ATT_OP_READ_BLOB_REQ, handle, 0x01); // Invalid handle
        return;
    }
    
    // 检查 offset 是否有效
    if (offset >= chr->value_len) {
        PR_ERR("handle_read_blob_req: invalid offset %d, value_len=%d", offset, chr->value_len);
        send_error_resp(sock, ATT_OP_READ_BLOB_REQ, handle, 0x07); // Invalid offset
        return;
    }
    
    // 计算可用的 MTU 空间（减去操作码 1 字节）
    uint16_t max_data_len = current_mtu - 1;
    uint16_t remaining_len = chr->value_len - offset;
    uint16_t actual_len = (remaining_len < max_data_len) ? remaining_len : max_data_len;
    
    PR_DEBUG("Read blob: offset=%d, value_len=%d, remaining=%d, sending=%d", 
             offset, chr->value_len, remaining_len, actual_len);
    
    uint8_t *resp = malloc(actual_len + 1);
    if (!resp) {
        PR_ERR("Failed to allocate response buffer");
        send_error_resp(sock, ATT_OP_READ_BLOB_REQ, handle, 0x08); // Insufficient resources
        return;
    }
    
    resp[0] = ATT_OP_READ_BLOB_RESP;
    if (chr->value && actual_len > 0) {
        memcpy(resp + 1, chr->value + offset, actual_len);
    }
    
    send(sock, resp, actual_len + 1, 0);
    free(resp);
}

// 处理写请求
static void handle_write_req(int sock, uint8_t *pdu, int len)
{
    uint16_t handle = pdu[1] | (pdu[2] << 8);
    PR_DEBUG("handle_write_req: handle=0x%04x", handle);
    gatt_characteristic_t *chr = find_characteristic_by_value_handle(handle);
    if (chr) {
        int data_len = len - 3;
        
        if(data_len > sizeof(chr->value)) {
            data_len = sizeof(chr->value);
        }

        memcpy(chr->value, pdu + 3, data_len);
        chr->value_len = data_len;
        
        // 触发写回调
        if (__gatt_write_request_event) {
            __gatt_write_request_event(chr->handle, chr->uuid.uuid.uuid16, pdu + 3, data_len);
        }
        
        // 发送写响应
        uint8_t resp = ATT_OP_WRITE_RESP;
        send(sock, &resp, 1, 0);
        return;
    }
    gatt_descriptor_t *desc = find_descriptor_by_handle(handle);
    if (desc) {
        int data_len = len - 3;

        if(data_len > sizeof(desc->value)) {
            data_len = sizeof(desc->value);
        }
        
        memcpy(desc->value, pdu + 3, data_len);
        desc->value_len = data_len;
        
        // 发送写响应
        uint8_t resp = ATT_OP_WRITE_RESP;
        send(sock, &resp, 1, 0);
        return;
    }

    PR_ERR("handle_write_req: characteristic not found");
    send_error_resp(sock, ATT_OP_WRITE_REQ, handle, 0x01);
    return;
}

// 处理写命令（无响应）
static void handle_write_cmd(int sock, uint8_t *pdu, int len)
{
    uint16_t handle = pdu[1] | (pdu[2] << 8);

    PR_DEBUG("handle_write_cmd: handle=0x%04x", handle);
    gatt_characteristic_t *chr = find_characteristic_by_handle(handle + 1);
    if (!chr) {
        PR_ERR("handle_write_cmd: characteristic not found");
        return; // 写命令不返回错误
    }

    int data_len = len - 3;
    memcpy(chr->value, pdu + 3, data_len);
    chr->value_len = data_len;
    
    if (__gatt_write_request_event) {
        __gatt_write_request_event(chr->handle, chr->uuid.uuid.uuid16, pdu + 3, data_len);
    }
}

// 处理按类型读取请求
static void handle_read_by_type_req(int sock, uint8_t *pdu, int len)
{
    uint16_t start_handle = pdu[1] | (pdu[2] << 8);
    uint16_t end_handle = pdu[3] | (pdu[4] << 8);
    uint16_t attr_type = pdu[5] | (pdu[6] << 8);

    PR_INFO("handle_read_by_type_req: start_handle=0x%04x, end_handle=0x%04x, attr_type=0x%04x",
            start_handle, end_handle, attr_type);
    
    // 通常用于发现特征 (GATT_CHARAC_UUID = 0x2803)
    if (attr_type == GATT_CHARAC_UUID) {
        uint8_t resp[23];
        resp[0] = ATT_OP_READ_BY_TYPE_RESP;
        
        int offset = 2;
        uint8_t item_len = 0; // 每个条目的长度
        
        for (int i = 0; i < service_count; i++) {
            for (int j = 0; j < services[i].char_count; j++) {
                uint16_t decl_handle = services[i].characteristics[j].handle;
                
                if (decl_handle >= start_handle && decl_handle <= end_handle) {
                    uint8_t uuid_type = services[i].characteristics[j].uuid.uuid_type;
                    uint8_t current_item_len = (uuid_type == 16) ? 7 : 21; // 7 for 16-bit, 21 for 128-bit
                    
                    // 确保所有条目长度一致
                    if (item_len == 0) {
                        item_len = current_item_len;
                        resp[1] = item_len;
                    } else if (item_len != current_item_len) {
                        // UUID 类型不同，跳过此条目
                        continue;
                    }
                    
                    // 检查是否有足够空间
                    if (offset + item_len > sizeof(resp)) {
                        break;
                    }
                    
                    resp[offset++] = decl_handle & 0xFF;
                    resp[offset++] = (decl_handle >> 8) & 0xFF;
                    resp[offset++] = services[i].characteristics[j].properties;
                    decl_handle++;
                    resp[offset++] = decl_handle & 0xFF;
                    resp[offset++] = (decl_handle >> 8) & 0xFF;
                    
                    if (uuid_type == 16) {
                        resp[offset++] = services[i].characteristics[j].uuid.uuid.uuid16 & 0xFF;
                        resp[offset++] = (services[i].characteristics[j].uuid.uuid.uuid16 >> 8) & 0xFF;
                    } else if (uuid_type == 128) {
                        memcpy(&resp[offset], services[i].characteristics[j].uuid.uuid.uuid128, 16);
                        offset += 16;
                    }
                }
            }
        }
        
        if (offset == 2) {
            send_error_resp(sock, ATT_OP_READ_BY_TYPE_REQ, start_handle, 0x0A); // Attribute not found
            PR_ERR("No characteristics found in range %d - %d", start_handle, end_handle);
            return;
        }
        PR_INFO("Read by type response length: %d", offset);
        send(sock, resp, offset, 0);
    } else {
        // 其他类型的属性读取
        send_error_resp(sock, ATT_OP_READ_BY_TYPE_REQ, start_handle, 0x0A); // Attribute not found
        PR_ERR("Unsupported attribute type: 0x%04x", attr_type);
    }
}

// 处理按组类型读取请求
static void handle_read_by_group_type_req(int sock, uint8_t *pdu, int len)
{
    uint16_t start_handle = pdu[1] | (pdu[2] << 8);
    uint16_t end_handle = pdu[3] | (pdu[4] << 8);
    uint16_t group_type = pdu[5] | (pdu[6] << 8);

    PR_INFO("handle_read_by_group_type_req: start_handle=0x%04x, end_handle=0x%04x, group_type=0x%04x",
            start_handle, end_handle, group_type);
    
    // 通常用于发现主服务 (GATT_PRIM_SVC_UUID = 0x2800)
    if (group_type != GATT_PRIM_SVC_UUID) {
        send_error_resp(sock, ATT_OP_READ_BY_GROUP_TYPE_REQ, start_handle, 0x0A); // Attribute not found
        PR_ERR("Unsupported group type: 0x%04x", group_type);
        return;
    }
    
    uint8_t resp[256];
    resp[0] = ATT_OP_READ_BY_GROUP_TYPE_RESP;
    int offset = 2;
    uint8_t item_len = 0; // 每个条目的长度
    
    for (int i = 0; i < service_count; i++) {
        if (services[i].handle >= start_handle && services[i].handle <= end_handle) {
            uint8_t uuid_type = services[i].uuid.uuid_type;
            uint8_t current_item_len = (uuid_type == 16) ? 6 : 20; // 6 for 16-bit, 20 for 128-bit
            
            // 确保所有条目长度一致
            if (item_len == 0) {
                item_len = current_item_len;
                resp[1] = item_len;
            } else if (item_len != current_item_len) {
                // UUID 类型不同，跳过此条目
                continue;
            }
            
            // 检查是否有足够空间
            if (offset + item_len > sizeof(resp)) {
                break;
            }
            
            resp[offset++] = services[i].handle & 0xFF;
            resp[offset++] = (services[i].handle >> 8) & 0xFF;
            
            // End group handle (服务的最后一个 handle)
            uint16_t end_grp_handle = services[i].handle;
            if (services[i].char_count > 0) {
                end_grp_handle = services[i].characteristics[services[i].char_count - 1].handle;
            }
            resp[offset++] = end_grp_handle & 0xFF;
            resp[offset++] = (end_grp_handle >> 8) & 0xFF;
            
            if (uuid_type == 16) {
                resp[offset++] = services[i].uuid.uuid.uuid16 & 0xFF;
                resp[offset++] = (services[i].uuid.uuid.uuid16 >> 8) & 0xFF;
            } else if (uuid_type == 128) {
                memcpy(&resp[offset], services[i].uuid.uuid.uuid128, 16);
                offset += 16;
            }
        }
    }
    
    if (offset == 2) {
        send_error_resp(sock, ATT_OP_READ_BY_GROUP_TYPE_REQ, start_handle, 0x0A);
        PR_ERR("No services found in range, start: 0x%04x, end: 0x%04x", start_handle, end_handle);
        return;
    }
    
    send(sock, resp, offset, 0);
}

// 处理查找信息请求
static void handle_find_info_req(int sock, uint8_t *pdu, int len)
{
    uint16_t start_handle = pdu[1] | (pdu[2] << 8);
    uint16_t end_handle = pdu[3] | (pdu[4] << 8);
    
    PR_INFO("handle_find_info_req: start_handle=0x%04x, end_handle=0x%04x",
            start_handle, end_handle);

    uint8_t resp[256];
    resp[0] = ATT_OP_FIND_INFO_RESP;
    int offset = 2;
    uint8_t format = 0; // 0: not set, 1: 16-bit UUIDs, 2: 128-bit UUIDs
    
    // 遍历所有服务和特征，查找在范围内的句柄
    for (int i = 0; i < service_count; i++) {
        // 检查服务句柄
        if (services[i].handle >= start_handle && services[i].handle <= end_handle) {
            uint8_t uuid_type = services[i].uuid.uuid_type;
            uint8_t new_format = (uuid_type == 16) ? 1 : 2;
            
            if (format == 0) {
                format = new_format;
                resp[1] = format;
            }
            
            // 只添加与当前格式匹配的 UUID
            if (format == new_format) {
                if (offset + 2 + (format == 1 ? 2 : 16) > sizeof(resp)) {
                    break;
                }
                
                resp[offset++] = services[i].handle & 0xFF;
                resp[offset++] = (services[i].handle >> 8) & 0xFF;
                resp[offset++] = GATT_PRIM_SVC_UUID & 0xFF;
                resp[offset++] = (GATT_PRIM_SVC_UUID >> 8) & 0xFF;
            }
        }
        
        // 检查特征
        for (int j = 0; j < services[i].char_count; j++) {
            gatt_characteristic_t *chr = &services[i].characteristics[j];
            
            // 特征声明句柄
            if (chr->handle >= start_handle && chr->handle <= end_handle) {
                if (format == 0) {
                    format = 1; // 特征声明总是使用 16 位 UUID (0x2803)
                    resp[1] = format;
                }
                
                if (format == 1) {
                    if (offset + 4 > sizeof(resp)) {
                        break;
                    }
                    
                    // 返回特征声明的 UUID (0x2803)
                    resp[offset++] = chr->handle & 0xFF;
                    resp[offset++] = (chr->handle >> 8) & 0xFF;
                    resp[offset++] = GATT_CHARAC_UUID & 0xFF;
                    resp[offset++] = (GATT_CHARAC_UUID >> 8) & 0xFF;
                }
                
                // 如果特征支持 notify 或 indicate，添加 CCCD 描述符
                if ((chr->properties & (LE_GATT_CHR_PROP_NOTIFY | LE_GATT_CHR_PROP_INDICATE)) && 
                    (chr->handle + 1) >= start_handle && (chr->handle + 1) <= end_handle) {
                    if (format == 0) {
                        format = 1;
                        resp[1] = format;
                    }
                    
                    if (format == 1) {
                        if (offset + 4 > sizeof(resp)) {
                            break;
                        }
                        
                        resp[offset++] = (chr->handle + 1) & 0xFF;
                        resp[offset++] = ((chr->handle + 1) >> 8) & 0xFF;
                        resp[offset++] = GATT_CLIENT_CHARAC_CFG_UUID & 0xFF;
                        resp[offset++] = (GATT_CLIENT_CHARAC_CFG_UUID >> 8) & 0xFF;
                    }
                }
            }
        }
    }
    
    if (offset == 2) {
        send_error_resp(sock, ATT_OP_FIND_INFO_REQ, start_handle, 0x0A); // Attribute not found
        PR_ERR("No attributes found in range 0x%04x - 0x%04x", start_handle, end_handle);
        return;
    }
    
    send(sock, resp, offset, 0);
    PR_DEBUG("Find info response sent, format: %d, bytes: %d", format, offset);
}

// 处理读多个属性请求
static void handle_read_multi_req(int sock, uint8_t *pdu, int len)
{
    if (len < 5) { // 至少需要 opcode(1) + 2 个 handle(4)
        send_error_resp(sock, ATT_OP_READ_MULTI_REQ, 0, 0x04); // Invalid PDU
        return;
    }
    
    int handle_count = (len - 1) / 2;
    uint16_t handles[handle_count];
    
    // 解析所有句柄
    for (int i = 0; i < handle_count; i++) {
        handles[i] = pdu[1 + i * 2] | (pdu[2 + i * 2] << 8);
    }
    
    PR_DEBUG("handle_read_multi_req: reading %d handles", handle_count);
    
    // 计算总数据长度
    uint16_t total_len = 0;
    gatt_characteristic_t *chrs[handle_count];
    
    for (int i = 0; i < handle_count; i++) {
        chrs[i] = find_characteristic_by_handle(handles[i] - 1);
        if (!chrs[i]) {
            PR_ERR("handle_read_multi_req: handle 0x%04x not found", handles[i]);
            send_error_resp(sock, ATT_OP_READ_MULTI_REQ, handles[i], 0x01); // Invalid handle
            return;
        }
        total_len += chrs[i]->value_len;
    }
    
    // 检查 MTU 限制
    uint16_t max_data_len = current_mtu - 1;
    if (total_len > max_data_len) {
        total_len = max_data_len;
    }
    
    uint8_t *resp = malloc(total_len + 1);
    if (!resp) {
        send_error_resp(sock, ATT_OP_READ_MULTI_REQ, handles[0], 0x08); // Insufficient resources
        return;
    }
    
    resp[0] = ATT_OP_READ_MULTI_RESP;
    uint16_t offset = 1;
    
    // 复制所有特征值
    for (int i = 0; i < handle_count && offset < total_len + 1; i++) {
        uint16_t copy_len = chrs[i]->value_len;
        if (offset + copy_len > total_len + 1) {
            copy_len = total_len + 1 - offset;
        }
        
        if (chrs[i]->value && copy_len > 0) {
            memcpy(resp + offset, chrs[i]->value, copy_len);
            offset += copy_len;
        }
    }
    
    send(sock, resp, offset, 0);
    free(resp);
    
    PR_DEBUG("Read multi response sent, total bytes: %d", offset);
}

// 处理准备写请求
static void handle_prepare_write_req(int sock, uint8_t *pdu, int len)
{
    if (len < 5) {
        send_error_resp(sock, ATT_OP_PREPARE_WRITE_REQ, 0, 0x04); // Invalid PDU
        return;
    }
    
    uint16_t handle = pdu[1] | (pdu[2] << 8);
    uint16_t offset = pdu[3] | (pdu[4] << 8);
    int data_len = len - 5;
    
    PR_DEBUG("handle_prepare_write_req: handle=0x%04x, offset=%d, len=%d", 
             handle, offset, data_len);
    
    gatt_characteristic_t *chr = find_characteristic_by_handle(handle - 1);
    
    if (!chr) {
        PR_ERR("handle_prepare_write_req: characteristic not found");
        send_error_resp(sock, ATT_OP_PREPARE_WRITE_REQ, handle, 0x01); // Invalid handle
        return;
    }
    
    // 检查是否有足够空间
    if (offset + data_len > chr->max_len) {
        send_error_resp(sock, ATT_OP_PREPARE_WRITE_REQ, handle, 0x07); // Invalid offset
        return;
    }
    
    // 扩展 value buffer 如果需要
    if (offset + data_len > chr->value_len) {
        chr->value_len = offset + data_len;
    }
    
    // 写入数据到指定偏移
    memcpy(chr->value + offset, pdu + 5, data_len);
    chr->value_offset = offset + data_len; // 保存当前偏移
    
    // 构造响应（回显请求）
    uint8_t *resp = malloc(len);
    if (!resp) {
        send_error_resp(sock, ATT_OP_PREPARE_WRITE_REQ, handle, 0x08); // Insufficient resources
        return;
    }
    
    resp[0] = ATT_OP_PREPARE_WRITE_RESP;
    memcpy(resp + 1, pdu + 1, len - 1);
    
    send(sock, resp, len, 0);
    free(resp);
    
    PR_DEBUG("Prepare write response sent");
}

// ATT 服务器线程
static void* att_server_thread(void *arg)
{
    uint8_t buf[512];
    
    while (att_running) {
        if (att_client_sock < 0) {
            struct sockaddr_l2 addr;
            socklen_t addr_len = sizeof(addr);
            
            att_client_sock = accept(att_server_sock, (struct sockaddr *)&addr, &addr_len);
            if (att_client_sock < 0) {
                usleep(100000);
                continue;
            }
            
            PR_DEBUG("ATT client connected");
            if (__gatt_connect_event) {
                __gatt_connect_event(1);
            }
        }
        
        int bytes = recv(att_client_sock, buf, sizeof(buf), 0);
        if (bytes <= 0) {
            close(att_client_sock);
            att_client_sock = -1;
            PR_DEBUG("ATT client disconnected");
            if (__gatt_connect_event) {
                __gatt_connect_event(0);
            }
            continue;
        }
        
        uint8_t opcode = buf[0];
        
        PR_DEBUG("Received ATT opcode: 0x%02x", opcode);
        switch (opcode) {
            case ATT_OP_MTU_REQ:
                handle_mtu_req(att_client_sock, buf, bytes);
                break;
            case ATT_OP_FIND_INFO_REQ:
                handle_find_info_req(att_client_sock, buf, bytes);
                break;
            case ATT_OP_READ_BY_GROUP_TYPE_REQ:
                handle_read_by_group_type_req(att_client_sock, buf, bytes);
                break;
            case ATT_OP_READ_BY_TYPE_REQ:
                handle_read_by_type_req(att_client_sock, buf, bytes);
                break;
            case ATT_OP_READ_REQ:
                handle_read_req(att_client_sock, buf, bytes);
                break;
            case ATT_OP_READ_BLOB_REQ:
                handle_read_blob_req(att_client_sock, buf, bytes);
                break;
            case ATT_OP_READ_MULTI_REQ:
                handle_read_multi_req(att_client_sock, buf, bytes);
                break;
            case ATT_OP_WRITE_REQ:
                handle_write_req(att_client_sock, buf, bytes);
                break;
            case ATT_OP_WRITE_CMD:
                handle_write_cmd(att_client_sock, buf, bytes);
                break;
            case ATT_OP_PREPARE_WRITE_REQ:
                handle_prepare_write_req(att_client_sock, buf, bytes);
                break;
            default:
                PR_DEBUG("Unhandled ATT opcode: 0x%02x", opcode);
                break;
        }
    }
    
    return NULL;
}

int tuya_gatt_init(void)
{
    struct sockaddr_l2 addr;
    int opt;
    
    // 创建 L2CAP socket
    att_server_sock = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (att_server_sock < 0) {
        PR_ERR("Failed to create L2CAP socket: %s", strerror(errno));
        return LE_COM_ERROR;
    }
    
    // 绑定到 ATT PSM
    memset(&addr, 0, sizeof(addr));
    addr.l2_family = AF_BLUETOOTH;
    addr.l2_cid = htobs(ATT_CID);
    addr.l2_bdaddr_type = BDADDR_LE_PUBLIC;
    bacpy(&addr.l2_bdaddr, BDADDR_ANY);
    
    if (bind(att_server_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        PR_ERR("Failed to bind L2CAP socket: %s", strerror(errno));
        close(att_server_sock);
        return LE_COM_ERROR;
    }
    
    // 监听连接
    if (listen(att_server_sock, 1) < 0) {
        PR_ERR("Failed to listen on L2CAP socket: %s", strerror(errno));
        close(att_server_sock);
        return LE_COM_ERROR;
    }
    
    // 启动 ATT 服务器线程
    att_running = true;
    if (pthread_create(&att_thread, NULL, att_server_thread, NULL) != 0) {
        PR_ERR("Failed to create ATT server thread");
        close(att_server_sock);
        return LE_COM_ERROR;
    }
    
    PR_DEBUG("GATT server initialized");
    return LE_SUCCESS;
}

gatt_uuid_t convet_tuya_uuid(TKL_BLE_UUID_T *p_uuid)
{
    gatt_uuid_t uuid;
    if (p_uuid->uuid_type == TKL_BLE_UUID_TYPE_16) {
        uuid.uuid_type = 16;
        uuid.uuid.uuid16 = p_uuid->uuid.uuid16;
    } else if (p_uuid->uuid_type == TKL_BLE_UUID_TYPE_32) {
        uuid.uuid_type = 32;
        uuid.uuid.uuid32 = p_uuid->uuid.uuid32;
    } else if (p_uuid->uuid_type == TKL_BLE_UUID_TYPE_128) {
        uuid.uuid_type = 128;
        for(int i=0; i<16; i++)
        {
            uuid.uuid.uuid128[i] = p_uuid->uuid.uuid128[i]; 
        }
    } else {
        uuid.uuid_type = 0; // invalid
    }
    return uuid;
}


int tuya_gatt_register_service(TKL_BLE_GATTS_PARAMS_T *p_service)
{
     for(int i = 0; i < p_service->svc_num; i++) {
        PR_INFO("register service %d, uuid_type: %d", i, p_service->p_service[i].svc_uuid.uuid_type);
        p_service->p_service[i].handle = tuya_gatt_add_service(convet_tuya_uuid(&p_service->p_service[i].svc_uuid));
        for(int j = 0; j < p_service->p_service[i].char_num; j++) {
            PR_INFO("  register characteristic %d, uuid_type: %d", j, p_service->p_service[i].p_char[j].char_uuid.uuid_type);
            p_service->p_service[i].p_char[j].handle = tuya_gatt_register_characteristic(
                p_service->p_service[i].handle,
                convet_tuya_uuid(&p_service->p_service[i].p_char[j].char_uuid),
                p_service->p_service[i].p_char[j].property,
                p_service->p_service[i].p_char[j].permission,
                p_service->p_service[i].p_char[j].value_len
            );
        }
    }
}

int tuya_gatt_add_service(gatt_uuid_t uuid)
{
    services = realloc(services, (service_count + 1) * sizeof(gatt_service_t));
    
    services[service_count].handle = next_handle++;
    services[service_count].uuid = uuid;
    services[service_count].characteristics = NULL;
    services[service_count].char_count = 0;
    
    service_count++;
    
    PR_DEBUG("Registered service UUID: 0x%04x, handle: %d", uuid.uuid.uuid16, services[service_count - 1].handle);
    return services[service_count - 1].handle;
}

int tuya_gatt_register_characteristic(int srv_handle, gatt_uuid_t char_uuid, uint8_t props, uint8_t perm, uint8_t value_len)
{
    // 查找服务
    gatt_service_t *svc = NULL;
    
    if(srv_handle <=0 ) {
        PR_ERR("Invalid service handle: %d", srv_handle);
        return LE_COM_ERROR;
    }
    for (int i = 0; i < service_count; i++) {
        if (services[i].handle == srv_handle) {
            svc = &services[i];
            break;
        }
    }
    
    if (!svc) {
        PR_ERR("Service handle: %d not found", srv_handle);
        return LE_COM_ERROR;
    }
    
    // 添加特征
    svc->characteristics = realloc(svc->characteristics, (svc->char_count + 1) * sizeof(gatt_characteristic_t));
    
    gatt_characteristic_t *chr = &svc->characteristics[svc->char_count];
    chr->handle = next_handle++;
    chr->uuid = char_uuid;
    chr->properties = props;
    chr->permissions = perm;
    chr->value_handle = next_handle++;
    chr->value_len = value_len;
    chr->max_len = 512;
    
    svc->char_count++;
    
    if (chr->uuid.uuid_type == 16) {
        PR_DEBUG("Registered characteristic UUID: 0x%04x, handle: %d", chr->uuid.uuid.uuid16, chr->handle);
    } else {
        PR_DEBUG("Registered characteristic, handle: %d", chr->handle);
    }
    return chr->handle;
}

int tuya_gatt_register_descriptor(int srv_handle, int char_handle, gatt_uuid_t desc_uuid, uint8_t perm)
{
      // 查找服务
    gatt_service_t *svc = NULL;
    
    if(srv_handle <=0 ) {
        PR_ERR("Invalid service handle: %d", srv_handle);
        return LE_COM_ERROR;
    }
    for (int i = 0; i < service_count; i++) {
        if (services[i].handle == srv_handle) {
            svc = &services[i];
            break;
        }
    }
    
    if (!svc) {
        PR_ERR("Service handle: %d not found", srv_handle);
        return LE_COM_ERROR;
    }
    
    // 添加描述符
    svc->descriptors = realloc(svc->descriptors, (svc->desc_count + 1) * sizeof(gatt_descriptor_t));
    gatt_descriptor_t *desc = &svc->descriptors[svc->desc_count];
    desc->handle = next_handle++;
    desc->char_handle = char_handle;
    desc->uuid = desc_uuid;
    desc->permissions = perm;
    svc->desc_count++;
    return desc->handle;
}

int tuya_gatt_server_send_characteristic_notification(uint16_t char_handle, uint8_t *data, uint8_t len)
{
    if (att_client_sock < 0) {
        PR_WARN("No client connected");
        return LE_COM_ERROR;
    }
    
    uint16_t cccd = get_cccd_by_char_handle(char_handle);
    if (cccd !=1 && cccd !=3) {
        PR_WARN("Notifications not enabled for handle: %d", char_handle);
        return LE_COM_ERROR;
    }

    // 发送通知
    uint8_t *pdu = malloc(len + 3);
    pdu[0] = ATT_OP_HANDLE_NOTIFY;
    pdu[1] = char_handle & 0xFF;
    pdu[2] = (char_handle >> 8) & 0xFF;
    memcpy(pdu + 3, data, len);
    
    int ret = send(att_client_sock, pdu, len + 3, 0);
    free(pdu);
    
    if (ret < 0) {
        PR_ERR("Failed to send notification: %s", strerror(errno));
        return LE_COM_ERROR;
    }
    
    return LE_SUCCESS;
}

void tuya_gatt_register_connect_event(void(*cb)(int status))
{
    __gatt_connect_event = cb;
}

void tuya_gatt_register_write_req_event(void(*cb)(int handle, uint16_t uuid, uint8_t *data, uint8_t len))
{
    __gatt_write_request_event = cb;
}

#elif defined USE_BLUEZ
#include "uuid.h"
#include "att.h"
#include "mainloop.h"
#include "gatt-server.h"
#include "gatt-db.h"

static void (*__gatt_connect_event)(int status) = NULL;
static void (*__gatt_write_request_event)(int handle, uint16_t uuid, uint8_t *data, uint8_t len) = NULL;

struct gatt_server_t
{
    struct bt_att *att;
    struct gatt_db *db;
    struct bt_gatt_server *gatt;
    struct gatt_db_attribute *svc;
    int hci_sock;
    bdaddr_t src_addr;
};

struct gatt_server_t gatt_server;
static void att_disconnect_cb(struct bt_att *att, void *user_data)
{
    PR_DEBUG("ATT disconnected callback called!\r\n");
    
    // 退出 mainloop 以便线程可以重新 accept
    mainloop_quit();
    
    if (__gatt_connect_event)
    {
        __gatt_connect_event(0);
    }
}

/* 蓝牙连接管理线程 */
void * l2cap_server_pthread(void *pReserved)
{
    int l2cap_sock = -1;
    int ret = -1;
    struct sockaddr_l2 l2cap_serveraddr;
    struct sockaddr_l2 l2cap_clientaddr;
    socklen_t l2cap_clientaddr_len;
    struct bt_security bt_sec;
    char l2cap_client_bdaddr[18];

    PR_DEBUG("Function l2cap_server_pthread called!\r\n");
    l2cap_sock = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (l2cap_sock < 0)
    {
        perror("Failed to socket!\r\n");
        return NULL;
    }

    memset(&l2cap_serveraddr, 0, sizeof(l2cap_serveraddr));
    l2cap_serveraddr.l2_family = AF_BLUETOOTH;
    l2cap_serveraddr.l2_cid = htobs(4);
    l2cap_serveraddr.l2_bdaddr_type = BDADDR_LE_PUBLIC;
    bacpy(&l2cap_serveraddr.l2_bdaddr, &gatt_server.src_addr);

    ret = bind(l2cap_sock, (struct sockaddr *)&l2cap_serveraddr, sizeof(l2cap_serveraddr));
    if (ret < 0)
    {
        perror("Failed to bind!\r\n");
        close(l2cap_sock);
        return NULL;
    }

    memset(&bt_sec, 0, sizeof(bt_sec));
    bt_sec.level = BT_SECURITY_LOW;
    ret = setsockopt(l2cap_sock, SOL_BLUETOOTH, BT_SECURITY, &bt_sec, sizeof(bt_sec));
    if (ret != 0)
    {
        perror("Failed to setsockopt!\r\n");
        close(l2cap_sock);
        return NULL;
    }

    ret = listen(l2cap_sock, 10);
    if (ret < 0)
    {
        perror("Fail to listen!\r\n");
        close(l2cap_sock);
        return NULL;
    }

    PR_DEBUG("Started listening on ATT channel. Waiting for connections...\r\n");

    // 进入 accept 循环，每次连接断开后重新 accept
    while (1)
    {
        mainloop_init();

        int l2cap_fd = -1;
        
        memset(&l2cap_clientaddr, 0, sizeof(l2cap_clientaddr));
        l2cap_clientaddr_len = sizeof(l2cap_clientaddr);
        
        PR_DEBUG("Waiting for new connection...\r\n");
        l2cap_fd = accept(l2cap_sock, (struct sockaddr *)&l2cap_clientaddr, &l2cap_clientaddr_len);
        
        if (l2cap_fd < 0)
        {
            perror("Fail to accept!\r\n");
            continue; // 继续等待下一个连接
        }

        ba2str(&l2cap_clientaddr.l2_bdaddr, l2cap_client_bdaddr);
        PR_DEBUG("Connect from : %s, fd: %d\r\n", l2cap_client_bdaddr, l2cap_fd);

        // 清理旧的 att/gatt 对象（如果存在）
        if (gatt_server.gatt)
        {
            bt_gatt_server_unref(gatt_server.gatt);
            gatt_server.gatt = NULL;
        }
        if (gatt_server.att)
        {
            bt_att_unref(gatt_server.att);
            gatt_server.att = NULL;
        }

        gatt_server.att = bt_att_new(l2cap_fd, false);
        if (!gatt_server.att)
        {
            perror("Failed to bt_att_new!\r\n");
            close(l2cap_fd);
            continue;
        }

        if (!bt_att_set_close_on_unref(gatt_server.att, true)) {
            fprintf(stderr, "Failed to set up ATT transport layer\n");
            close(l2cap_fd);
            continue;
        }
        
        bt_att_register_disconnect(gatt_server.att, att_disconnect_cb, NULL, NULL);
        
        gatt_server.gatt = bt_gatt_server_new(gatt_server.db, gatt_server.att, 512, 0);
        if (!gatt_server.gatt)
        {
            perror("Failed to bt_gatt_server_new!\r\n");
            bt_att_unref(gatt_server.att);
            gatt_server.att = NULL;
            close(l2cap_fd);
            continue;
        }
        
        // 触发连接事件回调
        if (__gatt_connect_event)
        {
            __gatt_connect_event(1);
        }

        // 进入 mainloop 处理该连接（阻塞直到 mainloop_quit 被调用）
        PR_DEBUG("Entering mainloop for client %s...\r\n", l2cap_client_bdaddr);
        mainloop_run();
        PR_DEBUG("Mainloop exited, cleaning up...\r\n");

        // mainloop 退出后清理资源
        if (gatt_server.gatt)
        {
            bt_gatt_server_unref(gatt_server.gatt);
            gatt_server.gatt = NULL;
        }
        if (gatt_server.att)
        {
            bt_att_unref(gatt_server.att);
            gatt_server.att = NULL;
        }
        close(l2cap_fd);
        
        PR_DEBUG("Connection closed, ready for next client.\r\n");
    }

    close(l2cap_sock);
    PR_DEBUG("Function l2cap_server_pthread exited!\r\n");
    return NULL;
}

int tuya_gatt_init(void)
{
    int ret = hci_devba(0, &gatt_server.src_addr);
    if (ret < 0)
    {
        perror("Fail to hci_devba!\r\n");
        return -1;
    }

    if( gatt_server.db == NULL )
    {
        gatt_server.db = gatt_db_new();
    }
    
    pthread_t l2cap_thread;
    ret = pthread_create(&l2cap_thread, NULL, l2cap_server_pthread, NULL);
    if (ret != 0)
    {
        perror("Fail to pthread_create!\r\n");
        return ;
    }
    pthread_detach(l2cap_thread);

    return LE_SUCCESS;
}

int tuya_gatt_close(void)
{
    // 这里不需要关闭 socket，因为它会在 accept 循环中被自动关闭
    // 只需要确保 mainloop 能够退出即可
    mainloop_quit();
    return LE_SUCCESS;
}

bt_uuid_t convert_gatt_uuid(gatt_uuid_t uuid)
{
    bt_uuid_t bt_uuid;
    if (uuid.uuid_type == 16)
    {
        bt_uuid.type = BT_UUID16;
        bt_uuid.value.u16 = uuid.uuid.uuid16;
    }
    else if (uuid.uuid_type == 32)
    {
        bt_uuid.type = BT_UUID32;
        bt_uuid.value.u32 = uuid.uuid.uuid32;
    }
    else if (uuid.uuid_type == 128)
    {
        bt_uuid.type = BT_UUID128;
        memcpy(bt_uuid.value.u128.data, uuid.uuid.uuid128, 16);
    }
    return bt_uuid;
}

bt_uuid_t from_tuya_uuid(TKL_BLE_UUID_T *p_uuid)
{
    bt_uuid_t bt_uuid;
    if (p_uuid->uuid_type == TKL_BLE_UUID_TYPE_16) {
        bt_uuid.type = BT_UUID16;
        bt_uuid.value.u16 = p_uuid->uuid.uuid16;
    } else if (p_uuid->uuid_type == TKL_BLE_UUID_TYPE_32) {
        bt_uuid.type = BT_UUID32;
        bt_uuid.value.u32 = p_uuid->uuid.uuid32;
    } else if (p_uuid->uuid_type == TKL_BLE_UUID_TYPE_128) {
        bt_uuid.type = BT_UUID128;
        for(int i=0; i<16; i++)
        {
            bt_uuid.value.u128.data[i] = p_uuid->uuid.uuid128[15-i];
        }
    } else {
        bt_uuid.type = 0; // invalid
    }
    return bt_uuid;
}

void _char_read(struct gatt_db_attribute *attrib,
					unsigned int id, uint16_t offset,
					uint8_t opcode, struct bt_att *att,
					void *user_data)
{
    
}

 void _char_write(struct gatt_db_attribute *attrib,
					unsigned int id, uint16_t offset,
					const uint8_t *value, size_t len,
					uint8_t opcode, struct bt_att *att,
					void *user_data)
{
    uint16_t uuid = 0;
    PR_DEBUG("Characteristic write request: handle=%d, offset=%d, len=%zu", gatt_db_attribute_get_handle(attrib), offset, len);
    PR_DEBUG("recv data len: [%d](", len);
    for (int i = 0; i < len; i++)
    {
        printf("%02X ", value[i]);
    }
    printf("\b)\r\n");
    if (__gatt_write_request_event)
    {
        uint16_t srv_handle;
        gatt_db_attribute_get_service_handles(attrib, &srv_handle, NULL);
        __gatt_write_request_event(srv_handle, gatt_db_attribute_get_handle(attrib), (uint8_t *)value, len);
    }
    gatt_db_attribute_write_result(attrib, id, 0);
}

uint16_t notify_value = 0;
void _notify_read_cb(struct gatt_db_attribute *attrib,
                         unsigned int id, uint16_t offset,
                         uint8_t opcode, struct bt_att *att,
                         void *user_data)
{
    struct gatt_server_t *server = (struct gatt_server_t *)user_data;
    uint value[2];
    value[0] = notify_value & 0xFF;
    value[1] = (notify_value >> 8) & 0xFF;
    gatt_db_attribute_read_result(attrib, id, 0, (uint8_t *)value, sizeof(value));
    PR_DEBUG("CCCD read: handle=%d, value=0x%04x", gatt_db_attribute_get_handle(attrib), notify_value);
}

void _notify_write_cb(struct gatt_db_attribute *attrib,
                          unsigned int id, uint16_t offset,
                          const uint8_t *value, size_t len,
                          uint8_t opcode, struct bt_att *att,
                          void *user_data)
{
    struct gatt_server_t *server = (struct gatt_server_t *)user_data;
    if (len >= 2)
    {
        notify_value = value[0] | (value[1] << 8);
        gatt_db_attribute_write_result(attrib, id, 0);
        PR_DEBUG("CCCD write: handle=%d, value=0x%04x", gatt_db_attribute_get_handle(attrib), notify_value);
    }
}

int tuya_gatt_register_service(TKL_BLE_GATTS_PARAMS_T *p_service)
{
    if( gatt_server.db == NULL )
    {
        gatt_server.db = gatt_db_new();
    }
    if (!gatt_server.db)
    {
        perror("Fail to gatt_db_new!\r\n");
        return -1;
    }
    char uuid_str[64] = {0};
     for(int i = 0; i < p_service->svc_num; i++) {
        bt_uuid_t srv_uuid = from_tuya_uuid(&p_service->p_service[i].svc_uuid);
        bt_uuid_to_string(&srv_uuid, uuid_str, sizeof(uuid_str));
        PR_INFO("register service %d, uuid: %s", i, uuid_str);
        struct gatt_db_attribute *svc = gatt_db_add_service(gatt_server.db, &srv_uuid, true, 8);
        p_service->p_service[i].handle = gatt_db_attribute_get_handle(svc);
        for(int j = 0; j < p_service->p_service[i].char_num; j++) {
            TKL_BLE_CHAR_PARAMS_T *p_char = &p_service->p_service[i].p_char[j];
            bt_uuid_t char_uuid = from_tuya_uuid(&p_char->char_uuid);
            bt_uuid_to_string(&char_uuid, uuid_str, sizeof(uuid_str));
            PR_INFO("  register characteristic %d, uuid: %s", j, uuid_str);
            struct gatt_db_attribute *chr = gatt_db_service_add_characteristic(svc, &char_uuid, p_char->permission, p_char->property, _char_read, _char_write, NULL);
            p_char->handle = gatt_db_attribute_get_handle(chr);
            if(p_char->property & TKL_BLE_GATT_CHAR_PROP_NOTIFY)
            {
                PR_INFO("    add cccd for notify");
                bt_uuid_t uuid;
                uuid.type = BT_UUID16;
                uuid.value.u16 = GATT_CLIENT_CHARAC_CFG_UUID;
                gatt_db_service_add_descriptor(svc, &uuid, BT_ATT_PERM_READ | BT_ATT_PERM_WRITE,
                                   _notify_read_cb, _notify_write_cb, &gatt_server);
            }
        }
        gatt_db_service_set_active(svc, true);
    }
    return 0;
}

int tuya_gatt_register_characteristic(gatt_uuid_t svc_uuid, gatt_uuid_t char_uuid, uint8_t props, uint8_t perm, uint8_t value_len)
{
    return 0;
}

int tuya_gatt_server_send_characteristic_notification(uint16_t handle, uint8_t *data, uint8_t len)
{
    bt_gatt_server_send_notification(gatt_server.gatt, handle, data, len);
    return LE_SUCCESS;
}


void tuya_gatt_register_connect_event(void(*cb)(int status))
{
    __gatt_connect_event = cb;
}

void tuya_gatt_register_write_req_event(void(*cb)(int handle, uint16_t uuid, uint8_t *data, uint8_t len))
{
    __gatt_write_request_event = cb;
}


#elif defined USE_BLUEZ_DBUS_GLIB

#else

static void (*__gatt_connect_event)(int status) = NULL;
static void (*__gatt_write_request_event)(int handle, uint16_t uuid, uint8_t *data, uint8_t len) = NULL;

int tuya_gatt_init(void)
{
    return LE_SUCCESS;
}


int tuya_gatt_register_service(uint16_t uuid)
{
    
    return LE_SUCCESS;
}


int tuya_gatt_server_send_characteristic_notification(uint16_t uuid, uint8_t *data, uint8_t len)
{
    return LE_SUCCESS;
}

int tuya_gatt_register_characteristic(uint16_t svc_uuid, const uint8_t *chr_uuid, uint8_t props, uint16_t desc_uuid, uint8_t desc_props)
{
    return LE_SUCCESS;
}

void tuya_gatt_register_connect_event(void(*cb)(int status))
{
    __gatt_connect_event = cb;
}

void tuya_gatt_register_write_req_event(void(*cb)(int handle, uint16_t uuid, uint8_t *data, uint8_t len))
{
    __gatt_write_request_event = cb;
}

#endif