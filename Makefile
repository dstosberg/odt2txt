
ifdef RELEASE
CFLAGS = -O2 -DQUIET
else
CFLAGS = -O0 -g -Wall
endif

KUNZIP_OBJS = kunzip/fileio.o kunzip/zipfile.o kunzip/kinflate.o

BIN = odt2txt
OBJ = odt2txt.o stringops.o $(KUNZIP_OBJS)

$(BIN): $(OBJ)

stringtest: stringtest.o stringops.o

$(OBJ): Makefile

all: $(BIN)

clean:
	rm -fr $(OBJ) $(BIN)

.PHONY: clean
