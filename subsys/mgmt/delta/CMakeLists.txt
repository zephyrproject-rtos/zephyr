# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(delta)

zephyr_library_sources_ifdef(CONFIG_DELTA_UPDATE delta_mem.c)
zephyr_library_sources_ifdef(CONFIG_BSDIFF_HS_ALGO backend_diff/delta_backend_bsdiff.c)
