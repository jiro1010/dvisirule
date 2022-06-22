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

# Parses '.sirule' file which is generated at latex compile-time, and generates
# the page-numbered DVI filenames.
# The sequential pages which don't contain "sitabular" are concatenated and such
# filename will be "PG-PG.dvi". Otherwise the filename will be "PG.dvi".

function pr(begin, end)
{
	if (!begin)
		begin = 1;
	if (begin == end)
		print begin ".dvi";
	else
		print begin "-" end ".dvi";
}
BEGIN {
	lastpg = 0;
}
{
	pg = $0;
	if (lastpg+1 < pg)
		pr(lastpg+1, pg-1);
	print pg ".sidvi";
	lastpg = pg;
}
END {
	if (lastpg+1 < npages)
		pr(lastpg+1, npages);
}
