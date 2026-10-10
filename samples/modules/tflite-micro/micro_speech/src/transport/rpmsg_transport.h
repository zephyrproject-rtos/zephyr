/*
 * Copyright (c) 2025, NXP
 * Author: Thong Phan <quang.thong2001@gmail.com>
 *
 * Heavily based on pwm_mcux_ftm.c, which is:
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MICRO_SPEECH_OPENAMP_RPMSG_TRANSPORT_H
#define MICRO_SPEECH_OPENAMP_RPMSG_TRANSPORT_H

#include <zephyr/kernel.h>
#include <openamp/open_amp.h>

#ifdef __cplusplus
extern "C" {
#endif

struct rpmsg_rcv_msg {
	void *data;
	size_t len;
};

extern struct k_msgq tty_msgq;
extern struct rpmsg_endpoint tty_ept;
extern struct k_sem data_tty_ready_sem;
extern struct rpmsg_device *rpdev;

void rpmsg_transport_start(void);
int rpmsg_recv_tty_callback(struct rpmsg_endpoint *ept, void *data, size_t len, uint32_t src,
			    void *priv);

#ifdef __cplusplus
}
#endif

#endif /* RPMSG_TRANSPORT_H */
