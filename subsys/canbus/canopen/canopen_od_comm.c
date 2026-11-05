/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "canopen_internal.h"

#define CANOPEN_OD_DEVICE_TYPE       0x1000U
#define CANOPEN_OD_ERROR_REGISTER    0x1001U
#define CANOPEN_OD_ERROR_FIELD       0x1003U
#define CANOPEN_OD_COB_ID_SYNC       0x1005U
#define CANOPEN_OD_SYNC_PERIOD       0x1006U
#define CANOPEN_OD_SYNC_WINDOW       0x1007U
#define CANOPEN_OD_DEVICE_NAME       0x1008U
#define CANOPEN_OD_HW_VERSION        0x1009U
#define CANOPEN_OD_SW_VERSION        0x100AU
#define CANOPEN_OD_STORE             0x1010U
#define CANOPEN_OD_RESTORE           0x1011U
#define CANOPEN_OD_COB_ID_EMCY       0x1014U
#define CANOPEN_OD_INHIBIT_EMCY      0x1015U
#define CANOPEN_OD_PRODUCER_HB_TIME  0x1017U
#define CANOPEN_OD_IDENTITY          0x1018U
#define CANOPEN_OD_SYNC_OVERFLOW     0x1019U
#define CANOPEN_OD_ERROR_BEHAVIOR    0x1029U
#define CANOPEN_OD_SDO_SERVER_PARAM  0x1200U
#define CANOPEN_OD_SDO_CLIENT_PARAM  0x1280U
#define CANOPEN_OD_PDO_FIRST         0x1400U
#define CANOPEN_OD_PDO_LAST          0x1BFFU

static const uint32_t comm_device_type = CONFIG_CANOPEN_DEVICE_TYPE;
static const uint32_t comm_identity[] = {
	CONFIG_CANOPEN_VENDOR_ID,
	CONFIG_CANOPEN_PRODUCT_CODE,
	CONFIG_CANOPEN_REVISION_NUMBER,
};
static const uint8_t comm_identity_highest = 4U;
static const uint8_t comm_sdo_server_highest = 2U;
static const uint8_t comm_error_behavior_highest = 1U;
static const char comm_device_name[] = CONFIG_CANOPEN_DEVICE_NAME;
static const char comm_hw_version[] = CONFIG_CANOPEN_HW_VERSION;
static const char comm_sw_version[] = CONFIG_CANOPEN_SW_VERSION;

void canopen_od_comm_ref(struct canopen_od_ref *ref, uint16_t index, uint8_t sub, uint8_t type,
			 uint8_t attr, const void *src, void *dst, uint16_t size)
{
	ref->entry = NULL;
	ref->src = src;
	ref->dst = dst;
	ref->size = size;
	ref->index = index;
	ref->sub = sub;
	ref->type = type;
	ref->attr = attr;
}

static uint32_t comm_var(struct canopen_od_ref *ref, uint16_t index, uint8_t sub, uint8_t type,
			 uint8_t attr, const void *src, void *dst, uint16_t size)
{
	if (sub != 0U) {
		return CANOPEN_SDO_ABORT_NO_SUB_INDEX;
	}

	canopen_od_comm_ref(ref, index, sub, type, attr, src, dst, size);

	return CANOPEN_SDO_ABORT_NONE;
}

static uint32_t comm_string(struct canopen_od_ref *ref, uint16_t index, uint8_t sub,
			    const char *str, size_t size)
{
	if (size <= 1U) {
		return CANOPEN_SDO_ABORT_NO_OBJECT;
	}

	return comm_var(ref, index, sub, CANOPEN_OD_TYPE_VISIBLE_STRING, CANOPEN_OD_CONST, str,
			NULL, size - 1U);
}

static uint32_t comm_identity_find(struct canopen_node *node, uint8_t sub,
				   struct canopen_od_ref *ref)
{
	const uint16_t index = CANOPEN_OD_IDENTITY;

	if (sub == 0U) {
		canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED8, CANOPEN_OD_CONST,
				    &comm_identity_highest, NULL, sizeof(uint8_t));
	} else if (sub <= ARRAY_SIZE(comm_identity)) {
		canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED32, CANOPEN_OD_RO,
				    &comm_identity[sub - 1U], NULL, sizeof(uint32_t));
	} else if (sub == comm_identity_highest) {
		canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED32, CANOPEN_OD_RO,
				    &node->cfg->serial_number, NULL, sizeof(uint32_t));
	} else {
		return CANOPEN_SDO_ABORT_NO_SUB_INDEX;
	}

	return CANOPEN_SDO_ABORT_NONE;
}

