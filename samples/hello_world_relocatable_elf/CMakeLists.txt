# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

# QEMU's ELF loader places each segment at its LMA, so raising every LMA by
# this offset after linking makes the image run away from its link address and
# exercises the self-relocation code. Use 0 to run the image in place.
set(SAMPLE_LOAD_OFFSET 0x1000 CACHE STRING "Offset added to the LMA of every segment of zephyr.elf")

# Zephyr reads this property while find_package(Zephyr) processes its own
# build, so it has to be set before. The generator expressions are evaluated
# later, once the toolchain and the final executable are known. The objcopy
# must support --change-section-lma: it is CMAKE_OBJCOPY, except that an LLVM
# objcopy up to version 19 is replaced by the SDK's GNU objcopy (see below).
set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  COMMAND $<TARGET_PROPERTY:app,SAMPLE_OBJCOPY>
          --change-section-lma "*+${SAMPLE_LOAD_OFFSET}" $<TARGET_FILE:zephyr_final>
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_world_relocatable_elf)

set(sample_objcopy ${CMAKE_OBJCOPY})
execute_process(COMMAND ${CMAKE_OBJCOPY} --version
  OUTPUT_VARIABLE objcopy_version
  ERROR_QUIET
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(objcopy_version MATCHES "LLVM version ([0-9]+)")
  set(llvm_major ${CMAKE_MATCH_1})
endif()
if(DEFINED llvm_major AND llvm_major LESS_EQUAL 19)
  # The SDK's GNU toolchain directories are named <arch>*-zephyr-*. Its
  # cmake mapping from ARCH to the triple is not loaded with an LLVM
  # toolchain, so match on the architecture prefix instead.
  if(NOT DEFINED ZEPHYR_SDK_INSTALL_DIR AND DEFINED ENV{ZEPHYR_SDK_INSTALL_DIR})
    set(ZEPHYR_SDK_INSTALL_DIR $ENV{ZEPHYR_SDK_INSTALL_DIR})
  endif()
  file(GLOB gnu_objcopy_candidates "${ZEPHYR_SDK_INSTALL_DIR}/gnu/${ARCH}*-zephyr-*/bin/*-objcopy")
  if(NOT ZEPHYR_SDK_INSTALL_DIR OR NOT gnu_objcopy_candidates)
    message(FATAL_ERROR "${CMAKE_OBJCOPY} is LLVM ${llvm_major} and the SDK's GNU objcopy was"
      " not found. --change-section-lma needs GNU objcopy or LLVM objcopy 20 or later.")
  endif()
  list(GET gnu_objcopy_candidates 0 sample_objcopy)
endif()
set_property(TARGET app PROPERTY SAMPLE_OBJCOPY ${sample_objcopy})

target_sources(app PRIVATE src/main.c)
target_compile_definitions(app PRIVATE SAMPLE_LOAD_OFFSET=${SAMPLE_LOAD_OFFSET})
