/*
 * Copyright 2023 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* regssssssss, masks, values */
#define CS40L50_REG_DEVID 0x0
#define CS40L50_DEVID     0x40a50

#define CS40L50_REG_REVID 0x4
#define CS40L50_REVID_B0  0xb0

#define CS40L50_REG_DSP_VIRTUAL1_MBOX_1 0x11020
#define CS40L50_DSP_MBOX_RESET          0x0
#define CS40L50_DSP_MBOX_PM_CMD_BASE    0x2000001

#define CS40L50_REG_DSP_STATUS_0   0x28021e0
#define CS40L50_HALO_STATE         GENMASK(4, 0)
#define CS40L50_HALO_STATE_RUNNING 0x2

#define CS40L50_REG_DSP1_CCM_CORE_CONTROL 0x2bc1000
#define CS40L50_DSP1_CCM_PM_REGMAP        0x80

#define CS40L50_REG_
#define CS40L50_REG_
#define CS40L50_REG_
#define CS40L50_REG_
#define CS40L50_REG_
#define CS40L50_REG_
#define CS40L50_REG_
#define CS40L50_REG_
#define CS40L50_REG_
#define CS40L50_REG_

/* timings */
#define CS40L50_T_RLPW_US 1000
#define CS40L50_T_IRS_US  2200
