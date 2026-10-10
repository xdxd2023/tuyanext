
#include "uni_log.h"
#include "tuya_hci.h"
#undef TKL_BLUEZ_BLE_TEST
#define TKL_BLUEZ_BLE_TEST 1
#if defined(TKL_BLUEZ_BLE_TEST) && TKL_BLUEZ_BLE_TEST == 1
#include <string.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

/*
 * Some controllers (e.g. WS73 on this board) only accept the BLE 5.0
 * "extended advertising" HCI commands and reject the legacy ones with
 * 0x0c (Command Disallowed).  We therefore try extended advertising first
 * and transparently fall back to legacy advertising for old controllers.
 *
 * The extended advertising set is configured with the "legacy advertising
 * PDUs" property (0x13 = connectable | scannable | legacy ADV_IND) so that
 * phones / legacy scanners can still discover and connect to us.
 */
#ifndef OCF_LE_SET_EXT_ADVERTISING_PARAMETERS
#define OCF_LE_SET_EXT_ADVERTISING_PARAMETERS   0x0036
#endif
#ifndef OCF_LE_SET_EXT_ADVERTISING_DATA
#define OCF_LE_SET_EXT_ADVERTISING_DATA         0x0037
#endif
#ifndef OCF_LE_SET_EXT_SCAN_RESPONSE_DATA
#define OCF_LE_SET_EXT_SCAN_RESPONSE_DATA       0x0038
#endif
#ifndef OCF_LE_SET_EXT_ADVERTISING_ENABLE
#define OCF_LE_SET_EXT_ADVERTISING_ENABLE       0x0039
#endif

#define EXT_ADV_HANDLE          0x00
#define EXT_ADV_PROP_LEGACY     (0x0001 | 0x0002 | 0x0010) /* connectable | scannable | legacy PDU */
#define HCI_ERR_COMMAND_DISALLOWED  0x0c

typedef struct {
    uint8_t  handle;
    uint16_t properties;
    uint8_t  min_interval[3];
    uint8_t  max_interval[3];
    uint8_t  channel_map;
    uint8_t  own_addr_type;
    uint8_t  peer_addr_type;
    uint8_t  peer_addr[6];
    uint8_t  filter_policy;
    int8_t   tx_power;
    uint8_t  primary_phy;
    uint8_t  secondary_max_skip;
    uint8_t  secondary_phy;
    uint8_t  sid;
    uint8_t  scan_req_notification_enable;
} __attribute__((packed)) le_set_ext_adv_params_cp_t;

typedef struct {
    uint8_t handle;
    uint8_t operation;
    uint8_t fragment_preference;
    uint8_t data_len;
    uint8_t data[251];
} __attribute__((packed)) le_set_ext_adv_data_cp_t;

typedef struct {
    uint8_t  enable;
    uint8_t  num_sets;
    uint8_t  handle;
    uint16_t duration;
    uint8_t  max_events;
} __attribute__((packed)) le_set_ext_adv_enable_cp_t;

/* -1: not decided yet, 1: extended advertising, 0: legacy advertising */
static int g_adv_mode = -1;

static int hci_run_cmd(uint16_t ocf, void *cparam, int clen, uint8_t *status, int rlen)
{
    int device = hci_open_dev(hci_get_route(NULL));
    if (device < 0) {
        return -1;
    }

    struct hci_request req;
    memset(&req, 0, sizeof(req));
    req.ogf = OGF_LE_CTL;
    req.ocf = ocf;
    req.cparam = cparam;
    req.clen = clen;
    req.rparam = status;
    req.rlen = rlen;

    int ret = hci_send_req(device, &req, 1000);
    hci_close_dev(device);
    return ret;
}

static void set_interval_3byte(uint8_t *out, uint16_t interval)
{
    out[0] = (uint8_t)(interval & 0xff);
    out[1] = (uint8_t)((interval >> 8) & 0xff);
    out[2] = 0x00;
}

static int ext_set_adv_params(uint16_t min_interval, uint16_t max_interval)
{
    le_set_ext_adv_params_cp_t cp;
    uint8_t status[2] = {0};

    memset(&cp, 0, sizeof(cp));
    cp.handle = EXT_ADV_HANDLE;
    cp.properties = htobs(EXT_ADV_PROP_LEGACY);
    set_interval_3byte(cp.min_interval, min_interval);
    set_interval_3byte(cp.max_interval, max_interval);
    cp.channel_map = 0x07;
    cp.own_addr_type = 0x00;
    cp.peer_addr_type = 0x00;
    cp.filter_policy = 0x00;
    cp.tx_power = 0x7f;               /* host has no preference */
    cp.primary_phy = 0x01;            /* LE 1M */
    cp.secondary_max_skip = 0x00;
    cp.secondary_phy = 0x01;          /* LE 1M */
    cp.sid = 0x00;
    cp.scan_req_notification_enable = 0x00;

    if (hci_run_cmd(OCF_LE_SET_EXT_ADVERTISING_PARAMETERS, &cp, sizeof(cp), status, 2) != 0) {
        return -1;
    }
    PR_DEBUG("Set Extended Advertisement Parameters, status 0x%02x, tx_power %d",
             status[0], (int8_t)status[1]);
    return (status[0] == 0x00) ? 0 : status[0];
}

