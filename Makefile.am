## Process this file with automake to produce Makefile.in
# Copyright 2010  Jochen Voss

AM_CPPFLAGS = $(GTK_CFLAGS)

dist_man_MANS = jvqplot.1

bin_PROGRAMS = jvqplot
jvqplot_SOURCES = data.c layout.c draw.c jvqplot.c jvqplot.h
jvqplot_LDADD = $(GTK_LIBS)

noinst_PROGRAMS = dump-png
dump_png_SOURCES = data.c layout.c draw.c dump-png.c jvqplot.h
dump_png_LDADD = $(GTK_LIBS)

EXTRA_DIST = examples/circle.dat examples/wiggles.dat
