#
# Copyright (c) 2020 Bosch Sensortec GmbH
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_BMA400 bma400.c)
