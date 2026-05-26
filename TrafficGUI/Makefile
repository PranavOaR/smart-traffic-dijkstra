CC      ?= gcc
PKG_CONFIG ?= pkg-config

RAYLIB_CFLAGS := $(shell $(PKG_CONFIG) --cflags raylib 2>/dev/null)
RAYLIB_LIBS   := $(shell $(PKG_CONFIG) --libs raylib 2>/dev/null)

CFLAGS  = -std=c11 -Wall -Wextra -O2 $(RAYLIB_CFLAGS)

ifeq ($(OS),Windows_NT)
	TARGET = traffic_gui.exe
	LDFLAGS = $(if $(RAYLIB_LIBS),$(RAYLIB_LIBS),-lraylib -lopengl32 -lgdi32 -lwinmm)
	RM = del /Q
else
	TARGET = traffic_gui
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		CFLAGS += -I/opt/homebrew/include
		LDFLAGS = $(if $(RAYLIB_LIBS),$(RAYLIB_LIBS),-L/opt/homebrew/lib -lraylib -lm -framework IOKit -framework Cocoa -framework OpenGL)
	else
		LDFLAGS = $(if $(RAYLIB_LIBS),$(RAYLIB_LIBS),-lraylib -lm -ldl -lpthread -lGL -lrt -lX11)
	endif
	RM = rm -f
endif

SRCS = main.c graph.c dijkstra.c gui.c
OBJ  = $(SRCS:.c=.o)

$(TARGET): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

all: $(TARGET)

%.o: %.c traffic.h
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) $(OBJ) $(TARGET)
