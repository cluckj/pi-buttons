IDIR =../include
CC=gcc
CFLAGS=-I$(IDIR)

LIBS=-lrt -lpthread

_DEPS = pi-buttons.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

OBJ = pi-buttons.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

pi-buttons: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o *~ core $(INCDIR)/*~
