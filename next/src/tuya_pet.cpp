#include <stdint.h>
#include <netinet/in.h>

#include <iomanip>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>

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
#include "tuya_pet.h"
#include "tuya_robot.h"
#include "tuya_enums.h"
#include "tuya_dp_utils.h"
#include "tuya_dp_function.h"
#include "tuya_utils.h"
#include "tuyaos_sdk.h"

#undef TAG
#define TAG "TUYA_PET"

using json = nlohmann::json;

bool PetReportBoolState(int dpId, bool value);
bool PetReportEnumState(int dpId, int value);
bool PetReportValueState(int dpId, int value);
bool PetReportStringState(int dpId, const std::string &value);
bool PetReportRawState(int dpId, const uint8_t *data, int len, int timeoutMs);
bool PetCruiseStartMapping();
bool PetCruiseGoto(const mars_message::AppSpotClean &spot);
bool PetCruiseGetSpot(mars_message::AppSpotClean *spot);
bool PetCruiseSetPatrol(bool enable);

namespace
{

	mars_message::AppSpotClean g_petCruiseSpot = {};
	bool g_hasPetCruiseSpot = false;
	tuya_message::RobotState g_petRobotState = {};
	std::string g_petLastCruiseMode = "idle";

	struct PetDpHandlerEntry
	{
		int dpId;
		void (*handler)(TY_OBJ_DP_S *);
	};

	struct PetRawHandlerEntry
	{
		int dpId;
		void (*handler)(int dpId, uint8_t *data, int len);
	};

	std::map<int, TuyaType> kPetDpTypes = {
		{1, TuyaTypeBool},
		{2, TuyaTypeBool},
		{3, TuyaTypeBool},
		{4, TuyaTypeEnum},
		{5, TuyaTypeEnum},
		{23, TuyaTypeValue},
		{24, TuyaTypeValue},
		{25, TuyaTypeBool},
		{27, TuyaTypeBool},
		{28, TuyaTypeRaw},
		{29, TuyaTypeBool},
		{36, TuyaTypeBool},
		{37, TuyaTypeBool},
		{43, TuyaTypeValue},
		{44, TuyaTypeBool},
		{45, TuyaTypeValue},
		{46, TuyaTypeBool},
		{48, TuyaTypeRaw},
		{50, TuyaTypeBool},
		{51, TuyaTypeEnum},
		{52, TuyaTypeBool},
		{53, TuyaTypeBool},
		{54, TuyaTypeBool},
		{56, TuyaTypeBool},
		{57, TuyaTypeEnum},
		{58, TuyaTypeBool},
		{59, TuyaTypeBool},
		{60, TuyaTypeBool},
		{61, TuyaTypeEnum},
		{62, TuyaTypeBool},
		{63, TuyaTypeRaw},
		{64, TuyaTypeValue},
		{65, TuyaTypeValue},
		{66, TuyaTypeRaw},
		{69, TuyaTypeRaw},
		{72, TuyaTypeBool},
		{73, TuyaTypeBool},
		{74, TuyaTypeEnum},
		{75, TuyaTypeValue},
		{76, TuyaTypeEnum},
		{78, TuyaTypeEnum},
		{80, TuyaTypeBool},
		{83, TuyaTypeBool},
		{84, TuyaTypeBool},
		{86, TuyaTypeBool},
		{212, TuyaTypeRaw},
	};

	bool PetReportObjectDp(const TY_OBJ_DP_S &dp)
	{
		TY_OBJ_DP_S temp = dp;
		OPERATE_RET ret = dev_report_dp_json_async(NULL, &temp, 1);
		if (ret != OPRT_OK)
		{
			LOGD(TAG, "report object dp failed, dpId:{}, ret:{}", dp.dpid, ret);
			return false;
		}
		return true;
	}

	std::string PetRawToHex(const uint8_t *data, int len)
	{
		std::ostringstream oss;
		for (int i = 0; i < len; ++i)
		{
			oss << std::hex << std::setw(2) << std::setfill('0')
				<< static_cast<int>(data[i]);
		}
		return oss.str();
	}

