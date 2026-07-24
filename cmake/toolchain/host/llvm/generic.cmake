
# Todo: deprecate CLANG_ROOT_DIR
set_ifndef(LLVM_TOOLCHAIN_PATH "$ENV{CLANG_ROOT_DIR}")
set(TOOLCHAIN_VARIANT_COMPILER llvm CACHE STRING "Variant compiler being used")
zephyr_get(LLVM_TOOLCHAIN_PATH)

if(NOT LLVM_TOOLCHAIN_PATH)
  # Generate search paths dynamically to support LLVM versions 15 through 25
  set(LLVM_SEARCH_PATHS
    /opt/llvm/bin
    /usr/bin
    /usr/local/bin
    /bin
    /opt/homebrew/bin
    /opt/homebrew/opt/llvm/bin
    /usr/local/opt/llvm/bin
    /usr/lib/llvm/bin
  )
  foreach(ver RANGE 25 15 -1)
    list(APPEND LLVM_SEARCH_PATHS "/opt/homebrew/opt/llvm@${ver}/bin")
    list(APPEND LLVM_SEARCH_PATHS "/usr/lib/llvm-${ver}/bin")
  endforeach()

  # Find the host LLVM toolchain directory containing clang / llvm-config
  find_path(LLVM_BINARY_DIR
    NAMES
    clang
    llvm-config

    PATHS
    ${LLVM_SEARCH_PATHS}

    HINTS
    $ENV{LLVM_TOOLCHAIN_PATH}/bin
    $ENV{LLVM_TOOLCHAIN_PATH}
    $ENV{LLVM_ROOT}/bin
    $ENV{LLVM_ROOT}
  )
  if(LLVM_BINARY_DIR)
    get_filename_component(LLVM_TOOLCHAIN_PATH "${LLVM_BINARY_DIR}" DIRECTORY)
  endif()
endif()

if(LLVM_TOOLCHAIN_PATH)
  set(TOOLCHAIN_HOME ${LLVM_TOOLCHAIN_PATH}/bin/)
endif()

set(LLVM_TOOLCHAIN_PATH ${LLVM_TOOLCHAIN_PATH} CACHE PATH "clang install directory")

set(COMPILER clang)
set(BINTOOLS llvm)

# LLVM is flexible, meaning that it can in principle always support newlib or picolibc.
# This is not decided by LLVM itself, but depends on libraries distributed with  the installation.
# Also newlib or picolibc may be created as add-ons. Thus always stating that LLVM does not have
# newlib or picolibc would be wrong. Same with stating that LLVM has newlib or Picolibc.
# The best assumption for TOOLCHAIN_HAS_<NEWLIB|PICOLIBC> is to check for the  presence of
# '_newlib_version.h' / 'picolibc' and have the default value set accordingly.
# This provides a best effort mechanism to allow developers to have the newlib C / Picolibc library
# selection available in Kconfig.
# Developers can manually indicate library support with '-DTOOLCHAIN_HAS_<NEWLIB|PICOLIBC>=<ON|OFF>'

# Support for newlib is indicated by the presence of '_newlib_version.h' in the toolchain path.
if(NOT LLVM_TOOLCHAIN_PATH STREQUAL "")
  # Use find_file() instead of GLOB_RECURSE to avoid a slow recursive scan
  # of the entire LLVM toolchain directory tree at configure time.
  find_file(newlib_header _newlib_version.h
    PATHS ${LLVM_TOOLCHAIN_PATH}
    PATH_SUFFIXES include
    NO_DEFAULT_PATH
    NO_CACHE
  )
  if(newlib_header)
    set(TOOLCHAIN_HAS_NEWLIB ON CACHE BOOL "True if toolchain supports newlib")
  endif()

  find_file(picolibc_header picolibc.h
    PATHS ${LLVM_TOOLCHAIN_PATH}
    PATH_SUFFIXES include include/picolibc
    NO_DEFAULT_PATH
    NO_CACHE
  )
  if(picolibc_header OR NOT "${ARCH}" STREQUAL "posix")
    set(TOOLCHAIN_HAS_PICOLIBC ON CACHE BOOL "True if toolchain supports picolibc")
  endif()
endif()
set(TOOLCHAIN_HAS_LIBCXX ON CACHE BOOL "True if toolchain supports libc++")

message(STATUS "Found toolchain: llvm (clang/ld)")
