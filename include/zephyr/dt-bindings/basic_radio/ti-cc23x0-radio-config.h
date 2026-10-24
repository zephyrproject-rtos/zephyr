/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Devicetree constants for TI CC23x0 radio configurations
 * (ti,cc23x0-radio-config nodes).
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_BASIC_RADIO_TI_CC23X0_RADIO_CONFIG_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_BASIC_RADIO_TI_CC23X0_RADIO_CONFIG_H_

/*
 * Pointer-slot tokens for ti,words register segments.
 *
 * A few words of an LRF register segment hold C addresses of software
 * tables instead of register values. DT cannot express addresses, so the
 * SmartRF export places one of these tokens in that word, and the driver
 * substitutes the address of the matching object at compile time. The
 * converter rejects exports in which a plain data word equals a token.
 */
#define TI_LRF_PTR_TX_POWER_TABLE 0xA1F00001 /* LRF_TxPowerTable of this config */
#define TI_LRF_PTR_FCFG_TRIMS     0xA1F00002 /* &fcfg->appTrims (device trims) */
#define TI_LRF_PTR_SW_CONFIG_0    0xA1F00010 /* LRF_SwConfig from ti,sw-config-0 */
#define TI_LRF_PTR_SW_CONFIG_1    0xA1F00011 /* LRF_SwConfig from ti,sw-config-1 */
#define TI_LRF_PTR_SW_CONFIG_2    0xA1F00012 /* LRF_SwConfig from ti,sw-config-2 */
#define TI_LRF_PTR_SW_CONFIG_3    0xA1F00013 /* LRF_SwConfig from ti,sw-config-3 */

/*
 * One ti,tx-power-table entry (LRF_TxPowerTable_Entry), packed in one cell:
 * bits 0-7 dBm (two's complement), 8-15 temperature coefficient, 16-21 PA
 * bias (ib), 22-24 PA gain, 25 PA mode.
 */
#define TI_LRF_TX_POWER(dbm, temp_coeff, ib, gain, mode)                     \
	(((dbm) & 0xFF) | (((temp_coeff) & 0xFF) << 8) | (((ib) & 0x3F) << 16) | \
	 (((gain) & 0x7) << 22) | (((mode) & 0x1) << 25))

/* txShape cell value of ti,sw-config-N meaning "no TX shape" (NULL). */
#define TI_LRF_NO_TX_SHAPE 255

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_BASIC_RADIO_TI_CC23X0_RADIO_CONFIG_H_ */