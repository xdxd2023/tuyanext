
#include <pthread.h>
#include "uni_log.h"
#include "tuya_bluez_api.h"
#include "tuya_hci.h"
#include "tuya_gatt.h"
#include "bluetooth.h"
#include "l2cap.h"
#include "mainloop.h"
#include "uuid.h"
#include "gatt-db.h"
#undef TKL_BLUEZ_BLE_TEST
#define TKL_BLUEZ_BLE_TEST 1
#if defined(TKL_BLUEZ_BLE_TEST) && TKL_BLUEZ_BLE_TEST == 1

static int g_bluez_inited = FALSE;

static void *__loop_run(void *arg)
{
    return NULL;
}

int tuya_bluez_init(void)
{
    int ret = 0;
    pthread_t tid;

    if (g_bluez_inited) {
        PR_WARN("tuya bluez had been initialized");
        return LE_SUCCESS;
    }

    //mainloop_init();

    ret = tuya_gatt_init();
    if (ret != 0) {
        PR_ERR("tuya_gatt_init error");
        return ret;
    }

    // ret = hci_devba(hci_get_route(NULL), &gatt_server.src_addr);
    // if (ret < 0)
    // {
    //     perror("Fail to hci_devba!\r\n");
    //     return LE_COM_ERROR;
    // }

    // gatt_server.db = gatt_db_new();
    // if (!gatt_server.db)
    // {
    //     perror("Fail to gatt_db_new!\r\n");
    //     return LE_COM_ERROR;
    // }

    // pthread_create(&l2cap_server_thread, NULL, l2cap_server_pthread, NULL);
    // pthread_detach(l2cap_server_thread);
    
    g_bluez_inited = TRUE;

    return LE_SUCCESS;
}

int tuya_bluez_deinit(void)
{
    PR_DEBUG("tuya_bluez_deinit -------------------");
    tuya_gatt_close();
    g_bluez_inited = FALSE;
    return 0;
}

int tuya_bluez_le_set_adv_params(le_set_adv_params_t *params)
{
    if (params == NULL) {
        return LE_INVALID_PARAM;
    }
    PR_DEBUG("hci_le_set_advertise_parameters min_interval: %d, max_interval: %d, advtype: %d",
             params->min_interval, params->max_interval, params->advtype);
    return tuya_hci_le_set_adv_params(params->min_interval, params->max_interval, params->advtype);
}

int tuya_bluez_le_set_adv_enable(bool enable)
{
    return tuya_hci_le_set_adv_enable(enable);
}

int tuya_bluez_le_set_adv_data(uint8_t *data, uint8_t len)
{
    if ((data == NULL) || (len == 0)) {
        return LE_INVALID_PARAM;
    }

    return tuya_hci_le_set_adv_data(data, len);
}

int tuya_bluez_le_set_scan_rsp_data(uint8_t *data, uint8_t len)
{
    if ((data == NULL) || (len == 0)) {
        return LE_INVALID_PARAM;
    }

    return tuya_hci_le_set_scan_rsp_data(data, len);
}

int tuya_bluez_le_add_gatt_service(TKL_BLE_GATTS_PARAMS_T *p_service)
{
    PR_DEBUG("register gatt service, num: %d", p_service->svc_num);

    if ((p_service == NULL) || (p_service->svc_num == 0)) {
        return LE_INVALID_PARAM;
    }
    tuya_gatt_register_service(p_service);
    return LE_SUCCESS;
}

int tuya_bluez_le_gatts_value_notify(uint16_t uuid, uint8_t *value, uint16_t len)
{
    return tuya_gatt_server_send_characteristic_notification(uuid, value, len);
}

void tuya_bluez_le_register_connect_event(void(*cb)(int status))
{
    tuya_gatt_register_connect_event(cb);
}

/**
 * @brief Register write request event callback
 */
void tuya_bluez_le_register_write_req_event(void(*cb)(int handle, uint16_t uuid, uint8_t *data, uint8_t len))
{
    tuya_gatt_register_write_req_event(cb);
}
#endif