	void PetPublishString(const std::string &channel, const std::string &value)
	{
		mars_message::String msg = {};
		msg.value = value;
		TuyaComm::Get()->Publish(channel, &msg);
	}

	void PetPublishRaw(const std::string &channel, int dpId, const uint8_t *data, int len)
	{
		if (data == NULL || len <= 0)
		{
			return;
		}

		json payload;
		payload["dpid"] = dpId;
		payload["len"] = len;
		payload["hex"] = PetRawToHex(data, len);
		PetPublishString(channel, payload.dump());
	}

	bool PetSendRequest(const std::string &channel, int a = 0, int b = 0, int c = 0)
	{
		tuya_message::Request req = {a, b, c};
		tuya_message::Result res = {};
		bool ok = TuyaComm::Get()->Send(channel, &req, &res);
		if (!ok)
		{
			LOGD(TAG, "send failed, channel:{}, req:[{},{},{}]", channel, a, b, c);
		}
		return ok;
	}

	bool PetActionOnTrue(const std::string &channel, TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return false;
		}
		if (!dp->value.dp_bool)
		{
			return true;
		}
		return PetSendRequest(channel, 1, 0, 0);
	}

	void PetHandleSwitchGo(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		// 没有地图就快速建图，有地图巡逻
		tuya_message::Request req = {dp->value.dp_bool};
		tuya_message::Result res = {};
		TuyaComm::Get()->Send("pet_go", &req, &res);
	}

	void PetHandlePause(TY_OBJ_DP_S *dp)
	{
		tuya_message::Request req = {dp->value.dp_bool};
		tuya_message::Result res = {};
		TuyaComm::Get()->Send("pet_pause", &req, &res);
	}

	void PetHandleSwitchCharge(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		tuya_message::Request req = {dp->value.dp_bool};
		tuya_message::Result res = {};
		TuyaComm::Get()->Send("pet_charge", &req, &res);
	}

	void PetHandleMode(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		PetSendRequest("pet_mode", dp->value.dp_enum, 0, 0);
	}

	void PetHandleVolumeSet(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		PetSendRequest("pet_volume_set", dp->value.dp_value, 0, 0);
	}

	void PetHandleChildLock(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		PetSendRequest("pet_child_lock", dp->value.dp_bool, 0, 0);
	}

	void PetHandleSeek(TY_OBJ_DP_S *dp)
	{
		PetActionOnTrue("pet_seek", dp);
	}

	void PetHandleRobotLedSwitch(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		PetSendRequest("pet_robot_led_switch", dp->value.dp_bool, 0, 0);
	}

	void PetHandleFillLightSwitch(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		PetSendRequest("pet_fill_light_switch", dp->value.dp_bool, 0, 0);
	}

	void PetHandleBasicPrivate(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		PetSendRequest("pet_basic_private", dp->value.dp_bool, 0, 0);
	}

	void PetHandleMovementDirection(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		LOGD(TAG, "MovementDirection: {}", dp->value.dp_value);
		PetSendRequest("pet_movement_direction", dp->value.dp_value, 0, 0);
	}

	void PetHandleMovementStops(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		PetSendRequest("pet_movement_stops", dp->value.dp_bool, 0, 0);
	}

	void PetHandleSpeed(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		PetSendRequest("pet_speed", dp->value.dp_value, 0, 0);
	}

	void PetHandleCruiseSwitch(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}

		if (PetCruiseSetPatrol(dp->value.dp_bool))
		{
			PetReportBoolState(dp->dpid, dp->value.dp_bool);
		}
		else
		{
			LOGE(TAG, "pet cruise switch failed");
		}
	}

	void PetHandleExpressionLight(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		PetSendRequest("pet_expression_light", dp->value.dp_bool, 0, 0);
	}

	void PetHandleLaser(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		PetSendRequest("pet_laser", dp->value.dp_enum, 0, 0);
	}

	void PetHandlePlaying(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		PetSendRequest("pet_playing", dp->value.dp_bool, 0, 0);
	}

	void PetHandleFollow(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		PetSendRequest("pet_follow", dp->value.dp_bool, 0, 0);
	}

	void PetHandleAutomaticFeeding(TY_OBJ_DP_S *dp)
	{
		PetActionOnTrue("pet_automatic_feeding", dp);
	}

	void PetHandleOpenVideoRecording(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		PetSendRequest("pet_open_video_recording", dp->value.dp_bool, 0, 0);
	}

	void PetHandleBasicNightVision(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		PetSendRequest("pet_basic_nightvision", dp->value.dp_enum, 0, 0);
	}

	void PetHandleBasicOsd(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		PetSendRequest("pet_basic_osd", dp->value.dp_bool, 0, 0);
	}

	void PetHandleScreenSwitch(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		PetSendRequest("pet_screen_switch", dp->value.dp_bool, 0, 0);
	}

	void PetHandleRecordSwitch(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		PetSendRequest("pet_record_switch", dp->value.dp_bool, 0, 0);
	}

	void PetHandleRecordMode(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		PetSendRequest("pet_record_mode", dp->value.dp_enum, 0, 0);
	}

	void PetHandleSdFormat(TY_OBJ_DP_S *dp)
	{
		PetActionOnTrue("pet_sd_format", dp);
	}

	void PetHandleAudioId(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL || dp->value.dp_str == NULL)
		{
			return;
		}
		PetPublishString("pet_audio_id", dp->value.dp_str);
	}

	void PetHandleFactoryReset(TY_OBJ_DP_S *dp)
	{
		PetActionOnTrue("pet_factory_reset", dp);
	}

	void PetHandleDeviceRestart(TY_OBJ_DP_S *dp)
	{
		PetActionOnTrue("pet_device_restart", dp);
	}

	void PetHandleSwitchPerspective(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		PetSendRequest("pet_switch_perspective", dp->value.dp_enum, 0, 0);
	}

	void PetHandleScreenRotation(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		PetSendRequest("pet_screen_rotation", dp->value.dp_value, 0, 0);
	}

	void PetHandleSpeedSet(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		PetSendRequest("pet_speed_set", dp->value.dp_enum, 0, 0);
	}

	void PetHandleCatTeaser(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		PetSendRequest("pet_cat_teaser", dp->value.dp_enum, 0, 0);
	}

	void PetHandlePowerChange(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		PetSendRequest("pet_power_change", dp->value.dp_bool, 0, 0);
	}

	void PetHandleDriveAway(TY_OBJ_DP_S *dp)
	{
		PetActionOnTrue("pet_drive_away", dp);
	}

	void PetHandlePitching(TY_OBJ_DP_S *dp)
	{
		PetActionOnTrue("pet_pitching", dp);
	}

	void PetHandleScreenFlip(TY_OBJ_DP_S *dp)
	{
		if (dp == NULL)
		{
			return;
		}
		PetSendRequest("pet_screen_flip", dp->value.dp_bool, 0, 0);
	}

	void PetHandleTotalErrorRaw(int dpId, uint8_t *data, int len)
	{
		PetPublishRaw("pet_total_error_raw", dpId, data, len);
	}

	void PetHandleTimedBasicPrivateRaw(int dpId, uint8_t *data, int len)
	{
		PetPublishRaw("pet_timed_basic_private_raw", dpId, data, len);
	}

	void PetHandleInitiativeMessageRaw(int dpId, uint8_t *data, int len)
	{
		PetPublishRaw("pet_initiative_message_raw", dpId, data, len);
	}

	void PetHandleCruiseCommandRaw(int dpId, uint8_t *data, int len)
	{
		 if (data[0] == 0xAA)
		{
			TuyaHandleStandardFunction(dpId, data, len);
		}
		if (data[0] == 0xAB)
		{
			TuyaHandleExtentedFuction(dpId, data, len);
		}
		return;
	}

	PetDpHandlerEntry kPetDpHandlers[] = {
		{1, PetHandleSwitchGo},
		{2, PetHandlePause},
		{3, PetHandleSwitchCharge},
		{4, PetHandleMode},
		{24, PetHandleVolumeSet},
		{25, PetHandleChildLock},
		{27, PetHandleSeek},
		{29, PetHandleRobotLedSwitch},
		{36, PetHandleFillLightSwitch},
		{37, PetHandleBasicPrivate},
		{43, PetHandleMovementDirection},
		{44, PetHandleMovementStops},
		{45, PetHandleSpeed},
		{46, PetHandleCruiseSwitch},
		{50, PetHandleExpressionLight},
		{51, PetHandleLaser},
		{52, PetHandlePlaying},
		{53, PetHandleFollow},
		{54, PetHandleAutomaticFeeding},
		{56, PetHandleOpenVideoRecording},
		{57, PetHandleBasicNightVision},
		{58, PetHandleBasicOsd},
		{59, PetHandleScreenSwitch},
		{60, PetHandleRecordSwitch},
		{61, PetHandleRecordMode},
		{62, PetHandleSdFormat},
		{69, PetHandleAudioId},
		{72, PetHandleFactoryReset},
		{73, PetHandleDeviceRestart},
		{74, PetHandleSwitchPerspective},
		{75, PetHandleScreenRotation},
		{76, PetHandleSpeedSet},
		{78, PetHandleCatTeaser},
		{80, PetHandlePowerChange},
		{83, PetHandleDriveAway},
		{84, PetHandlePitching},
		{86, PetHandleScreenFlip},
	};

	PetRawHandlerEntry kPetRawDpHandlers[] = {
		{15, PetHandleCruiseCommandRaw},
		{28, PetHandleTotalErrorRaw},
		{48, PetHandleTimedBasicPrivateRaw},
		{212, PetHandleInitiativeMessageRaw},
	};

	bool PetParseCruiseJson(const std::string &payload, json *out)
	{
		if (out == NULL)
		{
			return false;
		}

		try
		{
			*out = json::parse(payload);
			return true;
		}
		catch (const std::exception &ex)
		{
			LOGE(TAG, "Failed to parse pet cruise payload: {}", ex.what());
			return false;
		}
	}

	bool PetCruiseStatusToSpot(const json &status, mars_message::AppSpotClean *spot)
	{
		if (spot == NULL || !status.is_object() || !status.contains("active_goal"))
		{
			return false;
		}

		const json &goal = status["active_goal"];
		if (!goal.is_object() || !goal.contains("x") || !goal.contains("y"))
		{
			return false;
		}

		if (status.value("mode", std::string()) != "goto")
		{
			return false;
		}

		spot->version = 1;
		spot->mode = 0;
		spot->suction = 0;
		spot->cistern = 0;
		spot->cleanRepeat = 0;
		spot->spot.x = goal.value("x", 0.0);
		spot->spot.y = goal.value("y", 0.0);
		spot->spot.z = 0.0;
		return true;
	}

	bool PetSendCruiseRequest(const json &cmd, json *reply)
	{
		mars_message::String req = {};
		mars_message::String res = {};
		req.value = cmd.dump();
		if (!TuyaComm::Get()->Send("CruiseCommand", &req, &res, 1500, 1))
		{
			LOGE(TAG, "Pet cruise rpc failed: {}", req.value);
			return false;
		}

		if (!PetParseCruiseJson(res.value, reply))
		{
			return false;
		}

		if (reply != NULL && reply->is_object() && reply->contains("ok") && !reply->value("ok", false))
		{
			LOGE(TAG, "Pet cruise rejected: {}", reply->dump());
			return false;
		}
		return true;
	}

	void PetUpdateCruiseSpotCache(const mars_message::AppSpotClean &spot)
	{
		g_petCruiseSpot = spot;
		if (g_petCruiseSpot.version <= 0)
		{
			g_petCruiseSpot.version = 1;
		}
		g_hasPetCruiseSpot = true;
	}

	bool PetIsCruiseActive(const std::string &mode)
	{
		return !mode.empty() && mode != "idle";
	}

	void PetReportCruiseSwitch(bool enabled)
	{
		PetReportBoolState(46, enabled);
	}

} // namespace

