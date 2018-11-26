# No official documentation exists for the "Generic" value, except their wiki.
#
# https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/CrossCompiling:
#   CMAKE_SYSTEM_NAME : this one is mandatory, it is the name of the target
#   system, i.e. the same as CMAKE_SYSTEM_NAME would have if CMake would run
#   on the target system.  Typical examples are "Linux" and "Windows". This
#   variable is used for constructing the file names of the platform files
#   like Linux.cmake or Windows-gcc.cmake. If your target is an embedded
#   system without OS set CMAKE_SYSTEM_NAME to "Generic".
set(CMAKE_SYSTEM_NAME Generic)

# https://cmake.org/cmake/help/latest/variable/CMAKE_SYSTEM_PROCESSOR.html:
#   The name of the CPU CMake is building for.
#
# https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/CrossCompiling:
#   CMAKE_SYSTEM_PROCESSOR : optional, processor (or hardware) of the
#   target system. This variable is not used very much except for one
#   purpose, it is used to load a
#   CMAKE_SYSTEM_NAME-compiler-CMAKE_SYSTEM_PROCESSOR.cmake file,
#   which can be used to modify settings like compiler flags etc. for
#   the target
set(CMAKE_SYSTEM_PROCESSOR ${ARCH})

# https://cmake.org/cmake/help/latest/variable/CMAKE_SYSTEM_VERSION.html:
#   When the CMAKE_SYSTEM_NAME variable is set explicitly to enable cross
#   compiling then the value of CMAKE_SYSTEM_VERSION must also be set
#   explicitly to specify the target system version.
set(CMAKE_SYSTEM_VERSION ${PROJECT_VERSION})

# https://cmake.org/cmake/help/latest/variable/CMAKE_FIND_ROOT_PATH_MODE_PROGRAM.html:
#   [...] controls whether the CMAKE_FIND_ROOT_PATH and CMAKE_SYSROOT
#   are used by find_program().  If set to ONLY, then only the roots in
#   CMAKE_FIND_ROOT_PATH will be searched.  If set to NEVER, then the roots
#   in CMAKE_FIND_ROOT_PATH will be ignored and only the host system root
#   will be used.  If set to BOTH, then the host system paths and the paths
#   in CMAKE_FIND_ROOT_PATH will be searched.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

if(NOT TOOLCHAIN_ROOT)
  if(DEFINED ENV{TOOLCHAIN_ROOT})
    # Support for out-of-tree toolchain
    set(TOOLCHAIN_ROOT $ENV{TOOLCHAIN_ROOT})
  else()
    # Default toolchain cmake file
    set(TOOLCHAIN_ROOT ${ZEPHYR_BASE})
  endif()
endif()

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
  elseif(CROSS_COMPILE OR (DEFINED ENV{CROSS_COMPILE}))
    set(ZEPHYR_TOOLCHAIN_VARIANT cross-compile)
  endif()
endif()

# Until we completely deprecate it
if("${ZEPHYR_TOOLCHAIN_VARIANT}" STREQUAL "gccarmemb")
  message(WARNING "gccarmemb is deprecated, please use gnuarmemb instead")
  set(ZEPHYR_TOOLCHAIN_VARIANT "gnuarmemb")
endif()


set(TOOLCHAIN_ROOT ${TOOLCHAIN_ROOT} CACHE STRING "Zephyr toolchain root")
assert(TOOLCHAIN_ROOT "Zephyr toolchain root path invalid: please set the TOOLCHAIN_ROOT-variable")

set(ZEPHYR_TOOLCHAIN_VARIANT ${ZEPHYR_TOOLCHAIN_VARIANT} CACHE STRING "Zephyr toolchain variant")
assert(ZEPHYR_TOOLCHAIN_VARIANT "Zephyr toolchain variant invalid: please set the ZEPHYR_TOOLCHAIN_VARIANT-variable")

if(CONFIG_ARCH_POSIX OR (ZEPHYR_TOOLCHAIN_VARIANT STREQUAL "host"))
  set(COMPILER host-gcc)
endif()


# Configure the toolchain based on what SDK/toolchain is in use.
if(NOT (COMPILER STREQUAL "host-gcc"))
  include(${TOOLCHAIN_ROOT}/cmake/toolchain/${ZEPHYR_TOOLCHAIN_VARIANT}.cmake)
endif()

# Configure the toolchain based on what toolchain technology is used
# (gcc, host-gcc etc.)
include(${ZEPHYR_BASE}/cmake/compiler/${COMPILER}.cmake OPTIONAL)

# Uniquely identify the toolchain wrt. it's capabilities.
#
# What we are looking for, is a signature definition that is defined
# like this: Toolchains with the same signature will always support the same
# set of flags.
# It is not clear how this signature should be constructed. The
# strategy chosen is to md5sum the CC binary.
file(MD5 ${CMAKE_C_COMPILER} CMAKE_C_COMPILER_MD5_SUM)
set(TOOLCHAIN_SIGNATURE ${CMAKE_C_COMPILER_MD5_SUM})
