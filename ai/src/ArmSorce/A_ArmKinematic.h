#pragma once

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include "RbArmConfig.h"
#include "RbConfig.h"
#include "A_ArmUservo.h"

extern void forward_kinematic_v2(A_JointAngkeSt_ *joint_angle, A_RotPosSt_ *RotPos);
extern bool inverse_kinematic(A_RotPosSt_ *RotPos,A_JointAngkeSt_ *joint_angle ,bool last_joint_angle);;
extern bool inverse_kinematic_3_Dof(A_RotPosSt_ *RotPos,A_JointAngkeSt_ *joint_angle ,bool last_joint_angle);