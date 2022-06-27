# Copyright (c) 2022 Würth Elektronik eiSos GmbH & Co. KG
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(tids.c)
zephyr_library_sources(WSEN_TIDS_2521020222501.c)
zephyr_library_sources_ifdef(CONFIG_TIDS_TRIGGER    tids_trigger.c)

zephyr_library_include_directories(../weplatform)
