# Copyright (c) 2022 Würth Elektronik eiSos GmbH & Co. KG
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(hids.c)
zephyr_library_sources(WSEN_HIDS_2523020210001.c)
zephyr_library_sources_ifdef(CONFIG_HIDS_TRIGGER hids_trigger.c)

zephyr_library_include_directories(../weplatform)
