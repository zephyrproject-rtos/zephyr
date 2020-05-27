/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sys/notify.h>

int sys_notify_validate(struct sys_notify *notify)
{
	int rv = 0;

	if (notify == NULL) {
		return -EINVAL;
	}

	/* Validate configuration based on mode */
	switch (sys_notify_get_method(notify)) {
	case SYS_NOTIFY_METHOD_SPINWAIT:
		break;
	case SYS_NOTIFY_METHOD_CALLBACK:
		if (notify->method.callback == NULL) {
			rv = -EINVAL;
		}
		break;
#ifdef CONFIG_POLL
	case SYS_NOTIFY_METHOD_SIGNAL:
		if (notify->method.signal == NULL) {
			rv = -EINVAL;
		}
		break;
#endif /* CONFIG_POLL */
	default:
		rv = -EINVAL;
		break;
	}

	/* Clear the result here instead of in all callers. */
	if (rv == 0) {
		notify->result = 0;
	}

	return rv;
}

sys_notify_generic_callback sys_notify_finalize(struct sys_notify *notify,
						    int res)
{
	struct k_poll_signal *sig = NULL;
	sys_notify_generic_callback rv = 0;
	uint32_t method = sys_notify_get_method(notify);

	/* Store the result and capture secondary notification
	 * information.
	 */
	notify->result = res;
	switch (method) {
	case SYS_NOTIFY_METHOD_SPINWAIT:
		break;
	case SYS_NOTIFY_METHOD_CALLBACK:
		rv = notify->method.callback;
		break;
	case SYS_NOTIFY_METHOD_SIGNAL:
		sig = notify->method.signal;
		break;
	default:
		__ASSERT_NO_MSG(false);
	}

	/* Mark completion by clearing the flags field to the
	 * completed state, releasing any spin-waiters, then complete
	 * secondary notification.
	 */
	compiler_barrier();
	notify->flags = SYS_NOTIFY_METHOD_COMPLETED;

	if (IS_ENABLED(CONFIG_POLL) && (sig != NULL)) {
		k_poll_signal_raise(sig, res);
	}

	return rv;
}
