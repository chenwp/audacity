# Make PortAudio for Linux

LIBS =  -lm -lpthread

CDEFINES = -I../pa_common
CFLAGS = -g
PASRC = ../pa_common/pa_lib.c pa_unix_oss.c
PAINC = ../pa_common/portaudio.h

# Tests that work.
TESTC   = $(PASRC) ../pa_tests/patest_sine.c
#TESTC   = $(PASRC) ../pa_tests/patest_sine_time.c
#TESTC   = $(PASRC) ../pa_tests/patest_maxsines.c
#TESTC   = $(PASRC) ../pa_tests/patest_stop.c
#TESTC   = $(PASRC) ../pa_tests/patest_sync.c
#TESTC   = $(PASRC) ../pa_tests/patest_pink.c
#TESTC   = $(PASRC) ../pa_tests/patest_leftright.c
#TESTC   = $(PASRC) ../pa_tests/patest_clip.c
#TESTC   = $(PASRC) ../pa_tests/patest_dither.c
#TESTC   = $(PASRC) ../pa_tests/pa_devs.c
#TESTC   = $(PASRC) ../pa_tests/patest_many.c
#TESTC   = $(PASRC) ../pa_tests/patest_record.c
#TESTC   = $(PASRC) ../pa_tests/pa_fuzz.c
#TESTC   = $(PASRC) ../pa_tests/patest_wire.c
#TESTC   = $(PASRC) ../pa_tests/paqa_devs.c

# Tests that do not yet work.

TESTH   = $(PAINC)

all: patest

patest: $(TESTC) $(TESTH) Makefile
	gcc $(CFLAGS) $(TESTC) $(CDEFINES) $(LIBS) -o patest

run: patest
	./patest

