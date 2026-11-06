/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include "canopen_internal.h"

#define PDO_INDEX_TX     0x0800U
#define PDO_INDEX_MAP    0x0200U
#define PDO_INDEX_NUMBER 0x01FFU

#define PDO_COB_TPDO       0x180U
#define PDO_COB_RPDO       0x200U
#define PDO_COB_STEP       0x100U
#define PDO_PREDEFINED     4U
#define PDO_NO_RTR         CANOPEN_COB_ID_BIT30

#define PDO_SUB_COB_ID     1U
#define PDO_SUB_TYPE       2U
#define PDO_SUB_INHIBIT    3U
#define PDO_SUB_EVENT      5U
#define PDO_SUB_SYNC_START 6U

#define PDO_SYNC_MAX       240U
#define PDO_MAX_LEN        CAN_MAX_DLC
#define PDO_DUMMY_LAST     0x001BU
#define PDO_INHIBIT_US     100U

#define PDO_FLAG_PENDING    BIT(0)
#define PDO_FLAG_WAIT_START BIT(1)
#define PDO_FLAG_LENGTH     BIT(2)
#define PDO_FLAG_TIMEOUT    BIT(3)

static const uint8_t pdo_tpdo_highest = PDO_SUB_SYNC_START;
static const uint8_t pdo_rpdo_highest = PDO_SUB_EVENT;

static bool pdo_valid(const struct canopen_pdo *pdo)
{
	return (pdo->param.cob_id & CANOPEN_COB_ID_INVALID) == 0U;
}

static bool pdo_event_type(uint8_t type)
{
	return type >= CANOPEN_PDO_TYPE_EVENT;
}

static bool pdo_type_valid(uint32_t type)
{
	return (type <= PDO_SYNC_MAX) || ((type >= CANOPEN_PDO_TYPE_EVENT) && (type <= UINT8_MAX));
}

static bool pdo_is_dummy(uint16_t index, uint8_t sub)
{
	return (index <= PDO_DUMMY_LAST) && (sub == 0U) &&
	       (canopen_od_type_size((uint8_t)index) != 0U);
}

static uint32_t pdo_map_check(struct canopen_node *node, bool tx, uint32_t entry, size_t *len)
{
	uint16_t index = (uint16_t)(entry >> 16);
	uint8_t sub = (uint8_t)(entry >> 8);
	uint8_t bits = (uint8_t)entry;
	struct canopen_od_ref ref;
	uint32_t abort;

	if ((bits == 0U) || ((bits % 8U) != 0U)) {
		return CANOPEN_SDO_ABORT_NOT_MAPPABLE;
	}

	if (pdo_is_dummy(index, sub)) {
		if (tx || ((canopen_od_type_size((uint8_t)index) * 8U) != bits)) {
			return CANOPEN_SDO_ABORT_NOT_MAPPABLE;
		}
	} else {
		abort = canopen_od_find(node, index, sub, &ref);
		if (abort != CANOPEN_SDO_ABORT_NONE) {
			return abort;
		}

		if (((ref.attr & (tx ? CANOPEN_OD_ATTR_TPDO : CANOPEN_OD_ATTR_RPDO)) == 0U) ||
		    (ref.type == CANOPEN_OD_TYPE_VISIBLE_STRING) ||
		    ((canopen_od_wire_size(&ref) * 8U) != bits)) {
			return CANOPEN_SDO_ABORT_NOT_MAPPABLE;
		}
	}

	*len += bits / 8U;

	return CANOPEN_SDO_ABORT_NONE;
}

static uint32_t pdo_config_cob_id(struct canopen_node *node, const struct canopen_pdo_config *cfg,
				  uint16_t n, bool tx)
{
	if (((cfg->cob_id & CAN_STD_ID_MASK) != 0U) || (n >= PDO_PREDEFINED)) {
		return cfg->cob_id;
	}

	return cfg->cob_id | ((tx ? PDO_COB_TPDO : PDO_COB_RPDO) + (n * PDO_COB_STEP) +
			      node->cfg->node_id);
}

