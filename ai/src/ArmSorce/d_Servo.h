#pragma once

#include <stdint.h>

#include "armTask.h"

#ifndef NULL
#define NULL ((void *)0)
#endif


#define INST_PING 0x01        //**查询工作状态
#define INST_READ 0x02        //**查询控制表里的字符
#define INST_WRITE 0x03       //**往控制表里写字符
#define INST_REG_WRITE 0x04   //**类似于WRITE DATA，但是控制字符写入后并不马上动作，直到ACTION指令到达
#define INST_REG_ACTION 0x05  //**触发REG WRITE写入的动作
#define INST_SYNC_WRITE 0x83  //**用于同时控制多个舵机


//波特率定义
#define SCSCL_1M 0
#define	SCSCL_0_5M 1
#define	SCSCL_250K 2
#define	SCSCL_128K 3
#define	SCSCL_115200 4
#define	SCSCL_76800 5
#define	SCSCL_57600	6
#define	SCSCL_38400	7


enum SCS_ERR_LIST
{
	SCS_ERR_NO_REPLY = 1,
	SCS_ERR_CRC_CMP  = 2,
	SCS_ERR_SLAVE_ID = 3,
	SCS_ERR_BUFF_LEN = 4,
};

void setEnd(uint8_t _End);
void Host2SCS(uint8_t *DataL, uint8_t* DataH, int Data);
int SCS2Host(uint8_t DataL, uint8_t DataH);
void writeBuf(uint8_t ID, uint8_t MemAddr, uint8_t *nDat, uint8_t nLen, uint8_t Fun);
int genWrite(uint8_t ID, uint8_t MemAddr, uint8_t *nDat, uint8_t nLen);
int regWrite(uint8_t ID, uint8_t MemAddr, uint8_t *nDat, uint8_t nLen);
int regAction(uint8_t ID);
void syncWrite(uint8_t ID[], uint8_t IDN, uint8_t MemAddr, uint8_t *nDat, uint8_t nLen);
int Read(uint8_t ID, uint8_t MemAddr, uint8_t *nData, uint8_t nLen);
int readByte(uint8_t ID, uint8_t MemAddr);
int readWord(uint8_t ID, uint8_t MemAddr);
int	Ping(uint8_t ID);