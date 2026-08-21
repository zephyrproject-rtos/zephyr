# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0
#
# Sysbuild-level ordering for Espressif QEMU images.
#
# QEMU boots one merged SPI flash image. Under MCUboot that image is assembled
# in the application domain from the application binary plus the bootloader
# binary of the sibling mcuboot domain. Sysbuild adds each image as an
# independent ExternalProject with no build dependency between them, so the two
# domains build concurrently and the merge can read a missing or partially
# written mcuboot binary. Order the application after the bootloader to make
# the merge deterministic.
#
# Do not gate on CONFIG_ESPRESSIF_QEMU here: this file runs in the sysbuild
# CMake context, where that image Kconfig is not reliably visible. Checking
# that both ExternalProject targets exist is enough; the dependency is
# harmless for non-QEMU MCUboot builds on these boards.

if(TARGET mcuboot AND TARGET ${DEFAULT_IMAGE})
  add_dependencies(${DEFAULT_IMAGE} mcuboot)
endif()
