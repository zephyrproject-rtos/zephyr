#
# Copyright (c) 2020 Linumiz
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_HAWKBIT hawkbit.c)
zephyr_library_sources_ifdef(CONFIG_HAWKBIT_AUTOHANDLER hawkbit_autohandler.c)
zephyr_library_sources_ifdef(CONFIG_HAWKBIT hawkbit_device.c)
zephyr_library_sources_ifdef(CONFIG_HAWKBIT hawkbit_firmware.c)
zephyr_library_sources_ifdef(CONFIG_HAWKBIT_SHELL shell.c)

zephyr_library_link_libraries(MCUBOOT_BOOTUTIL)
