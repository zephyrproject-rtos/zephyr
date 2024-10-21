# SPDX-License-Identifier: Apache-2.0

find_package(Deprecated COMPONENTS ZEPHYR)

include(${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr/generic.cmake)

set(TOOLCHAIN_KCONFIG_DIR ${ZEPHYR_SDK_INSTALL_DIR}/cmake/zephyr)

message(STATUS "Found toolchain: zephyr-sdk-gnu ${SDK_VERSION} (${ZEPHYR_SDK_INSTALL_DIR})")
