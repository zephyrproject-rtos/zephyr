# ------------------------------------------------------------------------------
# Makefile for documentation build
# SPDX-License-Identifier: Apache-2.0

define set_cmake_variable_if_defined
  $(if ${$1},-D$1="${$1}")
endef

BUILDDIR ?= _build

# ------------------------------------------------------------------------------
# Documentation targets

.PHONY: configure clean html html-fast latex pdf doxygen

html-fast:
	${MAKE} html DT_TURBO_MODE=1

html latex pdf linkcheck doxygen: configure
	cmake --build ${BUILDDIR} --target $@

configure:
	cmake \
		-GNinja \
		-B${BUILDDIR} \
		-S. \
		$(call set_cmake_variable_if_defined,SPHINXOPTS) \
		$(call set_cmake_variable_if_defined,SPHINXOPTS_EXTRA) \
		$(call set_cmake_variable_if_defined,LATEXMKOPTS) \
		$(call set_cmake_variable_if_defined,DT_TURBO_MODE) \
		$(call set_cmake_variable_if_defined,DOC_TAG) \
		$(call set_cmake_variable_if_defined,DTS_ROOTS)

clean:
	cmake --build ${BUILDDIR} --target clean
