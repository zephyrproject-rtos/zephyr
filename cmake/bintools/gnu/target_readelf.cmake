# SPDX-License-Identifier: Apache-2.0

# Construct a commandline suitable for calling the toolchain binary tools
#  version of readelf.
#
#  Usage:
#    bintools_readelf(
#      RESULT_CMD_LIST    <List of commands to be executed, usually after build>
#      RESULT_BYPROD_LIST <List of command output byproducts>
#
#      HEADERS            <Display all the headers in the input file>
#
#      FILE_INPUT         <The input file>
#      FILE_OUTPUT        <The output file>
#    )
function(bintools_readelf)
  cmake_parse_arguments(
    # Prefix of output variables
    BINTOOLS_READELF
    # List of argument names without values, hence boolean
    "HEADERS"
    # List of argument names with one value
    "RESULT_CMD_LIST;RESULT_BYPROD_LIST;FILE_INPUT;FILE_OUTPUT"
    # List of argument names with multible values
    ""
    # Parser input
    ${ARGN}
  )

  # Verify arguments
  if(NOT DEFINED BINTOOLS_READELF_RESULT_CMD_LIST OR NOT DEFINED ${BINTOOLS_READELF_RESULT_CMD_LIST})
    message(FATAL_ERROR "RESULT_CMD_LIST is required.")
  elseif(NOT DEFINED BINTOOLS_READELF_FILE_INPUT)
    message(FATAL_ERROR "FILE_INPUT is required.")
  endif()

  # Handle headers
  set(readelf_headers "")
  if(${BINTOOLS_READELF_HEADERS})
    set(readelf_headers "-e") # --headers
  endif()

  # Handle output
  set(readelf_output "")
  if(DEFINED BINTOOLS_READELF_FILE_OUTPUT)
    set(readelf_output > ${BINTOOLS_READELF_FILE_OUTPUT})
  endif()

  # Construct the command
  set(readelf_cmd
    # Base command
    COMMAND ${CMAKE_READELF} ${readelf_headers}
    # Input and Output
    ${BINTOOLS_READELF_FILE_INPUT} ${readelf_output}
  )

  # Place command in the parent provided variable
  set(${BINTOOLS_READELF_RESULT_CMD_LIST} ${readelf_cmd} PARENT_SCOPE)

endfunction(bintools_readelf)
