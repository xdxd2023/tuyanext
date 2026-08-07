#pragma once

#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "RbArmConfig.h"
#include "RbConfig.h"
#include "A_Scscl.h"

extern float get_joint_angle(uint8_t joint_name);
extern void get_joint_angle_list(A_JointAngkeSt_ *joint_angle);
extern void set_tool_pose(A_posSt_ *tool_posi,A_JointAngkeSt_ *joint_angle ,A_RotJointSpeedSt_ *JointSpeedSt,bool toolPitchAuto,uint8_t dofnum);
extern void ArmShowOgrProgram(A_RotJointSpeedSt_ *JointSpeedSt);
extern void ArmShowUpProgram(A_JointAngkeSt_ *AngleSt,A_RotJointSpeedSt_ *JointSpeedSt);
extern void set_servo_angle(A_JointAngkeSt_ *joint_angle,A_RotJointSpeedSt_ *JointSpeedSt,uint8_t dofnum);