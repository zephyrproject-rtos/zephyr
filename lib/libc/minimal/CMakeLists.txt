# SPDX-License-Identifier: Apache-2.0

zephyr_system_include_directories(include)

add_library(c)

target_link_libraries(c PRIVATE zephyr_interface kernel c_common)
set_target_properties(c PROPERTIES LINK_INTERFACE_MULTIPLICITY 3)
target_link_libraries(kernel INTERFACE c)

set(GEN_DIR ${ZEPHYR_BINARY_DIR}/include/generated)
set(STRERROR_TABLE_H ${GEN_DIR}/libc/minimal/strerror_table.h)

if(NOT ${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "armclang")

  # Create a libgcc import library to apply dependencies

  add_library(gcc INTERFACE IMPORTED GLOBAL)
  set_property(TARGET gcc PROPERTY IMPORTED_LIBNAME "gcc")

  target_link_libraries(gcc INTERFACE c)
  target_link_libraries(c INTERFACE gcc)
endif()

target_compile_options(c PRIVATE $<TARGET_PROPERTY:compiler,no_builtin>)

target_sources(c PRIVATE
  source/stdlib/atoi.c
  source/stdlib/strtol.c
  source/stdlib/strtoul.c
  source/stdlib/strtoll.c
  source/stdlib/strtoull.c
  source/stdlib/bsearch.c
  source/stdlib/exit.c
  source/stdlib/qsort.c
  source/string/strerror.c
  source/string/strncasecmp.c
  source/string/strstr.c
  source/string/string.c
  source/string/strspn.c
  source/stdout/stdout_console.c
  source/stdout/sprintf.c
  source/stdout/fprintf.c
  source/math/sqrtf.c
  source/math/sqrt.c
  ${STRERROR_TABLE_H}
)

target_sources_ifdef(CONFIG_MINIMAL_LIBC_TIME
  c
  PRIVATE
  source/time/gmtime.c
  )

target_sources_ifdef(CONFIG_MINIMAL_LIBC_RAND
  c
  PRIVATE
  source/stdlib/rand.c
  )

add_custom_command(
  OUTPUT ${STRERROR_TABLE_H}
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_BASE}/scripts/build/gen_strerror_table.py
  -i include/errno.h
  -o ${STRERROR_TABLE_H}
  DEPENDS include/errno.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
