/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CAVS_TEST_H
#define ZEPHYR_INCLUDE_CAVS_TEST_H

/* The cavstool.py script that launched us listens for a very simple
 * set of IPC commands to help test.  Pass one of the following values
 * as the "data" argument to intel_adsp_ipc_send_message(INTEL_ADSP_IPC_HOST_DEV, ...):
 */
enum cavstool_cmd {
	/* The host takes no action, but signals DONE to complete the message */
	IPCCMD_SIGNAL_DONE,

	/* The host returns done after a short timeout */
	IPCCMD_ASYNC_DONE_DELAY,

	/* The host issues a new message with the ext_data value as its "data" */
	IPCCMD_RETURN_MSG,

	/* The host writes the given value to ADSPCS */
	IPCCMD_ADSPCS,

	/* The host emits a (real/host time) timestamp into the log stream */
	IPCCMD_TIMESTAMP,

	/* The host copies OUTBOX[ext_data >> 16] to INBOX[ext_data & 0xffff] */
	IPCCMD_WINCOPY,

	/* The host clears the run bit and resets the HDA stream */
	IPCCMD_HDA_RESET,

	/* The host configures an HDA stream (with provided buffer size and stream id) */
	IPCCMD_HDA_CONFIG,

	/* The host runs (sets the SDxCTL.RUN bit) for a given HDA stream */
	IPCCMD_HDA_START,

	/* The host stops (sets the SDxCTL.RUN bit) for a given HDA stream */
	IPCCMD_HDA_STOP,

	/* The host validates an HDA byte stream contains an 8bit counter and received a given
	 * number of bytes
	 */
	IPCCMD_HDA_VALIDATE,

	/* Host sends some data */
	IPCCMD_HDA_SEND,

	/* Host prints some data */
	IPCCMD_HDA_PRINT
};

#endif /* ZEPHYR_INCLUDE_CAVS_TEST_H */
