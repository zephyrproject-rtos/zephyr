# Copyright (c) 2023 Würth Elektronik eiSos GmbH & Co. KG
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(wsen_pads.c)
zephyr_library_sources_ifdef(CONFIG_WSEN_PADS_TRIGGER wsen_pads_trigger.c)
