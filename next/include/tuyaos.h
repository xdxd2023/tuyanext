#pragma once

int tuyaos_init(const char * ty_uuid, const char * ty_authkey, const char * ty_pid, const char* system_version, const char* mcu_version);
bool tuyaos_is_online(void);
int tuyaos_post_init(void);
void tuyaos_ble_on(void);