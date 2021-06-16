# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_HT16K33 ht16k33.c)
zephyr_sources_ifdef(CONFIG_LED_GPIO led_gpio.c)
zephyr_sources_ifdef(CONFIG_LED_PWM led_pwm.c)
zephyr_sources_ifdef(CONFIG_LP3943 lp3943.c)
zephyr_sources_ifdef(CONFIG_LP503X lp503x.c)
zephyr_sources_ifdef(CONFIG_LP5562 lp5562.c)
zephyr_sources_ifdef(CONFIG_PCA9633 pca9633.c)

zephyr_sources_ifdef(CONFIG_LED_SHELL   led_shell.c)

zephyr_sources_ifdef(CONFIG_USERSPACE   led_handlers.c)