RobotPet::RobotPet()
	: initialized(false)
{
}

RobotPet *RobotPet::Get()
{
	static RobotPet robotPet;
	return &robotPet;
}

void RobotPet::Init()
{
	if (initialized)
	{
		return;
	}

	initialized = true;
	TuyaComm::Get()->Subscribe("ty_Map", &RobotPet::OnMap, this);
    TuyaComm::Get()->Subscribe("ty_Path", &RobotPet::OnPath, this);
    TuyaComm::Get()->Subscribe("ty_robot_state", &RobotPet::OnCruiseStatus, this);
    TuyaComm::Get()->Subscribe("ty_SaveMap", &RobotPet::OnSaveMap, this);
    TuyaComm::Get()->Subscribe("ty_UpdateMap", &RobotPet::OnUpdateMap, this);
    TuyaComm::Get()->Subscribe("Mevt", &RobotPet::OnEvent, this);
    TuyaComm::Get()->Subscribe("Kevt", &RobotPet::OnEvent, this);
    TuyaComm::Get()->Subscribe("Mtest", &RobotPet::OnTest, this);
}

void RobotPet::OnMap(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppMap *msg)
{
	TuyaMap tuyaMap = ToTuyaMap(*msg);
    SaveTuyaMap(tuyaMap, "/tmp/map.bin");
}

