/*
 * Copyright 2023 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define POWER_PROFILE_FRO192M_900MV 0xFF
#define POWER_PROFILE_FRO96M_800MV  0x1
#define POWER_PROFILE_AFTER_BOOT    0x0

__ramfunc int32_t power_manager_set_profile(uint32_t power_profile);
