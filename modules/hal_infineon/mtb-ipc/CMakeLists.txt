#
# SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG. All rights reserved.</text>
#
# SPDX-License-Identifier: Apache-2.0
#

set(mtb_ipc_dir ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/mtb-ipc)

zephyr_include_directories(${mtb_ipc_dir}/include)
zephyr_library_sources(${mtb_ipc_dir}/source/mtb_ipc.c)