void RobotPet::OnPath(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const AppPath *msg)
{
	path = *msg;
    TuyaPath tuyaPath = ToTuyaPath(*msg, msg->id, 0);
    SaveTuyaPath(tuyaPath, "/tmp/cleanPath.bin");
}

void RobotPet::OnCmd(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const Debug *msg)
{
}

void RobotPet::OnEvent(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const Event *msg)
{
	LOGD(TAG, "Event: {}", M2S(msg->event));
    switch (msg->event)
    {
    case MSG_RESET_WIFI:
	case MSG_WIFI_SOFTAP:
        tuya_iot_wf_gw_unactive();
        break;
    }
}

void RobotPet::OnSaveMap(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const tuya_message::RobotEvent *msg)
{
}

void RobotPet::OnUpdateMap(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const tuya_message::RobotEvent *msg)
{
}

void RobotPet::OnTest(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const Event *msg)
{
}

void RobotPet::OnCruiseStatus(const lcm::ReceiveBuffer *rbuf, const std::string &channel, const tuya_message::RobotState *msg)
{
	
	std::vector<TY_OBJ_DP_S> dps;
	tuya_message::RobotState status = *msg;
    nlohmann::json j = status;
    LOGD(TAG, "PetStatus: {}", j.dump(2));

	dps.emplace_back(DPReport(&kPetDpTypes, 1, status.switch_go));			 // swithgo
	dps.emplace_back(DPReport(&kPetDpTypes, 2, status.pause));				 // pause
	dps.emplace_back(DPReport(&kPetDpTypes, 3, status.switch_charge));		 // switch_charge
	dps.emplace_back(DPReport(&kPetDpTypes, 4, 0));							 // mode
	dps.emplace_back(DPReport(&kPetDpTypes, 5, status.status));				 // mode
	dps.emplace_back(DPReport(&kPetDpTypes, 23, status.battery));			 // battery
	dps.emplace_back(DPReport(&kPetDpTypes, 24, status.volume_set));		 // volume_set
	dps.emplace_back(DPReport(&kPetDpTypes, 25, 0));						 // child_lock
	dps.emplace_back(DPReport(&kPetDpTypes, 27, 0));						 // seek
	dps.emplace_back(DPReport(&kPetDpTypes, 29, 0));						 // robot_led_switch
	dps.emplace_back(DPReport(&kPetDpTypes, 36, 0));						 // fill_light_switch
	dps.emplace_back(DPReport(&kPetDpTypes, 37, 0));						 // basic_private
	dps.emplace_back(DPReport(&kPetDpTypes, 43, 0)); // movement_direction
	dps.emplace_back(DPReport(&kPetDpTypes, 44, 0));	 // movement_stops
	dps.emplace_back(DPReport(&kPetDpTypes, 45, 0));
	dps.emplace_back(DPReport(&kPetDpTypes, 46, 0));		   // cruise_switch
	dps.emplace_back(DPReport(&kPetDpTypes, 50, 0));	   // expression_light
	dps.emplace_back(DPReport(&kPetDpTypes, 51, 0));				   // laser
	dps.emplace_back(DPReport(&kPetDpTypes, 52, 0));			   // playing
	dps.emplace_back(DPReport(&kPetDpTypes, 53, 0));			   // follow
	dps.emplace_back(DPReport(&kPetDpTypes, 54, 0));	   // automatic_feeding
	dps.emplace_back(DPReport(&kPetDpTypes, 56, 0)); // open_video_recording
	dps.emplace_back(DPReport(&kPetDpTypes, 57, 0));	   // basic_nightvision
	dps.emplace_back(DPReport(&kPetDpTypes, 58, 0));			   // basic_osd
	dps.emplace_back(DPReport(&kPetDpTypes, 59, 0));		   // screen_switch
	dps.emplace_back(DPReport(&kPetDpTypes, 60, 0));		   // record_switch
	dps.emplace_back(DPReport(&kPetDpTypes, 61, 0));		   // record_mode
	dps.emplace_back(DPReport(&kPetDpTypes, 62, 0));			   // sd_format
	// dps.emplace_back(PetDPReport(69, status.audio_id));					//audio_id
	dps.emplace_back(DPReport(&kPetDpTypes, 72, 0));		 // factory_reset
	dps.emplace_back(DPReport(&kPetDpTypes, 73, 0));	 // device_restart
	dps.emplace_back(DPReport(&kPetDpTypes, 74, 0)); // switch_perspective
	dps.emplace_back(DPReport(&kPetDpTypes, 75, 0));	 // screen_rotation
	dps.emplace_back(DPReport(&kPetDpTypes, 76, 0));			 // speed_set
	dps.emplace_back(DPReport(&kPetDpTypes, 78, 0));		 // cat_teaser
	dps.emplace_back(DPReport(&kPetDpTypes, 80, 0));		 // power_change
	dps.emplace_back(DPReport(&kPetDpTypes, 83, 0));		 // drive_away
	dps.emplace_back(DPReport(&kPetDpTypes, 84, 0));			 // pitching
	dps.emplace_back(DPReport(&kPetDpTypes, 86, 0));		 // screen_flip

	for (size_t i = 0; i < dps.size(); ++i)
	{
		dev_report_dp_json_async(NULL, &dps[i], 1);
	}
}

