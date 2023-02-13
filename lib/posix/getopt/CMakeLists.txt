# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories_ifdef(
  CONFIG_GETOPT
  .
)

zephyr_sources_ifdef(
  CONFIG_GETOPT
  getopt.c
  getopt_common.c
)

zephyr_sources_ifdef(
  CONFIG_GETOPT_LONG
  getopt_long.c
)
