/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* BPDU encoding and validation, IEEE Std 802.1Q-2014 Clause 14 */

#include <errno.h>
#include <string.h>

#include <zephyr/sys/byteorder.h>

#include "rstp_internal.h"

#define PROTOCOL_ID  0x0000U
#define VERSION_STP  0U
#define VERSION_RSTP 2U
#define TYPE_CONFIG  0x00U
#define TYPE_TCN     0x80U
#define TYPE_RST     0x02U

/* Octet offsets, numbered from 0 (Figure 14-1) */
#define OFF_PROTOCOL_ID 0
#define OFF_VERSION     2
#define OFF_TYPE        3
#define OFF_FLAGS       4
#define OFF_ROOT_ID     5
#define OFF_ROOT_COST   13
#define OFF_BRIDGE_ID   17
#define OFF_PORT_ID     25
#define OFF_MSG_AGE     27
#define OFF_MAX_AGE     29
#define OFF_HELLO_TIME  31
#define OFF_FWD_DELAY   33
#define OFF_V1_LENGTH   35

/* Flags that a Configuration BPDU conveys, the others are ignored (14.4) */
#define STP_BPDU_FLAGS (ETH_RSTP_FLAG_TC | ETH_RSTP_FLAG_TC_ACK)

/* Timer values are in units of 1/256 s (14.2.8) */
static uint16_t time_decode(const uint8_t *buf)
{
	return (uint16_t)((sys_get_be16(buf) + 128U) / 256U);
}

static void time_encode(uint8_t *buf, uint16_t seconds)
{
	sys_put_be16((uint16_t)MIN(seconds, 255U) * 256U, buf);
}

int eth_rstp_bpdu_decode(const uint8_t *buf, size_t len, struct eth_rstp_bpdu *bpdu)
{
	uint8_t version;
	uint8_t type;

	if (len < ETH_RSTP_BPDU_TCN_LEN || sys_get_be16(&buf[OFF_PROTOCOL_ID]) != PROTOCOL_ID) {
		return -EINVAL;
	}

	version = buf[OFF_VERSION];
	type = buf[OFF_TYPE];

	memset(bpdu, 0, sizeof(*bpdu));

	/* 14.5 a) to c). An MST BPDU (version 3 or greater) is an RST BPDU
	 * for this Bridge: "MSTP encodes the CIST Regional Root Identifier in
	 * the BPDU field used by RSTP to convey the Designated Bridge
	 * Identifier (14.1.1), so an entire region appears to an RSTP capable
	 * Bridge as a single Bridge" (13.9).
	 */
	if (type == TYPE_TCN) {
		bpdu->type = ETH_RSTP_BPDU_TCN;
		return 0;
	} else if (type == TYPE_CONFIG && len >= ETH_RSTP_BPDU_CONFIG_LEN) {
		bpdu->type = ETH_RSTP_BPDU_CONFIG;
		bpdu->flags = buf[OFF_FLAGS] & STP_BPDU_FLAGS;
	} else if (type == TYPE_RST && version >= VERSION_RSTP && len >= ETH_RSTP_BPDU_RST_LEN) {
		bpdu->type = ETH_RSTP_BPDU_RST;
		bpdu->flags = buf[OFF_FLAGS] & (uint8_t)~ETH_RSTP_FLAG_TC_ACK;
	} else {
		return -EINVAL;
	}

	bpdu->root_id = sys_get_be64(&buf[OFF_ROOT_ID]);
	bpdu->root_path_cost = sys_get_be32(&buf[OFF_ROOT_COST]);
	bpdu->bridge_id = sys_get_be64(&buf[OFF_BRIDGE_ID]);
	bpdu->port_id = sys_get_be16(&buf[OFF_PORT_ID]);
	bpdu->times.message_age = time_decode(&buf[OFF_MSG_AGE]);
	bpdu->times.max_age = time_decode(&buf[OFF_MAX_AGE]);
	bpdu->times.hello_time = time_decode(&buf[OFF_HELLO_TIME]);
	bpdu->times.forward_delay = time_decode(&buf[OFF_FWD_DELAY]);

	return 0;
}

size_t eth_rstp_bpdu_encode(const struct eth_rstp_bpdu *bpdu, uint8_t *buf, size_t len)
{
	size_t bpdu_len;

	switch (bpdu->type) {
	case ETH_RSTP_BPDU_TCN:
		bpdu_len = ETH_RSTP_BPDU_TCN_LEN;
		break;
	case ETH_RSTP_BPDU_CONFIG:
		bpdu_len = ETH_RSTP_BPDU_CONFIG_LEN;
		break;
	case ETH_RSTP_BPDU_RST:
		bpdu_len = ETH_RSTP_BPDU_RST_LEN;
		break;
	default:
		return 0;
	}

	if (len < bpdu_len) {
		return 0;
	}

	memset(buf, 0, bpdu_len);
	sys_put_be16(PROTOCOL_ID, &buf[OFF_PROTOCOL_ID]);

	/* 14.3 */
	if (bpdu->type == ETH_RSTP_BPDU_TCN) {
		buf[OFF_VERSION] = VERSION_STP;
		buf[OFF_TYPE] = TYPE_TCN;
		return bpdu_len;
	}

	if (bpdu->type == ETH_RSTP_BPDU_CONFIG) {
		buf[OFF_VERSION] = VERSION_STP;
		buf[OFF_TYPE] = TYPE_CONFIG;
		buf[OFF_FLAGS] = bpdu->flags & STP_BPDU_FLAGS;
	} else {
		buf[OFF_VERSION] = VERSION_RSTP;
		buf[OFF_TYPE] = TYPE_RST;
		buf[OFF_FLAGS] = bpdu->flags & (uint8_t)~ETH_RSTP_FLAG_TC_ACK;
		/* Version 1 Length is transmitted as 0 (14.4 p) */
		buf[OFF_V1_LENGTH] = 0U;
	}

	sys_put_be64(bpdu->root_id, &buf[OFF_ROOT_ID]);
	sys_put_be32(bpdu->root_path_cost, &buf[OFF_ROOT_COST]);
	sys_put_be64(bpdu->bridge_id, &buf[OFF_BRIDGE_ID]);
	sys_put_be16(bpdu->port_id, &buf[OFF_PORT_ID]);
	time_encode(&buf[OFF_MSG_AGE], bpdu->times.message_age);
	time_encode(&buf[OFF_MAX_AGE], bpdu->times.max_age);
	time_encode(&buf[OFF_HELLO_TIME], bpdu->times.hello_time);
	time_encode(&buf[OFF_FWD_DELAY], bpdu->times.forward_delay);

	return bpdu_len;
}