void RobotPet::ReportStatus()
{
	tuya_message::Request req = {0};
    tuya_message::RobotState state = {0};
    LOGD(TAG, "获取设备状态");
    if (TuyaComm::Get()->Send("ty_state", &req, &state, 1000))
    {
		OnCruiseStatus(nullptr, "ty_robot_state", &state);
    }
    else
    {
        LOGE(TAG, "无法获取到设备状态");
    }
}

void RobotPet::ReportDeviceInfo()
{
}

void RobotPet::ReportMap()
{
}

void RobotPet::ReportPath()
{
}

void RobotPet::ReportMapAll()
{
}

void RobotPet::ReportAll()
{
	ReportStatus();
	ReportDeviceInfo();
}

bool PetCruiseStartMapping()
{
	json reply;
	if (!PetSendCruiseRequest(json{{"cmd", "start_mapping"}}, &reply))
	{
		return false;
	}

	g_petLastCruiseMode = "mapping";
	PetReportCruiseSwitch(true);
	return true;
}

bool PetCruiseGoto(const mars_message::AppSpotClean &spot)
{
	json reply;
	if (!PetSendCruiseRequest(json{{"cmd", "goto"}, {"x", spot.spot.x}, {"y", spot.spot.y}, {"yaw", 0.0}}, &reply))
	{
		return false;
	}

	PetUpdateCruiseSpotCache(spot);
	g_petLastCruiseMode = "goto";
	PetReportCruiseSwitch(true);
	return true;
}

