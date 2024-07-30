# Copyright (c) 2024 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_ADC_CMP_NPCX nuvoton_adc_cmp_npcx)
add_subdirectory_ifdef(CONFIG_TACH_NPCX nuvoton_tach_npcx)
# zephyr-keep-sorted-stop
