# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2020 Linumiz
# Author: Parthiban Nallathambi <parthiban@linumiz.com>

add_subdirectory(${SOC_SERIES})

zephyr_compile_definitions($<UPPER_CASE:${CONFIG_SOC}>_${CONFIG_SOC_PART_NUMBER})
