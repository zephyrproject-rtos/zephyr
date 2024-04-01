# Copyright (c) 2024 Freedom Veiculos Eletricos
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(freq_meter_stm32_icap.c)
zephyr_library_sources_ifdef(CONFIG_FREQ_METER_STM32_ICAP_GEN freq_meter_stm32_icap_gen.c)
