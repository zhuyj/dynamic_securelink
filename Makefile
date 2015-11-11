all: server.c
	gcc -o server server.c -DDEBUG_MODE 
	gcc -o locallisten local_listen.c
clean::
	rm -f server locallisten
