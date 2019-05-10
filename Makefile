SHELL := /bin/bash
PROJECT = smxrts

LOC_INC_DIR = include
LOC_SRC_DIR = src
LOC_LIB_DIR = lib

STATLIB = $(LOC_LIB_DIR)/lib$(PROJECT).a
DYNLIB = $(LOC_LIB_DIR)/lib$(PROJECT).so

SOURCES = $(wildcard $(LOC_SRC_DIR)/*.c)

OBJECTS = $(SOURCES:%.c=%.o)

INCLUDES = $(LOC_INC_DIR)/*

INCLUDES_DIR = -I$(LOC_INC_DIR) \
			   -I/usr/include/libxml2 \
			   -I./uthash/src \
			   -I.

LINK_FILE = -lpthread \
			-lzlog \
			-lxml2

CFLAGS = -Wall -c -fPIC
DEBUG_FLAGS = -g -O0

CC = gcc

# compile with dot stuff and debug flags
debug: CFLAGS += $(DEBUG_FLAGS)
debug: all

all: $(STATLIB) $(DYNLIB)

$(STATLIB): $(OBJECTS)
	mkdir -p lib
	ar -cq $@ $^

$(DYNLIB): $(OBJECTS)
	$(CC) -shared $^ -o $@

# compile project
$(OBJECTS): $(SOURCES) $(INCLUDES)
	$(CC) $(CFLAGS) $(SOURCES) $(INCLUDES_DIR) $(LINK_FILE) -o $@

.PHONY: clean install doc

install:
	mkdir -p /usr/local/include /usr/local/lib
	cp -a $(LOC_INC_DIR)/$(PROJECT).h /usr/local/include/.
	cp -a $(LOC_LIB_DIR)/lib$(PROJECT).a /usr/local/lib/.
	cp -a $(LOC_LIB_DIR)/lib$(PROJECT).so /usr/local/lib/.

clean:
	rm -f $(LOC_SRC_DIR)/$(PROJECT).o
	rm -f $(LOC_LIB_DIR)/lib$(PROJECT).a
	rm -f $(LOC_LIB_DIR)/lib$(PROJECT).so

doc:
	doxygen .doxygen

