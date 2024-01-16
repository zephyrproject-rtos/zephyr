# SPDX-License-Identifier: Apache-2.0

add_library(c_picolibc
  assert.c
  cbprintf.c
  chk_fail.c
  errno_wrap.c
  exit.c
  locks.c
  stdio.c
  write.c
  )

# There's a circular dependency between kernel, c, c_picolibc, c_common and
# zephyr_interface. Tell the kernel to depend upon c_picolibc, then
# construct the rest of the circular set here.

target_link_libraries(c_picolibc PUBLIC zephyr_interface c_common c)
target_link_libraries(kernel INTERFACE c_picolibc)

# We probably only need to set this to 2, but add another repeat to avoid
# link failures.
set_target_properties(c_picolibc PROPERTIES LINK_INTERFACE_MULTIPLICITY 3)

if(NOT ${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "armclang")

  # Create a libgcc import library to apply dependencies

  add_library(gcc INTERFACE IMPORTED GLOBAL)
  set_property(TARGET gcc PROPERTY IMPORTED_LIBNAME "gcc")

  target_link_libraries(gcc INTERFACE c)
  target_link_libraries(c_picolibc INTERFACE gcc)
endif()

# define __LINUX_ERRNO_EXTENSIONS__ so we get errno defines like -ESHUTDOWN
# used by the network stack
zephyr_compile_definitions(__LINUX_ERRNO_EXTENSIONS__)

if(NOT CONFIG_PICOLIBC_USE_MODULE)

  # Use picolibc provided with the toolchain
  # Create a C import library to apply dependencies

  add_library(c INTERFACE IMPORTED GLOBAL)
  set_property(TARGET c PROPERTY IMPORTED_LIBNAME "c")

  target_link_libraries(c INTERFACE c_common c_picolibc gcc)

  zephyr_compile_options(--specs=picolibc.specs)
  zephyr_link_libraries(--specs=picolibc.specs)
  if(CONFIG_PICOLIBC_IO_FLOAT)
    zephyr_compile_definitions(PICOLIBC_DOUBLE_PRINTF_SCANF)
    zephyr_link_libraries(-DPICOLIBC_DOUBLE_PRINTF_SCANF)
  elseif(CONFIG_PICOLIBC_IO_MINIMAL)
    zephyr_compile_definitions(PICOLIBC_MINIMAL_PRINTF_SCANF)
    zephyr_link_libraries(-DPICOLIBC_MINIMAL_PRINTF_SCANF)
  elseif(CONFIG_PICOLIBC_IO_LONG_LONG)
    zephyr_compile_definitions(PICOLIBC_LONG_LONG_PRINTF_SCANF)
    zephyr_link_libraries(-DPICOLIBC_LONG_LONG_PRINTF_SCANF)
  else()
    zephyr_compile_definitions(PICOLIBC_INTEGER_PRINTF_SCANF)
    zephyr_link_libraries(-DPICOLIBC_INTEGER_PRINTF_SCANF)
  endif()

endif()
