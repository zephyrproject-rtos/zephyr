/*
 * Copyright (c) 2020 Manivannan Sadhasivam <mani@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/zephyr.h>

#include "lw_priv.h"

#include <LoRaMac.h>

static const char *const status2str[] = {
	[LORAMAC_STATUS_OK] = "OK",
	[LORAMAC_STATUS_BUSY] = "Busy",
	[LORAMAC_STATUS_SERVICE_UNKNOWN] = "Service unknown",
	[LORAMAC_STATUS_PARAMETER_INVALID] = "Parameter invalid",
	[LORAMAC_STATUS_FREQUENCY_INVALID] = "Frequency invalid",
	[LORAMAC_STATUS_DATARATE_INVALID] = "Datarate invalid",
	[LORAMAC_STATUS_FREQ_AND_DR_INVALID] = "Frequency or datarate invalid",
	[LORAMAC_STATUS_NO_NETWORK_JOINED] = "No network joined",
	[LORAMAC_STATUS_LENGTH_ERROR] = "Length error",
	[LORAMAC_STATUS_REGION_NOT_SUPPORTED] = "Region not supported",
	[LORAMAC_STATUS_SKIPPED_APP_DATA] = "Skipped APP data",
	[LORAMAC_STATUS_DUTYCYCLE_RESTRICTED] = "Duty-cycle restricted",
	[LORAMAC_STATUS_NO_CHANNEL_FOUND] = "No channel found",
	[LORAMAC_STATUS_NO_FREE_CHANNEL_FOUND] = "No free channel found",
	[LORAMAC_STATUS_BUSY_BEACON_RESERVED_TIME] =
		"Busy beacon reserved time",
	[LORAMAC_STATUS_BUSY_PING_SLOT_WINDOW_TIME] =
		"Busy ping-slot window time",
	[LORAMAC_STATUS_BUSY_UPLINK_COLLISION] = "Busy uplink collision",
	[LORAMAC_STATUS_CRYPTO_ERROR] = "Crypto error",
	[LORAMAC_STATUS_FCNT_HANDLER_ERROR] = "FCnt handler error",
	[LORAMAC_STATUS_MAC_COMMAD_ERROR] = "MAC command error",
	[LORAMAC_STATUS_CLASS_B_ERROR] = "ClassB error",
	[LORAMAC_STATUS_CONFIRM_QUEUE_ERROR] = "Confirm queue error",
	[LORAMAC_STATUS_MC_GROUP_UNDEFINED] = "Multicast group undefined",
	[LORAMAC_STATUS_ERROR] = "Unknown error",
};

const char *lorawan_status2str(unsigned int status)
{
	if (status < ARRAY_SIZE(status2str)) {
		return status2str[status];
	} else {
		return "Unknown status!";
	}
}

static const char *const eventinfo2str[] = {
	[LORAMAC_EVENT_INFO_STATUS_OK] = "OK",
	[LORAMAC_EVENT_INFO_STATUS_ERROR] = "Error",
	[LORAMAC_EVENT_INFO_STATUS_TX_TIMEOUT] = "Tx timeout",
	[LORAMAC_EVENT_INFO_STATUS_RX1_TIMEOUT] = "Rx 1 timeout",
	[LORAMAC_EVENT_INFO_STATUS_RX2_TIMEOUT] = "Rx 2 timeout",
	[LORAMAC_EVENT_INFO_STATUS_RX1_ERROR] = "Rx1 error",
	[LORAMAC_EVENT_INFO_STATUS_RX2_ERROR] = "Rx2 error",
	[LORAMAC_EVENT_INFO_STATUS_JOIN_FAIL] = "Join failed",
	[LORAMAC_EVENT_INFO_STATUS_DOWNLINK_REPEATED] = "Downlink repeated",
	[LORAMAC_EVENT_INFO_STATUS_TX_DR_PAYLOAD_SIZE_ERROR] =
		"Tx DR payload size error",
	[LORAMAC_EVENT_INFO_STATUS_ADDRESS_FAIL] = "Address fail",
	[LORAMAC_EVENT_INFO_STATUS_MIC_FAIL] = "MIC fail",
	[LORAMAC_EVENT_INFO_STATUS_MULTICAST_FAIL] = "Multicast fail",
	[LORAMAC_EVENT_INFO_STATUS_BEACON_LOCKED] = "Beacon locked",
	[LORAMAC_EVENT_INFO_STATUS_BEACON_LOST] = "Beacon lost",
	[LORAMAC_EVENT_INFO_STATUS_BEACON_NOT_FOUND] = "Beacon not found",
};

const char *lorawan_eventinfo2str(unsigned int status)
{
	if (status < ARRAY_SIZE(eventinfo2str)) {
		return eventinfo2str[status];
	} else {
		return "Unknown event!";
	}
}

/*
 * MAC status and Event status to Zephyr error code conversion.
 * Direct mapping is not possible as statuses often indicate the domain from
 * which the error originated rather than its cause or meaning. -EINVAL has been
 * used as a general error code because those usually result from incorrect
 * configuration.
 */
