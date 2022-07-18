# Copyright (c) 2022 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  i3c_ccc.c
  i3c_common.c
)

zephyr_library_sources_ifdef(
  CONFIG_USERSPACE
  i3c_handlers.c
)
