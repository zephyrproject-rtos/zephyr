/*
 * Copyright (c) 2025 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/logging/log.h>

#include "lw_priv.h"

LOG_MODULE_DECLARE(lorawan);

int lorawan_txstatus2errno(smtc_modem_event_txdone_status_t status, bool confirmed)
{
	switch (status) {
	case SMTC_MODEM_EVENT_TXDONE_CONFIRMED:
		return 0;
	case SMTC_MODEM_EVENT_TXDONE_SENT:
		if (confirmed) {
			LOG_WRN("Confirmed uplink sent but no ACK received");
			return -EAGAIN;
		}
		return 0;
	case SMTC_MODEM_EVENT_TXDONE_NOT_SENT:
		LOG_ERR("Uplink was not sent (aborted)");
		return -EBUSY;
	default:
		LOG_ERR("Unknown TX status: %d", status);
		return -EIO;
	}
}
