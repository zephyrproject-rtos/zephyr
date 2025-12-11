# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
project(Zephyr-Kernel-Doc LANGUAGES)

set(MIN_WEST_VERSION 1.0.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE} .. COMPONENTS doc)

file(TO_CMAKE_PATH "${ZEPHYR_BASE}" ZEPHYR_BASE)
message(STATUS "Zephyr base: ${ZEPHYR_BASE}")

#-------------------------------------------------------------------------------
# Options

set(SPHINXOPTS "-j auto -W --keep-going -T" CACHE STRING "Default Sphinx Options")
set(SPHINXOPTS_EXTRA "" CACHE STRING "Extra Sphinx Options (added to defaults)")
set(LATEXMKOPTS "-halt-on-error -no-shell-escape" CACHE STRING "Default latexmk options")
set(DT_TURBO_MODE OFF CACHE BOOL "Enable DT turbo mode")
set(HW_FEATURES_TURBO_MODE OFF CACHE BOOL "Enable HW features turbo mode")
set(HW_FEATURES_VENDOR_FILTER "" CACHE STRING "Vendor filter for HW features")
set(DOC_TAG "development" CACHE STRING "Documentation tag")
set(DTS_ROOTS "${ZEPHYR_BASE}" CACHE STRING "DT bindings root folders")

separate_arguments(SPHINXOPTS)
separate_arguments(SPHINXOPTS_EXTRA)
separate_arguments(LATEXMKOPTS)

#-------------------------------------------------------------------------------
# Dependencies

find_package(Doxygen REQUIRED dot)

find_program(SPHINXBUILD sphinx-build)
if(NOT SPHINXBUILD)
  message(WARNING "The 'sphinx-build' command was not found; Sphinx-based targets will not be available.")
endif()
find_program(SPHINXAUTOBUILD sphinx-autobuild)

find_package(LATEX COMPONENTS PDFLATEX)
find_program(LATEXMK latexmk)
if(NOT LATEX_PDFLATEX_FOUND OR NOT LATEXMK)
  message(WARNING "LaTeX components not found. PDF build will not be available.")
endif()

#-------------------------------------------------------------------------------
# Environment & Paths

set(SPHINX_ENV
  DOXYGEN_EXECUTABLE=${DOXYGEN_EXECUTABLE}
  DOT_EXECUTABLE=${DOXYGEN_DOT_EXECUTABLE}
  DOCS_HTML_DIR=${CMAKE_CURRENT_BINARY_DIR}/html
)

if(DEFINED ENV{ZEPHYR_DOXYGEN_OVERLAY})
  if(EXISTS $ENV{ZEPHYR_DOXYGEN_OVERLAY})
    set(INCLUDE_CUSTOM_FILE "@INCLUDE = $ENV{ZEPHYR_DOXYGEN_OVERLAY}")
  else()
    message(FATAL_ERROR "Zephyr Doxygen overlay $ENV{ZEPHYR_DOXYGEN_OVERLAY} does not exist!")
  endif()
else()
  set(INCLUDE_CUSTOM_FILE "")
endif()

set(DOCS_CFG_DIR ${CMAKE_CURRENT_LIST_DIR})
set(DOCS_DOCTREE_DIR ${CMAKE_CURRENT_BINARY_DIR}/doctrees)
set(DOCS_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(DOCS_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/src)
set(DOCS_HTML_DIR ${CMAKE_CURRENT_BINARY_DIR}/html)
set(DOCS_LINKCHECK_DIR ${CMAKE_CURRENT_BINARY_DIR}/linkcheck)
set(DOCS_LATEX_DIR ${CMAKE_CURRENT_BINARY_DIR}/latex)

if(WIN32)
  set(SEP $<SEMICOLON>)
else()
  set(SEP :)
endif()

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
# devicetree generator.
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
set(ZEPHYR_VERSION "${Zephyr_VERSION}")

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
# Doxygen Coverage

add_custom_target(
  doxygen-coverage
  COMMAND ${PYTHON_EXECUTABLE} -m coverxygen
    --xml-dir ${CMAKE_CURRENT_BINARY_DIR}/doxygen/xml/
    --src-dir ${ZEPHYR_BASE}/include/
    --output ${CMAKE_CURRENT_BINARY_DIR}/doc-coverage.info
  COMMAND lcov
    --remove ${CMAKE_CURRENT_BINARY_DIR}/doc-coverage.info "*/deprecated"
    > ${CMAKE_CURRENT_BINARY_DIR}/new.info
  COMMAND genhtml
    --no-function-coverage
    --no-branch-coverage
    ${CMAKE_CURRENT_BINARY_DIR}/new.info
    -o ${CMAKE_CURRENT_BINARY_DIR}/coverage-report
  DEPENDS doxygen
  COMMENT "Generating Doxygen coverage info and HTML report..."
)

#-------------------------------------------------------------------------------
# devicetree

set(GEN_DEVICETREE_REST_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/_scripts/gen_devicetree_rest.py)

set(DTS_ARGS)
foreach(root ${DTS_ROOTS})
  list(APPEND DTS_ARGS --dts-root ${root})
endforeach()

if(DT_TURBO_MODE)
  list(APPEND DTS_ARGS --turbo-mode)
endif()

add_custom_target(
  devicetree
  COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH=${ZEPHYR_BASE}/scripts/dts/python-devicetree/src${SEP}$ENV{PYTHONPATH}
  ZEPHYR_BASE=${ZEPHYR_BASE}
  ${PYTHON_EXECUTABLE} ${GEN_DEVICETREE_REST_SCRIPT}
    --vendor-prefixes ${ZEPHYR_BASE}/dts/bindings/vendor-prefixes.txt
    ${DTS_ARGS}
    ${DOCS_SRC_DIR}/build/dts/api
  VERBATIM
  USES_TERMINAL
  COMMENT "Generating Devicetree bindings documentation..."
)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${GEN_DEVICETREE_REST_SCRIPT})

