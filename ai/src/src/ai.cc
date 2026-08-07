#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <time.h>
#include <unistd.h>
#include <lcm/lcm-cpp.hpp>

#include "../AiSorce/aiTask.h"
#include "../ArmSorce/armTask.h"
#include "mars_message/Video.hpp"
#include "mars_message/Event.hpp"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "libai.h"


pthread_t tid_yoloTask = NULL;
pthread_t tid_armTask = NULL;

int aiTask(fn_video_cb cb)
{
	printf("aiTask start!!!\n");
	if ((pthread_create(&tid_yoloTask, NULL, yoloThread, (void *)cb)) != 0)
	{
		perror("pthread_create error");
		return 1;
	}
	pthread_detach(tid_yoloTask);
}
#ifdef TYPE_EXE
lcm::LCM lcm_;
int video_cb(const uint8_t *data, int len, uint64_t ts, int frametype)
{
	//printf("AiVideo get ts=%llu len=%d type=%d\n", ts, len, frametype);
	if(!aiIsProfileEnabled(PROFILE_ENCODE))
	{
		return 0;
	}
	mars_message::Video msg;
	msg.timestampMs = ts;
	msg.data.resize(len);
	msg.type = frametype;
	msg.count = len;
	memcpy(msg.data.data(), data, len);
	lcm_.publish("AiVideo", &msg);
	//printf("AiVideo publish ts=%llu len=%d type=%d, (%d %d)\n", ts, msg.data.size(), frametype, msg.data[0], msg.data[10]);
	return 0;
}

// LCM 消息回调函数
class MessageHandler
{
public:
	void aiCmdHandler(const lcm::ReceiveBuffer* rbuf, const std::string& chan, 
                         const mars_message::Event* msg)
	{
		LOGD(TAG, "AiCmd receive cmd=%d param=%d", msg->event, msg->param);
		switch (msg->event)
		{
		case 0:
			aiEnableProfile(PROFILE_AI, msg->param != 0);
			break;
		case 1:
			aiEnableProfile(PROFILE_ENCODE, msg->param != 0);
			break;
		case 2:
			aiEnableProfile(PROFILE_TIME, msg->param != 0);
			break;
		case 3:
			aiEnableProfile(PROFILE_FPS, msg->param != 0);
			break;
		default:
			break;
		}
	}
};
MessageHandler handler;

int main(int argc, char *argv[]) 
{
	if (!lcm_.good())		
	{
		LOGE(TAG, "LCM no good");
		return 1;
	}
	LOGD(TAG, "AI Start!!!"); 
	aiTask(video_cb);
	lcm_.subscribe("AiCmd", &MessageHandler::aiCmdHandler, &handler);
	while (0 == lcm_.handle());
	return 0;
}
#endif