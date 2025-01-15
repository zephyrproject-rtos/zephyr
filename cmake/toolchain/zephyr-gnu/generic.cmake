# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/gnu/generic.cmake)

set(TOOLCHAIN_KCONFIG_DIR ${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr)

message(STATUS "Found toolchain: zephyr-gnu ${SDK_VERSION} (${ZEPHYR_SDK_INSTALL_DIR})")
