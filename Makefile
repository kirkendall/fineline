LIB=	lib
INCLUDE=include
CC=	gcc -g -fpic
CFLAGS=	-Wall -I$(INCLUDE)

all: libs tryfl fled

libs:
	cd src/fineline; make

fled:
	cd src/fled; make

tryfl: tryfl.c $(INCLUDE)/fineline.h $(LIB)/libfineline.so
	$(CC) $(CFLAGS) tryfl.c -L$(LIB) -lfineline -o tryfl

try: tryfl
	clear
	LD_LIBRARY_PATH=$(LIB) ./tryfl || (stty sane; [ -f core ] && LD_LIBRARY_PATH=$(LIB) gdb tryfl core)

tryed: fled
	LD_LIBRARY_PATH=$(LIB) bin/fled +55 testfile || (stty sane; [ -f core ] && LD_LIBRARY_PATH=$(LIB) gdb bin/fled core)

gdb: tryfl
	LD_LIBRARY_PATH=$(LIB) gdb tryfl

tags: src/*/*.[ch] $(INCLUDE)/fineline.h
	elvtags src/*/*.[ch] $(INCLUDE)/fineline.h

clean:
	$(RM) tryfl
	$(RM) tags
	cd src/fineline; make clean
	cd src/fled; make clean

wc:
	@echo $$(find . -name '*.[ch]' -exec cat {} \;|wc -l) lines of C code
	@echo $$(find . -name '*.[13]' -exec cat {} \;|wc -l) lines of documentation
