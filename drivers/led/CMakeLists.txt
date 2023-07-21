# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/led.h)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_HT16K33 ht16k33.c)
zephyr_library_sources_ifdef(CONFIG_IS31FL3216A is31fl3216a.c)
zephyr_library_sources_ifdef(CONFIG_LED_GPIO led_gpio.c)
zephyr_library_sources_ifdef(CONFIG_LED_NPM1300 led_npm1300.c)
zephyr_library_sources_ifdef(CONFIG_LED_PWM led_pwm.c)
zephyr_library_sources_ifdef(CONFIG_LED_XEC led_mchp_xec.c)
zephyr_library_sources_ifdef(CONFIG_LP3943 lp3943.c)
zephyr_library_sources_ifdef(CONFIG_LP50XX lp50xx.c)
zephyr_library_sources_ifdef(CONFIG_LP5562 lp5562.c)
zephyr_library_sources_ifdef(CONFIG_LP5569 lp5569.c)
zephyr_library_sources_ifdef(CONFIG_NCP5623 ncp5623.c)
zephyr_library_sources_ifdef(CONFIG_PCA9633 pca9633.c)
zephyr_library_sources_ifdef(CONFIG_TLC59108 tlc59108.c)
zephyr_library_sources_ifdef(CONFIG_IS31FL3733 is31fl3733.c)

zephyr_library_sources_ifdef(CONFIG_LED_SHELL   led_shell.c)

zephyr_library_sources_ifdef(CONFIG_USERSPACE   led_handlers.c)
