CC = gcc
DEBUGFLAGS = -g -Wall
CFLAGS = -D_REENTRANT $(DEBUGFLAGS) -D_XOPEN_SOURCE=500
LDFLAGS = -lpthread 

all: snowcast_listener snowcast_control snowcast_server

snowcast_listener: snowcast_listener.c networks.c
snowcast_control:  snowcast_control.c networks.c 
snowcast_server:   snowcast_server.c networks.c 
clean:
	rm -f snowcast_listener snowcast_control snowcast_server
