/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018-2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LWM2M_PULL_CONTEXT_H
#define LWM2M_PULL_CONTEXT_H

#include <stdio.h>
#include <zephyr/net/lwm2m.h>
#include <zephyr/sys/clock.h>

#define LWM2M_PACKAGE_URI_LEN CONFIG_LWM2M_SWMGMT_PACKAGE_URI_LEN

struct requesting_object {
	uint8_t obj_inst_id;
	bool is_firmware_uri;

	void (*result_cb)(uint16_t obj_inst_id, int error_code);
	lwm2m_engine_set_data_cb_t write_cb;
	int (*verify_cb)(void);
};

/*
 * The pull context is also used in the LwM2M's Software Management object.
 * This means that the transfer needs to know if it's used for firmware or
 * something else.
 *
 * A refused start is reported only through the return value; the result
 * callback runs for a transfer that did start. A start requested while the
 * previous transfer is still closing is queued and runs once the context is
 * released. Callers hold the registry lock, which the pull service takes
 * before it releases the context.
 */
int lwm2m_pull_context_start_transfer(char *uri, struct requesting_object req, k_timeout_t timeout);

/*
 * Stop the transfer in progress, or drop a queued one, without calling its
 * result callback. Blocks that arrive after the call are dropped. Only a
 * transfer of the given kind is stopped, so the firmware object cannot
 * interrupt a software management pull and vice versa. Callers hold the
 * registry lock, which the transfer's own callbacks take as well.
 *
 * Returns 0 when a transfer was stopped, -ENOENT when none is in progress and
 * -EBUSY when the transfer in progress belongs to the other kind of object.
 */
int lwm2m_pull_context_cancel(bool is_firmware_uri);

#endif /* LWM2M_PULL_CONTEXT_H */
