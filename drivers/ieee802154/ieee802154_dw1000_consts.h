/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is based on dw1000_regs.h and dw1000_mac.c from
 * https://github.com/Decawave/mynewt-dw1000-core.git
 * (d6b1414f1b4527abda7521a304baa1c648244108)
 * The content was modified and restructured to meet the
 * coding style and resolve namespace issues.
 *
 * This file should be included ONLY by ieee802154_dw1000.c
 *
 * This file is derived from material that is:
 *
 * Copyright (C) 2017-2018, Decawave Limited, All Rights Reserved
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef ZEPHYR_INCLUDE_DW1000_CONSTS_H_
#define ZEPHYR_INCLUDE_DW1000_CONSTS_H_

#include "ieee802154_dw1000_regs.h"

/*
 * Map the channel number to the index in the configuration arrays below.
 *                  Channel: na  1  2  3  4  5 na  7
 */
const uint8_t dwt_ch_to_cfg[] = {0, 0, 1, 2, 3, 4, 0, 5};

/* Defaults from Table 38: Sub-Register 0x28:0C– RF_TXCTRL values */
const uint32_t dwt_txctrl_defs[] = {
	DWT_RF_TXCTRL_CH1,
	DWT_RF_TXCTRL_CH2,
	DWT_RF_TXCTRL_CH3,
	DWT_RF_TXCTRL_CH4,
	DWT_RF_TXCTRL_CH5,
	DWT_RF_TXCTRL_CH7,
};

/* Defaults from Table 43: Sub-Register 0x2B:07 – FS_PLLCFG values */
const uint32_t dwt_pllcfg_defs[] = {
	DWT_FS_PLLCFG_CH1,
	DWT_FS_PLLCFG_CH2,
	DWT_FS_PLLCFG_CH3,
	DWT_FS_PLLCFG_CH4,
	DWT_FS_PLLCFG_CH5,
	DWT_FS_PLLCFG_CH7
};

/* Defaults from Table 44: Sub-Register 0x2B:0B – FS_PLLTUNE values */
const uint8_t dwt_plltune_defs[] = {
	DWT_FS_PLLTUNE_CH1,
	DWT_FS_PLLTUNE_CH2,
	DWT_FS_PLLTUNE_CH3,
	DWT_FS_PLLTUNE_CH4,
	DWT_FS_PLLTUNE_CH5,
	DWT_FS_PLLTUNE_CH7
};

/* Defaults from Table 37: Sub-Register 0x28:0B– RF_RXCTRLH values */
const uint8_t dwt_rxctrlh_defs[] = {
	DWT_RF_RXCTRLH_NBW,
	DWT_RF_RXCTRLH_NBW,
	DWT_RF_RXCTRLH_NBW,
	DWT_RF_RXCTRLH_WBW,
	DWT_RF_RXCTRLH_NBW,
	DWT_RF_RXCTRLH_WBW
};

/* Defaults from Table 40: Sub-Register 0x2A:0B – TC_PGDELAY */
const uint8_t dwt_pgdelay_defs[] = {
	DWT_TC_PGDELAY_CH1,
	DWT_TC_PGDELAY_CH2,
	DWT_TC_PGDELAY_CH3,
	DWT_TC_PGDELAY_CH4,
	DWT_TC_PGDELAY_CH5,
	DWT_TC_PGDELAY_CH7
};

/*
 * Defaults from Table 19: Reference values for Register file:
 *     0x1E – Transmit Power Control for Smart Transmit Power Control
 *     Transmit Power Control values for 16 MHz, with DIS_STXP = 0
 */
const uint32_t dwt_txpwr_stxp0_16[] = {
	0x15355575,
	0x15355575,
	0x0F2F4F6F,
	0x1F1F3F5F,
	0x0E082848,
	0x32527292
};

/*
 * Defaults from Table 19: Reference values for Register file:
 *     0x1E – Transmit Power Control for Smart Transmit Power Control
 *     Transmit Power Control values for 64 MHz, with DIS_STXP = 0
 */
const uint32_t dwt_txpwr_stxp0_64[] = {
	 0x07274767,
	 0x07274767,
	 0x2B4B6B8B,
	 0x3A5A7A9A,
	 0x25456585,
	 0x5171B1D1
};

/*
 * Default from Table 20: Reference values Register file:
 *     0x1E – Transmit Power Control for Manual Transmit Power Control
 *     Transmit Power Control values for 16 MHz, with DIS_STXP = 1
 */
const uint32_t dwt_txpwr_stxp1_16[] = {
	0x75757575,
	0x75757575,
	0x6F6F6F6F,
	0x5F5F5F5F,
	0x48484848,
	0x92929292
};

/*
 * Default from Table 20: Reference values Register file:
 *     0x1E – Transmit Power Control for Manual Transmit Power Control
 *     Transmit Power Control values for 64 MHz, with DIS_STXP = 1
 */
