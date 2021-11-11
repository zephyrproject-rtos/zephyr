# Copyright (c) 2021 Golioth, Inc.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_QCBOR)
  set(QCBOR_DIR ${ZEPHYR_CURRENT_MODULE_DIR})

  zephyr_library()

  zephyr_include_directories(${QCBOR_DIR}/inc)

  zephyr_library_compile_definitions(QCBOR_DISABLE_FLOAT_HW_USE)
  zephyr_library_sources(
    ${QCBOR_DIR}/src/ieee754.c
    ${QCBOR_DIR}/src/qcbor_decode.c
    ${QCBOR_DIR}/src/qcbor_encode.c
    ${QCBOR_DIR}/src/qcbor_err_to_str.c
    ${QCBOR_DIR}/src/UsefulBuf.c
  )
endif()
