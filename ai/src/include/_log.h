#ifndef _LOG_H__
#define _LOG_H__

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <csignal>
#include <mutex>

#define SUPPORT_ROLL_LOG

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#endif

#ifdef ROBOT_RK3308
#define LOG_SWITCH "/oem/etc/logon"
#else
#define LOG_SWITCH "/mnt/logon"
#endif
#ifdef SUPPORT_PRINT_COLOR
#define PRINT_COLV printf("\033[37m") // white
#define PRINT_COLD printf("\033[32m") // green
#define PRINT_COLW printf("\033[33m") // yellow
#define PRINT_COLE printf("\033[31m") // red
#define PRINT_COLN printf("\033[0m")  // color none
#else
#define PRINT_COLV
#define PRINT_COLD
#define PRINT_COLW
#define PRINT_COLE
#define PRINT_COLN
#endif

//#define LOG_IS_ON   ((char*)getenv("_LOG_LEVEL")==NULL)?-1:atoi(getenv("_LOG_LEVEL"))

#define DEBUG_LEVEL_LOG 0
#define DEBUG_LEVEL_VERBOSE 1
#define DEBUG_LEVEL_DEDBUG 2
#define DEBUG_LEVEL_WARNING 3
#define DEBUG_LEVEL_ERROR 4

static FILE *_fd = NULL;
static char _log_file_name[256] = {0};
static std::mutex mtx;

static void log_filename(const char *file_name)
{
    //printf("log-filename %s, _log_file_name=0x%x\n", file_name, _log_file_name);
    strcpy(_log_file_name, file_name);
}


/* 使用构造函数来初始化名字依赖于编译器实现,放弃此方法
class LOGFile
{
public:
    LOGFile(const char *filename)
    {
        printf("LOGFile-filename %s, _log_file_name=0x%x\n", filename, _log_file_name);
        strcpy(_log_file_name, filename);
    }
};
*/

class _loghelp
{
    int _log_level;
    FILE *log_fd;
	char gobal_log_file_name[256];

public:
    _loghelp() : _log_level(100)
    {
        log_fd = NULL;
		gobal_log_file_name[0] = 0;
    }
    static _loghelp *get_log()
    {
        static _loghelp logfile;
        if (logfile._log_level == 100)
        {
            logfile.log_start();
            std::signal(SIGUSR1, signal_handler);
        }
        return &logfile;
    }

    static void signal_handler(int signal)
    {
        get_log()->log_start();
    }

    static int get_filename_by_fd(FILE *fd, char *file_path, int max_len)
    {
        char buf[1024] = {'\0'};
        snprintf(buf,sizeof(buf), "/proc/self/fd/%d", fileno(fd));
        int ret =  readlink(buf,file_path, max_len-1);
        return ret;
    }

    void log_start()
    {
        if (access(LOG_SWITCH, F_OK) != 0)
        {
            _log_level = 99;
        }
        else
        {
            FILE *fp = fopen(LOG_SWITCH, "r");
            _log_level = 0;
            fscanf(fp, "%d", &_log_level);
            fclose(fp);
        }
    }

    FILE *log_openfile(const char *file)
    {
        if (file[0] == 0)
            return NULL;
        char filename[512] = {0};
        if (file[0] != '/')
        {
            strcpy(filename, "/tmp/");
        }
        strcat(filename, file);
		char exit_path[512] = {0};
		sprintf(exit_path,"%s.log",filename);
		if(access(exit_path,F_OK) == 0)
		{
		   char cmd[512] = {0};
		   sprintf(cmd,"mv %s %s.crash.log",exit_path,filename);
		   system(cmd);
		}
        strcat(filename, ".log");
        return fopen(filename, "w");
    }

    void logf_printf(const char *tag, const char *fmt, ...)
    {
        FILE *file = NULL;
        char filename[512] = {0};
        strcat(filename, tag);
        strcat(filename, ".log");
        file = fopen(filename, "a+");
        char log_buf[4096] = {0};

        va_list ap;
        va_start(ap, fmt);
        vsnprintf(log_buf + strlen(log_buf), 4096 - strlen(log_buf), fmt, ap);
        va_end(ap);
        if (file != NULL)
        {
            fwrite(log_buf, strlen(log_buf), 1, file);
            fclose(file);
        }
    }

    int get_proc_name(char *name, int max)
    {
        int rval = readlink("/proc/self/exe", name, max - 1);
        if (rval < 0)
        {
            return rval;
        }
        name[rval] = '\0';
        char *r = strrchr(name, '/');
        if (r)
        {
            strcpy(name, r + 1);
            return rval;
        }
        return -1;
    }

