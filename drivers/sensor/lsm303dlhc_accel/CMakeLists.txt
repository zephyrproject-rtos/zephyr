#
# Copyright (c) 2018 Philémon Jaermann
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources_ifdef(CONFIG_LSM303DLHC_ACCEL lsm303dlhc_accel.c)
