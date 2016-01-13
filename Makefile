CC=gcc
CINCLUDE=-I../libev4.2
CFLAGS=-Wall -g -fPIC $(CINCLUDE)
LDFLAGS=-lpthread

SOURCES=api.c
OBJS=$(patsubst %.c, %.o,$(SOURCES))
TARGET=libevapi.so

%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET):$(OBJS)
	$(CC) $(OBJS) -shared  -o $(TARGET) $(LDFLAGS)

clean:
	rm -rf *.o $(TARGET)

install:
	cp $(TARGET) ../main

