# SPDX-License-Identifier: Apache-2.0

# Construct a commandline suitable for calling the toolchain binary tools
#  version of strip.
#
#  Usage:
#    bintools_strip(
#      RESULT_CMD_LIST    <List of commands to be executed, usually after build>
#      RESULT_BYPROD_LIST <List of command output byproducts>
#
#      STRIP_ALL          <When present, remove relocation and symbol info>
#      STRIP_DEBUG        <When present, remove debugging symbols and sections>
#      STRIP_DWO          <When present, remove DWARF .dwo sections>
#
#      FILE_INPUT         <The input file>
#      FILE_OUTPUT        <The output file>
#    )
function(bintools_strip)
  cmake_parse_arguments(
    # Prefix of output variables
    BINTOOLS_STRIP
    # List of argument names without values, hence boolean
    "STRIP_ALL;STRIP_DEBUG;STRIP_DWO"
    # List of argument names with one value
    "RESULT_CMD_LIST;RESULT_BYPROD_LIST;FILE_INPUT;FILE_OUTPUT"
    # List of argument names with multible values
    ""
    # Parser input
    ${ARGN}
  )

  if(NOT DEFINED BINTOOLS_STRIP_RESULT_CMD_LIST OR NOT DEFINED ${BINTOOLS_STRIP_RESULT_CMD_LIST})
    message(FATAL_ERROR "RESULT_CMD_LIST is required.")
  elseif(NOT DEFINED BINTOOLS_STRIP_FILE_INPUT OR NOT DEFINED BINTOOLS_STRIP_FILE_OUTPUT)
    message(FATAL_ERROR "Both FILE_INPUT and FILE_OUTPUT are required.")
  endif()

  # Handle stripping
  set(strip_what "")
  if(${BINTOOLS_STRIP_STRIP_ALL})
    set(strip_what "--strip-all")
  elseif(${BINTOOLS_STRIP_STRIP_DEBUG})
    set(strip_what "--strip-debug")
  elseif(${BINTOOLS_STRIP_STRIP_DWO})
    set(strip_what "--strip-dwo")
  endif()

  # Handle output
  set(strip_output -o ${BINTOOLS_STRIP_FILE_OUTPUT})

  # Construct the command
  set(strip_cmd
    # Base command
    COMMAND ${CMAKE_STRIP} ${strip_what}
    # Input and Output
    ${BINTOOLS_STRIP_FILE_INPUT} ${strip_output}
  )

  # Place command in the parent provided variable
  set(${BINTOOLS_STRIP_RESULT_CMD_LIST} ${strip_cmd} PARENT_SCOPE)

endfunction(bintools_strip)
