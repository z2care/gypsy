#! /bin/sh

test -d m4 || mkdir -p m4

gtkdocize || exit 1

# back in the stupidity of autoreconf
autoreconf -v --install || exit 1

./configure "$@"

