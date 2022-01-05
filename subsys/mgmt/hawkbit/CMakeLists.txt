#
# Copyright (c) 2020 Linumiz
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

zephyr_library_sources(hawkbit.c)
zephyr_library_sources(hawkbit_device.c)
zephyr_library_sources(hawkbit_firmware.c)
zephyr_library_sources(hawkbit_config_data.c)
zephyr_library_sources_ifdef(CONFIG_HAWKBIT_SHELL shell.c)
