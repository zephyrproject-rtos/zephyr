# Copyright (c) 2022 Würth Elektronik eiSos GmbH & Co. KG
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(wsen_hids.c)
zephyr_library_sources_ifdef(CONFIG_WSEN_HIDS_TRIGGER wsen_hids_trigger.c)
