/*
 * Copyright (c) 2019 Texas Instruments Incorporated
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Add CC32xx flash header to disable flash verification */
. = 0x0;
KEEP(*(.dbghdr))
KEEP(*(".dbghdr.*"))
