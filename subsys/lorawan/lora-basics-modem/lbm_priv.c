/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>

#include <zephyr/sys/util.h>

#include "lbm_priv.h"

static const int rc2errno[] = {
	[SMTC_MODEM_RC_OK] = 0,
	[SMTC_MODEM_RC_NOT_INIT] = -EPERM,
	[SMTC_MODEM_RC_INVALID] = -EINVAL,
	[SMTC_MODEM_RC_BUSY] = -EBUSY,
	[SMTC_MODEM_RC_FAIL] = -EIO,
	[SMTC_MODEM_RC_NO_TIME] = -EAGAIN,
	[SMTC_MODEM_RC_INVALID_STACK_ID] = -ENODEV,
	[SMTC_MODEM_RC_NO_EVENT] = -ENOMSG,
};

static const char *const rc2str[] = {
	[SMTC_MODEM_RC_OK] = "OK",
	[SMTC_MODEM_RC_NOT_INIT] = "not initialised",
	[SMTC_MODEM_RC_INVALID] = "invalid parameter",
	[SMTC_MODEM_RC_BUSY] = "busy",
	[SMTC_MODEM_RC_FAIL] = "failed",
	[SMTC_MODEM_RC_NO_TIME] = "no time available",
	[SMTC_MODEM_RC_INVALID_STACK_ID] = "invalid stack id",
	[SMTC_MODEM_RC_NO_EVENT] = "no event",
};

static const char *const txdone2str[] = {
	[SMTC_MODEM_EVENT_TXDONE_NOT_SENT] = "not sent",
	[SMTC_MODEM_EVENT_TXDONE_SENT] = "sent",
	[SMTC_MODEM_EVENT_TXDONE_CONFIRMED] = "confirmed",
};

int lbm_rc2errno(smtc_modem_return_code_t rc)
{
	if (rc >= ARRAY_SIZE(rc2errno)) {
		return -EIO;
	}

	return rc2errno[rc];
}

const char *lbm_rc2str(smtc_modem_return_code_t rc)
{
	if (rc >= ARRAY_SIZE(rc2str)) {
		return "unknown";
	}

	return rc2str[rc];
}

int lbm_txdone2errno(smtc_modem_event_txdone_status_t status, bool confirmed)
{
	if (status == SMTC_MODEM_EVENT_TXDONE_NOT_SENT) {
		return -EIO;
	}

	/* A confirmed uplink is only done once the network has acknowledged it. */
	if (confirmed && status != SMTC_MODEM_EVENT_TXDONE_CONFIRMED) {
		return -EAGAIN;
	}

	return 0;
}

const char *lbm_txdone2str(smtc_modem_event_txdone_status_t status)
{
	if (status >= ARRAY_SIZE(txdone2str)) {
		return "unknown";
	}

	return txdone2str[status];
}
