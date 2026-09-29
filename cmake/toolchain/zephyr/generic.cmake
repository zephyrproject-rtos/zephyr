# SPDX-License-Identifier: Apache-2.0

if(TOOLCHAIN_VARIANT_COMPILER STREQUAL "gnu" OR
    NOT DEFINED TOOLCHAIN_VARIANT_COMPILER)
  if(EXISTS ${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/gnu/generic.cmake)
    include(${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/gnu/generic.cmake)
  else()
    include(${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/generic.cmake)
  endif()
  set(TOOLCHAIN_VARIANT_COMPILER "gnu" CACHE STRING "compiler used by the toolchain variant" FORCE)

  set(TOOLCHAIN_KCONFIG_DIR ${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr)
  # Zephyr SDK < 0.17.1 does not set TOOLCHAIN_HAS_GLIBCXX
  set(TOOLCHAIN_HAS_GLIBCXX ON CACHE BOOL "True if toolchain supports libstdc++")

  # ld.lld, when the SDK ships it, lives in the SDK's optional LLVM
  # component, not in the GNU per-target tree.
  set(_llext_lld_path ${ZEPHYR_SDK_INSTALL_DIR}/llvm/bin)
  message(STATUS "Found toolchain: zephyr ${SDK_VERSION} (${ZEPHYR_SDK_INSTALL_DIR})")
elseif(TOOLCHAIN_VARIANT_COMPILER STREQUAL "llvm")
  include(${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/llvm/generic.cmake)
  set(TOOLCHAIN_VARIANT_COMPILER "llvm" CACHE STRING "compiler used by the toolchain variant" FORCE)

  set(TOOLCHAIN_KCONFIG_DIR ${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr)
  set(_llext_lld_path ${TOOLCHAIN_HOME})
  message(STATUS "Found toolchain: zephyr ${SDK_VERSION} (${ZEPHYR_SDK_INSTALL_DIR})")
else()
  message(FATAL_ERROR "Unsupported TOOLCHAIN_VARIANT_COMPILER: ${TOOLCHAIN_VARIANT_COMPILER}")
endif()

# Respect an explicit cache override so a toolchain can disable LLD even when
# an unrelated host installation is on PATH.
if(NOT DEFINED TOOLCHAIN_HAS_LLEXT_LLD)
  find_program(TOOLCHAIN_LLEXT_LLD ld.lld PATHS ${_llext_lld_path} NO_DEFAULT_PATH)
  if(TOOLCHAIN_LLEXT_LLD)
    set(TOOLCHAIN_HAS_LLEXT_LLD ON CACHE BOOL
        "True if toolchain provides ld.lld for LLEXT")
  else()
    set(TOOLCHAIN_HAS_LLEXT_LLD OFF CACHE BOOL
        "True if toolchain provides ld.lld for LLEXT")
  endif()
endif()

if(TOOLCHAIN_HAS_LLEXT_LLD)
  set(TOOLCHAIN_LLEXT_FLAGS -fuse-ld=lld)
endif()

unset(TOOLCHAIN_LLEXT_LLD CACHE)
unset(TOOLCHAIN_LLEXT_LLD)
