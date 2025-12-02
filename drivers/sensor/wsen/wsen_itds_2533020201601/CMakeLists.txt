# Copyright (c) 2025 Würth Elektronik eiSos GmbH & Co. KG
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(wsen_itds_2533020201601.c)
zephyr_library_sources_ifdef(CONFIG_WSEN_ITDS_2533020201601_TRIGGER wsen_itds_2533020201601_trigger.c)

zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
