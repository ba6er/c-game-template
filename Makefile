.PHONY: all build clean

# Compiler
CC := gcc

# Target Binary Program
TB := game

# Extensions
SE := .c
OE := .o

# Directories
SD := src
OD := obj
BD := bin

# Compile Flags, Includes, Libraries
#CFLAGS  := -D_NO_DEBUG -O2 -std=c99 -pedantic -Wall -m64
CFLAGS  := -O0 -std=c99 -pedantic -Wall -m64
LDFLAGS := $(DEPS_OBJECT_FILES) -s -lSDL2 -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lm

SOURCE_FILES := $(wildcard $(SD)/*$(SE))
OBJECT_FILES := $(SOURCE_FILES:$(SD)/%$(SE)=$(OD)/%$(OE))
BINARY_FILE  := $(BD)/$(TB)

# Building The Program

all: build

build: $(BINARY_FILE)

$(BINARY_FILE): $(OBJECT_FILES)
	@mkdir -p $(BD)
	$(CC) $^ $(LDFLAGS) -o $@
	@echo "Build successful!"

$(OBJECT_FILES): $(OD)/%$(OE): $(SD)/%$(SE)
	@mkdir -p $(OD)
	$(CC) -c $< $(CFLAGS) -o $@

clean:
	rm -f $(OBJECT_FILES) $(BINARY_FILE)