static void pdo_default(struct canopen_node *node, struct canopen_pdo *pdo,
			const struct canopen_pdo_config *cfg, uint16_t n, bool tx)
{
	const struct canopen_pdo_config none = {
		.cob_id = CANOPEN_COB_ID_INVALID,
	};
	uint32_t cob_id = pdo_config_cob_id(node, (cfg != NULL) ? cfg : &none, n, tx);

	pdo->param.transmission_type = CANOPEN_PDO_TYPE_EVENT;
	pdo->param.inhibit_time = 0U;
	pdo->param.event_timer = 0U;
	pdo->param.sync_start = 0U;
	pdo->param.map_count = 0U;
	pdo->len = 0U;
	pdo->flags = 0U;
	pdo->event_next = sys_timepoint_calc(K_FOREVER);
	memset(pdo->param.map, 0, sizeof(pdo->param.map));

	if (cfg != NULL) {
		pdo->param.transmission_type = cfg->transmission_type;
		pdo->param.inhibit_time = cfg->inhibit_time;
		pdo->param.event_timer = cfg->event_timer;
		pdo->param.sync_start = cfg->sync_start;
		pdo->param.map_count = cfg->map_count;
		memcpy(pdo->param.map, cfg->map, cfg->map_count * sizeof(cfg->map[0]));

		for (uint8_t i = 0U; i < cfg->map_count; i++) {
			pdo->len += (uint8_t)(cfg->map[i] & 0xFFU) / 8U;
		}
	}

	pdo->param.cob_id = tx ? (cob_id | PDO_NO_RTR) : cob_id;
}

static int pdo_config_check(struct canopen_node *node, const struct canopen_pdo_config *cfg,
			    uint16_t count, uint16_t max, bool tx)
{
	if (count > max) {
		return -EINVAL;
	}

	for (uint16_t n = 0U; n < count; n++) {
		const struct canopen_pdo_config *c = &cfg[n];
		uint32_t cob_id = pdo_config_cob_id(node, c, n, tx);
		size_t len = 0U;

		if ((c->map_count > CONFIG_CANOPEN_PDO_MAPPED_MAX) ||
		    !pdo_type_valid(c->transmission_type) ||
		    (((cob_id & CANOPEN_COB_ID_INVALID) == 0U) &&
		     canopen_cob_id_restricted(cob_id & CAN_STD_ID_MASK))) {
			return -EINVAL;
		}

		for (uint8_t i = 0U; i < c->map_count; i++) {
			if (pdo_map_check(node, tx, c->map[i], &len) != CANOPEN_SDO_ABORT_NONE) {
				return -EINVAL;
			}
		}

		if (len > PDO_MAX_LEN) {
			return -EINVAL;
		}
	}

	return 0;
}

int canopen_pdo_init(struct canopen_node *node)
{
	const struct canopen_node_config *cfg = node->cfg;

	for (uint16_t n = 0U; n < CONFIG_CANOPEN_RPDO_COUNT; n++) {
		node->rpdo[n].filter = -1;
	}

	if ((pdo_config_check(node, cfg->tpdo, cfg->tpdo_count, CONFIG_CANOPEN_TPDO_COUNT, true) !=
	     0) ||
	    (pdo_config_check(node, cfg->rpdo, cfg->rpdo_count, CONFIG_CANOPEN_RPDO_COUNT, false) !=
	     0)) {
		return -EINVAL;
	}

	return 0;
}

void canopen_pdo_reset(struct canopen_node *node)
{
	const struct canopen_node_config *cfg = node->cfg;

	for (uint16_t n = 0U; n < CONFIG_CANOPEN_TPDO_COUNT; n++) {
		pdo_default(node, &node->tpdo[n], (n < cfg->tpdo_count) ? &cfg->tpdo[n] : NULL, n,
			    true);
	}

	for (uint16_t n = 0U; n < CONFIG_CANOPEN_RPDO_COUNT; n++) {
		pdo_default(node, &node->rpdo[n], (n < cfg->rpdo_count) ? &cfg->rpdo[n] : NULL, n,
			    false);
	}
}

