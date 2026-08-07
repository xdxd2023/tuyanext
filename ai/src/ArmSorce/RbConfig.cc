#include "RbConfig.h"

LEGS_COORDINATE_ last_coordinate={0};
LEGS_COORDINATE_ target_coordinate={0};

//=======�˶�ѧ����ó��ĸ��ؽڵĽǶ�ֵ=============
float rf_gamma;
float rf_alfa;
float rf_beta;

float lf_gamma;
float lf_alfa;
float lf_beta;

float rb_gamma;
float rb_alfa;
float rb_beta;

float lb_gamma;
float lb_alfa;
float lb_beta;

LEG_JOINT_ leg_joint_int =
{
	.rf_gamma=RF_G_INIT,  //��λ ��  ��   90
	.rf_alfa =RF_A_INIT,  //��λ ��  ���� 55
	.rf_beta =RF_B_INIT,   //��λ ��  С�� 90
	
	.rb_gamma=RB_G_INIT, //��λ ��  ��
	.rb_alfa =RB_A_INIT,  //��λ ��  ����
	.rb_beta =RB_B_INIT,  //��λ ��  С��  
	
	.lf_gamma=LF_G_INIT, //��λ ��  ��      90
	.lf_alfa =LF_A_INIT,  //��λ ��  ����   125
	.lf_beta =LF_B_INIT,  //��λ ��  С��    90
	
	.lb_gamma=LB_G_INIT, //��λ ��  ��
	.lb_alfa =LB_A_INIT,  //��λ ��  ����
	.lb_beta =LB_B_INIT //��λ ��  С��
};