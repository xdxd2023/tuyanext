#ifndef _A_SCSCL_H_
#define _A_SCSCL_H_

#include <stdint.h>
#include <stdbool.h>

//�����ʶ���
#define SCSCL_1M 0
#define	SCSCL_0_5M 1
#define	SCSCL_250K 2
#define	SCSCL_128K 3
#define	SCSCL_115200 4
#define	SCSCL_76800 5
#define	SCSCL_57600	6
#define	SCSCL_38400	7

//�ڴ������
//-------EPROM(ֻ��)--------
#define SCSCL_MODEL_L 3
#define SCSCL_MODEL_H 4

//-------EPROM(��д)--------
#define SCSCL_ID 5
#define SCSCL_BAUD_RATE 6
#define SCSCL_MIN_ANGLE_LIMIT_L 9
#define SCSCL_MIN_ANGLE_LIMIT_H 10
#define SCSCL_MAX_ANGLE_LIMIT_L 11
#define SCSCL_MAX_ANGLE_LIMIT_H 12
#define SCSCL_CW_DEAD 26             //**˳ʱ�벻������
#define SCSCL_CCW_DEAD 27            //**��ʱ�벻������

//-------SRAM(��д)--------
#define SCSCL_TORQUE_ENABLE 40   //**Ť�ؿ���
#define SCSCL_GOAL_POSITION_L 42 //**Ŀ��λ��
#define SCSCL_GOAL_POSITION_H 43 
#define SCSCL_GOAL_TIME_L 44     //**����ʱ��
#define SCSCL_GOAL_TIME_H 45 
#define SCSCL_GOAL_SPEED_L 46    //**�����ٶ�
#define SCSCL_GOAL_SPEED_H 47
#define SCSCL_LOCK 48

//-------SRAM(ֻ��)--------
#define SCSCL_PRESENT_POSITION_L 56   //**��ǰλ�� 0x38
#define SCSCL_PRESENT_POSITION_H 57   //**��ǰλ��
#define SCSCL_PRESENT_SPEED_L 58      //**��ǰ�ٶ�
#define SCSCL_PRESENT_SPEED_H 59      //**��ǰ�ٶ�
#define SCSCL_PRESENT_LOAD_L 60       //**��ǰ����
#define SCSCL_PRESENT_LOAD_H 61       //**��ǰ����
#define SCSCL_PRESENT_VOLTAGE 62      //**��ǰ��ѹ
#define SCSCL_PRESENT_TEMPERATURE 63  //**��ǰ�¶�
#define SCSCL_MOVING 66   //**�ƶ���־λ��������˶�ʱ��־Ϊ1�����ֹͣʱΪ0
#define SCSCL_PRESENT_CURRENT_L 69    //**��ǰ����
#define SCSCL_PRESENT_CURRENT_H 70    //**��ǰ����

extern int WritePos(uint8_t ID, uint16_t Position, uint16_t Time, uint16_t Speed);
extern int RegWritePos(uint8_t ID, uint16_t Position, uint16_t Time, uint16_t Speed);
extern int RegWritePosAngle(uint8_t ID, float Angle, uint16_t Time, uint16_t Speed);
extern void RegWriteAction();
extern void SyncWritePos(uint8_t ID[], uint8_t IDN, uint16_t Position[], uint16_t Time[], uint16_t Speed[]);
extern void SyncWritePosAngle(uint8_t ID[], uint8_t IDN, uint16_t Angle[], uint16_t Time[], uint16_t Speed[]);
extern int ReadPos(int ID);
extern int ReadSpeed(int ID);
extern int ReadLoad(int ID);
extern int ReadVoltage(int ID);
extern int ReadTemper(int ID);
extern int ReadMove(int ID);

extern int ReadPosAngle(int ID);

#endif
