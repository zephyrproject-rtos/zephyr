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
NO_EXTERNAL_DEPS ?= 0
DOXYGEN_FORCE_SINGLE_THREAD ?= 0
SKIP_DOXYGEN ?= 0
SKIP_KCONFIG ?= 0
SKIP_BOARD_CATALOG ?= 0
SKIP_EXTERNAL_CONTENT ?= 0

# ------------------------------------------------------------------------------
# Documentation targets

.PHONY: configure clean html html-fast html-quick html-live html-live-fast latex pdf doxygen doxygen-coverage doxygen-coverage-json

html-fast:
	${MAKE} html DT_TURBO_MODE=1 HW_FEATURES_TURBO_MODE=1

# Fastest profile for previewing narrative RST changes: skip the Doxygen/API,
# Kconfig and board catalog layers entirely (and trim devicetree). Cross-refs
# into the skipped layers degrade to plain text. NOT suitable for publishing.
html-quick:
	${MAKE} html \
		DT_TURBO_MODE=1 \
		HW_FEATURES_TURBO_MODE=1 \
		SKIP_DOXYGEN=0 \
		SKIP_KCONFIG=1 \
		SKIP_BOARD_CATALOG=1 \
		SKIP_EXTERNAL_CONTENT=1

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
		-DNO_EXTERNAL_DEPS=${NO_EXTERNAL_DEPS} \
		-DDOXYGEN_FORCE_SINGLE_THREAD=${DOXYGEN_FORCE_SINGLE_THREAD} \
		-DSKIP_DOXYGEN=${SKIP_DOXYGEN} \
		-DSKIP_KCONFIG=${SKIP_KCONFIG} \
		-DSKIP_BOARD_CATALOG=${SKIP_BOARD_CATALOG} \
		-DSKIP_EXTERNAL_CONTENT=${SKIP_EXTERNAL_CONTENT}

clean:
	cmake --build ${BUILDDIR} --target clean
