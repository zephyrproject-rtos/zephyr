# SPDX-License-Identifier: Apache-2.0

#[=======================================================================[.rst:
Git
***

Provides Git-related functionality for the Zephyr build system.

This module provides functions for interacting with Git repositories
within the Zephyr build system.

Commands
========

.. cmake:command:: git_describe

   Get a short Git description associated with a directory.

   .. code-block:: cmake

     git_describe(DIR OUTPUT)

   This function runs ``git describe --abbrev=12 --always`` in the specified
   directory and stores the result in the provided output variable.

   ``DIR``
     The directory to run the git command in.

   ``OUTPUT``
     The variable name where the git description will be stored.
     If the git command fails, this variable will not be set.

   The function will output status messages if:

   * The :command:`git` command fails (error message)
   * The :command:`git` command produces warnings (warning message)

Example Usage
-------------

.. code-block:: cmake

   include(git)

   git_describe(${CMAKE_CURRENT_SOURCE_DIR} GIT_DESCRIPTION)
   if(DEFINED GIT_DESCRIPTION)
     message(STATUS "Git description: ${GIT_DESCRIPTION}")
   endif()

#]=======================================================================]

include_guard(GLOBAL)

find_package(Git QUIET)

# Usage:
#   git_describe(<dir> <output>)
#
# Helper function to get a short GIT desciption associated with a directory.
# OUTPUT is set to the output of `git describe --abbrev=12 --always` as run
# from DIR.
#
function(git_describe DIR OUTPUT)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} describe --abbrev=12 --always
      WORKING_DIRECTORY                ${DIR}
      OUTPUT_VARIABLE                  DESCRIPTION
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_STRIP_TRAILING_WHITESPACE
      ERROR_VARIABLE                   stderr
      RESULT_VARIABLE                  return_code
    )
    if(return_code)
      message(STATUS "git describe failed: ${stderr}")
    elseif(NOT "${stderr}" STREQUAL "")
      message(STATUS "git describe warned: ${stderr}")
    else()
      # Save output
      set(${OUTPUT} ${DESCRIPTION} PARENT_SCOPE)
    endif()
  endif()
endfunction()
