/*
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Structure used to double buffer pointers of AD Data PDU buffer.
 * The first and last members are used to make modification to AD data to be
 * context safe. Thread always appends or updates the buffer pointed to
 * the array element indexed by the member last.
 * LLL in the ISR context, checks, traverses to the valid pointer indexed
 * by the member first, such that the buffer is the latest committed by
 * the thread context.
 */
struct lll_adv_pdu {
	uint8_t volatile first;
	uint8_t          last;
	uint8_t          *pdu[DOUBLE_BUFFER_SIZE];
};
