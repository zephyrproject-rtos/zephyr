# SPDX-License-Identifier: Apache-2.0

# FindTargetTools module for locating a set of tools to use on the host but
# targeting a remote target for Zephyr development.
#
# This module will lookup following target tools for Zephyr development:
# +---------------------------------------------------------------+
# | Tool               | Required |  Notes:                       |
# +---------------------------------------------------------------+
# | Target C-compiler  | Yes      |                               |
# | Target Assembler   | Yes      |                               |
# | Target linker      | Yes      |                               |
# +---------------------------------------------------------------+
#
# The module defines the following variables:
#
# 'CMAKE_C_COMPILER'
# Path to target C compiler.
# Set to 'CMAKE_C_COMPILER-NOTFOUND' if no C compiler was found.
#
# 'TargetTools_FOUND', 'TARGETTOOLS_FOUND'
# True if all required host tools were found.

find_package(HostTools)

if(TargetTools_FOUND)
  return()
endif()

# Prevent CMake from testing the toolchain
set(CMAKE_C_COMPILER_FORCED   1)
set(CMAKE_CXX_COMPILER_FORCED 1)

# https://cmake.org/cmake/help/latest/variable/CMAKE_SYSTEM_NAME.html:
#   The name of the operating system for which CMake is to build.
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

# https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_BYTE_ORDER.html
#   Byte order of <LANG> compiler target architecture, if known.
#
# Zephyr Kconfig defines BIG_ENDIAN according to arch, SoC, Board, so propagate
# this setting to allow users to read the standard CMake variable or use
# 'test_big_endian()' function.
if(CONFIG_BIG_ENDIAN)
  set(CMAKE_C_BYTE_ORDER   BIG_ENDIAN)
  set(CMAKE_CXX_BYTE_ORDER BIG_ENDIAN)
else()
  set(CMAKE_C_BYTE_ORDER   LITTLE_ENDIAN)
  set(CMAKE_CXX_BYTE_ORDER LITTLE_ENDIAN)
endif()

# Do not build dynamically loadable libraries by default
set(BUILD_SHARED_LIBS OFF)

# Custom targets for compiler and linker flags.
add_custom_target(asm)
add_custom_target(compiler)
add_custom_target(compiler-cpp)
add_custom_target(linker)

if(NOT (COMPILER STREQUAL "host-gcc"))
  include(${TOOLCHAIN_ROOT}/cmake/toolchain/${ZEPHYR_TOOLCHAIN_VARIANT}/target.cmake)
endif()

# The 'generic' compiler and the 'target' compiler might be different,
# so we unset the 'generic' one and thereby force the 'target' to
# re-set it.
unset(CMAKE_C_COMPILER)
unset(CMAKE_C_COMPILER CACHE)

# A toolchain consist of a compiler and a linker.
# In Zephyr, toolchains require a port under cmake/toolchain/.
# Each toolchain port must set COMPILER and LINKER.
# E.g. toolchain/llvm may pick {clang, ld} or {clang, lld}.
add_custom_target(bintools)

include(${TOOLCHAIN_ROOT}/cmake/compiler/${COMPILER}/target.cmake OPTIONAL)
include(${TOOLCHAIN_ROOT}/cmake/linker/${LINKER}/target.cmake OPTIONAL)
include(${ZEPHYR_BASE}/cmake/bintools/bintools_template.cmake)
include(${TOOLCHAIN_ROOT}/cmake/bintools/${BINTOOLS}/target.cmake OPTIONAL)

include(${TOOLCHAIN_ROOT}/cmake/linker/target_template.cmake)

set(TargetTools_FOUND TRUE)
set(TARGETTOOLS_FOUND TRUE)