void canopen_pdo_filters(struct canopen_node *node)
{
	for (uint16_t n = 0U; n < CONFIG_CANOPEN_RPDO_COUNT; n++) {
		struct canopen_pdo *pdo = &node->rpdo[n];

		pdo->len = 0U;
		for (uint8_t i = 0U; i < pdo->param.map_count; i++) {
			pdo->len += (uint8_t)(pdo->param.map[i] & 0xFFU) / 8U;
		}

		if (canopen_filter_set(node, &pdo->filter, pdo->param.cob_id) != 0) {
			pdo->param.cob_id |= CANOPEN_COB_ID_INVALID;
		}
	}
}

static void rpdo_errors_update(struct canopen_node *node)
{
	uint8_t flags = 0U;

	for (uint16_t n = 0U; n < CONFIG_CANOPEN_RPDO_COUNT; n++) {
		flags |= node->rpdo[n].flags;
	}

	if ((flags & PDO_FLAG_LENGTH) == 0U) {
		(void)canopen_emcy_unset(node, CANOPEN_EMCY_PDO_LENGTH);
	}

	if ((flags & PDO_FLAG_TIMEOUT) == 0U) {
		(void)canopen_emcy_unset(node, CANOPEN_EMCY_RPDO_TIMEOUT);
	}
}

static void rpdo_stop(struct canopen_node *node, struct canopen_pdo *pdo)
{
	pdo->flags = 0U;
	pdo->event_next = sys_timepoint_calc(K_FOREVER);
	rpdo_errors_update(node);
}

static void tpdo_start(struct canopen_node *node, struct canopen_pdo *pdo)
{
	bool event = pdo_event_type(pdo->param.transmission_type);

	pdo->sync_count = 0U;
	pdo->flags = event ? PDO_FLAG_PENDING : 0U;
	pdo->inhibit_end = sys_timepoint_calc(K_NO_WAIT);
	pdo->event_next = sys_timepoint_calc((event && (pdo->param.event_timer != 0U))
						     ? K_MSEC(pdo->param.event_timer)
						     : K_FOREVER);

	if ((pdo->param.sync_start != 0U) && (node->sync.overflow != 0U)) {
		pdo->flags |= PDO_FLAG_WAIT_START;
	}
}

static void tpdo_restart(struct canopen_node *node, struct canopen_pdo *pdo)
{
	if (pdo_valid(pdo) && (node->nmt_state == CANOPEN_NMT_OPERATIONAL)) {
		tpdo_start(node, pdo);
	}
}

static void tpdo_send(struct canopen_node *node, struct canopen_pdo *pdo)
{
	struct can_frame frame = {
		.id = pdo->param.cob_id & CAN_STD_ID_MASK,
	};
	struct canopen_od_ref ref;
	size_t len;

	for (uint8_t i = 0U; i < pdo->param.map_count; i++) {
		uint32_t entry = pdo->param.map[i];

		if ((canopen_od_find(node, (uint16_t)(entry >> 16), (uint8_t)(entry >> 8), &ref) !=
		     CANOPEN_SDO_ABORT_NONE) ||
		    (canopen_od_read_wire(&ref, &frame.data[frame.dlc], PDO_MAX_LEN - frame.dlc,
					  &len) != CANOPEN_SDO_ABORT_NONE)) {
			return;
		}

		frame.dlc += (uint8_t)len;
	}

	(void)canopen_send(node, &frame);
	pdo->flags &= ~PDO_FLAG_PENDING;

	if (pdo_event_type(pdo->param.transmission_type)) {
		pdo->inhibit_end =
			sys_timepoint_calc(K_USEC(pdo->param.inhibit_time * PDO_INHIBIT_US));
		if (pdo->param.event_timer != 0U) {
			pdo->event_next = sys_timepoint_calc(K_MSEC(pdo->param.event_timer));
		}
	}
}

