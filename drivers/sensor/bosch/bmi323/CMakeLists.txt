# Copyright (c) 2023 Trackunit Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(bmi323.c)
zephyr_library_sources_ifdef(CONFIG_BMI323_BUS_SPI bmi323_spi.c)
