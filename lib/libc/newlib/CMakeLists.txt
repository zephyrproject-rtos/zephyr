# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(libc-hooks.c)

# Zephyr normally uses -ffreestanding, which with current GNU toolchains
# means that the flag macros used by newlib 3.x <inttypes.h> to signal
# support for PRI.64 macros are not present.  To make them available we
# need to hook into the include path before the system files and
# explicitly include the newlib header that provides those macros.
zephyr_include_directories(include)

# LIBC_*_DIR may or may not have been set by the toolchain. E.g. when
# using ZEPHYR_TOOLCHAIN_VARIANT=cross-compile it will be either up to the
# toolchain to know where it's libc implementation is, or if it is
# unable to, it will be up to the user to specify LIBC_*_DIR vars to
# point to a newlib implementation.  Note that this does not change the
# directory order if LIBC_INCLUDE_DIR is already a system header
# directory.

if(LIBC_INCLUDE_DIR)
  zephyr_include_directories(${LIBC_INCLUDE_DIR})
endif()

if(LIBC_LIBRARY_DIR)
  set(LIBC_LIBRARY_DIR_FLAG -L${LIBC_LIBRARY_DIR})
endif()

# define __LINUX_ERRNO_EXTENSIONS__ so we get errno defines like -ESHUTDOWN
# used by the network stack
zephyr_compile_definitions(__LINUX_ERRNO_EXTENSIONS__)

# We are using
# - ${CMAKE_C_LINKER_WRAPPER_FLAG}${CMAKE_LINK_LIBRARY_FLAG}c
# - ${CMAKE_C_LINKER_WRAPPER_FLAG}${CMAKE_LINK_LIBRARY_FLAG}gcc
# - c
# in code below.
# This is needed because when linking with newlib on aarch64, then libgcc has a
# link dependency to libc (strchr), but libc also has dependencies to libgcc.
#
# CMake is capable of handling circular link dependencies for CMake defined
# static libraries, which can be further controlled using LINK_INTERFACE_MULTIPLICITY.
# However, libc and libgcc are not regular CMake libraries, and is seen as linker
# flags by CMake, and thus symbol de-duplications will be performed.
# CMake link options cannot be used, as that will place those libs first on the
# linker invocation. -Wl,--start-group is problematic as the placement of -lc
# and -lgcc is not guaranteed in case later libraries are also using
# -lc / -libbgcc as interface linker flags.
#
# Thus, we resort to use `${CMAKE_C_LINKER_WRAPPER_FLAG}${CMAKE_LINK_LIBRARY_FLAG}`
# as this ensures the uniqueness and thus avoids symbol de-duplication which means
# libc will be followed by libgcc, which is finally followed by libc again.

list(JOIN CMAKE_C_LINKER_WRAPPER_FLAG "" linker_wrapper_string)

zephyr_link_libraries(
  m
  "${linker_wrapper_string}${CMAKE_LINK_LIBRARY_FLAG}c"
  ${LIBC_LIBRARY_DIR_FLAG} # NB: Optional
  $<$<BOOL:${CONFIG_NEWLIB_LIBC_FLOAT_PRINTF}>:-u_printf_float>
  $<$<BOOL:${CONFIG_NEWLIB_LIBC_FLOAT_SCANF}>:-u_scanf_float>
  # Lib C depends on libgcc. e.g. libc.a(lib_a-fvwrite.o) references __aeabi_idiv
  "${linker_wrapper_string}${CMAKE_LINK_LIBRARY_FLAG}gcc"
  c
  )

if(CONFIG_NEWLIB_LIBC_NANO)
  zephyr_link_libraries(
    -specs=nano.specs
    )
  zephyr_compile_options(
    -specs=nano.specs
    )
endif()
