#
# Copyright (c) 2018-2021 mcumgr authors
#
# SPDX-License-Identifier: Apache-2.0
#

target_include_directories(MCUMGR INTERFACE
    include
)

zephyr_library_sources(
    src/os_mgmt.c
)

zephyr_library_sources_ifdef(CONFIG_REBOOT
    src/zephyr_os_mgmt.c
)
