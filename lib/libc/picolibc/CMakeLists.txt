# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(
  assert.c
  cbprintf.c
  chk_fail.c
  errno_wrap.c
  exit.c
  locks.c
  stdio.c
  )
zephyr_include_directories(include)

zephyr_library_compile_options($<TARGET_PROPERTY:compiler,prohibit_lto>)

# define __LINUX_ERRNO_EXTENSIONS__ so we get errno defines like -ESHUTDOWN
# used by the network stack
zephyr_compile_definitions(__LINUX_ERRNO_EXTENSIONS__)

if(NOT CONFIG_PICOLIBC_USE_MODULE)
  if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # Ask the compiler for the real path to `picolibc.specs`
    # ccache always fails to cache calls without a full path with the following
    # internal log error:
    #   [timestamp] Trying direct lookup
    #   [timestamp] Failed to lstat picolibc.specs: No such file or directory
    #   [timestamp] While processing -specs=picolibc.specs: picolibc.specs is missing
    #   [timestamp] Failed; falling back to running the real compiler
    execute_process(
      COMMAND ${CMAKE_C_COMPILER} -print-file-name=picolibc.specs
      OUTPUT_VARIABLE specs_file
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    # Normalise the path for clarity
    get_filename_component(specs_file "${specs_file}" REALPATH)
  else()
    set(specs_file picolibc.specs)
  endif()

  # Use picolibc provided with the toolchain. This requires a new enough
  # toolchain so that the version of picolibc supports auto-detecting a
  # Zephyr build (via the __ZEPHYR__ macro) to expose the Zephyr C API
  zephyr_compile_options(PROPERTY specs ${specs_file})
  zephyr_link_libraries(PROPERTY specs ${specs_file})
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
