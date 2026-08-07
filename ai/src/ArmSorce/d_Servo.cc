#include "d_Servo.h"
#include <stdint.h>
#include "armTask.h"

uint8_t wBuf[128];
uint8_t wLen = 0;

//���ջ�����ˢ��
void rFlushSCS()
{
	//Api.DelayMs(1);
	usleep(1000);
}

//UART �������ݽӿ�
int readSCS(unsigned char *nDat, int nLen)
{
//	return Api.ReadUart1(nDat);
//	return ftUart_Read(nDat, nLen);
	return serial.readTimeout(nDat, nLen, 100);
}

//UART �������ݽӿ�
int writeSCS(unsigned char *nDat, int nLen)
{
	while(nLen--)
	{
		if(wLen<sizeof(wBuf))
		{
			wBuf[wLen] = *nDat;
			wLen++;
			nDat++;
		}
	}
	return wLen;
}

//���ͻ�����ˢ��
void wFlushSCS()
{
	if(wLen)
	{
		// Api.WriteUart1(wBuf,wLen);
		// Api.WriteUart1(wBuf,wLen);
		// Api.WriteUart1(wBuf,wLen);
		// Api.WriteUart1(wBuf,wLen);	serial
		serial.write(wBuf,wLen);
		serial.write(wBuf,wLen);
		serial.write(wBuf,wLen);
		serial.write(wBuf,wLen);		
		wLen = 0;
	}
}



static uint8_t Level =1;//������صȼ�?1,Ĭ��дָ���Ӧ��
static uint8_t End = 0;//��������С�˽ṹ,Ĭ��С�˴洢��ʽ
static uint8_t u8Status;//���״�?
static uint8_t u8Error;//ͨ��״̬

void setEnd(uint8_t _End)
{
	End = _End;
}

uint8_t getEnd(void)
{
	return End;
}

void setLevel(uint8_t _Level)
{
	Level = _Level;
}

int getState(void)
{
	return u8Status;
}

int getLastError(void)
{
	return u8Error;
}

//1��16λ������?2��8λ��
//DataLΪ��λ��DataHΪ��λ
void Host2SCS(uint8_t *DataL, uint8_t* DataH, int Data)
{
	if(End){
		*DataL = (Data>>8);
		*DataH = (Data&0xff);
	}else{
		*DataH = (Data>>8);
		*DataL = (Data&0xff);
	}
}

//2��8λ������?1��16λ��
//DataLΪ��λ��DataHΪ��λ
int SCS2Host(uint8_t DataL, uint8_t DataH)
{
	int Data;
	if(End){
		Data = DataL;
		Data<<=8;
		Data |= DataH;
	}else{
		Data = DataH;
		Data<<=8;
		Data |= DataL;
	}
	return Data;
}

uint8_t bBuf[2] = {0, 0};
int checkHead(void)
{
	uint8_t bDat;
	bBuf[0] = 0;
	bBuf[1] = 0;
	uint8_t Cnt = 0;
	while(1){
		if(!readSCS(&bDat, 1)){
			return 0;
		}
		bBuf[1] = bBuf[0];
		bBuf[0] = bDat;
		if(bBuf[0]==0xff && bBuf[1]==0xff){
			break;
		}
		Cnt++;
		if(Cnt>10){
			return 0;
		}
	}
	return 1;
}

//ָ��Ӧ��
int	Ack(uint8_t ID)
{
	uint8_t bBuf[4];
	uint8_t calSum;
	u8Error = 0;
	if(ID!=0xfe && Level){
		if(!checkHead()){
			u8Error = SCS_ERR_NO_REPLY;
			return 0;
		}
		u8Status = 0;
		if(readSCS(bBuf, 4)!=4){
			u8Error = SCS_ERR_NO_REPLY;
			return 0;
		}
		if(bBuf[0]!=ID){
			u8Error = SCS_ERR_SLAVE_ID;
			return 0;
		}
		if(bBuf[1]!=2){
			u8Error = SCS_ERR_BUFF_LEN;
			return 0;
		}
		calSum = ~(bBuf[0]+bBuf[1]+bBuf[2]);
		if(calSum!=bBuf[3]){
			u8Error = SCS_ERR_CRC_CMP;
			return 0;			
		}
		u8Status = bBuf[2];
	}
	return 1;
}

