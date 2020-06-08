#
#  Copyright (c) 2020 Antmicro <www.antmicro.com>
#
#  SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ADXL345 adxl345.c)
