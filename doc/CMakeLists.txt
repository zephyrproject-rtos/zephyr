cmake_minimum_required(VERSION 3.8.2)
project(Zephyr-Kernel-Doc VERSION ${PROJECT_VERSION} LANGUAGES)

set(ZEPHYR_BASE $ENV{ZEPHYR_BASE})

message(STATUS "Zephyr base: ${ZEPHYR_BASE}")

find_package(PythonInterp 3.4)
set(DOXYGEN_SKIP_DOT True)
find_package(Doxygen REQUIRED)

find_program(
  SPHINXBUILD
  sphinx-build
  )
if(${SPHINXBUILD} STREQUAL SPHINXBUILD-NOTFOUND)
  message(FATAL_ERROR "The 'sphinx-build' command was not found. Make sure you have Sphinx installed.")
endif()

if(NOT DEFINED SPHINXOPTS)
  set(SPHINXOPTS -q)
else()
  separate_arguments(SPHINXOPTS)
endif()

if(NOT DEFINED SPHINX_OUTPUT_DIR)
  set(SPHINX_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/html
)
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

configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

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

add_custom_target(
  content
  # Copy all files in doc/ to the rst folder
  COMMAND ${CMAKE_COMMAND} -E env
  ZEPHYR_BUILD=${CMAKE_CURRENT_BINARY_DIR}
  ${PYTHON_EXECUTABLE} scripts/extract_content.py -a ${RST_OUT} doc
  # Copy the .rst files in samples/ and boards/ to the rst folder
  COMMAND ${CMAKE_COMMAND} -E env
  ZEPHYR_BUILD=${CMAKE_CURRENT_BINARY_DIR}
  ${PYTHON_EXECUTABLE} scripts/extract_content.py ${RST_OUT} samples boards
  # Copy the .rst files in samples/ and boards/ to the doc folder inside rst
  COMMAND ${CMAKE_COMMAND} -E env
  ZEPHYR_BUILD=${CMAKE_CURRENT_BINARY_DIR}
  ${PYTHON_EXECUTABLE} scripts/extract_content.py ${RST_OUT}/doc samples boards
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
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
  ENV_VAR_BOARD_DIR=boards/*/*/
  ENV_VAR_ARCH=*
  KERNELVERSION=${PROJECT_VERSION}
  SRCARCH=x86
  ${PYTHON_EXECUTABLE} scripts/genrest.py Kconfig ${RST_OUT}/doc/reference/kconfig/
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

set(KI_SCRIPT ${ZEPHYR_BASE}/scripts/filter-known-issues.py)
set(CONFIG_DIR ${ZEPHYR_BASE}/.known-issues/doc)

add_custom_target(
  html
  COMMAND ${CMAKE_COMMAND} -E env
  ZEPHYR_BUILD=${CMAKE_CURRENT_BINARY_DIR}
  ${SPHINXBUILD} -w ${SPHINX_LOG} -N -t ${DOC_TAG} -b html ${ALLSPHINXOPTS} ${RST_OUT}/doc ${SPHINX_OUTPUT_DIR}
  # Merge the Doxygen and Sphinx logs into a single file
  COMMAND ${CMAKE_COMMAND} -P ${ZEPHYR_BASE}/cmake/util/fmerge.cmake ${DOC_LOG} ${DOXY_LOG} ${SPHINX_LOG}
  COMMAND ${PYTHON_EXECUTABLE} ${KI_SCRIPT} --config-dir ${CONFIG_DIR} --errors ${DOC_WARN} --warnings ${DOC_WARN} ${DOC_LOG}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  ${SPHINX_USES_TERMINAL}
)

add_dependencies(html content doxy kconfig)

add_custom_target(
  htmldocs
)
add_dependencies(htmldocs html)

