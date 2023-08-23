# Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  arm_smmu_v3.c
  smmu_page_map.c
  page_map.c
)
