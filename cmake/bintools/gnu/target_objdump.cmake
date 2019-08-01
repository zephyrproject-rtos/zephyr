# SPDX-License-Identifier: Apache-2.0

# Construct a commandline suitable for calling the toolchain binary tools
#  version of objdump.
#
#  Usage:
#    bintools_objdump(
#      RESULT_CMD_LIST    <List of commands to be executed, usually after build>
#      RESULT_BYPROD_LIST <List of command output byproducts>
#
#      DISASSEMBLE        <Display the assembler mnemonics for the machine instructions from input>
#      DISASSEMBLE_SOURCE < Display source code intermixed with disassembly, if possible>
#
#      FILE_INPUT         <The input file>
#      FILE_OUTPUT        <The output file>
#    )
function(bintools_objdump)
  cmake_parse_arguments(
    # Prefix of output variables
    BINTOOLS_OBJDUMP
    # List of argument names without values, hence boolean
    "DISASSEMBLE;DISASSEMBLE_SOURCE"
    # List of argument names with one value
    "RESULT_CMD_LIST;RESULT_BYPROD_LIST;FILE_INPUT;FILE_OUTPUT"
    # List of argument names with multible values
    ""
    # Parser input
    ${ARGN}
  )

  # Verify arguments
  if(NOT DEFINED BINTOOLS_OBJDUMP_RESULT_CMD_LIST OR NOT DEFINED ${BINTOOLS_OBJDUMP_RESULT_CMD_LIST})
    message(FATAL_ERROR "RESULT_CMD_LIST is required.")
  elseif(NOT DEFINED BINTOOLS_OBJDUMP_FILE_INPUT)
    message(FATAL_ERROR "FILE_INPUT is required.")
  endif()

  # Handle disassembly
  set(obj_dump_disassemble "")
  if(${BINTOOLS_OBJDUMP_DISASSEMBLE_SOURCE})
    set(obj_dump_disassemble "-S") # --source
  elseif(${BINTOOLS_OBJDUMP_DISASSEMBLE})
    set(obj_dump_disassemble "-d") # --disassemble
  endif()

  # Handle output
  set(obj_dump_output "")
  if(DEFINED BINTOOLS_OBJDUMP_FILE_OUTPUT)
    set(obj_dump_output > ${BINTOOLS_OBJDUMP_FILE_OUTPUT})
  endif()

  # Construct the command
  set(obj_dump_cmd
    # Base command
    COMMAND ${CMAKE_OBJDUMP} ${obj_dump_disassemble}
    # Input and Output
    ${BINTOOLS_OBJDUMP_FILE_INPUT} ${obj_dump_output}
  )

  # Place command in the parent provided variable
  set(${BINTOOLS_OBJDUMP_RESULT_CMD_LIST} ${obj_dump_cmd} PARENT_SCOPE)

endfunction(bintools_objdump)
