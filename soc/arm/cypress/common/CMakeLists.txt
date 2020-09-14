# Copyright (c) 2020 Linaro Ltd.
# Copyright (c) 2020 ATL Electronics
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_PSOC6 soc_gpio.c)
