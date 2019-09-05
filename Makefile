SHELL := /bin/bash

PROJECT = smxrts
VMAJ := $(if $(VMAJ),$(VMAJ),0)
VMIN := $(if $(VMIN),$(VMIN),1)
VREV := $(if $(VREV),$(VREV),0)

VERSION_LIB = $(VMAJ).$(VMIN)

LOC_INC_DIR = include
LOC_SRC_DIR = src
LOC_OBJ_DIR = obj
LOC_LIB_DIR = lib
CREATE_DIR = $(LOC_OBJ_DIR) $(LOC_LIB_DIR)

LIBNAME = lib$(PROJECT)
SONAME = $(LIBNAME)-$(VMAJ).$(VMIN).so.$(VREV)
ANAME = $(LIBNAME)-$(VMAJ).$(VMIN).a

TGT_INCLUDE = /opt/smx/include
TGT_LIB = /opt/smx/lib

STATLIB = $(LOC_LIB_DIR)/$(LIBNAME).a
DYNLIB = $(LOC_LIB_DIR)/$(LIBNAME).so

SOURCES = $(wildcard $(LOC_SRC_DIR)/*.c)
OBJECTS := $(patsubst $(LOC_SRC_DIR)/%.c, $(LOC_OBJ_DIR)/%.o, $(SOURCES))

INCLUDES = $(LOC_INC_DIR)/*.h

INCLUDES_DIR = -I$(LOC_INC_DIR) \
			   -I/usr/include/libbson-1.0 \
			   -I./uthash/src \
			   -I.

LINK_DIR = -L/usr/local/lib

LINK_FILE = -lpthread \
	-lbson \
	-lzlog \
	-llttng-ust \
	-ldl

CFLAGS = -Wall -fPIC
DEBUG_FLAGS = -g -O0 -DLTTNG_UST_DEBUG -DLTTNG_UST_DEBUG_VALGRIND

CC = gcc

all: directories $(STATLIB) $(DYNLIB)

# compile with dot stuff and debug flags
debug: CFLAGS += $(DEBUG_FLAGS)
debug: all

unsafe: CFLAGS += -DSMX_LOG_UNSAFE
unsafe: all

$(STATLIB): $(OBJECTS)
	ar -cq $@ $^

$(DYNLIB): $(OBJECTS)
	$(CC) -shared -Wl,-soname,$(SONAME) $^ -o $@ $(LINK_DIR) $(LINK_FILE)

# compile project
$(LOC_OBJ_DIR)/%.o: $(LOC_SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES_DIR) -c $< -o $@ $(LINK_DIR) $(LINK_FILE)

.PHONY: clean install uninstall doc directories

directories: $(CREATE_DIR)

$(CREATE_DIR):
	mkdir -p $@

install:
	mkdir -p $(TGT_LIB) $(TGT_INCLUDE)
	cp -a $(INCLUDES) $(TGT_INCLUDE)/.
	cp -a $(LOC_LIB_DIR)/$(LIBNAME).a $(TGT_LIB)/$(ANAME)
	cp -a $(LOC_LIB_DIR)/$(LIBNAME).so $(TGT_LIB)/$(SONAME)
	ln -sf $(SONAME) $(TGT_LIB)/$(LIBNAME).so
	ln -sf $(ANAME) $(TGT_LIB)/$(LIBNAME).a

uninstall:
	rm $(addprefix $(TGT_INCLUDE)/,$(notdir $(wildcard $(INCLUDES))))
	rm $(TGT_LIB)/$(ANAME)
	rm $(TGT_LIB)/$(SONAME)
	rm $(TGT_LIB)/$(LIBNAME).a
	rm $(TGT_LIB)/$(LIBNAME).so

clean:
	rm -rf $(LOC_OBJ_DIR)
	rm -rf $(LOC_LIB_DIR)

doc:
	doxygen .doxygen
