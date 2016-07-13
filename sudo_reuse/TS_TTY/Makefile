CC = gcc

all: bindshell

bindshell: 
	$(CC) -o sudo_reuse_pty sudo_reuse_pty.c -lutil

clean:
	rm -f *.o
	rm -f sudo_reuse_pty
