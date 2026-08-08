# Copyright (c) 2026 Silicon Signals Pvt. Ltd.
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_NEOCHROM_GPU2D st_neochrom_gpu2d.c)
