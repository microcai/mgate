#!/bin/sh
set -x
autopoint
gtkdocize || die 1
aclocal -I m4
autoheader
automake --foreign --add-missing --copy
autoconf
