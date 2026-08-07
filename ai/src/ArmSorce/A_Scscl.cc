#include "A_Scscl.h"
#include "d_Servo.h"

static uint8_t Mem[SCSCL_PRESENT_CURRENT_H-SCSCL_PRESENT_POSITION_L+1];

uint8_t SCID = 0;
uint16_t SCPosition = 0;
uint16_t SCTime = 0;
uint16_t SCSpeed = 0;
uint8_t SCbBuf[6];
int WritePos(uint8_t ID, uint16_t Position, uint16_t Time, uint16_t Speed)
{
//	uint8_t bBuf[6];
	SCID = ID;
	SCPosition = Position;
	SCTime = Time;
	SCSpeed = Speed;
	Host2SCS(SCbBuf+0, SCbBuf+1, Position);
	Host2SCS(SCbBuf+2, SCbBuf+3, Time);
	Host2SCS(SCbBuf+4, SCbBuf+5, Speed);
	
	return genWrite(ID, SCSCL_GOAL_POSITION_L, SCbBuf, 6);
}

//**Position位置 Time时间  Speed 速度
int RegWritePos(uint8_t ID, uint16_t Position, uint16_t Time, uint16_t Speed)
{
	uint8_t bBuf[6];
	Host2SCS(bBuf+0, bBuf+1, Position);
	Host2SCS(bBuf+2, bBuf+3, Time);
	Host2SCS(bBuf+4, bBuf+5, Speed);
	
	return regWrite(ID, SCSCL_GOAL_POSITION_L, bBuf, 6);
}

//**Position位置 Time时间  Speed 速度
int RegWritePosAngle(uint8_t ID, float Angle, uint16_t Time, uint16_t Speed)
{
	uint16_t Pos = (uint16_t)((Angle*4095)/360);	
	return RegWritePos(ID, Pos,  Time,  Speed);
}

void RegWriteAction()
{
	regAction(0xfe);
}

void SyncWritePos(uint8_t ID[], uint8_t IDN, uint16_t Position[], uint16_t Time[], uint16_t Speed[])
{
	uint8_t offbuf[32*6];
	uint8_t i;
	for(i = 0; i<IDN; i++)
	{
		uint16_t T, V;
		if(Time)
		{
			T = Time[i];
		}
		else
		{
			T = 0;
		}
		if(Speed)
		{
			V = Speed[i];
		}
		else
		{
			V = 0;
		}
		if(i < 4)
		{
			setEnd(0);//SMS_STS舵机为大端存储结构			
		}
		else
		{
			setEnd(1);//SMS_STS舵机为大端存储结构			
		}
		
		Host2SCS(offbuf+i*6+0, offbuf+i*6+1, Position[i]);
		Host2SCS(offbuf+i*6+2, offbuf+i*6+3, T);
		Host2SCS(offbuf+i*6+4, offbuf+i*6+5, V);
	}
	syncWrite(ID, IDN, SCSCL_GOAL_POSITION_L, offbuf, 6);
}

void SyncWritePosAngle(uint8_t ID[], uint8_t IDN, uint16_t Angle[], uint16_t Time[], uint16_t Speed[])
{
	uint16_t Position[32];
	
	for(int i = 0; i<IDN; i++)
	{
		Position[i] = (uint16_t)((Angle[i]*4095)/360);
	}
	SyncWritePos(ID, IDN, Position,Time,Speed);
}

int FeedBack(int ID)
{
	int nLen = Read(ID, SCSCL_PRESENT_POSITION_L, Mem, sizeof(Mem));
	if(nLen!=sizeof(Mem)){
		return -1;
	}
	return nLen;
}

int ReadPos(int ID)
{
	int Pos = -1;
	if(ID==-1){
		Pos = Mem[SCSCL_PRESENT_POSITION_L-SCSCL_PRESENT_POSITION_L];
		Pos <<= 8;
		Pos |= Mem[SCSCL_PRESENT_POSITION_H-SCSCL_PRESENT_POSITION_L];
	}else{
		Pos = readWord(ID, SCSCL_PRESENT_POSITION_L);
	}
	if(Pos&(1<<15)){
		Pos = -(Pos&~(1<<15));
	}	
	return Pos;
}

int ReadSpeed(int ID)
{
	int Speed = -1;
	if(ID==-1){
		Speed = Mem[SCSCL_PRESENT_SPEED_L-SCSCL_PRESENT_POSITION_L];
		Speed <<= 8;
		Speed |= Mem[SCSCL_PRESENT_SPEED_H-SCSCL_PRESENT_POSITION_L];
	}else{
		Speed = readWord(ID, SCSCL_PRESENT_SPEED_L);
	}
	if(Speed&(1<<15)){
		Speed = -(Speed&~(1<<15));
	}	
	return Speed;
}

int ReadLoad(int ID)
{
	int Load = -1;
	if(ID==-1){
		Load = Mem[SCSCL_PRESENT_LOAD_L-SCSCL_PRESENT_POSITION_L];
		Load <<= 8;
		Load |= Mem[SCSCL_PRESENT_LOAD_H-SCSCL_PRESENT_POSITION_L];
	}else{
		Load = readWord(ID, SCSCL_PRESENT_LOAD_L);
	}
	if(Load&(1<<10)){
		Load = -(Load&~(1<<10));
	}
	return Load;
}

int ReadVoltage(int ID)
{
	int Voltage = -1;
	if(ID==-1){
		Voltage = Mem[SCSCL_PRESENT_VOLTAGE-SCSCL_PRESENT_POSITION_L];	
	}else{
		Voltage = readByte(ID, SCSCL_PRESENT_VOLTAGE);
	}
	return Voltage;
}

int ReadTemper(int ID)
{
	int Temper = -1;
	if(ID==-1){
		Temper = Mem[SCSCL_PRESENT_TEMPERATURE-SCSCL_PRESENT_POSITION_L];	
	}else{
		Temper = readByte(ID, SCSCL_PRESENT_TEMPERATURE);
	}
	return Temper;
}

int ReadMove(int ID)
{
	int Move = -1;
	if(ID==-1){
		Move = Mem[SCSCL_MOVING-SCSCL_PRESENT_POSITION_L];	
	}else{
		Move = readByte(ID, SCSCL_MOVING);
	}
	return Move;
}

int ReadCurrent(int ID)
{
	int Current = -1;
	if(ID==-1){
		Current = Mem[SCSCL_PRESENT_CURRENT_H-SCSCL_PRESENT_POSITION_L];
		Current <<= 8;
		Current |= Mem[SCSCL_PRESENT_CURRENT_L-SCSCL_PRESENT_POSITION_L];
	}else{
		Current = readWord(ID, SCSCL_PRESENT_CURRENT_L);
	}
	if(Current&(1<<15)){
		Current = -(Current&~(1<<15));
	}	
	return Current;
}

uint8_t servoID = 0;
int servoPos = 0;
int servoAngle = 0;
int ReadPosAngle(int ID)
{
	
	switch(ID)
	{
		case 0:
			servoAngle = (ReadPos(ID)*360)/4095;	
			return servoAngle-90;			
		break;
		case 1:
			servoAngle = (ReadPos(ID)*360)/4095;	
			return servoAngle-270;			
		break;
		case 2:
			servoAngle = (ReadPos(ID)*360)/4095;	
			return 180-servoAngle;			
		break;
		case 3:
			servoAngle = (ReadPos(ID)*360)/4095;	
			return servoAngle-180;			
		break;
		case 4:
			servoAngle = ReadPos(ID);	
			return servoAngle;			
		break;
		case 5:
			servoAngle = ReadPos(ID);
			return servoAngle;			
		break;		
	}
}