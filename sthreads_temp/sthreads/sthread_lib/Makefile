LIBRARIES = libsthread.a libsthread_start.a

OBJECTS = sthread.o sthread_user.o \
	sthread_ctx.o sthread_util.o sthread_time_slice.o \
	sthread_switch.o sthread_end.o queue.o
start_OBJECTS = sthread_start.o

DEFAULT_INCLUDES = -I. -I. -I../include
CCASCOMPILE = $(CCAS) $(CCASFLAGS)
COMPILE = $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CFLAGS)
AR = ar
CC = gcc
CCAS = gcc
CCASFLAGS = -g -O0 -Wall  -I ../include -D__ASM__
CFLAGS = -g -O0 -Wall 
ARFLAGS = cru
DEFS = -DHAVE_CONFIG_H
RANLIB = ranlib
INCLUDES = -I ../include

all: $(LIBRARIES)

libsthread.a: $(OBJECTS) 
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@
libsthread_start.a: $(start_OBJECTS)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

.S.o:
	$(CCASCOMPILE) -c $<

.c.o:
	$(COMPILE) -c -o $@ $<

clean: 
	rm -f *.o
	rm -f *.a