static void rpdo_apply(struct canopen_node *node, uint16_t n, const uint8_t *data)
{
	const struct canopen_pdo *pdo = &node->rpdo[n];
	struct canopen_od_ref ref;
	size_t pos = 0U;

	for (uint8_t i = 0U; i < pdo->param.map_count; i++) {
		uint32_t entry = pdo->param.map[i];
		uint16_t index = (uint16_t)(entry >> 16);
		uint8_t sub = (uint8_t)(entry >> 8);
		size_t len = (entry & 0xFFU) / 8U;

		if (!pdo_is_dummy(index, sub) &&
		    (canopen_od_find(node, index, sub, &ref) == CANOPEN_SDO_ABORT_NONE)) {
			(void)canopen_od_write_wire(node, &ref, &data[pos], len);
		}

		pos += len;
	}

	if (node->cfg->rpdo_cb != NULL) {
		node->cfg->rpdo_cb(node, n + 1U, node->cfg->user_data);
	}
}

uint32_t canopen_pdo_find(struct canopen_node *node, uint16_t index, uint8_t sub,
			  struct canopen_od_ref *ref)
{
	bool tx = (index & PDO_INDEX_TX) != 0U;
	uint16_t n = index & PDO_INDEX_NUMBER;
	struct canopen_pdo_param *par;

	if (n >= (tx ? CONFIG_CANOPEN_TPDO_COUNT : CONFIG_CANOPEN_RPDO_COUNT)) {
		return CANOPEN_SDO_ABORT_NO_OBJECT;
	}

	par = tx ? &node->tpdo[n].param : &node->rpdo[n].param;

	if ((index & PDO_INDEX_MAP) != 0U) {
		if (sub == 0U) {
			canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED8,
					    CANOPEN_OD_RW, &par->map_count, &par->map_count,
					    sizeof(par->map_count));
		} else if (sub <= CONFIG_CANOPEN_PDO_MAPPED_MAX) {
			canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED32,
					    CANOPEN_OD_RW, &par->map[sub - 1U], &par->map[sub - 1U],
					    sizeof(par->map[0]));
		} else {
			return CANOPEN_SDO_ABORT_NO_SUB_INDEX;
		}

		return CANOPEN_SDO_ABORT_NONE;
	}

	switch (sub) {
	case 0U:
		canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED8, CANOPEN_OD_CONST,
				    tx ? &pdo_tpdo_highest : &pdo_rpdo_highest, NULL,
				    sizeof(uint8_t));
		break;
	case PDO_SUB_COB_ID:
		canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED32, CANOPEN_OD_RW,
				    &par->cob_id, &par->cob_id, sizeof(par->cob_id));
		break;
	case PDO_SUB_TYPE:
		canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED8, CANOPEN_OD_RW,
				    &par->transmission_type, &par->transmission_type,
				    sizeof(par->transmission_type));
		break;
	case PDO_SUB_INHIBIT:
		canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED16, CANOPEN_OD_RW,
				    &par->inhibit_time, &par->inhibit_time,
				    sizeof(par->inhibit_time));
		break;
	case PDO_SUB_EVENT:
		canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED16, CANOPEN_OD_RW,
				    &par->event_timer, &par->event_timer, sizeof(par->event_timer));
		break;
	case PDO_SUB_SYNC_START:
		if (!tx) {
			return CANOPEN_SDO_ABORT_NO_SUB_INDEX;
		}
		canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED8, CANOPEN_OD_RW,
				    &par->sync_start, &par->sync_start, sizeof(par->sync_start));
		break;
	default:
		return CANOPEN_SDO_ABORT_NO_SUB_INDEX;
	}

	return CANOPEN_SDO_ABORT_NONE;
}

