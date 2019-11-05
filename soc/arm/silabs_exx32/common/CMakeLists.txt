# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c soc_gpio.c)

zephyr_sources_ifdef(CONFIG_SYS_POWER_MANAGEMENT soc_power.c)

zephyr_sources_ifdef(CONFIG_ARM_MPU arm_mpu_regions.c)
