PREFIX = /usr/local
BINDIR = ${PREFIX}/bin

X11PREFIX = /usr/X11R6

CFLAGS += -I${X11PREFIX}/include `pkg-config xft xrender xcomposite xdamage xfixes --cflags` -g -xc99=%all
LDFLAGS += -L${X11PREFIX}/lib -lX11 -lm `pkg-config xft xrender xcomposite xdamage xfixes --libs`

# Disable post-processing effects
# CFLAGS += -DNOEFFECTS

# Comment these out to disable Xinerama support
CFLAGS += -DXINERAMA
LDFLAGS += -lXext -lXinerama

# Uncomment this for Xinerama debugging
#CFLAGS += -DDEBUG

EXESUFFIX =

SOURCES = skippy.c wm.c dlist.c regex.c key.c mainwin.c clientwin.c layout.c focus.c config.c tooltip.c
HEADERS = skippy.h wm.h dlist.h regex.h key.h mainwin.h clientwin.h layout.h focus.h config.h tooltip.h

all: skippy${EXESUFFIX}

skippy${EXESUFFIX}: Makefile ${SOURCES} ${HEADERS}
	cc ${CFLAGS} -o skippy${EXESUFFIX} ${SOURCES} ${LDFLAGS}

clean:
	rm -f skippy${EXESUFFIX}

install:
	install -d ${DESTDIR}${BINDIR}
	install -m 755 skippy$(EXESUFFIX) ${DESTDIR}${BINDIR}/skippy${EXESUFFIX}
