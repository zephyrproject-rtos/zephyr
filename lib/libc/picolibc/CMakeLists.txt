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

zephyr_library_compile_options($<TARGET_PROPERTY:compiler,prohibit_lto>)

# define __LINUX_ERRNO_EXTENSIONS__ so we get errno defines like -ESHUTDOWN
# used by the network stack
zephyr_compile_definitions(__LINUX_ERRNO_EXTENSIONS__)

if(NOT CONFIG_PICOLIBC_USE_MODULE)

  # Use picolibc provided with the toolchain. This requires a new enough
  # toolchain so that the version of picolibc supports auto-detecting a
  # Zephyr build (via the __ZEPHYR__ macro) to expose the Zephyr C API

  if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    # ask GNU compiler about absolute location of the specs so that other tools like
    # ccache can find the specs as well.
    execute_process(
      COMMAND ${CMAKE_C_COMPILER} -print-file-name=picolibc.specs
      OUTPUT_VARIABLE PICOLIBC_SPECS
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    zephyr_compile_options(PROPERTY specs "${PICOLIBC_SPECS}")
    zephyr_link_libraries(PROPERTY specs "${PICOLIBC_SPECS}")
  endif()
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
