#include "A_ArmUservo.h"
#include "A_ArmKinematic.h"
#include "A_Scscl.h"

void set_servo_angle(A_JointAngkeSt_ *joint_angle,A_RotJointSpeedSt_ *JointSpeedSt,uint8_t dofnum);
/*
	def get_joint_angle(self, joint_name):
		'''��ȡ�ؽڽǶ�'''
		# ���ID 
		servo_id = JOINT_SERVO_ID_DICT[joint_name]
		# ��ѯ���ԭʼ�Ƕ�
		servo_angle = self.get_servo_angle(joint_name)
		# ������Ƕ�ת��Ϊ�ؽڻ���
		joint_angle = (servo_angle - JOINT2SERVO_B[servo_id]) / JOINT2SERVO_K[servo_id]
		# ��ȡ�ؽ�״̬
		return joint_angle
*/		
float get_joint_angle(uint8_t joint_name)
{
	float joint_angle = 0;
	
	return joint_angle;
}
/*
	def get_joint_angle_list(self, joint_name_list=None):
		'''������ȡ�ؽڽǶ�'''
		theta_list = [] # �ؽڻ����б�
		if joint_name_list is not None:
			for joint_name in joint_name_list:
				theta_list.append(self.get_joint_angle(joint_name))
		else:
			for joint_name in self.joint_name_list:
				theta = self.get_joint_angle(joint_name)
				# ���ӵ��ؽ��б�����
				theta_list.append(theta)
		return np.float64(theta_list)
*/			
void get_joint_angle_list(A_JointAngkeSt_ *joint_angle)
{
	joint_angle->joint_1 = get_joint_angle(JOINT1);
	joint_angle->joint_2 = get_joint_angle(JOINT2);
	joint_angle->joint_3 = get_joint_angle(JOINT3);
	joint_angle->joint_4 = get_joint_angle(JOINT4);
	joint_angle->joint_6 = get_joint_angle(JOINT6);
}
/*
	def adjust_tool_posi(self, p_tool:list):
		'''������������'''
		x, y, z = p_tool
		r = np.sqrt(x**2 + y**2)
		k, b = self.z_error_polyfit
		z_error = k*r + b
		return [x, y, z+z_error]
*/
void adjust_tool_posi(A_posSt_ *toolpos,A_posSt_ *adjToolpos)
{
	//'''������������'''
	adjToolpos->x = toolpos->x;
	adjToolpos->y = toolpos->y;
	adjToolpos->z = toolpos->z;
	float r = sqrt((adjToolpos->x*adjToolpos->x) + (adjToolpos->y*adjToolpos->y));
	float k = 0.0394;
	float b = -7.8793;
	float z_error = k*r + b;
	z_error = 0;	
	adjToolpos->z = toolpos->z + z_error;
}
/*
	def auto_gen_pitch(self, p_tool:list):
		'''�Զ����ɸ�����'''
		x, y, z = p_tool
		x0 = np.sqrt(x**2 + y**2)
		return PITCH_PANEL_A*x0 + PITCH_PANEL_B*z + PITCH_PANEL_C
*/

float auto_gen_pitch(A_posSt_ *toolpos)
{
	//'''�Զ����ɸ�����'''
	float x = toolpos->x;
	float y = toolpos->y;
	float z = toolpos->z;
	float x0 = sqrt(x*x + y*y);
	return PITCH_PANEL_A*x0 + PITCH_PANEL_B*z + PITCH_PANEL_C;
}

