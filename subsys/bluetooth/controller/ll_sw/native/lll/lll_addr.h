/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_CONTROLLER_LL_SW_NATIVE_LLL_LLL_ADDR_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_CONTROLLER_LL_SW_NATIVE_LLL_LLL_ADDR_H_

/* Every role filters the first address of a received PDU (AdvA, ScanA or
 * InitA) the same way. A NULL filter skips the Filter Accept List match.
 */
struct lll_addr_match {
	uint8_t devmatch_ok;
	uint8_t devmatch_id;
};

void lll_addr_match(const struct pdu_adv *pdu, const struct lll_filter *filter,
		    struct lll_addr_match *match);

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_CONTROLLER_LL_SW_NATIVE_LLL_LLL_ADDR_H_ */
