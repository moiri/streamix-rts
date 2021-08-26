SHELL := /bin/bash

include config.mk

LLIBNAME = lib$(LIBNAME)
LOC_INC_DIR = include
LOC_SRC_DIR = src
LOC_BUILD_DIR = build
LOC_OBJ_DIR = $(LOC_BUILD_DIR)/obj
LOC_LIB_DIR = $(LOC_BUILD_DIR)/lib
CREATE_DIR = $(LOC_OBJ_DIR) $(LOC_LIB_DIR)

LIB_VERSION = $(VMAJ).$(VMIN)
UPSTREAM_VERSION = $(LIB_VERSION).$(VREV)
DEBIAN_REVISION = $(VDEB)
VERSION = $(UPSTREAM_VERSION)-$(DEBIAN_REVISION)

VLIBNAME = $(LLIBNAME)-$(LIB_VERSION)
SONAME = $(LLIBNAME).so.$(LIB_VERSION)
ANAME = $(LLIBNAME).a

TGT_INCLUDE = $(DESTDIR)/usr/include/smx/$(VLIBNAME)
TGT_LIB = $(DESTDIR)/usr/lib/x86_64-linux-gnu
TGT_DOC = $(DESTDIR)/usr/share/doc/smx
TGT_CONF = $(DESTDIR)/usr/etc/smx
TGT_LOG = $(DESTDIR)/var/log/smx

STATLIB = $(LOC_LIB_DIR)/$(LLIBNAME).a
DYNLIB = $(LOC_LIB_DIR)/$(LLIBNAME).so

SOURCES = $(wildcard $(LOC_SRC_DIR)/*.c)
OBJECTS := $(patsubst $(LOC_SRC_DIR)/%.c, $(LOC_OBJ_DIR)/%.o, $(SOURCES))

INCLUDES = $(LOC_INC_DIR)/*.h

INCLUDES_DIR = -I$(LOC_INC_DIR) \
			   -I/usr/include/libbson-1.0 \
			   $(INC_SMXZLOG) \
			   -I.

LINK_DIR = -L/usr/local/lib

LINK_FILE = -lpthread \
	-lbson-1.0 \
	$(LIB_SMXZLOG) \
	-llttng-ust \
	-ldl

CFLAGS = -Wall -fPIC -DLIBSMXRTS_VERSION=\"$(UPSTREAM_VERSION)\"
DEBUG_FLAGS = -g -O0

CC = gcc

all: directories $(STATLIB) $(DYNLIB)

# compile with dot stuff and debug flags
debug: CFLAGS += $(DEBUG_FLAGS)
debug: all

lock: CFLAGS += -DSMX_LOG_LOCK
lock: all

no-log: CFLAGS += -DSMX_LOG_DISABLE
no-log: all

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
	mkdir -p $(TGT_LIB) $(TGT_INCLUDE) $(TGT_CONF) $(TGT_LOG)
	cp -a $(INCLUDES) $(TGT_INCLUDE)/.
	cp -a $(LOC_LIB_DIR)/$(LLIBNAME).so $(TGT_LIB)/$(SONAME)
	ln -sf $(SONAME) $(TGT_LIB)/$(VLIBNAME).so
	ln -sf $(SONAME) $(TGT_LIB)/$(LLIBNAME).so

uninstall:
	rm $(addprefix $(TGT_INCLUDE)/,$(notdir $(wildcard $(INCLUDES))))
	rm $(TGT_LIB)/$(SONAME)
	rm $(TGT_LIB)/$(VLIBNAME).so
	rm $(TGT_LIB)/$(LLIBNAME).so

clean:
	rm -rf $(LOC_LIB_DIR)
	rm -rf $(LOC_OBJ_DIR)
	rm -rf $(LOC_BUILD_DIR)

doc:
	doxygen .doxygen
