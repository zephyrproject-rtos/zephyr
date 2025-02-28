# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_DT_HAS_TI_K3_CTRL_PARTITION_ENABLED common/k3_ctrl_partition.c)

add_subdirectory(${SOC_SERIES})
