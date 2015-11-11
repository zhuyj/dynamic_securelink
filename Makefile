#CFLAG=-DDEBUG_MODE
CFLAG=
all: server.c
	gcc -o server server.c ${CFLAG}
	gcc -o locallisten local_listen.c
clean::
	rm -f server locallisten
