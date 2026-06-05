# Copyright (c) 2024 Alexander Kozhinov <ak.alexander.kozhinov@gmail.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

# CORDIC subsystem driver files
# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_CORDIC_STM32 cordic_stm32.c)
# zephyr-keep-sorted-stop
