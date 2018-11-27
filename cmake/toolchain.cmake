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

# We are building static libraries
set(BUILD_SHARED_LIBS OFF)


# CMake provides:
#   CMAKE_C_FLAGS_ENV_INIT
#   CMAKE_C_FLAGS_DEBUG
#   CMAKE_C_FLAGS_DEBUG_INIT
#   CMAKE_C_FLAGS_INIT
#   CMAKE_C_FLAGS_MINSIZEREL
#   CMAKE_C_FLAGS_MINSIZEREL_INIT
#   CMAKE_C_FLAGS_RELEASE
#   CMAKE_C_FLAGS_RELEASE_INIT
#   CMAKE_C_FLAGS_RELWITHDEBINFO
#   CMAKE_C_FLAGS_RELWITHDEBINFO_INIT
#   CMAKE_INCLUDE_FLAG_C          Search path for header files "-I" typically
#   CMAKE_LIBRARY_PATH_FLAG       Search path for libraries    "-L" typically
#   CMAKE_LINK_LIBRARY_FLAG       Link with a library          "-l" typically
#   CMAKE_LIBRARY_PATH_TERMINATOR
#   CMAKE_STATIC_LIBRARY_PREFIX   Handled by Zephyr's build system
#   CMAKE_STATIC_LIBRARY_SUFFIX   Handled by Zephyr's build system
#   CMAKE_EXECUTABLE_SUFFIX       Handled by Zephyr's build system
# However, CMake does not provide:
#   CMAKE_DEFINE_FLAG_C           We invent this ourselves as ZEPHYR_DEFINE_FLAG_C
set(ZEPHYR_DEFINE_FLAG_C "-D")  # set default but let toolchain override later if needed


# Don't inherit compiler flags from the environment
foreach(var CFLAGS CXXFLAGS)
  if(DEFINED ENV{${var}})
    message(WARNING "The environment variable '${var}' was set to $ENV{${var}},
but Zephyr ignores flags from the environment. Use 'cmake -DEXTRA_${var}=$ENV{${var}}' instead.")
    unset(ENV{${var}})
  endif()
endforeach()


##
## Determine TOOLCHAIN_ROOT
##

# TODO: TOOLCHAIN_ROOT is not really a good name, because it points to
# cmake files, not the toolchain installation location
if(NOT TOOLCHAIN_ROOT)
  if(DEFINED ENV{TOOLCHAIN_ROOT})
    # Support for out-of-tree/commercial toolchains
    set(TOOLCHAIN_ROOT $ENV{TOOLCHAIN_ROOT})
  else()
    # Default to toolchain cmake file
    set(TOOLCHAIN_ROOT ${ZEPHYR_BASE})
  endif()
endif()
set(TOOLCHAIN_ROOT ${TOOLCHAIN_ROOT} CACHE STRING "Path to folder. Available toolchains are the cmake files contained within the folder")
assert(TOOLCHAIN_ROOT "Zephyr toolchain root path invalid: please set the TOOLCHAIN_ROOT-variable")


##
## Determine ZEPHYR_TOOLCHAIN_VARIANT
##

# Backwards compatibility with ZEPHYR_GCC_VARIANT variable; until we completely remove it
if(NOT DEFINED ENV{ZEPHYR_TOOLCHAIN_VARIANT})
  if(DEFINED ENV{ZEPHYR_GCC_VARIANT})
    message(WARNING "ZEPHYR_GCC_VARIANT is deprecated, please use ZEPHYR_TOOLCHAIN_VARIANT instead")
    set(ZEPHYR_TOOLCHAIN_VARIANT $ENV{ZEPHYR_GCC_VARIANT})
  endif()
endif()

# Backwards compatibility with gccarmemb toolchain; until we completely remove it
if("${ZEPHYR_TOOLCHAIN_VARIANT}" STREQUAL "gccarmemb")
  message(WARNING "gccarmemb is deprecated, please use gnuarmemb instead")
  set(ZEPHYR_TOOLCHAIN_VARIANT "gnuarmemb")
endif()

# Special override for cross-compile
if(NOT ZEPHYR_TOOLCHAIN_VARIANT)
  if(DEFINED ENV{ZEPHYR_TOOLCHAIN_VARIANT})
    set(ZEPHYR_TOOLCHAIN_VARIANT $ENV{ZEPHYR_TOOLCHAIN_VARIANT})
  elseif(CROSS_COMPILE OR (DEFINED ENV{CROSS_COMPILE}))
    set(ZEPHYR_TOOLCHAIN_VARIANT cross-compile)
  endif()
endif()

# Special override for POSIX arch
if(CONFIG_ARCH_POSIX)
  if(NOT (ZEPHYR_TOOLCHAIN_VARIANT STREQUAL "host"))
    set(ZEPHYR_TOOLCHAIN_VARIANT "host")
  endif()
endif()

set(ZEPHYR_TOOLCHAIN_VARIANT ${ZEPHYR_TOOLCHAIN_VARIANT} CACHE STRING "Name-identifier of toolchain used")
assert(ZEPHYR_TOOLCHAIN_VARIANT "Zephyr toolchain variant invalid: please set the ZEPHYR_TOOLCHAIN_VARIANT-variable")


##
## Load toolchain: compiler, linker and bintools
##

# We refer to the toolchain in use via ZEPHYR_TOOLCHAIN_VARIANT.
# Roughly speaking, a toolchain consists of a compiler, linker and utils.
# Thus we leave it up to ${ZEPHYR_TOOLCHAIN_VARIANT}.cmake to determine:
#  * Which ${COMPILER} to use
#  * Which ${LINKER} to use
include(${TOOLCHAIN_ROOT}/cmake/toolchain/${ZEPHYR_TOOLCHAIN_VARIANT}.cmake)

# Backwards-compatibility: Not all toolchains may yet set LINKER
if (NOT LINKER)
  message(WARNING "LINKER not set: Assuming GNU ld. LINKER must be set by toolchain/${ZEPHYR_TOOLCHAIN_VARIANT}.cmake")
  set(LINKER "ld")
endif()

# Backwards-compatibility: Not all toolchains may yet set BINTOOLS
if (NOT BINTOOLS)
  message(WARNING "BINTOOLS not set: Assuming GNU binutils. BINTOOLS must be set by toolchain/${ZEPHYR_TOOLCHAIN_VARIANT}.cmake")
  set(BINTOOLS "binutils")
endif()

include(${TOOLCHAIN_ROOT}/cmake/compiler/${COMPILER}.cmake)
include(${TOOLCHAIN_ROOT}/cmake/linker/${LINKER}.cmake)
include(${TOOLCHAIN_ROOT}/cmake/bintools/${BINTOOLS}.cmake)


# Uniquely identify the toolchain wrt. it's capabilities.
#
# What we are looking for, is a signature definition that is defined
# like this: Toolchains with the same signature will always support the same
# set of flags.
# It is not clear how this signature should be constructed. The
# strategy chosen is to md5sum the CC binary.
file(MD5 ${CMAKE_C_COMPILER} CMAKE_C_COMPILER_MD5_SUM)
set(TOOLCHAIN_SIGNATURE ${CMAKE_C_COMPILER_MD5_SUM})
