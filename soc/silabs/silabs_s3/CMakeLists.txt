# Copyright (c) 2024 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_sources_ifdef(CONFIG_SOC_GECKO_USE_RAIL soc_radio.c)
if(CONFIG_SILABS_SISDK_PROTOCOL_CRYPTO OR CONFIG_MBEDTLS)
  zephyr_sources(soc_crypto.c)
endif()

zephyr_code_relocate(FILES ${ZEPHYR_BASE}/arch/arm/core/cortex_m/isr_wrapper.c LOCATION RAM)
zephyr_code_relocate(FILES ${ZEPHYR_BASE}/arch/arm/core/cortex_m/exc_exit.c LOCATION RAM)
