# Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

set(cpc_dir ${ZEPHYR_HAL_SILABS_MODULE_DIR}/cpc)
set(sisdk_dir ${ZEPHYR_HAL_SILABS_MODULE_DIR}/simplicity_sdk)

zephyr_include_directories(
  include
  ${ZEPHYR_BASE}/include/zephyr/portability
  ${cpc_dir}/inc
  ${cpc_dir}/src
  ${sisdk_dir}/platform/common/inc
  ${sisdk_dir}/platform/service/memory_manager/inc
)

zephyr_library_sources(
  ${cpc_dir}/src/sli_cpc.c
  ${cpc_dir}/src/sli_cpc_buf.c
  ${cpc_dir}/src/sli_cpc_bus.c
  ${cpc_dir}/src/sli_cpc_control.c
  ${cpc_dir}/src/sli_cpc_control_primary.c
  ${cpc_dir}/src/sli_cpc_crc.c
  ${cpc_dir}/src/sli_cpc_dispatcher.c
  ${cpc_dir}/src/sli_cpc_frame.c
  ${cpc_dir}/src/sli_cpc_memory.c
  ${cpc_dir}/src/sli_cpc_msgq.c
  src/sli_cpc_log.c
  src/sli_cpc_timer.c
)

# Platform bits of the Simplicity SDK that the core relies on. Silicon Labs
# SoCs already build them.
if(NOT CONFIG_HAS_SILABS_SISDK AND NOT CONFIG_HAS_SILABS_WISECONNECT)
  zephyr_library_sources(${sisdk_dir}/platform/common/src/sl_slist.c)
endif()
if(NOT CONFIG_HAS_SILABS_SISDK)
  zephyr_library_sources(../simplicity_sdk/src/sl_memory_manager_pool_shim.c)
endif()
