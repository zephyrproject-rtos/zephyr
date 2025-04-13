# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_SOC_ESP32C6_HPCORE
  vectors.S
  soc_irq.S
  soc.c
  ../common/loader.c
  )

zephyr_sources_ifdef(CONFIG_SOC_ESP32C6_LPCORE
  vector_table_lpcore.S
  vectors_lpcore.S
  start_lpcore.S
  soc_lpcore.c
  )

zephyr_include_directories(.)

if(CONFIG_SOC_ESP32C6_HPCORE)
  zephyr_sources_ifndef(CONFIG_BOOTLOADER_MCUBOOT hw_init.c)
  zephyr_library_sources_ifdef(CONFIG_PM power.c)
  zephyr_library_sources_ifdef(CONFIG_POWEROFF poweroff.c)
  zephyr_sources_ifdef(CONFIG_ULP_COPROC_ENABLED hpcore_init_ulp.c)
endif()
