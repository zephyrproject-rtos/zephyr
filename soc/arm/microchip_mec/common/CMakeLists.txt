# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories_ifdef(CONFIG_SOC_SERIES_MEC172X .)
zephyr_library_sources_ifdef(CONFIG_SOC_SERIES_MEC172X
  soc_i2c.c
)
