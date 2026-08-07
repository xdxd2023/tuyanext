#include "A_ArmKinematic.h"
#include "A_Transform.h"
#include "A_ArmUservo.h"

float fk_theta1 = 0;
float fk_theta2 = 0;
float fk_theta3 = 0;
float fk_theta4 = 0;
float fk_theta6 = 0;
float fk_theta23 = 0;
float fk_theta234 = 0;

void forward_kinematic_v2(A_JointAngkeSt_ *joint_angle, A_RotPosSt_ *RotPos)
{
//	float fk_theta1 = 0;
//	float fk_theta2 = 0;
//	float fk_theta3 = 0;
//	float fk_theta4 = 0;
//	float fk_theta6 = 0;
//	float fk_theta23 = 0;
//	float fk_theta234 = 0;

	float fk_s1 = 0;
	float fk_s2 = 0;
	float fk_s3 = 0;
	float fk_s4 = 0;
	float fk_s6 = 0;

	float fk_c1 = 0;
	float fk_c2 = 0;
	float fk_c3 = 0;
	float fk_c4 = 0;
	float fk_c6 = 0;

	float fk_s23 = 0;
	float fk_c23 = 0;

	float fk_s234 = 0;
	float fk_c234 = 0;

	float fk_x60 = 0;
	float fk_x6 = 0;
	float fk_y6 = 0;
	float fk_z6 = 0;	
	
// ��ȡ�ؽڽǶ�
	fk_theta1 = (joint_angle->joint_1*pi)/180;
	fk_theta2 = (joint_angle->joint_2*pi)/180;
	fk_theta3 = (joint_angle->joint_3*pi)/180;
	fk_theta4 = (joint_angle->joint_4*pi)/180;
	fk_theta6 = (joint_angle->joint_6*pi)/180;
//	# ���ϽǶ�
	fk_theta23 = fk_theta2 + fk_theta3;
	fk_theta234 = fk_theta23 + fk_theta4;
	
//	# Ԥ�ȼ������Ǻ���
	fk_s1 = sin(fk_theta1);
	fk_s2 = sin(fk_theta2);
	fk_s3 = sin(fk_theta3);
	fk_s4 = sin(fk_theta4);
	fk_s6 = sin(fk_theta6);
	
	fk_c1 = cos(fk_theta1);
	fk_c2 = cos(fk_theta2);
	fk_c3 = cos(fk_theta3);
	fk_c4 = cos(fk_theta4);
	fk_c6 = cos(fk_theta6);
	
	fk_s23 = sin(fk_theta23);
	fk_c23 = cos(fk_theta23);
	
	fk_s234 = sin(fk_theta234);
	fk_c234 = cos(fk_theta234);
	
//	# ��ά����
	fk_x60 = A1 + A2*fk_c2 + A3*fk_c23 + A4*fk_c234;
	fk_x6 = fk_x60 * fk_c1;
	fk_y6 = fk_x60 * fk_s1;
	fk_z6 = D1 - A2*fk_s2 - A3*fk_s23 - A4*fk_s234;
	
//	# �����ת����
	RotPos->rmat[0][0] = fk_s1*fk_s6 + fk_c1*fk_s234*fk_c6;
	RotPos->rmat[0][1] = fk_s1*fk_c6 - fk_c1*fk_s234*fk_s6;
	RotPos->rmat[0][2] = fk_c1*fk_c234;
	
	RotPos->rmat[1][0] = fk_s1*fk_s234*fk_c6 - fk_c1*fk_s6;
	RotPos->rmat[1][1] = - fk_s1*fk_s234*fk_s6 - fk_c1*fk_c6;
	RotPos->rmat[1][2] = fk_s1*fk_c234;	

	RotPos->rmat[2][0] = fk_c234*fk_c6;
	RotPos->rmat[2][1] = - fk_c234*fk_s6;
	RotPos->rmat[2][2] = - fk_s234;	

//	# ����λ�˶���
	rmat2euler(RotPos->posSt.roll,RotPos->posSt.pitch,RotPos->posSt.yaw,RotPos->rmat);
	RotPos->posSt.x = fk_x6;
	RotPos->posSt.y = fk_y6;
	RotPos->posSt.z = fk_z6;
}


