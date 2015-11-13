CFLAGS=-DDEBUG_MODE
#CFLAGS= -lpthread
all: dynamic_securelink.c local_listen.c dynamic_securelink.h
	gcc -o dynamic_securelink dynamic_securelink.c ${CFLAGS}
	gcc -o locallisten local_listen.c
clean::
	rm -f dynamic_securelink locallisten