const uint32_t dwt_txpwr_stxp1_64[] = {
	0x67676767,
	0x67676767,
	0x8B8B8B8B,
	0x9A9A9A9A,
	0x85858585,
	0xD1D1D1D1
};

/* Defaults from Table 24: Sub-Register 0x23:04 – AGC_TUNE1 values */
const uint16_t dwt_agc_tune1_defs[] = {
	DWT_AGC_TUNE1_16M,
	DWT_AGC_TUNE1_64M
};

/* Decawave non-standard SFD lengths */
const uint8_t dwt_ns_sfdlen[] = {
	DWT_DW_NS_SFD_LEN_110K,
	DWT_DW_NS_SFD_LEN_850K,
	DWT_DW_NS_SFD_LEN_6M8
};

/* Defaults from Table 30: Sub-Register 0x27:02 – DRX_TUNE0b values */
const uint16_t dwt_tune0b_defs[DWT_NUMOF_BRS][2] = {
	{
		DWT_DRX_TUNE0b_110K_STD,
		DWT_DRX_TUNE0b_110K_NSTD
	},
	{
		DWT_DRX_TUNE0b_850K_STD,
		DWT_DRX_TUNE0b_850K_NSTD
	},
	{
		DWT_DRX_TUNE0b_6M8_STD,
		DWT_DRX_TUNE0b_6M8_NSTD
	}
};

/* Defaults from Table 31: Sub-Register 0x27:04 – DRX_TUNE1a values */
const uint16_t dwt_tune1a_defs[] = {
	DWT_DRX_TUNE1a_PRF16,
	DWT_DRX_TUNE1a_PRF64
};

/* Defaults from Table 33: Sub-Register 0x27:08 – DRX_TUNE2 values */
const uint32_t dwt_tune2_defs[DWT_NUMOF_PRFS][DWT_NUMOF_PACS] = {
	{
		DWT_DRX_TUNE2_PRF16_PAC8,
		DWT_DRX_TUNE2_PRF16_PAC16,
		DWT_DRX_TUNE2_PRF16_PAC32,
		DWT_DRX_TUNE2_PRF16_PAC64
	},
	{
		DWT_DRX_TUNE2_PRF64_PAC8,
		DWT_DRX_TUNE2_PRF64_PAC16,
		DWT_DRX_TUNE2_PRF64_PAC32,
		DWT_DRX_TUNE2_PRF64_PAC64
	}
};

/*
 * Defaults from Table 51:
 * Sub-Register 0x2E:2804 – LDE_REPC configurations for (850 kbps & 6.8 Mbps)
 *
 * For 110 kbps the values have to be divided by 8.
 */
const uint16_t dwt_lde_repc_defs[] = {
	0,
	DWT_LDE_REPC_PCODE_1,
	DWT_LDE_REPC_PCODE_2,
	DWT_LDE_REPC_PCODE_3,
	DWT_LDE_REPC_PCODE_4,
	DWT_LDE_REPC_PCODE_5,
	DWT_LDE_REPC_PCODE_6,
	DWT_LDE_REPC_PCODE_7,
	DWT_LDE_REPC_PCODE_8,
	DWT_LDE_REPC_PCODE_9,
	DWT_LDE_REPC_PCODE_10,
	DWT_LDE_REPC_PCODE_11,
	DWT_LDE_REPC_PCODE_12,
	DWT_LDE_REPC_PCODE_13,
	DWT_LDE_REPC_PCODE_14,
	DWT_LDE_REPC_PCODE_15,
	DWT_LDE_REPC_PCODE_16,
	DWT_LDE_REPC_PCODE_17,
	DWT_LDE_REPC_PCODE_18,
	DWT_LDE_REPC_PCODE_19,
	DWT_LDE_REPC_PCODE_20,
	DWT_LDE_REPC_PCODE_21,
	DWT_LDE_REPC_PCODE_22,
	DWT_LDE_REPC_PCODE_23,
	DWT_LDE_REPC_PCODE_24
};

/*
 * Transmit Preamble Symbol Repetitions (TXPSR) and Preamble Extension (PE)
 * constants for TX_FCTRL - Transmit Frame Control register.
 * From Table 16: Preamble length selection
 *       BIT(19) | BIT(18) | BIT(21) | BIT(20)
 */
const uint32_t dwt_plen_cfg[] = {
	(0       | BIT(18) | 0       | 0),
	(0       | BIT(18) | 0       | BIT(20)),
	(0       | BIT(18) | BIT(21) | 0),
	(0       | BIT(18) | BIT(21) | BIT(20)),
	(BIT(19) | 0       | 0       | 0),
	(BIT(19) | 0       | BIT(21) | 0),
	(BIT(19) | BIT(18) | 0       | 0),
};

#endif /* ZEPHYR_INCLUDE_DW1000_CONSTS_H_ */
