# Copyright (c) 2023 Würth Elektronik eiSos GmbH & Co. KG
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(wsen_hids_2525020210001.c)
zephyr_library_sources_ifdef(CONFIG_WSEN_HIDS_2525020210001_TRIGGER wsen_hids_2525020210001_trigger.c)
