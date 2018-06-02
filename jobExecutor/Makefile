OBJS 	= main.o worker.o postinglist.o trie.o lists.o util.o
SOURCE	= main.c worker.c postinglist.c trie.c lists.c util.c
HEADER  = paths.h postinglist.h trie.h lists.h util.h
OUT  	= jobExecutor
CC		= gcc
FLAGS   = -g3 -c -pedantic -std=c99 -Wall

$(OUT): $(OBJS)
	$(CC) -g3 $(OBJS) -o $@

main.o: main.c $(HEADER)
	$(CC) $(FLAGS) main.c

worker.o: worker.c $(HEADER)
	$(CC) $(FLAGS) worker.c

postinglist.o: postinglist.c $(HEADER)
	$(CC) $(FLAGS) postinglist.c

trie.o: trie.c $(HEADER)
	$(CC) $(FLAGS) trie.c

lists.o: lists.c $(HEADER)
	$(CC) $(FLAGS) lists.c

util.o: util.c $(HEADER)
	$(CC) $(FLAGS) util.c


clean:
	rm -f $(OBJS) $(OUT)

count:
	wc $(SOURCE) $(HEADER)
