# SPDX-License-Identifier: Apache-2.0

# Purpose of the generic.cmake is to define a generic C compiler which can be
# used for devicetree pre-processing and other pre-processing tasks which must
# be performed before the target can be determined.

# Todo: deprecate CLANG_ROOT_DIR
set_ifndef(LLVM_TOOLCHAIN_PATH "$ENV{CLANG_ROOT_DIR}")
zephyr_get(LLVM_TOOLCHAIN_PATH)

if(LLVM_TOOLCHAIN_PATH)
  set(TOOLCHAIN_HOME ${LLVM_TOOLCHAIN_PATH}/bin/)
endif()

set(LLVM_TOOLCHAIN_PATH ${CLANG_ROOT_DIR} CACHE PATH "clang install directory")

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
  file(GLOB_RECURSE newlib_header ${LLVM_TOOLCHAIN_PATH}/_newlib_version.h)
  if(newlib_header)
    set(TOOLCHAIN_HAS_NEWLIB ON CACHE BOOL "True if toolchain supports newlib")
  endif()

  # Support for picolibc is indicated by the presence of 'picolibc.h' in the toolchain path.
  file(GLOB_RECURSE picolibc_header ${LLVM_TOOLCHAIN_PATH}/picolibc.h)
  if(picolibc_header)
    set(TOOLCHAIN_HAS_PICOLIBC ON CACHE BOOL "True if toolchain supports picolibc")
  endif()
endif()

message(STATUS "Found toolchain: llvm (clang/ld)")
