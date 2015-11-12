#CFLAGS=-DDEBUG_MODE
CFLAGS=
all: server.c
	gcc -o server server.c ${CFLAGS}
	gcc -o locallisten local_listen.c
clean::
	rm -f server locallisten