/*
def set_tool_pose(self, tool_posi=None, tool_pitch=None, \
		tool_roll=np.pi, pose_name=None, is_soft_move=True, T=1.0, \
		is_wait=False, is_debug=False, callback=None):
		'''���ù���λ��'''		
		# �����������λ��
		if pose_name is not None:
			# ʹ��λ������
			if pose_name in ARM_POSE_DICT.keys():
				x, y, z, tool_pitch, tool_roll = ARM_POSE_DICT[pose_name]
				tool_posi = [x, y, z]
			else:
				# δ֪��λ������
				return False
		# �Զ�����Ŀ��λ��
		tool_posi = self.adjust_tool_posi(tool_posi)
		# �Զ�����Pitch
		if tool_pitch is None:
			tool_pitch = self.auto_gen_pitch(tool_posi)
		# �����˶�ѧ
		ret, thetas = self.inverse_kinematic(\
			tool_posi, tool_pitch, tool_roll, is_debug=is_debug)
		# �ж��Ƿ��н�
		if not ret:
			logging.warn('��е��ĩ�˲��ܵ���: {} ������={} �����={}'.format(\
				tool_posi, tool_pitch, tool_roll))
			return False
		# ���ƹؽ�
		# print(f"���ùؽڽǶ�:  {thetas}")
		if is_soft_move:
			self.set_joint_angle_list_soft(thetas, T=T, callback=callback)
		else:
			return self.set_joint_angle_list(thetas, is_wait=is_wait, callback=callback)
*/


float toolPosi_pitch = 0;
bool ArmIkIsSolvable = false;
A_RotPosSt_  setToolRotPos = {0};

void set_tool_pose(A_posSt_ *tool_posi,A_JointAngkeSt_ *joint_angle ,A_RotJointSpeedSt_ *JointSpeedSt,bool toolPitchAuto,uint8_t dofnum)
{
	//�Զ�����Ŀ��λ��
	adjust_tool_posi(tool_posi,&setToolRotPos.posSt);
	//�Զ�����Pitch
	if(toolPitchAuto == true)
	{
		toolPosi_pitch = auto_gen_pitch(tool_posi);	
	}
	else
	{
		toolPosi_pitch = tool_posi->pitch;
	}
	setToolRotPos.posSt.pitch = toolPosi_pitch;
	setToolRotPos.posSt.roll = PI;
	if(dofnum == 5)
	{
		ArmIkIsSolvable = inverse_kinematic(&setToolRotPos,joint_angle ,false);
	}
	if(dofnum == 3)
	{
		ArmIkIsSolvable = inverse_kinematic_3_Dof(&setToolRotPos,joint_angle ,false);
	}



	
	set_servo_angle(joint_angle,JointSpeedSt,dofnum);
}
/*
��е�۶��
joint_1 ��ʱ�룺-��180��˳ʱ�룺+
joint_2 ��ʱ�룺+��180��˳ʱ�룺-
joint_3 ��ʱ�룺-��180��˳ʱ�룺+
joint_4 ��ʱ�룺+��180��˳ʱ�룺-
���
joint_1  ˮƽ����0 ��ʱ�룺+��˳ʱ�룺-
joint_2  ˮƽ����0 ��ʱ�룺+��˳ʱ�룺-
joint_3  ��lin1���� 0 ��ʱ�룺+��˳ʱ�룺-
joint_4  ��lin2���� 0 ��ʱ�룺+��˳ʱ�룺-

*/
A_JointAngkeSt_ ArmMidAngle = {90,270,180,180,180,180};
A_JointAngkeSt_ ServoAngle = {0};
uint8_t set_ID[6] = {0,1,2,3,4,5};
uint16_t set_Posi[6];
uint16_t set_Speed[6] = {800,800,800,1000,400,400};
uint16_t set_Time[6] = {0,0,0,0};

