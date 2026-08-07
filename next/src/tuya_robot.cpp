#include <stdint.h>
#include <netinet/in.h>

#include <vector>
#include <map>
#include <fstream>

#include <utils/json.hpp>
#include "utils/log_.h"
#include "lz4/lz4.h"

#include "tuya_cloud_com_defs.h"
#include "tuya_error_code.h"
#include "tuya_iot_wifi_api.h"
#include "tuya_svc_upgrade.h"

#define __NLOHMANN_JSON_CPP
#include "robot_msg.h"
#include "voice.h"
#include "tuya_robot.h"
#include "tuya_pet.h"
#include "tuya_sweeper.h"
#include "tuya_enums.h"
#include "tuya_dp_function.h"
#include "tuya_utils.h"
#include "tuya_dp_utils.h"
#include "tuyaos_sdk.h"


#define TAG "TUY"

#define APP_VERSION_FILE "/userdata/version/soft_version.json" 
extern std::string robot_type;

void TuyaHandleDPQuery(int dpId)
{
    LOGD(TAG, "DP query: {}", dpId);
    switch (dpId)
    {
    case 1:
        break;
    case 4:
        break;
    default:
        break;
    }
}
namespace {

RobotDevice *ResolveRobot()
{
    if(robot_type == "Sweeper")
    {
        return RobotSweeper::Get();
    }
    else
    {
        return RobotPet::Get();
    }
}


} // namespace

RobotDevice *GetRobot()
{
    return ResolveRobot();
}


void TuyaHandleRawDPCmd(int dpId, uint8_t *data, int len)
{
    LOGD(TAG, "Raw DP cmd: {}", dpId);
    GetRobot()->HandleRawDPCmd(dpId, data, len);
}

int GetDpId(void (*dpHanlder)(TY_OBJ_DP_S*))
{
    return GetRobot()->GetDpId(dpHanlder);
}


void TuyaHandleDPCmd(TY_OBJ_DP_S *dp)
{
    LOGD(TAG, "DP cmd: {}", dp->dpid);
    GetRobot()->HandleDPCmd(dp);
}

void PlayVoice(int v, int param)
{
    Event voice;
    voice.event = v;
    voice.param = param;
    TuyaComm::Get()->Publish("Voice", &voice);
}

void DevicePairingSuccess(void)
{
    PlayVoice(V_SERVER_CONNECTED, PLAY_QUEUE);
    tuya_message::Request req;
    tuya_message::Result res = {};
    TuyaComm::Get()->Send("ty_wifi_connect", &req, &res);
    
}

int camera_led[2] = {0, 0};

void CameraLed(int on, int reason)
{
    if(on)
    {
        LOGD(TAG, "Camera led on: {}", reason);
        camera_led[reason] = 1;
    }
    if(!on)
    {
        LOGD(TAG, "Camera led off: {}", reason);
        camera_led[reason] = 0;
        int sum = 0;
        for(int i=0; i<2; i++)
        {
            sum += camera_led[i];
        }
        if(sum != 0)
        {
            LOGD(TAG, "Camera led still on: {}", sum);
            return;
        }
    }
    Event voice;
    voice.event = 'CLED';
    voice.param = on;
    TuyaComm::Get()->Publish("Kevt", &voice);
}

void BleOn(bool on)
{
    Event e;
    e.event = on;
    TuyaComm::Get()->Publish("BleOn", &e);
}