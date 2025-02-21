# Copyright (c) 2025 Würth Elektronik eiSos GmbH & Co. KG
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(wsen_pads_2511020213301.c)
zephyr_library_sources_ifdef(CONFIG_WSEN_PADS_2511020213301_TRIGGER wsen_pads_2511020213301_trigger.c)
