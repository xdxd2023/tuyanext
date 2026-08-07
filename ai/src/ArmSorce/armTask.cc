#include "armTask.h"
#include "RbArmConfig.h"
#include "RbConfig.h"
#include "A_ArmUservo.h"
#include "A_ArmKinematic.h"
#include "time_tick.h"

#define F_vTaskDelay(id,ms) usleep(ms*1000)

Serial serial;

int ArmUartInit(void)
{
    char port[32] = {0};
    strcpy(port, MCU_PORT);

    LOGD("MCU", "=============================> Open {0} for mcu communication", port);
    if (serial.open(port, 1000000, 8, 1, 'N') < 0)
    {
        printf("总线串口打开失败 ======================");
        return -1;
    }
    else{
        serial.writeString("HC robot ctrl \r\n");
    }
	return 0;
}

void *ArmTask(void *argv){
    int i=0;

    uint8_t A_dofnum = 3;

    bool sendLock = false;
    A_RotPosSt_ A_RotPosStruct = {0};
    A_JointAngkeSt_ A_JointAngkeStruct = {0};
    A_RotJointSpeedSt_ A_JointSpeedSt = {0};

    A_RotPosSt_ A_RotPosStruct_ik = {0};
    A_JointAngkeSt_ A_JointAngkeStruct_ik = {0};
    A_RotPosSt_ A_RotPosStruct_fk = {0};
    A_JointAngkeSt_ A_JointAngkeStruct_fk = {0};

    bool pitchAuto = true;

    while(1){
        printf("tick = %d\r\n",TimeTick::Ms());
		usleep(1);

        // //非柱塞读取
        // fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);
        // char ch = getchar();
		// if(getchar())
		{
            printf("1234567890\r\n\r\n\r\n");
			A_JointSpeedSt.joint1 = 1000;
			A_JointSpeedSt.joint2 = 700;
			A_JointSpeedSt.joint3 = 1000;
			A_JointSpeedSt.joint4 = 1100;
			A_JointSpeedSt.joint5 = 1000;
			A_JointSpeedSt.joint6 = 1000;			
			
			sendLock = true;
			ArmShowOgrProgram(&A_JointSpeedSt);
			sendLock = false;
			sleep(3);
			
			A_JointSpeedSt.joint1 = 1000;
			A_JointSpeedSt.joint2 = 900;
			A_JointSpeedSt.joint3 = 1400;
			A_JointSpeedSt.joint4 = 1400;
			A_JointSpeedSt.joint5 = 600;
			A_JointSpeedSt.joint6 = 600;
			
			A_RotPosStruct_ik.posSt.x = 0;
			A_RotPosStruct_ik.posSt.y = 60;
			A_RotPosStruct_ik.posSt.z = 280;
			A_RotPosStruct_ik.posSt.pitch = 0.6;
			pitchAuto = false;	
			A_JointAngkeStruct_ik.joint_5 = 150;
			A_JointAngkeStruct_ik.joint_6 = 26;
			sendLock = true;
			set_tool_pose(&A_RotPosStruct_ik.posSt,&A_JointAngkeStruct_ik,&A_JointSpeedSt ,pitchAuto, A_dofnum);	
			sendLock = false;
			sleep(2);
		
			A_RotPosStruct_ik.posSt.x = 80;
			A_RotPosStruct_ik.posSt.y = 0;
			A_RotPosStruct_ik.posSt.z = 280;
			A_RotPosStruct_ik.posSt.pitch = 0.6;
			pitchAuto = false;	
			sendLock = true;
			set_tool_pose(&A_RotPosStruct_ik.posSt,&A_JointAngkeStruct_ik,&A_JointSpeedSt ,pitchAuto, A_dofnum);	
			sendLock = false;
			sleep(2);

//左上角
			A_RotPosStruct_ik.posSt.x = 190;
			A_RotPosStruct_ik.posSt.y = 120;
			A_RotPosStruct_ik.posSt.z = 200;
			A_RotPosStruct_ik.posSt.pitch = 0.6;
			pitchAuto = false;			
			A_JointSpeedSt.joint1 = 700;
			A_JointSpeedSt.joint2 = 700;
			A_JointSpeedSt.joint3 = 700;
			A_JointSpeedSt.joint4 = 700;	

			sendLock = true;
			set_tool_pose(&A_RotPosStruct_ik.posSt,&A_JointAngkeStruct_ik,&A_JointSpeedSt ,pitchAuto, A_dofnum);
			sendLock = false;
			sleep(2);

//**逆时针开爪
			A_JointAngkeStruct_ik.joint_5 = 240;
			A_JointAngkeStruct_ik.joint_6 = 98;
			pitchAuto = false;			
	
			sendLock = true;
			set_tool_pose(&A_RotPosStruct_ik.posSt,&A_JointAngkeStruct_ik,&A_JointSpeedSt ,pitchAuto, A_dofnum);
			sendLock = false;
			sleep(2);
			
//左下角
			A_RotPosStruct_ik.posSt.x = 190;
			A_RotPosStruct_ik.posSt.y = 120;
			A_RotPosStruct_ik.posSt.z = 25;
			A_RotPosStruct_ik.posSt.pitch = 0.8;
			pitchAuto = false;					
			sendLock = true;
			set_tool_pose(&A_RotPosStruct_ik.posSt,&A_JointAngkeStruct_ik,&A_JointSpeedSt ,pitchAuto, A_dofnum);
			sendLock = false;
			sleep(3);	

//闭爪
			A_JointAngkeStruct_ik.joint_6 = 26+20;
			pitchAuto = false;			
	
			sendLock = true;
			set_tool_pose(&A_RotPosStruct_ik.posSt,&A_JointAngkeStruct_ik,&A_JointSpeedSt ,pitchAuto, A_dofnum);
			sendLock = false;
			sleep(2);


//左上角+爪回正
			A_RotPosStruct_ik.posSt.x = 190;
			A_RotPosStruct_ik.posSt.y = 120;
			A_RotPosStruct_ik.posSt.z = 200;
			
			A_JointAngkeStruct_ik.joint_5 = 150;
			A_JointAngkeStruct_ik.joint_6 = 26+20;	
			
			A_RotPosStruct_ik.posSt.pitch = 0.6;
			pitchAuto = false;			

			A_JointSpeedSt.joint1 = 700;
			A_JointSpeedSt.joint2 = 700;
			A_JointSpeedSt.joint3 = 700;
			A_JointSpeedSt.joint4 = 700;			
			sendLock = true;
			set_tool_pose(&A_RotPosStruct_ik.posSt,&A_JointAngkeStruct_ik,&A_JointSpeedSt ,pitchAuto, A_dofnum);
			sendLock = false;
			sleep(2);
			
//右上角
			A_JointSpeedSt.joint1 = 1000;
			A_JointSpeedSt.joint2 = 700;
			A_JointSpeedSt.joint3 = 700;
			A_JointSpeedSt.joint4 = 700;
			A_RotPosStruct_ik.posSt.x = 180;
			A_RotPosStruct_ik.posSt.y = -100;
			A_RotPosStruct_ik.posSt.z = 200;
			A_RotPosStruct_ik.posSt.pitch = 0.6;
			pitchAuto = false;			
				
			sendLock = true;
			set_tool_pose(&A_RotPosStruct_ik.posSt,&A_JointAngkeStruct_ik,&A_JointSpeedSt ,pitchAuto, A_dofnum);
			sendLock = false;
			sleep(2);


//右下角 + 逆时针闭爪
			A_RotPosStruct_ik.posSt.x = 180;
			A_RotPosStruct_ik.posSt.y = -130;
			A_RotPosStruct_ik.posSt.z = 30;
			
			A_JointAngkeStruct_ik.joint_5 = 60;
			A_JointAngkeStruct_ik.joint_6 = 26+20;		
			
			A_RotPosStruct_ik.posSt.pitch = 0.8;
			pitchAuto = false;			
	
			A_JointSpeedSt.joint1 = 700;
			A_JointSpeedSt.joint2 = 700;
			A_JointSpeedSt.joint3 = 700;
			A_JointSpeedSt.joint4 = 700;			
			sendLock = true;
			set_tool_pose(&A_RotPosStruct_ik.posSt,&A_JointAngkeStruct_ik,&A_JointSpeedSt ,pitchAuto, A_dofnum);
			sendLock = false;
			sleep(2);

//开爪
			A_JointAngkeStruct_ik.joint_5 = 60;
			A_JointAngkeStruct_ik.joint_6 = 98;		
			
			pitchAuto = false;			
			
			sendLock = true;
			set_tool_pose(&A_RotPosStruct_ik.posSt,&A_JointAngkeStruct_ik,&A_JointSpeedSt ,pitchAuto, A_dofnum);
			sendLock = false;
			sleep(2);

//右上角
			A_RotPosStruct_ik.posSt.x = 180;
			A_RotPosStruct_ik.posSt.y = -130;
			A_RotPosStruct_ik.posSt.z = 200;
			A_RotPosStruct_ik.posSt.pitch = 0.6;
			
			A_JointAngkeStruct_ik.joint_5 = 150;
			A_JointAngkeStruct_ik.joint_6 = 26;		
			
			pitchAuto = false;			
		
			sendLock = true;
			set_tool_pose(&A_RotPosStruct_ik.posSt,&A_JointAngkeStruct_ik,&A_JointSpeedSt ,pitchAuto, A_dofnum);
			sendLock = false;
			sleep(2);
			

//右下角
			A_RotPosStruct_ik.posSt.x = 180;
			A_RotPosStruct_ik.posSt.y = -140;
			A_RotPosStruct_ik.posSt.z = 25;
			A_RotPosStruct_ik.posSt.pitch = 0.8;
			pitchAuto = false;			

			A_JointAngkeStruct_ik.joint_5 = 60;
			A_JointAngkeStruct_ik.joint_6 = 98;	

			sendLock = true;
			set_tool_pose(&A_RotPosStruct_ik.posSt,&A_JointAngkeStruct_ik,&A_JointSpeedSt ,pitchAuto, A_dofnum);
			sendLock = false;
			sleep(2);
			
			A_JointAngkeStruct_ik.joint_5 = 60;
			A_JointAngkeStruct_ik.joint_6 = 26+20;	

			sendLock = true;
			set_tool_pose(&A_RotPosStruct_ik.posSt,&A_JointAngkeStruct_ik,&A_JointSpeedSt ,pitchAuto, A_dofnum);
			sendLock = false;
			sleep(2);			
//右上角				
			A_RotPosStruct_ik.posSt.x = 180;
			A_RotPosStruct_ik.posSt.y = -140;
			A_RotPosStruct_ik.posSt.z = 200;
			A_RotPosStruct_ik.posSt.pitch = 0.6;
			pitchAuto = false;			
			A_JointAngkeStruct_ik.joint_5 = 150;
			A_JointAngkeStruct_ik.joint_6 = 26+20;			
			sendLock = true;
			set_tool_pose(&A_RotPosStruct_ik.posSt,&A_JointAngkeStruct_ik,&A_JointSpeedSt ,pitchAuto, A_dofnum);
			sendLock = false;
			sleep(2);

//左上角
			A_RotPosStruct_ik.posSt.x = 190;
			A_RotPosStruct_ik.posSt.y = 120;
			A_RotPosStruct_ik.posSt.z = 200;
			A_RotPosStruct_ik.posSt.pitch = 0.6;
			pitchAuto = false;			
			A_JointAngkeStruct_ik.joint_5 = 240;
			A_JointAngkeStruct_ik.joint_6 = 26+20;	
			
			A_JointSpeedSt.joint1 = 1000;
			A_JointSpeedSt.joint2 = 700;
			A_JointSpeedSt.joint3 = 700;
			A_JointSpeedSt.joint4 = 700;			
			sendLock = true;
			set_tool_pose(&A_RotPosStruct_ik.posSt,&A_JointAngkeStruct_ik,&A_JointSpeedSt ,pitchAuto, A_dofnum);
			sendLock = false;
			sleep(2);
//左下角
			A_RotPosStruct_ik.posSt.x = 190;
			A_RotPosStruct_ik.posSt.y = 120;
			A_RotPosStruct_ik.posSt.z = 30;
			A_RotPosStruct_ik.posSt.pitch = 0.8;
			pitchAuto = false;						
			sendLock = true;
			set_tool_pose(&A_RotPosStruct_ik.posSt,&A_JointAngkeStruct_ik,&A_JointSpeedSt ,pitchAuto, A_dofnum);
			sendLock = false;
			sleep(2);
		
			A_JointAngkeStruct_ik.joint_5 = 240;
			A_JointAngkeStruct_ik.joint_6 = 98;	
			
			sendLock = true;
			set_tool_pose(&A_RotPosStruct_ik.posSt,&A_JointAngkeStruct_ik,&A_JointSpeedSt ,pitchAuto, A_dofnum);
			sendLock = false;
			sleep(2);	
//左上角
			A_RotPosStruct_ik.posSt.x = 190;
			A_RotPosStruct_ik.posSt.y = 120;
			A_RotPosStruct_ik.posSt.z = 200;
			A_RotPosStruct_ik.posSt.pitch = 0.6;
			pitchAuto = false;			
			A_JointAngkeStruct_ik.joint_5 = 150;
			A_JointAngkeStruct_ik.joint_6 = 16;				
			sendLock = true;
			set_tool_pose(&A_RotPosStruct_ik.posSt,&A_JointAngkeStruct_ik,&A_JointSpeedSt ,pitchAuto, A_dofnum);
			sendLock = false;
			sleep(1);
			
			A_JointSpeedSt.joint1 = 1000;
			A_JointSpeedSt.joint2 = 900;
			A_JointSpeedSt.joint3 = 1400;
			A_JointSpeedSt.joint4 = 1400;
			A_JointSpeedSt.joint5 = 600;
			A_JointSpeedSt.joint6 = 600;
			
			A_RotPosStruct_ik.posSt.x = 0;
			A_RotPosStruct_ik.posSt.y = 60;
			A_RotPosStruct_ik.posSt.z = 280;
			A_RotPosStruct_ik.posSt.pitch = 0.6;
			pitchAuto = false;	
			A_JointAngkeStruct_ik.joint_5 = 150;
			A_JointAngkeStruct_ik.joint_6 = 26;
			sendLock = true;
			set_tool_pose(&A_RotPosStruct_ik.posSt,&A_JointAngkeStruct_ik,&A_JointSpeedSt ,pitchAuto, A_dofnum);	
			sendLock = false;
			sleep(2);

			sendLock = true;
			ArmShowOgrProgram(&A_JointSpeedSt);
			sendLock = false;
			sleep(3);
		}
    }
}