#-------------------------------------------------------------------------------
# html

set(SPHINX_TAGS "${DOC_TAG}")
if(HW_FEATURES_TURBO_MODE)
  list(APPEND SPHINX_TAGS "hw_features_turbo")
endif()

set(SPHINX_TAGS_ARGS "")
foreach(tag ${SPHINX_TAGS})
  list(APPEND SPHINX_TAGS_ARGS "-t" "${tag}")
endforeach()

if(HW_FEATURES_VENDOR_FILTER)
  list(JOIN HW_FEATURES_VENDOR_FILTER "," vendor_filter)
  list(APPEND SPHINXOPTS "-D" "zephyr_hw_features_vendor_filter=${vendor_filter}")
endif()

if(SPHINXBUILD)
  add_doc_target(
    html
    COMMAND ${CMAKE_COMMAND} -E env ${SPHINX_ENV} OUTPUT_DIR=${DOCS_HTML_DIR}
    ${SPHINXBUILD}
      -b html
      -c ${DOCS_CFG_DIR}
      -d ${DOCS_DOCTREE_DIR}
      -w ${DOCS_BUILD_DIR}/html.log
      ${SPHINX_TAGS_ARGS}
      ${SPHINXOPTS}
      ${SPHINXOPTS_EXTRA}
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

  add_dependencies(html devicetree)
endif()

#-------------------------------------------------------------------------------
# html-live

if(SPHINXBUILD)
  add_doc_target(
    html-live
    COMMAND ${CMAKE_COMMAND} -E env ${SPHINX_ENV} OUTPUT_DIR=${DOCS_HTML_DIR}
    ${SPHINXAUTOBUILD}
      --watch ${DOCS_CFG_DIR}
      --ignore ${DOCS_BUILD_DIR}
      -b html
      -c ${DOCS_CFG_DIR}
      -d ${DOCS_DOCTREE_DIR}
      -w ${DOCS_BUILD_DIR}/html.log
      ${SPHINX_TAGS_ARGS}
      ${SPHINXOPTS}
      ${SPHINXOPTS_EXTRA}
      ${DOCS_SRC_DIR}
      ${DOCS_HTML_DIR}
    USES_TERMINAL
    COMMENT "Running Sphinx HTML autobuild..."
  )

  set_target_properties(
    html-live html-live-nodeps
    PROPERTIES
      ADDITIONAL_CLEAN_FILES "${DOCS_SRC_DIR};${DOCS_HTML_DIR};${DOCS_DOCTREE_DIR}"
  )

  add_dependencies(html-live devicetree)
endif()
#-------------------------------------------------------------------------------
# pdf

if(SPHINXBUILD)
  add_doc_target(
    latex
    COMMAND ${CMAKE_COMMAND} -E env ${SPHINX_ENV} OUTPUT_DIR=${DOCS_LATEX_DIR}
    ${SPHINXBUILD}
      -b latex
      -c ${DOCS_CFG_DIR}
      -d ${DOCS_DOCTREE_DIR}
      -w ${DOCS_BUILD_DIR}/latex.log
      ${SPHINX_TAGS_ARGS}
      -t convertimages
      ${SPHINXOPTS}
      ${SPHINXOPTS_EXTRA}
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

  add_dependencies(latex devicetree)
endif()

if(SPHINXBUILD AND LATEX_PDFLATEX_FOUND AND LATEXMK)
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
# linkcheck

if(SPHINXBUILD)
  add_doc_target(
    linkcheck
    COMMAND ${CMAKE_COMMAND} -E env ${SPHINX_ENV} OUTPUT_DIR=${DOCS_LINKCHECK_DIR}
    ${SPHINXBUILD}
      -b linkcheck
      -c ${DOCS_CFG_DIR}
      -d ${DOCS_DOCTREE_DIR}
      -w ${DOCS_BUILD_DIR}/linkcheck.log
      ${SPHINX_TAGS_ARGS}
      ${SPHINXOPTS}
      ${SPHINXOPTS_EXTRA}
      ${DOCS_SRC_DIR}
      ${DOCS_LINKCHECK_DIR}
    USES_TERMINAL
    COMMENT "Running Sphinx link check..."
  )

  set_target_properties(
    linkcheck linkcheck-nodeps
    PROPERTIES
      ADDITIONAL_CLEAN_FILES "${DOCS_SRC_DIR};${DOCS_LINKCHECK_DIR};${DOCS_DOCTREE_DIR}"
  )

  add_dependencies(linkcheck devicetree)
endif()

#-------------------------------------------------------------------------------
# others

add_custom_target(
  pristine
  COMMAND ${CMAKE_COMMAND} -P ${ZEPHYR_BASE}/cmake/pristine.cmake
)
