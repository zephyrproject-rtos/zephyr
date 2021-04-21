# SPDX-License-Identifier: Apache-2.0

if(NOT TOOLCHAIN_ROOT)
  if(DEFINED ENV{TOOLCHAIN_ROOT})
    # Support for out-of-tree toolchain
    set(TOOLCHAIN_ROOT $ENV{TOOLCHAIN_ROOT})
  else()
    # Default toolchain cmake file
    set(TOOLCHAIN_ROOT ${ZEPHYR_BASE})
  endif()
endif()
zephyr_file(APPLICATION_ROOT TOOLCHAIN_ROOT)

# Don't inherit compiler flags from the environment
foreach(var AFLAGS CFLAGS CXXFLAGS CPPFLAGS LDFLAGS)
  if(DEFINED ENV{${var}})
    message(WARNING "The environment variable '${var}' was set to $ENV{${var}},
but Zephyr ignores flags from the environment. Use 'cmake -DEXTRA_${var}=$ENV{${var}}' instead.")
    unset(ENV{${var}})
  endif()
endforeach()

# Host-tools don't unconditionally set TOOLCHAIN_HOME anymore,
# but in case Zephyr's SDK toolchain is used, set TOOLCHAIN_HOME
if("${ZEPHYR_TOOLCHAIN_VARIANT}" STREQUAL "zephyr")
  set(TOOLCHAIN_HOME ${HOST_TOOLS_HOME})
endif()

set(TOOLCHAIN_ROOT ${TOOLCHAIN_ROOT} CACHE STRING "Zephyr toolchain root" FORCE)
assert(TOOLCHAIN_ROOT "Zephyr toolchain root path invalid: please set the TOOLCHAIN_ROOT-variable")

# Set cached ZEPHYR_TOOLCHAIN_VARIANT.
set(ZEPHYR_TOOLCHAIN_VARIANT ${ZEPHYR_TOOLCHAIN_VARIANT} CACHE STRING "Zephyr toolchain variant")

# Configure the toolchain based on what SDK/toolchain is in use.
include(${TOOLCHAIN_ROOT}/cmake/toolchain/${ZEPHYR_TOOLCHAIN_VARIANT}/generic.cmake)

set_ifndef(TOOLCHAIN_KCONFIG_DIR ${TOOLCHAIN_ROOT}/cmake/toolchain/${ZEPHYR_TOOLCHAIN_VARIANT})

# Configure the toolchain based on what toolchain technology is used
# (gcc, host-gcc etc.)
include(${TOOLCHAIN_ROOT}/cmake/compiler/${COMPILER}/generic.cmake OPTIONAL)
include(${TOOLCHAIN_ROOT}/cmake/linker/${LINKER}/generic.cmake OPTIONAL)
include(${TOOLCHAIN_ROOT}/cmake/bintools/${BINTOOLS}/generic.cmake OPTIONAL)
