#  Copyright (c) 2021 STMicroelectronics
#  SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_ADC board_adc_vref.c)
