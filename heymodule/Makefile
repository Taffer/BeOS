######################################################################
# Makefile for heymodule
#
# Copyright © 1998 Chris Herborth (chrish@qnx.com)
#                  Arcane Dragon Software
#
# $Id: Makefile,v 1.1.1.1 1999/06/08 12:49:37 chrish Exp $

PY_PATH:=/boot/home/Python-1.5.2
LINKMODULE:=$(PY_PATH)/BeOS/linkmodule
PY_VERSION:=1.5
HEYMODULE_VERSION:=1.1

LDSHARED:=$(LINKMODULE) -L/boot/home/config/lib -lpython$(PY_VERSION)

# Some stuff from the Python Makefiles...
CONFIGINCLDIR:=/boot/home/config/include/python$(PY_VERSION)
INCLDIR:=.
DEFS:=-DHAVE_CONFIG_H

# Destinations:
PYMODULES:=/boot/home/config/lib/python$(PY_VERSION)/BeOS
APPDIR:=/boot/apps/heymodule-$(HEYMODULE_VERSION)

# Things that differ by architecture.
ifeq "$(BE_HOST_CPU)" "ppc"
CC:=mwcc
OPT:=-w9 -O7 -opt schedule604
CFLAGS:=$(OPT) -I$(INCLDIR) -I$(CONFIGINCLDIR) $(DEFS)
else
CC:=gcc
OPT:=-Wall -Wno-multichar -Wno-ctor-dtor-privacy -O3
CFLAGS:=$(OPT) -I$(INCLDIR) -I$(CONFIGINCLDIR) $(DEFS)
endif

PARTS:=Hey.cpp Specifier.cpp heymodule.cpp

OBJS:=Hey.o Specifier.o heymodule.o

######################################################################
# Targets
all: heymodule.so

heymodule.so: $(OBJS)
	$(LDSHARED) $(OBJS) -o heymodule.so -lbe

heymodule.o: heymodule.cpp
	$(CC) $(CFLAGS) -c heymodule.cpp -o heymodule.o

Specifier.o: Specifier.cpp Specifier.h
	$(CC) $(CFLAGS) -c Specifier.cpp -o Specifier.o

Hey.o: Hey.cpp Hey.h Specifier.h
	$(CC) $(CFLAGS) -c Hey.cpp -o Hey.o

clean:
	-rm -f *~

spotless: clean
	-rm -f *.o

install: heymodule.so
	if [ ! -d $(PYMODULES) ] ; then \
		mkdir -p $(PYMODULES) ; \
	fi
	install -m 755 heymodule.so $(PYMODULES)
	mimeset -f -all $(PYMODULES)/heymodule.so
	if [ ! -d $(APPDIR) ] ; then \
		mkdir -p $(APPDIR) ; \
	fi
	install -m 444 heymodule.html $(APPDIR)
	settype -t text/html $(APPDIR)/heymodule.html
