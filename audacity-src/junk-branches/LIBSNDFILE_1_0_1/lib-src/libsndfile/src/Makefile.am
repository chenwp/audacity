# This file was automatically generated from the Makefile.am
# DO NOT EDIT!

SUBDIRS = GSM610 G72x

lib_LTLIBRARIES = libsndfile.la
include_HEADERS = sndfile.h

EXTRA_DIST = sndfile.h.in config.h.in

noinst_HEADERS = common.h sfendian.h float_cast.h wav_w64.h au.h

COMMON = sndfile.c common.c file_io.c command.c pcm.c ulaw.c alaw.c float32.c \
		double64.c ima_adpcm.c ms_adpcm.c gsm610.c dwvw.c sfendian.c interleave.c

FILESPECIFIC = aiff.c au.c au_g72x.c nist.c paf.c raw.c svx.c \
				wav.c wav_w64.c voc.c ircam.c w64.c mat4.c mat5.c \
				sd2.c rx2.c txw.c sds.c wve.c dwd.c
				
libsndfile_la_SOURCES = $(COMMON) $(FILESPECIFIC) $(noinst_HEADERS)
libsndfile_la_LDFLAGS = -version-info @SHARED_VERSION_INFO@
libsndfile_la_LIBADD  = GSM610/libgsm.la G72x/libg72x.la

# Disable autoheader. 
AUTOHEADER=echo
