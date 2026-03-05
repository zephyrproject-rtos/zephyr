# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_BFLB_FREERTOS_SHIM freertos_shim.c)
