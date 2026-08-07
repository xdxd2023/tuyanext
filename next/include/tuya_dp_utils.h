#pragma once

#include <stdint.h>
#include <map>
#include <string>
#include "tuya_cloud_com_defs.h"

enum TuyaType
{
    TuyaTypeBool = 0,
    TuyaTypeEnum = 1,
    TuyaTypeValue = 2,
    TuyaTypeBitmap = 3,
    TuyaTypeRaw = 4,
};

struct TuyaDPRawHandler
{
    int dpId;
    void (*dpHanlder)(int dpId, uint8_t *data, int len);
};

struct TuyaDPHandler
{
    int dpId;
    void (*dpHanlder)(TY_OBJ_DP_S*);
};

TY_OBJ_DP_S DPReportBool(int dpId, bool b);
TY_OBJ_DP_S DPReportEnum(int dpId, int e);
TY_OBJ_DP_S DPReportValue(int dpId, int v);
TY_OBJ_DP_S DPReportBitmap(int dpId, uint32_t b);
TY_OBJ_DP_S DPReportString(int dpId, const char *value);
TY_OBJ_DP_S DPReport(std::map<int, TuyaType> *dpTypes, int dpId, int32_t value);