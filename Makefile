
CFLAGS = -DZIP

ifdef RELEASE
CFLAGS += -O2 -DQUIET
else
CFLAGS += -O0 -g -Wall
endif

BIN = odt2txt
OBJ = odt2txt.o kunzip/fileio.o kunzip/zipfile.o kunzip/kinflate.o

$(BIN): $(OBJ)

$(OBJ): Makefile

all: $(BIN)

clean:
	rm -fr $(OBJ) $(BIN)

.PHONY: clean