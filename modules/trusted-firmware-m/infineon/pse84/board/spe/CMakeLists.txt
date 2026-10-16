# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
# TF-M IFX_BOARD_PATH board customization for PSE84, used by the Zephyr
# TF-M build. It reuses the TF-M PSE84 board support unchanged.

set(ifx_tfm_board_dir ${IFX_FAMILY_SOURCE_DIR}/board)

add_subdirectory(${ifx_tfm_board_dir}/spe ifx_board_spe)

target_include_directories(ifx_pdl_inc_s
    INTERFACE
        ${ifx_tfm_board_dir}/shared
        ${ifx_tfm_board_dir}/shared/device/include
        ${ifx_tfm_board_dir}/spe
)

# Replace the clock configuration of the board design with one generated
# from the Zephyr devicetree.
get_target_property(ifx_bsp_sources ifx_bsp SOURCES)
list(FILTER ifx_bsp_sources EXCLUDE REGEX "/cycfg_clocks\\.c$")
set_property(TARGET ifx_bsp PROPERTY SOURCES ${ifx_bsp_sources})

target_sources(ifx_bsp
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/cycfg_clocks.c
)

target_include_directories(ifx_bsp
    PRIVATE
        ${ZEPHYR_BASE}/include
        ${ZEPHYR_BINARY_DIR}/include/generated
)
