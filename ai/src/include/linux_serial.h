#ifndef _LINUX_SERAIL_H
#define _LINUX_SERIAL_H

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <termios.h>   
#include   <sys/ioctl.h> 
#include   <sys/socket.h> 
#include   <netinet/in.h> 
#include   <net/if.h>
#include "_log.h"
#include <string>

#undef TAG
#define TAG "SERIAL"

class Serial
{
private:
	int	serial_fd;
public:
	int open(const char* port, int baud, char databits, char stopbit, char parity)
	{
		int fd;
		/*以读写方式打开串口*/
		fd = ::open(port, O_RDWR);
		if (fd < 0) {
			/* 不能打开串口*/
			LOGE(TAG, "open serial {0} failed", port);
		}
		serial_fd = fd;
		set_speed(baud);
		set_parity(databits, stopbit, parity);
		return fd;
	}
	/**
	*@brief  设置串口通信速率
	*@param  fd     类型 int  打开串口的文件句柄
	*@param  speed  类型 int  串口速度
	*@return  void
	*/
	void set_speed(int speed) {
		int fd = serial_fd;
		int speed_arr[] = {B1000000, B500000, B460800,B230400, B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300,
	B38400, B19200, B9600, B4800, B2400, B1200, B300, };
		
		int name_arr[] = {1000000, 500000, 460800,230400,115200, 38400,  19200,  9600,  4800,  2400,  1200,  300, 38400,
	19200,  9600, 4800, 2400, 1200,  300, };

		int   i;
		int   status;
		struct termios   Opt;
		tcgetattr(fd, &Opt);
		for (i = 0; i < sizeof(speed_arr) / sizeof(int); i++) {
			if (speed == name_arr[i]) {
				tcflush(fd, TCIOFLUSH);
				cfsetispeed(&Opt, speed_arr[i]);
				cfsetospeed(&Opt, speed_arr[i]);
				status = tcsetattr(fd, TCSANOW, &Opt);
				if (status != 0) {
					LOGE(TAG, "tcsetattr fd1");
					return;
				}
				tcflush(fd, TCIOFLUSH);
			}
		}
	}

	/**
	*@brief   设置串口数据位，停止位和效验位
	*@param  databits 类型  char 数据位   取值 为 7 或者8
	*@param  stopbits 类型  char 停止位   取值为 1 或者2
	*@param  parity  类型  char  效验类型 取值为N,E,O,,S
	*/
	int set_parity(char databits, char stopbits, char parity)
	{
		int fd = serial_fd;
		struct termios options;
		if (tcgetattr(fd, &options) != 0) {
			LOGE(TAG, "Cannot open fd to get attribute");
			return(0);
		}
		options.c_cflag &= ~CSIZE;
		switch (databits) /*设置数据位数*/
		{
		case 7:
			options.c_cflag |= CS7;
			break;
		case 8:
			options.c_cflag |= CS8;
			break;
		default:
			LOGE(TAG, "Unsupported data size"); return (0);
		}
		switch (parity)
		{
		case 'n':
		case 'N':
			options.c_cflag &= ~PARENB;   /* Clear parity enable */
			options.c_iflag &= ~INPCK;     /* Enable parity checking */
			break;
		case 'o':
		case 'O':
			options.c_cflag |= (PARODD | PARENB); /* 设置为奇效验*/
			options.c_iflag |= INPCK;             /* Disnable parity checking */
			break;
		case 'e':
		case 'E':
			options.c_cflag |= PARENB;     /* Enable parity */
			options.c_cflag &= ~PARODD;   /* 转换为偶效验*/
			options.c_iflag |= INPCK;       /* Disnable parity checking */
			break;
		case 'S':
		case 's':  /*as no parity*/
			options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB; break;
		default:
			LOGE(TAG, "Unsupported parity");
			return (0);
		}
		/* 设置停止位*/
		switch (stopbits)
		{
		case 1:
			options.c_cflag &= ~CSTOPB;
			break;
		case 2:
			options.c_cflag |= CSTOPB;
			break;
		default:
			LOGE(TAG, "Unsupported stop bits");
			return (0);
		}
		// set tty to binary mode
		options.c_lflag &= ~(ICANON | ECHO  | ISIG | IEXTEN); /*Input*/
		options.c_oflag &= ~OPOST;                          /*Output*/
		options.c_iflag &= ~(ICRNL  | INPCK | INLCR | IGNCR | ISTRIP | IXON | IXOFF);
		options.c_cflag |= CLOCAL | CREAD;
		
		/* Set input parity option */
		if (parity != 'n' && parity != 'N')
			options.c_iflag |= INPCK;
		tcflush(fd, TCIFLUSH);
		options.c_cc[VTIME] = 1; /* 设置超时 15 seconds*/
		options.c_cc[VMIN] = 10; /* Update the options and do it NOW */
		
		if (tcsetattr(fd, TCSANOW, &options) != 0)
		{
			LOGE(TAG, "Set attribute to fd failed");
			return (0);
		}
		return (1);
	}
	int close()
	{
		int r = -1;
		if(serial_fd != -1)
		{
			r = ::close(serial_fd);
		}
		return r;
	}

