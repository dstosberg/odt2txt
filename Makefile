
UNAME_S := $(shell uname -s 2>/dev/null || echo unknown)
UNAME_O := $(shell uname -o 2>/dev/null || echo unknown)

ifdef DEBUG
CFLAGS = -O0 -g -Wextra -DMEMDEBUG -DSTRBUF_CHECK
#LDFLAGS = -lefence
LDFLAGS += -g
else
CFLAGS = -O2
endif

KUNZIP_OBJS = kunzip/fileio.o kunzip/zipfile.o
OBJ = odt2txt.o regex.o mem.o strbuf.o $(KUNZIP_OBJS)
TEST_OBJ = t/test-strbuf.o t/test-regex.o
LIBS = -lz
ALL_OBJ = $(OBJ) $(TEST_OBJ)

INSTALL = install
GROFF   = groff

DESTDIR = /usr/local
PREFIX  =
BINDIR  = $(PREFIX)/bin
MANDIR  = $(PREFIX)/share/man
MAN1DIR = $(MANDIR)/man1

ifeq ($(UNAME_S),FreeBSD)
	CFLAGS += -DICONV_CHAR="const char" -I/usr/local/include
	LDFLAGS += -L/usr/local/lib
	LIBS += -liconv
endif
ifeq ($(UNAME_S),OpenBSD)
	CFLAGS += -DICONV_CHAR="const char" -I/usr/local/include
	LDFLAGS += -L/usr/local/lib
	LIBS += -liconv
endif
ifeq ($(UNAME_S),NetBSD)
	CFLAGS += -DICONV_CHAR="const char"
endif
ifeq ($(UNAME_S),SunOS)
	ifeq ($(CC),cc)
		ifdef DEBUG
			CFLAGS = -v -g -DMEMDEBUG -DSTRBUF_CHECK
		else
			CFLAGS = -xO3
		endif
	endif
	CFLAGS += -DICONV_CHAR="const char"
endif
ifeq ($(UNAME_O),Cygwin)
	CFLAGS += -DICONV_CHAR="const char"
	LIBS += -liconv
	EXT = .exe
endif
ifneq ($(MINGW32),)
	CFLAGS += -DICONV_CHAR="const char" -I$(REGEX_DIR) -I$(ZLIB_DIR)
	LIBS = $(REGEX_DIR)/regex.o
	ifdef STATIC
		LIBS += $(wildcard $(ICONV_DIR)/lib/.libs/*.o)
		LIBS += $(ZLIB_DIR)/zlib.a
	else
		LIBS += -liconv
	endif
	EXT = .exe
endif

BIN = odt2txt$(EXT)
MAN = odt2txt.1

$(BIN): $(OBJ)
	$(CC) -o $@ $(LDFLAGS) $(OBJ) $(LIBS)

t/test-strbuf: t/test-strbuf.o strbuf.o mem.o
t/test-regex: t/test-regex.o regex.o strbuf.o mem.o

$(ALL_OBJ): Makefile

all: $(BIN)

install: $(BIN) $(MAN)
	$(INSTALL) -d -m755 $(DESTDIR)$(BINDIR)
	$(INSTALL) $(BIN) $(DESTDIR)$(BINDIR)
	$(INSTALL) -d -m755 $(DESTDIR)$(MAN1DIR)
	$(INSTALL) $(MAN) $(DESTDIR)$(MAN1DIR)

odt2txt.html: $(MAN)
	$(GROFF) -Thtml -man $(MAN) > $@

odt2txt.ps: $(MAN)
	$(GROFF) -Tps -man $(MAN) > $@

clean:
	rm -fr $(OBJ) $(BIN) odt2txt.ps odt2txt.html

.PHONY: clean

