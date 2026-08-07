#pragma once

#include <pthread.h>
#include <map>
#include <string>

#include <lcm/lcm-cpp.hpp>

#include "tuya_cloud_com_defs.h"
#include "tuya_dp_utils.h"
#include "message/core_messages.h"
#include "message/tuya_messages.h"
#include "mars_message/AppSpotClean.hpp"

using namespace mars_message;

typedef void (*RobotDpHandler)(TY_OBJ_DP_S *dp);
typedef void (*RobotRawDpHandler)(int dpId, uint8_t *data, int len);

class RobotDevice
{
public:
    virtual ~RobotDevice() = default;

    virtual void Init() = 0;

    virtual void OnMap(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppMap *msg) = 0;
    virtual void OnPath(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppPath *msg) = 0;
    virtual void OnCmd(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const Debug *msg) = 0;
    virtual void OnEvent(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const Event *msg) = 0;
    virtual void OnSaveMap(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const tuya_message::RobotEvent *msg) = 0;
    virtual void OnUpdateMap(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const tuya_message::RobotEvent *msg) = 0;
    virtual void OnTest(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const Event *msg) = 0;

    virtual void ReportStatus() = 0;
    virtual void ReportDeviceInfo() = 0;
    virtual void ReportMap() = 0;
    virtual void ReportPath() = 0;
    virtual void ReportMapAll() = 0;
    virtual void ReportAll() = 0;

    virtual void HandleDPCmd(TY_OBJ_DP_S *dp) = 0;
    virtual void HandleRawDPCmd(int dpId, uint8_t *data, int len) = 0;

    virtual const std::map<int, TuyaType> *DpTypes() const
    {
        return NULL;
    }

    virtual int GetDpId(RobotDpHandler handler) const = 0 ;

    virtual int GetRawDpId(RobotRawDpHandler handler) const = 0 ;
};

class TuyaComm
{
    int handlerId;
    TuyaComm()
    {
        handlerId = 0;
    };
public:
    lcm::LCM * lcm_;

    static TuyaComm* Get()
    {
        static TuyaComm tuyaComm;
        return &tuyaComm;
    }
    void SetLcm(lcm::LCM *lcm)
    {
        lcm_ = lcm;
    }

    template <class MessageType>
    inline int Publish(const std::string &channel, const MessageType *msg)
    {
        return lcm_->publish(channel, msg);
    }

    template <class MessageType, class MessageHandlerClass>
    void* Subscribe(const std::string &channel,
                             void (MessageHandlerClass::*handlerMethod)(const lcm::ReceiveBuffer *rbuf,
                                                                        const std::string &channel,
                                                                        const MessageType *msg),
                             MessageHandlerClass *handler)
    {
        return lcm_->subscribe(channel, handlerMethod, handler);
    }

    template <class MessageType, class MessageTypeRet>
    inline int Send(const std::string &channel, MessageType *msg, MessageTypeRet *ret, int timeoutMs = 100, int retryTimes = 1)
    {
        return lcm_->send(channel, msg, ret, timeoutMs, retryTimes) == 0;
    }

    template <class MessageType>
    inline bool Send(const std::string &channel, MessageType *msg, int timeoutMs = 100, int retryTimes = 1)
    {
        MessageType ret;
        int r = lcm_->send(channel, msg, &ret, timeoutMs, retryTimes);
        *msg = ret;
        return r == 0;
    }

    void HandleForever()
    {
        handlerId = pthread_self();
        while (0 == lcm_->handle());
    }

    int HandleTimeout(int timeoutMs)
    {
        handlerId = pthread_self();
        return lcm_->handleTimeout(timeoutMs);
    }
};

RobotDevice *GetRobot();
void TuyaHandleRawDPCmd(int dpId, uint8_t *data, int len);
void PlayVoice(int v, int param);
void DevicePairingSuccess(void);
void CameraLed(int on, int reason);
int TuyaBatteryGet(void);
#ifdef __cplusplus
extern "C" {
#endif
void BleOn(bool on);
#ifdef __cplusplus
}
#endif