void set_servo_angle(A_JointAngkeSt_ *joint_angle,A_RotJointSpeedSt_ *JointSpeedSt,uint8_t dofnum)
{
	if(dofnum == 5)
	{
		ServoAngle.joint_1 = ArmMidAngle.joint_1 + joint_angle->joint_1;
		ServoAngle.joint_2 = ArmMidAngle.joint_2 + joint_angle->joint_2;
		ServoAngle.joint_3 = ArmMidAngle.joint_3 - joint_angle->joint_3;
		ServoAngle.joint_4 = ArmMidAngle.joint_4 + joint_angle->joint_4;	
		ServoAngle.joint_5 = ArmMidAngle.joint_5 + joint_angle->joint_5;	
		ServoAngle.joint_6 = ArmMidAngle.joint_6 + joint_angle->joint_6;			
	}
	else if(dofnum == 3)
	{
		ServoAngle.joint_1 = 90 + joint_angle->joint_1;
		ServoAngle.joint_2 = 180-3;
		ServoAngle.joint_3 = - joint_angle->joint_3;
		ServoAngle.joint_4 = 360 + joint_angle->joint_4;
		ServoAngle.joint_5 = joint_angle->joint_5;
		ServoAngle.joint_6 = joint_angle->joint_6;
	}
	
	set_Posi[0] = (uint16_t)((ServoAngle.joint_1*4095)/360);
	set_Posi[1] = (uint16_t)((ServoAngle.joint_2*4095)/360);
	set_Posi[2] = (uint16_t)((ServoAngle.joint_3*4095)/360);
	set_Posi[3] = (uint16_t)((ServoAngle.joint_4*4095)/360);
	set_Posi[4] = (uint16_t)((ServoAngle.joint_5*1023)/300);
	set_Posi[5] = (uint16_t)((ServoAngle.joint_6*1023)/300);	

	set_Speed[0] = JointSpeedSt->joint1;
	set_Speed[1] = JointSpeedSt->joint2;
	set_Speed[2] = JointSpeedSt->joint3;
	set_Speed[3] = JointSpeedSt->joint4;
	set_Speed[4] = JointSpeedSt->joint5;
	set_Speed[5] = JointSpeedSt->joint6;	
	
	

//���(ID1/ID2)������ٶ�V=1500*0.059=88.5rpm��������P1=1000λ��
	SyncWritePos(set_ID, 6, set_Posi, set_Time, set_Speed);
			
//	//���(ID1/ID2)������ٶ�V=1500*0.059=88.5rpm��������P1=1000λ��
//	SyncWritePos(set_ID, 4, set_Posi, set_Time, set_Speed);
		
}

A_JointAngkeSt_ ArmOgrAngle = {180,269,355,352,150,26};
//A_JointAngkeSt_ ArmOgrAngle = {180,250,340,340,180,180};
void ArmShowOgrProgram(A_RotJointSpeedSt_ *JointSpeedSt)
{
	set_Posi[0] = (uint16_t)((ArmOgrAngle.joint_1*4095)/360);
	set_Posi[1] = (uint16_t)((ArmOgrAngle.joint_2*4095)/360);
	set_Posi[2] = (uint16_t)((ArmOgrAngle.joint_3*4095)/360);
	set_Posi[3] = (uint16_t)((ArmOgrAngle.joint_4*4095)/360);
	set_Posi[4] = (uint16_t)((ArmOgrAngle.joint_5*1023)/300);
	set_Posi[5] = (uint16_t)((ArmOgrAngle.joint_6*1023)/300);

	
	set_Speed[0] = JointSpeedSt->joint1;
	set_Speed[1] = JointSpeedSt->joint2;
	set_Speed[2] = JointSpeedSt->joint3;
	set_Speed[3] = JointSpeedSt->joint4;
	set_Speed[4] = JointSpeedSt->joint5;
	set_Speed[5] = JointSpeedSt->joint6;	
	
	SyncWritePos(set_ID, 6, set_Posi, set_Time, set_Speed);	
}

void ArmShowUpProgram(A_JointAngkeSt_ *AngleSt,A_RotJointSpeedSt_ *JointSpeedSt)
{
//	A_JointAngkeSt_ ArmUpAngle = {180,160,145,250,180,180};	
//	A_JointAngkeSt_ ArmUpAngle = {180,225,270,315,180,180};	
	
	set_Posi[0] = (uint16_t)((AngleSt->joint_1*4095)/360);
	set_Posi[1] = (uint16_t)((AngleSt->joint_2*4095)/360);
	set_Posi[2] = (uint16_t)((AngleSt->joint_3*4095)/360);
	set_Posi[3] = (uint16_t)((AngleSt->joint_4*4095)/360);

	set_Speed[0] = JointSpeedSt->joint1;
	set_Speed[1] = JointSpeedSt->joint2;
	set_Speed[2] = JointSpeedSt->joint3;
	set_Speed[3] = JointSpeedSt->joint4;	
	
	SyncWritePos(set_ID, 4, set_Posi, set_Time, set_Speed);	
}