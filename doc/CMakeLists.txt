# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
project(Zephyr-Kernel-Doc LANGUAGES)

set(NO_BOILERPLATE TRUE)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE} ..)

# Find west to (optionally) process modules for Kconfig
find_program(
  WEST
  west
  )
if(${WEST} STREQUAL WEST-NOTFOUND)
  unset(WEST)
endif()

file(TO_CMAKE_PATH "${ZEPHYR_BASE}" ZEPHYR_BASE)

message(STATUS "Zephyr base: ${ZEPHYR_BASE}")
if(DEFINED WEST)
  execute_process(COMMAND west --version
    OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE WEST_VERSION)
  message(STATUS "West: ${WEST} (west --version: \"${WEST_VERSION}\")")
else()
  message(STATUS "West: not found")
endif()

include(${ZEPHYR_BASE}/cmake/python.cmake)
set(DOXYGEN_SKIP_DOT True)
find_package(Doxygen REQUIRED)
find_package(LATEX)

find_program(
  SPHINXBUILD
  NAMES sphinx-build-3 sphinx-build
  )
if(${SPHINXBUILD} STREQUAL SPHINXBUILD-NOTFOUND)
  message(FATAL_ERROR "The 'sphinx-build' command was not found. Make sure you have Sphinx installed.")
endif()

# Include version info
include(${ZEPHYR_BASE}/cmake/version.cmake)
# Process modules
set(KCONFIG_BINARY_DIR ${CMAKE_BINARY_DIR}/Kconfig)
list(INSERT MODULE_EXT_ROOT 0 ${ZEPHYR_BASE})
file(MAKE_DIRECTORY ${KCONFIG_BINARY_DIR})
include(${ZEPHYR_BASE}/cmake/extensions.cmake)
include(${ZEPHYR_BASE}/cmake/zephyr_module.cmake)

# Note that this won't force fatal error if latexmk is not found.
# Not having LaTeX tools should not prevent people from generating HTML docs.
find_program(
  LATEXMK
  latexmk
  )
if(${LATEXMK} STREQUAL LATEXMK-NOTFOUND)
  message(WARNING "The 'latexmk' command was not found. Targets to build PDF will not be available.")
endif()

if(NOT DEFINED SPHINXOPTS)
  set(SPHINXOPTS -q)
else()
  separate_arguments(SPHINXOPTS)
endif()

if(NOT DEFINED SPHINX_OUTPUT_DIR)
  set(SPHINX_OUTPUT_DIR_HTML ${CMAKE_CURRENT_BINARY_DIR}/html)
  set(SPHINX_OUTPUT_DIR_LATEX ${CMAKE_CURRENT_BINARY_DIR}/latex)
  set(SPHINX_OUTPUT_DIR_PDF ${CMAKE_CURRENT_BINARY_DIR}/pdf)
else()
  # SPHINX_OUTPUT_DIR is used to specify exactly where HTML (or other
  # outputs) are placed, so no /html, /latex, /pdf suffixes are needed.
  set(SPHINX_OUTPUT_DIR_HTML ${SPHINX_OUTPUT_DIR})
  set(SPHINX_OUTPUT_DIR_LATEX ${SPHINX_OUTPUT_DIR})
  set(SPHINX_OUTPUT_DIR_PDF ${SPHINX_OUTPUT_DIR})
endif()

if(NOT DEFINED DOC_TAG)
  set(DOC_TAG development)
endif()

# Internal variables.
set(ALLSPHINXOPTS  -d ${CMAKE_CURRENT_BINARY_DIR}/doctrees ${SPHINXOPTS})
if("-q" IN_LIST ALLSPHINXOPTS)
  set(SPHINX_USES_TERMINAL )
else()
  set(SPHINX_USES_TERMINAL USES_TERMINAL)
endif()

# the i18n builder cannot share the environment and doctrees with the others
set(I18NSPHINXOPTS  ${SPHINXOPTS})

