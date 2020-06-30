# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(libc-hooks.c)

# Zephyr normally uses -ffreestanding, which with current GNU toolchains
# means that the flag macros used by picolibc <inttypes.h> to signal
# support for PRI.64 macros are not present.  To make them available we
# need to hook into the include path before the system files and
# explicitly include the picolibc header that provides those macros.
zephyr_include_directories(include)

# define __LINUX_ERRNO_EXTENSIONS__ so we get errno defines like -ESHUTDOWN
# used by the network stack
zephyr_compile_definitions(__LINUX_ERRNO_EXTENSIONS__)

zephyr_link_libraries(
  m
  c
  gcc # Lib C depends on libgcc.
  )

# The -T/dev/null avoids pulling in picolibc.ld
zephyr_link_libraries(
  --specs=picolibc.specs
  -T/dev/null
  )

zephyr_compile_options(
  --specs=picolibc.specs
  -D_GNU_SOURCE
  )

if(CONFIG_PICOLIBC_INTEGER_PRINTF)
  zephyr_compile_options(
    -DPICOLIBC_INTEGER_PRINTF_SCANF
    )
  zephyr_link_libraries(
    -DPICOLIBC_INTEGER_PRINTF_SCANF
    )
endif()
