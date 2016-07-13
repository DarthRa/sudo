#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if(getuid()==0)
    {
        printf("* run me without sudo after make sudo session\n");
        return 0;
    }

    printf("* If %s asked password, It means you don't have a sudo session.\n", argv[0]);
    printf("* Make a sudo session by invoke 'sudo id' or whatever you like, then try it again.\n");

    system("sudo touch this_file_owner_should_be_root");

    printf("* done\n");
    return 0;
}
