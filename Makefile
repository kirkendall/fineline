LIB=	lib
INCLUDE=include
CC=	gcc -g -fpic
CFLAGS=	-Wall -I$(INCLUDE)

all: libs tryfl fled

libs:
	cd src/fineline; make

fled:
	cd src/fled; make

tryfl: tryfl.c libs
	$(CC) $(CFLAGS) tryfl.c -L$(LIB) -lfineline -o tryfl

try: tryfl
	clear
	LD_LIBRARY_PATH=$(LIB) ./tryfl || (stty sane; [ -f core ] && LD_LIBRARY_PATH=$(LIB) gdb tryfl core)

gdb: tryfl
	LD_LIBRARY_PATH=$(LIB) gdb tryfl

tags: src/*/*.[ch] $(INCLUDE)/fineline.h
	elvtags $(SRC) fineline.h

clean:
	$(RM) tryfl
	$(RM) tags
	cd src/fineline; make clean
	cd src/fled; make clean
