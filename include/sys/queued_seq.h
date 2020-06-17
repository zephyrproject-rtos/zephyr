/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_QUEUED_SEQ_H_
#define ZEPHYR_INCLUDE_SYS_QUEUED_SEQ_H_

#include <sys/seq_mgr.h>
#include <sys/queued_operation.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SYS_QUEUED_SEQ_INITALIZER(_log_name) \
	{ \
		.seq_mgr = SYS_SEQ_MGR_INITIALIZER(_log_name) \
	}

struct queued_seq_mgr {
	struct queued_operation_manager qop_mgr;
	struct sys_seq_mgr seq_mgr;
	struct sys_notify seq_notify;
};

struct queued_seq {
	struct queued_operation qop;
	const struct sys_seq *seq;
};

int queued_seq_init(struct queued_seq_mgr *mgrs,
		    const struct sys_seq_functions *seq_mgr_vtable,
		    struct k_timer *delay_timer);

void queued_seq_callback(struct sys_seq_mgr *mgr,
			 struct sys_seq *seq,
			 int res,
			 sys_notify_generic_callback cb);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_QUEUED_SEQ_H_ */
