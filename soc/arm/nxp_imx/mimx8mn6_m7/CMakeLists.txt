#
# Copyright (c) 2021, Waseem Hassan
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources(
  soc.c
  )
  
zephyr_sources_ifdef(CONFIG_PM
  power.c
  )

zephyr_sources_ifdef(CONFIG_IMX8MNANO_PM
  soc_power.c
  )
    
