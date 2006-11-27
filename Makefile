
SYSTEM := $(shell uname -s)

ifdef RELEASE
CFLAGS = -O2
else
CFLAGS = -O0 -g -Wall -DMEMDEBUG -DSTRBUF_CHECK
endif

ifeq ($(SYSTEM),FreeBSD)
	CFLAGS += -DHAVE_STRLCPY -DICONV_CHAR="const char" -I/usr/local/include
	LDFLAGS += -L/usr/local/lib
	LIB += -liconv
endif
ifeq ($(SYSTEM),OpenBSD)
	CFLAGS += -DHAVE_STRLCPY -DICONV_CHAR="const char" -I/usr/local/include
	LDFLAGS += -L/usr/local/lib
	LIB += -liconv
endif
ifeq ($(SYSTEM),SunOS)
	ifeq ($(CC),cc)
		ifdef RELEASE
			CFLAGS = -xO3
		else
			CFLAGS = -DMEMDEBUG -v -g
		endif
	endif
	CFLAGS += -DHAVE_STRLCPY -DICONV_CHAR="const char"
endif

KUNZIP_OBJS = kunzip/fileio.o kunzip/zipfile.o kunzip/kinflate.o
OBJ = odt2txt.o regex.o mem.o strbuf.o $(KUNZIP_OBJS)
TEST_OBJ = t/test-strbuf.o
ALL_OBJ = $(OBJ) $(TEST_OBJ)
BIN = odt2txt

$(BIN): $(OBJ)
	$(CC) -o $@ $(LDFLAGS) $(LIB) $(OBJ)

stringtest: stringtest.o regex.o mem.o
t/test-strbuf: t/test-strbuf.o mem.o strbuf.o

$(ALL_OBJ): Makefile

all: $(BIN)

clean:
	rm -fr $(OBJ) $(BIN)

.PHONY: clean
