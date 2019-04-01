#
# Copyright (c) 2019, Microchip Technology Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories_ifdef(CONFIG_SOC_SERIES_MEC1701X .)

zephyr_include_directories_ifdef(CONFIG_SOC_SERIES_MEC1501X common)
zephyr_include_directories_ifdef(CONFIG_SOC_SERIES_MEC1501X mec1501)
