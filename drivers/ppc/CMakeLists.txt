# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_ARM_SSE200_SPCTRL  arm_sse200_spctrl)
add_subdirectory_ifdef(CONFIG_INFINEON_MXS40_PPC infineon_mxs40_ppc)
# zephyr-keep-sorted-stop
