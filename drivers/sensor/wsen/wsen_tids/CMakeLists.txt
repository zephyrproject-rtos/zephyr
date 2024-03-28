# Copyright (c) 2022 Würth Elektronik eiSos GmbH & Co. KG
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(wsen_tids.c)
zephyr_library_sources_ifdef(CONFIG_WSEN_TIDS_TRIGGER    wsen_tids_trigger.c)
