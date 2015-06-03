#
# Very simple makefile for compiling very simple LaTeX code.
# 
# We might use UseLATEX.cmake macros instead...
#
# Supported targets:
#
# make pdf        - makes a PDF from each of the TeX files in the directory
# make clean      - removes some files produced by `make pdf`
# make distclean  - as `make clean`, plus removes the PDF files
# make File.pdf   - creates the specified PDF file from File.tex
# make File.clean - removes some temporary files created bt LaTeX
#
# Author: Gianluca Petrillo (petrillo@fnal.gov) showing his poor makefile writing skill
# Date:   June 3rd, 2015
#

############################################################################
### Program names
###
PDFLATEX=pdflatex
PDFLATEXOPT=

############################################################################
### Variable definitions
###

ALL_TEX:=$(wildcard *.tex)
ALL_TARGETS=$(ALL_TEX:%.tex=%)
ALL_PDF=$(ALL_TARGETS:%=%.pdf)

############################################################################
### Targets and rules
###
.PHONY: all pdf clean distclean debug

all: pdf

pdf: $(ALL_PDF)

clean: $(ALL_TARGETS:%=%.clean)

distclean: $(ALL_TARGETS:%=%.distclean)

debug:
	@echo "Source files: $(ALL_TEX)"
	@echo "Targets:      $(ALL_TARGETS)"
	@echo "PDF files:    $(ALL_PDF)"

############################################################################
### Implicit targets
###
%.pdf: %.tex
	$(PDFLATEX) $(PDFLATEXOPT) $^

%.clean:
	$(RM) "$(*).log" "$(*).aux" "$(*).vrb"

%.distclean: %.clean
	$(RM) "$(*).pdf"

############################################################################
