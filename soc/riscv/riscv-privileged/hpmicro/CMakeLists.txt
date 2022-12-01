# Copyright (c) 2022 HPMicro
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  start.S
  soc.c
)

if(CONFIG_SOC_ANDES_V5_HWDSP)
  zephyr_cc_option(-mext-dsp)
endif()
