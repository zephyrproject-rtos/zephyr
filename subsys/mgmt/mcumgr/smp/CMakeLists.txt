#
# Copyright (c) 2018-2021 mcumgr authors
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
# Protocol API is only exposed to MCUmgr internals.
add_library(mgmt_mcumgr_protocol src/smp.c)

target_link_libraries(mgmt_mcumgr_protocol PRIVATE
  mgmt_mcumgr mgmt_mcumgr_util mgmt_mcumgr_transport zephyr_interface
)
