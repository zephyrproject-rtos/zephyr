#
# SPDX-FileCopyrightText: Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()
zephyr_include_directories(.)

# Boot data the ROM reads before the image runs: the flash configuration block
# for the NOR and the external memory configuration data that brings up the
# XSPI1 PSRAM.
if(CONFIG_NXP_IMXRT_BOOT_HEADER)
  zephyr_library_sources(xip/mimxrt2660_evk_xspi_nor_config.c)
endif()
