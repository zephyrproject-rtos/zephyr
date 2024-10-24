# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/llvm/generic.cmake)

set(TOOLCHAIN_KCONFIG_DIR ${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr)

message(STATUS "Found toolchain: zephyr-sdk-llvm ${SDK_VERSION} (${ZEPHYR_SDK_INSTALL_DIR})")
