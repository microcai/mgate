#!/bin/sh
set -x
autopoint
mkdir -p m4
aclocal -I m4
autoheader
automake --foreign --add-missing --copy
autoconf
