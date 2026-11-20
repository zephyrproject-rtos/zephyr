/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * Include this header ONLY in files that need CC23x0-specific knobs.
 * The vendor-neutral basic_radio API (<zephyr/drivers/basic_radio.h>) must not
 * include or mention any symbol from this file.
 *
 * High byte 0x01 is the TI CC23x0 vendor tag.  Every command value bakes
 * the tag in so a wrong-backend call falls through to ext_ctl's default:
 * arm and returns -ENOTSUP without any runtime mask check.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_BASIC_RADIO_TI_CC23X0_EXT_H_
#define ZEPHYR_INCLUDE_DRIVERS_BASIC_RADIO_TI_CC23X0_EXT_H_

#include <stdint.h>

enum ti_cc23x0_ext_cmd {
	TI_CC23X0_EXT_SET_TX_POWER = 0x01000001, /* arg: struct ti_cc23x0_ext_set_tx_power */
	TI_CC23X0_EXT_GET_STATS    = 0x01000002, /* arg: struct ti_cc23x0_ext_stats */
};

struct ti_cc23x0_ext_set_tx_power {
	int8_t dbm;
};

struct ti_cc23x0_ext_stats {
	uint32_t rx_ok;
	uint32_t rx_crc_err;
	uint32_t sched_reject;
	uint32_t cmd_started;
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_BASIC_RADIO_TI_CC23X0_EXT_H_ */