#
# makefile for flash_policy server
#

TARGET	= flash_policyd

all: $(TARGET)

# Which compiler
CC = gcc

# Where are include files kept
INCLUDE =

# Use this for Linux client software
CDEFINE = -DLINUX -D_GNU_SOURCE


# Options for development
CFLAGS		= $(INCLUDE) $(CDEFINE) -g -Wall -Os 
LDFLAGS		= 
LINKFLAGS	= -o 

LIBS            = 
LIBS_INC        = 

# Options for release
# CFLAGS = -O -Wall -ansi

#Dependancies for all
DEPENDALL	= arch.h config.h mytypes.h daemonize.h debug.h flash_policy.h yselect.h

#Objects
OBJ	 = flash_policy.o daemonize.o yselect.o arch.o file_config.o

$(TARGET): $(OBJ) 
	$(CC) $(CFLAGS) $(LINKFLAGS) $(TARGET) $(OBJ) $(LIBS)
	cp $(TARGET) ../$(TARGET)

clean:
	-rm *.o $(MYLIB)
	 rm -f *.map
	 rm -f $(TARGET)
	 rm -f ../$(TARGET)


