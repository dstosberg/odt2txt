
SYSTEM := $(shell uname -s)

ifdef RELEASE
CFLAGS = -O2
else
CFLAGS = -O0 -g -Wall -DMEMDEBUG -DSTRBUF_CHECK
#LDFLAGS = -lefence
endif

ifeq ($(SYSTEM),FreeBSD)
	CFLAGS += -DICONV_CHAR="const char" -I/usr/local/include
	LDFLAGS += -L/usr/local/lib
	LIB += -liconv
endif
ifeq ($(SYSTEM),OpenBSD)
	CFLAGS += -DICONV_CHAR="const char" -I/usr/local/include
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
	CFLAGS += -DICONV_CHAR="const char"
endif

KUNZIP_OBJS = kunzip/fileio.o kunzip/zipfile.o kunzip/kinflate.o
OBJ = odt2txt.o regex.o mem.o strbuf.o $(KUNZIP_OBJS)
TEST_OBJ = t/test-strbuf.o t/test-regex.o
ALL_OBJ = $(OBJ) $(TEST_OBJ)
BIN = odt2txt

$(BIN): $(OBJ)
	$(CC) -o $@ $(LDFLAGS) $(LIB) $(OBJ)

t/test-strbuf: t/test-strbuf.o strbuf.o mem.o
t/test-regex: t/test-regex.o regex.o strbuf.o mem.o

$(ALL_OBJ): Makefile

all: $(BIN)

clean:
	rm -fr $(OBJ) $(BIN)

.PHONY: clean
