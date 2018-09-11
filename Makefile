#
# Top level makefile for things not covered by cmake
#

ifndef ZEPHYR_BASE
$(error The ZEPHYR_BASE environment variable must be set)
endif

BUILDDIR ?= doc/_build
DOC_TAG ?= development
SPHINXOPTS ?= -q

# Documentation targets
# ---------------------------------------------------------------------------
htmldocs:
	mkdir -p ${BUILDDIR} && cmake -GNinja -DDOC_TAG=${DOC_TAG} -DSPHINXOPTS=${SPHINXOPTS} -B${BUILDDIR} -Hdoc/ && ninja -C ${BUILDDIR} htmldocs

pdfdocs:
	mkdir -p ${BUILDDIR} && cmake -GNinja -DDOC_TAG=${DOC_TAG} -DSPHINXOPTS=${SPHINXOPTS} -B${BUILDDIR} -Hdoc/ && ninja -C ${BUILDDIR} pdfdocs