    void logl_printf(const char *file_name, FILE **fd, int debug_level, const char *fmt, ...)
    {
        if (debug_level < _log_level)
            return;

        std::unique_lock<std::mutex> lck(mtx);

        char log_buf[4096] = {0};

        {
            time_t t;
            time(&t);
            struct tm *dt = localtime(&t);
            sprintf(log_buf, "%02d-%02d %02d:%02d:%02d\t", dt->tm_mon+1, dt->tm_mday, dt->tm_hour, dt->tm_min, dt->tm_sec);
        };

        va_list ap;
        va_start(ap, fmt);
        vsnprintf(log_buf + strlen(log_buf), 4096 - strlen(log_buf), fmt, ap);
        va_end(ap);

		if(strlen(log_buf) < 4096 && log_buf[strlen(log_buf)-1] != '\n')
			 log_buf[strlen(log_buf)] = '\n';

        FILE *fd_use = NULL;
        int type = -1;
        // 如果 filename 和 *fd 都为空，说明使用默认名称
        if (file_name[0] == '\0' && *fd == NULL)
        {
            // 这种情况直接使用全局 log_fd，不使用 *fd，避免无法关闭所有的局部 *fd
            if (log_fd == NULL)
            {
                // 第一次进入，打开文件，未定义过文件则采用定义文件
                if(gobal_log_file_name[0] == '\0')
                {
                  get_proc_name(gobal_log_file_name, 256);
                  log_fd = log_openfile(gobal_log_file_name);
		        }else
                {
                  log_fd = log_openfile(gobal_log_file_name);
                }
            }
            else
            {
                // 使用全局 log_fd
                //printf("------1------\n");
            }
            fd_use = log_fd;
            type = 1;
        }
        // 如果 filename 不为空，*fd 为空，说明使用自定义名称，并且第一次进入该函数
        if (file_name[0] != '\0' && *fd == NULL)
        {  
		    if(0 == strcmp(gobal_log_file_name,file_name))
			{
			   if (log_fd == NULL)
			   {
			      log_fd = log_openfile(file_name);
				  *fd = log_fd;
			   }else
			   {
			      *fd = log_fd;
			   }
			}else
			{
			   *fd = log_openfile(file_name); // 如果 file_name 与全局名称相同，可能会导致问题
			   if(gobal_log_file_name[0] == '\0')
			   {
			     strcpy(gobal_log_file_name, file_name);
			   }
			   if (log_fd == NULL)
               {
                // 如果 log_fd 没有被使用，就赋值给 log_fd ??
                 log_fd = *fd;
               }
			   
			}
            fd_use = *fd;
            type = 2;
			//printf("------2------\n");
        }
        // 如果 filename 和 *fd 都不为空，说明使用自定义名称，并且再次进入改函数
        if (file_name[0] != '\0' && *fd != NULL)
        {
            if(0 == strcmp(gobal_log_file_name,file_name))
			{
			  if(*fd != log_fd && log_fd != NULL)
			  {
			    *fd = log_fd;
			  }
			  else if(log_fd == NULL)//全局已经关闭，fd 已无效
			  {
			    log_fd = log_openfile(file_name);
				*fd    = log_fd;
			  }
			}
            fd_use = *fd;
            type = 3;
        }

		if (file_name[0] == '\0' && *fd != NULL)
        {
           printf("------3------\n");
        }

        switch (debug_level)
        {
        case DEBUG_LEVEL_LOG:
            break;
        case DEBUG_LEVEL_VERBOSE:
            PRINT_COLV;
            break;
        case DEBUG_LEVEL_DEDBUG:
            PRINT_COLD;
            break;
        case DEBUG_LEVEL_WARNING:
            PRINT_COLW;
            break;
        case DEBUG_LEVEL_ERROR:
            PRINT_COLE;
            break;
        }
        printf("%s", log_buf);
        PRINT_COLN;

        if (fd_use != NULL)
        {
            fwrite(log_buf, strlen(log_buf), 1, fd_use);
            fflush(fd_use);
        }

        if (fd_use != NULL)
		{
		char filen_Test[1024] = {0};
        get_filename_by_fd(fd_use, filen_Test, 1024);
		//printf("------file real %s------\n",filen_Test);
        }else
        {
          printf("------error fd------\n");
        }
#ifndef MAX_LOG_SIZE 
#define MAX_LOG_SIZE 1024 * 1024
#endif
#ifdef SUPPORT_ROLL_LOG
        // 如果文件的大小超过设定值，关闭文件
        if (fd_use != NULL)
        {    
            //printf("------size %d------\n",ftell(fd_use));
            if (ftell(fd_use) > MAX_LOG_SIZE)
            {
                char filen[1024] = {0};
                get_filename_by_fd(fd_use, filen, 1024);
				//printf("------4------\n");
                fclose(fd_use);
                // 关闭对应的 fd
                switch (type)
                {
                case 1:
                    log_fd = NULL;
                    break;
                case 2:
                    PRINT_COLE;
                    printf("E: come here is abnormal\n");
                    PRINT_COLN;
                    break;
                case 3:
                    if (log_fd == *fd)
                    {
                        log_fd = NULL;
                    }
                    *fd = NULL;
                    break;
                default:
                    PRINT_COLE;
                    printf("E: error type\n");
                    PRINT_COLN;
                    break;
                }
                char buf[1024] = {0};
				//printf("------5------\n");
                sprintf(buf, "mv %s %s-0.log", filen, filen);
				//printf("------file mv %s------\n",filen);
                system(buf);
            }
        }
#endif
    }

