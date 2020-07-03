# SPDX-License-Identifier: Apache-2.0

# Configures binary tools as mwdt binutils

find_program(CMAKE_ELF2BIN ${CROSS_COMPILE}elf2bin   PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_OBJDUMP ${CROSS_COMPILE}elfdumpac PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_AS      ${CROSS_COMPILE}ccac      PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_AR      ${CROSS_COMPILE}arac      PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_RANLIB  ${CROSS_COMPILE}arac      PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_READELF ${CROSS_COMPILE}elfdumpac PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_NM      ${CROSS_COMPILE}nmac      PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_STRIP   ${CROSS_COMPILE}stripac   PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_SIZE    ${CROSS_COMPILE}sizeac    PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
find_program(CMAKE_ELF2HEX ${CROSS_COMPILE}elf2hex   PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)

SET(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> -rq <TARGET> <LINK_FLAGS> <OBJECTS>")
SET(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> -rq <TARGET> <LINK_FLAGS> <OBJECTS>")
SET(CMAKE_CXX_ARCHIVE_FINISH "<CMAKE_AR> -sq <TARGET>")
SET(CMAKE_C_ARCHIVE_FINISH "<CMAKE_AR> -sq <TARGET>")

find_program(CMAKE_GDB     ${CROSS_COMPILE}mdb     PATH ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)


# MWDT binutils don't support required features like section renameing, so we
# temporarily had to use GNU objcopy instead
find_program(CMAKE_OBJCOPY arc-elf32-objcopy)
if (NOT CMAKE_OBJCOPY)
  find_program(CMAKE_OBJCOPY arc-linux-objcopy)
endif()

if (NOT CMAKE_OBJCOPY)
  find_program(CMAKE_OBJCOPY objcopy)
endif()

if(NOT CMAKE_OBJCOPY)
  message(FATAL_ERROR "Zephyr unable to find any GNU objcopy (ARC or host one)")
endif()

# Add and/or prepare print of memory usage report
#
# Usage:
#   bintools_print_mem_usage(
#     RESULT_CMD_LIST    <List of commands to be executed, usually after build>
#     RESULT_BYPROD_LIST <List of command output byproducts>
#   )
#
function(bintools_print_mem_usage)
  cmake_parse_arguments(
    # Prefix of output variables
    BINTOOLS_MEMUSAGE
    ""
    # List of argument names with one value
    "RESULT_CMD_LIST;RESULT_BYPROD_LIST"
    ""
    # Parser input
    ${ARGN}
  )

  # Verify arguments
  if(NOT DEFINED BINTOOLS_MEMUSAGE_RESULT_CMD_LIST OR NOT DEFINED ${BINTOOLS_MEMUSAGE_RESULT_CMD_LIST})
    message(FATAL_ERROR "RESULT_CMD_LIST is required.")
  endif()

  # no copy right msg + gnu format
  set(memusage_args "-gq")

  # Construct the command
  set(memusage_cmd
    # Base command
    COMMAND ${CMAKE_SIZE} ${memusage_args}
    ${KERNEL_ELF_NAME}
  )

  # Place command in the parent provided variable
  set(${BINTOOLS_MEMUSAGE_RESULT_CMD_LIST} ${memusage_cmd} PARENT_SCOPE)

endfunction()


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
  if (DEFINED BINTOOLS_OBJCOPY_STRIP OR DEFINED BINTOOLS_OBJCOPY_STRIP_ALL)
    if(${BINTOOLS_OBJCOPY_STRIP_ALL})
      set(obj_copy_strip "-qs")
    elseif(${BINTOOLS_OBJCOPY_STRIP_DEBUG})
      set(obj_copy_strip "-ql")
    endif()
    set(obj_copy_cmd
        COMMAND ${CMAKE_STRIP} ${obj_copy_strip}
        ${BINTOOLS_OBJCOPY_FILE_INPUT} ${BINTOOLS_OBJCOPY_FILE_OUTPUT})
  endif()

  # no support of --srec-len in mwdt

  # Handle Input and Output target types
  if(DEFINED BINTOOLS_OBJCOPY_TARGET_OUTPUT)
    set(obj_copy_target_output "")
    set(obj_copy_gap_fill "")
    if(DEFINED BINTOOLS_OBJCOPY_GAP_FILL)
    set(obj_copy_gap_fill "-f;${BINTOOLS_OBJCOPY_GAP_FILL}")
    endif()
      # only mwdt's elf2hex supports gap fill
    if(${BINTOOLS_OBJCOPY_TARGET_OUTPUT} STREQUAL "srec")
      set(obj_copy_target_output "-m")
    elseif(${BINTOOLS_OBJCOPY_TARGET_OUTPUT} STREQUAL "ihex")
      set(obj_copy_target_output "-I")
    elseif(${BINTOOLS_OBJCOPY_TARGET_OUTPUT} STREQUAL "binary")
      set(obj_copy_target_output "-B")
    endif()
     set(obj_copy_cmd
        COMMAND ${CMAKE_ELF2HEX} ${obj_copy_gap_fill} ${obj_copy_target_output}
        -o ${BINTOOLS_OBJCOPY_FILE_OUTPUT} ${BINTOOLS_OBJCOPY_FILE_INPUT})
  endif()

  # Handle sections, if any
  # 1. Section only selection(s)
  set(obj_copy_sections_only "")
  if(DEFINED BINTOOLS_OBJCOPY_SECTION_ONLY)
    foreach(section_only ${BINTOOLS_OBJCOPY_SECTION_ONLY})
      list(APPEND obj_copy_sections_only "-sn;${section_only}")
    endforeach()
    set(obj_copy_cmd
        COMMAND ${CMAKE_ELF2BIN} -q ${obj_copy_sections_only}
        ${BINTOOLS_OBJCOPY_FILE_INPUT} ${BINTOOLS_OBJCOPY_FILE_OUTPUT})
  endif()

  set(obj_copy_sections_rename "")
  if(DEFINED BINTOOLS_OBJCOPY_SECTION_RENAME)
    foreach(section_rename ${BINTOOLS_OBJCOPY_SECTION_RENAME})
      if(NOT ${section_rename} MATCHES "^.*=.*$")
        message(FATAL_ERROR "Malformed section renaming. Must be from=to, have ${section_rename}")
      else()
        list(APPEND obj_copy_sections_rename "--rename-section;${section_rename}")
      endif()
    endforeach()
    set(obj_copy_cmd
        COMMAND ${CMAKE_OBJCOPY} ${obj_copy_sections_rename}
        ${BINTOOLS_OBJCOPY_FILE_INPUT} ${BINTOOLS_OBJCOPY_FILE_OUTPUT})
  endif()

 # no support of remove sections

  # Place command in the parent provided variable
  set(${BINTOOLS_OBJCOPY_RESULT_CMD_LIST} ${obj_copy_cmd} PARENT_SCOPE)

endfunction(bintools_objcopy)


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
    set(obj_dump_disassemble "-T") # --disassemble
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
    set(readelf_headers "-qshp") # --headers
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
#
#      FILE_INPUT         <The input file>
#      FILE_OUTPUT        <The output file>
#    )
function(bintools_strip)
  cmake_parse_arguments(
    # Prefix of output variables
    BINTOOLS_STRIP
    # List of argument names without values, hence boolean
    "STRIP_ALL;STRIP_DEBUG"
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
    set(strip_what "-qls")
  elseif(${BINTOOLS_STRIP_STRIP_DEBUG})
    set(strip_what "-ql")
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
