SHELL := /bin/bash
PROJECT = smxrts

LOC_INC_DIR = include
LOC_SRC_DIR = src
LOC_LIB_DIR = lib

STATLIB = $(LOC_LIB_DIR)/lib$(PROJECT).a

SOURCES = $(wildcard $(LOC_SRC_DIR)/*.c)

OBJECTS = $(SOURCES:%.c=%.o)

INCLUDES = $(LOC_INC_DIR)/*

INCLUDES_DIR = -I$(LOC_INC_DIR) \
			   -I.

LINK_FILE = -lpthread \
			-lzlog

CFLAGS = -Wall -c
DEBUG_FLAGS = -g -O0

CC = gcc

all: $(STATLIB)

# compile with dot stuff and debug flags
debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(LOC_SRC_DIR)/$(PROJECT).o

$(STATLIB): $(OBJECTS)
	mkdir -p lib
	ar -cq $@ $^

# compile project
$(OBJECTS): $(SOURCES) $(INCLUDES)
	$(CC) $(CFLAGS) $(SOURCES) $(INCLUDES_DIR) $(LINK_FILE) -o $@

.PHONY: clean install

install:
	mkdir -p /usr/local/include /usr/local/lib
	cp -a $(LOC_INC_DIR)/$(PROJECT).h /usr/local/include/.
	cp -a $(LOC_SRC_DIR)/$(PROJECT).o /usr/local/lib/.
	cp -a $(LOC_LIB_DIR)/lib$(PROJECT).a /usr/local/lib/.

clean:
	rm -f $(LOC_SRC_DIR)/$(PROJECT).o
	rm -f $(LOC_LIB_DIR)/lib$(PROJECT).a
