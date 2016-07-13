CC = gcc

all: bindshell check

bindshell: 
	$(CC) -o sudo_reuse_ppid sudo_reuse_ppid.c -lutil

check:
	$(CC) -o check check.c 

clean:
	rm -f *.o
	rm -f sudo_reuse_ppid
	rm -f check
