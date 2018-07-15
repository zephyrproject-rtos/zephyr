#
# Top level makefile for things not covered by cmake
#

BUILDDIR ?= doc/_build
DOC_TAG ?= development
SPHINXOPTS ?= -q

# Documentation targets
# ---------------------------------------------------------------------------
htmldocs:
	mkdir -p ${BUILDDIR} && cmake -G"Unix Makefiles" -DDOC_TAG=${DOC_TAG} -DSPHINXOPTS=${SPHINXOPTS} -B${BUILDDIR} -Hdoc/ && make -s -C ${BUILDDIR} htmldocs
