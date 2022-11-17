# Copyright (c) 2022 Huawei France Technologies SAS
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_ARM_MMU mmu_regions.c)
