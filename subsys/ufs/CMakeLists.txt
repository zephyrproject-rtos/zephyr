# Copyright (c) 2025 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_UFS_STACK)
  zephyr_interface_library_named(UFS)

  zephyr_library()
  zephyr_library_sources(ufs.c ufs_ops.c)

endif()
