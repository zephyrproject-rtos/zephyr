# Copyright (c) 2022 GARDENA GmbH
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(
  CONFIG_FACTORY_DATA_FCB
  factory_data_common.c
  factory_data_fcb.c
  )

zephyr_library_sources_ifdef(
  CONFIG_FACTORY_DATA_LV
  factory_data_common.c
  factory_data_lv.c
  )

zephyr_library_sources_ifdef(
  CONFIG_FACTORY_DATA_DUMMY
  factory_data_dummy.c
  )

zephyr_library_sources_ifdef(
  CONFIG_FACTORY_DATA_SHELL
  factory_data_shell.c
  )