	int write(uint8_t *data, int len)
	{
		// LOGL(TAG);
		return ::write(serial_fd, data, len);
	}

	int writeString(const char *data)
	{
		// LOGL(TAG);
		return ::write(serial_fd, data, strlen(data));
	}

	// for read
	int set_timeout(int timeout_100ms)
	{
		int fd = serial_fd;
		struct termios options;
		if (tcgetattr(fd, &options) != 0) {
			LOGE(TAG, "Cannot open fd to get attribute");
			return(-1);
		}

		options.c_cc[VTIME] = timeout_100ms; /* 设置超时 timeout_100ms * 100ms*/
		options.c_cc[VMIN] = 0; /* Update the options and do it NOW */
		
		if (tcsetattr(fd, TCSANOW, &options) != 0)
		{
			LOGE(TAG, "Set attribute to fd failed");
			return (-1);
		}

		if(timeout_100ms == 0)
		{
			return fcntl(fd,F_SETFL,FNDELAY);
		}
		else
		{
			return fcntl(fd,F_SETFL,0);
		}

		return 0;
	}

	int read(uint8_t *data, int len)
	{
		return ::read(serial_fd, data, len);
	}

	int readTimeout(uint8_t *data, int len, int us)
	{
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(serial_fd, &readfds);
		struct timeval selectTimeOut;
		selectTimeOut.tv_usec = us;
		selectTimeOut.tv_sec = (long)0;
		int ret = select(serial_fd + 1, &readfds, NULL, NULL, &selectTimeOut);
		if(ret > 0 && FD_ISSET(serial_fd, &readfds))
		{
			return ::read(serial_fd, data, len);
		}
		else
		{
			return 0;
		}
	}

	int write(uint8_t *data, int len, int timeout_ms)
	{
		fd_set writefds;
		FD_ZERO(&writefds);
		FD_SET(serial_fd, &writefds);
		struct timeval selectTimeOut;
		selectTimeOut.tv_usec = timeout_ms*1000L;
		selectTimeOut.tv_sec = (long)0;
		int ret = select(serial_fd + 1, NULL, &writefds, NULL, &selectTimeOut);
		if(ret == 0)
		{
			return ::write(serial_fd, data, len);
		}
		else if(ret > 0)
		{
			// time out
			return 0;
		}
		else if(ret < 0)
		{
			// error;
			return -1;
		}
	}

	int read(uint8_t *data, int len, int timeout_100ms)
	{
		int fd = serial_fd;
		struct termios options;
		if (tcgetattr(fd, &options) != 0) {
			LOGE(TAG, "Cannot open fd to get attribute");
			return(0);
		}

		options.c_cc[VTIME] = timeout_100ms; /* 设置超时 timeout_100ms * 100ms*/
		options.c_cc[VMIN] = len; /* Update the options and do it NOW */
		
		if (tcsetattr(fd, TCSANOW, &options) != 0)
		{
			LOGE(TAG, "Set attribute to fd failed");
			return (0);
		}
		return ::read(serial_fd, data, len);
	}

	enum{
		PURGE_READ,
		PURGE_WRITE,
		PURGE_RW
	};
	int purge(int p)
	{
		switch(p)
		{
			case PURGE_READ:
			return tcflush(serial_fd, TCIFLUSH);
			break;
			case PURGE_WRITE:
			return tcflush(serial_fd, TCOFLUSH);			
			break;
			case PURGE_RW:
			return tcflush(serial_fd, TCIOFLUSH);			
			break;
		}
		return -1;
	}

	bool is_open()
	{
		return serial_fd > 0;
	}
};

#endif
