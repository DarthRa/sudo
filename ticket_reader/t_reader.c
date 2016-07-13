/*
 * This program reads Ticket Container and print out all entries to stdout.
 * It works well with the latest version of sudo(currently sudo-1.8.12b3).
 *
 * Author : Jeong JiHoon
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

#define TS_VERSION      1

/* Time stamp entry types */
#define TS_GLOBAL       0x01
#define TS_TTY          0x02
#define TS_PPID         0x03

/* Time stamp flags */
#define TS_DISABLED     0x01    /* entry disabled */
#define TS_ANYUID       0x02    /* ignore uid, only valid in the key */

char *sudo_path = "/var/run/sudo/ts/";

// structure from sudo-1.8.12b3
struct timestamp_entry {
    unsigned short version; /* version number */
    unsigned short size;    /* entry size */
    unsigned short type;    /* TS_GLOBAL, TS_TTY, TS_PPID */
    unsigned short flags;   /* TS_DISABLED, TS_ANYUID */
    uid_t auth_uid;     /* uid to authenticate as */
    pid_t sid;          /* session ID associated with tty/ppid */
    struct timespec ts;     /* timestamp (CLOCK_MONOTONIC) */
    union {
    dev_t ttydev;       /* tty device number */
    pid_t ppid;     /* parent pid */
    } u;
};

char *get_type(short type)
{
    switch(type)
    {
        case 1: return "TS_GLOBAL";
        case 2: return "TS_TTY";
        case 3: return "TS_PPID";
        default: return "TS_UNKNOWN";
    }
}

char *get_flag(short flag)
{
    switch(flag)
    {
        case 0: return "TS_NO_FLAG";
        case 1: return "TS_DISABLED";
        case 2: return "TS_ANYUID";
        case 3: return "TS_DISABLED | TS_ANYUID";
        default: return "TS_UNKNOWN";
    }
}

void print_out_ticket(unsigned char *tdb, int size)
{
    struct timestamp_entry *p = NULL;
    struct timespec ct;
    int i, rem_time;

    printf("%78s\n", "PTS ID"); 
    printf("%79s\n", "--------"); 
    printf("%4s %4s %10s %14s %8s %8s %10s %4s %4s %4s %8s\n", 
            "VER", "SIZE", "TYPE", "FLAG", "UID", "SID", "TIMESTAMP", "REM", "MAJ", "MIN", "PPID");

    for(i=0 ; i<(79+9) ; i++)
        putchar('-');
    putchar('\n');

    clock_gettime(CLOCK_BOOTTIME, &ct);

    while(size>0)
    {
        p = (struct timestamp_entry *)tdb;

        rem_time = (ct.tv_sec - p->ts.tv_sec);

        // calculate remaining time
        if(rem_time<5*60)
            rem_time = (5*60) - rem_time;
        else
            rem_time = 0;

        if(p->type==TS_TTY)
        {
            printf("%4d %4d %10s %14s %8d %8d %10ld %4d %4d %4d %8s\n", 
                    p->version, p->size, get_type(p->type), get_flag(p->flags),
                    p->auth_uid, p->sid, p->ts.tv_sec, rem_time, major(p->u.ttydev), minor(p->u.ttydev), "-");
        }
        else if(p->type==TS_PPID)
        {
            printf("%4d %4d %10s %14s %8d %8d %10ld %4d %4s %4s %8d\n", 
                    p->version, p->size, get_type(p->type), get_flag(p->flags),
                    p->auth_uid, p->sid, p->ts.tv_sec, rem_time, "-", "-", p->u.ppid);

        }
        else
            printf("* I don't know how to treat TS_GLOBAL entry .. Is there a example??\n");

        tdb += p->size;
        size -= p->size;
    }
}

int main(int argc, char **argv)
{
    char completePath[256];
    unsigned char *ticket_db = NULL;
    int fsize;
    FILE *fp = NULL;
    
    if(getuid()!=0)
    {
        printf("* I need a root privilege for reading tickets\n");
        return -1;
    }

    if(argc!=2)
    {
        printf("* Usage: %s [username]\n", argv[0]);
        return -1;
    }

    sprintf(completePath, "%s%s", sudo_path, argv[1]); 

    if((fp = fopen(completePath, "rb"))==NULL)
    {
        printf("* failed to read ticket container(path=%s).\n", completePath);
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if((ticket_db = malloc(fsize))==NULL)
    {
        printf("* failed to allocate memory\n");
        fclose(fp);
        return -1;
    }

    fread(ticket_db, fsize, 1, fp);

    print_out_ticket(ticket_db, fsize);

    fclose(fp);
    free(ticket_db);

    return 0;
}
