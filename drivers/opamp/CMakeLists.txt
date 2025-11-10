# Copyright 2025 NXP
# Copyright (c) 2025 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_OPAMP_MCUX_OPAMP opamp_mcux_opamp.c)
zephyr_library_sources_ifdef(CONFIG_OPAMP_MCUX_OPAMP_FAST opamp_mcux_opamp_fast.c)
zephyr_library_sources_ifdef(CONFIG_OPAMP_STM32 opamp_stm32_opamp.c)
# zephyr-keep-sorted-stop
