SHELL := /bin/bash

PROJECT = tcp
BOXGEN = boxgen
GENPATH = smx_gen

OUTGML = $(GENPATH)/$(PROJECT).gml
OUTSRC = $(GENPATH)/$(PROJECT).c \
		 $(GENPATH)/$(BOXGEN).c
OUTINC = $(GENPATH)/$(BOXGEN).h

SOURCES = $(OUTSRC) \
		  boximpl.c

INCLUDES = $(OUTINC) \
		   boximpl.h

INCLUDES_DIR = -I/usr/local/ \
			   -I$(GENPATH) \
			   -I.

LINK_DIR = -L/usr/local/lib

LINK_FILE = -lsmxrts \
			-lpthread \
			-lzlog


CC = gcc
CFLAGS = -Wall
DEBUG_FLAGS = -g -O0

all: $(PROJECT).out

# compile with dot stuff and debug flags
debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(PROJECT)

$(PROJECT).out: $(SOURCES) $(INCLUDES)
	$(CC) $(CFLAGS) $(SOURCES) $(INCLUDES_DIR) $(LINK_DIR) $(LINK_FILE) -o $@

$(OUTSRC) $(OUTINC): $(OUTGML)
	gml2c $^

$(OUTGML): $(PROJECT).smx
	smxc $^ > $@

clean:
	rm -f $(OUTGML)
	rm -f $(OUTSRC)
	rm -f $(OUTINC)
	rm -f $(PROJECT).out
