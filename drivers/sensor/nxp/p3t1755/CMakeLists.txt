# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(p3t1755.c)

zephyr_library_sources_ifdef(CONFIG_EMUL_SENSOR_REGMAP p3t1755_emul.c)
