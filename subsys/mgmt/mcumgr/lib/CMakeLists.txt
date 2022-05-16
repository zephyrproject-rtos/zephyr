#
# Copyright (c) 2018-2021 mcumgr authors
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_interface_library_named(MCUMGR)

target_include_directories(MCUMGR INTERFACE
    mgmt/include
    util/include
    smp/include
)

add_subdirectory(cmd)
add_subdirectory(mgmt)
add_subdirectory(smp)
add_subdirectory(util)
