# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_CRC_HARDWARE_ACCELERATOR crc_new_device.c)
zephyr_library_sources_ifdef(CONFIG_CRC_SOFTWARE_BASED crc_new_software.c)
