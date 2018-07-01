IDIR = ../include
CC = gcc
CFLAGS = -I$(IDIR)
LIBS = -lrt -lpthread

DESTDIR =
ETCDIR = $(DESTDIR)/etc
BINDIR = $(DESTDIR)/usr/bin
SCRIPTS=../scripts
SYSDDIR = $(DESTDIR)/etc/systemd/system

MAKEUSERID = $(shell id -u)

_DEPS = pi-buttons.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

OBJ = pi-buttons.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

pi-buttons: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -f pi-buttons *.o *~ core $(INCDIR)/*~

.PHONY: install
install: pi-buttons
	mkdir -p $(BINDIR)
	cp $< $(BINDIR)/pi-buttons

.PHONY: uninstall
uninstall:
	rm -f $(BINDIR)/pi-buttons

.PHONY: install_service
install_service:
	mkdir -p $(ETCDIR)
	cp $(SCRIPTS)/pi-buttons.conf $(ETCDIR)/pi-buttons.conf
	mkdir -p $(SYSDDIR)
	cp $(SCRIPTS)/pi-buttons.service $(SYSDDIR)/pi-buttons.service
	mkdir -p $(BINDIR)
	cp $(SCRIPTS)/pi-buttons.sh $(BINDIR)/pi-buttons.sh
ifeq ($(MAKEUSERID),0)
	systemctl daemon-reload
endif

.PHONY: uninstall_service
uninstall_service:
ifeq ($(MAKEUSERID),0)
	systemctl stop pi-buttons.service
	systemctl disable pi-buttons.service
endif
	mv -f $(ETCDIR)/pi-buttons.conf $(ETCDIR)/pi-buttons.saved
	rm $(SYSDDIR)/pi-buttons.service
	rm $(BINDIR)/pi-buttons.sh
ifeq ($(MAKEUSERID),0)
	systemctl daemon-reload
endif
