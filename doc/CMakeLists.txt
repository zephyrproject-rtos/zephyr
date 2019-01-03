cmake_minimum_required(VERSION 3.13.1)
project(Zephyr-Kernel-Doc LANGUAGES)

set(ZEPHYR_BASE $ENV{ZEPHYR_BASE})

message(STATUS "Zephyr base: ${ZEPHYR_BASE}")

include(${ZEPHYR_BASE}/cmake/version.cmake)

find_package(PythonInterp 3.4)
set(DOXYGEN_SKIP_DOT True)
find_package(Doxygen REQUIRED)
find_package(LATEX)

find_program(
  SPHINXBUILD
  sphinx-build
  )
if(${SPHINXBUILD} STREQUAL SPHINXBUILD-NOTFOUND)
  message(FATAL_ERROR "The 'sphinx-build' command was not found. Make sure you have Sphinx installed.")
endif()

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

# SPHINX_OUTPUT_DIR is used to specify exactly where HTML (or other
# outputs) are placed, so no /html, /latex, /pdf suffixes are needed.
# Build target is to use SPHINX_OUTPUT_DIR as-is if defined, else
# use SPHINX_OUTPUT_DEFAULT_DIR to construct a path.
if(NOT DEFINED SPHINX_OUTPUT_DIR)
  set(SPHINX_OUTPUT_DEFAULT_DIR ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(NOT DEFINED DOC_TAG)
  set(DOC_TAG development)
endif()

# Internal variables.
if("-q" IN_LIST SPHINXOPTS)
  set(SPHINX_USES_TERMINAL )
else()
  set(SPHINX_USES_TERMINAL USES_TERMINAL)
endif()

# the i18n builder cannot share the environment and doctrees with the others
set(I18NSPHINXOPTS  ${SPHINXOPTS})

set(DOXYFILE_IN ${CMAKE_CURRENT_LIST_DIR}/zephyr.doxyfile.in)
set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/zephyr.doxyfile)
set(DOC_LOG ${CMAKE_CURRENT_BINARY_DIR}/doc.log)
set(DOXY_LOG ${CMAKE_CURRENT_BINARY_DIR}/doxy.log)
set(SPHINX_LOG ${CMAKE_CURRENT_BINARY_DIR}/sphinx.log)
set(DOC_WARN ${CMAKE_CURRENT_BINARY_DIR}/doc.warnings)

configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

#
# Macro to generate target for document content
#
# Note: this is a macro because variables set by this
#       are being used in constructing other targets.
#
macro(construct_content_target scope)
  # This command is used to copy all documentation source files into the build
  # directory,
  #
  # We need to make copies because Sphinx requires a single
  # documentation root directory, but Zephyr's documentation is
  # scattered around the tree in samples/, boards/, and doc/. Putting
  # them into a single rooted tree in the build directory is a
  # workaround for this limitation.
  set(EXTRACT_CONTENT_COMMAND
    ${CMAKE_COMMAND} -E env
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
    ${TGT_NAME_CONTENT}
    # Copy all files in doc/ to the rst folder
    COMMAND ${EXTRACT_CONTENT_COMMAND}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  )
endmacro()

set(DOXY_ARGS ${DOXYFILE_OUT})

add_custom_target(
  doxy
  COMMAND ${CMAKE_COMMAND}
    -DCOMMAND=${DOXYGEN_EXECUTABLE}
    -DARGS="${DOXY_ARGS}"
    -DOUTPUT_FILE=${DOXY_LOG}
    -DERROR_FILE=${DOXY_LOG}
    -DWORKING_DIRECTORY=${CMAKE_CURRENT_LIST_DIR}
    -P ${ZEPHYR_BASE}/cmake/util/execute_process.cmake
)

if(WIN32)
  set(SEP ;)
else()
  set(SEP :)
endif()

