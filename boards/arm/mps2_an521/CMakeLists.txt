#
# Copyright (c) 2019,2020 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()
zephyr_library_sources(pinmux.c)

if(CONFIG_SOC_MPS2_AN521_CPU1 AND NOT CONFIG_OPENAMP)
# Building a firmware image for CPU1: this requires a binary
# for CPU0, which will boot the device and wake up CPU1.
# However, if building with OPENAMP, there is no need to build
# any binary for CPU0, as this is built by the dual core sample.

  set(CPU0_BINARY_DIR ${BOARD_DIR}/empty_cpu0-prefix/src/empty-cpu0-build/zephyr)

  include(ExternalProject)

  ExternalProject_Add(
    empty_cpu0
    SOURCE_DIR ${BOARD_DIR}/empty_cpu0
    INSTALL_COMMAND  ""
    CMAKE_CACHE_ARGS -DBOARD:STRING=mps2_an521
    BUILD_BYPRODUCTS "${CPU0_BINARY_DIR}/${KERNEL_BIN_NAME}"
    BUILD_ALWAYS True
  )

endif()
