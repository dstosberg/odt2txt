
#DEBUG=-DDEBUG
FLAGS=-s -O2 -Wall $(DEBUG)
CC=gcc

#PATH=/opt/xmingw/bin:/usr/bin
#CC=i386-mingw32msvc-gcc
#DLLWRAP=i386-mingw32msvc-dllwrap
#WINDRES=i386-mingw32msvc-windres

default:
	$(CC) -c fileio.c $(FLAGS)
	$(CC) -c kinflate.c $(FLAGS) -DZIP
	$(CC) -c zipfile.c $(FLAGS)
	$(CC) -o kunzip kunzip.c fileio.o kinflate.o zipfile.o $(FLAGS)

dll:
	$(CC) -c fileio.c $(FLAGS)
	$(CC) -c kinflate.c $(FLAGS) -DZIP
	$(CC) -c zipfile.c $(FLAGS) -DQUIET -DDLL
	$(WINDRES) -i kunzip.rc -o kunzipres.o
	$(DLLWRAP) --def kunzip.def --dllname kunzip -o kunzip.dll \
	        fileio.o kinflate.o kunzipres.o zipfile.o $(FLAGS)

lib:
	$(CC) -c fileio.c $(FLAGS)
	$(CC) -c kinflate.c $(FLAGS) -DZIP
	$(CC) -c zipfile.c $(FLAGS)
	$(CC) -o libkunzip.so -shared fileio.o kinflate.o zipfile.o $(FLAGS)

clean:
	@if [ -e fileio.o ]; then rm fileio.o; fi
	@if [ -e kinflate.o ]; then rm kinflate.o; fi
	@if [ -e zipfile.o ]; then rm zipfile.o; fi
	@if [ -e kunzipres.o ]; then rm kunzipres.o; fi
	@if [ -e kunzip.exe ]; then rm kunzip.exe; fi
	@if [ -e libkunzip.so ]; then rm libkunzip.so; fi
	@if [ -e kunzip.dll ]; then rm kunzip.dll; fi
	@if [ -e kunzip ]; then rm kunzip; fi
	@echo "Clean!"



