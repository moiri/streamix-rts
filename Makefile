SHELL := /bin/bash

include config.mk

VERSION_LIB = $(VMAJ).$(VMIN)

LOC_INC_DIR = include
LOC_SRC_DIR = src
LOC_BUILD_DIR = build
LOC_OBJ_DIR = $(LOC_BUILD_DIR)/obj
LOC_LIB_DIR = $(LOC_BUILD_DIR)/lib
CREATE_DIR = $(LOC_OBJ_DIR) $(LOC_LIB_DIR)

DPKG_DIR = dpkg
DPKG_CTL_DIR = build/debian
DPKG_TGT = DEBIAN
DPKGS = $(DPKG_DIR)/$(LIBNAME)_$(VERSION)_amd64 \
	   $(DPKG_DIR)/$(LIBNAME)_amd64-dev

LIB_VERSION = $(VMAJ).$(VMIN)
UPSTREAM_VERSION = $(LIB_VERSION).$(VREV)
DEBIAN_REVISION = 0
VERSION = $(UPSTREAM_VERSION)-$(DEBIAN_REVISION)

VLIBNAME = $(LIBNAME)-$(LIB_VERSION)
SONAME = $(LIBNAME).so.$(LIB_VERSION)
ANAME = $(LIBNAME).a

TGT_INCLUDE = /opt/smx/include
TGT_DOC = /opt/smx/doc
TGT_CONF = /opt/smx/conf
TGT_LOG = /opt/smx/log
TGT_LIB = /opt/smx/lib
TGT_LIB_E = \/opt\/smx\/lib

STATLIB = $(LOC_LIB_DIR)/$(LIBNAME).a
DYNLIB = $(LOC_LIB_DIR)/$(LIBNAME).so

SOURCES = $(wildcard $(LOC_SRC_DIR)/*.c)
OBJECTS := $(patsubst $(LOC_SRC_DIR)/%.c, $(LOC_OBJ_DIR)/%.o, $(SOURCES))

INCLUDES = $(LOC_INC_DIR)/*.h

INCLUDES_DIR = -I$(LOC_INC_DIR) \
			   -I/usr/include/libbson-1.0 \
			   -I.

LINK_DIR = -L/usr/local/lib

LINK_FILE = -lpthread \
	-lbson-1.0 \
	-lzlog \
	-llttng-ust \
	-ldl

CFLAGS = -Wall -fPIC -DLIBSMXRTS_VERSION=\"$(UPSTREAM_VERSION)\"
DEBUG_FLAGS = -g -O0

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

.PHONY: clean install uninstall doc directories dpkg $(DPKGS)

directories: $(CREATE_DIR)

$(CREATE_DIR):
	mkdir -p $@

install:
	mkdir -p $(TGT_LIB) $(TGT_INCLUDE) $(TGT_CONF) $(TGT_LOG)
	cp -a tpl/default.zlog $(TGT_CONF)/.
	cp -a $(INCLUDES) $(TGT_INCLUDE)/.
	cp -a $(LOC_LIB_DIR)/$(LIBNAME).so $(TGT_LIB)/$(SONAME)
	ln -sf $(SONAME) $(TGT_LIB)/$(VLIBNAME).so
	ln -sf $(SONAME) $(TGT_LIB)/$(LIBNAME).so

uninstall:
	rm $(addprefix $(TGT_INCLUDE)/,$(notdir $(wildcard $(INCLUDES))))
	rm $(TGT_LIB)/$(SONAME)
	rm $(TGT_LIB)/$(LIBNAME).so
	rm $(TGT_LIB)/$(VLIBNAME).so

clean:
	rm -rf $(LOC_BUILD_DIR)

doc:
	doxygen .doxygen
