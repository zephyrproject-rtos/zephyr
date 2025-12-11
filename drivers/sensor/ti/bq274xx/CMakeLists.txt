# Copyright (c) 2020 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(bq274xx.c)
zephyr_library_sources_ifdef(CONFIG_BQ274XX_TRIGGER bq274xx_trigger.c)
