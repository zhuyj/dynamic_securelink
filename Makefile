all: server.c
	gcc -o server server.c -lpthread
	gcc -o locallisten local_listen.c
clean::
	rm -f server locallisten