static int ext_set_adv_data(const uint8_t *data, uint8_t len)
{
    le_set_ext_adv_data_cp_t cp;
    uint8_t status = 0;

    if (len > sizeof(cp.data)) {
        len = sizeof(cp.data);
    }
    memset(&cp, 0, sizeof(cp));
    cp.handle = EXT_ADV_HANDLE;
    cp.operation = 0x03;              /* complete advertising data */
    cp.fragment_preference = 0x01;    /* minimize fragmentation */
    cp.data_len = len;
    memcpy(cp.data, data, len);

    if (hci_run_cmd(OCF_LE_SET_EXT_ADVERTISING_DATA, &cp, 4 + len, &status, 1) != 0) {
        return -1;
    }
    PR_DEBUG("Set Extended Advertisement Data, status 0x%02x", status);
    return (status == 0x00) ? 0 : status;
}

static int ext_set_scan_rsp_data(const uint8_t *data, uint8_t len)
{
    le_set_ext_adv_data_cp_t cp;
    uint8_t status = 0;

    if (len > sizeof(cp.data)) {
        len = sizeof(cp.data);
    }
    memset(&cp, 0, sizeof(cp));
    cp.handle = EXT_ADV_HANDLE;
    cp.operation = 0x03;              /* complete scan response data */
    cp.fragment_preference = 0x01;
    cp.data_len = len;
    memcpy(cp.data, data, len);

    if (hci_run_cmd(OCF_LE_SET_EXT_SCAN_RESPONSE_DATA, &cp, 4 + len, &status, 1) != 0) {
        return -1;
    }
    PR_DEBUG("Set Extended Scan Response Data, status 0x%02x", status);
    return (status == 0x00) ? 0 : status;
}

static int ext_set_adv_enable(bool enable)
{
    le_set_ext_adv_enable_cp_t cp;
    uint8_t status = 0;

    memset(&cp, 0, sizeof(cp));
    cp.enable = enable ? 0x01 : 0x00;
    cp.num_sets = 0x01;
    cp.handle = EXT_ADV_HANDLE;
    cp.duration = 0x0000;
    cp.max_events = 0x00;

    if (hci_run_cmd(OCF_LE_SET_EXT_ADVERTISING_ENABLE, &cp, sizeof(cp), &status, 1) != 0) {
        return -1;
    }
    PR_DEBUG("Set Extended Advertisement %s, status 0x%02x", enable ? "Enable" : "Disable", status);
    return (status == 0x00) ? 0 : status;
}

int tuya_hci_le_set_adv_params(uint16_t min_interval, uint16_t max_interval, uint8_t advtype)
{
    /* The controller may still be advertising from a previous (crashed)
     * instance.  LE Set Advertising Parameters is disallowed while a set is
     * enabled, so always try to disable advertising first. */
    if (g_adv_mode != 0) {
        int ret = ext_set_adv_params(min_interval, max_interval);
        if (ret == HCI_ERR_COMMAND_DISALLOWED) {
            /* Advertising may have been left enabled by a previous (crashed)
             * instance.  Disable it and retry once before giving up. */
            ext_set_adv_enable(false);
            ret = ext_set_adv_params(min_interval, max_interval);
        }
        if (ret == 0) {
            g_adv_mode = 1;
            return LE_SUCCESS;
        }
        /* only fall back when the controller explicitly rejects the command */
        if (ret != HCI_ERR_COMMAND_DISALLOWED) {
            return LE_READ_ERROR;
        }
        PR_DEBUG("extended adv params rejected, fallback to legacy advertising");
        g_adv_mode = 0;
    }

    /* legacy advertising fallback */
    int device = hci_open_dev(hci_get_route(NULL));
    if (device < 0) {
        return LE_OPEN_ERROR;
    }

    uint8_t status = 0;
    le_set_advertising_parameters_cp adv_params_cp;
    memset(&adv_params_cp, 0, sizeof(adv_params_cp));
    adv_params_cp.advtype = advtype;
    adv_params_cp.min_interval = htobs(min_interval);
    adv_params_cp.max_interval = htobs(max_interval);
    adv_params_cp.chan_map = 7;

    struct hci_request req;
    memset(&req, 0, sizeof(req));
    req.ogf = OGF_LE_CTL;
    req.ocf = OCF_LE_SET_ADVERTISING_PARAMETERS;
    req.cparam = &adv_params_cp;
    req.clen = LE_SET_ADVERTISING_PARAMETERS_CP_SIZE;
    req.rparam = &status;
    req.rlen = 1;

    if (hci_send_req(device, &req, 1000) != 0) {
        hci_close_dev(device);
        return LE_READ_ERROR;
    }
    PR_DEBUG("Set Advertisement Parameters, status 0x%02x", status);
    hci_close_dev(device);
    return LE_SUCCESS;
}

