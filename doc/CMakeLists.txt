# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
project(Zephyr-Kernel-Doc LANGUAGES)

set(NO_BOILERPLATE TRUE)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE} ..)

file(TO_CMAKE_PATH "${ZEPHYR_BASE}" ZEPHYR_BASE)
message(STATUS "Zephyr base: ${ZEPHYR_BASE}")

#-------------------------------------------------------------------------------
# Options

set(SPHINXOPTS "-j auto" CACHE STRING "Default Sphinx Options")
set(DOC_TAG "development" CACHE STRING "Documentation tag")

separate_arguments(SPHINXOPTS)

#-------------------------------------------------------------------------------
# Dependencies

set(DOXYGEN_SKIP_DOT True)
find_package(Doxygen REQUIRED)

find_program(SPHINXBUILD sphinx-build)
if(NOT SPHINXBUILD)
  message(FATAL_ERROR "The 'sphinx-build' command was not found")
endif()

find_package(LATEX COMPONENTS PDFLATEX)
find_program(LATEXMK latexmk)
if(NOT LATEX_PDFLATEX_FOUND OR NOT LATEXMK)
  message(WARNING "LaTeX components not found. PDF build will not be available.")
endif()

include(${ZEPHYR_BASE}/cmake/python.cmake)

# Find west to (optionally) process modules for Kconfig
find_program(WEST west)

# Include version info
include(${ZEPHYR_BASE}/cmake/version.cmake)
# Process modules
set(KCONFIG_BINARY_DIR ${CMAKE_BINARY_DIR}/Kconfig)
list(INSERT MODULE_EXT_ROOT 0 ${ZEPHYR_BASE})
file(MAKE_DIRECTORY ${KCONFIG_BINARY_DIR})
include(${ZEPHYR_BASE}/cmake/extensions.cmake)
include(${ZEPHYR_BASE}/cmake/zephyr_module.cmake)

set(RST_OUT ${CMAKE_CURRENT_BINARY_DIR}/src)

#-------------------------------------------------------------------------------
# Doxygen (standalone)

set(DOXY_OUT ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
set(DOXYFILE_IN ${CMAKE_CURRENT_LIST_DIR}/zephyr.doxyfile.in)
set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/zephyr.doxyfile)

configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

add_custom_target(
  doxygen
  COMMAND
    ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
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

#
# Generated Kconfig .rst documents
#

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
  COMMAND ${CMAKE_COMMAND} -E make_directory ${RST_OUT}/reference/kconfig
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
    ${PYTHON_EXECUTABLE} _scripts/gen_kconfig_rest.py ${RST_OUT}/reference/kconfig/
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
  COMMENT "Running gen_kconfig_rest.py ${RST_OUT}/reference/kconfig/"
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
# - DTS_BINDINGS_RST_OUT: ${RST_OUT}/reference/devicetree
#

set(GEN_DEVICETREE_REST_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/_scripts/gen_devicetree_rest.py)

if(NOT DTS_ROOTS)
  set(DTS_ROOTS ${ZEPHYR_BASE})
endif()

set(DTS_ROOT_ARGS)
foreach(root ${DTS_ROOTS})
  list(APPEND DTS_ROOT_ARGS --dts-root ${root})
endforeach()

if(NOT DTS_BINDINGS_RST_OUT)
  set(DTS_BINDINGS_RST_OUT ${RST_OUT}/reference/devicetree)
endif()

add_custom_target(
  devicetree
  COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH=${ZEPHYR_BASE}/scripts/dts/python-devicetree/src${SEP}$ENV{PYTHONPATH}
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

#-------------------------------------------------------------------------------
# Environment & Paths

set(SPHINX_ENV
  ZEPHYR_BASE=${ZEPHYR_BASE}
  ZEPHYR_BUILD=${CMAKE_CURRENT_BINARY_DIR}
  DOXYGEN_EXECUTABLE=${DOXYGEN_EXECUTABLE}
)

set(DOCS_CFG_DIR ${CMAKE_CURRENT_LIST_DIR})
set(DOCS_DOCTREE_DIR ${CMAKE_CURRENT_BINARY_DIR}/doctrees)
set(DOCS_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(DOCS_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/src)
set(DOCS_HTML_DIR ${CMAKE_CURRENT_BINARY_DIR}/html)
set(DOCS_LATEX_DIR ${CMAKE_CURRENT_BINARY_DIR}/latex)

#-------------------------------------------------------------------------------
# Functions

# Create a custom doc target.
#
# This function has the same signature as `add_custom_target()`
#
# The function will create two targets for the doc build system.
# - Target 1 named: `<name>`
# - Target 2 named: `<name>-nodeps`
#
# Both targets will produce same result, but target 2 must have no dependencies.
# This is useful to, e.g. re-run the Sphinx build without dependencies such as
# devicetree or Kconfig generators.
#
function(add_doc_target name)
  add_custom_target(${name} ${ARGN})
  add_custom_target(${name}-nodeps ${ARGN})
endfunction()

#-------------------------------------------------------------------------------
# html

add_doc_target(
  html
  COMMAND ${CMAKE_COMMAND} -E env ${SPHINX_ENV}
  ${SPHINXBUILD}
    -b html
    -c ${DOCS_CFG_DIR}
    -d ${DOCS_DOCTREE_DIR}
    -w ${DOCS_BUILD_DIR}/html.log
    -t ${DOC_TAG}
    ${SPHINXOPTS}
    ${DOCS_SRC_DIR}
    ${DOCS_HTML_DIR}
  USES_TERMINAL
)

add_dependencies(html devicetree kconfig)

#-------------------------------------------------------------------------------
# pdf

set(FIX_TEX_SCRIPT ${ZEPHYR_BASE}/doc/_scripts/fix_tex.py)

add_doc_target(
  latex
  COMMAND ${CMAKE_COMMAND} -E env ${SPHINX_ENV}
  ${SPHINXBUILD}
    -b latex
    -c ${DOCS_CFG_DIR}
    -d ${DOCS_DOCTREE_DIR}
    -w ${DOCS_BUILD_DIR}/latex.log
    -t ${DOC_TAG}
    -t svgconvert
    ${SPHINXOPTS}
    ${DOCS_SRC_DIR}
    ${DOCS_LATEX_DIR}
  COMMAND ${PYTHON_EXECUTABLE} ${FIX_TEX_SCRIPT} ${DOCS_LATEX_DIR}/zephyr.tex
  USES_TERMINAL
)

add_dependencies(latex kconfig devicetree)

if(LATEX_PDFLATEX_FOUND AND LATEXMK)
  add_custom_target(
    pdf
    COMMAND ${CMAKE_COMMAND} -E env LATEXOPTS="-halt-on-error -no-shell-escape"
    ${LATEXMK} -quiet -pdf -dvi- -ps-
    WORKING_DIRECTORY ${DOCS_LATEX_DIR}
  )

  add_dependencies(pdf latex)
endif()
