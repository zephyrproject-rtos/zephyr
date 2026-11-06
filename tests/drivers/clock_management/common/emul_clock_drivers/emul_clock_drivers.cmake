# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026, Analog Devices

target_sources_ifdef(CONFIG_CLOCK_MANAGEMENT_EMUL_DIV app PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/emul_clock_div.c)
target_sources_ifdef(CONFIG_CLOCK_MANAGEMENT_EMUL_MUX app PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/emul_clock_mux.c)
