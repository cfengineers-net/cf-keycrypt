CC=gcc
# Solaris
# CFLAGS= -g -Wall -I/usr/local/ssl/include
# For Linux cf-ppcrypt compilation: export LD_RUN_PATH=/var/cfengine/lib && make
# Solaris:
# LDFLAGS= -R/var/cfengine/lib -L/var/cfengine/lib
LDFLAGS= -L/var/cfengine/lib
LDLIBS=-Wl,-rpath,/var/cfengine/lib -lcrypto 
EXAMPLES=cf-keycrypt

all: $(EXAMPLES) 

clean:	
	rm -f $(EXAMPLES) 

