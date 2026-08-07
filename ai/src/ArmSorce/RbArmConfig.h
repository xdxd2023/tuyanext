#pragma once

//�ؽڱ��?
#define	JOINT1   0
#define	JOINT2   1
#define	JOINT3   2
#define	JOINT4   3
#define	JOINT6   4

// DH�� ���˳��ȶ���, ��λmm
//#define	D1  70.00  //**�����߶�
//#define	A1  0.00   //**����ת������ת������˳���?
//#define	A2  80.00  //**������˳���?
//#define	A3  76.15  //**С�����˳���
//#define	A4  185.40 //**ǰ��+��еצ����

#define	D1  42.00  //**�����߶�
#define	A1  0.00   //**����ת������ת������˳���?
#define	A2  90.00  //**������˳���?
#define	A3  180.00  //**С�����˳���
#define	A4  180.00 //**ǰ��+��еצ����


#define	PI             3.1415926f
#define	MINUS_PI       -3.1415926f
#define	PI_M_2         PI * 2.0 
#define	PI_D_2         PI / 2.0 // pi/2
#define	MIMUS_PI_D_2   -1.0 * PI_D_2 // -pi/2

//Pitchƽ����Ͻ��
#define	PITCH_PANEL_A   -0.002490
#define	PITCH_PANEL_B   -0.004985
#define	PITCH_PANEL_C   3.048636


typedef struct{
	float joint_1;
	float joint_2;
	float joint_3;
	float joint_4;
	int joint_5;
	int joint_6;
}A_JointAngkeSt_;

typedef struct{
	float roll;
	float pitch;
	float yaw;
	float x;
	float y;
	float z;
}A_posSt_;

typedef struct{
	float rmat[3][3];
	A_posSt_ posSt;
}A_RotPosSt_;

typedef struct{
	int joint1;
	int joint2;
	int joint3;
	int joint4;
	int joint5;	
	int joint6;
}A_RotJointSpeedSt_;

//'''Arm5DoFЭ����е���˶�ѧ'''
//# �ؽڽǶ�����, ��λ: ����
extern float JOINT_ANGLE_LOWERB[6];
//# �ؽڽǶ�������, ��λ: ����
extern float JOINT_ANGLE_UPPERB[6];
//	# Ĭ�ϹؽڽǶ� ��λ: ����
//	# ע: �Ƕ�ת���� 
extern A_JointAngkeSt_ JOINT_ANGLE_DEFAULT ;