static uint32_t comm_sdo_server_find(struct canopen_node *node, uint8_t sub,
				     struct canopen_od_ref *ref)
{
	const uint16_t index = CANOPEN_OD_SDO_SERVER_PARAM;

	if (sub == 0U) {
		canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED8, CANOPEN_OD_CONST,
				    &comm_sdo_server_highest, NULL, sizeof(uint8_t));
	} else if (sub <= comm_sdo_server_highest) {
		canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED32, CANOPEN_OD_CONST,
				    &node->sdo_cob_id[sub - 1U], NULL, sizeof(uint32_t));
	} else {
		return CANOPEN_SDO_ABORT_NO_SUB_INDEX;
	}

	return CANOPEN_SDO_ABORT_NONE;
}

static uint32_t comm_error_behavior_find(struct canopen_node *node, uint8_t sub,
					 struct canopen_od_ref *ref)
{
	const uint16_t index = CANOPEN_OD_ERROR_BEHAVIOR;

	if (sub == 0U) {
		canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED8, CANOPEN_OD_CONST,
				    &comm_error_behavior_highest, NULL, sizeof(uint8_t));
	} else if (sub == 1U) {
		canopen_od_comm_ref(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED8, CANOPEN_OD_RW,
				    &node->error_behavior, &node->error_behavior,
				    sizeof(node->error_behavior));
	} else {
		return CANOPEN_SDO_ABORT_NO_SUB_INDEX;
	}

	return CANOPEN_SDO_ABORT_NONE;
}

uint32_t canopen_od_comm_find(struct canopen_node *node, uint16_t index, uint8_t sub,
			      struct canopen_od_ref *ref)
{
	switch (index) {
	case CANOPEN_OD_DEVICE_TYPE:
		return comm_var(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED32, CANOPEN_OD_CONST,
				&comm_device_type, NULL, sizeof(comm_device_type));
	case CANOPEN_OD_ERROR_REGISTER:
		return comm_var(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED8, CANOPEN_OD_RO,
				&node->error_register, NULL, sizeof(node->error_register));
	case CANOPEN_OD_ERROR_FIELD:
	case CANOPEN_OD_COB_ID_EMCY:
	case CANOPEN_OD_INHIBIT_EMCY:
		return canopen_emcy_find(node, index, sub, ref);
	case CANOPEN_OD_HB_CONSUMER:
		return canopen_hb_find(node, sub, ref);
	case CANOPEN_OD_TIME:
		if (!IS_ENABLED(CONFIG_CANOPEN_TIME)) {
			return CANOPEN_SDO_ABORT_NO_OBJECT;
		}
		return canopen_time_find(node, sub, ref);
	case CANOPEN_OD_GUARD_TIME:
	case CANOPEN_OD_LIFE_FACTOR:
		if (!IS_ENABLED(CONFIG_CANOPEN_NODE_GUARDING)) {
			return CANOPEN_SDO_ABORT_NO_OBJECT;
		}
		return canopen_guard_find(node, index, sub, ref);
	case CANOPEN_OD_ERROR_BEHAVIOR:
		return comm_error_behavior_find(node, sub, ref);
	case CANOPEN_OD_STORE:
	case CANOPEN_OD_RESTORE:
		if (!IS_ENABLED(CONFIG_CANOPEN_STORAGE)) {
			return CANOPEN_SDO_ABORT_NO_OBJECT;
		}
		return canopen_storage_find(index, sub, ref);
	case CANOPEN_OD_DEVICE_NAME:
		return comm_string(ref, index, sub, comm_device_name, sizeof(comm_device_name));
	case CANOPEN_OD_HW_VERSION:
		return comm_string(ref, index, sub, comm_hw_version, sizeof(comm_hw_version));
	case CANOPEN_OD_SW_VERSION:
		return comm_string(ref, index, sub, comm_sw_version, sizeof(comm_sw_version));
	case CANOPEN_OD_PRODUCER_HB_TIME:
		return comm_var(ref, index, sub, CANOPEN_OD_TYPE_UNSIGNED16, CANOPEN_OD_RW,
				&node->hb_producer_ms, &node->hb_producer_ms,
				sizeof(node->hb_producer_ms));
	case CANOPEN_OD_IDENTITY:
		return comm_identity_find(node, sub, ref);
	case CANOPEN_OD_SDO_SERVER_PARAM:
		return comm_sdo_server_find(node, sub, ref);
	case CANOPEN_OD_SDO_CLIENT_PARAM:
		if (!IS_ENABLED(CONFIG_CANOPEN_MASTER)) {
			return CANOPEN_SDO_ABORT_NO_OBJECT;
		}
		return canopen_sdo_client_find(node, sub, ref);
	case CANOPEN_OD_COB_ID_SYNC:
	case CANOPEN_OD_SYNC_PERIOD:
	case CANOPEN_OD_SYNC_WINDOW:
	case CANOPEN_OD_SYNC_OVERFLOW:
		return canopen_sync_find(node, index, sub, ref);
	default:
		if ((index >= CANOPEN_OD_PDO_FIRST) && (index <= CANOPEN_OD_PDO_LAST)) {
			return canopen_pdo_find(node, index, sub, ref);
		}
		return CANOPEN_SDO_ABORT_NO_OBJECT;
	}
}

