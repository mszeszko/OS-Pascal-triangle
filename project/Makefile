CC = gcc
CFLAGS = -Wall -ansi --pedantic -c
LDFLAGS = -Wall -ansi --pedantic

OBJECTS = err.o common.o
ALL = pascal worker

all: $(ALL)

%.o : %.c
	$(CC) $(CFLAGS) $<

$(ALL) : % : %.o $(OBJECTS)       
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f *.o $(ALL) *~
