# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/generic.cmake)

set(TOOLCHAIN_KCONFIG_DIR ${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr)

# Zephyr SDK < 0.17.1 does not set TOOLCHAIN_HAS_GLIBCXX
set(TOOLCHAIN_HAS_GLIBCXX ON CACHE BOOL "True if toolchain supports libstdc++")

message(STATUS "Found toolchain: zephyr ${SDK_VERSION} (${ZEPHYR_SDK_INSTALL_DIR})")
