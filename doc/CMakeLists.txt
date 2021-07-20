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
set(LATEXMKOPTS "-halt-on-error -no-shell-escape" CACHE STRING "Default latexmk options")
set(DOC_TAG "development" CACHE STRING "Documentation tag")
set(DTS_ROOTS "${ZEPHYR_BASE}" CACHE STRING "DT bindings root folders")

separate_arguments(SPHINXOPTS)
separate_arguments(LATEXMKOPTS)

#-------------------------------------------------------------------------------
# Dependencies

find_package(Doxygen REQUIRED dot)

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

#-------------------------------------------------------------------------------
# Environment & Paths

set(SPHINX_ENV
  DOXYGEN_EXECUTABLE=${DOXYGEN_EXECUTABLE}
  DOT_EXECUTABLE=${DOXYGEN_DOT_EXECUTABLE}
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
# Doxygen (standalone)

set(DOXY_OUT ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
set(DOXYFILE_IN ${CMAKE_CURRENT_LIST_DIR}/zephyr.doxyfile.in)
set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/zephyr.doxyfile)

configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

add_custom_target(
  doxygen
  COMMAND
    ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
  COMMENT "Running Doxygen..."
)

set_target_properties(
  doxygen
  PROPERTIES
    ADDITIONAL_CLEAN_FILES "${DOXY_OUT}"
)

#-------------------------------------------------------------------------------
# kconfig

set(KCONFIG_BINARY_DIR ${CMAKE_BINARY_DIR}/Kconfig)
list(INSERT MODULE_EXT_ROOT 0 ${ZEPHYR_BASE})
file(MAKE_DIRECTORY ${KCONFIG_BINARY_DIR})

include(${ZEPHYR_BASE}/cmake/extensions.cmake)
include(${ZEPHYR_BASE}/cmake/zephyr_module.cmake)

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

if(WIN32)
  set(SEP $<SEMICOLON>)
else()
  set(SEP :)
endif()

set(GEN_KCONFIG_REST_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/_scripts/gen_kconfig_rest.py)

add_custom_target(
  kconfig
  COMMAND ${CMAKE_COMMAND} -E make_directory ${DOCS_SRC_DIR}/reference/kconfig
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
    ${PYTHON_EXECUTABLE} ${GEN_KCONFIG_REST_SCRIPT} ${DOCS_SRC_DIR}/reference/kconfig/
      --separate-all-index
      --keep-module-paths
      --modules Architecture,arch,${ZEPHYR_BASE}/arch
                Driver,drivers,${ZEPHYR_BASE}/drivers
                Kernel,kernel,${ZEPHYR_BASE}/kernel
                Library,lib,${ZEPHYR_BASE}/lib
                Subsystem,subsys,${ZEPHYR_BASE}/subsys
                "External Module,modules,${ZEPHYR_BASE}/modules"

  VERBATIM
  COMMENT "Generating Kconfig documentation..."
)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${GEN_KCONFIG_REST_SCRIPT})

#-------------------------------------------------------------------------------
# devicetree

set(GEN_DEVICETREE_REST_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/_scripts/gen_devicetree_rest.py)

set(DTS_ROOT_ARGS)
foreach(root ${DTS_ROOTS})
  list(APPEND DTS_ROOT_ARGS --dts-root ${root})
endforeach()

add_custom_target(
  devicetree
  COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH=${ZEPHYR_BASE}/scripts/dts/python-devicetree/src${SEP}$ENV{PYTHONPATH}
  ZEPHYR_BASE=${ZEPHYR_BASE}
  ${PYTHON_EXECUTABLE} ${GEN_DEVICETREE_REST_SCRIPT}
    --vendor-prefixes ${ZEPHYR_BASE}/dts/bindings/vendor-prefixes.txt
    ${DTS_ROOT_ARGS}
    ${DOCS_SRC_DIR}/reference/devicetree
  VERBATIM
  USES_TERMINAL
  COMMENT "Generating Devicetree bindings documentation..."
)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${GEN_DEVICETREE_REST_SCRIPT})

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
  COMMENT "Running Sphinx HTML build..."
)

set_target_properties(
  html html-nodeps
  PROPERTIES
    ADDITIONAL_CLEAN_FILES "${DOCS_SRC_DIR};${DOCS_HTML_DIR};${DOCS_DOCTREE_DIR}"
)

add_dependencies(html devicetree kconfig)

#-------------------------------------------------------------------------------
# pdf

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
  USES_TERMINAL
  COMMENT "Running Sphinx LaTeX build..."
)

set_target_properties(
  latex latex-nodeps
  PROPERTIES
    ADDITIONAL_CLEAN_FILES "${DOCS_SRC_DIR};${DOCS_LATEX_DIR};${DOCS_DOCTREE_DIR}"
)

add_dependencies(latex kconfig devicetree)

if(LATEX_PDFLATEX_FOUND AND LATEXMK)
  if(WIN32)
    set(PDF_BUILD_COMMAND "make.bat")
  else()
    find_program(MAKE make)
    if(NOT MAKE)
      message(FATAL_ERROR "The 'make' command was not found")
    endif()
    set(PDF_BUILD_COMMAND ${MAKE})
  endif()

  add_custom_target(
    pdf
    COMMAND ${CMAKE_COMMAND} -E env LATEXMKOPTS="${LATEXMKOPTS}"
    ${PDF_BUILD_COMMAND}
    WORKING_DIRECTORY ${DOCS_LATEX_DIR}
    COMMENT "Building PDF file..."
    USES_TERMINAL
  )

  add_dependencies(pdf latex)
endif()

#-------------------------------------------------------------------------------
# others

add_custom_target(
  pristine
  COMMAND ${CMAKE_COMMAND} -P ${ZEPHYR_BASE}/cmake/pristine.cmake
)
