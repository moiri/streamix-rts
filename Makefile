SHELL := /bin/bash

PROJECT = smxrts
MAJ_VERSION = 0.1

LOC_INC_DIR = include
LOC_SRC_DIR = src
LOC_OBJ_DIR = obj
LOC_LIB_DIR = lib

LIBNAME = lib$(PROJECT)

TGT_INCLUDE = /opt/smx/include
TGT_LIB = /opt/smx/lib

STATLIB = $(LOC_LIB_DIR)/$(LIBNAME).a
DYNLIB = $(LOC_LIB_DIR)/$(LIBNAME).so

SOURCES = $(wildcard $(LOC_SRC_DIR)/*.c)
OBJECTS := $(patsubst $(LOC_SRC_DIR)/%.c, $(LOC_OBJ_DIR)/%.o, $(SOURCES))

INCLUDES = $(LOC_INC_DIR)/*.h

INCLUDES_DIR = -I$(LOC_INC_DIR) \
			   -I/usr/include/libxml2 \
			   -I./uthash/src \
			   -I.

CFLAGS = -Wall -fPIC
DEBUG_FLAGS = -g -O0

CC = gcc

all: $(STATLIB) $(DYNLIB)

# compile with dot stuff and debug flags
debug: CFLAGS += $(DEBUG_FLAGS)
debug: all

unsafe: CFLAGS += -DSMX_LOG_UNSAFE
unsafe: all

$(STATLIB): $(OBJECTS)
	mkdir -p lib
	ar -cq $@ $^

$(DYNLIB): $(OBJECTS)
	$(CC) -shared $^ -o $@

# compile project
$(LOC_OBJ_DIR)/%.o: $(LOC_SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES_DIR) $(LINK_FILE) -c $< -o $@

.PHONY: clean install uninstall doc

install:
	mkdir -p $(TGT_LIB) $(TGT_INCLUDE)
	cp -a $(INCLUDES) $(TGT_INCLUDE)/.
	cp -a $(LOC_LIB_DIR)/$(LIBNAME).a $(TGT_LIB)/$(LIBNAME).a.$(MAJ_VERSION)
	cp -a $(LOC_LIB_DIR)/$(LIBNAME).so $(TGT_LIB)/$(LIBNAME).so.$(MAJ_VERSION)
	ln -sf $(LIBNAME).so.$(MAJ_VERSION) $(TGT_LIB)/$(LIBNAME).so
	ln -sf $(LIBNAME).a.$(MAJ_VERSION) $(TGT_LIB)/$(LIBNAME).a

uninstall:
	rm $(addprefix $(TGT_INCLUDE)/,$(notdir $(wildcard $(INCLUDES))))
	rm $(TGT_LIB)/$(LIBNAME).a.$(MAJ_VERSION)
	rm $(TGT_LIB)/$(LIBNAME).so.$(MAJ_VERSION)
	rm $(TGT_LIB)/$(LIBNAME).a
	rm $(TGT_LIB)/$(LIBNAME).so

clean:
	rm -f $(LOC_OBJ_DIR)/*
	rm -f $(LOC_LIB_DIR)/$(LIBNAME).a
	rm -f $(LOC_LIB_DIR)/$(LIBNAME).so

doc:
	doxygen .doxygen
