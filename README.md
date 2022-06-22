
---
```
Copyright 2012-2022 Jiro Senju

This package is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
any later version.

This package is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this package.  If not, see <http://www.gnu.org/licenses/>.
```
---

# dvisirule

Superimpose the covered / hidden or covered `\hline` and `\vline` in a
LaTeX tabular / colortbl environment.

Here *hidden* or *covered* means the unintentionally overwritten
`\hline`s and `\vline`s in a tabular.
They are surely drawn but other things such as a row background color
painted after hide them unintentionally.
See the example in `dvisirule.pdf`.

This package provides `dvisirule.sty` and `dvisirule` command.
The command is a `sh` script, and it internally uses `dvisirule-bin`,
`dvisirule-pgnum.awk`, `dvisirule-expg.mk`, and `dvisirule-marker.awk`
files.


## Compile & Install

```
$ cd $this_dir/src
$ configure -q
$ cd ..
$ make -s Dir=/tmp
```

Then you will get these files.

```
/tmp/dvisirule
/tmp/dvisirule-bin
/tmp/dvisirule-expg.mk
/tmp/dvisirule-marker.awk
/tmp/dvisirule-pgnum.awk
/tmp/dvisirule.sty
```

Now you can install them by

```
$ make InstallBase=/tmp/texmf-dist
```

Here `${InstallBase}` is a make-variable which is referred by other make-variables.

```
InstallBin ?= ${InstallBase}/bin
InstallLib ?= ${InstallBase}/lib
InstallSty ?= ${InstallBase}/lib/texinputs
```

and the files will be installed to these dirs.

```
${InstallBin}/dvisirule

${InstallLib}/dvisirule-bin
${InstallLib}/dvisirule-expg.mk
${InstallLib}/dvisirule-marker.awk
${InstallLib}/dvisirule-pgnum.awk

${InstallSty}/dvisirule.sty
```

Please note that these installed dirs have to be recognized by
`Kpathsea` since the main `sh` script `dvisirule` searches these
sub-files by `kpsewhich(1)`.


## Usage

`sitabular` environment is a wrapper of `tabular`, and all the syntax
is kept. You can use it by simply replacing `tabular` by `sitabular`.

`a.tex`

> `\usepackage{dvisirule}`

> `\begin{sitabular} ... \end{sitabular}`

and then

```
$ latex a.tex
$ cp -p a.dvi a.dvi.save
$ dvisirule a.dvi a-si.dvi
$ mv a-si.dvi a.dvi
```

For more details, refer to `dvisirule.pdf` and other source files.
