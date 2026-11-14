# Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_MEMREF memref.c)
zephyr_sources_ifdef(CONFIG_MEMREF memref_variable.c)
zephyr_sources_ifdef(CONFIG_MEMREF memref_block.c)
