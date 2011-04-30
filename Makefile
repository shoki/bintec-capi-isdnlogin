# Makefile - BinTec Remote CAPI Library
#

SHELL	= /bin/sh

# be sure to get a BINTEC_PUBLIC_RELEASE_1_7 tagged capi, because the HEAD
# release is still buggy.
CAPIDIR = ../bintec_capi

CC	= gcc
LD	= ld
DEFS	= -I. -I/usr/local/include -I$(CAPIDIR)
COPTS	= -O -Wall 
CWARN	= 
CFLAGS	= $(COPTS) $(CWARN) $(DEFS)
LIBS	= $(CAPIDIR)/libcapi.a 
LORDER  = sh ./lorder
TSORT	= tsort
LN	= ln
AR	= ar rv
INSTALL	= install
RANLIB	= ranlib
OSTYPE  = `uname -srm | tr " " _`

.SUFFIXES:
.SUFFIXES: .out .o .po .So .s .S .c .cc .cpp .cxx .m .C .f .y .l

.c.o:	isdnlogin.h Makefile
	${CC} ${CFLAGS} -c $< -o $@

prefix	= /usr/local
bindir	= $(prefix)/bin
libdir	= $(prefix)/lib
incdir	= $(prefix)/include

CAPIAPP		= isdnlogin
ISDNLOGIN	= isdnlogin.o il_capi.o il_tty.o il_call.o il_poll.o \
		  il_event.o il_proc.o xmodem.o

all:	$(CAPIAPP)

# dynamically linked version
isdnlogin:	Makefile $(ISDNLOGIN)
	$(CC) -o $@ $(CFLAGS) $(ISDNLOGIN) $(LIBS)

# the static version of isdnlogin app
isdnlogin.static:	Makefile $(ISDNLOGIN)
	$(CC) -o $@ $(CFLAGS) $(ISDNLOGIN) $(CAPIDIR)/libcapi.a

install:	all installdirs
	@for prog in $(CAPIAPP);			\
	do						\
	    echo $(INSTALL) $$prog $(bindir);		\
	    $(INSTALL) $$prog $(bindir) || exit 1;	\
	done

installdirs:
	@$(SHELL) mkinstalldirs $(prefix) $(bindir)

dist:	Makefile isdnlogin
	@echo "Generating distribution for OS $(OSTYPE)"
	@rm -f isdnlogin.$(OSTYPE)
	@rm -f isdnlogin.$(OSTYPE).gz
	@cp isdnlogin isdnlogin.$(OSTYPE)
	@strip isdnlogin.$(OSTYPE)
	@gzip -9 isdnlogin.$(OSTYPE)
	@echo "Your archive is named 'isdnlogin.$(OSTYPE).gz'."

clean:
	rm -f $(CAPIAPP)
	rm -f core *.o

distclean: clean
	rm -f $(CAPIAPP) 
