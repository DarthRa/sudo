#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char **argv)
{
    int nread, fd, flags;
    char *pipe_path = "/tmp/sproxy";
    char buf[1024];
    fd_set rdfs, wrdfs;
    struct timeval tv;

    FD_ZERO(&rdfs);
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    
    if((fd=open(pipe_path, O_RDWR))<0)
    {
        fprintf(stderr, "* failed to open pipe\n");
        return -1;
    }

    if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
       flags = 0;

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        return 1;
    }

    write(fd, argv[1], strlen(argv[1]));

    FD_SET(fd, &rdfs);

    while (1) {
        wrdfs = rdfs;
        switch(select(fd+1, &wrdfs, NULL, NULL, &tv))
        {
            case -1:
                break;
            case 0:
                close(fd);
                return 0;
            default:
                nread = read(fd, buf, 254);
                if (nread == -1) {
                    if (errno == EAGAIN) {
                    // no, sleep and try again
                    usleep(1000);
                    continue;
                }
                perror("read");
                break;
            }   
            int i;
            for (i = 0; i < nread; i++) {
                putchar(buf[i]);
            }
        }
    }

    close(fd);

    return 0;
}
