# Copyright (c) 2020 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set(exports
  "
CC = ${CMAKE_C_COMPILER}
CXX = ${CMAKE_CXX_COMPILER}
OBJCOPY = ${CMAKE_OBJCOPY}
OBJDUMP = ${CMAKE_OBJDUMP}
AS = ${CMAKE_AS}
AR = ${CMAKE_AR}
NM = ${CMAKE_NM}
GDB = ${CMAKE_GDB}
Z_CFLAGS = -I$<JOIN:$<TARGET_PROPERTY:zephyr_interface,INTERFACE_INCLUDE_DIRECTORIES>, -I> -isystem $<JOIN:$<TARGET_PROPERTY:zephyr_interface,INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>, -isystem > -D$<JOIN:$<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_DEFINITIONS>, -D> $<JOIN:$<TARGET_PROPERTY:zephyr_interface,INTERFACE_COMPILE_OPTIONS>, >
"
  )

# file(GENERATE writes a file at Generation time. Also, it writes one
# file per detected configuration, in this case, each COMPILE_LANGUAGE
# is a new configuration.
#
# We use 'file(GENERATE' instead of configure_file because we want to
# generate the file after Configure-time to have all the
# metadata. Also, we don't use 'add_custom_command' because it cannot
# read the generator expressions that we use.
file(GENERATE
  OUTPUT  ${CMAKE_BINARY_DIR}/Makefile.exports.$<COMPILE_LANGUAGE>
  CONTENT "${exports}"
)
