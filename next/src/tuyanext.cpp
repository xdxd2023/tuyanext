#include <iostream>
#include <fstream>
#include <string>

#include "utils.h"
#include "tuya_robot.h"
#include "tuya_utils.h"
#include "tuyaos.h"
#include "voice.h"
#include "time_tick.h"
#include "tkl_bluetooth.h"
using namespace mars_message;
#define TAG "TUY"

#define PRODUCT_ID          "cnyiygalqpueeiwf"
#define DEVICE_UUID         "uuid3658996f48293db1"
#define PRODUCT_AUTHKEY     "BG15bQZz92o2iPUR7X1LDJYLXOlGpLwe"

#define APP_STORAGE_PATH    "./tuya/"
#define APP_DEFAULT_KEY     "./tuya/tuya.key"
#define APP_UPGRADE_PATH    "../version/"
#define APP_UPGRADE_FILE    "../version/upgrade.zip"
#define APP_VERSION_FILE    "../version/soft_version.json"

#define TY_SDK_P2P_NUM_MAX     5           //最大支持 5 个客户端
#define TY_APP_STORAGE_PATH    "/data/"     //虚拟存储路径
#define TY_SDK_ONLINE_LOG_PATH "/tmp/"     //虚拟存储路径


/*certification information(essential)*/
std::string id = PRODUCT_ID;
std::string uuid = DEVICE_UUID;
std::string authKey = PRODUCT_AUTHKEY;

//app version
std::string soft_version;
std::string MCU_version ;
std::string system_version;

std::string robot_type = "Sweeper";

class TestHandler
{
public:
    void OnGet(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const mars_message::KeyValue *msg)
    {
        KeyValue ret;
        ret.value = msg->value;
        switch(msg->value)
        {
            case 'PID':
            ret.key = id;
            break;
            case 'CVER':
            ret.key = soft_version;
            break;
            case 'MAC':
            ret.key = shell::valueof("ifconfig eth0 | grep HWaddr | awk '{print $5}'");
            break;
        }
        TuyaComm::Get()->Publish("Mgetr", &ret);
    }

    void OnSet(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const mars_message::KeyValue *msg)
    {
        KeyValue ret;
        ret.value = msg->value;
        ret.key = "0";
        switch(msg->value)
        {
            case 'PID':
            // todo: 改变 pid
            ret.key = "1";
            break;
            case 'CVER':
            break;
            case 'MAC':
            break;
        }
        //TuyaComm::Get()->Publish("Msetr", &ret);
    }
};

bool tuyaIsPairMode = false;

int main(int argc, char ** argv)
{
    LOGVERSION("v1.0.15");
    //StackTrace stackTrack;
    TuyaLoadKey(APP_DEFAULT_KEY, id, uuid, authKey, robot_type);


    lcm::LCM lcm;
    if(!lcm.good())
    {
        return -1;
    }  
    TuyaComm::Get()->SetLcm(&lcm);
    TestHandler testHandler;
    TuyaComm::Get()->Subscribe("Mget", &TestHandler::OnGet, &testHandler);
    TuyaComm::Get()->Subscribe("Mset", &TestHandler::OnSet, &testHandler);

    uint64_t startTime = 0;
    if(argc == 2 && std::string(argv[1]) == "reset")
    {
        startTime = TimeTick::Ms();
        BleOn(false);
        sleep(3);
        system("bluetoothctl disconnect");
        tuyaIsPairMode = true;
    }

    GetRobot()->Init();

    std::ofstream uuid_file("/tmp/uuid.txt");
    if (uuid_file.is_open()) {
        uuid_file << uuid;
        uuid_file.close();
    } else {
        LOGD(TAG, "Unable to open file to write UUID");
    }

    SoftVersion(APP_VERSION_FILE, soft_version, MCU_version, system_version);
    if(MCU_version.empty())
    {
        MCU_version = "1.0.0";
    }
    LOGD(TAG, "soft_version: {}, MCU_version: {}, system_version: {}", soft_version, MCU_version, system_version);
    tuyaos_init(uuid.c_str(), authKey.c_str(), id.c_str(), soft_version.c_str(), MCU_version.c_str());
    bool is_post_init = false;
    bool is_online = false;

    while(true)
    {
        TuyaComm::Get()->HandleTimeout(1);
        if(tuyaos_is_online())
        {
            if(!is_post_init)
            {
                is_post_init = true;
                LOGD(TAG, "Device is online, performing post-initialization");
                tuyaos_post_init();
            }
            if(!is_online)
            {
                tkl_ble_stack_deinit(0);
                sleep(1);
                BleOn(true);
                is_online = true;
                GetRobot()->ReportAll();
            }
        }
        else
        {
            is_online = false;
        }
        if(startTime != 0 && TimeTick::Ms() - startTime > 180 * 1000)
        {
            tkl_ble_stack_deinit(0);
            sleep(1);
            BleOn(true);
            startTime = 0;
        }
    }   
    return 0;
}
