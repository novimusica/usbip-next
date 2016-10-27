#!/bin/sh

if [ -r Makefile ]; then
	make distclean
fi

FILES="aclocal.m4 autom4te.cache compile config.guess config.h.in config.log \
	config.status config.sub configure cscope.out depcomp install-sh      \
	src/Makefile src/Makefile.in libtool ltmain.sh Makefile         \
	Makefile.in missing"

rm -vRf $FILES
