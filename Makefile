CC = gcc
CFLAGS = `pkg-config --cflags gtk+-3.0` -Wall
LDFLAGS = `pkg-config --libs gtk+-3.0`
PREFIX = /usr

all: task-heatmap

task-heatmap: task_heatmap.c
	$(CC) $(CFLAGS) -o task-heatmap task_heatmap.c $(LDFLAGS)

install: task-heatmap
	install -Dm755 task-heatmap $(DESTDIR)$(PREFIX)/bin/task-heatmap

clean:
	rm -f task-heatmap

