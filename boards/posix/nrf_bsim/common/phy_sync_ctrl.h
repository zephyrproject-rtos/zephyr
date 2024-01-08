/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BOARDS_POSIX_BSIM_COMMON_PHY_SYNC_CTRL_H
#define BOARDS_POSIX_BSIM_COMMON_PHY_SYNC_CTRL_H

#include "bs_types.h"

#ifdef __cplusplus
extern "C"{
#endif

void phy_sync_ctrl_set_last_phy_sync_time(bs_time_t time);
void phy_sync_ctrl_set_max_resync_offset(bs_time_t max_resync_offset);
void phy_sync_ctrl_connect_to_2G4_phy(void);
void phy_sync_ctrl_pre_boot2(void);
void phy_sync_ctrl_pre_boot3(void);

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_POSIX_BSIM_COMMON_PHY_SYNC_CTRL_H */
