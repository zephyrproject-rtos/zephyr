# Copyright (c) 2026 Analog Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(ltc4286.c)
zephyr_library_sources_ifdef(CONFIG_LTC4286_TRIGGER ltc4286_trigger.c)
