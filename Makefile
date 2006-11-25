
SYSTEM := $(shell uname -s)

ifdef RELEASE
CFLAGS = -O2
else
CFLAGS = -O0 -g -Wall -DMEMDEBUG
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
OBJ = odt2txt.o stringops.o mem.o $(KUNZIP_OBJS)
BIN = odt2txt

$(BIN): $(OBJ)
	$(CC) -o $@ $(LDFLAGS) $(LIB) $(OBJ)

stringtest: stringtest.o stringops.o mem.o

$(OBJ): Makefile

all: $(BIN)

clean:
	rm -fr $(OBJ) $(BIN)

.PHONY: clean