#
# Function to generate target for kconfig RST files
#
function(construct_kconfig_target scope)
  if(TARGET ${TGT_NAME_KCONFIG})
    return()
  endif()

  add_custom_target(
    ${TGT_NAME_KCONFIG}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RST_OUT}/doc/reference/kconfig
    COMMAND ${CMAKE_COMMAND} -E env
    PYTHONPATH="${ZEPHYR_BASE}/scripts/kconfig${SEP}$ENV{PYTHONPATH}"
    srctree=${ZEPHYR_BASE}
    KERNELVERSION=${KERNELVERSION}
    BOARD_DIR=boards/*/*/
    ARCH=*
    SOC_DIR=soc/
    SRCARCH=x86
    KCONFIG_TURBO_MODE=${KCONFIG_TURBO_MODE}
    ${PYTHON_EXECUTABLE} scripts/genrest.py Kconfig ${RST_OUT}/doc/reference/kconfig/
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  )
endfunction()

set(KI_SCRIPT ${ZEPHYR_BASE}/scripts/filter-known-issues.py)
set(KI_CONFIG_DIR ${ZEPHYR_BASE}/.known-issues/doc)
set(FIX_TEX_SCRIPT ${ZEPHYR_BASE}/doc/scripts/fix_tex.py)

#
# Function to generate a target with HTML output
#
function(construct_html_target scope)
  if(DEFINED SPHINX_OUTPUT_DIR)
    # SPHINX_OUTPUT_DIR is used to specify exactly where HTML (or other
    # outputs) are placed, so no /html, /latex, /pdf suffixes are needed.
    set(SPHINX_OUTPUT_DIR_HTML ${SPHINX_OUTPUT_DIR})
  else()
    if("${scope}" STREQUAL "")
      set(SPHINX_OUTPUT_DIR_HTML ${SPHINX_OUTPUT_DEFAULT_DIR}/html)
    else()
      set(SPHINX_OUTPUT_DIR_HTML ${SPHINX_OUTPUT_DEFAULT_DIR}/${scope}_html)
    endif()
  endif()

  set(TGT_NAME_HTML "${scope}html")
  set(TGT_NAME_HTML_DOCS "${TGT_NAME_HTML}docs")

  set(SPHINX_BUILD_HTML_COMMAND
    ${CMAKE_COMMAND} -E env
    ZEPHYR_BUILD=${CMAKE_CURRENT_BINARY_DIR}
    ${SPHINXBUILD} -w ${SPHINX_LOG} -N -t ${DOC_TAG} -b html ${SCOPE_TAG}
    -d ${CACHE_DIR} ${SPHINXOPTS} ${RST_OUT}/doc ${SPHINX_OUTPUT_DIR_HTML})

  # Add a custom target (e.g. html) to invoke sphinx-build
  add_custom_target(
    ${TGT_NAME_HTML}
    COMMAND ${SPHINX_BUILD_HTML_COMMAND}
    # Merge the Doxygen and Sphinx logs into a single file
    COMMAND ${CMAKE_COMMAND} -P ${ZEPHYR_BASE}/cmake/util/fmerge.cmake ${DOC_LOG} ${DOXY_LOG} ${SPHINX_LOG}
    COMMAND ${PYTHON_EXECUTABLE} ${KI_SCRIPT} --config-dir ${KI_CONFIG_DIR} --errors ${DOC_WARN} --warnings ${DOC_WARN} ${DOC_LOG}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMENT "Generating HTML documentation"
    ${SPHINX_USES_TERMINAL}
  )

  add_dependencies(${TGT_NAME_HTML} doxy ${TGT_NAME_CONTENT} ${TGT_NAME_KCONFIG})

  # Final build target (e.g. htmldocs):
  # Having an extra target allows steps to be added
  # between Sphinx and final target by adding
  # dependencies in between.
  add_custom_target(
    ${TGT_NAME_HTML_DOCS}
    DEPENDS ${TGT_NAME_HTML}
  )
endfunction()

