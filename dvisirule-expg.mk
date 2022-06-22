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

# Splits the given DVI into some pages according to the generated
# filenames which are the given make-targets.
# Those filenames have the absolute page-numbers.

%.sidvi: %.dvi
	"${DVITYPE}" -show-opcodes $< |\
	awk -f "${Marker}" |\
	"${SIRULE}" $< $@

%.dvi: range = $(basename $@)
%.dvi:
	"${DVISELECT}" -i "${Input}" -o $@ -s =${range}
