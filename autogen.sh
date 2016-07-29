#!/bin/sh

aclocal \
  && autoheader \
  && automake --add-missing --foreign --copy \
  && autoconf \
  && ./configure --enable-maintainer-mode $@