bool PetCruiseSetPatrol(bool enable)
{
	json reply;
	if (!enable)
	{
		if (!PetSendCruiseRequest(json{{"cmd", "stop"}}, &reply))
		{
			return false;
		}
		g_petLastCruiseMode = "idle";
		PetReportCruiseSwitch(false);
		return true;
	}

	if (!PetSendCruiseRequest(json{{"cmd", "random_patrol"}, {"count", 8}, {"loop", true}}, &reply))
	{
		return false;
	}

	g_petLastCruiseMode = "random_patrol";
	PetReportCruiseSwitch(true);
	return true;
}

bool PetCruiseGetSpot(mars_message::AppSpotClean *spot)
{
	if (spot == NULL)
	{
		return false;
	}

	json reply;
	if (PetSendCruiseRequest(json{{"cmd", "status"}}, &reply))
	{
		mars_message::AppSpotClean currentSpot = {};
		if (PetCruiseStatusToSpot(reply, &currentSpot))
		{
			PetUpdateCruiseSpotCache(currentSpot);
			*spot = g_petCruiseSpot;
			return true;
		}
	}

	if (!g_hasPetCruiseSpot)
	{
		return false;
	}

	*spot = g_petCruiseSpot;
	return true;
}

bool RobotPet::CruiseStartMapping()
{
	return PetCruiseStartMapping();
}

