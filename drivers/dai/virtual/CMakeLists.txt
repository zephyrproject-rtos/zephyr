# Copyright (c) 2025 Suraj Sonawane <surajsonawane0215@gmail.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_DAI_VIRTUAL	virtual_dai.c)
