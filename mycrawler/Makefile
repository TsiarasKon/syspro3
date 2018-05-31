OBJS 	= main.o requests.o lists.o util.o
SOURCE	= main.c requests.c lists.c util.c
HEADER  = requests.h lists.h util.h
OUT  	= mycrawler
CC		= gcc
FLAGS   = -g3 -c -pedantic -std=c99 -Wall
LIB 	= -lpthread

myhttpd: $(OBJS)
	$(CC) -g3 $(OBJS) -o $(OUT) $(LIB)

main.o: main.c
	$(CC) $(FLAGS) main.c $(LIB)

requests.o: requests.c
	$(CC) $(FLAGS) requests.c

lists.o: lists.c
	$(CC) $(FLAGS) lists.c

util.o: util.c
	$(CC) $(FLAGS) util.c


clean:
	rm -f $(OBJS) $(OUT)

count:
	wc $(SOURCE) $(HEADER)
