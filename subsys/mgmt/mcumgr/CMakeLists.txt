#
# Copyright (c) 2018-2021 mcumgr authors
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(mgmt)
add_subdirectory(smp)
add_subdirectory(util)
add_subdirectory(grp)
add_subdirectory(transport)
add_subdirectory_ifdef(CONFIG_SMP_CLIENT    smp_client)
