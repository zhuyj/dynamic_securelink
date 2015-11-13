#CFLAGS=-DDEBUG_MODE
CFLAGS= -lpthread
all: server.c local_listen.c dynamic_securelink.h
	gcc -o server server.c ${CFLAGS}
	gcc -o locallisten local_listen.c
clean::
	rm -f server locallisten