#
# Function to generate a target with LaTeX and PDF output
#
function(construct_latex_pdf_target scope)
  if(DEFINED SPHINX_OUTPUT_DIR)
    # SPHINX_OUTPUT_DIR is used to specify exactly where HTML (or other
    # outputs) are placed, so no /html, /latex, /pdf suffixes are needed.
    set(SPHINX_OUTPUT_DIR_LATEX ${SPHINX_OUTPUT_DIR})
  else()
    if("${scope}" STREQUAL "")
      set(SPHINX_OUTPUT_DIR_LATEX ${SPHINX_OUTPUT_DEFAULT_DIR}/latex)
    else()
      set(SPHINX_OUTPUT_DIR_LATEX ${SPHINX_OUTPUT_DEFAULT_DIR}/${scope}_latex)
    endif()
  endif()

  if("${scope}" STREQUAL "")
    set(LATEXPDF_FILESTEM "zephyr")
  else()
    set(LATEXPDF_FILESTEM "${scope}")
  endif()

  set(TGT_NAME_LATEX "${scope}latex")
  set(TGT_NAME_LATEX_DOCS "${TGT_NAME_LATEX}docs")

  set(SPHINX_BUILD_LATEX_COMMAND
    ${CMAKE_COMMAND} -E env
    ZEPHYR_BUILD=${CMAKE_CURRENT_BINARY_DIR}
    ${SPHINXBUILD} -w ${SPHINX_LOG} -N -t ${DOC_TAG} -b latex -t svgconvert ${SCOPE_TAG}
    -d ${CACHE_DIR} ${SPHINXOPTS} ${RST_OUT}/doc ${SPHINX_OUTPUT_DIR_LATEX})

  add_custom_command(
    OUTPUT ${SPHINX_OUTPUT_DIR_LATEX}/${LATEXPDF_FILESTEM}.tex
    COMMAND ${SPHINX_BUILD_LATEX_COMMAND}
    # Merge the Doxygen and Sphinx logs into a single file
    COMMAND ${CMAKE_COMMAND} -P ${ZEPHYR_BASE}/cmake/util/fmerge.cmake ${DOC_LOG} ${DOXY_LOG} ${SPHINX_LOG}
    COMMAND ${PYTHON_EXECUTABLE} ${KI_SCRIPT} --config-dir ${KI_CONFIG_DIR} --errors ${DOC_WARN} --warnings ${DOC_WARN} ${DOC_LOG}
    COMMAND ${PYTHON_EXECUTABLE} ${FIX_TEX_SCRIPT} ${SPHINX_OUTPUT_DIR_LATEX}/${LATEXPDF_FILESTEM}.tex
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMENT "Generating LaTeX documentation"
    ${SPHINX_USES_TERMINAL}
  )

  add_custom_target(
    ${TGT_NAME_LATEX}
    DEPENDS ${SPHINX_OUTPUT_DIR_LATEX}/${LATEXPDF_FILESTEM}.tex
  )

  add_dependencies(${TGT_NAME_LATEX} doxy ${TGT_NAME_CONTENT} ${TGT_NAME_KCONFIG})

  # Final build target (e.g. htmldocs):
  # Having an extra target allows steps to be added
  # between Sphinx and final target by adding
  # dependencies in between.
  add_custom_target(
    ${TGT_NAME_LATEX_DOCS}
    DEPENDS ${TGT_NAME_LATEX}
  )

  if(NOT ${LATEXMK} STREQUAL LATEXMK-NOTFOUND)
    if(DEFINED SPHINX_OUTPUT_DIR)
      # SPHINX_OUTPUT_DIR is used to specify exactly where HTML (or other
      # outputs) are placed, so no /html, /latex, /pdf suffixes are needed.
      set(SPHINX_OUTPUT_DIR_PDF ${SPHINX_OUTPUT_DIR})
    else()
      set(SPHINX_OUTPUT_DIR_PDF ${SPHINX_OUTPUT_DEFAULT_DIR}/pdf)
    endif()

    set(TGT_NAME_PDF "${scope}pdf")
    set(TGT_NAME_PDF_DOCS "${TGT_NAME_PDF}docs")

    add_custom_command(
      OUTPUT ${SPHINX_OUTPUT_DIR_LATEX}/${LATEXPDF_FILESTEM}.pdf
      DEPENDS ${TGT_NAME_LATEX_DOCS} ${SPHINX_OUTPUT_DIR_LATEX}/${LATEXPDF_FILESTEM}.tex
      COMMAND ${CMAKE_COMMAND} -E env
      LATEXOPTS="-halt-on-error -no-shell-escape"
      ${LATEXMK} -quiet -pdf -dvi- -ps-
      WORKING_DIRECTORY ${SPHINX_OUTPUT_DIR_LATEX}
    )

    # Although latexmk allows specifying output directory,
    # makeindex fails if one is specified.
    # Hence the need of this to copy the PDF file over.
    add_custom_command(
      OUTPUT ${SPHINX_OUTPUT_DIR_PDF}/${LATEXPDF_FILESTEM}.pdf
      COMMAND ${CMAKE_COMMAND} -E make_directory ${SPHINX_OUTPUT_DIR_PDF}
      COMMAND ${CMAKE_COMMAND} -E copy ${SPHINX_OUTPUT_DIR_LATEX}/${LATEXPDF_FILESTEM}.pdf ${SPHINX_OUTPUT_DIR_PDF}/${LATEXPDF_FILESTEM}.pdf
      DEPENDS ${SPHINX_OUTPUT_DIR_LATEX}/${LATEXPDF_FILESTEM}.pdf
    )

    add_custom_target(
      ${TGT_NAME_PDF}
      DEPENDS ${SPHINX_OUTPUT_DIR_PDF}/${LATEXPDF_FILESTEM}.pdf
    )

    add_custom_target(
      ${TGT_NAME_PDF_DOCS}
      DEPENDS ${TGT_NAME_PDF}
    )
  endif()

