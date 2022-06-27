# Copyright (c) 2022 Würth Elektronik eiSos GmbH & Co. KG
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(itds.c)
zephyr_library_sources(WSEN_ITDS_2533020201601.c)
zephyr_library_sources_ifdef(CONFIG_ITDS_TRIGGER itds_trigger.c)

zephyr_library_include_directories(../weplatform)