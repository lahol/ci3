CC=gcc
CFLAGS=-Wall -g `pkg-config --cflags glib-2.0 gio-2.0 gtk+-3.0 json-glib-1.0`
LIBS=`pkg-config --libs glib-2.0 gio-2.0 gtk+-3.0 json-glib-1.0` -lcinet

CIVERSION := '$(shell git describe --tags --always)  ($(shell git log --pretty=format:%cd --date=short -n1), branch \"$(shell git describe --tags --always --all | sed s:heads/::)\")'

CFLAGS += -DCIVERSION=\"${CIVERSION}\"

ci_SRC := $(wildcard *.c)
ci_OBJ := $(ci_SRC:.c=.o)
ci_HEADERS := $(wildcard *.h)

all: ciclient

ciclient: $(ci_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

%.o: %.c $(ci_HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f ciclient $(ci_OBJ)

.PHONY: all clean
