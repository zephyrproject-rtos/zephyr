# SPDX-License-Identifier: Apache-2.0

find_package(Git QUIET)

# Usage:
#   git_describe(<dir> <output>)
#
# Helper function to run `git describe --abbrev=12 --always` in a directory.
# OUTPUT is set to the output of the command.
#
function(git_describe dir output)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} describe --abbrev=12 --always
      WORKING_DIRECTORY                ${dir}
      OUTPUT_VARIABLE                  description
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
      set(${output} ${description} PARENT_SCOPE)
    endif()
  endif()
endfunction()
