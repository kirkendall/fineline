LIB=	../../lib
INCLUDE=../../include
SRC=	char.c draw.c edit.c .c fineline.c history.c image.c paste.c tty.c
OBJ=	char.o draw.o edit.o fineline.o history.o image.o paste.o tty.o
CC=	gcc -g -fpic
CFLAGS=	-Wall -I$(INCLUDE)

all: libs

libs: $(LIB)/libfineline.so $(LIB)/libfineline.a

$(LIB)/libfineline.so: $(OBJ)
	$(CC) -shared $(OBJ) $(LIBS) -o $(LIB)/libfineline.so

$(LIB)/libfineline.a: $(OBJ)
	$(RM) $(LIB)/libfineline.a
	ar q $(LIB)/libfineline.a $(OBJ)

clean:
	$(RM) $(OBJ)
	$(RM) $(LIB)/libfineline.so $(LIB)/libfineline.a
	$(RM) tags
