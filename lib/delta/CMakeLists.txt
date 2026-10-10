# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(delta)
zephyr_library_sources_ifdef(CONFIG_DELTA delta.c)

add_subdirectory_ifdef(CONFIG_DELTA_BACKEND_BSDIFFHS backends/bsdiffhs)