//**   ���ID��MemAddr�ڴ����ַ��д�����ݣ�д�볤��?  ָ�� 
void writeBuf(uint8_t ID, uint8_t MemAddr, uint8_t *nDat, uint8_t nLen, uint8_t Fun)
{
	uint8_t i;
	uint8_t msgLen = 2;
	uint8_t bBuf[6];
	uint8_t CheckSum = 0;
	bBuf[0] = 0xff;
	bBuf[1] = 0xff;
	bBuf[2] = ID;
	bBuf[4] = Fun; //**ָ��
	if(nDat){              //**������
		msgLen += nLen + 1;
		bBuf[3] = msgLen;  //**����
		bBuf[5] = MemAddr; //**�ڴ���?
		writeSCS(bBuf, 6); //**��ǰ6���ֽ���д��wBuf[128]��
		
	}else{                //**��������
		bBuf[3] = msgLen;
		writeSCS(bBuf, 5); //**��������ֻ��ǰ5���ֽ���д��wBuf[128]��
	}
	CheckSum = ID + msgLen + Fun + MemAddr; //**У���?
	if(nDat){
		for(i=0; i<nLen; i++){
			CheckSum += nDat[i];            //**У���?
		}
		writeSCS(nDat, nLen);   //**�Ѳ���д��wBuf[128]��   
	}
	CheckSum = ~CheckSum;       //**У���ȡ��?
	writeSCS(&CheckSum, 1);     //**У���д��wBuf[128]��
}

//��ͨдָ��
//���ID��MemAddr�ڴ����ַ��д�����ݣ�д�볤��?
int genWrite(uint8_t ID, uint8_t MemAddr, uint8_t *nDat, uint8_t nLen)
{
	rFlushSCS(); //���ջ�����ˢ�£�FT��������л���ʱ��ʱ�����10us
	writeBuf(ID, MemAddr, nDat, nLen, INST_WRITE);
	wFlushSCS(); //���ͻ�����ˢ��
	return Ack(ID);
}

//�첽дָ��
//���ID��MemAddr�ڴ����ַ��д�����ݣ�д�볤��?
int regWrite(uint8_t ID, uint8_t MemAddr, uint8_t *nDat, uint8_t nLen)
{
	rFlushSCS();
	writeBuf(ID, MemAddr, nDat, nLen, INST_REG_WRITE);
	wFlushSCS();
	return Ack(ID);
}

//�첽дִ����
int regAction(uint8_t ID)
{
	rFlushSCS();
	writeBuf(ID, 0, NULL, 0, INST_REG_ACTION);
	wFlushSCS();
	return Ack(ID);
}

//ͬ��дָ��  //**����ͬʱ���ƶ�����
//���ID[]���飬IDN���鳤�ȣ�MemAddr�ڴ����ַ��д�����ݣ�д�볤��?
void syncWrite(uint8_t ID[], uint8_t IDN, uint8_t MemAddr, uint8_t *nDat, uint8_t nLen)
{
	uint8_t mesLen = ((nLen+1)*IDN+4);
	uint8_t Sum = 0;
	uint8_t bBuf[7];
	uint8_t i, j;
	
	bBuf[0] = 0xff;
	bBuf[1] = 0xff;
	bBuf[2] = 0xfe;
	bBuf[3] = mesLen; //**�ܳ���
	bBuf[4] = INST_SYNC_WRITE; //**����ͬʱ���ƶ�����
	bBuf[5] = MemAddr;
	bBuf[6] = nLen; //**ÿ����������ĳ��ȣ����������ID
	
	rFlushSCS();
	writeSCS(bBuf, 7);

	Sum = 0xfe + mesLen + INST_SYNC_WRITE + MemAddr + nLen;

	for(i=0; i<IDN; i++)
	{
		writeSCS(&ID[i], 1);
		writeSCS(nDat+i*nLen, nLen);
		Sum += ID[i];
		for(j=0; j<nLen; j++)
		{
			Sum += nDat[i*nLen+j];
		}
	}
	Sum = ~Sum;
	writeSCS(&Sum, 1);
	wFlushSCS();
}

int writeByte(uint8_t ID, uint8_t MemAddr, uint8_t bDat)
{
	rFlushSCS();
	writeBuf(ID, MemAddr, &bDat, 1, INST_WRITE);
	wFlushSCS();
	return Ack(ID);
}

int writeWord(uint8_t ID, uint8_t MemAddr, uint16_t wDat)
{
	uint8_t buf[2];
	Host2SCS(buf+0, buf+1, wDat); //**//1��16λ������?2��8λ��
	rFlushSCS();
	writeBuf(ID, MemAddr, buf, 2, INST_WRITE);
	wFlushSCS();
	return Ack(ID);
}

