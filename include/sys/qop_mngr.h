/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_QOP_MNGR_H_
#define ZEPHYR_INCLUDE_SYS_QOP_MNGR_H_

#include <kernel.h>
#include <zephyr/types.h>
#include <sys/async_client.h>

#ifdef __cplusplus
extern "C" {
#endif

#define QOP_MNGR_PRI_LOWEST 0
#define QOP_MNGR_PRI_HIGHEST UINT8_MAX

#define QOP_MNGR_FLAGS_OP_SLEEPS	BIT(0)
#define QOP_MNGR_FLAGS_PRI		BIT(1)

struct qop_mngr;

typedef int (*qop_mngr_fn)(struct qop_mngr *mngr);

struct qop_mngr {
	sys_slist_t ops;
	struct k_spinlock lock;
	qop_mngr_fn op_fn;
	u16_t flags;
};

struct qop_op;

typedef void (*qop_op_callback)(struct qop_mngr *mngr, struct qop_op *op,
				int res);

struct qop_op {
	/* Links the client into the set of waiting service users. */
	sys_snode_t node;

	struct async_client *async_cli;

	void *data;
};

int qop_mngr_init(struct qop_mngr *mngr, qop_mngr_fn fn, u16_t flags);
void qop_op_done_notify(struct qop_mngr *mngr, int res);
int qop_op_schedule(struct qop_mngr *mngr, struct qop_op *op);
int qop_op_cancel(struct qop_mngr *mngr, struct qop_op *op);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_QOP_MNGR_H_ */
