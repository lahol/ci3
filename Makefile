CC=gcc
CFLAGS=-Wall `pkg-config --cflags glib-2.0 gio-2.0`
LIBS=`pkg-config --libs glib-2.0 gio-2.0` -lcinet

all: ciclient

ciclient: client.o main.o
	$(CC) -o ciclient $(LIBS) client.o main.o

client.o: client.h client.c
	$(CC) $(CFLAGS) -c -o client.o client.c

main.o: main.c
	$(CC) $(CFLAGS) -c -o main.o main.c

clean:
	rm -f ciclient *.o
