/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 *
 * Layer 7 (Application) — APDU service handlers.
 *
 * Ownership convention (same as L4):
 *   __ind  : borrows pkt — must NOT free it (L4 frees after dispatch).
 *   __req / __res : own pkt — must free it or pass to T layer which frees it.
 *
 * APDU byte layout for connected services (lsdu = T_Data_Connected payload):
 *   lsdu[0] = TPCI byte  (T_Data_Connected_PDU | seqNo | APCI[9:8])
 *   lsdu[1] = APCI[7:0]
 *   lsdu[2..] = data
 *
 * Point-to-point connectionless (T_Data_Individual) uses the
 * identical layout minus the sequence number (lsdu[0] = T_Data_Individual_PDU
 * | APCI[9:8]) — l7_dispatch_apdu() (layer4_transport.c) and send_response()
 * below handle both through the same code, distinguished only by
 * pkt->connectionless.
 *
 * APDU byte layout for group / broadcast (lsdu IS the APDU):
 *   lsdu[0] = APCI[9:2]   (high byte)
 *   lsdu[1] = APCI[1:0] | small-data (low byte)
 *   lsdu[2..] = data bytes (for larger data types)
 */

#include "layer7_application.h"
#include "layer4_transport.h"
#include "object_device.h"
#include "object_group_table.h"
#include "object_interface.h"
#include "object_memory.h"
#include <zephyr/knx/knx_pkt.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>
#include <string.h>
#include <stdbool.h>

LOG_MODULE_REGISTER(knx_l7, CONFIG_KNX_STACK_LOG_LEVEL);

/* Forward declarations of static response helpers used by work functions. */
static void send_broadcast_response(knx_priority_t priority, const uint8_t *apdu, uint8_t apdu_len);

/* Delayed reboot work — lets the MasterReset response frame reach the bus
 * before the device restarts (KNX spec 3/3/7 §3.7.4).
 */
static struct k_work_delayable s_reboot_work;

/*
 * Delayed A_IndividualAddress_Response work.
 *
 * KNX spec §15.1 / §15.4 (TP1): when a device in programming mode receives
 * A_IndividualAddress_Read it must wait a random 0–10 × T_media (0–200 ms)
 * before responding.  Multiple devices may be in programming mode at the same
 * time; the staggered delay prevents simultaneous bus collisions.
 *
 * The priority is captured from the incoming pkt before the handler returns,
 * since the pkt is freed by L4 immediately after the .ind call.
 * k_work_reschedule_for_queue is used so a second Read resets the timer
 * rather than queuing a second response.
 */
static struct k_work_delayable s_ia_read_resp_work;
static knx_priority_t s_ia_read_resp_priority;

static void ia_read_resp_work_fn(struct k_work *w)
{
	ARG_UNUSED(w);

	uint8_t apdu[2] = {
		(A_IndividualAddress_Response >> 8) & 0xFFu,
		A_IndividualAddress_Response & 0xFFu,
	};

	send_broadcast_response(s_ia_read_resp_priority, apdu, sizeof(apdu));
}

static void reboot_work_fn(struct k_work *w)
{
	ARG_UNUSED(w);
	memory_write();
	sys_reboot(SYS_REBOOT_COLD);
}

/* APCI for A_Restart MasterReset response (restart_type=1 | response indicator). */
#define A_Restart_Response (A_Restart | 0x20u)

#define MAX_PROPERTY_READ_SIZE 32

/* ============================================================================
 * Internal helpers — allocate a response pkt and dispatch it
 * ============================================================================
 */

/*
 * send_response — reply to an L7 __ind request over whichever transport it
 * arrived on: T_Data_Individual (point-to-point
 * connectionless, replying to req->src) if req->connectionless was set by
 * l7_dispatch_apdu(), T_Data_Connected (the open connection) otherwise. This
 * is the one place that distinction matters — every __ind handler just
 * calls this the same way regardless of which transport its request used.
 */
static void send_response(struct knx_pkt *req, const uint8_t *apdu, uint8_t apdu_len)
{
	struct knx_pkt *resp = knx_pkt_alloc(K_NO_WAIT);

	if (!resp) {
		LOG_ERR("alloc failed");
		return;
	}
	memcpy(resp->buf, apdu, apdu_len);
	resp->lsdu = resp->buf;
	resp->lsdu_len = apdu_len;
	resp->priority = req->priority;
	resp->hop_count = device_routing_count();
	resp->ack_request = 0;

	if (req->connectionless) {
		resp->dst = req->src;
		T_Data_Individual__req(resp);
	} else {
		T_Data_Connected__req(resp);
	}
}

static void send_group_response(uint16_t tsap, knx_priority_t priority, const uint8_t *apdu,
				uint8_t apdu_len)
{
	struct knx_pkt *resp = knx_pkt_alloc(K_NO_WAIT);

	if (!resp) {
		LOG_ERR("alloc failed");
		return;
	}
	memcpy(resp->buf, apdu, apdu_len);
	resp->lsdu = resp->buf;
	resp->lsdu_len = apdu_len;
	resp->tsap = tsap;
	resp->priority = priority;
	resp->hop_count = device_routing_count();
	resp->ack_request = 0;
	T_Data_Group__req(resp);
}

static void send_broadcast_response(knx_priority_t priority, const uint8_t *apdu, uint8_t apdu_len)
{
	struct knx_pkt *resp = knx_pkt_alloc(K_NO_WAIT);

	if (!resp) {
		LOG_ERR("alloc failed");
		return;
	}
	memcpy(resp->buf, apdu, apdu_len);
	resp->lsdu = resp->buf;
	resp->buf_len = apdu_len;
	resp->lsdu_len = apdu_len;
	resp->priority = priority;
	resp->hop_count = device_routing_count();
	resp->ack_request = 0;
	T_Data_Broadcast__req(resp);
}

/* ============================================================================
 * A_GroupValue_Read Service
 * ============================================================================
 */

void A_GroupValue_Read__req(struct knx_pkt *pkt)
{
	uint8_t apdu[2];

	apdu[0] = (A_GroupValue_Read >> 8) & 0xFF;
	apdu[1] = A_GroupValue_Read & 0xFF;

	memcpy(pkt->buf, apdu, 2);
	pkt->lsdu = pkt->buf;
	pkt->lsdu_len = 2;
	T_Data_Group__req(pkt);
}

