/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define L2CAP_MTU 150
#define L2CAP_SDU_LEN (L2CAP_MTU - 2)
#define L2CAP_PSM 0x0080
/* use the first dynamic channel ID */
#define L2CAP_CID 0x0040
