# SPDX-License-Identifier: Apache-2.0

# Construct a commandline suitable for calling the toolchain binary tools
#  version of objcopy.
#
#  Usage:
#    bintools_objcopy(
#      RESULT_CMD_LIST    <List of commands to be executed, usually after build>
#      RESULT_BYPROD_LIST <List of command output byproducts>
#
#      STRIP_ALL          <When present, remove relocation and symbol info>
#      STRIP_DEBUG        <When present, remove debugging symbols and sections>
#
#      TARGET_INPUT       <Input file format type>
#      TARGET_OUTPUT      <Output file format type>
#
#      GAP_FILL           <Value used for gap fill, empty or not set, no gap fill>
#      SREC_LEN           <For srec format only, max length of the records>
#
#      SECTION_ONLY       <One or more section names to be included>
#      SECTION_REMOVE     <One or more section names to be excluded>
#      SECTION_RENAME     <One or more section names to be renamed 'from=to'>
#
#      FILE_INPUT         <The input file>
#      FILE_OUTPUT        <The output file>
#    )
function(bintools_objcopy)
  cmake_parse_arguments(
    # Prefix of output variables
    BINTOOLS_OBJCOPY
    # List of argument names without values, hence boolean
    "STRIP_ALL;STRIP_DEBUG"
    # List of argument names with one value
    "RESULT_CMD_LIST;RESULT_BYPROD_LIST;TARGET_INPUT;TARGET_OUTPUT;GAP_FILL;SREC_LEN;FILE_INPUT;FILE_OUTPUT"
    # List of argument names with multible values
    "SECTION_ONLY;SECTION_RENAME;SECTION_REMOVE"
    # Parser input
    ${ARGN}
  )

  # Verify arguments
  if(NOT DEFINED BINTOOLS_OBJCOPY_RESULT_CMD_LIST OR NOT DEFINED ${BINTOOLS_OBJCOPY_RESULT_CMD_LIST})
    message(FATAL_ERROR "RESULT_CMD_LIST is required.")
  elseif(NOT DEFINED BINTOOLS_OBJCOPY_FILE_INPUT OR NOT DEFINED BINTOOLS_OBJCOPY_FILE_OUTPUT)
    message(FATAL_ERROR "Both FILE_INPUT and FILE_OUTPUT is required.")
  endif()

  # Handle stripping
  set(obj_copy_strip "")
  if(${BINTOOLS_OBJCOPY_STRIP_ALL})
    set(obj_copy_strip "-S")
  elseif(${BINTOOLS_OBJCOPY_STRIP_DEBUG})
    set(obj_copy_strip "-g")
  endif()

  # Handle gap filling
  set(obj_copy_gap_fill "")
  if(DEFINED BINTOOLS_OBJCOPY_GAP_FILL)
    set(obj_copy_gap_fill "--gap-fill;${BINTOOLS_OBJCOPY_GAP_FILL}")
  endif()

  # Handle srec len, but only if target output is srec
  set(obj_copy_srec_len "")
  if(DEFINED BINTOOLS_OBJCOPY_SREC_LEN)
    if(NOT ${BINTOOLS_OBJCOPY_TARGET_OUTPUT} STREQUAL "srec")
      message(WARNING "Ignoring srec len, for non srec target: ${BINTOOLS_OBJCOPY_TARGET_OUTPUT}")
    else()
      set(obj_copy_srec_len "--srec-len;${BINTOOLS_OBJCOPY_SREC_LEN}")
    endif()
  endif()

  # Handle Input and Output target types
  set(obj_copy_target_input "")
  if(DEFINED BINTOOLS_OBJCOPY_TARGET_INPUT)
    set(obj_copy_target_input "--input-target=${BINTOOLS_OBJCOPY_TARGET_INPUT}")
  endif()
  set(obj_copy_target_output "")
  if(DEFINED BINTOOLS_OBJCOPY_TARGET_OUTPUT)
    set(obj_copy_target_output "--output-target=${BINTOOLS_OBJCOPY_TARGET_OUTPUT}")
  endif()

  # Handle sections, if any
  # 1. Section only selection(s)
  set(obj_copy_sections_only "")
  if(DEFINED BINTOOLS_OBJCOPY_SECTION_ONLY)
    foreach(section_only ${BINTOOLS_OBJCOPY_SECTION_ONLY})
      list(APPEND obj_copy_sections_only "--only-section=${section_only}")
    endforeach()
  endif()

  # 2. Section rename selection(s)
  set(obj_copy_sections_rename "")
  if(DEFINED BINTOOLS_OBJCOPY_SECTION_RENAME)
    foreach(section_rename ${BINTOOLS_OBJCOPY_SECTION_RENAME})
      if(NOT ${section_rename} MATCHES "^.*=.*$")
        message(FATAL_ERROR "Malformed section renaming. Must be from=to, have ${section_rename}")
      else()
        list(APPEND obj_copy_sections_rename "--rename-section;${section_rename}")
      endif()
    endforeach()
  endif()

  # 3. Section remove selection(s)
  set(obj_copy_sections_remove "")
  if(DEFINED BINTOOLS_OBJCOPY_SECTION_REMOVE)
    foreach(section_remove ${BINTOOLS_OBJCOPY_SECTION_REMOVE})
      list(APPEND obj_copy_sections_remove "--remove-section=${section_remove}")
    endforeach()
  endif()

  # Construct the command
  set(obj_copy_cmd
    # Base command
    COMMAND ${CMAKE_OBJCOPY} ${obj_copy_strip} ${obj_copy_gap_fill} ${obj_copy_srec_len}
    # Input and Output target types
    ${obj_copy_target_input} ${obj_copy_target_output}
    # Sections
    ${obj_copy_sections_only} ${obj_copy_sections_rename} ${obj_copy_sections_remove}
    # Input and output files
    ${BINTOOLS_OBJCOPY_FILE_INPUT} ${BINTOOLS_OBJCOPY_FILE_OUTPUT}
  )

  # Place command in the parent provided variable
  set(${BINTOOLS_OBJCOPY_RESULT_CMD_LIST} ${obj_copy_cmd} PARENT_SCOPE)

endfunction(bintools_objcopy)
