#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>

/* major/minor number */
#include <sys/sysmacros.h>

#include <signal.h>   /* catch signals */
#include "readproc.h"

#define proc_path "/proc"
#define KLF "L"

int simple_nextpid(DIR *procfs)
{
    static struct dirent *ent;
    char *path = proc_path;
    int tid;

    while(1)
    {
        ent = readdir(procfs);
        if(ent==NULL || ent->d_name==NULL)
            return 0;
        if((*ent->d_name>'0') && (*ent->d_name <= '9'))
            break;
    }
   
    tid = strtoul(ent->d_name, NULL, 10);
    return tid; 
}

int file2str(const char *directory, const char *what, char *ret, int cap) {
    static char filename[80];
    int fd, num_read;

    sprintf(filename, "%s/%s", directory, what);
    fd = open(filename, O_RDONLY, 0);
    if(fd==-1) return -1;
    num_read = read(fd, ret, cap - 1);
    close(fd);
    if(num_read<=0) return -1;
    ret[num_read] = '\0';
    return num_read;
}

void stat2proc(char *S, proc_t *P)
{
    unsigned num;
    int size;
    char *tmp;

    /* fill in default values for older kernels */
    P->processor = 0;
    P->rtprio = -1;
    P->sched = -1;
    P->nlwp = 0;

    S = strchr(S, '(') + 1;
    tmp = strrchr(S, ')');
    num = tmp - S;
    if(num >= sizeof(P->cmd)) num = sizeof(P->cmd) - 1;
    memcpy(P->cmd, S, num);
    P->cmd[num] = '\0';
    S = tmp + 2;                 // skip ") "

    num = sscanf(S,
       "%c "
       "%d %d %d %d %d "
       "%lu %lu %lu %lu %lu "
       "%Lu %Lu %Lu %Lu "  /* utime stime cutime cstime */
       "%ld %ld "
       "%d "
       "%ld "
       "%Lu "  /* start_time */
       "%lu "
       "%ld "
       "%lu %"KLF"u %"KLF"u %"KLF"u %"KLF"u %"KLF"u "
       "%*s %*s %*s %*s " /* discard, no RT signals & Linux 2.1 used hex */
       "%"KLF"u %*lu %*lu "
       "%d %d "
       "%lu %lu",
       &P->state,
       &P->ppid, &P->pgrp, &P->session, &P->tty, &P->tpgid,
       &P->flags, &P->min_flt, &P->cmin_flt, &P->maj_flt, &P->cmaj_flt,
       &P->utime, &P->stime, &P->cutime, &P->cstime,
       &P->priority, &P->nice,
       &P->nlwp,
       &P->alarm,
       &P->start_time,
       &P->vsize,
       &P->rss,
       &P->rss_rlim, &P->start_code, &P->end_code, &P->start_stack, &P->kstk_esp, &P->kstk_eip,
/*     P->signal, P->blocked, P->sigignore, P->sigcatch,   */ /* can't use */
       &P->wchan, /* &P->nswap, &P->cnswap, */  /* nswap and cnswap dead for 2.4.xx and up */
/* -- Linux 2.0.35 ends here -- */
       &P->exit_signal, &P->processor,  /* 2.2.1 ends with "exit_signal" */
/* -- Linux 2.2.8 to 2.5.17 end here -- */
       &P->rtprio, &P->sched  /* both added to 2.5.18 */
    ); 

    if(!P->nlwp)
        P->nlwp = 1;
}

int main(int argc, char **argv)
{
    DIR *procfs = NULL;
    int tid;
    proc_t P;


    while(1)
    {
        procfs = opendir(proc_path);
        while((tid=simple_nextpid(procfs)))
        {
            char proc_stat_path[128];
            char sbuf[1024];
            sprintf(proc_stat_path, "%s/%d/", proc_path, tid);
            file2str(proc_stat_path, "stat", sbuf, sizeof(sbuf));

            if(strstr(sbuf, "sudo"))
            {
                stat2proc(sbuf, &P); 
                printf("SUDO : PID: %5d, PPID: %5d, TTY: %d, pts/%d, SID: %5d\n", tid, P.ppid, major(P.tty), minor(P.tty), getsid(tid));
                return -1;
            }
        }
        closedir(procfs);

        sleep(1);
    }

    return 0;
} 
