# Copyright (c) 2023 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com>
# Copyright (c) 2024 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(common)
zephyr_include_directories_ifdef(CONFIG_HAS_RENESAS_RA_FSP common_fsp)

add_subdirectory(${SOC_SERIES})
