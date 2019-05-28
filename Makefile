SHELL := /bin/bash
PROJECT = smxrts

LOC_INC_DIR = include
LOC_SRC_DIR = src
LOC_OBJ_DIR = obj
LOC_LIB_DIR = lib

STATLIB = $(LOC_LIB_DIR)/lib$(PROJECT).a
DYNLIB = $(LOC_LIB_DIR)/lib$(PROJECT).so

SOURCES = $(wildcard $(LOC_SRC_DIR)/*.c)
OBJECTS := $(patsubst $(LOC_SRC_DIR)/%.c, $(LOC_OBJ_DIR)/%.o, $(SOURCES))
#SOURCES = $(LOC_SRC_DIR)/smxrts.c

#OBJECTS = $(SOURCES:%.c=%.o)

INCLUDES = $(LOC_INC_DIR)/smxrts.h

INCLUDES_DIR = -I$(LOC_INC_DIR) \
			   -I/usr/include/libxml2 \
			   -I./uthash/src \
			   -I.

LINK_FILE = -lpthread \
			-lzlog \
			-lxml2

CFLAGS = -Wall -fPIC
DEBUG_FLAGS = -g -O0

CC = gcc

all: $(STATLIB) $(DYNLIB)

# compile with dot stuff and debug flags
debug: CFLAGS += $(DEBUG_FLAGS)
debug: all

$(STATLIB): $(OBJECTS)
	mkdir -p lib
	ar -cq $@ $^

$(DYNLIB): $(OBJECTS)
	$(CC) -shared $^ -o $@

# compile project
#$(OBJECTS): $(SOURCES) $(INCLUDES)
$(LOC_OBJ_DIR)/%.o: $(LOC_SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES_DIR) $(LINK_FILE) -c $< -o $@

.PHONY: clean install doc

install:
	mkdir -p /usr/local/include/libsmxrts /usr/local/lib
	cp -a $(LOC_INC_DIR)/*.h /usr/local/include/libsmxrts/.
	cp -a $(LOC_LIB_DIR)/lib$(PROJECT).a /usr/local/lib/.
	cp -a $(LOC_LIB_DIR)/lib$(PROJECT).so /usr/local/lib/.

clean:
	rm -f $(LOC_OBJ_DIR)/*
	rm -f $(LOC_LIB_DIR)/lib$(PROJECT).a
	rm -f $(LOC_LIB_DIR)/lib$(PROJECT).so

doc:
	doxygen .doxygen

