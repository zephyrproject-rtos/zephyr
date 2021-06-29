# Copyright (c) 2021 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_HAS_SEDI)
  set(SEDI_DIR ${ZEPHYR_CURRENT_MODULE_DIR}/)
  set(sedi_lib_src_dir ${ZEPHYR_CURRENT_MODULE_DIR}/soc/arm/elkhart_lake_pse/sedi)

  zephyr_interface_library_named(INTEL_BSP_SEDI)
  zephyr_library()
  zephyr_library_link_libraries(INTEL_BSP_SEDI)
  target_link_libraries(zephyr_interface INTERFACE INTEL_BSP_SEDI)

  target_include_directories(INTEL_BSP_SEDI INTERFACE ${sedi_lib_src_dir}/include)
  target_include_directories(INTEL_BSP_SEDI INTERFACE ${sedi_lib_src_dir}/include/driver)

  zephyr_library_include_directories(${sedi_lib_src_dir}/drivers)
  zephyr_include_directories(${sedi_lib_src_dir}/soc/ehl_pse/include)

  zephyr_library_include_directories(${sedi_lib_src_dir}/include)
  zephyr_library_sources(${sedi_lib_src_dir}/drivers/common/sedi_common.c)
  zephyr_library_sources(${sedi_lib_src_dir}/drivers/pm/pm.c)
  zephyr_library_sources_ifdef(CONFIG_UART_SEDI   ${sedi_lib_src_dir}/drivers/usart/dw_uart.c)
  zephyr_library_sources(${sedi_lib_src_dir}/drivers/ipc/ipc_bsp.c
    ${sedi_lib_src_dir}/drivers/ipc/fwst.c)

endif()