static uint32_t pdo_cob_id_write(struct canopen_node *node, struct canopen_pdo *pdo, bool tx,
				 uint32_t value)
{
	uint32_t id = value & CAN_STD_ID_MASK;
	bool was_valid = pdo_valid(pdo);

	if (((value & (CANOPEN_COB_ID_FRAME | CANOPEN_COB_ID_RESERVED)) != 0U) ||
	    (tx && ((value & PDO_NO_RTR) == 0U))) {
		return CANOPEN_SDO_ABORT_INVALID_VALUE;
	}

	if (((value & CANOPEN_COB_ID_INVALID) == 0U) &&
	    ((was_valid && (id != (pdo->param.cob_id & CAN_STD_ID_MASK))) ||
	     canopen_cob_id_restricted(id))) {
		return CANOPEN_SDO_ABORT_INVALID_VALUE;
	}

	pdo->param.cob_id = value;

	if (was_valid == pdo_valid(pdo)) {
		return CANOPEN_SDO_ABORT_NONE;
	}

	if (tx) {
		tpdo_restart(node, pdo);
	} else {
		rpdo_stop(node, pdo);
		if (canopen_filter_set(node, &pdo->filter, value) != 0) {
			pdo->param.cob_id |= CANOPEN_COB_ID_INVALID;
			return CANOPEN_SDO_ABORT_INTERNAL_INCOMPATIBLE;
		}
	}

	return CANOPEN_SDO_ABORT_NONE;
}

static uint32_t pdo_map_write(struct canopen_node *node, struct canopen_pdo *pdo, bool tx,
			      uint8_t sub, uint32_t value)
{
	size_t len = 0U;
	uint32_t abort;

	if (pdo_valid(pdo)) {
		return CANOPEN_SDO_ABORT_UNSUPPORTED_ACCESS;
	}

	if (sub != 0U) {
		if (pdo->param.map_count != 0U) {
			return CANOPEN_SDO_ABORT_UNSUPPORTED_ACCESS;
		}

		if (value != 0U) {
			abort = pdo_map_check(node, tx, value, &len);
			if (abort != CANOPEN_SDO_ABORT_NONE) {
				return abort;
			}
		}

		pdo->param.map[sub - 1U] = value;

		return CANOPEN_SDO_ABORT_NONE;
	}

	if (value > CONFIG_CANOPEN_PDO_MAPPED_MAX) {
		return CANOPEN_SDO_ABORT_MAP_LENGTH;
	}

	for (uint8_t i = 0U; i < value; i++) {
		abort = pdo_map_check(node, tx, pdo->param.map[i], &len);
		if (abort != CANOPEN_SDO_ABORT_NONE) {
			return abort;
		}
	}

	if (len > PDO_MAX_LEN) {
		return CANOPEN_SDO_ABORT_MAP_LENGTH;
	}

	pdo->param.map_count = (uint8_t)value;
	pdo->len = (uint8_t)len;

	return CANOPEN_SDO_ABORT_NONE;
}

uint32_t canopen_pdo_write(struct canopen_node *node, uint16_t index, uint8_t sub,
			   uint32_t value)
{
	bool tx = (index & PDO_INDEX_TX) != 0U;
	uint16_t n = index & PDO_INDEX_NUMBER;
	struct canopen_pdo *pdo = tx ? &node->tpdo[n] : &node->rpdo[n];

	if ((index & PDO_INDEX_MAP) != 0U) {
		return pdo_map_write(node, pdo, tx, sub, value);
	}

	switch (sub) {
	case PDO_SUB_COB_ID:
		return pdo_cob_id_write(node, pdo, tx, value);
	case PDO_SUB_TYPE:
		if (!pdo_type_valid(value)) {
			return CANOPEN_SDO_ABORT_INVALID_VALUE;
		}
		pdo->param.transmission_type = (uint8_t)value;
		if (tx) {
			tpdo_restart(node, pdo);
		}
		return CANOPEN_SDO_ABORT_NONE;
	case PDO_SUB_EVENT:
		pdo->param.event_timer = (uint16_t)value;
		if (tx) {
			pdo->event_next =
				sys_timepoint_calc((value != 0U) ? K_MSEC(value) : K_FOREVER);
		} else {
			rpdo_stop(node, pdo);
		}
		return CANOPEN_SDO_ABORT_NONE;
	default:
		if (pdo_valid(pdo)) {
			return CANOPEN_SDO_ABORT_INVALID_VALUE;
		}
		if (sub == PDO_SUB_INHIBIT) {
			pdo->param.inhibit_time = (uint16_t)value;
		} else {
			pdo->param.sync_start = (uint8_t)value;
		}
		return CANOPEN_SDO_ABORT_NONE;
	}
}

