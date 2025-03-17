# Copyright (c) 2025 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_UFSHC)

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_UFS_AMD_VERSAL2 ufshc_amd_versal2.c)

endif()
