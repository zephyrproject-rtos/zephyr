# Copyright (c) 2025 GP Orcullo
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(../../atmel/sam0/common/soc_port.c)
zephyr_include_directories(../../atmel/sam0/common)

add_subdirectory(${SOC_SERIES})
