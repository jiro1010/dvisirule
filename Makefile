
Dir ?= /tmp
export Dir

InstallBase ?= ${Dir}/texmf-dist
InstallLib ?= ${InstallBase}/lib
InstallSty ?= ${InstallBase}/lib/texinputs
InstallBin ?= ${InstallBase}/bin
export InstallLib InstallSty InstallBin

include cmd.mk
Lo = '\def\Dir{'${Dir}'}'
CFLAGS = -Wall -O

########################################

Name = dvisirule
Pdf = ${Dir}/${Name}.pdf
Runtime = $(addprefix ${Dir}/, \
	${Name}.sty ${Name} \
	$(addprefix ${Name}-, bin pgnum.awk expg.mk marker.awk) \
	)
Tgt = ${Pdf} ${Runtime}
Dtx = ${Name}.dtx

HelpTxt = ${Dir}/help.txt
Fig = demo
FigPdf = $(addprefix ${Dir}/, $(addsuffix .pdf, ${Fig}))
FigTex = $(addprefix ${Dir}/, $(addsuffix print.tex, ${Fig}))

-include privar.mk

########################################

all: ${Tgt}

clean:
	${RM} *~ ${Tgt} ${HelpTxt} ${FigPdf} ${FigTex}
	${MAKE} -C src $@

install: ${Runtime}
	install -m 444 -pD ${Dir}/${Name}.sty \
		${InstallSty}/${Name}.sty
	install -m 444 -pD ${Dir}/${Name}-pgnum.awk \
		${Dir}/${Name}-marker.awk \
		${Dir}/${Name}-expg.mk \
		${InstallLib}/
	install -m 555 -pD ${Dir}/${Name} \
		${InstallBin}/${Name}
	install -m 555 -pD -s ${Dir}/${Name}-bin \
		${InstallLib}/${Name}-bin
#	echo ${MAKE} -C src $@

########################################

sty: ${Dir}/${Name}.sty
${Dir}/${Name}.sty: ${Name}.ins ${Dtx}
	$(call Latex, $<)
	ls -l $@

define MakePdf
	$(call Latex, ${1}.dtx)
	$(call Latex, ${1}.dtx)
	cd ${Dir}; \
	${DVIPDFMX} -o ${2} ${1}.dvi
endef

${Dir}/${Name}.pdf: %.pdf: %.sty ${HelpTxt} ${FigPdf} ${FigTex}
	$(call MakePdf,${Name},$@)
	ls -l $@

${FigPdf}: bname = $(notdir $(basename $@))
${FigPdf}: ${Dir}/%.pdf: ${CURDIR}/%.tex ${Runtime}
	$(call Latex, $<)
	cd ${Dir}; \
	cp -p ${bname}.dvi ${bname}.dvi.save && \
	${Dir}/${Name} ${bname}.dvi ${bname}-si.dvi && \
	${DVIPDFMX} ${bname}-si.dvi && \
	${PDFCROP} ${bname}-si.pdf $@
	ebb -O $@ > ${Dir}/${bname}.bb

${FigTex}: demo.tex
	sed -n -e '/definecolor/,/hskip0pt/p' $< |\
	egrep -v '(hfill|hskip)' > $@

${HelpTxt}: ${Dir}/${Name}
	sh $< -h  | sed -e 's/\.sh//' > $@

${Dir}/${Name}: ${Name}.sh
	install -m 555 -pD $< $@

FORCE:
${Dir}/${Name}-bin: b = $(notdir $@)
${Dir}/${Name}-bin: FORCE
	test -e src/Makefile || sh reconf.sh src
	${MAKE} -C src ${b}
	cp -pu src/${b} $@

${Dir}/%.awk: %.awk
	cp -pu $< $@

${Dir}/%.mk: %.mk
	cp -pu $< $@

-include priv.mk