    void log_array(const char *file_name, FILE **fd, uint8_t *data, int len)
    {
        char log_buf[4096] = {0};
        int ll_ = 0;
        int pos = 0;
        for (ll_ = 0; ll_ < len; ll_++)
        {
            if (ll_ && ((ll_ & 0xF) == 0))
            {
                sprintf(log_buf + pos, "\n");
                pos += 1;
            }
            sprintf(log_buf + pos, "[%02X] ", data[ll_]);
            pos += 5;
        }
        sprintf(log_buf + pos, "\n");
        logl_printf(file_name, fd, DEBUG_LEVEL_LOG, "%s", log_buf);
    }

    void log_carray(const char *file_name, FILE **fd, uint8_t *data, int len)
    {
        char log_buf[4096] = {0};
        int ll_ = 0;
        int pos = 0;
        for (ll_ = 0; ll_ < len; ll_++)
        {
            if (ll_ && ((ll_ & 0xF) == 0))
            {
                sprintf(log_buf + pos, "\n");
                pos += 1;
            }
            sprintf(log_buf + pos, "%c", data[ll_]);
            pos += 1;
        }
        sprintf(log_buf + pos, "\n");
        logl_printf(file_name, fd, DEBUG_LEVEL_LOG, "%s", log_buf);
    }
};

/*
使用说明
0. 此文件仅支持 c++
1. 在需要打印的源文件中 #include "_log.h"
2. 在需要打印的源文件中入口函数中调用 log_filename("xxx") 或者定义全局类变量 LOGFile l("xxx") 来设定日志文件的名称，每个源文件可以设定单独的日志文件
   也可以整个工程只调用一次 log_filename("xxx") 或者定义全局类 LOGFile l("xxx")，整个工程的打印都打印到该文件中
   也可以完全不调用log_filename,直接调用 log_printf 如果不设定的话，就打印到默认文件名的日志中，默认文件名是当前工程的可执行文件的名称
   日志文件默认存在 /tmp 目录下，如果文件名是全路径如 "/mnt/wifi",日志文件会存在 /mnt 目录下
   日志文件默认使用 .log 后缀
3. 默认日志是不输出的,需要创建文件 logon,开启打印功能
4. 开机时没有开启打印,开机后再创建 logon 文件，终端输入 
    killall -10 进程名 
   可以使其实时生效，开启后，程序之后的打印会立刻打印到文件中
5. 打印可以设定等级，对应以下不同的函数，在终端打印时不同等级的日志颜色不同
6. 可以通过在logon文件中写入文本来控制某些打印等级的打印不生效，这个设定也可以通过 
    killall -10 进程名 
   使其实时生效
   0 所有打印都生效
   1 VERBOSE 以上等级(包含 DEBUG，WARNING，ERROR)才会打印，依次类推
   4 仅仅打印 ERROR 等级的日志
7。 logf_printf 和 logfa_printf 用于将日志打印到文件中，这两个函数不受打印开关影响，其中 tag 为打印文件名
    logf_printf 每次程序重新开启会覆盖之前的打印文件
    logfa_printf 每次程序重新开启会追加之前的打印文件
*/

#define log_printf(x, ...) _loghelp::get_log()->logl_printf(_log_file_name, &_fd, DEBUG_LEVEL_LOG, x, ##__VA_ARGS__)
#define logv_printf(x, ...) _loghelp::get_log()->logl_printf(_log_file_name, &_fd, DEBUG_LEVEL_VERBOSE, x, ##__VA_ARGS__)
#define logd_printf(x, ...) _loghelp::get_log()->logl_printf(_log_file_name, &_fd, DEBUG_LEVEL_DEDBUG, x, ##__VA_ARGS__)
#define logw_printf(x, ...) _loghelp::get_log()->logl_printf(_log_file_name, &_fd, DEBUG_LEVEL_WARNING, x, ##__VA_ARGS__)
#define loge_printf(x, ...) _loghelp::get_log()->logl_printf(_log_file_name, &_fd, DEBUG_LEVEL_ERROR, x, ##__VA_ARGS__)

#define LOG printf
#define LOGV printf
#define LOGD printf
#define LOGW printf
#define LOGE printf

#define LOGL() LOGV("--%s--%d------------\n", __FUNCTION__, __LINE__)
#define LOGV_ARRAY(a, l) _loghelp::get_log()->log_array(_log_file_name, &_fd, (uint8_t *)a, l)
#endif // !_LOG_H__
