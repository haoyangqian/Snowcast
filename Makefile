CC=gcc
DEBUGFLAGS= -g -Wall

all:snowcast_control snowcast_listener

snowcast_control: snowcast_control.c networks.c
snowcast_listener:snowcast_listener.c networks.c

clean:
	rm -f snowcast_control snowcast_listener
