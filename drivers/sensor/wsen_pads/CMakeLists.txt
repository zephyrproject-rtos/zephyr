# Copyright (c) 2022 Würth Elektronik eiSos GmbH & Co. KG
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(pads.c)
zephyr_library_sources(WSEN_PADS_2511020213301.c)
zephyr_library_sources_ifdef(CONFIG_PADS_TRIGGER pads_trigger.c)

zephyr_library_include_directories(../weplatform)
