# Copyright (c) 2025 NVIDIA Corporation <jholdsworth@nvidia.com>
# SPDX-License-Identifier: Apache-2.0

# OpenRISC runs code from RAM (non-XIP), so the RAM region in the linker
# script legitimately needs RWX permissions.  Suppress the ld >= 2.39
# warning that would otherwise become an error with --fatal-warnings.
list(APPEND TOOLCHAIN_LD_FLAGS -Wl,--no-warn-rwx-segments)
