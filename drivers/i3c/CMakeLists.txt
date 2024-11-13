# Copyright (c) 2022 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/i3c.h)

zephyr_library()

zephyr_library_sources(
  i3c_ccc.c
  i3c_common.c
)

zephyr_library_sources_ifdef(
  CONFIG_USERSPACE
  i3c_handlers.c
)

zephyr_library_sources_ifdef(
  CONFIG_I3C_SHELL
  i3c_shell.c
)

zephyr_library_sources_ifdef(
  CONFIG_I3C_IBI_WORKQUEUE
  i3c_ibi_workq.c
)

zephyr_library_sources_ifdef(
  CONFIG_I3C_MCUX
  i3c_mcux.c
)

zephyr_library_sources_ifdef(
  CONFIG_I3C_CADENCE
  i3c_cdns.c
)

zephyr_library_sources_ifdef(
  CONFIG_I3C_NPCX
  i3c_npcx.c
)

zephyr_library_sources_ifdef(
  CONFIG_I3C_NPCM
  i3c_npcm.c
)

zephyr_library_sources_ifdef(
  CONFIG_I3C_TEST
  i3c_test.c
)
