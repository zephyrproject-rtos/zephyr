# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# TF-M IFX_BOARD_PATH board customization for PSE84, non-secure side.
# Reuses the TF-M PSE84 board support unchanged.

set(ifx_tfm_board_dir ${IFX_FAMILY_SOURCE_DIR}/board)

add_subdirectory(${ifx_tfm_board_dir}/nspe ifx_board_nspe)

target_include_directories(ifx_pdl_inc_ns
    INTERFACE
        ${ifx_tfm_board_dir}/shared
        ${ifx_tfm_board_dir}/shared/device/include
        ${ifx_tfm_board_dir}/nspe
)
