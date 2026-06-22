# Copyright (c) 2024 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories_ifdef(CONFIG_HAS_RENESAS_RZ_FSP common)

add_subdirectory(${SOC_SERIES})
