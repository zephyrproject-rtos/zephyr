# SPDX-License-Identifier: Apache-2.0

# NB: This could be dangerous to execute, it is assuming the user is
# checking that the build is out-of-source with code like this:
#
# if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
#   message(FATAL_ERROR "Source directory equals build directory.\
#  In-source builds are not supported.\
#  Please specify a build directory, e.g. cmake -Bbuild -H.")
# endif()

file(GLOB build_dir_contents ${CMAKE_BINARY_DIR}/*)
foreach(file ${build_dir_contents})
  if (EXISTS ${file})
     file(REMOVE_RECURSE ${file})
  endif()
endforeach(file)
