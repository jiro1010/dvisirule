
Lo =
LATEX ?= latex

texver = $(shell ${LATEX} --version | head -1)
tl = $(findstring TeX Live, ${texver})
mik = $(findstring MiKTeX, ${texver})
ifeq (${tl},TeX Live)
#$(info texlive)
LATEX += -no-mktex tfm
else ifeq (${mik},MiKTeX)
#$(info miktex)
LATEX += -quiet
else
$(error Unknown latex system)
endif

define Latex # texsrc
	tmp=/tmp/$${$$}; \
	rc=0; \
	{ \
	${LATEX} \
		-halt-on-error -interaction=nonstopmode \
		-file-line-error \
		-output-directory ${Dir} \
		${Lo}'\input' ${1} > $${tmp} ||\
		{ rc=$${?}; tail -20 $${tmp}; }; \
	${RM} $${tmp}; \
	test $${rc} -eq 0; \
	}
endef

TEXINPUTS := ${Dir}:${TEXINPUTS}
export TEXINPUTS
DVIPDFMX ?= dvipdfmx -q -f erewhon.map -f newtx.map -f Chivo.map
PDFCROP ?= pdfcrop --noverbose
