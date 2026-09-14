/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <errno.h>
#include <zephyr/sys/crc.h>
#include <zephyr/logging/log.h>
#include <zephyr/pmci/mctp/mctp_i3c_pec.h>
LOG_MODULE_REGISTER(mctp_i3c_pec, CONFIG_MCTP_LOG_LEVEL);

/* PEC as per DSP0233 1.0.0: CRC-8 (CCITT, poly 0x07) with address byte seed */
uint8_t mctp_i3c_calculate_pec(const uint8_t *buf, size_t len, uint8_t addr_byte)
{
	uint8_t pec = crc8_ccitt(0, &addr_byte, sizeof(addr_byte));

	return crc8_ccitt(pec, buf, len);
}

int mctp_i3c_verify_pec(const uint8_t *buf, size_t len, uint8_t addr_byte)
{
	/* Minimum: 1 byte payload + 1 byte PEC */
	if (len < (1U + I3C_PROTOCOL_PEC_SZ)) {
		LOG_ERR("Packet too short to contain PEC: %zu bytes", len);
		return -EBADMSG;
	}

	uint8_t received_pec = buf[len - I3C_PROTOCOL_PEC_SZ];
	uint8_t calculated_pec = mctp_i3c_calculate_pec(buf, len - I3C_PROTOCOL_PEC_SZ, addr_byte);

	if (calculated_pec != received_pec) {
		LOG_ERR("PEC mismatch: got 0x%02x expected 0x%02x",
			received_pec, calculated_pec);
		return -EBADMSG;
	}

	return 0;
}
