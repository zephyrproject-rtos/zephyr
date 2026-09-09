# Copyright (c) 2024 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_SOC_SERIES_SIWG917 soc.c)
zephyr_sources_ifdef(CONFIG_SILABS_SIWX91X_NWP siwx91x_nwp.c)
zephyr_sources_ifdef(CONFIG_PM power.c)
zephyr_sources_ifdef(CONFIG_SIWX91X_FIRMWARE_UPGRADE_SHELL siwx91x_fota_shell.c)
zephyr_sources_ifdef(CONFIG_SIWX91X_VERSION_SHELL siwx91x_version_shell.c)

zephyr_compile_definitions_ifdef(CONFIG_SOC_SIWG917Y111MGABA
  SI91X_32kHz_EXTERNAL_OSCILLATOR=1
  SL_SI91X_ACX_MODULE=1
  SLI_SI91X_MCU_EXTERNAL_LDO_FOR_PSRAM=1
)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
