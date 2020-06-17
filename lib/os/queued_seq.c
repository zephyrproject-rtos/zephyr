/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <sys/queued_seq.h>

static void qop_callback(struct queued_operation_manager *mgr,
			 struct queued_operation *qop,
			 sys_notify_generic_callback cb)
{
	int res;
	int err;

	err = sys_notify_fetch_result(&qop->notify, &res);
	if (err < 0) {
		__ASSERT_NO_MSG(0);
		res = err;
	}

	cb(mgr, qop, res);
}

static void seq_process(struct queued_operation_manager *mgr,
			struct queued_operation *qop)
{
	struct queued_seq *op = CONTAINER_OF(qop, struct queued_seq, qop);
	struct queued_seq_mgr *mgrs =
			CONTAINER_OF(mgr, struct queued_seq_mgr, qop_mgr);
	int err;

	sys_notify_init_callback(&mgrs->seq_notify,
				   queued_operation_finalize);
	err = sys_seq_process(&mgrs->seq_mgr, op->seq, &mgrs->seq_notify);
	if (err < 0) {
		queued_operation_finalize(&mgrs->qop_mgr, err);
	}
}

static const struct queued_operation_functions queued_seq_qop_functions = {
	.process = seq_process,
	.callback = qop_callback
};

void queued_seq_callback(struct sys_seq_mgr *mgr,
			 struct sys_seq *seq,
			 int res,
			 sys_notify_generic_callback cb)
{
	struct queued_seq_mgr *mgrs =
		CONTAINER_OF(mgr, struct queued_seq_mgr, seq_mgr);

	cb(&mgrs->qop_mgr, res);
}

static const struct sys_seq_functions queued_seq_functions = {
	.callback = queued_seq_callback
};

int queued_seq_init(struct queued_seq_mgr *mgrs,
		     const struct sys_seq_functions *seq_mgr_vtable,
		     struct k_timer *delay_timer)
{
	mgrs->qop_mgr.vtable = &queued_seq_qop_functions;

	if (seq_mgr_vtable) {
		__ASSERT_NO_MSG(seq_mgr_vtable->callback = queued_seq_callback);
	} else {
		seq_mgr_vtable = &queued_seq_functions;
	}

	return sys_seq_mgr_init(&mgrs->seq_mgr, seq_mgr_vtable, delay_timer);
}
