#pragma once

#include <math.h>
#include <stdint.h>
#include <stdlib.h>


#define  pi 3.1415926f
#define  TS 1  //����  �ڶ����֧��������ں�
#define  FI 0.5f //֧�����������е�ռ�ձ�
#define  TM  TS*FI      //�ڶ��������

#define RF_G 0 
#define RF_A 1
#define RF_B 2

#define RB_G 3
#define RB_A 4
#define RB_B 12

#define LF_G 11
#define LF_A 12
#define LF_B 13

#define LB_G 8
#define LB_A 9
#define LB_B 10


#define  FOOT_LENGTH 196.0f//ǰ�������ľ���
#define  FOOT_WIDTH  166.0f//���������ľ���
#define  BODY_WIDTH  90.0f//�����������ԭ���ľ���

#define  HU 60.0f   //���ȳ���
#define  HL 60.0f  //С�ȳ���
#define  STAND_HEIGHT  84.85f //sqrt(HU*HU+HL*HL)  //վ���ĸ߶�

#define  H          38
#define  H_OFFSET   0//20 //y��ƫ����

#define ROLL_MAX 40
#define ROLL_MIN -ROLL_MAX
#define PITCH_MAX 25
#define PITCH_MIN -PITCH_MAX
#define YALL_MAX 40
#define YALL_MIN -YALL_MAX
#define POSX_MAX 40
#define POSX_MIN -POSX_MAX
#define POSY_MAX 40
#define POSY_MIN -POSY_MAX
#define POSZ_MAX 30
#define POSZ_MIN -POSZ_MAX


#define RF_G_INIT 90     
#define RF_A_INIT 135 
#define RF_B_INIT 90

#define LF_G_INIT 90    
#define LF_A_INIT 45 
#define LF_B_INIT 90  

#define RB_G_INIT 90
#define RB_A_INIT 135
#define RB_B_INIT 90

#define LB_G_INIT 90     
#define LB_A_INIT 45
#define LB_B_INIT 90

typedef enum
{
	CALI=0,
	STAND,
	STEP,
    TROT_BACK,
	TROT_FORWARD,
    TROT_BACK_FAST,
	TROT_FORWARD_FAST,	
	WALKFORWARD,
	WALKBACK,
	LEFT,
	RIGHT,
	ROTATE_LEFT,
	ROTATE_RIGHT,
	TROT_FORWARD_TURN_LEFT,
	TROT_FORWARD_TURN_RIGHT,
	TROT_BACK_TURN_LEFT,
	TROT_BACK_TURN_RIGHT,
	YALL_TURN_LEFT,
	YALL_TURN_RIGHT,
    POSTURE	,
}MODE_Enum;

typedef enum
{
    lf=0,
	lb,
	rb,
	rf,
}LEGS_Enum;


typedef struct
{
	float rf_x;
	float rf_y;
	float rf_z;
	
	float rb_x;
	float rb_y;
	float rb_z;

	float lf_x;
	float lf_y;
	float lf_z;

	float lb_x;
	float lb_y;
	float lb_z;	
}LEGS_COORDINATE_;

typedef struct
{
	float rf_gamma;
	float rf_alfa;
	float rf_beta;
	
	float rb_gamma;
	float rb_alfa;
	float rb_beta;

	float lf_gamma;
	float lf_alfa;
	float lf_beta;

	float lb_gamma;
	float lb_alfa;
	float lb_beta;	
}LEG_JOINT_;


extern float rf_gamma;
extern float rf_alfa;
extern float rf_beta;

extern float lf_gamma;
extern float lf_alfa;
extern float lf_beta;

extern float rb_gamma;
extern float rb_alfa;
extern float rb_beta;

extern float lb_gamma;
extern float lb_alfa;
extern float lb_beta;

extern LEGS_COORDINATE_ last_coordinate;
extern LEGS_COORDINATE_ target_coordinate;
extern LEG_JOINT_ leg_joint_int;
