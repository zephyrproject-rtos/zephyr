/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ipc/ipc_icmsg_buf.h>

enum icmsg_state {
	ICMSG_STATE_OFF,
	ICMSG_STATE_BUSY,
	ICMSG_STATE_READY
};
