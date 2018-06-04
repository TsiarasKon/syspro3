COMMON 			= common
COMMON_OBJS		= $(COMMON)/requests.o $(COMMON)/lists.o $(COMMON)/util.o

SERVER			= myhttpd
SERVER_OBJS 	= $(SERVER)/main.o
SERVER_OUT  	= $(SERVER)/myhttpd

CRAWLER			= mycrawler
CRAWLER_OBJS 	= $(CRAWLER)/main.o
CRAWLER_OUT  	= $(CRAWLER)/mycrawler

JOBEX 	 		= jobExecutor
JOBEX_OBJS		= $(JOBEX)/main.o $(JOBEX)/worker.o $(JOBEX)/postinglist.o $(JOBEX)/trie.o $(JOBEX)/lists.o $(JOBEX)/util.o
JOBEX_OUT		= $(JOBEX)/jobExecutor

CC		= gcc
FLAGS   = -g3 -c -pedantic -std=c99 -Wall
LIB 	= -lpthread

.PHONY: all myhttpd mycrawler jobExecutor

all: myhttpd mycrawler

myhttpd:
	cd $(SERVER) && $(MAKE)

mycrawler:
	cd $(CRAWLER) && $(MAKE)
	
jobExecutor:
	cd $(JOBEX) && $(MAKE)


clean:
	rm -f $(COMMON_OBJS) $(SERVER_OBJS) $(SERVER_OUT) $(CRAWLER_OBJS) $(CRAWLER_OUT) $(JOBEX_OBJS) $(JOBEX_OUT)


