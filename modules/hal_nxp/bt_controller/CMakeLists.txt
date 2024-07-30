#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

set(hal_nxp_dir           ${ZEPHYR_HAL_NXP_MODULE_DIR})
set(hal_nxp_blobs_dir     ${hal_nxp_dir}/zephyr/blobs)
set(blob_gen_file         ${ZEPHYR_BINARY_DIR}/include/generated/bt_nxp_ctlr_fw.h)

if(CONFIG_BT_NXP_NW612)
    set(blob_file ${hal_nxp_blobs_dir}/iw612/uart_nw61x_se.h)
endif()

if (NOT DEFINED blob_file)
    message(FATAL_ERROR "Unsupported controller. Please select a BT conntroller, refer to ./driver/bluetooth/hci/Kconfig.nxp")
endif()

zephyr_blobs_verify(FILES ${blob_file} REQUIRED)

configure_file(${blob_file} ${blob_gen_file} COPYONLY)
