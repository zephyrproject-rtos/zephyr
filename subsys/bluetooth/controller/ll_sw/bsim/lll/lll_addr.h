/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Every role filters the first address of a received PDU (AdvA, ScanA or
 * InitA) the same way. A NULL filter skips the Filter Accept List match.
 */
struct lll_addr_match {
	uint8_t devmatch_ok;
	uint8_t devmatch_id;
};

void lll_addr_match(const struct pdu_adv *pdu, const struct lll_filter *filter,
		    struct lll_addr_match *match);
