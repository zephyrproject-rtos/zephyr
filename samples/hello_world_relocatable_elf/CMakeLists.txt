# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

# QEMU's ELF loader places each segment at its LMA, so raising every LMA by
# this offset after linking makes the image run away from its link address and
# exercises the self-relocation code. Use 0 to run the image in place.
# The offset must be a multiple of 16, the stack pointer alignment: the startup
# code halts on any other value.
set(SAMPLE_LOAD_OFFSET 0x1000 CACHE STRING "Offset added to the LMA of every segment of zephyr.elf")

math(EXPR sample_offset_rem "${SAMPLE_LOAD_OFFSET} % 16")
if(NOT sample_offset_rem EQUAL 0)
  message(FATAL_ERROR "SAMPLE_LOAD_OFFSET=${SAMPLE_LOAD_OFFSET} must be a multiple of 16")
endif()

# Zephyr reads this property while find_package(Zephyr) processes its own
# build, so it has to be set before. The generator expressions are evaluated
# later, once the toolchain and the final executable are known. The objcopy
# must support --change-section-lma: it is the first candidate that does (see
# below), normally CMAKE_OBJCOPY.
set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  COMMAND $<TARGET_PROPERTY:app,SAMPLE_OBJCOPY>
          --change-section-lma "*+${SAMPLE_LOAD_OFFSET}" $<TARGET_FILE:zephyr_final>
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_world_relocatable_elf)

# --change-section-lma is missing from LLVM objcopy before version 20 and from
# older SDK LLVM builds, so test for the option instead of the version.
# Candidates, in order: CMAKE_OBJCOPY, an objcopy next to the compiler or in
# the toolchain directory, then the SDK's GNU objcopy. The SDK's GNU toolchain
# directories are named <arch>*-zephyr-*. Its cmake mapping from ARCH to the
# triple is not loaded with an LLVM toolchain, so match on the architecture
# prefix instead.
if(NOT DEFINED ZEPHYR_SDK_INSTALL_DIR AND DEFINED ENV{ZEPHYR_SDK_INSTALL_DIR})
  set(ZEPHYR_SDK_INSTALL_DIR $ENV{ZEPHYR_SDK_INSTALL_DIR})
endif()
get_filename_component(sample_compiler_dir ${CMAKE_C_COMPILER} DIRECTORY)
find_program(sample_toolchain_objcopy NAMES llvm-objcopy objcopy NO_CACHE
  HINTS ${TOOLCHAIN_HOME} ${sample_compiler_dir} NO_DEFAULT_PATH
)
set(sample_objcopy_candidates ${CMAKE_OBJCOPY} ${sample_toolchain_objcopy})
if(ZEPHYR_SDK_INSTALL_DIR)
  file(GLOB sample_gnu_objcopy "${ZEPHYR_SDK_INSTALL_DIR}/gnu/${ARCH}*-zephyr-*/bin/*-objcopy")
  list(APPEND sample_objcopy_candidates ${sample_gnu_objcopy})
endif()
list(REMOVE_DUPLICATES sample_objcopy_candidates)

set(sample_objcopy "")
foreach(candidate IN LISTS sample_objcopy_candidates)
  execute_process(COMMAND ${candidate} --help
    OUTPUT_VARIABLE candidate_help
    ERROR_QUIET
    RESULT_VARIABLE candidate_rc
  )
  if(candidate_rc EQUAL 0 AND candidate_help MATCHES "--change-section-lma")
    set(sample_objcopy ${candidate})
    break()
  endif()
endforeach()
if(NOT sample_objcopy)
  message(FATAL_ERROR "No objcopy with --change-section-lma found. Tried:"
    " ${sample_objcopy_candidates}. Use GNU objcopy or LLVM objcopy 20 or later.")
endif()
set_property(TARGET app PROPERTY SAMPLE_OBJCOPY ${sample_objcopy})

target_sources(app PRIVATE src/main.c)
target_compile_definitions(app PRIVATE SAMPLE_LOAD_OFFSET=${SAMPLE_LOAD_OFFSET})
