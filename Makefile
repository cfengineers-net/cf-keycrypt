# Make with:
# 
# make CFENGINE_SOURCE=/path/to/cfengine/source CFENGINE_INSTALLDIR=/path/to/cfengine/installdir
#
CC=gcc

CFENGINE_SOURCE=/usr/local/src
CFENGINE_PREFIX=/usr/local
CFLAGS=-I$(CFENGINE_PREFIX)/include -I$(CFENGINE_SOURCE) -I$(CFENGINE_SOURCE)/libutils -I$(CFENGINE_SOURCE)/libpromises -I$(CFENGINE_SOURCE)/libcfnet
LDFLAGS=-L$(CFENGINE_PREFIX)/lib
LDLIBS=-Wl,-rpath,$(CFENGINE_PREFIX)/lib -lpromises -lcrypto

OBJECTS=cf-keycrypt

all: $(OBJECTS)