//��ָ��
//���ID��MemAddr�ڴ����ַ����������nData�����ݳ���nLen
uint8_t result = 0;
uint8_t ReadBuf[50];
uint8_t ReadBufLen;
uint8_t ReadBufIndex;
uint8_t ReadBufStep;
int Read(uint8_t ID, uint8_t MemAddr, uint8_t *nData, uint8_t nLen)
{
	int Size;
	uint8_t bBuf[4];
	uint8_t calSum = 0;
	uint8_t i;
	rFlushSCS();
	writeBuf(ID, MemAddr, &nLen, 1, INST_READ);
	wFlushSCS();	
	u8Error = 0;
	ReadBufLen = 0;
	
	//ReadBufLen = Api.ReadUart1(ReadBuf);

	ReadBufLen = serial.readTimeout(ReadBuf,50,50);
	
	ReadBufIndex = 0;
	ReadBufStep = 0;
	for(int n=0;n<ReadBufLen;n++)
	{
		switch(ReadBufStep)
		{
			case 0:
				if(ReadBuf[ReadBufIndex] == 0xff && ReadBufIndex <= ReadBufLen)
				{
					ReadBufStep = 1;
					ReadBufIndex = 1;
				}
				else
				{
					ReadBufStep = 0;
					ReadBufIndex = 0;
					u8Error = SCS_ERR_NO_REPLY;
					return 0;
				}
			break;
			case 1:
				if(ReadBuf[ReadBufIndex] == 0xff && ReadBufIndex <= ReadBufLen)
				{
					ReadBufStep = 2;
					ReadBufIndex = 2;
				}
				else
				{
					ReadBufIndex = 0;
					ReadBufIndex = 0;
					u8Error = SCS_ERR_NO_REPLY;
					return 0;
				}			
			break;

			case 2:
				if((ReadBuf[ReadBufIndex] == ID ||  ID == 0xfe) && (ReadBufIndex <= ReadBufLen))
				{
					ReadBufStep = 3;
					ReadBufIndex = 3;
				}
				else
				{
					ReadBufStep = 0;
					ReadBufIndex = 0;
					return 0;
				}			
			break;
			case 3:
				if((ReadBuf[ReadBufIndex] == (nLen+2)) && (ReadBufIndex <= ReadBufLen))
				{
					ReadBufStep = 4;
					ReadBufIndex = 5;
				}
				else
				{
					ReadBufStep = 0;
					ReadBufIndex = 0;
					u8Error = SCS_ERR_BUFF_LEN;
					return 0;
				}			
			break;			
			case 4:
				if(ReadBufIndex <= ReadBufLen)
				{
					for(int j=0; j<nLen;j++)
					{
						nData[j] = ReadBuf[ReadBufIndex+j];
					}
					calSum = 0;
					ReadBufStep = 5;
					ReadBufIndex += nLen;
				}
				else
				{
					ReadBufStep = 0;
					ReadBufIndex = 0;
					return 0;
				}			
			break;
			case 5:
				if(ReadBufIndex <= ReadBufLen)
				{
					for(int k=2; k<=ReadBufIndex;k++)
					{
						calSum += ReadBuf[k];
					}
					calSum = ~calSum;
					ReadBufStep = 6;
					ReadBufIndex += 1;
				}
				else
				{
					ReadBufStep = 0;
					ReadBufIndex = 0;
					u8Error = SCS_ERR_CRC_CMP;
					return 0;
				}			
			break;	
			case 6:
				if(ReadBufIndex <= ReadBufLen)
				{
					if(calSum == ReadBuf[ReadBufIndex])
					{
						ReadBufStep = 0;
						ReadBufIndex = 0;
						return nLen;
					}
				}
				else
				{
					ReadBufStep = 0;
					ReadBufIndex = 0;
					u8Error = SCS_ERR_CRC_CMP;
					return 0;
				}			
			break;				
		}		
	}
}

//��1�ֽڣ���ʱ����-1
int readByte(uint8_t ID, uint8_t MemAddr)
{
	uint8_t bDat;
	int Size = Read(ID, MemAddr, &bDat, 1);
	if(Size!=1){
		return -1;
	}else{
		return bDat;
	}
}

//��2�ֽڣ���ʱ����-1
int readWord(uint8_t ID, uint8_t MemAddr)
{	
	uint8_t nDat[2];
	int Size;
	uint16_t wDat;
	Size = Read(ID, MemAddr, nDat, 2);
	if(Size!=2)
		return -1;
	wDat = SCS2Host(nDat[0], nDat[1]);
	return wDat;
}

//Pingָ����ض��ID����ʱ����-1
int	Ping(uint8_t ID)
{
	uint8_t bBuf[4];
	uint8_t calSum;
	rFlushSCS();
	writeBuf(ID, 0, NULL, 0, INST_PING);
	wFlushSCS();
	u8Status = 0;
	if(!checkHead()){
		u8Error = SCS_ERR_NO_REPLY;
		return -1;
	}
	u8Error = 0;
	if(readSCS(bBuf, 4)!=4){
		u8Error = SCS_ERR_NO_REPLY;
		return -1;
	}
	if(bBuf[0]!=ID && ID!=0xfe){
		u8Error = SCS_ERR_SLAVE_ID;
		return -1;
	}
	if(bBuf[1]!=2){
		u8Error = SCS_ERR_BUFF_LEN;
		return -1;
	}
	calSum = ~(bBuf[0]+bBuf[1]+bBuf[2]);
	if(calSum!=bBuf[3]){
		u8Error = SCS_ERR_CRC_CMP;
		return -1;			
	}
	u8Status = bBuf[2];
	return bBuf[0];
}


//int FeedBack(int ID)
//{
//	int nLen = Read(ID, SCSCL_PRESENT_POSITION_L, Mem, sizeof(Mem));
//	if(nLen!=sizeof(Mem)){
//		return -1;
//	}
//	return nLen;
//}