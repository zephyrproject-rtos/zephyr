# Copyright (c) 2025 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG.
#
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(${SOC_SERIES})
zephyr_compile_definitions($<UPPER_CASE:${CONFIG_SOC}>_${CONFIG_SOC_PART_NUMBER})
