#
# Top level makefile for documentation build
#

BUILDDIR ?= doc/_build
DOC_TAG ?= development
SPHINXOPTS ?= -q

# Documentation targets
# ---------------------------------------------------------------------------
clean:
	rm -rf ${BUILDDIR}

htmldocs:
	mkdir -p ${BUILDDIR} && cmake -GNinja -DDOC_TAG=${DOC_TAG} -DSPHINXOPTS=${SPHINXOPTS} -B${BUILDDIR} -Hdoc/ && ninja -C ${BUILDDIR} htmldocs

htmldocs-fast:
	mkdir -p ${BUILDDIR} && cmake -GNinja -DKCONFIG_TURBO_MODE=1 -DDOC_TAG=${DOC_TAG} -DSPHINXOPTS=${SPHINXOPTS} -B${BUILDDIR} -Hdoc/ && ninja -C ${BUILDDIR} htmldocs

pdfdocs:
	mkdir -p ${BUILDDIR} && cmake -GNinja -DDOC_TAG=${DOC_TAG} -DSPHINXOPTS=${SPHINXOPTS} -B${BUILDDIR} -Hdoc/ && ninja -C ${BUILDDIR} pdfdocs
