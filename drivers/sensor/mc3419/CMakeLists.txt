# MC3419 3-axis accelerometer
#
# Cpoyright (c) 2023, Linumiz
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(mc3419.c)
zephyr_library_sources_ifdef(CONFIG_MC3419_TRIGGER mc3419_trigger.c)
