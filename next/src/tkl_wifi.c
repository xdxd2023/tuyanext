#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <stdint.h>
#include <stdbool.h>

#include "tkl_wifi.h"
//#include "tkl_wifi_log.h"
//#include "tkl_gen_net.h"

// Ensure all functions in this file are strong symbols (override SDK defaults)
#ifdef TUYA_WEAK_ATTRIBUTE
#undef TUYA_WEAK_ATTRIBUTE
#endif
#define TUYA_WEAK_ATTRIBUTE

#define TKL_LOGD printf 
#define TKL_LOGE printf

static WIFI_EVENT_CB _wifi_status_cb = NULL; /* wifi 工作事件上报回调 */


void wifi_status_event_cb(WF_EVENT_E event, VOID_T *arg)
{
    if (_wifi_status_cb) {
        _wifi_status_cb(event, arg);
    }
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_init(WIFI_EVENT_CB cb)
{
    _wifi_status_cb = cb;
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_deinit(VOID_T)
{
    return OPRT_OK;	
}


TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_scan_ap(CONST SCHAR_T *ssid, AP_IF_S **ap_ary, UINT_T *num)
{
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
    TKL_LOGD("#####: tkl_wifi_scan_ap ssid %s\n", ssid != NULL ? ssid : "NULL");
    if (ap_ary == NULL || num == NULL) {
        TKL_LOGE("Invalid parameters");
        return OPRT_INVALID_PARM;
    }
    
    *ap_ary = NULL;
    *num = 0;
    
    FILE *fp;
    char line[512];
    AP_IF_S *ap_list = NULL;
    int ap_count = 0;
    int ap_capacity = 10; // Initial capacity
    
    // Allocate initial memory for AP list
    ap_list = (AP_IF_S *)malloc(ap_capacity * sizeof(AP_IF_S));
    if (ap_list == NULL) {
        TKL_LOGE("Failed to allocate memory for AP list");
        return OPRT_MALLOC_FAILED;
    }
    
    // Trigger scan and get results
    fp = popen("iw dev wlan0 scan 2>/dev/null", "r");
    if (fp == NULL) {
        TKL_LOGE("Failed to run iw scan command");
        free(ap_list);
        return OPRT_COM_ERROR;
    }
    
    AP_IF_S current_ap;
    memset(&current_ap, 0, sizeof(AP_IF_S));
    bool has_bss = false;
    
    while (fgets(line, sizeof(line), fp) != NULL) {
        char *ptr = line;
        
        // Skip leading whitespace
        while (*ptr == ' ' || *ptr == '\t') ptr++;
        
        // New BSS entry
        if (strncmp(ptr, "BSS ", 4) == 0) {
            // Save previous AP if valid
            if (has_bss && strlen(current_ap.ssid) > 0) {
                // Check if we need to expand the array
                if (ap_count >= ap_capacity) {
                    ap_capacity *= 2;
                    AP_IF_S *new_list = (AP_IF_S *)realloc(ap_list, ap_capacity * sizeof(AP_IF_S));
                    if (new_list == NULL) {
                        TKL_LOGE("Failed to reallocate memory for AP list");
                        free(ap_list);
                        pclose(fp);
                        return OPRT_MALLOC_FAILED;
                    }
                    ap_list = new_list;
                }
                
                // Filter by SSID if specified
                if (ssid == NULL || strcmp(current_ap.ssid, ssid) == 0) {
                    memcpy(&ap_list[ap_count], &current_ap, sizeof(AP_IF_S));
                    ap_count++;
                }
            }
            
            // Reset for new AP
            memset(&current_ap, 0, sizeof(AP_IF_S));
            has_bss = true;
            
            // Parse BSSID
            ptr += 4; // Skip "BSS "
            int values[6];
            if (sscanf(ptr, "%x:%x:%x:%x:%x:%x", 
                       &values[0], &values[1], &values[2], 
                       &values[3], &values[4], &values[5]) == 6) {
                for (int i = 0; i < 6; i++) {
                    current_ap.bssid[i] = (UCHAR_T)values[i];
                }
            }
        }
        // Parse SSID
        else if (strstr(ptr, "SSID:") != NULL) {
            char *ssid_ptr = strstr(ptr, "SSID:");
            ssid_ptr += 5; // Skip "SSID:"
            while (*ssid_ptr == ' ' || *ssid_ptr == '\t') ssid_ptr++;
            
            // Remove newline
            char *newline = strchr(ssid_ptr, '\n');
            if (newline) *newline = '\0';
            
            // Handle hex-encoded SSID (check if contains any \x patterns)
            
            if (strstr(ssid_ptr, "\\x") != NULL) {
                int i = 0, j = 0;
                while (ssid_ptr[i] != '\0' && j < sizeof(current_ap.ssid) - 1) {
                    if (ssid_ptr[i] == '\\' && ssid_ptr[i+1] == 'x' && 
                        ssid_ptr[i+2] != '\0' && ssid_ptr[i+3] != '\0') {
                        char hex[3] = {ssid_ptr[i+2], ssid_ptr[i+3], '\0'};
                        current_ap.ssid[j++] = (char)strtol(hex, NULL, 16);
                        i += 4;
                    } else {
                        current_ap.ssid[j++] = ssid_ptr[i++];
                    }
                }
                current_ap.ssid[j] = '\0';
            } else {
                strncpy(current_ap.ssid, ssid_ptr, sizeof(current_ap.ssid) - 1);
                current_ap.ssid[sizeof(current_ap.ssid) - 1] = '\0';
            }
            TKL_LOGD("Parsed SSID: %s\n", current_ap.ssid);
        }
        // Parse frequency/channel
        else if (strstr(ptr, "freq:") != NULL) {
            char *freq_ptr = strstr(ptr, "freq:");
            freq_ptr += 5;
            while (*freq_ptr == ' ' || *freq_ptr == '\t') freq_ptr++;
            
            int freq = atoi(freq_ptr);
            if (freq >= 2412 && freq <= 2484) {
                // 2.4GHz band
                if (freq == 2484) {
                    current_ap.channel = 14;
                } else {
                    current_ap.channel = (freq - 2412) / 5 + 1;
                }
            } else if (freq >= 5170 && freq <= 5825) {
                // 5GHz band
                current_ap.channel = (freq - 5000) / 5;
            }
        }
        // Parse signal strength
        else if (strstr(ptr, "signal:") != NULL) {
            char *signal_ptr = strstr(ptr, "signal:");
            signal_ptr += 7;
            while (*signal_ptr == ' ' || *signal_ptr == '\t') signal_ptr++;
            
            current_ap.rssi = (SCHAR_T)atoi(signal_ptr);
        }
        // Parse security (WPA/WPA2/WEP)
        else if (strstr(ptr, "Privacy") != NULL) {
            current_ap.security = WAAM_WEP;
        }
        else if (strstr(ptr, "WPA2") != NULL) {
            current_ap.security = WAAM_WPA2_PSK;
        }
        else if (strstr(ptr, "WPA:") != NULL) {
            current_ap.security = WAAM_WPA_PSK;
        }
    }
    
    // Save last AP if valid
    if (has_bss && strlen(current_ap.ssid) > 0) {
        if (ap_count >= ap_capacity) {
            ap_capacity++;
            AP_IF_S *new_list = (AP_IF_S *)realloc(ap_list, ap_capacity * sizeof(AP_IF_S));
            if (new_list == NULL) {
                TKL_LOGE("Failed to reallocate memory for AP list");
                free(ap_list);
                pclose(fp);
                return OPRT_MALLOC_FAILED;
            }
            ap_list = new_list;
        }
        
        if (ssid == NULL || strcmp(current_ap.ssid, ssid) == 0) {
            memcpy(&ap_list[ap_count], &current_ap, sizeof(AP_IF_S));
            ap_count++;
        }
    }
    
    pclose(fp);
    
    if (ap_count == 0) {
        free(ap_list);
        *ap_ary = NULL;
        *num = 0;
    } else {
        // Resize to exact size
        AP_IF_S *final_list = (AP_IF_S *)realloc(ap_list, ap_count * sizeof(AP_IF_S));
        if (final_list != NULL) {
            ap_list = final_list;
        }
        
        *ap_ary = ap_list;
        *num = ap_count;
        
        TKL_LOGD("Found %d access points", ap_count);
    }
    
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_release_ap(AP_IF_S *ap)
{
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_set_cur_channel(CONST UCHAR_T chan)
{
//     int iret = 0;

// #ifdef DEBUG_VERSION
//     TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
// #endif
//     iret = tkl_gen_net_set_cur_channel(chan);
//     if(iret < 0) {
//         TKL_LOGE("set channel failed");
//         return OPRT_COM_ERROR;
//     }

    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_get_cur_channel(UCHAR_T *chan)
{
//     int iret = 0;
// #ifdef DEBUG_VERSION
//     TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
// #endif
//     iret = tkl_gen_net_get_cur_channel(chan);
//     if(iret < 0) {
//         TKL_LOGE("get channel failed");
//         return OPRT_COM_ERROR;
//     }

    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_set_sniffer(CONST BOOL_T en, CONST SNIFFER_CALLBACK cb)
{
//     int iret = 0;

// #ifdef DEBUG_VERSION
//     TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
// #endif
//     iret = tkl_gen_net_set_sniffer((int)en,(const SNIFFER_CB)cb);
//     if(iret < 0) {
//         TKL_LOGE("set wifi sniffer failed");
//         return OPRT_COM_ERROR;
//     }

    return OPRT_OK;
}

static OPERATE_RET wifi_get_ipv4(CONST WF_IF_E wf, NW_IP_S *ip)
{
    FILE *fp;
    char line[256], *ptr;
    char cmd[64];
    
    // This platform runs both station and provisioning AP on wlan0, so always
    // query wlan0.  (The previous wlan1 branch was for a VAP_COUNT=3 design
    // that this WS73 driver does not support.)
    snprintf(cmd, sizeof(cmd), "ifconfig wlan0");
    
    // Execute ifconfig command
    fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Failed to run ifconfig command\n");
        return OPRT_COM_ERROR;
    }
    // Parse output to find IP address. Support both the old net-tools format
    // ("inet addr:1.2.3.4") and the newer one ("inet 1.2.3.4").
    while (fgets(line, sizeof(line), fp) != NULL) {
        if ((ptr = strstr(line, "inet ")) != NULL) {
            ptr += 5; // Skip "inet "
            if (strncmp(ptr, "addr:", 5) == 0) {
                ptr += 5; // Skip "addr:"
            }
            // Find the end of IP address (usually space or newline)
            char *end = ptr;
            while (*end && *end != ' ' && *end != '\n') {
                end++;
            }
            *end = '\0';
            
            // Copy the IP address to the output parameter
            if (ip != NULL) {
            #ifdef RK3326_TUYAOS
                snprintf(ip->ip, sizeof(ip->ip), "%s", ptr);
            #else
                snprintf(ip->nwipstr, sizeof(ip->nwipstr), "%s", ptr);
            #endif
                //ip->type = TY_AF_INET;
            }
            break;
        }
    }
    
    pclose(fp);

    return OPRT_OK;
}

static OPERATE_RET wifi_get_ipv6(CONST WF_IF_E wf, int type, NW_IP_S *ip)
{
    return OPRT_OK;
}

#ifndef RK3326_TUYAOS
/**
 * @brief get wifi ipv6 info.when wifi works in
 *        ap+station mode, wifi has two ips.
 * 
 * @param[in]       wf          wifi function type
 * @param[out]      ip          the ip addr info
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_get_ipv6(CONST WF_IF_E wf, NW_IP_TYPE type, NW_IP_S *ip)
{
    return OPRT_NOT_SUPPORTED;	
}
#endif

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_get_ip(CONST WF_IF_E wf, NW_IP_S *ip)
{
    OPERATE_RET ret = OPRT_COM_ERROR;
    if (ip == NULL) {
        printf( "%s %d: invalid param\n", __func__, __LINE__ );
        return OPRT_INVALID_PARM;
    }

    // if ( TY_AF_INET == ip->type ) 
    {
        ret = wifi_get_ipv4( wf, ip );
    }
    // else if ( TY_AF_INET6 == ip->type ) {
    //     ret = wifi_get_ipv6( wf, NW_IPV6, ip );
    // }
    // else {
    //     printf( "%s %d: invalid param type=%d\n", __func__, __LINE__, ip->type );
    // }

    return ret;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_set_mac(CONST WF_IF_E wf, CONST NW_MAC_S *mac)
{
    int iret = 0;
    // not implemented in this version
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_get_mac(CONST WF_IF_E wf, NW_MAC_S *mac)
{
    if (mac == NULL) {
        TKL_LOGE("Invalid parameter: mac is NULL");
        return OPRT_INVALID_PARM;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        TKL_LOGE("Failed to open socket");
        return OPRT_COM_ERROR;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));

    // Set interface name based on the wifi function type
    if (wf == WF_STATION) {
        strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ-1);
    } else {
        strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ-1);
    }

    // Get hardware address
    if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
        TKL_LOGE("Failed to get MAC address");
        close(sock);
        return OPRT_COM_ERROR;
    }

    // Copy MAC address to output parameter
    memcpy(mac->mac, ifr.ifr_hwaddr.sa_data, 6);
    TKL_LOGD("MAC address: %02x:%02x:%02x:%02x:%02x:%02x",
        mac->mac[0], mac->mac[1], mac->mac[2], 
        mac->mac[3], mac->mac[4], mac->mac[5]);

    close(sock);
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_set_work_mode(CONST WF_WK_MD_E mode)
{
    int iret = 0;

// #ifdef DEBUG_VERSION
//     TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
// #endif
//     iret = tkl_gen_set_work_mode(mode);
//     if(iret < 0) {
//         TKL_LOGE("WIFI SET work mode failed");
//         return OPRT_COM_ERROR;
//     } else {
//         switch (mode) {
//             case WWM_POWERDOWN:
//                 TKL_LOGD("WIFI SET work mode success, set mode :wifi power down");
//                 break;
//             case WWM_SNIFFER:
//                 TKL_LOGD("WIFI SET work mode success, set mode :sniffer mode");
//                 break;
//             case WWM_STATION:
//                 TKL_LOGD("WIFI SET work mode success, set mode :station mode");
//                 break;
//             case WWM_SOFTAP:
//                 TKL_LOGD("WIFI SET work mode success, set mode :softap mode");
//                 break;
//             case WWM_STATIONAP:
//                 TKL_LOGD("WIFI SET work mode success, set mode :station on ap mode");
//                 break;
//             default:
//                 break;
//         }
//     }

    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_get_work_mode(WF_WK_MD_E *mode)
{
    int iret = 0;

    // Default to station mode if we can't determine
    *mode = WWM_STATION;
    
    FILE *fp;
    char line[256];
    
    // Use iw dev to check interface mode
    fp = popen("iw dev wlan0 info | grep type", "r");
    if (fp != NULL) {
        if (fgets(line, sizeof(line), fp) != NULL) {
            TKL_LOGD("iw interface type: %s", line);
            
            if (strstr(line, "AP")) {
                // We're in AP mode, but need to check if we're also in station mode
                *mode = WWM_SOFTAP;
                pclose(fp);
                
                // Check if we're also connected as a station
                fp = popen("iw dev wlan0 station dump", "r");
                if (fp != NULL) {
                    if (fgets(line, sizeof(line), fp) != NULL) {
                        // If there are connected stations and we're connected to another AP
                        // we are in station+AP mode
                        *mode = WWM_STATIONAP;
                    }
                    pclose(fp);
                }
                
                return OPRT_OK;
            } else if (strstr(line, "managed")) {
                // We're in station mode, check if connected
                pclose(fp);
                
                fp = popen("iw dev wlan0 link", "r");
                if (fp != NULL) {
                    if (fgets(line, sizeof(line), fp) != NULL && !strstr(line, "Not connected")) {
                        *mode = WWM_STATION;
                    }
                    pclose(fp);
                }
                
                return OPRT_OK;
            } else if (strstr(line, "monitor")) {
                *mode = WWM_SNIFFER;
                pclose(fp);
                return OPRT_OK;
            }
        }
        pclose(fp);
    }
    
    TKL_LOGD("Current WiFi mode: %d", *mode);
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_start_ap(CONST WF_AP_CFG_IF_S *cfg)
{
    int iret = 0;
#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    if(cfg == NULL){
        TKL_LOGE("invalid parm");
        return OPRT_INVALID_PARM;
    }
#ifdef RK3326_TUYAOS
    TKL_LOGD("Start AP SSID:%s ,CHANNEL:%d, ip:%s", cfg->ssid,cfg->chan, cfg->ip.ip);
#else
    TKL_LOGD("Start AP SSID:%s ,CHANNEL:%d, ip:%s", cfg->ssid,cfg->chan, cfg->ip.nwipstr);
#endif
#if 0
    iret = tkl_gen_ap_start(cfg);
#else
    char cmd[128] = {0};
    /* Quote ssid/password so that an empty password does not collapse and
     * shift the channel into the wrong argument. */
    snprintf(cmd, sizeof(cmd), "./hostap.sh \"%s\" \"%s\" %d",
                cfg->ssid, cfg->passwd, cfg->chan);
    iret = system(cmd);
#endif
    if(iret != 0){
        TKL_LOGE(" WIFI start ap failed, iret = %d\n", iret);
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_stop_ap(VOID_T)
{
    int iret = 0;

    /* Stop the provisioning AP started by tkl_wifi_start_ap. This also
     * restores the STA configuration so the subsequent station connect can
     * use the wireless interface again. */
    iret = system("./hostap.sh stop");
    sleep(2);
    if (iret != 0) {
        TKL_LOGE(" WIFI stop ap failed, iret = %d\n", iret);
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_get_connected_ap_info(FAST_WF_CONNECTED_AP_INFO_T **fast_ap_info)
{
    return OPRT_NOT_SUPPORTED;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_station_fast_connect(CONST FAST_WF_CONNECTED_AP_INFO_T *fast_ap_info)
{
    return OPRT_OK;
}

int _wifi_connect_state = -1;

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_station_connect(CONST SCHAR_T *ssid, CONST SCHAR_T *passwd)
{
#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
#if 0
    iret = tkl_gen_station_connect(ssid,passwd);
#else
    char cmd[128] = {0};
    if (ssid == NULL || passwd == NULL) {
        TKL_LOGE("ssid or passwd is null");
        return OPRT_INVALID_PARM;
    }
    snprintf(cmd, sizeof(cmd), "./wific.sh \"%s\" \"%s\"", ssid, passwd);
    _wifi_connect_state = system(cmd);
#endif
    switch(_wifi_connect_state)
    {
        case 0:
            TKL_LOGD("WIFI Station connect ap success");
            break;
        case 3:
            TKL_LOGE("WIFI Station connect ap failed: no ap found");
            wifi_status_event_cb(WFE_CONNECT_FAILED, NULL);
            return OPRT_ROUTER_NOT_FOUND;
            break;
        case 5:
            TKL_LOGE("WIFI Station connect ap failed: passwd wrong");
            wifi_status_event_cb(WFE_CONNECT_FAILED, NULL);
            return OPRT_AUTHENTICATION_FAIL;
            break;
        default:
            TKL_LOGE("WIFI Station connect ap failed: unknown reason");
            wifi_status_event_cb(WFE_CONNECT_FAILED, NULL);
            return OPRT_COM_ERROR;
            break;
    }
    wifi_status_event_cb(WFE_CONNECTED, NULL);
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_station_disconnect(VOID_T)
{
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_station_get_conn_ap_rssi(SCHAR_T *rssi)
{
    if (!rssi) {
        TKL_LOGE("rssi is null");
        return OPRT_INVALID_PARM;
    }

    *rssi = 0;
    
    FILE *fp;
    char line[256];
    char *ptr;
    
    // 使用 iw 命令获取连接信息
    fp = popen("iw dev wlan0 link", "r");
    if (fp == NULL) {
        TKL_LOGE("Failed to run iw command");
        return OPRT_COM_ERROR;
    }
    
    // 解析输出查找 signal 信息
    while (fgets(line, sizeof(line), fp) != NULL) {
        // 查找 "signal: -XX dBm" 格式
        if ((ptr = strstr(line, "signal:")) != NULL) {
            ptr += 7; // 跳过 "signal:" 
            while (*ptr == ' ') ptr++; // 跳过空格
            
            int signal_value = atoi(ptr);
            *rssi = (SCHAR_T)signal_value;
            
            TKL_LOGD("RSSI: %d dBm", *rssi);
            pclose(fp);
            return OPRT_OK;
        }
    }
    
    pclose(fp);
    TKL_LOGE("Failed to get RSSI from iw command");
    return OPRT_COM_ERROR;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_get_bssid(UCHAR_T *mac)
{
    if (mac == NULL) {
        TKL_LOGE("Invalid parameter: mac is NULL");
        return OPRT_INVALID_PARM;
    }

    FILE *fp;
    char line[256];
    char *ptr;
    
    // 使用 iw 命令获取连接信息
    fp = popen("iw dev wlan0 link", "r");
    if (fp == NULL) {
        TKL_LOGE("Failed to run iw command");
        return OPRT_COM_ERROR;
    }
    
    // 解析输出查找 BSSID 信息
    while (fgets(line, sizeof(line), fp) != NULL) {
        // 查找 "Connected to XX:XX:XX:XX:XX:XX" 格式
        if ((ptr = strstr(line, "Connected to ")) != NULL) {
            ptr += 13; // 跳过 "Connected to " 
            
            // 解析 MAC 地址格式：aa:bb:cc:dd:ee:ff
            int values[6];
            if (sscanf(ptr, "%x:%x:%x:%x:%x:%x", 
                       &values[0], &values[1], &values[2], 
                       &values[3], &values[4], &values[5]) == 6) {
                
                for (int i = 0; i < 6; i++) {
                    mac[i] = (UCHAR_T)values[i];
                }
                
                TKL_LOGD("BSSID: %02x:%02x:%02x:%02x:%02x:%02x",
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                
                pclose(fp);
                return OPRT_OK;
            }
        }
    }
    
    pclose(fp);
    TKL_LOGE("Failed to get BSSID - not connected or parse error");
    return OPRT_COM_ERROR;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_station_get_status(WF_STATION_STAT_E *stat)
{
  switch(_wifi_connect_state) {
        case 0:
            *stat = WSS_GOT_IP;
            break;
        case 3:
            *stat = WSS_NO_AP_FOUND;
            break;
        case 5:
            *stat = WSS_PASSWD_WRONG;
            break;
        case -1:
            *stat = WSS_IDLE;
            break;
        default:
            *stat = WSS_CONN_FAIL;
    }
    TKL_LOGD("##### tkl_wifi_station_get_status stat=%d\n", *stat);
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_set_country_code(CONST COUNTRY_CODE_E ccode)
{
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_send_mgnt(CONST UCHAR_T *buf, CONST UINT_T len)
{
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_register_recv_mgnt_callback(CONST BOOL_T enable, CONST WIFI_REV_MGNT_CB recv_cb)
{
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_set_rf_calibrated(VOID_T)
{
#ifdef DEBUG_VERSION
    TKL_LOGD("#####:  func=%s, line=%d\n", __func__, __LINE__);
#endif
    TKL_LOGD("set rf calibrated is not support");
    return OPRT_COM_ERROR;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_set_lp_mode(CONST BOOL_T enable, CONST UCHAR_T dtim)
{
    return OPRT_OK;
}

TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wifi_ioctl(WF_IOCTL_CMD_E cmd,  VOID *args) 
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_wifi_get_ssid(char *ssid)
{
    if (ssid == NULL) {
        TKL_LOGE("Invalid parameter: ssid is NULL");
        return OPRT_INVALID_PARM;
    }

    FILE *fp;
    char line[256];
    char *ptr;
    bool found = false;
    
    // 使用 iw 命令获取连接信息
    fp = popen("iw dev wlan0 link", "r");
    if (fp == NULL) {
        TKL_LOGE("Failed to run iw command");
        return OPRT_COM_ERROR;
    }
    
    // 解析输出查找 SSID 信息
    while (fgets(line, sizeof(line), fp) != NULL) {
        // 查找 "SSID: xxx" 格式
        if ((ptr = strstr(line, "SSID:")) != NULL) {
            ptr += 5; // 跳过 "SSID:"
            while (*ptr == ' ' || *ptr == '\t') ptr++; // 跳过空格和制表符
            
            // 移除换行符
            char *newline = strchr(ptr, '\n');
            if (newline) *newline = '\0';
            
            // 检查是否包含十六进制编码的 SSID (格式：\xAB\xCD...)
            if (strstr(ptr, "\\x") != NULL) {
                // 解码十六进制 SSID
                int i = 0, j = 0;
                while (ptr[i] != '\0' && j < 127) {
                    if (ptr[i] == '\\' && ptr[i+1] == 'x' && ptr[i+2] != '\0' && ptr[i+3] != '\0') {
                        char hex[3] = {ptr[i+2], ptr[i+3], '\0'};
                        ssid[j++] = (char)strtol(hex, NULL, 16);
                        i += 4;
                    } else {
                        ssid[j++] = ptr[i++];
                    }
                }
                ssid[j] = '\0';
            } else {
                // 直接复制普通 SSID
                strncpy(ssid, ptr, 127);
                ssid[127] = '\0';
            }
            
            found = true;
            TKL_LOGD("SSID: %s", ssid);
            break;
        }
    }
    
    pclose(fp);
    
    if (!found) {
        TKL_LOGE("Failed to get SSID - not connected");
        return OPRT_COM_ERROR;
    }
    
    return OPRT_OK;
}