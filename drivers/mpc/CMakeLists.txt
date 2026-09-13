# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_ARM_TZ_MPC        arm_tz_mpc)
add_subdirectory_ifdef(CONFIG_INFINEON_MPC infineon_mpc)
# zephyr-keep-sorted-stop
