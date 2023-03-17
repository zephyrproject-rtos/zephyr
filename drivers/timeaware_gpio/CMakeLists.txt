# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_TIMEAWARE_GPIO_INTEL timeaware_gpio_intel.c)
if(CONFIG_TIMEAWARE_GPIO_INTEL)
zephyr_library_sources_ifdef(CONFIG_USERSPACE  timeaware_gpio_handlers.c)
endif()
