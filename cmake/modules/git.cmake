# SPDX-License-Identifier: Apache-2.0

#[=======================================================================[.rst:
git
***

Provides Git-related functionality for the Zephyr build system.

This module provides functions for interacting with Git repositories
within the Zephyr build system.

Commands
========

.. cmake:command:: git_describe

   Get a short Git description associated with a directory or file.

   .. code-block:: cmake

     git_describe(<file|dir> OUTPUT)

   This function runs ``git describe --abbrev=12 --always`` on the provided
   directory, or if provided a file, the directory containing that file.
   In both cases, the result is stored in the provided output variable.

   ``file|dir``
     The directory or file to run the git command for.

   ``OUTPUT``
     The variable name where the git description will be stored.
     If the git command fails or Git is not found, this variable
     will not be set.

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
#   git_describe(<file|dir> <output>)
#
# Helper function to get a short GIT description associated with a directory
# or file. OUTPUT is set to the output of `git describe --abbrev=12 --always`
# as run from the provided file or directory.
#
function(git_describe ARG OUTPUT)
  if(IS_DIRECTORY "${ARG}")
    set(dir "${ARG}")
  else()
    cmake_path(GET ARG PARENT_PATH dir)
  endif()

  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} describe --abbrev=12 --always
      WORKING_DIRECTORY                ${dir}
      OUTPUT_VARIABLE                  DESCRIPTION
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_STRIP_TRAILING_WHITESPACE
      ERROR_VARIABLE                   stderr
      RESULT_VARIABLE                  return_code
    )
    if(return_code)
      message(STATUS "git describe failed in ${dir}: ${stderr}")
    elseif(NOT "${stderr}" STREQUAL "")
      message(STATUS "git describe warned: ${stderr}")
    else()
      # Save output
      set(${OUTPUT} ${DESCRIPTION} PARENT_SCOPE)
    endif()
  endif()
endfunction()
