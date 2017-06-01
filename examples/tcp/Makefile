SHELL := /bin/bash

PROJECT = tcp
BOXGEN = boxgen
GENPATH = smx_gen
FORMAT = graphml

OUTGRAPH = $(GENPATH)/$(PROJECT).$(FORMAT)
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

$(OUTSRC) $(OUTINC): $(OUTGRAPH)
	graph2c $^ -p $(GENPATH)/ -f $(FORMAT)

$(OUTGRAPH): $(PROJECT).smx
	mkdir -p $(GENPATH)
	smxc -f $(FORMAT) -o $(OUTGRAPH) $^

clean:
	rm -f $(OUTGRAPH)
	rm -f $(OUTSRC)
	rm -f $(OUTINC)
	rm -f $(PROJECT).out
