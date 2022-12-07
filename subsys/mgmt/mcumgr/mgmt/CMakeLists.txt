#
# Copyright (c) 2018-2021 mcumgr authors
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

# Provides MCUmgr services, group registration, event handling etc.
add_library(mgmt_mcumgr_mgmt src/mgmt.c)

target_include_directories(mgmt_mcumgr_mgmt PUBLIC include)

target_link_libraries(mgmt_mcumgr_mgmt PRIVATE zephyr_interface)
target_link_libraries(mgmt_mcumgr INTERFACE mgmt_mcumgr_mgmt)
