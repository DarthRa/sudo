CC = gcc

all: lib fwd

lib: proxy.o
	$(CC) -shared -o libshproxy.so *.o -lpthread -lutil

fwd:
	$(CC) -o fwd fwd.c

proxy.o:
	$(CC) -c -fPIC proxy.c

clean:
	rm -f *.o
	rm -f *.so
	rm -f fwd
