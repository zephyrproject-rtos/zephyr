# SPDX-License-Identifier: Apache-2.0

if (NOT DEFINED SOC_SERIES_MEC1701X)
zephyr_include_directories(.)
zephyr_sources(
  soc_gpio.c
  soc_i2c.c
)
endif()