bool is_joint_angle_legal(float angle, uint8_t joint_index)
{
	if((angle >= JOINT_ANGLE_LOWERB[joint_index]) && (angle <= JOINT_ANGLE_UPPERB[joint_index]))
	{
		return true;
	}
	else
	{
		return false;
	}
}
		
typedef struct{
	float theta;
	bool Solvable;	
}A_ThetaSolveSt_;

typedef struct{
	float angle;
	float theta2;
	float theta4;
}A_Theta3St_;

A_Theta3St_ theta3St_1;
A_Theta3St_ theta3St_2;

float theta1 = 0;
float theta2 = 0;
float theta3 = 0;
float theta4 = 0;
float theta6 = 0;
float theta234 = 0;
float theta1_1 = 0;
float theta1_2 = 0;
float theta3_1 = 0;
float theta3_2 =0;
//'''�����˶�ѧ'''
bool inverse_kinematic(A_RotPosSt_ *RotPos,A_JointAngkeSt_ *joint_angle ,bool last_joint_angle)
{
	A_ThetaSolveSt_ Theta_1St[2] ={0};
	A_ThetaSolveSt_ Theta_3St[2] ={0};
	bool A_IKSolvable = false;
//	float theta1 = 0;
//	float theta2 = 0;
//	float theta3 = 0;
//	float theta4 = 0;
//	float theta6 = 0;
//	float theta234 = 0;
	float c234 = 0;
	float s234 = 0;
	float x_16 = 0;
	float z_16 = 0;
//	float theta1_1 = 0;
//	float theta1_2 = 0;
	float c1 = 0;
	float s1 = 0;
	float b1 = 0;
	float b2 = 0;
	float c3 = 0;
	float s3_pw2 = 0;
	float s3_abs = 0;
//	float theta3_1 = 0;
//	float theta3_2 =0;
	float K1 = 0;
	float K2 = 0;
	A_RotPosSt_ ikRotPos = {0};

	float ik_theta1 = 0;
	float ik_theta2 = 0;
	float ik_theta3 = 0;
	float ik_theta4 = 0;
	float ik_theta6 = 0;	
	
	A_IKSolvable = false;
	A_JointAngkeSt_ joint_angle_ik={0};
	ikRotPos.posSt.x = RotPos->posSt.x;
	ikRotPos.posSt.y = RotPos->posSt.y;
	ikRotPos.posSt.z = RotPos->posSt.z;
	
	//# ��һ�εĹؽڽǶ�
	if(last_joint_angle == true)
	{ 
	//# ��ȡ�ϴεĹؽڽǶ�
		get_joint_angle_list(&joint_angle_ik);				
	}
	else
	{
		joint_angle_ik.joint_1 = JOINT_ANGLE_DEFAULT.joint_1;
		joint_angle_ik.joint_2 = JOINT_ANGLE_DEFAULT.joint_2;
		joint_angle_ik.joint_3 = JOINT_ANGLE_DEFAULT.joint_3;
		joint_angle_ik.joint_4 = JOINT_ANGLE_DEFAULT.joint_4;
		joint_angle_ik.joint_5 = JOINT_ANGLE_DEFAULT.joint_5;
		joint_angle_ik.joint_6 = JOINT_ANGLE_DEFAULT.joint_6;
	}
//		# ��ѡ�ؽڽǶ�
//		candi_joint_angle_list = []
	//# ���theta6
	theta6 = RotPos->posSt.roll - PI;
//		theta6 = RotPos->posSt.roll;
	if(theta6 < MINUS_PI)
	{
		theta6 += PI_M_2;
	}
	if(is_joint_angle_legal(theta6, JOINT6) == false)
	{
		return false;
	}

//		# ���theta234
//	theta234 = RotPos->posSt.pitch - (PI_D_2);
		theta234 = RotPos->posSt.pitch;
	c234 = A_round(cos(theta234));
	s234 = A_round(sin(theta234));
//		# ���ؽ�1�ĽǶ�

	if (ikRotPos.posSt.x == 0.0 && ikRotPos.posSt.y == 0.0)
	{
		theta1_1 = joint_angle_ik.joint_1;
	}
	else
	{
		theta1_1 = atan2(ikRotPos.posSt.y , ikRotPos.posSt.x);
		theta1_2 = theta1_1 + PI;
		if(theta1_2 > PI)
		{
			theta1_2 -= PI_M_2;
		}
		//for theta1 in self.joint_angle_filter(\theta1_list, self.JOINT1, is_debug):
		//theta1_1,theta1_2 ��С��������
		if(theta1_1 < theta1_2)
		{
			Theta_1St[0].theta = theta1_1;
			Theta_1St[1].theta = theta1_2;			
		}
		else
		{
			Theta_1St[0].theta = theta1_2;
			Theta_1St[1].theta = theta1_1;			
		}			
	}
	for(int i = 0;i < 2; i++)
	{
		if(is_joint_angle_legal(Theta_1St[i].theta, JOINT1) == true)
		{
			c1 = cos(Theta_1St[i].theta);
			s1 = sin(Theta_1St[i].theta);
			// ���ؽ�6�ڹؽ�1����ϵ�µ�����
			x_16 = c1*RotPos->posSt.x + s1*RotPos->posSt.y;
			z_16 = RotPos->posSt.z - D1;
			//����b1, b2 
			b1 = x_16 - A1 - A4 * c234;
			b2 = -z_16  - A4 * s234;
			c3 = (b1*b1 + b2*b2 - A2*A2 - A3*A3) / (2*A2*A3);
			//��ֵԭ�� cos(theta3)���ܻ���1.0������Ϊ�˱��ж�Ϊ�Ƿ�, ����������⴦����c3 = np.round(c3, 10) //C3ȡ��С�����ʮλ

			s3_pw2 = 1.0 - c3*c3; //np.round(1.0 - c3**2, 10)
//				if(fabs(c3) > 1.0 || fabs(s3_pw2) > 1.0)
//				{
//					//�Ƿ�cosֵ, �������ﲻ�����˳�����forѭ��
//					continue;
//				}
//				else //**��Ч��ͼ�������
			{				
				s3_abs = sqrt(s3_pw2);
				theta3_1 = atan2(s3_abs, c3);
				theta3_2 = atan2(-s3_abs, c3);
				
				theta3St_1.angle = atan2(s3_abs, c3);
				theta3St_2.angle = atan2(-s3_abs, c3);

				//���theta2
				//for theta3 in self.joint_angle_filter([theta3_1, theta3_2], self.JOINT3, is_debug):
				//theta3_1,theta3_2 ��С��������	
				if(theta3_1 < theta3_2)
				{
					Theta_3St[0].theta = theta3_2;
					Theta_3St[1].theta = theta3_1;									
				}
				else
				{
					Theta_3St[0].theta = theta3_1;
					Theta_3St[1].theta = theta3_2;									
				}
				
//				K1 = A2 + A3 * c3;
//				K2 = A3 * sin(theta3St_1.angle);
//				theta3St_1.theta2 = atan2(b2, b1) - atan2(K2, K1);	
//				theta3St_1.theta4 = theta234 - theta3St_1.theta2 - theta3St_1.angle;
//				
//				K2 = A3 * sin(theta3St_2.angle);
//				theta3St_2.theta2 = atan2(b2, b1) - atan2(K2, K1);	
//				theta3St_2.theta4 = theta234 - theta3St_2.theta2 - theta3St_2.angle;
				
				
				
				for(int j = 0;j < 2; j++)
				{
					if(is_joint_angle_legal(Theta_3St[j].theta, JOINT3) == true)
					{
						K1 = A2 + A3 * c3;
						K2 = A3 * sin(Theta_3St[j].theta);
						theta2 = atan2(b2, b1) - atan2(K2, K1);							
						
						if(is_joint_angle_legal(theta2, JOINT2) == false)	 //**�ж�theta2�ĺϷ���		
						{
							//**��Ч�� �˳�����forѭ��
							continue;
						}
						else
						{
							//**��Ч��
							//���theta4
							theta4 = theta234 - theta2 - Theta_3St[j].theta;
							
							if(is_joint_angle_legal(theta4, JOINT4) == false) //**�ж�theta4�ĺϷ���				
							{
								continue; //�˳�����forѭ��
							}
							else
							{
								A_IKSolvable = true;
//									joint_angle->joint_1 = Theta_1St[i].theta;
//									joint_angle->joint_2 = theta2;
//									joint_angle->joint_3 = Theta_3St[j].theta;
//									joint_angle->joint_4 = theta4;
//									joint_angle->joint_6 = theta6;	

								joint_angle->joint_1 = (Theta_1St[i].theta*180)/pi;
								joint_angle->joint_2 = (theta2*180)/pi;
								joint_angle->joint_3 = (Theta_3St[j].theta*180)/pi;
								joint_angle->joint_4 = (theta4*180)/pi;
								joint_angle->joint_6 = (theta6*180)/pi;									
								
								ik_theta1 = joint_angle->joint_1;
								ik_theta2 = joint_angle->joint_2;
								ik_theta3 = joint_angle->joint_3;
								ik_theta4 = joint_angle->joint_4;
								ik_theta6 = joint_angle->joint_6;
								
								
							}					
							break;
						}
					}
				}
			}				
		}
		if(A_IKSolvable == true) //**ֻҪ��һ��theta1�н�����
		{
			break;
		}			
	}
	return A_IKSolvable;
}

