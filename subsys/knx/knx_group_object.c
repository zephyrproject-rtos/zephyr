/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "knx_group_object.h"
#include "object_group_table.h"
#include "object_association_table.h"
#include "object_device.h"
#include "layer7_application.h"
#include <zephyr/knx/knx_pkt.h>
#include <zephyr/knx/dpt.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(knx_go, CONFIG_KNX_STACK_LOG_LEVEL);

bool knx_group_object_write(uint8_t asap)
{
	uint8_t flag = group_object_flag(asap);
	uint8_t *data = group_object_data(asap);
	uint8_t size = group_object_size(asap);
	bool is_short = group_object_is_short_form(asap);
	int32_t tsap;
	struct knx_pkt *pkt;
	uint8_t apdu[2 + 14];
	uint8_t apdu_len;

	if (data == NULL) {
		LOG_WRN("write: no object for ASAP %u", asap);
		return false;
	}
	if (!((flag & GROUP_OBJECT_FLAG_C) && (flag & GROUP_OBJECT_FLAG_T))) {
		LOG_DBG("write: C/T flag not set for ASAP %u", asap);
		return false;
	}

	tsap = association_table_translate_asap(asap);
	if (tsap < 0) {
		LOG_WRN("write: no TSAP bound to ASAP %u", asap);
		return false;
	}

	pkt = knx_pkt_alloc(K_NO_WAIT);
	if (!pkt) {
		LOG_ERR("write: pkt alloc failed for ASAP %u", asap);
		return false;
	}

	if (is_short) {
		/* Short form: pack <=6-bit value into APCI_lo bits[5:0]. */
		apdu[0] = (A_GroupValue_Write >> 8) & 0xFFu;
		apdu[1] = (A_GroupValue_Write & 0xFFu) | (data[0] & 0x3Fu);
		apdu_len = 2;
	} else {
		apdu[0] = (A_GroupValue_Write >> 8) & 0xFFu;
		apdu[1] = A_GroupValue_Write & 0xFFu;
		memcpy(&apdu[2], data, size);
		apdu_len = 2 + size;
	}

	memcpy(pkt->buf, apdu, apdu_len);
	pkt->lsdu = pkt->buf;
	pkt->lsdu_len = apdu_len;
	pkt->tsap = (uint16_t)tsap;
	pkt->priority = (knx_priority_t)(flag & GROUP_OBJECT_FLAG_PRIORITY);
	pkt->hop_count = device_routing_count();
	pkt->ack_request = 0; /* matches the existing L7 outbound-response convention */

	A_GroupValue_Write__req(pkt);
	return true;
}

/*
 * Group Object 'I' flag (read-on-init). Not in the AIL
 * spec's 5-flag config set (03_04_01 AIL v02.01.01 §3.2, p.6-7: only
 * Read/Write/Transmit/Communication/Update Enable) — this is the de facto
 * 6th descriptor bit every KNX stack checked for this project implements
 * identically. Confirmed bit-for-bit against the reference (known-passing)
 * stack: object_group_table.h's GROUP_OBJECT_FLAG_I = 0x20 is bit 13 of the
 * 16-bit descriptor there (GroupObject::valueReadOnInit(),
 * group_object.cpp:49), the same position as U=bit15/0x80 and T=bit14/0x40
 * map onto our byte. Communication Enable (C) gates the actual send, exactly
 * as bau_systemB.cpp:61 does before dispatching a queued ReadRequest — I
 * alone only decides whether to queue one.
 *
 * Called on every transition of the Group Object Table's Load State into
 * LS_LOADED — see object_group_table_properties_written() for the live ETS
 * download path, and knx_core.c for the boot-restore path (NVS already
 * holding LS_LOADED, so no write ever happens to trigger the callback).
 */
void group_object_read_on_init_trigger(void)
{
	uint16_t count;

	if (!group_object_table_is_loaded()) {
		return;
	}
	count = group_object_count();

	for (uint16_t asap = 1; asap <= count; asap++) {
		uint8_t flag = group_object_flag(asap);
		int32_t tsap;
		struct knx_pkt *pkt;

		if (!((flag & GROUP_OBJECT_FLAG_I) && (flag & GROUP_OBJECT_FLAG_C))) {
			continue;
		}

		tsap = association_table_translate_asap(asap);
		if (tsap < 0) {
			LOG_WRN("read_on_init: no TSAP bound to ASAP %u", asap);
			continue;
		}

		pkt = knx_pkt_alloc(K_NO_WAIT);
		if (!pkt) {
			LOG_ERR("read_on_init: pkt alloc failed for ASAP %u", asap);
			continue;
		}

		pkt->tsap = (uint16_t)tsap;
		pkt->priority = (knx_priority_t)(flag & GROUP_OBJECT_FLAG_PRIORITY);
		pkt->hop_count = device_routing_count();
		pkt->ack_request = 0;

		A_GroupValue_Read__req(pkt);
	}
}

/* ============================================================================
 * Typed accessors — encode/decode in place, no bus traffic.
 * ============================================================================
 */

