VERSION = 0.2
SOVERSION = 0
SONAME = libacpi.so.${SOVERSION}

# customize below to fit your system
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

# flags
SOFLAGS = -shared -Wl,-soname,${SONAME}
CFLAGS += -fPIC -g --pedantic -Wall -Wextra

# Compiler and linker
CC = cc
LD = ${CC}
AR = ar cr
RANLIB = ranlib
