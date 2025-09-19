# Copyright (c) 2025 Realtek Semiconductor, Inc.
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(common)
add_subdirectory_ifdef(CONFIG_SOC_RTS5817 rts5817)
