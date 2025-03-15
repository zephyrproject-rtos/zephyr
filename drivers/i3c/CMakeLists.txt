# Copyright (c) 2022 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/drivers/i3c.h)

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_I3C_CONTROLLER i3c_ccc.c)
zephyr_library_sources_ifdef(CONFIG_I3C_CONTROLLER i3c_common.c)

if(DEFINED CONFIG_I3C_NUM_OF_DESC_MEM_SLABS AND CONFIG_I3C_NUM_OF_DESC_MEM_SLABS GREATER 0)
  zephyr_library_sources(i3c_mem_slab.c)
endif()

if(DEFINED CONFIG_I3C_I2C_NUM_OF_DESC_MEM_SLABS AND CONFIG_I3C_I2C_NUM_OF_DESC_MEM_SLABS GREATER 0)
  zephyr_library_sources(i3c_i2c_mem_slab.c)
endif()

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
  CONFIG_I3C_STM32
  i3c_stm32.c
)

zephyr_library_sources_ifdef(
  CONFIG_I3C_DW
  i3c_dw.c
)

zephyr_library_sources_ifdef(
  CONFIG_I3C_TEST
  i3c_test.c
)

zephyr_library_sources_ifdef(
  CONFIG_I3C_RTIO
	i3c_rtio.c
	i3c_rtio_default.c
)