int tuya_hci_le_set_adv_enable(bool enable)
{
    if (g_adv_mode == 1) {
        return (ext_set_adv_enable(enable) == 0) ? LE_SUCCESS : LE_READ_ERROR;
    }

    int device = hci_open_dev(hci_get_route(NULL));
    if (device < 0) {
        return LE_OPEN_ERROR;
    }

    uint8_t status = 0;
    le_set_advertise_enable_cp advertise_cp;
    memset(&advertise_cp, 0, sizeof(advertise_cp));
    advertise_cp.enable = enable;

    struct hci_request req;
    memset(&req, 0, sizeof(req));
    req.ogf = OGF_LE_CTL;
    req.ocf = OCF_LE_SET_ADVERTISE_ENABLE;
    req.cparam = &advertise_cp;
    req.clen = LE_SET_ADVERTISE_ENABLE_CP_SIZE;
    req.rparam = &status;
    req.rlen = 1;

    if (hci_send_req(device, &req, 1000) != 0) {
        hci_close_dev(device);
        return LE_READ_ERROR;
    }
    PR_DEBUG("Set Advertisement %s, status 0x%02x", enable ? "Enable" : "Disable", status);
    hci_close_dev(device);
    return LE_SUCCESS;
}

int tuya_hci_le_set_adv_data(uint8_t *data, uint8_t len)
{
    if ((data == NULL) || (len == 0)) {
        return LE_INVALID_PARAM;
    }

    if (g_adv_mode != 0) {
        int ret = ext_set_adv_data(data, len);
        if (ret == 0) {
            g_adv_mode = 1;
            return LE_SUCCESS;
        }
        if (ret != HCI_ERR_COMMAND_DISALLOWED) {
            return LE_READ_ERROR;
        }
        g_adv_mode = 0;
    }

    int device = hci_open_dev(hci_get_route(NULL));
    if (device < 0) {
        return LE_OPEN_ERROR;
    }

    uint8_t status = 0;
    le_set_advertising_data_cp adv_data_cp;
    memset(&adv_data_cp, 0, sizeof(adv_data_cp));
    memcpy(adv_data_cp.data, data, len);
    adv_data_cp.length = len;

    struct hci_request req;
    memset(&req, 0, sizeof(req));
    req.ogf = OGF_LE_CTL;
    req.ocf = OCF_LE_SET_ADVERTISING_DATA;
    req.cparam = &adv_data_cp;
    req.clen = LE_SET_ADVERTISING_DATA_CP_SIZE;
    req.rparam = &status;
    req.rlen = 1;

    if (hci_send_req(device, &req, 1000) != 0) {
        hci_close_dev(device);
        return LE_READ_ERROR;
    }
    PR_DEBUG("Set Advertisement Data, status 0x%02x", status);
    hci_close_dev(device);
    return LE_SUCCESS;
}

int tuya_hci_le_set_scan_rsp_data(uint8_t *data, uint8_t len)
{
    if ((data == NULL) || (len == 0)) {
        return LE_INVALID_PARAM;
    }

    if (g_adv_mode != 0) {
        int ret = ext_set_scan_rsp_data(data, len);
        if (ret == 0) {
            g_adv_mode = 1;
            return LE_SUCCESS;
        }
        if (ret != HCI_ERR_COMMAND_DISALLOWED) {
            return LE_READ_ERROR;
        }
        g_adv_mode = 0;
    }

    int device = hci_open_dev(hci_get_route(NULL));
    if (device < 0) {
        return LE_OPEN_ERROR;
    }

    uint8_t status = 0;
    le_set_scan_response_data_cp scan_rsp_data_cp;
    memset(&scan_rsp_data_cp, 0, sizeof(scan_rsp_data_cp));
    memcpy(scan_rsp_data_cp.data, data, len);
    scan_rsp_data_cp.length = len;

    struct hci_request req;
    memset(&req, 0, sizeof(req));
    req.ogf = OGF_LE_CTL;
    req.ocf = OCF_LE_SET_SCAN_RESPONSE_DATA;
    req.cparam = &scan_rsp_data_cp;
    req.clen = LE_SET_SCAN_RESPONSE_DATA_CP_SIZE;
    req.rparam = &status;
    req.rlen = 1;

    if (hci_send_req(device, &req, 1000) != 0) {
        hci_close_dev(device);
        return LE_READ_ERROR;
    }
    PR_DEBUG("Set Scan Response Data, status 0x%02x", status);
    hci_close_dev(device);
    return LE_SUCCESS;
}
#endif
