# Copyright (c) 2026  Microchip Technology Inc. and its subsidiaries
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_EMC1702 emc1702.c)
zephyr_library_sources_ifdef(CONFIG_EMC1702_TRIGGER emc1702_trigger.c)
