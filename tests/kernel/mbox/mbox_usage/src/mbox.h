/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct k_msg {
	/* Mailbox Id*/
	struct k_mbox *mailbox;
	/** Size of message (bytes) */
	uint32_t size;
	/** information field, free for user */
	uint32_t info;
	/** pointer to message data at sender side */
	void *tx_data;
	/** pointer to message data at receiver */
	void *rx_data;
	/** for async message posting */
}
