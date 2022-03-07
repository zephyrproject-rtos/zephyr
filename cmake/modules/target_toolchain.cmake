# SPDX-License-Identifier: Apache-2.0

include_guard(GLOBAL)

# Prevent CMake from testing the toolchain
set(CMAKE_C_COMPILER_FORCED   1)
set(CMAKE_CXX_COMPILER_FORCED 1)

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

# We are not building dynamically loadable libraries
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
