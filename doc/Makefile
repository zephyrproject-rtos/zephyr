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
HW_FEATURES_VENDOR_FILTER ?=
DOXYGEN_FORCE_SINGLE_THREAD ?= 0

# ------------------------------------------------------------------------------
# Documentation targets

.PHONY: configure clean html html-fast html-live html-live-fast latex pdf doxygen doxygen-coverage doxygen-coverage-json

html-fast:
	${MAKE} html DT_TURBO_MODE=1 HW_FEATURES_TURBO_MODE=1

html-live-fast:
	${MAKE} html-live DT_TURBO_MODE=1 HW_FEATURES_TURBO_MODE=1

doxygen-coverage doxygen-coverage-json:
	${MAKE} configure DOXYGEN_FORCE_SINGLE_THREAD=1
	cmake --build ${BUILDDIR} --target $@

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
		-DHW_FEATURES_TURBO_MODE=${HW_FEATURES_TURBO_MODE} \
		-DHW_FEATURES_VENDOR_FILTER=${HW_FEATURES_VENDOR_FILTER} \
		-DDOXYGEN_FORCE_SINGLE_THREAD=${DOXYGEN_FORCE_SINGLE_THREAD}

clean:
	cmake --build ${BUILDDIR} --target clean
