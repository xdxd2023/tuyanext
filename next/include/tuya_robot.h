#pragma once

#include <lcm/lcm-cpp.hpp>

#include "message/core_messages.h"
#include "message/tuya_messages.h"

using namespace mars_message;

class TuyaComm
{
private:
    GridMap         map;
    AppPath         path;
    AppPartsLife    partsLife;
    AppCleanInfo    cleanInfo;
    AppVirtualWall  virtualWall;
    AppRestrictedArea   restrictedArea;
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
        lcm_->subscribe("ty_Map", &TuyaComm::OnMap, this);
        lcm_->subscribe("ty_Path", &TuyaComm::OnPath, this);
        lcm_->subscribe("ty_VirtualWall", &TuyaComm::OnVirtualWall, this);
        lcm_->subscribe("ty_RestrictedArea", &TuyaComm::OnRestrictedArea, this);
        lcm_->subscribe("ty_CleanRecord", &TuyaComm::OnCleanRecord, this);
        lcm_->subscribe("ty_m_schedule", &TuyaComm::OnSchedule, this);

        //lcm_->subscribe("APPCMD", &TuyaComm::OnCmd, this);
        lcm_->subscribe("ty_robot_state", &TuyaComm::OnSweeperStatus, this);
        lcm_->subscribe("ty_PartsLife", &TuyaComm::OnPartsLife, this);
        lcm_->subscribe("ty_CleanInfo", &TuyaComm::OnCleanInfo, this);
        lcm_->subscribe("ty_SaveMap", &TuyaComm::OnSaveMap, this);
        lcm_->subscribe("ty_UpdateMap", &TuyaComm::OnUpdateMap, this);
        lcm_->subscribe("Mevt", &TuyaComm::OnEvent, this);
        lcm_->subscribe("Kevt", &TuyaComm::OnEvent, this);
        lcm_->subscribe("Mtest", &TuyaComm::OnTest, this);
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

public:
    void OnMap(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppMap *msg);
    void OnPath(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppPath *msg);
    void OnCmd(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const Debug *msg);
    void OnSweeperStatus(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const tuya_message::RobotState *msg);
    void OnPartsLife(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppPartsLife *msg);
    void OnCleanInfo(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppCleanInfo *msg);
    void OnVirtualWall(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppVirtualWall *msg);
    void OnRestrictedArea(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppRestrictedArea *msg);
    void OnCleanRecord(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppCleanRecord *msg);
    void OnSchedule(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppLocalAlert *msg);
    void OnEvent(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const Event *msg);
    void OnSaveMap(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const tuya_message::RobotEvent *msg);
    void OnUpdateMap(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const tuya_message::RobotEvent *msg);
    void OnTest(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const Event *msg);
public:
    void ReportStatus();
    void ReportDeviceInfo();
    void ReportMap();
    void ReportPath();
    void ReportMapAll();
    void ReportAll();
    void ReportPartsLife();
    void ReportCleanInfo();
    bool ReportCleanRecords(int recordId, int cleanTime, int cleanArea, int cleanMode, int workMode, int cleaningResult, int startMethod);
};

void TuyaReportStatus(SweeperStatus status);
void TuyaHandleRawDPCmd(int dpId, uint8_t *data, int len);
void PlayVoice(int v, int param);
void DevicePairingSuccess(void);
void CameraLed(int on, int reason);