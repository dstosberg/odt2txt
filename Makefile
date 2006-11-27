
UNAME_S := $(shell uname -s)
UNAME_O := $(shell uname -o)

ifdef RELEASE
CFLAGS = -O2
else
CFLAGS = -O0 -g -Wall -DMEMDEBUG -DSTRBUF_CHECK
#LDFLAGS = -lefence
endif

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
ifeq ($(UNAME_S),SunOS)
	ifeq ($(CC),cc)
		ifdef RELEASE
			CFLAGS = -xO3
		else
			CFLAGS = -v -g -DMEMDEBUG -DSTRBUF_CHECK
		endif
	endif
	CFLAGS += -DICONV_CHAR="const char"
endif
ifeq ($(UNAME_O),Cygwin)
	CFLAGS += -DICONV_CHAR="const char"
	LIBS += -liconv
	EXT = .exe
endif

KUNZIP_OBJS = kunzip/fileio.o kunzip/zipfile.o kunzip/kinflate.o
OBJ = odt2txt.o regex.o mem.o strbuf.o $(KUNZIP_OBJS)
TEST_OBJ = t/test-strbuf.o t/test-regex.o
ALL_OBJ = $(OBJ) $(TEST_OBJ)
BIN = odt2txt$(EXT)

$(BIN): $(OBJ)
	$(CC) -o $@ $(LDFLAGS) $(OBJ) $(LIBS)

t/test-strbuf: t/test-strbuf.o strbuf.o mem.o
t/test-regex: t/test-regex.o regex.o strbuf.o mem.o

$(ALL_OBJ): Makefile

all: $(BIN)

clean:
	rm -fr $(OBJ) $(BIN)

.PHONY: clean