void A_GroupValue_Read__Lcon(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void A_GroupValue_Read__Rcon(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}

/*
 * Returns true when this ASAP generated the response, so that
 * T_Data_Group__ind() can stop walking the association table — 03_03_07
 * §3.1.1 (p.12): the Application Layer informs every associated ASAP, but
 * only one of them generates the A_GroupValue_Response.  Answering from each
 * associated ASAP put N identical responses on the bus for one read.
 *
 * Note the reference (Thelsing) stack does NOT do this — it responds from
 * every ASAP whose R and C flags are set.  Its passing ETS is therefore no
 * evidence either way here; the spec text is what governs.
 */
bool A_GroupValue_Read__ind(struct knx_pkt *pkt)
{
	uint16_t asap = pkt->asap;
	uint8_t flag = group_object_flag(asap);
	uint8_t *data = group_object_data(asap);
	uint8_t size = group_object_size(asap);
	bool is_short = group_object_is_short_form(asap);
	uint8_t apdu[2 + 14];
	uint8_t apdu_len;

	LOG_DBG("ind asap=%u short=%d", asap, is_short);

	if (data == NULL) {
		LOG_WRN("ind: no object for ASAP %u", asap);
		return false;
	}
	if (!((flag & GROUP_OBJECT_FLAG_C) && (flag & GROUP_OBJECT_FLAG_R))) {
		/* Not a candidate responder — the caller keeps looking. */
		LOG_DBG("ind: R/C flag not set for ASAP %u", asap);
		return false;
	}

	/* Give the application a chance to refresh the cached value before we
	 * answer from it (e.g. take a fresh sensor reading). Optional — with
	 * no handler registered this is a no-op and we answer straight from
	 * cache, same as before.
	 */
	group_object_read_requested(asap);

	if (is_short) {
		/* Short form: pack ≤6-bit value into APCI_lo bits[5:0]. */
		apdu[0] = (A_GroupValue_Response >> 8) & 0xFFu;
		apdu[1] = (A_GroupValue_Response & 0xFFu) | (data[0] & 0x3Fu);
		apdu_len = 2;
	} else {
		apdu[0] = (A_GroupValue_Response >> 8) & 0xFFu;
		apdu[1] = A_GroupValue_Response & 0xFFu;
		memcpy(&apdu[2], data, size);
		apdu_len = 2 + size;
	}

	send_group_response(pkt->tsap, pkt->priority, apdu, apdu_len);
	return true;
}

void A_GroupValue_Read__res(struct knx_pkt *pkt)
{
	T_Data_Group__req(pkt);
}

void A_GroupValue_Read__Acon(struct knx_pkt *pkt)
{
	uint16_t asap = pkt->asap;
	uint8_t flag = group_object_flag(asap);
	uint8_t *go_value = group_object_data(asap);
	bool is_short = group_object_is_short_form(asap);

	LOG_DBG("Acon asap=%u short=%d", asap, is_short);

	if (go_value == NULL) {
		return;
	}
	if (!((flag & GROUP_OBJECT_FLAG_C) && (flag & GROUP_OBJECT_FLAG_U))) {
		return;
	}

	if (is_short) {
		/* Short form: value is in lsdu[1] bits[5:0]. */
		go_value[0] = pkt->lsdu[1] & 0x3Fu;
	} else {
		/* Long form: data starts at lsdu[2]. */
		uint8_t size = group_object_size(asap);

		memcpy(go_value, &pkt->lsdu[2], size);
	}
	group_object_updated(asap);
}

/* ============================================================================
 * A_GroupValue_Write Service
 * ============================================================================
 */

void A_GroupValue_Write__req(struct knx_pkt *pkt)
{
	T_Data_Group__req(pkt);
}

void A_GroupValue_Write__Lcon(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void A_GroupValue_Write__Rcon(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}

void A_GroupValue_Write__ind(struct knx_pkt *pkt)
{
	uint16_t asap = pkt->asap;
	uint8_t flag = group_object_flag(asap);
	uint8_t *go_value = group_object_data(asap);
	uint8_t size = group_object_size(asap);
	bool is_short = group_object_is_short_form(asap);

	LOG_DBG("ind asap=%u size=%u short=%d lsdu_len=%u", asap, size, is_short, pkt->lsdu_len);

	if (go_value == NULL) {
		LOG_WRN("ind: no object for ASAP %u", asap);
		return;
	}
	if (!((flag & GROUP_OBJECT_FLAG_C) && (flag & GROUP_OBJECT_FLAG_W))) {
		LOG_WRN("ind: W/C flag not set for ASAP %u", asap);
		return;
	}

	if (is_short) {
		/* Short form: ≤6-bit data packed into APCI_lo bits[5:0].
		 * lsdu = {APCI_hi, APCI_lo|data}; lsdu_len must be 2.
		 */
		if (pkt->lsdu_len != 2) {
			LOG_WRN("ind: short-form ASAP %u but lsdu_len=%u", asap, pkt->lsdu_len);
			return;
		}
		go_value[0] = pkt->lsdu[1] & 0x3Fu;
	} else {
		/* Long form: data starts at lsdu[2]. */
		uint8_t data_len = (pkt->lsdu_len > 2) ? (pkt->lsdu_len - 2) : 0;

		if (data_len != size) {
			LOG_WRN("ind: size mismatch ASAP %u: %u vs %u", asap, data_len, size);
			return;
		}
		memcpy(go_value, &pkt->lsdu[2], size);
	}
	group_object_updated(asap);
}

/* ============================================================================
 * A_IndividualAddress_Write / Read Services
 * ============================================================================
 */

void A_IndividualAddress_Write__req(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void A_IndividualAddress_Write__Lcon(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}

void A_IndividualAddress_Write__ind(struct knx_pkt *pkt)
{
	/* lsdu[0]=APCI_hi, lsdu[1]=APCI_lo, lsdu[2]=addr_hi, lsdu[3]=addr_lo */
	knx_addr_t new_addr = ((knx_addr_t)pkt->lsdu[2] << 8) | pkt->lsdu[3];

	LOG_DBG("ind new_addr=" KNX_ADDR_FMT, KNX_ADDR_VAL(new_addr));
	device_individual_address_set(new_addr);
	/* Debounced write — avoids blocking knx_app_wq for a flash erase cycle. */
	memory_modified();
}

void A_IndividualAddress_Read__req(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void A_IndividualAddress_Read__Lcon(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void A_IndividualAddress_Read__Rcon(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void A_IndividualAddress_Read__Acon(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}

void A_IndividualAddress_Read__ind(struct knx_pkt *pkt)
{
	/*
	 * Pseudo-random delay 0–199 ms (T_media=20 ms, up to 10 intervals).
	 * Uses kernel uptime: no entropy hardware required, and the arrival
	 * time of the telegram varies enough between devices and requests to
	 * avoid systematic collisions during programming-mode scanning.
	 */
	uint32_t delay_ms = k_uptime_get_32() % 200u;

	LOG_DBG("ind — responding after %u ms", delay_ms);

	s_ia_read_resp_priority = pkt->priority;
	k_work_init_delayable(&s_ia_read_resp_work, ia_read_resp_work_fn);
	k_work_reschedule_for_queue(&knx_app_wq, &s_ia_read_resp_work, K_MSEC(delay_ms));
}

void A_IndividualAddress_Read__res(struct knx_pkt *pkt)
{
	T_Data_Broadcast__req(pkt);
}

/* ============================================================================
 * A_IndividualAddressSerialNumber Services (stubs)
 * ============================================================================
 */

void A_IndividualAddressSerialNumber_Read__req(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void A_IndividualAddressSerialNumber_Read__Lcon(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void A_IndividualAddressSerialNumber_Read__ind(struct knx_pkt *pkt)
{
	/*
	 * Respond with our serial number if it matches the requested SN in the PDU,
	 * or if the PDU has no SN filter (len == 2, just APCI).
	 * Response: A_IndividualAddressSerialNumber_Response(serial, IA)
	 */
	bool respond = false;

	if (pkt->lsdu_len <= 2) {
		respond = true; /* no serial number filter */
	} else if (pkt->lsdu_len >= 8) {
		/* Compare 6-byte SN in lsdu[2..7] with our serial */
		bool match = true;

		for (int i = 0; i < 6 && match; i++) {
			if (pkt->lsdu[2 + i] != device_serial_number(i)) {
				match = false;
			}
		}
		respond = match;
	}

	if (!respond) {
		return;
	}

	uint16_t own_ia = device_individual_address();
	uint8_t apdu[10];

	apdu[0] = (A_IndividualAddressSerialNumber_Response >> 8) & 0xFFu;
	apdu[1] = A_IndividualAddressSerialNumber_Response & 0xFFu;
	for (int i = 0; i < 6; i++) {
		apdu[2 + i] = device_serial_number(i);
	}
	apdu[8] = (own_ia >> 8) & 0xFFu;
	apdu[9] = own_ia & 0xFFu;

	send_broadcast_response(pkt->priority, apdu, sizeof(apdu));
}
void A_IndividualAddressSerialNumber_Read__res(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void A_IndividualAddressSerialNumber_Read__Rcon(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void A_IndividualAddressSerialNumber_Read__Acon(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}

void A_IndividualAddressSerialNumber_Write__req(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void A_IndividualAddressSerialNumber_Write__Lcon(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void A_IndividualAddressSerialNumber_Write__ind(struct knx_pkt *pkt)
{
	bool respond = false;

	if (pkt->lsdu_len <= 2) {
		respond = true; /* no serial number filter */
	} else if (pkt->lsdu_len >= 8) {
		/* Compare 6-byte SN in lsdu[2..7] with our serial */
		bool match = true;

		for (int i = 0; i < 6 && match; i++) {
			if (pkt->lsdu[2 + i] != device_serial_number(i)) {
				match = false;
			}
		}
		respond = match;
	}

	if (!respond) {
		return;
	}

	/* lsdu[0]=APCI_hi, lsdu[1]=APCI_lo, lsdu[2]=addr_hi, lsdu[3]=addr_lo */
	knx_addr_t new_addr = ((knx_addr_t)pkt->lsdu[8] << 8) | pkt->lsdu[9];

	LOG_DBG("ind new_addr=" KNX_ADDR_FMT, KNX_ADDR_VAL(new_addr));
	device_individual_address_set(new_addr);
	/* Debounced write — avoids blocking knx_app_wq for a flash erase cycle. */
	memory_modified();
}

/* ============================================================================
 * A_DeviceDescriptor_Read Service
 * ============================================================================
 */

void A_DeviceDescriptor_Read__ind(struct knx_pkt *pkt)
{
	/* lsdu[0]=TPCI, lsdu[1]=APCI_lo (descriptor_type in bits[5:0]) */
	uint8_t descriptor_type = pkt->lsdu[1] & 0x3F;
	uint8_t apdu[16];
	uint8_t data_len;

	LOG_DBG("ind descriptor_type=%u asap=%u", descriptor_type, pkt->asap);

	apdu[0] = (A_DeviceDescriptor_Response >> 8) & 0xFF;
	apdu[1] = (A_DeviceDescriptor_Response & 0xFF) | (descriptor_type & 0x3F);

	switch (descriptor_type) {
	case 0: {
		uint16_t descriptor = device_descriptor();

		apdu[2] = (descriptor >> 8) & 0xFF;
		apdu[3] = descriptor & 0xFF;
		data_len = 4;
		break;
	}
	case 2: {
		uint16_t descriptor = device_descriptor();
		uint16_t manufacturer_id = device_manufacturer_id();
		uint16_t version = device_version();

		apdu[2] = (descriptor >> 8) & 0xFF;
		apdu[3] = descriptor & 0xFF;
		apdu[4] = (manufacturer_id >> 8) & 0xFF;
		apdu[5] = manufacturer_id & 0xFF;
		apdu[6] = device_serial_number(2);
		apdu[7] = device_serial_number(3);
		apdu[8] = device_serial_number(4);
		apdu[9] = device_serial_number(5);
		apdu[10] = device_hardware_type(0);
		apdu[11] = device_hardware_type(1);
		apdu[12] = device_hardware_type(2);
		apdu[13] = device_hardware_type(3);
		apdu[14] = (version >> 8) & 0xFF;
		apdu[15] = version & 0xFF;
		data_len = 16;
		break;
	}
	default:
		apdu[1] = (A_DeviceDescriptor_Response & 0xFF) | 0x3F;
		data_len = 2;
		break;
	}

	send_response(pkt, apdu, data_len);
}

void A_DeviceDescriptor_Read__res(struct knx_pkt *pkt)
{
	T_Data_Connected__req(pkt);
}

/* ============================================================================
 * A_Restart Service
 * ============================================================================
 */

void A_Restart__ind(struct knx_pkt *pkt)
{
	/* lsdu[0]=TPCI, lsdu[1]=APCI_lo (restart_type in bit[0]),
	 * lsdu[2]=Erase_Code, lsdu[3]=Channel_Number (Master Reset only).
	 */
	uint8_t restart_type = pkt->lsdu[1] & 0x01u;
	uint8_t erase_code = (pkt->lsdu_len > 2) ? pkt->lsdu[2] : 0;
	uint8_t channel = (pkt->lsdu_len > 3) ? pkt->lsdu[3] : 0;

	LOG_DBG("ind restart_type=%u erase_code=%u channel=%u", restart_type, erase_code, channel);

	if (restart_type == 0) {
		/* Basic restart: no response (KNX spec §3.7.4).
		 * PID_PROGMODE must be cleared to 0 on every reset (spec §15.7).
		 */
		device_prog_mode_set(false);
		memory_write();
		sys_reboot(SYS_REBOOT_COLD);
		return;
	}

	/*
	 * MasterReset: apply the erase operation, send A_Restart_Response,
	 * then schedule reboot after 50 ms so the response can leave the bus.
	 *
	 * Erase codes per KNX spec 3/3/7 §3.7.4 and Resources §15.6:
	 *   0x01 Confirmed Restart     — no data cleared, just reboot
	 *   0x02 Factory Reset         — IA + links + app + params + group objects
	 *   0x03 ResetIA               — IA only → 0xFFFF
	 *   0x04 ResetAP               — application program segment only
	 *   0x05 ResetParam            — application parameters only
	 *   0x06 ResetLinks            — address table + association table only
	 *   0x07 Factory Reset w/o IA  — same as 0x02 but IA preserved
	 *   0x08 Erase persistent data — device-specific persistent app data
	 */
	bool valid = false;

	switch (erase_code) {
	case 0x01: /* Confirmed Restart — no data change */
		valid = true;
		break;

	case 0x02: /* Factory Reset — IA + links + app + params + group objects */
		device_individual_address_set(0xFFFF);
		memset(__memory.addresses, 0, sizeof(__memory.addresses));
		memset(__memory.associations, 0, sizeof(__memory.associations));
		memset(__memory.application_program, 0, sizeof(__memory.application_program));
		memset(__memory.group_object, 0, sizeof(__memory.group_object));
		group_object_data_reset(); /* RAM-only, not part of __memory */
		memset(__memory.program_version_2, 0, sizeof(__memory.program_version_2));
		memset(__memory.interface_program, 0, sizeof(__memory.interface_program));
		interface_unload_object(OBJ_IDX_ADDRESS_TABLE);
		interface_unload_object(OBJ_IDX_ASSOCIATION_TABLE);
		interface_unload_object(OBJ_IDX_GROUP_OBJ_TABLE);
		interface_unload_object(OBJ_IDX_APPLICATION_PROG);
		interface_unload_object(OBJ_IDX_INTERFACE_PROG);
		valid = true;
		break;

	case 0x03: /* ResetIA — individual address only */
		device_individual_address_set(0xFFFF);
		valid = true;
		break;

	case 0x04: /* ResetAP — application program segment only */
		memset(__memory.application_program, 0, sizeof(__memory.application_program));
		interface_unload_object(OBJ_IDX_APPLICATION_PROG);
		valid = true;
		break;

	case 0x05: /* ResetParam — application parameters (embedded in app program) */
		memset(__memory.application_program, 0, sizeof(__memory.application_program));
		interface_unload_object(OBJ_IDX_APPLICATION_PROG);
		valid = true;
		break;

	case 0x06: /* ResetLinks — address table + association table */
		memset(__memory.addresses, 0, sizeof(__memory.addresses));
		memset(__memory.associations, 0, sizeof(__memory.associations));
		interface_unload_object(OBJ_IDX_ADDRESS_TABLE);
		interface_unload_object(OBJ_IDX_ASSOCIATION_TABLE);
		valid = true;
		break;

	case 0x07: /* Factory Reset without IA — same as 0x02 but IA preserved */
		memset(__memory.addresses, 0, sizeof(__memory.addresses));
		memset(__memory.associations, 0, sizeof(__memory.associations));
		memset(__memory.application_program, 0, sizeof(__memory.application_program));
		memset(__memory.group_object, 0, sizeof(__memory.group_object));
		group_object_data_reset(); /* RAM-only, not part of __memory */
		memset(__memory.program_version_2, 0, sizeof(__memory.program_version_2));
		memset(__memory.interface_program, 0, sizeof(__memory.interface_program));
		interface_unload_object(OBJ_IDX_ADDRESS_TABLE);
		interface_unload_object(OBJ_IDX_ASSOCIATION_TABLE);
		interface_unload_object(OBJ_IDX_GROUP_OBJ_TABLE);
		interface_unload_object(OBJ_IDX_APPLICATION_PROG);
		interface_unload_object(OBJ_IDX_INTERFACE_PROG);
		valid = true;
		break;

	case 0x08:                         /* Erase persistent app data — device-specific */
		group_object_data_reset(); /* RAM-only, not part of __memory */
		valid = true;
		break;

	default:
		LOG_WRN("ind: invalid erase_code=0x%02x", erase_code);
		break;
	}

	if (!valid) {
		return;
	}

	/*
	 * KNX spec 3/3/7 §3.7.4: Master Reset must respond before rebooting.
	 * Response PDU: [APCI_hi] [APCI_lo|restart_type|response] [error_code]
	 *               [process_time_hi] [process_time_lo]
	 * process_time unit: 100 ms → 0x01 = 100 ms, which covers our 50 ms delay.
	 */
	uint8_t apdu[5];

	apdu[0] = (uint8_t)((A_Restart_Response >> 8) & 0xFFu);
	apdu[1] = (uint8_t)((A_Restart_Response & 0xFFu) | 0x01u); /* restart_type=1 */
	apdu[2] = 0x00u;                                           /* error_code = E_SUCCESS */
	apdu[3] = 0x00u;                                           /* process_time high */
	apdu[4] = 0x01u;                                           /* process_time = 1 × 100 ms */
	send_response(pkt, apdu, sizeof(apdu));

	ARG_UNUSED(channel);

	k_work_init_delayable(&s_reboot_work, reboot_work_fn);
	k_work_schedule(&s_reboot_work, K_MSEC(50));
}

/* ============================================================================
 * A_Authorize Service (KNX spec 3/3/7 §3.9)
 * ============================================================================
 */

void A_Authorize_Request__ind(struct knx_pkt *pkt)
{
	/*
	 * lsdu[0]=TPCI, lsdu[1]=APCI_lo(=0xD1 base), lsdu[2]=reserved(0),
	 * lsdu[3..6]=key (4 bytes).
	 *
	 * Response: A_Authorize_Response with access level (0=full, 255=denied).
	 * CONFIG_KNX_AUTHORIZE_GRANT_ALL=y: always respond with level 0.
	 * When n security layer is implemented, verify the key against the key table.
	 */
	uint8_t level = IS_ENABLED(CONFIG_KNX_AUTHORIZE_GRANT_ALL) ? 0u : 0xFFu;
	uint8_t apdu[3];

	LOG_DBG("ind (grant_all=%d)", IS_ENABLED(CONFIG_KNX_AUTHORIZE_GRANT_ALL));

	/*
	 * Record the granted level so interface_write_property() can enforce the
	 * per-property write levels.  Without this the level was answered on the
	 * bus and then forgotten, which is why the access checks had nothing to
	 * compare against.  The level reverts when the connection closes.
	 */
	if (level != 0xFFu) {
		interface_set_access_level(level);
	}

	apdu[0] = (A_Authorize_Response >> 8) & 0xFFu;
	apdu[1] = A_Authorize_Response & 0xFFu;
	apdu[2] = level;

	send_response(pkt, apdu, sizeof(apdu));
}

/* ============================================================================
 * A_Key_Write Service (KNX spec 3/3/7 §3.5.8) — mandatory, System B §13.2
 * ============================================================================
 */

void A_Key_Write__ind(struct knx_pkt *pkt)
{
	/*
	 * lsdu[0]=TPCI, lsdu[1]=APCI_lo, lsdu[2]=level, lsdu[3..6]=key (4 bytes).
	 * key==0xFFFFFFFF deletes the key entry for that level.
	 *
	 * This device has no Access Policy / key table (A_Authorize_Request
	 * always fail-opens to level 0 — KNX spec fallback rule: an
	 * unprotected device must grant level 0 for any key, including an
	 * illegal one). Since the writer's "current access level" is
	 * therefore always 0 (full access), the spec's "current level must
	 * be <= requested level" check trivially always passes: accept and
	 * echo the level back, without persisting a key nothing checks.
	 */
	uint8_t level = pkt->lsdu[2];
	uint8_t apdu[3];

	LOG_DBG("ind level=%u", level);

	apdu[0] = (A_Key_Response >> 8) & 0xFFu;
	apdu[1] = A_Key_Response & 0xFFu;
	apdu[2] = level;

	send_response(pkt, apdu, sizeof(apdu));
}

/* ============================================================================
 * A_ADC_Read Service (KNX spec 3/3/7 §3.5.2) — mandatory, System B §13.2
 * (channels 1 = bus voltage, 4 = PEI type)
 * ============================================================================
 */

void A_ADC_Read__ind(struct knx_pkt *pkt)
{
	/* lsdu[0]=TPCI, lsdu[1]=APCI_lo, lsdu[2]=channel_nr, lsdu[3]=read_count. */
	uint8_t channel_nr = pkt->lsdu[2];
	uint8_t read_count = pkt->lsdu[3];
	uint8_t apdu[6];

	LOG_DBG("ind channel_nr=%u read_count=%u", channel_nr, read_count);

	/*
	 * No ADC sampling path exists yet from the NCN5130 driver up to this
	 * layer (bus-voltage/PEI-type readback would need new driver work,
	 * not a protocol fix) — report the standard-defined "problem" result
	 * (read_count=0, sum=0) rather than fabricate a reading, so ETS sees
	 * a well-formed response instead of a silent timeout.
	 */
	apdu[0] = (A_ADC_Response >> 8) & 0xFFu;
	apdu[1] = A_ADC_Response & 0xFFu;
	apdu[2] = channel_nr;
	apdu[3] = 0u; /* read_count = 0: value not available */
	apdu[4] = 0u; /* sum high */
	apdu[5] = 0u; /* sum low */

	send_response(pkt, apdu, sizeof(apdu));
}

/* ============================================================================
 * A_PropertyValue_Read Service
 * ============================================================================
 */

void A_PropertyValue_Read__ind(struct knx_pkt *pkt)
{
	/*
	 * A_PropertyValue_Read/Write PDUs are at least 6 octets (TPCI, APCI_lo,
	 * object index, PID, count|start_hi, start_lo).  A shorter one would make
	 * lsdu[4]/lsdu[5] stale packet-buffer bytes, so the object index, PID and
	 * start index would all come from the previous frame.
	 */
	if (pkt->lsdu_len < 6) {
		LOG_WRN("%s: PDU too short (%u)", __func__, pkt->lsdu_len);
		return;
	}

	/*
	 * lsdu[0]=TPCI, lsdu[1]=APCI_lo,
	 * lsdu[2]=object_index, lsdu[3]=property_id,
	 * lsdu[4]=(count[3:0]<<4)|(start[11:8]), lsdu[5]=start[7:0]
	 */
	uint8_t object_index = pkt->lsdu[2];
	uint8_t property_id = pkt->lsdu[3];
	uint8_t number_of_elements = (pkt->lsdu[4] >> 4) & 0x0F;
	uint16_t start_index = (((uint16_t)(pkt->lsdu[4] & 0x0F)) << 8) | pkt->lsdu[5];
	uint8_t property_data[MAX_PROPERTY_READ_SIZE];
	uint32_t actual_count = number_of_elements;
	uint8_t data_length;
	uint8_t apdu[6 + MAX_PROPERTY_READ_SIZE];

	LOG_DBG("ind obj=%u prop=%u count=%u start=%u", object_index, property_id,
		number_of_elements, start_index);

	data_length = interface_read_property(object_index, property_id, start_index, &actual_count,
					      property_data, MAX_PROPERTY_READ_SIZE);
	if (data_length == 0) {
		actual_count = 0;
	}

	apdu[0] = (A_PropertyValue_Response >> 8) & 0xFF;
	apdu[1] = A_PropertyValue_Response & 0xFF;
	apdu[2] = object_index;
	apdu[3] = property_id;
	apdu[4] = ((actual_count & 0x0F) << 4) | ((start_index >> 8) & 0x0F);
	apdu[5] = start_index & 0xFF;
	if (data_length > 0) {
		memcpy(&apdu[6], property_data, data_length);
	}

	send_response(pkt, apdu, 6 + data_length);
}

void A_PropertyValue_Read__res(struct knx_pkt *pkt)
{
	T_Data_Connected__req(pkt);
}

/* ============================================================================
 * A_PropertyValue_Write Service
 * ============================================================================
 */

void A_PropertyValue_Write__ind(struct knx_pkt *pkt)
{
	/*
	 * A_PropertyValue_Read/Write PDUs are at least 6 octets (TPCI, APCI_lo,
	 * object index, PID, count|start_hi, start_lo).  A shorter one would make
	 * lsdu[4]/lsdu[5] stale packet-buffer bytes, so the object index, PID and
	 * start index would all come from the previous frame.
	 */
	if (pkt->lsdu_len < 6) {
		LOG_WRN("%s: PDU too short (%u)", __func__, pkt->lsdu_len);
		return;
	}

	/*
	 * lsdu[0]=TPCI, lsdu[1]=APCI_lo,
	 * lsdu[2]=object_index, lsdu[3]=property_id,
	 * lsdu[4]=(count<<4)|(start_hi), lsdu[5]=start_lo,
	 * lsdu[6..]=data
	 */
	uint8_t object_index = pkt->lsdu[2];
	uint8_t property_id = pkt->lsdu[3];
	uint8_t number_of_elements = (pkt->lsdu[4] >> 4) & 0x0F;
	uint16_t start_index = (((uint16_t)(pkt->lsdu[4] & 0x0F)) << 8) | pkt->lsdu[5];
	uint8_t data_length = (pkt->lsdu_len > 6) ? (pkt->lsdu_len - 6) : 0;
	bool ok;
	uint8_t property_data[MAX_PROPERTY_READ_SIZE];
	uint32_t actual_count;
	uint8_t resp_len;
	uint8_t apdu[6 + MAX_PROPERTY_READ_SIZE];

	LOG_DBG("ind obj=%u prop=%u count=%u start=%u data_len=%u", object_index, property_id,
		number_of_elements, start_index, data_length);

	/*
	 * number_of_elements (elements) and data_length (octets) are distinct
	 * arguments.  Passing data_length as the element count
	 * used to work only because every writeable property today has
	 * max_nr_of_elem == 1 or a 1-octet datatype.
	 */
	ok = interface_write_property(object_index, property_id, start_index, number_of_elements,
				      &pkt->lsdu[6], data_length);

	/*
	 * KNX spec 3/4/1 §3.1.4: send a readback response ONLY when Verify Mode
	 * is active (PID_DEVICE_CONTROL bit 2).  When inactive, the TL-level ACK
	 * is sufficient confirmation.  On write failure always respond with count=0
	 * to inform the tool that the write was rejected.
	 *
	 * PID_LOAD_STATE_CONTROL is a special case (KNX spec 3/5/1 §15.5, DM_LoadSM):
	 * ETS relies on the readback of the resulting LoadState to detect a
	 * transition into LS_ERROR during download, so it must always be
	 * answered regardless of Verify Mode — confirmed against the reference
	 * (Thelsing) stack, which responds to every Load State write with a
	 * 1-byte state readback even with Verify Mode inactive.
	 */
	bool verify_mode =
		(__memory.device_control & 0x04u) != 0 || property_id == PID_LOAD_STATE_CONTROL;

	if (!ok) {
		/* Write failed — respond with count=0 to signal rejection. */
		LOG_WRN("ind: write failed obj=%u prop=%u", object_index, property_id);
		apdu[0] = (A_PropertyValue_Response >> 8) & 0xFFu;
		apdu[1] = A_PropertyValue_Response & 0xFFu;
		apdu[2] = object_index;
		apdu[3] = property_id;
		apdu[4] = (uint8_t)((start_index >> 8) & 0x0Fu); /* count=0 */
		apdu[5] = start_index & 0xFFu;
		send_response(pkt, apdu, 6);
	} else if (verify_mode) {
		/* Verify Mode active — read back the written value. */
		actual_count = number_of_elements;
		resp_len = interface_read_property(object_index, property_id, start_index,
						   &actual_count, property_data,
						   MAX_PROPERTY_READ_SIZE);
		if (resp_len == 0) {
			actual_count = 0;
		}
		apdu[0] = (A_PropertyValue_Response >> 8) & 0xFFu;
		apdu[1] = A_PropertyValue_Response & 0xFFu;
		apdu[2] = object_index;
		apdu[3] = property_id;
		apdu[4] = (uint8_t)(((actual_count & 0x0Fu) << 4) | ((start_index >> 8) & 0x0Fu));
		apdu[5] = start_index & 0xFFu;
		if (resp_len > 0) {
			memcpy(&apdu[6], property_data, resp_len);
		}
		send_response(pkt, apdu, 6 + resp_len);
	}
	/* else: write succeeded, verify mode off — TL ACK is sufficient. */
}

/* ============================================================================
 * A_PropertyDescription_Read Service
 * ============================================================================
 */

void A_PropertyDescription_Read__ind(struct knx_pkt *pkt)
{
	/*
	 * lsdu[0]=TPCI, lsdu[1]=APCI_lo,
	 * lsdu[2]=object_index, lsdu[3]=property_id, lsdu[4]=property_index
	 */
	uint8_t object_index = pkt->lsdu[2];
	uint8_t property_id = pkt->lsdu[3];
	uint8_t property_index = pkt->lsdu[4];
	bool write_enable = false;
	uint8_t type = 0;
	uint16_t max_nr_of_elem = 0;
	uint8_t access = 0;
	uint8_t prop_idx_found;
	uint8_t apdu[9];

	LOG_DBG("ind obj=%u prop=%u idx=%u", object_index, property_id, property_index);

	interface_read_property_description(object_index, property_id, property_index,
					    &write_enable, &type, &max_nr_of_elem, &access,
					    &prop_idx_found);

	apdu[0] = (A_PropertyDescription_Response >> 8) & 0xFF;
	apdu[1] = A_PropertyDescription_Response & 0xFF;
	apdu[2] = object_index;
	apdu[3] = property_id;
	apdu[4] = prop_idx_found;
	apdu[5] = type | ((write_enable ? 1u : 0u) << 7);
	apdu[6] = (max_nr_of_elem >> 8) & 0x0F;
	apdu[7] = max_nr_of_elem & 0xFF;
	apdu[8] = access;

	send_response(pkt, apdu, 9);
}

void A_PropertyDescription_Read__res(struct knx_pkt *pkt)
{
	T_Data_Connected__req(pkt);
}

/* ============================================================================
 * A_FunctionPropertyCommand / A_FunctionPropertyState_Read Services
 * ============================================================================
 */

static void dispatch_function_property(struct knx_pkt *pkt, bool is_state_read)
{
	/*
	 * lsdu[0]=TPCI, lsdu[1]=APCI_lo,
	 * lsdu[2]=object_index, lsdu[3]=property_id,
	 * lsdu[4..]=input data
	 */
	if (pkt->lsdu_len < 4) {
		LOG_WRN("FunctionProperty: PDU too short (%u)", pkt->lsdu_len);
		return;
	}

	uint8_t object_index = pkt->lsdu[2];
	uint8_t property_id = pkt->lsdu[3];
	uint8_t data_in_len = (pkt->lsdu_len > 4) ? (pkt->lsdu_len - 4) : 0;
	uint8_t data_out[32];
	uint8_t out_len;

	if (is_state_read) {
		out_len = interface_function_property_state_read(object_index, property_id,
								 &pkt->lsdu[4], data_in_len,
								 data_out, sizeof(data_out));
	} else {
		out_len = interface_function_property_command(object_index, property_id,
							      &pkt->lsdu[4], data_in_len, data_out,
							      sizeof(data_out));
	}

	uint8_t apdu[4 + 32];

	apdu[0] = (A_FunctionPropertyState_Response >> 8) & 0xFFu;
	apdu[1] = A_FunctionPropertyState_Response & 0xFFu;
	apdu[2] = object_index;
	apdu[3] = property_id;
	if (out_len > 0) {
		memcpy(&apdu[4], data_out, out_len);
	}
	send_response(pkt, apdu, 4 + out_len);
}

void A_FunctionPropertyCommand__ind(struct knx_pkt *pkt)
{
	dispatch_function_property(pkt, false);
}

void A_FunctionPropertyState_Read__ind(struct knx_pkt *pkt)
{
	dispatch_function_property(pkt, true);
}

/* ============================================================================
 * A_Memory_Read Service
 * ============================================================================
 */

void A_Memory_Read__req(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void A_Memory_Read__Lcon(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}

void A_Memory_Read__ind(struct knx_pkt *pkt)
{
	/*
	 * lsdu[0]=TPCI, lsdu[1]=APCI_lo (number in bits[5:0]),
	 * lsdu[2]=addr_hi, lsdu[3]=addr_lo
	 */
	uint8_t number = pkt->lsdu[1] & 0x3F;
	uint16_t memory_address = ((uint16_t)pkt->lsdu[2] << 8) | pkt->lsdu[3];
	uint8_t *memory_ptr = (uint8_t *)&__memory;
	uint32_t memory_size = sizeof(__memory);
	uint8_t apdu[4 + 63];

	LOG_DBG("ind num=%u addr=0x%04x", number, memory_address);

	apdu[0] = (A_Memory_Response >> 8) & 0xFF;

	if (number == 0 || number > 63 || (uint32_t)memory_address + number > memory_size) {
		LOG_WRN("ind: invalid num=%u or addr=0x%04x", number, memory_address);
		apdu[1] = (A_Memory_Response & 0xC0); /* number = 0 */
		apdu[2] = (memory_address >> 8) & 0xFF;
		apdu[3] = memory_address & 0xFF;
		send_response(pkt, apdu, 4);
		return;
	}

	apdu[1] = (A_Memory_Response & 0xC0) | (number & 0x3F);
	apdu[2] = (memory_address >> 8) & 0xFF;
	apdu[3] = memory_address & 0xFF;
	memcpy(&apdu[4], &memory_ptr[memory_address], number);

	send_response(pkt, apdu, 4 + number);
}

void A_Memory_Read__res(struct knx_pkt *pkt)
{
	T_Data_Connected__req(pkt);
}

/* ============================================================================
 * A_Memory_Write Service
 * ============================================================================
 */

void A_Memory_Write__req(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void A_Memory_Write__Lcon(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}

void A_Memory_Write__ind(struct knx_pkt *pkt)
{
	/*
	 * lsdu[0]=TPCI, lsdu[1]=APCI_lo (number in bits[5:0]),
	 * lsdu[2]=addr_hi, lsdu[3]=addr_lo, lsdu[4..]=data
	 */
	uint8_t number = pkt->lsdu[1] & 0x3F;
	uint16_t memory_address = ((uint16_t)pkt->lsdu[2] << 8) | pkt->lsdu[3];
	uint8_t *data = &pkt->lsdu[4];
	/* memory_address is a byte offset into __memory (not an MCU address). */
	uint32_t offset = memory_address;
	bool verify_mode;
	uint8_t apdu[4 + 63];

	LOG_DBG("ind num=%u addr=0x%04x", number, memory_address);

	if (number == 0 || number > 63) {
		LOG_WRN("ind: invalid number=%u", number);
		apdu[0] = (A_Memory_Response >> 8) & 0xFF;
		apdu[1] = (A_Memory_Response & 0xC0); /* number = 0 */
		apdu[2] = (memory_address >> 8) & 0xFF;
		apdu[3] = memory_address & 0xFF;
		send_response(pkt, apdu, 4);
		return;
	}

	if (!memory_user_write(offset, number, data)) {
		LOG_WRN("ind: write failed offset=0x%04x", memory_address);
		apdu[0] = (A_Memory_Response >> 8) & 0xFF;
		apdu[1] = (A_Memory_Response & 0xC0);
		apdu[2] = (memory_address >> 8) & 0xFF;
		apdu[3] = memory_address & 0xFF;
		send_response(pkt, apdu, 4);
		return;
	}

	verify_mode = (__memory.device_control & 0x04) != 0;
	if (verify_mode) {
		apdu[0] = (A_Memory_Response >> 8) & 0xFF;
		apdu[1] = (A_Memory_Response & 0xC0) | (number & 0x3F);
		apdu[2] = (memory_address >> 8) & 0xFF;
		apdu[3] = memory_address & 0xFF;
		memcpy(&apdu[4], ((uint8_t *)&__memory) + offset, number);
		send_response(pkt, apdu, 4 + number);
	}
}

void A_Memory_Write__res(struct knx_pkt *pkt)
{
	T_Data_Connected__req(pkt);
}

void A_Memory_Write__Acon(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}

/* ============================================================================
 * A_UserMemory_Write Service
 * ============================================================================
 */

void A_UserMemory_Write__ind(struct knx_pkt *pkt)
{
	/*
	 * lsdu[0]=TPCI, lsdu[1]=APCI_lo,
	 * lsdu[2]=(addr[19:16]<<4)|(number[3:0]),
	 * lsdu[3]=addr[15:8], lsdu[4]=addr[7:0], lsdu[5..]=data
	 */
	uint8_t number = pkt->lsdu[2] & 0x0F;
	uint32_t memory_address = (((uint32_t)(pkt->lsdu[2] & 0xF0)) << 12) |
				  ((uint32_t)pkt->lsdu[3] << 8) | pkt->lsdu[4];
	uint8_t *data = &pkt->lsdu[5];
	uint8_t apdu[5 + 15];
	bool verify_mode = (__memory.device_control & 0x04) != 0;

	LOG_DBG("ind num=%u addr=0x%05x", number, memory_address);

	apdu[0] = (A_UserMemory_Response >> 8) & 0x3F;
	apdu[1] = A_UserMemory_Response & 0xFF;

	/* §3.5.6.3: with inactive Verify Mode the remote process does not
	 * respond at the application layer at all — only the transport-layer
	 * ack. Mirrors A_Memory_Write__ind's verify_mode gate.
	 */
	if (memory_user_write(memory_address, number, data)) {
		if (!verify_mode) {
			return;
		}
		apdu[2] = ((memory_address >> 12) & 0xF0) | (number & 0x0F);
		apdu[3] = (memory_address >> 8) & 0xFF;
		apdu[4] = memory_address & 0xFF;
		if (number > 0) {
			memcpy(&apdu[5], data, number);
		}
		send_response(pkt, apdu, 5 + number);
	} else {
		if (!verify_mode) {
			return;
		}
		apdu[2] = ((memory_address >> 12) & 0xF0); /* number = 0 */
		apdu[3] = (memory_address >> 8) & 0xFF;
		apdu[4] = memory_address & 0xFF;
		send_response(pkt, apdu, 5);
	}
}

void A_UserMemory_Write__res(struct knx_pkt *pkt)
{
	T_Data_Connected__req(pkt);
}

/* ============================================================================
 * A_UserMemory_Read Service (mandatory for 07B0h, "DMA on
 * User Memory", Volume 6 §4.2 row 2 p.37)
 * ============================================================================
 */

void A_UserMemory_Read__ind(struct knx_pkt *pkt)
{
	/*
	 * Same request layout as A_UserMemory_Write__ind (both address the
	 * same 20-bit User Memory space, which is __memory itself — 03_05_03
	 * §4: "A_Memory_Write and A_UserMemory_Write address the same memory
	 * when the top 12 bits of PID_TABLE_REFERENCE are zero", which they
	 * always are here, every segment fitting inside __memory).
	 * lsdu[0]=TPCI, lsdu[1]=APCI_lo,
	 * lsdu[2]=(addr[19:16]<<4)|(number[3:0]),
	 * lsdu[3]=addr[15:8], lsdu[4]=addr[7:0]
	 */
	uint8_t number = pkt->lsdu[2] & 0x0F;
	uint32_t memory_address = (((uint32_t)(pkt->lsdu[2] & 0xF0)) << 12) |
				  ((uint32_t)pkt->lsdu[3] << 8) | pkt->lsdu[4];
	uint8_t *memory_ptr = (uint8_t *)&__memory;
	uint32_t memory_size = sizeof(__memory);
	uint8_t apdu[5 + 15];

	LOG_DBG("ind num=%u addr=0x%05x", number, memory_address);

	apdu[0] = (A_UserMemory_Response >> 8) & 0x3F;
	apdu[1] = A_UserMemory_Response & 0xFF;

	if (number == 0 || memory_address + number > memory_size) {
		LOG_WRN("ind: invalid num=%u or addr=0x%05x", number, memory_address);
		apdu[2] = ((memory_address >> 12) & 0xF0); /* number = 0, error per §4-5 */
		apdu[3] = (memory_address >> 8) & 0xFF;
		apdu[4] = memory_address & 0xFF;
		send_response(pkt, apdu, 5);
		return;
	}

	apdu[2] = ((memory_address >> 12) & 0xF0) | (number & 0x0F);
	apdu[3] = (memory_address >> 8) & 0xFF;
	apdu[4] = memory_address & 0xFF;
	memcpy(&apdu[5], &memory_ptr[memory_address], number);

	send_response(pkt, apdu, 5 + number);
}

void A_UserMemory_Read__res(struct knx_pkt *pkt)
{
	T_Data_Connected__req(pkt);
}

/* ============================================================================
 * A_UserMemoryBit_Write Service (mandatory for 07B0h,
 * 03_03_07 §3.5.6.4, Figures 82-83, p.122-123)
 * ============================================================================
 */

void A_UserMemoryBit_Write__ind(struct knx_pkt *pkt)
{
	/*
	 * Unlike A_UserMemory_Write/Read, this PDU has no address-extension
	 * nibble — memory_address is a plain 16-bit offset, and number gets
	 * its own full octet (Figure 83):
	 * lsdu[0]=TPCI, lsdu[1]=APCI (full octet), lsdu[2]=number,
	 * lsdu[3]=addr_hi, lsdu[4]=addr_lo,
	 * lsdu[5..5+number-1]=and_data, lsdu[5+number..5+2*number-1]=xor_data.
	 * result_bit(i) = (and_data_bit(i) AND block_bit(i)) XOR xor_data_bit(i).
	 */
	uint8_t number = pkt->lsdu[2];
	uint16_t memory_address = ((uint16_t)pkt->lsdu[3] << 8) | pkt->lsdu[4];
	uint8_t *and_data = &pkt->lsdu[5];
	uint8_t *xor_data = &pkt->lsdu[5 + number];
	uint8_t result[5];
	uint8_t apdu[5 + 5];
	bool verify_mode = (__memory.device_control & 0x04) != 0;
	uint8_t i;

	LOG_DBG("ind num=%u addr=0x%04x", number, memory_address);

	apdu[0] = (A_UserMemory_Response >> 8) & 0x3F;
	apdu[1] = A_UserMemory_Response & 0xFF;

	/* §3.5.6.4 Error handling: ignore an out-of-range write outright;
	 * only Verify Mode gets an (empty, number=0) response for it.
	 */
	if (number == 0 || number > 5 || (uint32_t)memory_address + number > sizeof(__memory)) {
		LOG_WRN("ind: invalid num=%u or addr=0x%04x", number, memory_address);
		if (verify_mode) {
			apdu[2] = 0; /* number = 0, error */
			apdu[3] = (memory_address >> 8) & 0xFF;
			apdu[4] = memory_address & 0xFF;
			send_response(pkt, apdu, 5);
		}
		return;
	}

	for (i = 0; i < number; i++) {
		uint8_t existing = ((uint8_t *)&__memory)[memory_address + i];

		result[i] = (existing & and_data[i]) ^ xor_data[i];
	}
	memory_user_write(memory_address, number, result);

	if (verify_mode) {
		apdu[2] = number & 0x0F;
		apdu[3] = (memory_address >> 8) & 0xFF;
		apdu[4] = memory_address & 0xFF;
		memcpy(&apdu[5], result, number);
		send_response(pkt, apdu, 5 + number);
	}
}

void A_UserMemoryBit_Write__res(struct knx_pkt *pkt)
{
	T_Data_Connected__req(pkt);
}

/* ============================================================================
 * A_UserManufacturerInfo_Read Service (03_03_07 §3.5.6.5, Figures 84-85,
 * p.125 — mandatory for 07B0h. NOTE 13 in the spec itself deprecates this
 * service for new implementations, since a single octet can't carry a KNX
 * manufacturer ID above 255; this device's id (0x00FA) still fits.)
 * ============================================================================
 */

void A_UserManufacturerInfo_Read__ind(struct knx_pkt *pkt)
{
	uint8_t apdu[5];

	apdu[0] = (A_UserManufacturerInfo_Response >> 8) & 0x3F;
	apdu[1] = A_UserManufacturerInfo_Response & 0xFF;
	apdu[2] = device_manufacturer_id() & 0xFF;
	apdu[3] = 0; /* manufacturer-specific — none defined */
	apdu[4] = 0; /* manufacturer-specific — none defined */

	send_response(pkt, apdu, 5);
}

void A_UserManufacturerInfo_Read__res(struct knx_pkt *pkt)
{
	T_Data_Connected__req(pkt);
}
