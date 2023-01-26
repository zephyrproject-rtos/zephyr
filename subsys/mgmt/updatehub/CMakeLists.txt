#
# Copyright (c) 2018-2023 O.S.Systems
#
# SPDX -License-Identifier: Apache-2.0
#

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_UPDATEHUB updatehub.c)
zephyr_library_sources_ifdef(CONFIG_UPDATEHUB updatehub_device.c)
zephyr_library_sources_ifdef(CONFIG_UPDATEHUB updatehub_firmware.c)
zephyr_library_sources_ifdef(CONFIG_UPDATEHUB updatehub_timer.c)
zephyr_library_sources_ifdef(CONFIG_UPDATEHUB updatehub_integrity.c)
zephyr_library_sources_ifdef(CONFIG_UPDATEHUB updatehub_storage.c)
zephyr_library_sources_ifdef(CONFIG_UPDATEHUB_SHELL shell.c)

zephyr_library_sources_ifdef(CONFIG_USERSPACE updatehub_handlers.c)

zephyr_include_directories(
  ${ZEPHYR_BASE}/subsys/mgmt/updatehub/include
)
