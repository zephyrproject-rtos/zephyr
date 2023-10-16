# SPDX-License-Identifier: Apache-2.0

# Define the toolchain to be used (by eg CI)
if(${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "zephyr")
  set(COMPILER_FULL_PATH ${ZEPHYR_SDK_INSTALL_DIR}/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc)
elseif(${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "gnuarmemb")
  set(COMPILER_FULL_PATH ${GNUARMEMB_TOOLCHAIN_PATH}/bin/arm-none-eabi-gcc)
endif()

zephyr_library_sources(board.c)
