/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct node_tx {
	union {
		void        *next;
		memq_link_t *link;
	};

	u8_t pdu[];
};
