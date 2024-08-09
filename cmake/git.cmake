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

# Usage:
#   git_commit_hash(<dir> <output>)
#
# Helper function to extract the full 160bit git commit hash associated with a directory.
# output is set to the output of `git rev-parse --short=40 HEAD` as run from dir.
#
function(git_commit_hash dir output)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --short=40 HEAD
      WORKING_DIRECTORY                ${dir}
      OUTPUT_VARIABLE                  commit_hash
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
      set(${output} ${commit_hash} PARENT_SCOPE)
    endif()
  endif()
endfunction()

# Usage:
#   git_commit_hash_short(<full_hash> <short_hash> <short_hash_int>)
#
# Helper function to truncate a full GIT commit hash to a value that
# fits in a uint32_t. short_hash_str is set to the first 8 characters of
# full_hash_str, while short_hash_uint32 is the decimal value equivalent of
# 0x${short_hash_str}
#
function(git_commit_hash_short full_hash_str short_hash_str short_hash_uint32)
  # Input validation
  string(LENGTH "${full_hash_str}" full_hash_str_len)
  if(full_hash_str_len LESS 8)
    message(FATAL_ERROR "Expected ${full_hash_str} to be at least 8 characters")
  endif()
  if(NOT "${full_hash_str}" MATCHES "^[0-9a-fA-F]+$")
    message(FATAL_ERROR "Expected ${full_hash_str} to be hexadecimal string")
  endif()
  # Extract first 8 characters of hash
  string(SUBSTRING ${full_hash_str} 0 8 short_str)
  # Convert to a decimal value
  math(EXPR short_int "0x${short_str}" OUTPUT_FORMAT DECIMAL)
  # Save output
  set(${short_hash_str} ${short_str} PARENT_SCOPE)
  set(${short_hash_uint32} ${short_int} PARENT_SCOPE)
endfunction()
