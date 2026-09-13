# Copyright (c) 2025 TOKITA Hiroshi
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_VHOST vringh.c)
zephyr_library_sources_ifdef(CONFIG_VHOST_XEN_MMIO vhost_xen_mmio.c)
