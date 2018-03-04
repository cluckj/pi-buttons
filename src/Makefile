IDIR = ../include
CC = gcc
CFLAGS = -I$(IDIR)
LIBS = -lrt -lpthread

DESTDIR=
BINDIR = /usr/bin
SCRIPTS = ../scripts
SYSDDIR = /etc/systemd/system


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

.PHONY: install
install: pi-buttons
	mkdir -p $(DESTDIR)$(BINDIR)
	cp $< $(DESTDIR)$(BINDIR)/pi-buttons

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(BINDIR)/pi-buttons

.PHONY: install_service
install_service:
	cp $(SCRIPTS)/pi-buttons.conf /etc/pi-buttons.conf
	cp $(SCRIPTS)/pi-buttons.service $(SYSDDIR)/pi-buttons.service
	cp $(SCRIPTS)/pi-buttons.sh $(BINDIR)/pi-buttons.sh
	systemctl daemon-reload
