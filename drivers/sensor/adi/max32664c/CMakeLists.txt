# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_sources_ifdef(CONFIG_MAX32664C_USE_FIRMWARE_LOADER max32664c_bl.c)

zephyr_sources_ifdef(CONFIG_MAX32664C_USE_INTERRUPT max32664c_interrupt.c)

zephyr_sources_ifdef(CONFIG_SENSOR_MAX32664C max32664c.c
                                             max32664c_worker.c
                                             max32664c_init.c
                                             max32664c_acc.c
                                             )
