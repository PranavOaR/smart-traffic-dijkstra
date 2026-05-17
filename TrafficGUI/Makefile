CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2 -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib -lraylib -lm \
          -framework IOKit -framework Cocoa -framework OpenGL

SRCS = main.c graph.c dijkstra.c gui.c
OBJ  = $(SRCS:.c=.o)

traffic_gui: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.c traffic.h
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJ) traffic_gui
