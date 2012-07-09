VERS_MAJOR = 0
VERS_MINOR = 6
VERS_MICRO := $(shell hg tip --template '.{rev}\n')
VERSION = $(VERS_MAJOR).$(VERS_MINOR)$(VERS_MICRO)
DATE := $(shell /usr/bin/date "+%Y.%m.%d")

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
MANDIR = $(PREFIX)/man
SHAREDIR = $(PREFIX)/share

X11PREFIX = /usr/X11
PROG = xreveal
CC = gcc

CFLAGS += \
	-I$(X11PREFIX)/include \
	`pkg-config xft xrender xcomposite xdamage xfixes --cflags` \
	-g \
	-std=gnu99 -Wall

LDFLAGS += \
	-L$(X11PREFIX)/lib \
	-lX11 \
	-lm \
	`pkg-config xft xrender xcomposite xdamage xfixes --libs`

# Disable post-processing effects
# CFLAGS += -DNOEFFECTS

# Comment these out to disable Xinerama support
CFLAGS += -DXINERAMA
LDFLAGS += -lXext -lXinerama

# Uncomment this for Xinerama debugging
#CFLAGS += -DDEBUG

OBJS = \
	$(PROG).o \
	clientwin.o \
	config.o \
	dlist.o \
	focus.o \
	key.o \
	layout.o \
	mainwin.o \
	regex.o \
	tooltip.o \
	wm.o

MAN_PROG = $(PROG).1
MAN_RC = $(PROG)rc.5
RCSAMPLE = $(PROG)rc.sample

all: $(PROG)

$(PROG).c: $(PROG).c.in
	sed -e 's/\<REPLACEME_VERSION\>/$(VERSION)/g;' \
	    -e 's/\<REPLACEME_DATE\>/$(DATE)/g;' $< > $@

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

clean:
	rm -f $(PROG) $(OBJS) $(PROG).c

install: $(PROG) $(MAN_PROG) $(MAN_RC) $(RCSAMPLE)
	install -D -m 755 $(PROG) $(BINDIR)/$(PROG)
	install -D -m 644 $(MAN_PROG) $(MANDIR)/man1/$(MAN_PROG)
	install -D -m 644 $(MAN_RC) $(MANDIR)/man5/$(MAN_RC)
	install -D -m 644 $(RCSAMPLE) $(SHAREDIR)/$(RCSAMPLE)
