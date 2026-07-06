INCLUDE=../../include
LIB=	../../lib
BIN=	../../bin
CFLAGS=	-I$(INCLUDE)
LDFLAGS=-L$(LIB)

all: $(BIN)/fled

$(BIN)/fled: fled
	cp fled $(BIN)/

fled: fled.c
	$(CC) $(CFLAGS) $(LDFLAGS) fled.c -lfineline -ofled

clean:
	$(RM) fled

