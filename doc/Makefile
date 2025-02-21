# ------------------------------------------------------------------------------
# Makefile for documentation build
# SPDX-License-Identifier: Apache-2.0

BUILDDIR ?= _build
DOC_TAG ?= development
SPHINXOPTS ?= -j auto -W --keep-going -T
SPHINXOPTS_EXTRA ?=
LATEXMKOPTS ?= -halt-on-error -no-shell-escape
DT_TURBO_MODE ?= 0
HW_FEATURES_TURBO_MODE ?= 0

# ------------------------------------------------------------------------------
# Documentation targets

.PHONY: configure clean html html-fast html-live html-live-fast latex pdf doxygen

html-fast:
	${MAKE} html DT_TURBO_MODE=1 HW_FEATURES_TURBO_MODE=1

html-live-fast:
	${MAKE} html-live DT_TURBO_MODE=1 HW_FEATURES_TURBO_MODE=1

html html-live latex pdf linkcheck doxygen: configure
	cmake --build ${BUILDDIR} --target $@

configure:
	cmake \
		-GNinja \
		-B${BUILDDIR} \
		-S. \
		-DDOC_TAG=${DOC_TAG} \
		-DSPHINXOPTS="${SPHINXOPTS}" \
		-DSPHINXOPTS_EXTRA="${SPHINXOPTS_EXTRA}" \
		-DLATEXMKOPTS="${LATEXMKOPTS}" \
		-DDT_TURBO_MODE=${DT_TURBO_MODE} \
		-DHW_FEATURES_TURBO_MODE=${HW_FEATURES_TURBO_MODE}

clean:
	cmake --build ${BUILDDIR} --target clean
