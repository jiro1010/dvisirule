#!/bin/sh

set -eu

cd $1
autoscan
diff -u configure.scan configure.ac || :

autoreconf -if
./configure -q
make -st
make -s clean