static const int status2errno[] = {
	[LORAMAC_STATUS_BUSY] = -EBUSY,
	[LORAMAC_STATUS_SERVICE_UNKNOWN] = -ENOPROTOOPT,
	[LORAMAC_STATUS_PARAMETER_INVALID] = -EINVAL,
	[LORAMAC_STATUS_FREQUENCY_INVALID] = -EINVAL,
	[LORAMAC_STATUS_DATARATE_INVALID] = -EINVAL,
	[LORAMAC_STATUS_FREQ_AND_DR_INVALID] = -EINVAL,
	[LORAMAC_STATUS_NO_NETWORK_JOINED] = -ENOTCONN,
	[LORAMAC_STATUS_LENGTH_ERROR] = -EMSGSIZE,
	[LORAMAC_STATUS_REGION_NOT_SUPPORTED] = -EPFNOSUPPORT,
	[LORAMAC_STATUS_SKIPPED_APP_DATA] = -EMSGSIZE,
	[LORAMAC_STATUS_DUTYCYCLE_RESTRICTED] = -ECONNREFUSED,
	[LORAMAC_STATUS_NO_CHANNEL_FOUND] = -ENOTCONN,
	[LORAMAC_STATUS_NO_FREE_CHANNEL_FOUND] = -ENOTCONN,
	[LORAMAC_STATUS_BUSY_BEACON_RESERVED_TIME] = -EBUSY,
	[LORAMAC_STATUS_BUSY_PING_SLOT_WINDOW_TIME] = -EBUSY,
	[LORAMAC_STATUS_BUSY_UPLINK_COLLISION] = -EBUSY,
	[LORAMAC_STATUS_CRYPTO_ERROR] = -EINVAL,
	[LORAMAC_STATUS_FCNT_HANDLER_ERROR] = -EINVAL,
	[LORAMAC_STATUS_MAC_COMMAD_ERROR] = -EINVAL,
	[LORAMAC_STATUS_CLASS_B_ERROR] = -EINVAL,
	[LORAMAC_STATUS_CONFIRM_QUEUE_ERROR] = -EINVAL,
	[LORAMAC_STATUS_MC_GROUP_UNDEFINED] = -EINVAL,
};

const int lorawan_status2errno(unsigned int status)
{
	if (status < ARRAY_SIZE(status2errno) && status2errno[status] != 0) {
		return status2errno[status];
	} else {
		return status == LORAMAC_STATUS_OK ? 0 : -EINVAL;
	}
}

static const int eventinfo2errno[] = {
	[LORAMAC_EVENT_INFO_STATUS_ERROR] = -EINVAL,
	[LORAMAC_EVENT_INFO_STATUS_TX_TIMEOUT] = -ETIMEDOUT,
	[LORAMAC_EVENT_INFO_STATUS_RX1_TIMEOUT] = -ETIMEDOUT,
	[LORAMAC_EVENT_INFO_STATUS_RX2_TIMEOUT] = -ETIMEDOUT,
	[LORAMAC_EVENT_INFO_STATUS_RX1_ERROR] = -EINVAL,
	[LORAMAC_EVENT_INFO_STATUS_RX2_ERROR] = -EINVAL,
	[LORAMAC_EVENT_INFO_STATUS_JOIN_FAIL] = -EINVAL,
	[LORAMAC_EVENT_INFO_STATUS_DOWNLINK_REPEATED] = -ECONNRESET,
	[LORAMAC_EVENT_INFO_STATUS_TX_DR_PAYLOAD_SIZE_ERROR] = -EMSGSIZE,
	[LORAMAC_EVENT_INFO_STATUS_ADDRESS_FAIL] = -EACCES,
	[LORAMAC_EVENT_INFO_STATUS_MIC_FAIL] = -EACCES,
	[LORAMAC_EVENT_INFO_STATUS_MULTICAST_FAIL] = -EINVAL,
	[LORAMAC_EVENT_INFO_STATUS_BEACON_LOCKED] = -EINVAL,
	[LORAMAC_EVENT_INFO_STATUS_BEACON_LOST] = -EINVAL,
	[LORAMAC_EVENT_INFO_STATUS_BEACON_NOT_FOUND] = -EINVAL,
};

const int lorawan_eventinfo2errno(unsigned int status)
{
	if (status < ARRAY_SIZE(eventinfo2errno) &&
	    eventinfo2errno[status] != 0) {
		return eventinfo2errno[status];
	} else {
		return status == LORAMAC_EVENT_INFO_STATUS_OK ? 0 : -EINVAL;
	}
}