set(DOXYFILE_IN ${CMAKE_CURRENT_LIST_DIR}/zephyr.doxyfile.in)
set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/zephyr.doxyfile)
set(DOXY_OUT ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
set(RST_OUT ${CMAKE_CURRENT_BINARY_DIR}/rst)
set(DOC_LOG ${CMAKE_CURRENT_BINARY_DIR}/doc.log)
set(DOXY_LOG ${CMAKE_CURRENT_BINARY_DIR}/doxy.log)
set(SPHINX_LOG ${CMAKE_CURRENT_BINARY_DIR}/sphinx.log)
set(DOC_WARN ${CMAKE_CURRENT_BINARY_DIR}/doc.warnings)
set(CONTENT_OUTPUTS ${CMAKE_CURRENT_BINARY_DIR}/extracted-content.txt)

configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

# This command is used to copy all documentation source files from the
# ZEPHYR_BASE/ environment variable into the build directory,
#
# We need to make copies because Sphinx requires a single
# documentation root directory, but Zephyr's documentation is
# scattered around the tree in samples/, boards/, and doc/. Putting
# them into a single rooted tree in the build directory is a
# workaround for this limitation.
set(EXTRACT_CONTENT_COMMAND
  ${CMAKE_COMMAND} -E env
  ZEPHYR_BASE=${ZEPHYR_BASE}
  ${PYTHON_EXECUTABLE} scripts/extract_content.py
  # Ignore any files in the output directory.
  --ignore ${CMAKE_CURRENT_BINARY_DIR}
  # Copy all files in doc to the rst folder.
  "*:doc:${RST_OUT}"
  # We want to copy the .rst files in samples/ and boards/ to the rst
  # folder, and also the doc folder inside rst.
  #
  # Some files refer to items in samples/ and boards/ relative to
  # their actual position in the Zephyr tree. For example, in
  # subsystems/sensor.rst:
  #
  # .. literalinclude:: ../../samples/sensor/mcp9808/src/main.c
  #
  # The additional copy is a hackaround so these references work.
  "*.rst:samples:${RST_OUT}" "*.rst:boards:${RST_OUT}"
  "*.rst:samples:${RST_OUT}/doc" "*.rst:boards:${RST_OUT}/doc")

add_custom_target(
  content
  # Copy all files in doc/ to the rst folder
  COMMAND ${EXTRACT_CONTENT_COMMAND}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMENT "Copying files to ${RST_OUT}"
)

# For incremental builds not to miss any source change this MUST be kept
# a superset of INPUT= and FILE_PATTERNS= in zephyr.doxyfile.in
#
# NOTE: any changes here should be reflected in .github/workflows/doc-build.yml
file(GLOB_RECURSE  DOXY_SOURCES
  ${ZEPHYR_BASE}/include/*.[c,h,S]
  ${ZEPHYR_BASE}/kernel/include/kernel_arch_interface.h
  ${ZEPHYR_BASE}/lib/libc/*.[c,h,S]
  ${ZEPHYR_BASE}/subsys/testsuite/ztest/include/*.[h,c,S]
  ${ZEPHYR_BASE}/tests/*.[h,c,S]
  )
# For debug. Also find generated list in doc/_build/(build.ninja|CMakeFiles/)
# message("DOXY_SOURCES= " ${DOXY_SOURCES})

set(ARGS ${DOXYFILE_OUT})
set(DOXY_RUN_TSTAMP ${CMAKE_CURRENT_BINARY_DIR}/last_doxy_run_tstamp)


# Create timestamp first so we re-run if source files are edited while
# doxygen is running
add_custom_command(
  OUTPUT ${DOXY_RUN_TSTAMP}
  COMMAND cmake -E touch ${DOXY_RUN_TSTAMP}
  COMMAND ${CMAKE_COMMAND}
    -DCOMMAND=${DOXYGEN_EXECUTABLE}
    -DARGS="${ARGS}"
    -DOUTPUT_FILE=${DOXY_LOG}
    -DERROR_FILE=${DOXY_LOG}
    -DWORKING_DIRECTORY=${CMAKE_CURRENT_LIST_DIR}
    -P ${ZEPHYR_BASE}/cmake/util/execute_process.cmake
  DEPENDS ${DOXY_SOURCES}
  COMMENT "Running ${DOXYGEN_EXECUTABLE}"
)

# Doxygen doesn't support incremental builds.
# It could be ok because it's pretty fast.
# But it's not because it has a cascade effect on sphinx:
# https://sourceforge.net/p/doxygen/mailman/message/36580807/
# For now this optimization speeds-up non-doxygen documentation work
# only (by one order of magnitude).
add_custom_target(
  doxy_real_modified_times
  COMMAND ${CMAKE_COMMAND} -E env
    ${PYTHON_EXECUTABLE} scripts/restore_modification_times.py
    --loglevel  WARN  ${DOXY_OUT}/xml
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  DEPENDS ${DOXY_RUN_TSTAMP}
  COMMENT "Fixing modification times of ${DOXY_OUT}/xml/ output"
)

add_custom_target(
  pristine
  COMMAND ${CMAKE_COMMAND} -P ${ZEPHYR_BASE}/cmake/pristine.cmake
)

if(WIN32)
  set(SEP $<SEMICOLON>)
else()
  set(SEP :)
endif()

set(KI_SCRIPT ${ZEPHYR_BASE}/scripts/filter-known-issues.py)
set(FIX_TEX_SCRIPT ${ZEPHYR_BASE}/doc/scripts/fix_tex.py)
set(CONF_DIR ${ZEPHYR_BASE}/.known-issues/doc)

#
# Generated Kconfig .rst documents
#

file(WRITE ${KCONFIG_BINARY_DIR}/Kconfig.soc.defconfig
     "osource \"${ZEPHYR_BASE}/soc/$(ARCH)/*/Kconfig.defconfig\"\n"
)
file(WRITE ${KCONFIG_BINARY_DIR}/Kconfig.soc
     "osource \"${ZEPHYR_BASE}/soc/$(ARCH)/*/Kconfig.soc\"\n"
)
file(WRITE ${KCONFIG_BINARY_DIR}/Kconfig.shield.defconfig
     "osource \"${ZEPHYR_BASE}/boards/shields/*/Kconfig.defconfig\"\n"
)
file(WRITE ${KCONFIG_BINARY_DIR}/Kconfig.shield
     "osource \"${ZEPHYR_BASE}/boards/shields/*/Kconfig.shield\"\n"
)
file(WRITE ${KCONFIG_BINARY_DIR}/Kconfig.soc.arch
     "osource \"${ZEPHYR_BASE}/soc/$(ARCH)/Kconfig\"\n"
     "osource \"${ZEPHYR_BASE}/soc/$(ARCH)/*/Kconfig\"\n"
)

foreach(module_name ${ZEPHYR_MODULE_NAMES})
  zephyr_string(SANITIZE TOUPPER MODULE_NAME_UPPER ${module_name})
  list(APPEND
       ZEPHYR_KCONFIG_MODULES
       "ZEPHYR_${MODULE_NAME_UPPER}_MODULE_DIR=${ZEPHYR_${MODULE_NAME_UPPER}_MODULE_DIR}"
  )

  if(ZEPHYR_${MODULE_NAME_UPPER}_KCONFIG)
    list(APPEND
         ZEPHYR_KCONFIG_MODULES
         "ZEPHYR_${MODULE_NAME_UPPER}_KCONFIG=${ZEPHYR_${MODULE_NAME_UPPER}_KCONFIG}"
  )
  endif()
endforeach()

add_custom_target(
  kconfig
  COMMAND ${CMAKE_COMMAND} -E make_directory ${RST_OUT}/doc/reference/kconfig
  COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH=${ZEPHYR_BASE}/scripts/kconfig${SEP}$ENV{PYTHONPATH}
  ZEPHYR_BASE=${ZEPHYR_BASE}
  srctree=${ZEPHYR_BASE}
  BOARD_DIR=boards/*/*
  ARCH=*
  ARCH_DIR=arch
  SOC_DIR=soc
  KCONFIG_BINARY_DIR=${KCONFIG_BINARY_DIR}
  KCONFIG_WARN_UNDEF=y
  KCONFIG_TURBO_MODE=${KCONFIG_TURBO_MODE}
  KCONFIG_DOC_MODE=1
  ${ZEPHYR_KCONFIG_MODULES}
    ${PYTHON_EXECUTABLE} scripts/gen_kconfig_rest.py ${RST_OUT}/doc/reference/kconfig/
      --separate-all-index
      --keep-module-paths
      --modules Architecture,arch,${ZEPHYR_BASE}/arch
                Driver,drivers,${ZEPHYR_BASE}/drivers
                Kernel,kernel,${ZEPHYR_BASE}/kernel
                Library,lib,${ZEPHYR_BASE}/lib
                Subsystem,subsys,${ZEPHYR_BASE}/subsys
                "External Module,modules,${ZEPHYR_BASE}/modules"

  VERBATIM
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMENT "Running gen_kconfig_rest.py ${RST_OUT}/doc/reference/kconfig/"
)

#
# Generated devicetree .rst documents
#
# The devicetree bindings discovered in ${DTS_ROOTS} are parsed and
# documentation for them is generated in the directory
# ${DTS_BINDINGS_RST_OUT}.
#
# The CMake variable GEN_DEVICETREE_REST_ZEPHYR_DOCSET will
# be passed to the script in the environment. This allows separating
# the bindings documentation into a standalone Sphinx docset that
# nonetheless can link to Zephyr documentation using intersphinx.
# If empty, the variable has no effect on the script.
#
# If not set, these are the default values for these variables:
#
# - DTS_ROOTS: ZEPHYR_BASE
# - DTS_BINDINGS_RST_OUT: ${RST_OUT}/doc/reference/devicetree
#

set(GEN_DEVICETREE_REST_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/scripts/gen_devicetree_rest.py)

if(NOT DTS_ROOTS)
  set(DTS_ROOTS ${ZEPHYR_BASE})
endif()

set(DTS_ROOT_ARGS)
foreach(root ${DTS_ROOTS})
  list(APPEND DTS_ROOT_ARGS --dts-root ${root})
endforeach()

if(NOT DTS_BINDINGS_RST_OUT)
  set(DTS_BINDINGS_RST_OUT ${RST_OUT}/doc/reference/devicetree)
endif()

add_custom_target(
  devicetree
  COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH=${ZEPHYR_BASE}/scripts/dts${SEP}$ENV{PYTHONPATH}
  ZEPHYR_BASE=${ZEPHYR_BASE}
  GEN_DEVICETREE_REST_ZEPHYR_DOCSET=${GEN_DEVICETREE_REST_ZEPHYR_DOCSET}
  ${PYTHON_EXECUTABLE} ${GEN_DEVICETREE_REST_SCRIPT}
    --vendor-prefixes ${ZEPHYR_BASE}/dts/bindings/vendor-prefixes.txt
    ${DTS_ROOT_ARGS} ${DTS_BINDINGS_RST_OUT}
  VERBATIM
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMENT "Running gen_devicetree_rest.py ${DTS_BINDINGS_RST_OUT}"
  USES_TERMINAL
)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${GEN_DEVICETREE_REST_SCRIPT})

#
# HTML section
#
set(SPHINX_BUILD_HTML_COMMAND
  ${CMAKE_COMMAND} -E env
  ZEPHYR_BASE=${ZEPHYR_BASE}
  ZEPHYR_BUILD=${CMAKE_CURRENT_BINARY_DIR}
  ${SPHINXBUILD} -w ${SPHINX_LOG} -N -t ${DOC_TAG} -b html ${ALLSPHINXOPTS} ${RST_OUT}/doc ${SPHINX_OUTPUT_DIR_HTML})

# The sphinx-html target is provided as a convenience for incremental
# re-builds of content files without regenerating the entire docs
# pipeline. It can be significantly faster than re-running the full
# HTML build, but it has no idea if Doxygen, Kconfig, etc. need to be
# regenerated. Use with caution.
add_custom_target(
  sphinx-html
  COMMAND ${SPHINX_BUILD_HTML_COMMAND}
  COMMENT "Just re-generating HTML (USE WITH CAUTION)"
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  USES_TERMINAL
)
add_dependencies(sphinx-html content)

# "breathe", the sphinx plugin that parses XML output from doxygen, has
# an "everything on everything" dependency issue reported at:
# https://github.com/michaeljones/breathe/issues/420 In other words
# changing 1 source file costs the same build time than changing all
# source files. breathe is fortunately smart enough not to run at all
# when *nothing* has changed.
add_custom_target(
  html
  COMMAND ${SPHINX_BUILD_HTML_COMMAND}
  # Merge the Doxygen and Sphinx logs into a single file
  COMMAND ${CMAKE_COMMAND} -P ${ZEPHYR_BASE}/cmake/util/fmerge.cmake ${DOC_LOG} ${DOXY_LOG} ${SPHINX_LOG}
  COMMAND ${PYTHON_EXECUTABLE} ${KI_SCRIPT} --config-dir ${CONF_DIR} --errors ${DOC_WARN} --warnings ${DOC_WARN} ${DOC_LOG}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMENT "Generating HTML documentation with ${SPHINXBUILD} ${SPHINXOPTS}"
  ${SPHINX_USES_TERMINAL}
)

#
# LaTEX section
#
set(SPHINX_BUILD_LATEX_COMMAND
  ${CMAKE_COMMAND} -E env
  ZEPHYR_BUILD=${CMAKE_CURRENT_BINARY_DIR}
  ${SPHINXBUILD} -w ${SPHINX_LOG} -N -t ${DOC_TAG} -b latex -t svgconvert ${ALLSPHINXOPTS} ${RST_OUT}/doc ${SPHINX_OUTPUT_DIR_LATEX})

# The sphinx-latex target works similarly to sphinx-html, and carries
# the same warnings.
add_custom_target(
  sphinx-latex
  COMMAND ${SPHINX_BUILD_LATEX_COMMAND}
  COMMENT "Just re-generating LaTeX (USE WITH CAUTION)"
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  USES_TERMINAL
)
add_dependencies(sphinx-latex content)


add_custom_command(
  OUTPUT ${SPHINX_OUTPUT_DIR_LATEX}/zephyr.tex
  COMMAND ${SPHINX_BUILD_LATEX_COMMAND}
  # Merge the Doxygen and Sphinx logs into a single file
  COMMAND ${CMAKE_COMMAND} -P ${ZEPHYR_BASE}/cmake/util/fmerge.cmake ${DOC_LOG} ${DOXY_LOG} ${SPHINX_LOG}
  COMMAND ${PYTHON_EXECUTABLE} ${KI_SCRIPT} --config-dir ${CONF_DIR} --errors ${DOC_WARN} --warnings ${DOC_WARN} ${DOC_LOG}
  COMMAND ${PYTHON_EXECUTABLE} ${FIX_TEX_SCRIPT} ${SPHINX_OUTPUT_DIR_LATEX}/zephyr.tex
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMENT "Generating LaTeX documentation"
  ${SPHINX_USES_TERMINAL}
)

add_custom_target(
  latex
  DEPENDS ${SPHINX_OUTPUT_DIR_LATEX}/zephyr.tex
)

#
# PDF section
#
if(NOT ${LATEXMK} STREQUAL LATEXMK-NOTFOUND)

add_custom_command(
  OUTPUT ${SPHINX_OUTPUT_DIR_LATEX}/zephyr.pdf
  DEPENDS latexdocs ${SPHINX_OUTPUT_DIR_LATEX}/zephyr.tex
  COMMAND ${CMAKE_COMMAND} -E env
  LATEXOPTS="-halt-on-error -no-shell-escape"
  ${LATEXMK} -quiet -pdf -dvi- -ps-
  WORKING_DIRECTORY ${SPHINX_OUTPUT_DIR_LATEX}
  COMMENT "Generating PDF documentation"
)

if(NOT DEFINED SPHINX_OUTPUT_DIR)
# Although latexmk allows specifying output directory,
# makeindex fails if one is specified.
# Hence the need of this to copy the PDF file over.
add_custom_command(
  OUTPUT ${SPHINX_OUTPUT_DIR_PDF}/zephyr.pdf
  COMMAND ${CMAKE_COMMAND} -E make_directory ${SPHINX_OUTPUT_DIR_PDF}
  COMMAND ${CMAKE_COMMAND} -E copy ${SPHINX_OUTPUT_DIR_LATEX}/zephyr.pdf ${SPHINX_OUTPUT_DIR_PDF}/zephyr.pdf
  DEPENDS ${SPHINX_OUTPUT_DIR_LATEX}/zephyr.pdf
)
endif()

add_custom_target(
  pdf
  DEPENDS ${SPHINX_OUTPUT_DIR_PDF}/zephyr.pdf
)

endif()

#
# Dependencies and final targets
#
add_dependencies(html content doxy_real_modified_times kconfig devicetree)

add_custom_target(
  htmldocs
)
add_dependencies(htmldocs html)


add_custom_target(
  doxygen
)
add_dependencies(doxygen doxy_real_modified_times)

add_dependencies(latex content doxy_real_modified_times kconfig devicetree)

add_custom_target(
  latexdocs
)
add_dependencies(latexdocs latex)

if(NOT ${LATEXMK} STREQUAL LATEXMK-NOTFOUND)

add_custom_target(
  pdfdocs
  DEPENDS latexdocs pdf
)
add_dependencies(pdfdocs pdf)

endif()
