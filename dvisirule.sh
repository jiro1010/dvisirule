#!/bin/sh
#
# Copyright (C) 2012-2022 Jiro Senju
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# any later version.
#
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this package.  If not, see <http://www.gnu.org/licenses/>.
#

# Superimpose the hidden or covered `\hline` and `\vline` in a LaTeX
# tabular / colortbl environment.

set -eu

usage()
{
	cat <<- EOF
	$(basename $0) [option] in.dvi [out.dvi]
	option:
		-j jobs
		   number of jobs in parallel.
		   a job means handling the extracted page.
		   default: the number of processors online
		-t dvitype-cmd
		   'dvitype' command.
		   default: dvitype or \$DIVTYPE
		-c dviconcat-cmd
		   'dviconcat' command.
		   default: dviconcat or \$DIVCONCAT
		-s dviselect-cmd
		   'dviselect' command.
		   default: dviselect or \$DIVSELECT
		-l library-dir
		   library path where dvisirule-bin, \*.awk and \*.mk
		   are located.
		   default: a dir kpsewhich(1) returned.
			if you set "library-dir" by this option, then
			"kpsewhich -path \"library-dir\"" is used.
	Also this command creates a temporary directory under \$TMPDIR
	or /tmp. For details, refer to dvisirule.pdf.
	EOF
}

jobs=0
dvitype=${DVITYPE:-dvitype}
dviconcat=${DVICONCAT:-dviconcat}
dviselect=${DVISELECT:-dviselect}
libdir=""
# posix compliant builtin
while getopts j:t:c:s:l:h i
do
	case $i in
	j)
		{
		test "$2" -eq $(($2 + 0))
		test "$2" -ge 1
		} || {
		echo Bad arg "$2" 1>&2
		exit 1
		}
		jobs="$2"
		shift
		;;
	t)	dvitype="$2"
		shift
		;;
	c)	dviconcat="$2"
		shift
		;;
	l)	libdir="-path \"$2\""
		shift
		;;
	h)	usage
		exit 0
		;;
	*)	usage 1>&2
		exit 1
		;;
	esac
	shift
done
shift $(($OPTIND - 1))

find_lib() # file
{
	eval kpsewhich $libdir $1 ||
	kpsewhich $1
}

pgnum="$(find_lib dvisirule-pgnum.awk)"
expg="$(find_lib dvisirule-expg.mk)"
marker="$(find_lib dvisirule-marker.awk)"
sirule="$(find_lib dvisirule-bin)"

# find the input file
f=$(realpath "$1")
d=$(dirname "$f")
b=$(basename "$f" .dvi)
tables="$d/$b.sirule"
test -e "$tables" ||
{
	echo No "$tables" 1>&2
	exit 1
}
# no need to handle it
test -s "$tables" || exit 0

# decide the output file
out="$d/$b-si.dvi"
test $# -eq 2 &&
out=$(realpath -m "$2")
test $# -ge 3 &&
{
	echo Bad arg 1>&2
	exit 1
}

d="${TMPDIR:-/tmp}"
test -d "$d" ||
{
	echo "$d" is not a directory 1>&2
	exit 1
}
tmp="$d/$(basename $0).$$"
mkdir "$tmp"
cd "$tmp"

# parallel processing per page
test $jobs -eq 0 &&
jobs=$(getconf _NPROCESSORS_ONLN)
# set +eu
# if [ "$MAKEFLAGS" ]
# then
#	echo "$MAKEFLAGS" | fgrep -q -- -j
#	if [ $? -eq 1 ]
#	then
#		MAKEFLAGS="$MAKEFLAGS -j$jobs"
#	fi
# else
#	MAKEFLAGS="-j$jobs"
# fi
# export MAKEFLAGS
# set -eu

# which pages to handle?
npages=$("$dvitype" -max-pages=1 "$f" |
	fgrep totalpages= |
	sed -e 's/^.*totalpages=//')
uniq "$tables" |
awk -v npages=$npages -f "$pgnum" |
tee tables2 |
xargs -r make -s -j$jobs -f "$expg" \
	DVITYPE="$dvitype" \
	Input="$f" \
	Marker="$marker" \
	SIRULE="$sirule" \
	DVISELECT="$dviselect"

# concatenate all pages
err=0
"$dviconcat" -o "$out" $(cat tables2) 2> err ||
err=$?
test $err -ne 0 &&
grep -vx 'Wrote [0-9]* pages, [0-9]* bytes' err
#test $err -eq 0 # debugging

cd $OLDPWD
rm -fr "$tmp"

exit $err
