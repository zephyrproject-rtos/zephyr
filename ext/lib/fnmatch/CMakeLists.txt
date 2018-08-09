# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories_ifdef(
  CONFIG_FNMATCH
  .
)

zephyr_sources_ifdef(
  CONFIG_FNMATCH
  fnmatch.c
)