endfunction()

#
# Function the necessary build targets for a certain output_format/scope
# combinations.
#
# Param: output_format: format to be generated (e.g. html or latexpdf).
#
# Param: scope: document scope to be generated (e.g. kernel).
#
# Param: has_custom_content_tgt:
#            FALSE: if use the default generated target;
#            TRUE:  if special steps are needed to generate content.
#                   A target named "${scope}content" will need to be defined manually.
#
# Param: has_custom_kconfig_tgt:
#            FALSE: if use the default generated target;
#            TRUE:  if special steps are needed to generate kconfig RST files.
#                   A target named "${scope}kconfig" will need to be defined manually.
#
function(construct_target output_format scope has_custom_content_tgt has_custom_kconfig_tgt)
  # Directory for RST files and supporing content
  if("${scope}" STREQUAL "")
    set(RST_OUT ${CMAKE_CURRENT_BINARY_DIR}/rst)
  else()
    set(RST_OUT ${CMAKE_CURRENT_BINARY_DIR}/${scope}_rst)
  endif()

  # Common targets used across multiple targets
  set(TGT_NAME_CONTENT "${scope}content")
  set(TGT_NAME_KCONFIG "${scope}kconfig")

  #
  # If a cached environment is built with one output format and used
  # subsequently for another output format (e.g. HTML then LaTeX),
  # the cached environment may be corrupted (e.g. app.env.found_docs
  # is different between builds even though nothing has changed).
  # This behvaior is not observed if the cached environment is being
  # used multiple time with the same format (with or without changes
  # to RST files). As a precaution, have different cached
  # environments for each output format.
  #
  if("${scope}" STREQUAL "")
    set(CACHE_DIR ${CMAKE_CURRENT_BINARY_DIR}/doctrees.${output_format})
  else()
    set(CACHE_DIR ${CMAKE_CURRENT_BINARY_DIR}/doctrees.${scope}.${output_format})
  endif()

  # Set tag for Sphinx
  if("${scope}" STREQUAL "")
    set(SCOPE_TAG "")
  else()
    set(SCOPE_TAG "-t" "${scope}")
  endif()

  # Custom content generation target?
  # Use the generic one if not.
  if(NOT ${has_custom_content_tgt})
    if(NOT TARGET ${TGT_NAME_CONTENT})
      construct_content_target("${scope}")
    endif()
  endif()

  # Custom kconfig RST generation target?
  # Use the generic one if not.
  if(NOT ${has_custom_kconfig_tgt})
    construct_kconfig_target("${scope}")
  endif()

  if("${output_format}" STREQUAL "html")
    construct_html_target("${scope}")
  elseif("${output_format}" STREQUAL "latexpdf")
    construct_latex_pdf_target("${scope}")
  else()
    message(FATAL_ERROR "output_format '${output_format}' not recognized")
  endif()
endfunction()

# HTML, LaTeX and PDF targets for whole document
construct_target("html" "" FALSE FALSE)
construct_target("latexpdf" "" FALSE FALSE)
