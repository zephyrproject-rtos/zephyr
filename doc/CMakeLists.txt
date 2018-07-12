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
endif()

if(NOT DEFINED DOC_TAG)
  set(DOC_TAG development)
endif()

# Internal variables.
set(ALLSPHINXOPTS  -d ${CMAKE_BINARY_DIR}/doctrees ${SPHINXOPTS})
# the i18n builder cannot share the environment and doctrees with the others
set(I18NSPHINXOPTS  ${SPHINXOPTS} .)

set(DOXYFILE ${CMAKE_CURRENT_SOURCE_DIR}/zephyr.doxyfile)
set(BUILD_DOXYFILE ${CMAKE_BINARY_DIR}/build.doxyfile)
set(DOC_LOG doc.log)
set(DOXY_LOG doxy.log)
set(SPHINX_LOG sphinx.log)
set(DOC_WARN doc.warnings)

configure_file(${DOXYFILE} ${BUILD_DOXYFILE} COPYONLY)
file(APPEND ${BUILD_DOXYFILE} "STRIP_FROM_PATH=${ZEPHYR_BASE}\n")

set(ARGS ${BUILD_DOXYFILE})

add_custom_target(
  doxy
  COMMAND ${CMAKE_COMMAND}
    -DCOMMAND=${DOXYGEN_EXECUTABLE}
    -DARGS="${ARGS}"
    -DOUTPUT_FILE=${DOXY_LOG}
    -DERROR_FILE=${DOXY_LOG}
    -DWORKING_DIRECTORY=${CMAKE_CURRENT_SOURCE_DIR}
    -P ${ZEPHYR_BASE}/cmake/util/execute_process.cmake
)

add_custom_target(
  pristine
  COMMAND ${CMAKE_COMMAND} -P ${ZEPHYR_BASE}/cmake/pristine.cmake
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/html
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/xml
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/doxygen
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/latex
  COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_CURRENT_SOURCE_DIR}/reference/kconfig/*.rst
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/samples
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/boards
  COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_CURRENT_SOURCE_DIR}/${DOXY_LOG}
  COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_CURRENT_SOURCE_DIR}/${SPHINX_LOG}
  COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_LOG}
)

add_custom_target(
  htmldocs
  DEPENDS doxy html
)

add_custom_target(
  content
  COMMAND ${PYTHON_EXECUTABLE} scripts/extract_content.py
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WIN32)
  set(SEP ;)
else()
  set(SEP :)
endif()

add_custom_target(
  kconfig
  ${CMAKE_COMMAND} -E env
  PYTHONPATH="${ZEPHYR_BASE}/scripts/kconfig${SEP}$ENV{PYTHONPATH}"
  srctree=${ZEPHYR_BASE}
  ENV_VAR_BOARD_DIR=boards/*/*/
  ENV_VAR_ARCH=*
  KERNELVERSION=${PROJECT_VERSION}
  SRCARCH=x86
  ${PYTHON_EXECUTABLE} scripts/genrest.py ../Kconfig reference/kconfig/
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

set(KI_SCRIPT ${ZEPHYR_BASE}/scripts/filter-known-issues.py)
set(CONFIG_DIR ${ZEPHYR_BASE}/.known-issues/doc)

add_custom_target(
  html
  COMMAND ${SPHINXBUILD} -w ${SPHINX_LOG} -N -t ${DOC_TAG} -b html ${ALLSPHINXOPTS} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR}/html
  # Merge the Doxygen and Sphinx logs into a single file
  COMMAND ${CMAKE_COMMAND} -P ${ZEPHYR_BASE}/cmake/util/fmerge.cmake ${DOC_LOG} ${DOXY_LOG} ${SPHINX_LOG}
  COMMAND ${PYTHON_EXECUTABLE} ${KI_SCRIPT} --config-dir ${CONFIG_DIR} --errors ${DOC_WARN} --warnings ${DOC_WARN} ${DOC_LOG}
  COMMAND ${CMAKE_COMMAND} -E remove_directory samples
  COMMAND ${CMAKE_COMMAND} -E remove_directory boards
  DEPENDS doxy content kconfig
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
