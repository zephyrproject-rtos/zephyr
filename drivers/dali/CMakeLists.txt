# Copyright (c) 2025 by Sven Hädrich <sven.haedrich@sevenlab.de>
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/dali.h)

zephyr_library()

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_DALI_COUNTER  dali_counter.c)
zephyr_library_sources_ifdef(CONFIG_DALI_LPC11U6X dali_lpc11u6x.c)
zephyr_library_sources_ifdef(CONFIG_DALI_PWM      dali_pwm.c)
# zephyr-keep-sorted-stop
