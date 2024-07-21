# SPDX-License-Identifier: Apache-2.0

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

# Usage:
#   git_commit_hash(<dir> <output>)
#
# Helper function to extract the GIT commit hash associated with a directory.
# OUTPUT is set to the output of `git rev-parse HEAD` as run from DIR.
#
function(git_commit_hash DIR OUTPUT)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
      WORKING_DIRECTORY                ${DIR}
      OUTPUT_VARIABLE                  COMMIT_HASH
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_STRIP_TRAILING_WHITESPACE
      ERROR_VARIABLE                   stderr
      RESULT_VARIABLE                  return_code
    )
    if(return_code)
      message(WARNING "git rev-parse failed: ${stderr}")
    elseif(NOT "${stderr}" STREQUAL "")
      message(WARNING "git rev-parse warned: ${stderr}")
    else()
      # Save output
      set(${OUTPUT} ${COMMIT_HASH} PARENT_SCOPE)
    endif()
  endif()
endfunction()

# Usage:
#   git_commit_hash_short(<full_hash> <short_hash> <short_hash_int>)
#
# Helper function to truncate a full GIT commit hash to a value that
# fits in a uint32_t. SHORT_HASH is set to the first 8 characters of
# FULL_HASH, while SHORT_HASH_INT is the decimal value equivalent of
# 0x${SHORT_HASH}
#
function(git_commit_hash_short FULL_HASH SHORT_HASH SHORT_HASH_INT)
  # Extract first 8 characters of hash
  string(SUBSTRING ${FULL_HASH} 0 8 SHORT_STR)
  # Convert to a decimal value
  math(EXPR SHORT_INT "0x${SHORT_STR}" OUTPUT_FORMAT DECIMAL)
  # Save output
  set(${SHORT_HASH} ${SHORT_STR} PARENT_SCOPE)
  set(${SHORT_HASH_INT} ${SHORT_INT} PARENT_SCOPE)
endfunction()
