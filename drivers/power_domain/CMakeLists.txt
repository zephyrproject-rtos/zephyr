# Copyright (c) 2022, CSIRO
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_POWER_DOMAIN_GPIO power_domain_gpio.c)
zephyr_library_sources_ifdef(CONFIG_POWER_DOMAIN_GPIO_MONITOR power_domain_gpio_monitor.c)
zephyr_library_sources_ifdef(CONFIG_POWER_DOMAIN_INTEL_ADSP power_domain_intel_adsp.c)
