# ------------------------------------------------------------------------------
# Makefile for documentation build
# SPDX-License-Identifier: Apache-2.0

BUILDDIR ?= _build
DOC_TAG ?= development
SPHINXOPTS ?= -j auto
LATEXMKOPTS ?= -halt-on-error -no-shell-escape
DT_TURBO_MODE ?= 0

# ------------------------------------------------------------------------------
# Documentation targets

.PHONY: configure clean html html-fast latex pdf doxygen

html-fast:
	${MAKE} html DT_TURBO_MODE=1

# The `minimal` tag turns off all slow sphinx features for a much faster
# incremental build when editing .rst files interactively. As usual when
# making significant sphinx configuration changes, you must delete
# doc/_build/ first. WARNING: doxygen integration (breathe) will be
# missing, so building from scratch will print a LOT of warnings!
# However this is very useful for small, drive-by .rst fixes.
html-minimal:
	${MAKE} html DT_TURBO_MODE=1 SPHINXOPTS='-t minimal ${SPHINXOPTS}'


html latex pdf linkcheck doxygen: configure
	cmake --build ${BUILDDIR} --target $@

configure:
	cmake \
		-GNinja \
		-B${BUILDDIR} \
		-S. \
		-DDOC_TAG=${DOC_TAG} \
		-DSPHINXOPTS="${SPHINXOPTS}" \
		-DLATEXMKOPTS="${LATEXMKOPTS}" \
		-DDT_TURBO_MODE=${DT_TURBO_MODE}

clean:
	cmake --build ${BUILDDIR} --target clean
