# SPDX-License-Identifier: Apache-2.0

# NB: This could be dangerous to execute.

macro(print_usage)
  message("
usage: cmake -DBINARY_DIR=<build-path> -DSOURCE_DIR=<source-path>
             -P ${CMAKE_SCRIPT_MODE_FILE}

mandatory arguments:
  -DBINARY_DIR=<build-path>:  Absolute path to the build directory to pristine
  -DSOURCE_DIR=<source-path>: Absolute path to the source directory used when
                              creating <build-path>
")
  # Making the usage itself a fatal error messes up the formatting when printing.
  message(FATAL_ERROR "")
endmacro()

if(NOT DEFINED BINARY_DIR OR NOT DEFINED SOURCE_DIR)
  print_usage()
endif()

if(NOT IS_ABSOLUTE ${BINARY_DIR} OR NOT IS_ABSOLUTE ${SOURCE_DIR})
  print_usage()
endif()

get_filename_component(BINARY_DIR ${BINARY_DIR} REALPATH)
get_filename_component(SOURCE_DIR ${SOURCE_DIR} REALPATH)

string(FIND ${SOURCE_DIR} ${BINARY_DIR} INDEX)
if(NOT INDEX EQUAL -1)
  message(FATAL_ERROR "Refusing to run pristine in in-source build folder.")
endif()

file(GLOB build_dir_contents ${BINARY_DIR}/*)
foreach(file ${build_dir_contents})
  if (EXISTS ${file})
     file(REMOVE_RECURSE ${file})
  endif()
endforeach(file)
