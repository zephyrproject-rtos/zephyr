# Copyright (c) 2022 Würth Elektronik eiSos GmbH & Co. KG
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(pdus.c)
zephyr_library_sources(WSEN_PDUS_25131308XXX01.c)

zephyr_library_include_directories(../weplatform)
