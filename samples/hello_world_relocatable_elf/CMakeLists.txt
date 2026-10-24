# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

# QEMU's ELF loader places each segment at its LMA, so raising every LMA by
# this offset after linking makes the image run away from its link address and
# exercises the self-relocation code. Use 0 to run the image in place.
set(SAMPLE_LOAD_OFFSET 0x1000 CACHE STRING "Offset added to the LMA of every segment of zephyr.elf")

# Zephyr reads this property while find_package(Zephyr) processes its own
# build, so it has to be set before. The generator expressions are evaluated
# later, once the toolchain and the final executable are known. Needs an
# objcopy that supports --change-section-lma (GNU, or LLVM 19 and later).
set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  COMMAND $<TARGET_PROPERTY:bintools,elfconvert_command>
          --change-section-lma "*+${SAMPLE_LOAD_OFFSET}" $<TARGET_FILE:zephyr_final>
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_world_relocatable_elf)

target_sources(app PRIVATE src/main.c)
target_compile_definitions(app PRIVATE SAMPLE_LOAD_OFFSET=${SAMPLE_LOAD_OFFSET})