#define BASE_H  D1+A2

float three_dof_L = 0; //xyƽ��ͶӰ
float three_dof_L1 = 0; //���
float three_dof_L2 = 0; //С��
//float three_dof_B = 0;  //����
float three_dof_H = 0; //б��
float three_dof_S = 0; //�̱�
float three_dof_alpha = 0;
float three_dof_beta = 0;
float three_dof_gamma = 0;
float three_dof_beta_1 = 0;
float three_dof_beta_2 = 0;
float three_dof_angle_alpha = 0;
float three_dof_angle_beta = 0;
float three_dof_angle_gamma = 0;


bool inverse_kinematic_3_Dof(A_RotPosSt_ *RotPos,A_JointAngkeSt_ *joint_angle ,bool last_joint_angle)
{
	//����ͶӰ����
	three_dof_L = sqrtf((RotPos->posSt.x * RotPos->posSt.x) + (RotPos->posSt.y * RotPos->posSt.y));
	//����̱߳���
	three_dof_S = fabs(BASE_H - RotPos->posSt.z);
	//����б�߳���
	three_dof_H = sqrtf((three_dof_S * three_dof_S) + (three_dof_L * three_dof_L));
	
	three_dof_alpha = atan2(RotPos->posSt.y,RotPos->posSt.x);
	
	three_dof_beta_1 = acos(((A3 * A3)+(three_dof_H * three_dof_H)-(A4 * A4))/(2*A3*three_dof_H));
	
    //�Ե����߶Ⱥ�z�᳤���жϣ���Ϊ���������
	if(BASE_H == RotPos->posSt.z)
	{
		three_dof_beta_2 = 0;
	}
	else if(BASE_H > RotPos->posSt.z)
	{
		three_dof_beta_2 = atan2(three_dof_L,three_dof_S);
	}
	else if(BASE_H < RotPos->posSt.z)
	{
		three_dof_beta_2 = atan2(three_dof_S,three_dof_L) + PI_D_2;
	}
	three_dof_beta = -(three_dof_beta_1 +three_dof_beta_2);
	
	three_dof_gamma = -acos(((A3 * A3) + (A4 * A4) - (three_dof_H * three_dof_H))/(2*A3*A4));
	
//	three_dof_angle_alpha = ((three_dof_alpha * 180)/pi) + 90;
//	three_dof_angle_beta = ((three_dof_beta * 180)/pi) + 180;
//	three_dof_angle_gamma =((three_dof_gamma * 180)/pi) + 180;
	
	three_dof_angle_alpha = (three_dof_alpha * 180)/pi;
	three_dof_angle_beta = (three_dof_beta * 180)/pi;
	three_dof_angle_gamma =(three_dof_gamma * 180)/pi;	
	
	joint_angle->joint_1 = three_dof_angle_alpha;
	joint_angle->joint_2 = 90;
	joint_angle->joint_3 = three_dof_angle_beta;
	joint_angle->joint_4 = three_dof_angle_gamma;
	
	return 1;
}