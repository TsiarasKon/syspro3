COMMON 			= ../common
COMMON_HEADERS  = $(COMMON)/requests.h $(COMMON)/lists.h $(COMMON)/util.h
COMMON_OBJS		= $(COMMON)/requests.o $(COMMON)/lists.o $(COMMON)/util.o
COMMON_SOURCE	= $(COMMON)/requests.c $(COMMON)/lists.c $(COMMON)/util.c

JOBEX 	 		= ../jobExecutor
JOBEX_OBJS		= $(JOBEX)/main.o $(JOBEX)/worker.o $(JOBEX)/postinglist.o $(JOBEX)/trie.o $(JOBEX)/lists.o $(JOBEX)/util.o
JOBEX_OUT		= $(JOBEX)/jobExecutor

OBJS 	= main.o 
SOURCE	= main.c 
OUT  	= mycrawler
CC		= gcc
FLAGS   = -g3 -c -pedantic -std=c99 -Wall
LIB 	= -lpthread

mycrawler: $(OBJS) $(COMMON_OBJS) $(COMMON_HEADERS) jobExecutor
	$(CC) -g3 $(OBJS) $(COMMON_OBJS) -o $(OUT) $(LIB)

.PHONY: jobExecutor
jobExecutor:
	cd $(JOBEX) && $(MAKE)

main.o: main.c $(COMMON_HEADERS)
	$(CC) $(FLAGS) main.c $(LIB)

requests.o: $(COMMON)/requests.c $(COMMON_HEADERS)
	$(CC) $(FLAGS) $(COMMON)/requests.c

lists.o: $(COMMON)/lists.c $(COMMON_HEADERS)
	$(CC) $(FLAGS) $(COMMON)/lists.c

util.o: $(COMMON)/util.c $(COMMON_HEADERS)
	$(CC) $(FLAGS) $(COMMON)/util.c


clean:
	rm -f $(OBJS) $(COMMON_OBJS) $(OUT) $(JOBEX_OBJS) $(JOBEX_OUT)

count:
	wc $(SOURCE) $(COMMON_SOURCE) $(HEADER) $(COMMON_HEADERS)

