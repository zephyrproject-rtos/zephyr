# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0
#
# Sysbuild-level ordering and MCUboot Kconfig for Espressif QEMU images.
#
# DevKitC sysbuild.cmake includes this file for every variant of the board.
# Nothing here applies to a hardware qualifier: hardware already loads
# boot/zephyr/socs/<soc>.conf through CONF_FILE, and it does not merge a
# second flash image.
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
# CMake context, where that image Kconfig is not reliably visible. The board
# qualifier is.
#
# MCUboot looks up boot/zephyr/socs/<qualifiers>.conf using the full board
# qualifier. /qemu variants therefore search for files such as
# esp32_procpu_qemu.conf, which are not in the MCUboot module.
#
# Sysbuild has already added <application>/sysbuild/mcuboot.conf to
# mcuboot_EXTRA_CONF_FILE when that file exists (see
# share/sysbuild/cmake/modules/sysbuild_extensions.cmake). That path belongs
# to the application being built. This file does not read or replace it.
# In-tree fragments are:
#   tests/boot/test_mcuboot/sysbuild/mcuboot.conf
#   tests/boot/mcuboot_multiple_keys/sysbuild/mcuboot.conf
#   samples/sysbuild/with_mcuboot/sysbuild/mcuboot.conf
#   samples/subsys/lorawan/fuota/sysbuild/mcuboot.conf
# Prepend the hardware SoC fragment so each of those stays later and can
# still override it. Applications with no sysbuild/mcuboot.conf only receive
# the SoC fragment.

if(NOT "${BOARD_QUALIFIERS}" MATCHES "(^|/)qemu$")
  return()
endif()

if(TARGET mcuboot)
  string(REPLACE "/" "_" _esp_qemu_mcuboot_socs "${BOARD_QUALIFIERS}")
  string(REGEX REPLACE "^_" "" _esp_qemu_mcuboot_socs "${_esp_qemu_mcuboot_socs}")
  string(REGEX REPLACE "_qemu$" "" _esp_qemu_mcuboot_socs "${_esp_qemu_mcuboot_socs}")
  set(_esp_qemu_mcuboot_conf
    "${ZEPHYR_MCUBOOT_MODULE_DIR}/boot/zephyr/socs/${_esp_qemu_mcuboot_socs}.conf")
  if(NOT EXISTS "${_esp_qemu_mcuboot_conf}")
    message(FATAL_ERROR
      "Espressif QEMU: MCUboot SoC fragment not found: ${_esp_qemu_mcuboot_conf}")
  endif()
  list(PREPEND mcuboot_EXTRA_CONF_FILE "${_esp_qemu_mcuboot_conf}")
  list(REMOVE_DUPLICATES mcuboot_EXTRA_CONF_FILE)
  set(mcuboot_EXTRA_CONF_FILE "${mcuboot_EXTRA_CONF_FILE}" CACHE INTERNAL
    "Hardware MCUboot socs fragment (same settings as the non-/qemu qualifier)")
endif()

if(TARGET mcuboot AND TARGET ${DEFAULT_IMAGE})
  add_dependencies(${DEFAULT_IMAGE} mcuboot)
endif()
