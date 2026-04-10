# Copyright (c) 2024 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SILABS_SIWX91X_NWP)
  zephyr_sources(
    siwx91x_nwp.c
    siwx91x_nwp_bus.c
    siwx91x_nwp_api.c
    siwx91x_nwp_fw.c
    siwx91x_nwp_fw_version.c)
  zephyr_include_directories(.)
endif()

if(CONFIG_SIWX91X_NWP_RELOCATION_SRAM)
  zephyr_code_relocate(FILES siwx91x_nwp_bus.c LOCATION RAM)
endif()
