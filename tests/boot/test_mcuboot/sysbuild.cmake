# Copyright 2022 NXP
# SPDX-License-Identifier: Apache-2.0

if(NOT SB_FULL_CONFIG)
  message(ERROR "Please pass -DSB_FULL_CONFIG=y when building this sample")
endif()

if (SB_CONFIG_BOOTLOADER_MCUBOOT)
  # Add the mcuboot key file to the secondary swapped app
  # This must be done here to ensure that the same key file is used for signing
  # both the primary and secondary apps
  set(swapped_app_CONFIG_MCUBOOT_SIGNATURE_KEY_FILE
      \"${SB_CONFIG_BOOT_SIGNATURE_KEY_FILE}\" CACHE STRING
      "Signature key file for signing" FORCE)

  # Add the swapped app to the build
  ExternalZephyrProject_Add(
    APPLICATION swapped_app
    SOURCE_DIR ${APP_DIR}/swapped_app
  )

  # Add the swapped app to the list of images to flash
  # Ensure the order of images is as follows:
  # - mcuboot
  # - swapped app
  # - primary app (test_mcuboot)
  # This order means that if the debugger resets the MCU in between flash
  # iterations, the MCUBoot swap won't be triggered until the secondary app
  # is actually present in flash.
  set(IMAGES "mcuboot" "swapped_app" "test_mcuboot")
else()
  message(WARNING "Target board ${BOARD} does not support mcuboot")
endif()