bool RobotPet::CruiseGoto(const mars_message::AppSpotClean &spot)
{
	return PetCruiseGoto(spot);
}

bool RobotPet::CruiseSetPatrol(bool enable)
{
	return PetCruiseSetPatrol(enable);
}

bool RobotPet::CruiseStop()
{
	json reply;
	if (!PetSendCruiseRequest(json{{"cmd", "stop"}}, &reply))
	{
		return false;
	}

	g_petLastCruiseMode = "idle";
	PetReportCruiseSwitch(false);
	return true;
}

bool RobotPet::CruiseGetSpot(mars_message::AppSpotClean *spot)
{
	return PetCruiseGetSpot(spot);
}

void TuyaReportStatus(tuya_message::RobotState status)
{
	g_petRobotState = status;
	PetReportCruiseSwitch(PetIsCruiseActive(g_petLastCruiseMode) || status.cruise_switch);
}

TY_OBJ_DP_S PetDPReport(int dpId, int32_t value)
{
	std::map<int, TuyaType>::const_iterator it = kPetDpTypes.find(dpId);
	if (it == kPetDpTypes.end())
	{
		return DPReportValue(dpId, value);
	}

	switch (it->second)
	{
	case TuyaTypeBool:
		return DPReportBool(dpId, value != 0);
	case TuyaTypeEnum:
		return DPReportEnum(dpId, value);
	case TuyaTypeValue:
		return DPReportValue(dpId, value);
	default:
		return DPReportValue(dpId, value);
	}
}

bool PetReportBoolState(int dpId, bool value)
{
	return PetReportObjectDp(DPReportBool(dpId, value));
}

bool PetReportEnumState(int dpId, int value)
{
	return PetReportObjectDp(DPReportEnum(dpId, value));
}

bool PetReportValueState(int dpId, int value)
{
	return PetReportObjectDp(DPReportValue(dpId, value));
}

bool PetReportStringState(int dpId, const std::string &value)
{
	return PetReportObjectDp(DPReportString(dpId, value.c_str()));
}

bool PetReportRawState(int dpId, const uint8_t *data, int len, int timeoutMs)
{
	if (data == NULL || len <= 0)
	{
		return false;
	}

	OPERATE_RET ret = dev_report_dp_raw_sync(NULL, dpId, data, len, timeoutMs);
	if (ret != OPRT_OK)
	{
		LOGD(TAG, "report raw dp failed, dpId:{}, ret:{}", dpId, ret);
		return false;
	}
	return true;
}

void PetSendTuyaRaw(int dpId, std::vector<uint8_t> *raw)
{
	if (raw == NULL || raw->empty())
	{
		return;
	}

	(*raw)[2] = raw->size() - 3;
	uint8_t sum = 0;
	for (size_t i = 3; i < raw->size(); ++i)
	{
		sum += (*raw)[i];
	}
	raw->push_back(sum);
	PetReportRawState(dpId, &(*raw)[0], raw->size(), 100);
}