uint32_t canopen_od_comm_write(struct canopen_node *node, const struct canopen_od_ref *ref,
			       uint32_t value)
{
	switch (ref->index) {
	case CANOPEN_OD_ERROR_FIELD:
	case CANOPEN_OD_COB_ID_EMCY:
	case CANOPEN_OD_INHIBIT_EMCY:
		return canopen_emcy_write(node, ref->index, value);
	case CANOPEN_OD_HB_CONSUMER:
		return canopen_hb_write(node, ref->sub, value);
	case CANOPEN_OD_TIME:
		if (!IS_ENABLED(CONFIG_CANOPEN_TIME)) {
			return CANOPEN_SDO_ABORT_READ_ONLY;
		}
		return canopen_time_write(node, value);
	case CANOPEN_OD_GUARD_TIME:
	case CANOPEN_OD_LIFE_FACTOR:
		if (!IS_ENABLED(CONFIG_CANOPEN_NODE_GUARDING)) {
			return CANOPEN_SDO_ABORT_READ_ONLY;
		}
		return canopen_guard_write(node, ref->index, value);
	case CANOPEN_OD_ERROR_BEHAVIOR:
		if (value > CANOPEN_ERROR_BEHAVIOR_STOPPED) {
			return CANOPEN_SDO_ABORT_INVALID_VALUE;
		}
		node->error_behavior = (uint8_t)value;
		return CANOPEN_SDO_ABORT_NONE;
	case CANOPEN_OD_PRODUCER_HB_TIME:
		node->hb_producer_ms = (uint16_t)value;
		canopen_nmt_hb_restart(node);
		return CANOPEN_SDO_ABORT_NONE;
	case CANOPEN_OD_COB_ID_SYNC:
	case CANOPEN_OD_SYNC_PERIOD:
	case CANOPEN_OD_SYNC_WINDOW:
	case CANOPEN_OD_SYNC_OVERFLOW:
		return canopen_sync_write(node, ref->index, value);
	case CANOPEN_OD_STORE:
	case CANOPEN_OD_RESTORE:
		if (!IS_ENABLED(CONFIG_CANOPEN_STORAGE)) {
			return CANOPEN_SDO_ABORT_READ_ONLY;
		}
		return canopen_storage_write(node, ref->index, ref->sub, value);
	case CANOPEN_OD_SDO_CLIENT_PARAM:
		if (!IS_ENABLED(CONFIG_CANOPEN_MASTER)) {
			return CANOPEN_SDO_ABORT_READ_ONLY;
		}
		return canopen_sdo_client_write(node, ref->sub, value);
	default:
		if ((ref->index >= CANOPEN_OD_PDO_FIRST) && (ref->index <= CANOPEN_OD_PDO_LAST)) {
			return canopen_pdo_write(node, ref->index, ref->sub, value);
		}
		return CANOPEN_SDO_ABORT_READ_ONLY;
	}
}

void canopen_od_comm_reset(struct canopen_node *node)
{
	node->hb_producer_ms = CONFIG_CANOPEN_HEARTBEAT_PRODUCER_MS;
	node->error_behavior = CANOPEN_ERROR_BEHAVIOR_PRE_OPERATIONAL;
	node->sdo_cob_id[0] = CANOPEN_COB_SDO_RX + node->cfg->node_id;
	node->sdo_cob_id[1] = CANOPEN_COB_SDO_TX + node->cfg->node_id;
	canopen_emcy_reset(node, false);
	canopen_hb_reset(node);
	canopen_sync_reset(node);
	canopen_pdo_reset(node);

	if (IS_ENABLED(CONFIG_CANOPEN_MASTER)) {
		canopen_sdo_client_reset(node);
	}

	if (IS_ENABLED(CONFIG_CANOPEN_TIME)) {
		canopen_time_reset(node);
	}

	if (IS_ENABLED(CONFIG_CANOPEN_NODE_GUARDING)) {
		canopen_guard_reset(node);
	}

	if (IS_ENABLED(CONFIG_CANOPEN_STORAGE)) {
		canopen_storage_load(node, false);
	}
}