void canopen_pdo_rx(struct canopen_node *node, const struct can_frame *frame)
{
	for (uint16_t n = 0U; n < CONFIG_CANOPEN_RPDO_COUNT; n++) {
		struct canopen_pdo *pdo = &node->rpdo[n];

		if (!pdo_valid(pdo) || ((pdo->param.cob_id & CAN_STD_ID_MASK) != frame->id)) {
			continue;
		}

		if (node->nmt_state != CANOPEN_NMT_OPERATIONAL) {
			return;
		}

		if (can_dlc_to_bytes(frame->dlc) < pdo->len) {
			if ((pdo->flags & PDO_FLAG_LENGTH) == 0U) {
				pdo->flags |= PDO_FLAG_LENGTH;
				(void)canopen_emcy_set(node, CANOPEN_EMCY_PDO_LENGTH,
						       CANOPEN_ERR_REG_COMMUNICATION, NULL);
			}
			return;
		}

		if ((pdo->flags & (PDO_FLAG_LENGTH | PDO_FLAG_TIMEOUT)) != 0U) {
			pdo->flags &= ~(PDO_FLAG_LENGTH | PDO_FLAG_TIMEOUT);
			rpdo_errors_update(node);
		}

		if (pdo->param.event_timer != 0U) {
			pdo->event_next = sys_timepoint_calc(K_MSEC(pdo->param.event_timer));
		}

		if (pdo_event_type(pdo->param.transmission_type)) {
			rpdo_apply(node, n, frame->data);
		} else if (canopen_sync_window_open(node)) {
			memcpy(pdo->data, frame->data, pdo->len);
			pdo->flags |= PDO_FLAG_PENDING;
		}

		return;
	}
}

void canopen_pdo_sync(struct canopen_node *node, uint8_t counter)
{
	if (node->nmt_state != CANOPEN_NMT_OPERATIONAL) {
		return;
	}

	for (uint16_t n = 0U; n < CONFIG_CANOPEN_RPDO_COUNT; n++) {
		struct canopen_pdo *pdo = &node->rpdo[n];

		if ((pdo->flags & PDO_FLAG_PENDING) != 0U) {
			pdo->flags &= ~PDO_FLAG_PENDING;
			rpdo_apply(node, n, pdo->data);
		}
	}

	for (uint16_t n = 0U; n < CONFIG_CANOPEN_TPDO_COUNT; n++) {
		struct canopen_pdo *pdo = &node->tpdo[n];

		if (!pdo_valid(pdo) || pdo_event_type(pdo->param.transmission_type)) {
			continue;
		}

		if (pdo->param.transmission_type == CANOPEN_PDO_TYPE_SYNC_ACYCLIC) {
			if ((pdo->flags & PDO_FLAG_PENDING) != 0U) {
				tpdo_send(node, pdo);
			}
			continue;
		}

		if ((pdo->flags & PDO_FLAG_WAIT_START) != 0U) {
			if (counter != pdo->param.sync_start) {
				continue;
			}
			pdo->flags &= ~PDO_FLAG_WAIT_START;
		}

		pdo->sync_count++;
		if (pdo->sync_count >= pdo->param.transmission_type) {
			pdo->sync_count = 0U;
			tpdo_send(node, pdo);
		}
	}
}