void PetReportSpotCleanRaw(int dpId, uint8_t cmd, mars_message::AppSpotClean *spot)
{
	if (spot == NULL)
	{
		return;
	}

	std::vector<uint8_t> raw;
	raw.push_back(0xAA);
	raw.push_back(0x00);
	raw.push_back(0x00);
	raw.push_back(cmd);
	if (spot->version == 0 || cmd == 0x17)
	{
		std::vector<uint8_t> data = tonb16(MarsXToTuya(spot->spot));
		raw.insert(raw.end(), data.begin(), data.end());
		data = tonb16(MarsYToTuya(spot->spot));
		raw.insert(raw.end(), data.begin(), data.end());
	}
	else
	{
		raw.push_back(0x01);
		raw.push_back(spot->mode);
		raw.push_back(spot->suction);
		raw.push_back(spot->cistern);
		raw.push_back(spot->cleanRepeat);
		std::vector<uint8_t> data = tonb16(MarsXToTuya(spot->spot));
		raw.insert(raw.end(), data.begin(), data.end());
		data = tonb16(MarsYToTuya(spot->spot));
		raw.insert(raw.end(), data.begin(), data.end());
	}
	PetSendTuyaRaw(dpId, &raw);
}

void PetReportAutoMappingResultRaw(int dpId, uint8_t cmd, uint8_t ret, uint8_t errCode)
{
	std::vector<uint8_t> raw;
	raw.push_back(0xAA);
	raw.push_back(0x00);
	raw.push_back(0x00);
	raw.push_back(cmd);
	if (ret == 0x00)
	{
		raw.push_back(0x00);
		raw.push_back(errCode);
	}
	else
	{
		raw.push_back(0x01);
	}
	PetSendTuyaRaw(dpId, &raw);
}

int PetGetDpId(void (*handler)(TY_OBJ_DP_S *))
{
	for (size_t i = 0; i < sizeof(kPetDpHandlers) / sizeof(kPetDpHandlers[0]); ++i)
	{
		if (kPetDpHandlers[i].handler == handler)
		{
			return kPetDpHandlers[i].dpId;
		}
	}
	return 0;
}

int PetGetRawDpId(void (*handler)(int dpId, uint8_t *data, int len))
{
	for (size_t i = 0; i < sizeof(kPetRawDpHandlers) / sizeof(kPetRawDpHandlers[0]); ++i)
	{
		if (kPetRawDpHandlers[i].handler == handler)
		{
			return kPetRawDpHandlers[i].dpId;
		}
	}
	return 0;
}

const std::map<int, TuyaType> *RobotPet::DpTypes() const
{
	return &kPetDpTypes;
}

int RobotPet::GetDpId(RobotDpHandler handler) const
{
	return PetGetDpId(handler);
}

int RobotPet::GetRawDpId(RobotRawDpHandler handler) const
{
	return PetGetRawDpId(handler);
}

void PetHandleDPCmd(TY_OBJ_DP_S *dp)
{
	if (dp == NULL)
	{
		return;
	}

	LOGD(TAG, "dp cmd:{}, type:{}", dp->dpid, dp->type);
	for (size_t i = 0; i < sizeof(kPetDpHandlers) / sizeof(kPetDpHandlers[0]); ++i)
	{
		if (kPetDpHandlers[i].dpId == dp->dpid)
		{
			kPetDpHandlers[i].handler(dp);
			return;
		}
	}

	LOGD(TAG, "handler missing, dpId:{}", dp->dpid);
}

void PetHandleRawDPCmd(int dpId, uint8_t *data, int len)
{
	LOGD(TAG, "raw dp cmd:{}, len:{}", dpId, len);
	for (size_t i = 0; i < sizeof(kPetRawDpHandlers) / sizeof(kPetRawDpHandlers[0]); ++i)
	{
		if (kPetRawDpHandlers[i].dpId == dpId)
		{
			kPetRawDpHandlers[i].handler(dpId, data, len);
			return;
		}
	}

	LOGD(TAG, "raw handler missing, dpId:{}", dpId);
}

void RobotPet::HandleDPCmd(TY_OBJ_DP_S *dp)
{
	PetHandleDPCmd(dp);
}

void RobotPet::HandleRawDPCmd(int dpId, uint8_t *data, int len)
{
	PetHandleRawDPCmd(dpId, data, len);
}
