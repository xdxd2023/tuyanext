#pragma once

#include "tuya_robot.h"

class RobotSweeper : public RobotDevice
{
    bool initialized;
    GridMap         map;
    AppPath         path;
    AppPartsLife    partsLife;
    AppCleanInfo    cleanInfo;
    AppVirtualWall  virtualWall;
    AppRestrictedArea   restrictedArea;
    AppSpotClean    cruiseSpot;
    tuya_message::RobotState sweeper;
    RobotSweeper();
public:
    static RobotSweeper *Get();
    static std::map<int, TuyaType> &GetDpTypes();
    static std::map<int, RobotDpHandler> &GetDpHandlers();
    void Init() override;
public:
    void OnMap(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppMap *msg) override;
    void OnPath(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppPath *msg) override;
    void OnCmd(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const Debug *msg) override;
    void OnSweeperStatus(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const tuya_message::RobotState *msg);
    void OnPartsLife(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppPartsLife *msg);
    void OnCleanInfo(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppCleanInfo *msg);
    void OnVirtualWall(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppVirtualWall *msg);
    void OnRestrictedArea(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppRestrictedArea *msg);
    void OnCleanRecord(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppCleanRecord *msg);
    void OnSchedule(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppLocalAlert *msg);
    void OnEvent(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const Event *msg) override;
    void OnSaveMap(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const tuya_message::RobotEvent *msg) override;
    void OnUpdateMap(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const tuya_message::RobotEvent *msg) override;
    void OnTest(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const Event *msg) override;
    void OnRoomClean(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const mars_message::AppRoomClean *msg);
public:
    void ReportStatus() override;
    void ReportDeviceInfo() override;
    void ReportMap() override;
    void ReportPath() override;
    void ReportMapAll() override;
    void ReportAll() override;
    void ReportPartsLife();
    void ReportCleanInfo();
    bool ReportCleanRecords(int recordId, int cleanTime, int cleanArea, int cleanMode, int workMode, int cleaningResult, int startMethod);
    void TuyaReportRobotStatus(tuya_message::RobotState status);
    void HandleDPCmd(TY_OBJ_DP_S *dp) override;
    void HandleRawDPCmd(int dpId, uint8_t *data, int len) override;
    const std::map<int, TuyaType> *DpTypes() const override;
    int GetDpId(RobotDpHandler handler) const override;
    int GetRawDpId(RobotRawDpHandler handler) const override;
};