void canopen_pdo_state(struct canopen_node *node)
{
	for (uint16_t n = 0U; n < CONFIG_CANOPEN_RPDO_COUNT; n++) {
		node->rpdo[n].flags = 0U;
		node->rpdo[n].event_next = sys_timepoint_calc(K_FOREVER);
	}

	rpdo_errors_update(node);

	for (uint16_t n = 0U; n < CONFIG_CANOPEN_TPDO_COUNT; n++) {
		tpdo_restart(node, &node->tpdo[n]);
	}
}

static bool tpdo_event_due(const struct canopen_pdo *pdo)
{
	return ((pdo->flags & PDO_FLAG_PENDING) != 0U) || sys_timepoint_expired(pdo->event_next);
}

void canopen_pdo_process(struct canopen_node *node)
{
	if (node->nmt_state != CANOPEN_NMT_OPERATIONAL) {
		return;
	}

	for (uint16_t n = 0U; n < CONFIG_CANOPEN_TPDO_COUNT; n++) {
		struct canopen_pdo *pdo = &node->tpdo[n];

		if (pdo_valid(pdo) && pdo_event_type(pdo->param.transmission_type) &&
		    tpdo_event_due(pdo) && sys_timepoint_expired(pdo->inhibit_end)) {
			tpdo_send(node, pdo);
		}
	}

	for (uint16_t n = 0U; n < CONFIG_CANOPEN_RPDO_COUNT; n++) {
		struct canopen_pdo *pdo = &node->rpdo[n];

		if (((pdo->flags & PDO_FLAG_TIMEOUT) == 0U) &&
		    sys_timepoint_expired(pdo->event_next)) {
			pdo->flags |= PDO_FLAG_TIMEOUT;
			(void)canopen_emcy_set(node, CANOPEN_EMCY_RPDO_TIMEOUT,
					       CANOPEN_ERR_REG_COMMUNICATION, NULL);
		}
	}
}

k_timepoint_t canopen_pdo_deadline(struct canopen_node *node)
{
	k_timepoint_t next = sys_timepoint_calc(K_FOREVER);

	if (node->nmt_state != CANOPEN_NMT_OPERATIONAL) {
		return next;
	}

	for (uint16_t n = 0U; n < CONFIG_CANOPEN_TPDO_COUNT; n++) {
		const struct canopen_pdo *pdo = &node->tpdo[n];
		k_timepoint_t due;

		if (!pdo_valid(pdo) || !pdo_event_type(pdo->param.transmission_type)) {
			continue;
		}

		due = tpdo_event_due(pdo) ? pdo->inhibit_end : pdo->event_next;
		if (sys_timepoint_cmp(due, next) < 0) {
			next = due;
		}
	}

	for (uint16_t n = 0U; n < CONFIG_CANOPEN_RPDO_COUNT; n++) {
		const struct canopen_pdo *pdo = &node->rpdo[n];

		if (((pdo->flags & PDO_FLAG_TIMEOUT) == 0U) &&
		    (sys_timepoint_cmp(pdo->event_next, next) < 0)) {
			next = pdo->event_next;
		}
	}

	return next;
}

int canopen_tpdo_trigger(struct canopen_node *node, uint16_t pdo)
{
	struct canopen_pdo *tpdo;

	if ((pdo == 0U) || (pdo > CONFIG_CANOPEN_TPDO_COUNT)) {
		return -EINVAL;
	}

	tpdo = &node->tpdo[pdo - 1U];

	k_mutex_lock(&node->lock, K_FOREVER);
	if ((tpdo->param.transmission_type == CANOPEN_PDO_TYPE_SYNC_ACYCLIC) ||
	    pdo_event_type(tpdo->param.transmission_type)) {
		tpdo->flags |= PDO_FLAG_PENDING;
	}
	k_mutex_unlock(&node->lock);

	k_poll_signal_raise(&node->signal, 0);

	return 0;
}
