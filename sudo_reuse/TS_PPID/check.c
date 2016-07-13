#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main()
{
    printf("ppid: %d, sid: %d\n", getppid(), getsid(getpid()));
    return 0;
}
