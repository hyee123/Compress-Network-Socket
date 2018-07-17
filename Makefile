#!/bin/bash
#Name:Harry Yee
#Email:hyee1234@gmail.com
#UID:704754172

default:
	#compile
	gcc -Wall -Wextra -o lab1b-client lab1b-client.c -lz
	gcc -Wall -Wextra -o lab1b-server lab1b-server.c -lz

client:
	gcc -Wall -Wextra -o lab1b-client lab1b-client.c -lz
server:
	gcc -Wall -Wextra -o lab1b-server lab1b-server.c -lz
clean:

	rm lab1b-client lab1b-server lab1b-704754172.tar.gz

dist: default

	tar -czf lab1b-704754172.tar.gz lab1b-client.c lab1b-server.c  README Makefile
