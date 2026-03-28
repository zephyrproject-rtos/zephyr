# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(libc-hooks.c)

# Do not allow LTO when compiling libc-hooks.c file
set_source_files_properties(libc-hooks.c PROPERTIES COMPILE_OPTIONS $<TARGET_PROPERTY:compiler,prohibit_lto>)

# Zephyr normally uses -ffreestanding, which with current GNU toolchains
# means that the flag macros used by newlib 3.x <inttypes.h> to signal
# support for PRI.64 macros are not present.  To make them available we
# need to hook into the include path before the system files and
# explicitly include the newlib header that provides those macros.
zephyr_include_directories(include)

# LIBC_LIBRARY_DIR may or may not have been set by the toolchain. E.g. when
# using ZEPHYR_TOOLCHAIN_VARIANT=cross-compile it will be either up to the
# toolchain to know where it's libc implementation is, or if it is
# unable to, it will be up to the user to specify LIBC_LIBRARY_DIR vars to
# point to a newlib implementation.
if(LIBC_LIBRARY_DIR)
  set(LIBC_LIBRARY_DIR_FLAG -L${LIBC_LIBRARY_DIR})
endif()

# Define _ANSI_SOURCE in order to prevent Newlib from defining POSIX primitives
# in its headers when GNU dialect is used (-std=gnu*). Newlib features.h
# defines _DEFAULT_SOURCE when __STRICT_ANSI__ is not defined by GCC (i.e. when
# -std=gnu*), which leads to various POSIX definitions being provided by the
# Newlib headers and conflicts with the POSIX definitions provided by Zephyr.
zephyr_compile_definitions(_ANSI_SOURCE)

# define __LINUX_ERRNO_EXTENSIONS__ so we get errno defines like -ESHUTDOWN
# used by the network stack
zephyr_compile_definitions(__LINUX_ERRNO_EXTENSIONS__)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  zephyr_link_libraries(
    ${LIBC_LIBRARY_DIR_FLAG} # NB: Optional
    $<$<BOOL:${CONFIG_NEWLIB_LIBC_FLOAT_PRINTF}>:-u_printf_float>
    $<$<BOOL:${CONFIG_NEWLIB_LIBC_FLOAT_SCANF}>:-u_scanf_float>
    )
endif()

if(CONFIG_NEWLIB_LIBC_NANO)
  zephyr_link_libraries(
    -specs=nano.specs
    )
  zephyr_compile_options(
    -specs=nano.specs
    )
endif()
