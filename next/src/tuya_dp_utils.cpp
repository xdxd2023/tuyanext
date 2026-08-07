#include "tuya_dp_utils.h"

#include <map>

extern std::map<int, TuyaType> *dpTypes;

TY_OBJ_DP_S DPReportBool(int dpId, bool b)
{
    TY_OBJ_DP_S dp = {0};
    dp.dpid = dpId;
    dp.type = PROP_BOOL;
    dp.value.dp_bool = b;
    dp.time_stamp = 0;
    return dp;
}

TY_OBJ_DP_S DPReportEnum(int dpId, int e)
{
    TY_OBJ_DP_S dp = {0};
    dp.dpid = dpId;
    dp.type = PROP_ENUM;
    dp.value.dp_enum = e;
    dp.time_stamp = 0;
    return dp;
}

TY_OBJ_DP_S DPReportValue(int dpId, int v)
{
    TY_OBJ_DP_S dp = {0};
    dp.dpid = dpId;
    dp.type = PROP_VALUE;
    dp.value.dp_value = v;
    dp.time_stamp = 0;
    return dp;
}

TY_OBJ_DP_S DPReportBitmap(int dpId, uint32_t b)
{
    TY_OBJ_DP_S dp = {0};
    dp.dpid = dpId;
    dp.type = PROP_BITMAP;
    dp.value.dp_bitmap = b;
    dp.time_stamp = 0;
    return dp;
}

TY_OBJ_DP_S DPReportString(int dpId, const char *value)
{
    TY_OBJ_DP_S dp = {0};
    dp.dpid = dpId;
    dp.type = PROP_STR;
    dp.value.dp_str = const_cast<char *>(value != NULL ? value : "");
    dp.time_stamp = 0;
    return dp;
}

TY_OBJ_DP_S DPReport(std::map<int, TuyaType> *dpTypes, int dpId, int32_t value)
{
    if (dpTypes == NULL)
    {
        return DPReportValue(dpId, value);
    }

    std::map<int, TuyaType>::const_iterator it = dpTypes->find(dpId);
    if (it == dpTypes->end())
    {
        return DPReportValue(dpId, value);
    }

    TuyaType tuyaType = it->second;
    switch (tuyaType)
    {
    case TuyaTypeBool:
        return DPReportBool(dpId, value);
    case TuyaTypeEnum:
        return DPReportEnum(dpId, value);
    case TuyaTypeValue:
        return DPReportValue(dpId, value);
    case TuyaTypeBitmap:
        return DPReportBitmap(dpId, value);
    default:
        break;
    }
    return DPReportValue(dpId, value);
}