bool knx_group_object_set_bool(uint8_t asap, bool value)
{
	uint8_t *data = group_object_data(asap);

	if (data == NULL || group_object_size(asap) != 1) {
		LOG_WRN("set_bool: ASAP %u missing or size mismatch", asap);
		return false;
	}
	dpt1_encode(value, data);
	return true;
}

bool knx_group_object_get_bool(uint8_t asap)
{
	uint8_t *data = group_object_data(asap);

	if (data == NULL || group_object_size(asap) != 1) {
		LOG_WRN("get_bool: ASAP %u missing or size mismatch", asap);
		return false;
	}
	return dpt1_decode(data);
}

bool knx_group_object_set_u8(uint8_t asap, uint8_t value)
{
	uint8_t *data = group_object_data(asap);

	if (data == NULL || group_object_size(asap) != 1) {
		LOG_WRN("set_u8: ASAP %u missing or size mismatch", asap);
		return false;
	}
	dpt5_encode(value, data);
	return true;
}

uint8_t knx_group_object_get_u8(uint8_t asap)
{
	uint8_t *data = group_object_data(asap);

	if (data == NULL || group_object_size(asap) != 1) {
		LOG_WRN("get_u8: ASAP %u missing or size mismatch", asap);
		return 0;
	}
	return dpt5_decode(data);
}

bool knx_group_object_set_u16(uint8_t asap, uint16_t value)
{
	uint8_t *data = group_object_data(asap);

	if (data == NULL || group_object_size(asap) != 2) {
		LOG_WRN("set_u16: ASAP %u missing or size mismatch", asap);
		return false;
	}
	dpt7_encode(value, data);
	return true;
}

uint16_t knx_group_object_get_u16(uint8_t asap)
{
	uint8_t *data = group_object_data(asap);

	if (data == NULL || group_object_size(asap) != 2) {
		LOG_WRN("get_u16: ASAP %u missing or size mismatch", asap);
		return 0;
	}
	return dpt7_decode(data);
}

bool knx_group_object_set_i16(uint8_t asap, int16_t value)
{
	uint8_t *data = group_object_data(asap);

	if (data == NULL || group_object_size(asap) != 2) {
		LOG_WRN("set_i16: ASAP %u missing or size mismatch", asap);
		return false;
	}
	dpt8_encode(value, data);
	return true;
}

int16_t knx_group_object_get_i16(uint8_t asap)
{
	uint8_t *data = group_object_data(asap);

	if (data == NULL || group_object_size(asap) != 2) {
		LOG_WRN("get_i16: ASAP %u missing or size mismatch", asap);
		return 0;
	}
	return dpt8_decode(data);
}

bool knx_group_object_set_u32(uint8_t asap, uint32_t value)
{
	uint8_t *data = group_object_data(asap);

	if (data == NULL || group_object_size(asap) != 4) {
		LOG_WRN("set_u32: ASAP %u missing or size mismatch", asap);
		return false;
	}
	dpt12_encode(value, data);
	return true;
}

uint32_t knx_group_object_get_u32(uint8_t asap)
{
	uint8_t *data = group_object_data(asap);

	if (data == NULL || group_object_size(asap) != 4) {
		LOG_WRN("get_u32: ASAP %u missing or size mismatch", asap);
		return 0;
	}
	return dpt12_decode(data);
}

bool knx_group_object_set_i32(uint8_t asap, int32_t value)
{
	uint8_t *data = group_object_data(asap);

	if (data == NULL || group_object_size(asap) != 4) {
		LOG_WRN("set_i32: ASAP %u missing or size mismatch", asap);
		return false;
	}
	dpt13_encode(value, data);
	return true;
}

int32_t knx_group_object_get_i32(uint8_t asap)
{
	uint8_t *data = group_object_data(asap);

	if (data == NULL || group_object_size(asap) != 4) {
		LOG_WRN("get_i32: ASAP %u missing or size mismatch", asap);
		return 0;
	}
	return dpt13_decode(data);
}

bool knx_group_object_set_float16(uint8_t asap, float value)
{
	uint8_t *data = group_object_data(asap);

	if (data == NULL || group_object_size(asap) != 2) {
		LOG_WRN("set_float16: ASAP %u missing or size mismatch", asap);
		return false;
	}
	dpt9_encode(value, data);
	return true;
}

float knx_group_object_get_float16(uint8_t asap)
{
	uint8_t *data = group_object_data(asap);

	if (data == NULL || group_object_size(asap) != 2) {
		LOG_WRN("get_float16: ASAP %u missing or size mismatch", asap);
		return 0.0f;
	}
	return dpt9_decode(data);
}

bool knx_group_object_set_float32(uint8_t asap, float value)
{
	uint8_t *data = group_object_data(asap);

	if (data == NULL || group_object_size(asap) != 4) {
		LOG_WRN("set_float32: ASAP %u missing or size mismatch", asap);
		return false;
	}
	dpt14_encode(value, data);
	return true;
}

float knx_group_object_get_float32(uint8_t asap)
{
	uint8_t *data = group_object_data(asap);

	if (data == NULL || group_object_size(asap) != 4) {
		LOG_WRN("get_float32: ASAP %u missing or size mismatch", asap);
		return 0.0f;
	}
	return dpt14_decode(data);
}
