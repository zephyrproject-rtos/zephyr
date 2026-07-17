/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2026 EPAM Systems
 */

#include <zephyr/sys/util.h>
#include <zephyr/xen/flask.h>
#include <zephyr/arch/arm64/hypercall.h>

static inline int do_flask_op(struct xen_flask_op *op)
{
	op->interface_version = XEN_FLASK_INTERFACE_VERSION;
	return HYPERVISOR_xsm_op(op);
}

int flask_context_to_sid(char *buf, uint32_t size, uint32_t *sid)
{
	int err;
	struct xen_flask_op op = {
		.cmd = FLASK_CONTEXT_TO_SID,
		.u.sid_context.size = size,
	};

	set_xen_guest_handle(op.u.sid_context.context, buf);

	err = do_flask_op(&op);
	*sid = op.u.sid_context.sid;
	return err;
}

int flask_getenforce(void)
{
	struct xen_flask_op op = {
		.cmd = FLASK_GETENFORCE,
	};

	return do_flask_op(&op);
}
