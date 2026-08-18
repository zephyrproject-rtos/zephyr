# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0
#
# Sysbuild-level ordering and MCUboot Kconfig for Espressif QEMU images.
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
#
# MCUboot looks up boot/zephyr/socs/<qualifiers>.conf using the full board
# qualifier. /qemu variants therefore search for files such as
# esp32_procpu_qemu.conf, which are not in the MCUboot module. Point
# mcuboot_EXTRA_CONF_FILE at the existing hardware fragment (esp32_procpu.conf
# and the S3/C3/C6 siblings) so /qemu sysbuild gets the same Espressif profile
# without adding files outside this repository.

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
  list(APPEND mcuboot_EXTRA_CONF_FILE "${_esp_qemu_mcuboot_conf}")
  list(REMOVE_DUPLICATES mcuboot_EXTRA_CONF_FILE)
  set(mcuboot_EXTRA_CONF_FILE "${mcuboot_EXTRA_CONF_FILE}" CACHE INTERNAL
    "Hardware MCUboot socs fragment (same settings as the non-/qemu qualifier)")
endif()

if(TARGET mcuboot AND TARGET ${DEFAULT_IMAGE})
  add_dependencies(${DEFAULT_IMAGE} mcuboot)
endif()
