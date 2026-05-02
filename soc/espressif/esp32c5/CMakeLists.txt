# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_SOC_ESP32C5_HPCORE
  vectors.S
  soc_irq.S
  soc.c
  ../common/loader.c
  )

zephyr_sources_ifdef(CONFIG_SOC_ESP32C5_LPCORE
  vector_table_lpcore.S
  vectors_lpcore.S
  start_lpcore.S
  soc_lpcore.c
  )

zephyr_include_directories(.)

if(CONFIG_SOC_ESP32C5_HPCORE)
  zephyr_sources_ifndef(CONFIG_BOOTLOADER_MCUBOOT hw_init.c)
  zephyr_library_sources_ifdef(CONFIG_PM ../common/power.c)
  zephyr_library_sources_ifdef(CONFIG_POWEROFF ../common/poweroff.c)
  zephyr_sources_ifdef(CONFIG_ESP32_ULP_COPROC_ENABLED hpcore_init_ulp.c)
  zephyr_sources_ifdef(CONFIG_RISCV_PMP pmp_regions.c)
endif()
