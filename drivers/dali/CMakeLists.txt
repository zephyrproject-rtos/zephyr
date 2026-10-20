# Copyright (c) 2025 by Sven Hädrich <sven.haedrich@sevenlab.de>
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)
# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_DALI_PWM      dali_pwm.c)
# zephyr-keep-sorted-stop
