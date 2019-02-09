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
  NAMES sphinx-build-3 sphinx-build
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
set(RST_OUT ${CMAKE_CURRENT_BINARY_DIR}/rst)
set(DOC_LOG ${CMAKE_CURRENT_BINARY_DIR}/doc.log)
set(DOXY_LOG ${CMAKE_CURRENT_BINARY_DIR}/doxy.log)
set(SPHINX_LOG ${CMAKE_CURRENT_BINARY_DIR}/sphinx.log)
set(DOC_WARN ${CMAKE_CURRENT_BINARY_DIR}/doc.warnings)
set(CONTENT_OUTPUTS ${CMAKE_CURRENT_BINARY_DIR}/extracted-content.txt)

configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)
file(TOUCH ${CMAKE_CURRENT_BINARY_DIR}/Kconfig.modules)

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
  content
  # Copy all files in doc/ to the rst folder
  COMMAND ${EXTRACT_CONTENT_COMMAND}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

set(ARGS ${DOXYFILE_OUT})

add_custom_target(
  doxy
  COMMAND ${CMAKE_COMMAND}
    -DCOMMAND=${DOXYGEN_EXECUTABLE}
    -DARGS="${ARGS}"
    -DOUTPUT_FILE=${DOXY_LOG}
    -DERROR_FILE=${DOXY_LOG}
    -DWORKING_DIRECTORY=${CMAKE_CURRENT_LIST_DIR}
    -P ${ZEPHYR_BASE}/cmake/util/execute_process.cmake
)

add_custom_target(
  pristine
  COMMAND ${CMAKE_COMMAND} -P ${ZEPHYR_BASE}/cmake/pristine.cmake
)

if(WIN32)
  set(SEP ;)
else()
  set(SEP :)
endif()

add_custom_target(
  kconfig
  COMMAND ${CMAKE_COMMAND} -E make_directory ${RST_OUT}/doc/reference/kconfig
  COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH="${ZEPHYR_BASE}/scripts/kconfig${SEP}$ENV{PYTHONPATH}"
  srctree=${ZEPHYR_BASE}
  KERNELVERSION=${KERNELVERSION}
  BOARD_DIR=boards/*/*/
  ARCH=*
  ARCH_DIR=arch/
  SOC_DIR=soc/
  SRCARCH=x86
  PROJECT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
  KCONFIG_TURBO_MODE=${KCONFIG_TURBO_MODE}
  KCONFIG_DOC_MODE=1
  ${PYTHON_EXECUTABLE} scripts/genrest.py Kconfig ${RST_OUT}/doc/reference/kconfig/
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

set(KI_SCRIPT ${ZEPHYR_BASE}/scripts/filter-known-issues.py)
set(FIX_TEX_SCRIPT ${ZEPHYR_BASE}/doc/scripts/fix_tex.py)
set(CONFIG_DIR ${ZEPHYR_BASE}/.known-issues/doc)

#
# HTML section
#
set(SPHINX_BUILD_HTML_COMMAND
  ${CMAKE_COMMAND} -E env
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
  DEPENDS ${EXTRACT_CONTENT_OUTPUTS}
  COMMENT "Just re-generating HTML (USE WITH CAUTION)"
  USES_TERMINAL
)

add_custom_target(
  html
  COMMAND ${SPHINX_BUILD_HTML_COMMAND}
  # Merge the Doxygen and Sphinx logs into a single file
  COMMAND ${CMAKE_COMMAND} -P ${ZEPHYR_BASE}/cmake/util/fmerge.cmake ${DOC_LOG} ${DOXY_LOG} ${SPHINX_LOG}
  COMMAND ${PYTHON_EXECUTABLE} ${KI_SCRIPT} --config-dir ${CONFIG_DIR} --errors ${DOC_WARN} --warnings ${DOC_WARN} ${DOC_LOG}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMENT "Generating HTML documentation"
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
  DEPENDS ${EXTRACT_CONTENT_OUTPUTS}
  COMMENT "Just re-generating LaTeX (USE WITH CAUTION)"
  USES_TERMINAL
)

add_custom_command(
  OUTPUT ${SPHINX_OUTPUT_DIR_LATEX}/zephyr.tex
  COMMAND ${SPHINX_BUILD_LATEX_COMMAND}
  # Merge the Doxygen and Sphinx logs into a single file
  COMMAND ${CMAKE_COMMAND} -P ${ZEPHYR_BASE}/cmake/util/fmerge.cmake ${DOC_LOG} ${DOXY_LOG} ${SPHINX_LOG}
  COMMAND ${PYTHON_EXECUTABLE} ${KI_SCRIPT} --config-dir ${CONFIG_DIR} --errors ${DOC_WARN} --warnings ${DOC_WARN} ${DOC_LOG}
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
add_dependencies(html content doxy kconfig)

add_custom_target(
  htmldocs
)
add_dependencies(htmldocs html)

add_dependencies(latex content doxy kconfig)

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
