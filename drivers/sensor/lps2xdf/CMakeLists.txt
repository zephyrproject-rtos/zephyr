# ST Microelectronics LPS22DF pressure and temperature sensor
#
# Copyright (c) 2023 STMicroelectronics
# Copyright (c) 2023 PHYTEC Messtechnik GmbH
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(lps2xdf.c)
zephyr_library_sources_ifdef(CONFIG_DT_HAS_ST_LPS22DF_ENABLED  lps22df.c )
zephyr_library_sources_ifdef(CONFIG_DT_HAS_ST_LPS28DFW_ENABLED lps28dfw.c )
zephyr_library_sources_ifdef(CONFIG_LPS2XDF_TRIGGER            lps2xdf_trigger.c)

zephyr_library_include_directories(../stmemsc)
