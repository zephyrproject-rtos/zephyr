# Copyright (c) 2025 Würth Elektronik eiSos GmbH & Co. KG
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(wsen_isds_2536030320001.c)
zephyr_library_sources_ifdef(CONFIG_WSEN_ISDS_2536030320001_TRIGGER wsen_isds_2536030320001_trigger.c)

zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
