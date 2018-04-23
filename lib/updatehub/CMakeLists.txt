#
# Copyright (c) 2018 O.S.Systems
#
# SPDX -License-Identifier: Apache-2.0
#

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_UPDATEHUB updatehub.c)
zephyr_library_sources_ifdef(CONFIG_UPDATEHUB updatehub_device.c)
zephyr_library_sources_ifdef(CONFIG_UPDATEHUB updatehub_firmware.c)
zephyr_library_sources_ifdef(CONFIG_UPDATEHUB_SHELL shell.c)
