# Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_BUZZER_GPIO buzzer_gpio.c)
zephyr_library_sources_ifdef(CONFIG_BUZZER_PWM  buzzer_pwm.c)
