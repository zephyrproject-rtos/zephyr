# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_LED_MATRIX_UNO_Q 
    uno_q_ledmatrix.c
    uno_q_matrix_hal.c
    )
zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../include)