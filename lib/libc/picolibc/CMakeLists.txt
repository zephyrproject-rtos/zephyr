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

  # Use picolibc provided with the toolchain. This requires a new enough
  # toolchain so that the version of picolibc supports auto-detecting a
  # Zephyr build (via the __ZEPHYR__ macro) to expose the Zephyr C API
  zephyr_compile_options(PROPERTY specs picolibc.specs)
  zephyr_link_libraries(PROPERTY specs picolibc.specs)
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

  # When not using GCC, specs files are not available to translate the
  # -DPICOLIBC_*_PRINTF_SCANF flags into --defsym directives. Pass the
  # --defsym flags directly so the linker resolves vfprintf/vfscanf to
  # the correct picolibc variant (minimal, integer, long-long, etc.).
  get_target_property(specs compiler specs)
  get_target_property(defsym linker defsym)
  if(NOT specs AND defsym)
    if(CONFIG_PICOLIBC_IO_FLOAT)
      zephyr_link_libraries(${defsym}vfprintf=__d_vfprintf ${defsym}vfscanf=__d_vfscanf)
    elseif(CONFIG_PICOLIBC_IO_MINIMAL)
      zephyr_link_libraries(${defsym}vfprintf=__m_vfprintf ${defsym}vfscanf=__m_vfscanf)
    elseif(CONFIG_PICOLIBC_IO_LONG_LONG)
      zephyr_link_libraries(${defsym}vfprintf=__l_vfprintf ${defsym}vfscanf=__l_vfscanf)
    else()
      zephyr_link_libraries(${defsym}vfprintf=__i_vfprintf ${defsym}vfscanf=__i_vfscanf)
    endif()
  endif()

endif()
