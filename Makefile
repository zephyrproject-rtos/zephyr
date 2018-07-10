#
# Top level makefile for things not covered by cmake
#

BUILDDIR ?= doc/_build

# Documentation targets
# ---------------------------------------------------------------------------
htmldocs:
	mkdir -p ${BUILDDIR} && cmake -G"Unix Makefiles" -B${BUILDDIR} -Hdoc/ && make -s -C ${BUILDDIR} htmldocs
