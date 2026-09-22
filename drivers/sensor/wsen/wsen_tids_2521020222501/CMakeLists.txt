# Copyright (c) 2025 Würth Elektronik eiSos GmbH & Co. KG
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(wsen_tids_2521020222501.c)
zephyr_library_sources_ifdef(CONFIG_WSEN_TIDS_2521020222501_TRIGGER    wsen_tids_2521020222501_trigger.c)
