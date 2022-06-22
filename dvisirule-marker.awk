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

# Parses the output of DVITYPE command, searches sirule BOL/EOL marker,
# extracts SETRULE/PUTRULE instractions between the markers, and print them with
# the coordinates.
# One important thing is the offset in DVI-page. dvisirule-bin command which is
# executed after this AWK, copies those instructions using the offset.

BEGIN {
	if (FGdef == "")
		FGdef = "gray 0";
}
{
	gsub(/hh:*=[0-9]*/, "");
	gsub(/vv:*=[0-9]*/, "");
}

/beginning of page/ {
	print "bop", $0;
}

/^[0-9]*: *eop/ {
	print $0, h " " v;
}

function extract_hv(s)
{
	gsub(/:/, "", s);
	gsub(/=[-+0-9]*=/, "=", s);
	return s;
}

/[hv]:*=/ {
	n = split($0, a, /[ ,()]*/);
	for (i = 1; i <= n; i++) {
		if (a[i] ~ /^h/)
			h = extract_hv(a[i]);
		else if (a[i] ~ /^v/)
			v = extract_hv(a[i]);
	}
}

/sirule BOL/ {
	print "bol", $0;
	in_superimpose++;
}
in_superimpose && /sirule EOL/ {
	print "eol", $0;
	in_superimpose--;
}
in_superimpose && /(put|set)rule/ {
	# {239} means "special xxx1" instruction
	if (color[cur - 1] == "")
		color[cur - 1] = "0:{239}color push " FGdef;
	# there are several variants of the color model, but we don't care.
	# handle any color model as a single string.
	gsub(/ /, "_", color[cur - 1]);
	print h " " v " " color[cur - 1] " " $0;
}

/color (push|pop)/ {
	gsub(/ xxx \x27 */, "");
	gsub(/\x27 */, "");
	if ($0 ~ /color push/)
		color[cur++] = $0;
	else
		cur--;
}

/pdf:[be]color/ {
	gsub(/ xxx \x27 */, "");
	gsub(/\x27 */, "");
	if ($0 ~ /bcolor/)
		color[cur++] = $0;
	else
		cur--;
}
