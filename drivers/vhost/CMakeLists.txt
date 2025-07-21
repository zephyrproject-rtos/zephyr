# Copyright (c) 2025 TOKITA Hiroshi
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_VHOST vringh.c)
