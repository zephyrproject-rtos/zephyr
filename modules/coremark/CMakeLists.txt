# SPDX-License-Identifier: Apache-2.0

if(NOT CONFIG_COREMARK)
  return()
endif()

zephyr_library_named(coremark)

zephyr_include_directories(${ZEPHYR_COREMARK_MODULE_DIR} .)

zephyr_library_sources(${ZEPHYR_COREMARK_MODULE_DIR}/core_main.c
			${ZEPHYR_COREMARK_MODULE_DIR}/core_matrix.c
			${ZEPHYR_COREMARK_MODULE_DIR}/core_state.c
			${ZEPHYR_COREMARK_MODULE_DIR}/core_util.c
			${ZEPHYR_COREMARK_MODULE_DIR}/core_list_join.c
			core_portme.c)
