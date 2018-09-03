set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR ${ARCH})

set(BUILD_SHARED_LIBS OFF)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Don't inherit compiler flags from the environment
foreach(var CFLAGS CXXFLAGS)
  if(DEFINED ENV{${var}})
    message(WARNING "The environment variable '${var}' was set to $ENV{${var}},
but Zephyr ignores flags from the environment. Use 'cmake -DEXTRA_${var}=$ENV{${var}}' instead.")
    unset(ENV{${var}})
  endif()
endforeach()

# Until we completely deprecate it
if(NOT DEFINED ENV{ZEPHYR_TOOLCHAIN_VARIANT})
  if(DEFINED ENV{ZEPHYR_GCC_VARIANT})
    message(WARNING "ZEPHYR_GCC_VARIANT is deprecated, please use ZEPHYR_TOOLCHAIN_VARIANT instead")
    set(ZEPHYR_TOOLCHAIN_VARIANT $ENV{ZEPHYR_GCC_VARIANT})
  endif()
endif()

if(NOT ZEPHYR_TOOLCHAIN_VARIANT)
  if(DEFINED ENV{ZEPHYR_TOOLCHAIN_VARIANT})
    set(ZEPHYR_TOOLCHAIN_VARIANT $ENV{ZEPHYR_TOOLCHAIN_VARIANT})
  elseif(CROSS_COMPILE OR CONFIG_CROSS_COMPILE OR (DEFINED ENV{CROSS_COMPILE}))
    set(ZEPHYR_TOOLCHAIN_VARIANT cross-compile)
  endif()
endif()

# Until we completely deprecate it
if(${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "gccarmemb")
  message(WARNING "gccarmemb is deprecated, please use gnuarmemb instead")
  set(ZEPHYR_TOOLCHAIN_VARIANT "gnuarmemb")
endif()

set(ZEPHYR_TOOLCHAIN_VARIANT ${ZEPHYR_TOOLCHAIN_VARIANT} CACHE STRING "Zephyr toolchain variant")
assert(ZEPHYR_TOOLCHAIN_VARIANT "Zephyr toolchain variant invalid: please set the ZEPHYR_TOOLCHAIN_VARIANT-variable")

if(CONFIG_ARCH_POSIX OR (ZEPHYR_TOOLCHAIN_VARIANT STREQUAL "host"))
  set(COMPILER host-gcc)
endif()


# Configure the toolchain based on what SDK/toolchain is in use.
if(NOT (COMPILER STREQUAL "host-gcc"))
  include(${ZEPHYR_BASE}/cmake/toolchain/${ZEPHYR_TOOLCHAIN_VARIANT}.cmake)
endif()

# Configure the toolchain based on what toolchain technology is used
# (gcc, host-gcc etc.)
include(${ZEPHYR_BASE}/cmake/compiler/${COMPILER}.cmake OPTIONAL)

# Uniquely identify the toolchain wrt. it's capabilities.
#
# What we are looking for, is a signature definition that is defined
# like this:

# Toolchains with the same signature will always support the same set
# of flags.

# It is not clear how this signature should be constructed. The
# strategy chosen is to md5sum the CC binary.

file(MD5 ${CMAKE_C_COMPILER} CMAKE_C_COMPILER_MD5_SUM)
set(TOOLCHAIN_SIGNATURE ${CMAKE_C_COMPILER_MD5_SUM})
