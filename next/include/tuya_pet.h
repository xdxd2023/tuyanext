#pragma once

#include "tuya_robot.h"

class RobotPet : public RobotDevice
{
    bool initialized;
    GridMap         map;
    AppPath         path;
    RobotPet();
public:
    static RobotPet *Get();
    void Init() override;
public:
    void OnMap(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppMap *msg) override;
    void OnPath(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppPath *msg) override;
    void OnCmd(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const Debug *msg) override;
    void OnEvent(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const Event *msg) override;
    void OnSaveMap(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const tuya_message::RobotEvent *msg) override;
    void OnUpdateMap(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const tuya_message::RobotEvent *msg) override;
    void OnTest(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const Event *msg) override;
public:
    void ReportStatus() override;
    void ReportDeviceInfo() override;
    void ReportMap() override;
    void ReportPath() override;
    void ReportMapAll() override;
    void ReportAll() override;

    void OnCruiseStatus(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const tuya_message::RobotState *msg);
    bool CruiseStartMapping() ;
    bool CruiseGoto(const mars_message::AppSpotClean &spot);
    bool CruiseSetPatrol(bool enable);
    bool CruiseStop();
    bool CruiseGetSpot(mars_message::AppSpotClean *spot);
public:
    void HandleDPCmd(TY_OBJ_DP_S * dp) override;
    void HandleRawDPCmd(int dpId, uint8_t *data, int len) override;
    const std::map<int, TuyaType> *DpTypes() const override;
    int GetDpId(RobotDpHandler handler) const override;
    int GetRawDpId(RobotRawDpHandler handler) const override;
};