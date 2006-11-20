
SYSTEM := $(shell uname -s)

ifdef RELEASE
CFLAGS = -O2 -DQUIET
else
CFLAGS = -O0 -g -Wall -DMEMDEBUG
endif

ifeq ($(SYSTEM),FreeBSD)
	CFLAGS += -DHAVE_STRLCPY -I/usr/local/include
	LDFLAGS += -L/usr/local/lib
	LIB += -liconv
endif
ifeq ($(SYSTEM),OpenBSD)
	CFLAGS += -DHAVE_STRLCPY -I/usr/local/include
	LDFLAGS += -L/usr/local/lib
	LIB += -liconv
endif

KUNZIP_OBJS = kunzip/fileio.o kunzip/zipfile.o kunzip/kinflate.o

BIN = odt2txt
OBJ = odt2txt.o stringops.o mem.o $(KUNZIP_OBJS)

$(BIN): $(OBJ)
	$(CC) -o $@ $(LDFLAGS) $(LIB) $(OBJ)

stringtest: stringtest.o stringops.o

$(OBJ): Makefile

all: $(BIN)

clean:
	rm -fr $(OBJ) $(BIN)

.PHONY: clean
