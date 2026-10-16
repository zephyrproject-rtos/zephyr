# Copyright (c) 2025 Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_SOC_FAMILY_MICROCHIP_PIC32CX_BZ6      bz6x)

if(CONFIG_DEVICE_SUPPORT_REQUIRED)
  zephyr_blobs_verify(MODULE hal_microchip REQUIRED)
endif()
