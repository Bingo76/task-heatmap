CC = gcc
CFLAGS = `pkg-config --cflags gtk+-3.0` -Wall
LDFLAGS = `pkg-config --libs gtk+-3.0`
PREFIX = /usr

all: tasktracker

tasktracker: task_tracker.c
	$(CC) $(CFLAGS) -o tasktracker task_tracker.c $(LDFLAGS)

install: tasktracker
	install -Dm755 tasktracker $(DESTDIR)$(PREFIX)/bin/tasktracker

clean:
	rm -f tasktracker

