/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ptp_msg, CONFIG_PTP_LOG_LEVEL);

#include <zephyr/kernel.h>

#include "msg.h"

static struct k_mem_slab msg_slab;

K_MEM_SLAB_DEFINE_STATIC(msg_slab, sizeof(struct ptp_msg), CONFIG_PTP_MSG_POLL_SIZE, 8);

struct ptp_msg *ptp_msg_alloc(void)
{
	struct ptp_msg *msg = NULL;
	int ret = k_mem_slab_alloc(&msg_slab, (void **)&msg, K_FOREVER);

	if (ret) {
		LOG_ERR("Couldn't allocate memory for the message");
		return NULL;
	}

	memset(msg, 0, sizeof(*msg));
	atomic_inc(&msg->ref);

	return msg;
}

void ptp_msg_unref(struct ptp_msg *msg)
{
	__ASSERT_NO_MSG(msg != NULL);

	atomic_t val = atomic_dec(&msg->ref);

	if (val > 1) {
		return;
	}

	k_mem_slab_free(&msg_slab, (void *)msg);
}

void ptp_msg_ref(struct ptp_msg *msg)
{
	__ASSERT_NO_MSG(msg != NULL);

	atomic_inc(&msg->ref);
}
