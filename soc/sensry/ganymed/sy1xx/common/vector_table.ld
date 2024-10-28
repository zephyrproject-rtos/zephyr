/*
 * Copyright (c) 2019 Foundries.io Ltd
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


KEEP(*(.reset.*))
KEEP(*(".exception.entry.*")) /* contains _isr_wrapper */
*(".exception.other.*")

KEEP(*(.openocd_debug))
KEEP(*(".openocd_debug.*"))
