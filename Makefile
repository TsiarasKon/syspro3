COMMON 			= common
COMMON_HEADERS  = $(COMMON)/requests.h $(COMMON)/lists.h $(COMMON)/util.h
COMMON_OBJS		= $(COMMON)/requests.o $(COMMON)/lists.o $(COMMON)/util.o
COMMON_SOURCE	= $(COMMON)/requests.c $(COMMON)/lists.c $(COMMON)/util.c

SERVER_DIR		= myhttpd
SERVER_OBJS 	= $(SERVER_DIR)/main.o
SERVER_SOURCE	= $(SERVER_DIR)/main.c
SERVER_OUT  	= $(SERVER_DIR)/myhttpd

CRAWLER_DIR		= mycrawler
CRAWLER_OBJS 	= $(CRAWLER_DIR)/main.o
CRAWLER_SOURCE	= $(CRAWLER_DIR)/main.c
CRAWLER_OUT  	= $(CRAWLER_DIR)/mycrawler

CC		= gcc
FLAGS   = -g3 -c -pedantic -std=c99 -Wall
LIB 	= -lpthread

.PHONY: all myhttpd mycrawler

all: myhttpd mycrawler

myhttpd:
	cd $(SERVER_DIR) && $(MAKE)

mycrawler:
	cd $(CRAWLER_DIR) && $(MAKE)


clean:
	rm -f $(COMMON_OBJS) $(SERVER_OBJS) $(SERVER_OUT) $(CRAWLER_OBJS) $(CRAWLER_OUT)

count:
	wc $(COMMON_SOURCE) $(COMMON_HEADERS) $(SERVER_SOURCE) $(SERVER_HEADERS) $(CRAWLER_SOURCE) $(CRAWLER_HEADERS)



