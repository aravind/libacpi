# libacpi - general purpose acpi library
# (C)opyright 2007 Nico Golde <nico@ngolde.de>

include config.mk

SRC = libacpi.c list.c
SRC_test = test-libacpi.c libacpi.c list.c
OBJ = ${SRC:.c=.o}
OBJ_test = ${SRC_test:.c=.o}

all: options libacpi.a libacpi.so test-libacpi

options:
	@echo libacpi build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "CC       = ${CC}"
	@echo "SOFLAGS  = ${SOFLAGS}"
	@echo "LD       = ${LD}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.mk libacpi.h

libacpi.a: ${OBJ}
	@echo AR $@
	@${AR} $@ ${OBJ}
	@${RANLIB} $@

libacpi.so: ${OBJ}
	@echo LD $@
	@${LD} ${SOFLAGS} -o $@.${SOVERSION} ${OBJ}

test-libacpi: ${OBJ_test}
	@echo LD $@
	@${LD} -o $@ ${OBJ_test} ${LDFLAGS}
	@strip $@

install: all
	@echo installing header to ${DESTDIR}${PREFIX}/include
	@mkdir -p ${DESTDIR}${PREFIX}/include
	@cp -f libacpi.h ${DESTDIR}${PREFIX}/include
	@chmod 644 ${DESTDIR}${PREFIX}/include/libacpi.h
	@echo installing library to ${DESTDIR}${PREFIX}/lib
	@mkdir -p ${DESTDIR}${PREFIX}/lib
	@cp -f libacpi.a ${DESTDIR}${PREFIX}/lib
	@chmod 644 ${DESTDIR}${PREFIX}/lib/libacpi.a
	@cp -f ${SONAME} ${DESTDIR}${PREFIX}/lib/
	@chmod 644 ${DESTDIR}${PREFIX}/lib/${SONAME}
	@ln -s ${SONAME} ${DESTDIR}${PREFIX}/lib/libacpi.so
	@echo installing test-libacpi to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f test-libacpi ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/test-libacpi
	@echo installing manual page to ${DESTDIR}${MANPREFIX}/man3
	@mkdir -p ${DESTDIR}${MANPREFIX}/man3
	@cp libacpi.3 ${DESTDIR}${MANPREFIX}/man3
	@echo installing documentation and misc files to ${DESTDIR}${PREFIX}/share/doc/libacpi
	@mkdir -p ${DESTDIR}${PREFIX}/share/doc/libacpi
	@cp -r AUTHORS CHANGES README LICENSE doc ${DESTDIR}${PREFIX}/share/doc/libacpi
	@echo finished installation

uninstall:
	@echo removing header file from ${DESTDIR}${PREFIX}/include
	@rm -f ${DESTDIR}${PREFIX}/include/libacpi.h
	@echo removing library file from ${DESTDIR}${PREFIX}/lib
	@rm -f ${DESTDIR}${PREFIX}/lib/libacpi.a
	@echo removing shared object file from ${DESTDIR}${PREFIX}/lib
	@rm -f ${DESTDIR}${PREFIX}/lib/libacpi.so
	@rm -f ${DESTDIR}${PREFIX}/lib/${SONAME}
	@echo removing test-libacpi client from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/test-libacpi
	@echo removing manual page from ${DESTDIR}${MANPREFIX}/man3
	@rm -f ${DESTDIR}${MANPREFIX}/man3/libacpi.3
	@echo removing documentation and misc files from ${DESTDIR}${PREFIX}/share/doc/libacpi
	@rm -rf ${DESTDIR}${PREFIX}/share/doc/libacpi
	@echo uninstalled everything

dist:
	@echo creating dist tarball
	@mkdir -p libacpi-${VERSION}
	@cp libacpi.3 TODO AUTHORS CHANGES config.mk Makefile *.c *.h README LICENSE Doxyfile libacpi-${VERSION}
	@(cd libacpi-${VERSION}; doxygen)
	@rm -f libacpi-${VERSION}/Doxyfile
	@tar -cf libacpi-${VERSION}.tar libacpi-${VERSION}
	@gzip libacpi-${VERSION}.tar
	@rm -rf libacpi-${VERSION}

clean:
	@echo cleaning
	@rm -f libacpi.a libacpi.so* test-libacpi ${OBJ_test} libacpi-${VERSION}.tar.gz

.PHONY: all options clean dist install uninstall
