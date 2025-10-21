/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ETH_RENESAS_RA_RMAC_H__
#define ZEPHYR_INCLUDE_DRIVERS_ETH_RENESAS_RA_RMAC_H__

#include <stdint.h>
#include "r_rmac_phy.h"

/* Organizationally Unique Identifier for MAC */
#define RENESAS_OUI_B0 0x74
#define RENESAS_OUI_B1 0x90
#define RENESAS_OUI_B2 0x50

#define RENESAS_RA_MPIC_LSC_10   0
#define RENESAS_RA_MPIC_LSC_100  1
#define RENESAS_RA_MPIC_LSC_1000 2

#define RENESAS_RA_ETHA_DISABLE_MODE   1
#define RENESAS_RA_ETHA_CONFIG_MODE    2
#define RENESAS_RA_ETHA_OPERATION_MODE 3

extern uint8_t r_rmac_phy_get_operation_mode(rmac_phy_instance_ctrl_t *p_instance_ctrl);
extern void r_rmac_phy_set_operation_mode(uint8_t channel, uint8_t mode);

#endif /* ZEPHYR_INCLUDE_DRIVERS_ETH_RENESAS_RA_RMAC_H__ */
