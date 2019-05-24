# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_TIMER_RANDOM_GENERATOR          rand32_timer.c)
zephyr_sources_ifdef(CONFIG_X86_TSC_RANDOM_GENERATOR        rand32_timestamp.c)
zephyr_sources_ifdef(CONFIG_ENTROPY_DEVICE_RANDOM_GENERATOR rand32_entropy_device.c)
zephyr_sources_ifdef(CONFIG_XOROSHIRO_RANDOM_GENERATOR      rand32_xoroshiro128.c)
