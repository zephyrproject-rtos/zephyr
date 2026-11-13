# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(generic_emul.c)
zephyr_library_sources_ifdef(CONFIG_SENSOR_ASYNC_API generic_emul_async.c)
