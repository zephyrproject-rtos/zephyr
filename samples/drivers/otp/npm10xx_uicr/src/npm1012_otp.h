/*
 * Copyright (C) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Example UICR configuration. The bits to be burned are as follows:
 *   - bit 6: disconnect GPIO2 input buffer and pull-down (prevent leakage through LED wired to it)
 *   - bit 9: enable battery charging
 *   - bit 13: enable weak battery charging (disable regulators until VBAT reaches a set threshold)
 *   - bits 17, 18, 21: set default charge current to 0x13 [0.5 + 0.5 * 19 = 10.0 mA]
 *   - bits 70, 72: set weak VBAT threshold to 0x5 [2.5 + 0.1 * 5 = 3.0 V]
 *   - bit 76: set Buck VOUT based on the register value instead of the VSET pin
 *   - bits 77, 78, 79, 82: set Buck VOUT to 0x27 [0.55 + 0.05 * 39 = 2.5 V]
 *   - bit 88: enable the boot monitor
 *   - bit 97: OTP user version = 0x01
 */
#define NPM1012_OTP_ARRAY                                                                          \
	{                                                                                          \
		0x40, 0x22, 0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0xF1, 0x04, 0x01, 0x02,      \
	}
