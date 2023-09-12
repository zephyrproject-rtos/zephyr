/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2022 T-Mobile USA, Inc.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/gatt.h>
#include <rsi_ble.h>
#include <rsi_ble_apis.h>
#include <rsi_ble_common_config.h>
#include <rsi_bt_apis.h>
#include <rsi_bt_common.h>
#include <rsi_common_apis.h>
#include <zephyr/kernel.h>
#define LOG_MODULE_NAME rsi_bt_gatt
#include "rs9116w_ble_conn.h"
#if defined(BT_L2CAP_RX_MTU) && defined(BT_L2CAP_TX_MTU)
#define BT_ATT_MTU (MIN(BT_L2CAP_RX_MTU, BT_L2CAP_TX_MTU))
#else
#define BT_ATT_MTU (RSI_BLE_MTU_SIZE + 8)
#endif

extern uint16_t conn_mtu[CONFIG_BT_MAX_CONN];

/**
 * @brief Device name characteristic read callback
 */
static ssize_t read_name(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	const char *name = bt_get_name();

	return bt_gatt_attr_read(conn, attr, buf, len, offset, name,
				 strlen(name));
}

static const uint16_t gap_appearance = CONFIG_BT_DEVICE_APPEARANCE;

/**
 * @brief Device appearance characteristic read callback
 */
static ssize_t read_appearance(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	uint16_t appearance = sys_cpu_to_le16(gap_appearance);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &appearance,
				 sizeof(appearance));
}

/* TODO: Add other columns to GAP Service */
BT_GATT_SERVICE_DEFINE(
	_2_gap_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_GAP),
	BT_GATT_CHARACTERISTIC(BT_UUID_GAP_DEVICE_NAME, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_name, NULL, NULL),
	BT_GATT_CHARACTERISTIC(BT_UUID_GAP_APPEARANCE, BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ, read_appearance, NULL,
			       NULL),);

struct rsi_attr_handle_table_entry_s {
	uint16_t handle;
	const struct bt_gatt_attr *attr;
};

struct rsi_attr_handle_table_entry_s att_handle_table[RSI_BLE_MAX_NBR_ATT_REC] = {0};

ssize_t bt_gatt_attr_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  void *buf, uint16_t buf_len, uint16_t offset,
			  const void *value, uint16_t value_len)
{
	uint16_t len;

	if (offset > value_len) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	len = MIN(buf_len, value_len - offset);

	BT_DBG("handle 0x%04x offset %u length %u", attr->handle, offset, len);

	memcpy(buf, (uint8_t *)value + offset, len);

	return len;
}

ssize_t bt_gatt_attr_read_service(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr, void *buf,
				  uint16_t len, uint16_t offset)
{
	struct bt_uuid *uuid = attr->user_data;

	if (uuid->type == BT_UUID_TYPE_16) {
		uint16_t uuid16 = sys_cpu_to_le16(BT_UUID_16(uuid)->val);

		return bt_gatt_attr_read(conn, attr, buf, len, offset, &uuid16,
					 2);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 BT_UUID_128(uuid)->val, 16);
}

struct gatt_incl {
	uint16_t start_handle;
	uint16_t end_handle;
	uint16_t uuid16;
} __packed;

enum {
	RSI_EVT_NONE,
	RSI_EVT_READ,
	RSI_EVT_WRITE,
	RSI_EVT_MTU,
	RSI_EVT_PREP_WRITE,
	RSI_EVT_EXEC_WRITE
};

struct pw_dummy {
	bt_addr_t addr;
	uint16_t handle;
};

struct rsi_event_s {
	uint8_t event_type;
	union {
		rsi_ble_event_write_t w;
		rsi_ble_read_req_t r;
		bt_addr_t addr;
#if CONFIG_BT_ATT_PREPARE_COUNT > 0
		rsi_ble_event_prepare_write_t *pw;
#else
		struct pw_dummy pw;
#endif
		rsi_ble_execute_write_t ew;
	};
	void *next;
};

struct rsi_event_s gatt_event_queue[CONFIG_RSI_BT_EVENT_QUEUE_SIZE] = {0};
int gatt_event_ptr;
K_SEM_DEFINE(gatt_evt_queue_sem, 1, 1);

/**
 * @brief Get the event slot pointer
 *
 * @return Event slot pointer or NULL if none available
 */
static struct rsi_event_s *get_event_slot(void)
{
	k_sem_take(&gatt_evt_queue_sem, K_FOREVER);
	int old_event_ptr = gatt_event_ptr;

	gatt_event_ptr++;
	gatt_event_ptr %= CONFIG_RSI_BT_EVENT_QUEUE_SIZE;
	struct rsi_event_s *target_event = &gatt_event_queue[gatt_event_ptr];

	if (target_event->event_type) {
		gatt_event_ptr = old_event_ptr;
		rsi_bt_raise_evt(); /* Raise event to force processing */
		k_sem_give(&gatt_evt_queue_sem);
		return NULL;
	}
	k_sem_give(&gatt_evt_queue_sem);
	return target_event;
}
#if CONFIG_BT_ATT_PREPARE_COUNT > 0
rsi_ble_event_prepare_write_t prepared_writes[CONFIG_BT_ATT_PREPARE_COUNT] = {
	0};
size_t prepared_writes_count;
#else
uint16_t last_pw_handle;
#endif

/**
 * @brief Callback for Bluetooth LE GATT prepare write event
 *
 * @param event_id Prepare write id
 * @param rsi_app_ble_prepared_write_event Prepare write event data
 */
static void rsi_ble_on_gatt_prepare_write_event(
	uint16_t event_id,
	rsi_ble_event_prepare_write_t *rsi_app_ble_prepared_write_event)
{
	struct rsi_event_s *target_event = get_event_slot();

	if (!target_event) {
		BT_ERR("Event queue full!");
		return;
	}
	target_event->event_type = RSI_EVT_PREP_WRITE;
#if CONFIG_BT_ATT_PREPARE_COUNT > 0
	if (prepared_writes_count < CONFIG_BT_ATT_PREPARE_COUNT) {
		memcpy(&prepared_writes[prepared_writes_count],
		       rsi_app_ble_prepared_write_event,
		       sizeof(rsi_ble_event_prepare_write_t));
		target_event->pw = &prepared_writes[prepared_writes_count];
		prepared_writes_count++;
	} else {
		target_event->pw = NULL;
	}
#else
	last_pw_handle =
		rsi_bytes2R_to_uint16(rsi_app_ble_prepared_write_event->handle);
	memcpy(target_event->pw.addr.val,
	       rsi_app_ble_prepared_write_event->dev_addr, 6);
	target_event->pw.handle =
		rsi_bytes2R_to_uint16(rsi_app_ble_prepared_write_event->handle);
#endif
	rsi_bt_raise_evt();
}

/**
 * @brief Callback for Bluetooth LE GATT execute write event
 *
 * @param event_id Execute write id
 * @param rsi_app_ble_execute_write_event Execute write event data
 */
static void rsi_ble_on_execute_write_event(
	uint16_t event_id,
	rsi_ble_execute_write_t *rsi_app_ble_execute_write_event)
{

	struct rsi_event_s *target_event = get_event_slot();

	if (!target_event) {
		BT_ERR("Event queue full!");
		return;
	}
	target_event->event_type = RSI_EVT_EXEC_WRITE;
	memcpy(&target_event->ew, rsi_app_ble_execute_write_event,
	       sizeof(rsi_ble_execute_write_t));
	rsi_bt_raise_evt();
}

static uint8_t get_service_handles(const struct bt_gatt_attr *attr,
				   uint16_t handle, void *user_data)
{
	struct gatt_incl *include = user_data;

	/* Stop if attribute is a service */
	if (!bt_uuid_cmp(attr->uuid, BT_UUID_GATT_PRIMARY) ||
	    !bt_uuid_cmp(attr->uuid, BT_UUID_GATT_SECONDARY)) {
		return BT_GATT_ITER_STOP;
	}

	include->end_handle = handle;

	return BT_GATT_ITER_CONTINUE;
}

ssize_t bt_gatt_attr_read_included(struct bt_conn *conn,
				   const struct bt_gatt_attr *attr, void *buf,
				   uint16_t len, uint16_t offset)
{
	struct bt_gatt_attr *incl = attr->user_data;
	uint16_t handle = bt_gatt_attr_get_handle(incl);
	struct bt_uuid *uuid = incl->user_data;
	struct gatt_incl pdu;
	uint8_t value_len;

	/* first attr points to the start handle */
	pdu.start_handle = sys_cpu_to_le16(handle);
	value_len = sizeof(pdu.start_handle) + sizeof(pdu.end_handle);

	/*
	 * Core 4.2, Vol 3, Part G, 3.2,
	 * The Service UUID shall only be present when the UUID is a
	 * 16-bit Bluetooth UUID.
	 */
	if (uuid->type == BT_UUID_TYPE_16) {
		pdu.uuid16 = sys_cpu_to_le16(BT_UUID_16(uuid)->val);
		value_len += sizeof(pdu.uuid16);
	}

	/* Lookup for service end handle */
	bt_gatt_foreach_attr(handle + 1, 0xffff, get_service_handles, &pdu);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &pdu, value_len);
}

/**
 * @brief Clear CCC status
 */
static void clear_ccc_cfg(struct bt_gatt_ccc_cfg *cfg)
{
	bt_addr_le_copy(&cfg->peer, BT_ADDR_LE_ANY);
	cfg->id = 0U;
	cfg->value = 0U;
}

/**
 * @brief Callback for CCC change events
 *
 * @param attr CCC attribute
 * @param ccc CCC value
 */
static void gatt_ccc_changed(const struct bt_gatt_attr *attr,
			     struct _bt_gatt_ccc *ccc)
{
	int i;
	uint16_t value = 0x0000;

	for (i = 0; i < ARRAY_SIZE(ccc->cfg); i++) {
		if (ccc->cfg[i].value > value) {
			value = ccc->cfg[i].value;
		}
	}

	BT_DBG("ccc %p value 0x%04x", ccc, value);

	if (value != ccc->value) {
		ccc->value = value;
		if (ccc->cfg_changed) {
			ccc->cfg_changed(attr, value);
		}
	}
}

/**
 * @brief Find config for a provided CCC
 *
 * @param conn Connection to seek
 * @param ccc CCC value
 * @return CCC config or NULL if not found
 */
static struct bt_gatt_ccc_cfg *find_ccc_cfg(const struct bt_conn *conn,
					    struct _bt_gatt_ccc *ccc)
{
	for (size_t i = 0; i < ARRAY_SIZE(ccc->cfg); i++) {
		struct bt_gatt_ccc_cfg *cfg = &ccc->cfg[i];

		if (conn) {
			if (bt_conn_is_peer_addr_le(conn, cfg->id,
						    &cfg->peer)) {
				return cfg;
			}
		} else if (!memcmp(&cfg->peer.a.val, BT_ADDR_LE_ANY->a.val,
				   6)) {
			return cfg;
		}
	}

	return NULL;
}

ssize_t bt_gatt_attr_write_ccc(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, const void *buf,
			       uint16_t len, uint16_t offset, uint8_t flags)
{
	struct _bt_gatt_ccc *ccc = attr->user_data;
	struct bt_gatt_ccc_cfg *cfg = NULL;
	uint16_t value;

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (!len || len > sizeof(uint16_t)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (len < sizeof(uint16_t)) {
		value = *(uint8_t *)buf;
	} else {
		value = sys_get_le16(buf);
	}

	cfg = find_ccc_cfg(conn, ccc);
	if (!cfg) {
		/* If there's no existing entry, but the new value is zero,
		 * we don't need to do anything, since a disabled CCC is
		 * behavioraly the same as no written CCC.
		 */
		if (!value) {
			return len;
		}

		cfg = find_ccc_cfg(NULL, ccc);
		if (!cfg) {
			BT_WARN("No space to store CCC cfg");
			return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
		}

		bt_addr_le_copy(&cfg->peer, &conn->le.dst);
		cfg->id = conn->id;
	}

	/* Confirm write if cfg is managed by application */
	if (ccc->cfg_write) {
		ssize_t write = ccc->cfg_write(conn, attr, value);

		if (write < 0) {
			return write;
		}

		/* Accept size=1 for backwards compatibility */
		if (write != sizeof(value) && write != 1) {
			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}
	}

	cfg->value = value;

	BT_DBG("handle 0x%04x value %u", attr->handle, cfg->value);

	/* Update cfg if don't match */
	if (cfg->value != ccc->value) {
		gatt_ccc_changed(attr, ccc);
	}

	/* Disabled CCC is the same as no configured CCC, so clear the entry */
	if (!value) {
		clear_ccc_cfg(cfg);
	}

	return len;
}

ssize_t bt_gatt_attr_read_cep(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	const struct bt_gatt_cep *value = attr->user_data;
	uint16_t props = sys_cpu_to_le16(value->properties);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &props,
				 sizeof(props));
}

ssize_t bt_gatt_attr_read_cud(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	const char *value = attr->user_data;

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 strlen(value));
}

/*Dummy functions (aren't actually called)*/

ssize_t bt_gatt_attr_read_chrc(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	return -ENOTSUP;
}
ssize_t bt_gatt_attr_read_ccc(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	return -ENOTSUP;
}

struct gatt_cpf {
	uint8_t format;
	int8_t exponent;
	uint16_t unit;
	uint8_t name_space;
	uint16_t description;
} __packed;

ssize_t bt_gatt_attr_read_cpf(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	const struct bt_gatt_cpf *cpf = attr->user_data;
	struct gatt_cpf value;

	value.format = cpf->format;
	value.exponent = cpf->exponent;
	value.unit = sys_cpu_to_le16(cpf->unit);
	value.name_space = cpf->name_space;
	value.description = sys_cpu_to_le16(cpf->description);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &value,
				 sizeof(value));
}

/**
 * @brief Inserts an attribute into the handle table
 *
 * @param handle Attribute handle
 * @param attr Attribute
 * @return Table index or UINT16_MAX on table full
 */
static uint16_t rsi_attr_handle_table_insert(uint16_t handle,
					     const struct bt_gatt_attr *attr)
{
	uint16_t target;

	target = handle % RSI_BLE_MAX_NBR_ATT_REC;
	for (int i = 0; i < RSI_BLE_MAX_NBR_ATT_REC; i++) {
		if (att_handle_table[target].handle == 0 ||
		    att_handle_table[target].handle == UINT16_MAX ||
		    att_handle_table[target].handle == handle) {
			att_handle_table[target].handle = handle;
			att_handle_table[target].attr = attr;
			return target;
		}
		target++;
		target %= RSI_BLE_MAX_NBR_ATT_REC;
	}
	return UINT16_MAX;
}

/**
 * @brief Finds an attribute using the provided handle
 *
 * @param handle Attribute Handle
 * @return Attribute pointer or NULL on handle not found
 */
const static struct bt_gatt_attr *rsi_attr_handle_table_search(uint16_t handle)
{
	uint16_t target;

	target = handle % RSI_BLE_MAX_NBR_ATT_REC;
	for (int i = 0; i < RSI_BLE_MAX_NBR_ATT_REC; i++) {
		if (att_handle_table[target].handle == 0) {
			return NULL;
		} else if (att_handle_table[target].handle == handle) {
			return att_handle_table[target].attr;
		}
		target++;
		target %= RSI_BLE_MAX_NBR_ATT_REC;
	}
	return NULL;
}

static atomic_t gatt_inited;

uint16_t mtu_size;

static uint16_t last_static_handle = UINT16_MAX;

/**
 * @brief Conversion function to infer properties from permissions
 *
 * @param perms Permissions value
 * @return Properties value
 */
uint8_t perm_to_property(uint8_t perms)
{
	return (perms & BT_GATT_PERM_READ)	  ? RSI_BLE_ATT_PROPERTY_READ
	       : 0 | (perms & BT_GATT_PERM_WRITE) ? RSI_BLE_ATT_PROPERTY_WRITE
	       : 0 | (perms & BT_GATT_PERM_READ_ENCRYPT)
		       ? RSI_BLE_ATT_PROPERTY_READ
	       : 0 | (perms & BT_GATT_PERM_WRITE_ENCRYPT)
		       ? RSI_BLE_ATT_PROPERTY_WRITE
	       : 0 | (perms & BT_GATT_PERM_READ_AUTHEN)
		       ? RSI_BLE_ATT_PROPERTY_READ
	       : 0 | (perms & BT_GATT_PERM_WRITE_AUTHEN)
		       ? RSI_BLE_ATT_PROPERTY_WRITE
		       : 0;
}

/**
 * @brief Add a new characteristic attribute
 *
 * @param serv_handler Service handler
 * @param handle Characteristic attribute handle
 * @param val_prop Value properties
 * @param att_val_handle Attribute Handle
 * @param att_val_uuid Attribute UUID
 */
static void rsi_ble_add_char_serv_att(void *serv_handler, uint16_t handle,
				      uint8_t val_prop, uint16_t att_val_handle,
				      uuid_t att_val_uuid)
{
	rsi_ble_req_add_att_t new_att = {0};

	/* Prepare the structure */
	new_att.serv_handler = serv_handler;
	new_att.handle = handle;
	new_att.att_uuid.size = 2;
	new_att.att_uuid.val.val16 = RSI_BLE_CHAR_SERV_UUID;
	new_att.property = RSI_BLE_ATT_PROPERTY_READ;

	/* Perpare the attribute's value */
	new_att.data_len = att_val_uuid.size + 4;
	new_att.data[0] = val_prop;
	rsi_uint16_to_2bytes(&new_att.data[2], att_val_handle);
	if (att_val_uuid.size == BT_UUID_SIZE_16) {
		rsi_uint16_to_2bytes(&new_att.data[4], att_val_uuid.val.val16);
	} else if (att_val_uuid.size == BT_UUID_SIZE_32) {
		rsi_uint32_to_4bytes(&new_att.data[4], att_val_uuid.val.val32);
	} else if (att_val_uuid.size == BT_UUID_SIZE_128) {
		memcpy(&new_att.data[4], &att_val_uuid.val.val128,
		       att_val_uuid.size);
	}

	/* Add the attribute to the service */
	rsi_ble_add_attribute(&new_att);
}

/**
 * @brief Add new characteristic value attribute
 *
 * @param serv_handler Service handler
 * @param handle Value handle
 * @param att_type_uuid Type UUID
 * @param val_prop Properties
 * @param use_security Decides whether to set the security bitmap flag
 */
static void rsi_ble_add_char_val_att(void *serv_handler, uint16_t handle,
				     uuid_t att_type_uuid, uint8_t val_prop,
				     bool use_security)
{
	rsi_ble_req_add_att_t new_att = {0};

	/* Prepare the structure */
	new_att.serv_handler = serv_handler;
	new_att.handle = handle;
	new_att.config_bitmap = BIT(0) | (use_security ? BIT(1) : 0);
	memcpy(&new_att.att_uuid, &att_type_uuid, sizeof(uuid_t));
	new_att.property = val_prop;

	new_att.data_len = BT_ATT_MTU;
	/* Add the attribute to the service */
	rsi_ble_add_attribute(&new_att);
}

/**
 * @brief Adds a managed CCC attribute
 *
 * @param serv_handler Service handler
 * @param handle CCC handle
 */
static void rsi_ble_add_ccc_att(void *serv_handler, uint16_t handle)
{
	/* Preparing the CCC attribute & value */
	rsi_ble_req_add_att_t new_att = {0};

	new_att.serv_handler = serv_handler;
	new_att.handle = handle;
	new_att.att_uuid.size = 2;
	new_att.att_uuid.val.val16 = RSI_BLE_CLIENT_CHAR_UUID;
	new_att.property =
		RSI_BLE_ATT_PROPERTY_READ | RSI_BLE_ATT_PROPERTY_WRITE;
	new_att.data_len = 2;

	/* Add the attribute to the service */
	rsi_ble_add_attribute(&new_att);
}

/**
 * @brief Initializes all defined GATT static services
 */
static void bt_gatt_service_init(void)
{
	STRUCT_SECTION_FOREACH(bt_gatt_service_static, svc)
	{
		uuid_t new_uuid = {0};
		rsi_ble_resp_add_serv_t new_serv_resp = {0};
		struct bt_uuid *service_uuid = NULL;

		for (int i = 0; i < svc->attr_count; i++) {
			const struct bt_gatt_attr *attr = &svc->attrs[i];

			if (!bt_uuid_cmp(attr->uuid, BT_UUID_GATT_PRIMARY)) {
				service_uuid = attr->user_data;
				break;
			}
		}
		if (service_uuid == NULL) {
			BT_ERR("Failed to add service: No primary UUID!");
			continue;
		}
		rsi_uuid_convert(service_uuid, &new_uuid);
		rsi_ble_add_service(new_uuid, &new_serv_resp);

		const struct bt_uuid *prop_known_uuids[3] = {0};
		uint8_t prop_knowns[3];

		last_static_handle = new_serv_resp.start_handle;

		for (int i = 0; i < svc->attr_count; i++) {
			const struct bt_gatt_attr *attr = &svc->attrs[i];
			uint16_t handle = ++last_static_handle;
			bool use_security =
				(attr->perm & (BT_GATT_PERM_READ_ENCRYPT |
					       BT_GATT_PERM_WRITE_ENCRYPT |
					       BT_GATT_PERM_READ_AUTHEN |
					       BT_GATT_PERM_WRITE_AUTHEN));

			if (!bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CHRC)) {
				struct bt_gatt_chrc *chrc = attr->user_data;

				rsi_uuid_convert(chrc->uuid, &new_uuid);
				rsi_ble_add_char_serv_att(
					new_serv_resp.serv_handler, handle,
					chrc->properties, handle + 1, new_uuid);
				for (int j = 0; j < 3; j++) {
					if (prop_known_uuids[i] == NULL) {
						prop_known_uuids[j] =
							chrc->uuid;
						prop_knowns[j] =
							chrc->properties;
						break;
					}
				}
				if (rsi_attr_handle_table_insert(
					    handle, attr) == UINT16_MAX) {
					BT_ERR("Failed to add attribute, table "
					       "full!");
					return;
				}
			} else if (!bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CCC)) {
				rsi_ble_add_ccc_att(new_serv_resp.serv_handler,
						    handle);
				if (rsi_attr_handle_table_insert(
					    handle, attr) == UINT16_MAX) {
					BT_ERR("Failed to add attribute, table "
					       "full!");
					return;
				}
			} else {
				uint8_t props = 0;

				for (int j = 0; j < 3; j++) {
					if (prop_known_uuids[j] &&
					    !bt_uuid_cmp(prop_known_uuids[j],
							 attr->uuid)) {
						props = prop_knowns[j];
						prop_known_uuids[j] = NULL;
						break;
					}
				}
				if (!props) {
					props = perm_to_property(attr->perm);
				}
				rsi_uuid_convert(attr->uuid, &new_uuid);
				rsi_ble_add_char_val_att(
					new_serv_resp.serv_handler, handle,
					new_uuid, props, use_security);
				if (rsi_attr_handle_table_insert(
					    handle, attr) == UINT16_MAX) {
					BT_ERR("Failed to add attribute, table "
					       "full!");
					return;
				}
			}
		}
	}
}

/**
 * @brief Callback for Bluetooth LE GATT write event
 *
 * @param event_id Write id
 * @param rsi_ble_write Write event data
 */
static void rsi_ble_on_gatt_write_event(uint16_t event_id,
					rsi_ble_event_write_t *rsi_ble_write)
{
	struct rsi_event_s *target_event = get_event_slot();

	if (!target_event) {
		BT_ERR("Event queue full!");
		return;
	}
	target_event->event_type = RSI_EVT_WRITE;
	memcpy(&target_event->w, rsi_ble_write, sizeof(rsi_ble_event_write_t));
	rsi_bt_raise_evt();
}

/**
 * @brief Callback for Bluetooth LE GATT read event
 *
 * @param event_id Read id
 * @param rsi_ble_read_req  Read event data
 */
static void rsi_ble_on_read_req_event(uint16_t event_id,
				      rsi_ble_read_req_t *rsi_ble_read_req)
{
	struct rsi_event_s *target_event = get_event_slot();

	if (!target_event) {
		BT_ERR("Event queue full!");
		return;
	}
	target_event->event_type = RSI_EVT_READ;
	memcpy(&target_event->r, rsi_ble_read_req, sizeof(rsi_ble_read_req_t));
	rsi_bt_raise_evt();
}

/**
 * @brief Callback for Bluetooth LE GATT MTU event
 *
 * @param rsi_ble_mtu MTU data
 */
static void rsi_ble_on_mtu_event(rsi_ble_event_mtu_t *rsi_ble_mtu)
{
	bt_addr_le_t addr;

	memcpy(addr.a.val, rsi_ble_mtu->dev_addr, 6);
	struct bt_conn *conn = bt_conn_lookup_addr_le(0, &addr);

	if (!conn) {
		BT_WARN("MTU Event: Unable to find connection");
		return;
	}

	conn_mtu[conn->handle] = rsi_ble_mtu->mtu_size;
	bt_conn_unref(conn);
	rsi_bt_raise_evt();
}

/**
 * @brief Initializes Bluetooth LE GATT callbacks
 *
 */
static void bt_gatt_cb_init(void)
{
	rsi_ble_gatt_register_callbacks(
		NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		rsi_ble_on_gatt_write_event,
		rsi_ble_on_gatt_prepare_write_event,
		rsi_ble_on_execute_write_event, rsi_ble_on_read_req_event,
		rsi_ble_on_mtu_event, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL);
}

/**
 * @brief Initializes MTU exchange on device connect
 *
 * @param conn Device connection object
 */
void bt_gatt_connected(struct bt_conn *conn)
{
	struct rsi_event_s *target_event = get_event_slot();

	if (!target_event) {
		BT_ERR("Event queue full!");
		return;
	}
	target_event->event_type = RSI_EVT_MTU;
	memcpy(target_event->addr.val, conn->le.dst.a.val, 6);
	rsi_bt_raise_evt();
}

/**
 * @brief Cleans up after device disconnect
 *
 * @param conn Device connection object
 */
void bt_gatt_disconnected(struct bt_conn *conn)
{
/* Clear Prepare Writes */
#if CONFIG_BT_ATT_PREPARE_COUNT > 0
	for (int i = 0; i < ARRAY_SIZE(prepared_writes); i++) {
		if (!memcmp(prepared_writes[i].dev_addr, conn->le.dst.a.val,
			    6)) {
			for (int j = i + 1; i < ARRAY_SIZE(prepared_writes);
			     j++) {
				memcpy(&prepared_writes[j - 1],
				       &prepared_writes[j],
				       sizeof(rsi_ble_event_prepare_write_t));
			}
			memset(&prepared_writes[ARRAY_SIZE(prepared_writes) -
						1],
			       0, sizeof(rsi_ble_event_prepare_write_t));
		}
	}
#endif
	const struct bt_gatt_attr *attr;
	struct _bt_gatt_ccc *ccc;
	bool value_used;

	for (int i = 0; i < RSI_BLE_MAX_NBR_ATT_REC; i++) {
		attr = att_handle_table[i].attr;
		if (attr == NULL) {
			continue;
		}
		/* Check attribute user_data must be of type struct _bt_gatt_ccc */
		if (attr->write != bt_gatt_attr_write_ccc) {
			continue;
		}

		ccc = attr->user_data;

		/* If already disabled skip */
		if (!ccc->value) {
			continue;
		}

		/* Checking if all values are disabled */
		value_used = false;

		for (size_t j = 0; j < ARRAY_SIZE(ccc->cfg); j++) {
			struct bt_gatt_ccc_cfg *cfg = &ccc->cfg[j];

			/* Ignore configurations with disabled value */
			if (!cfg->value) {
				continue;
			}

			if (!bt_conn_is_peer_addr_le(conn, cfg->id,
						     &cfg->peer)) {
				struct bt_conn *tmp;

				/* Skip if there is another peer connected */
				tmp = bt_conn_lookup_addr_le(cfg->id,
							     &cfg->peer);
				if (tmp) {
					if (tmp->state == BT_CONN_CONNECTED) {
						value_used = true;
					}

					bt_conn_unref(tmp);
				}
			} else {
				bt_addr_le_copy(&cfg->peer, &conn->le.dst);
			}
		}

		/* If all values are now disabled, reset value while
		 * disconnected
		 */
		if (!value_used) {
			ccc->value = 0U;
			if (ccc->cfg_changed) {
				ccc->cfg_changed(attr, ccc->value);
			}

			BT_DBG("ccc %p reseted", ccc);
		}
	}

	/* Clear leftover CCCs */
	for (int i = 0; i < RSI_BLE_MAX_NBR_ATT_REC; i++) {
		attr = att_handle_table[i].attr;

		if (attr == NULL) {
			continue;
		}

		struct bt_gatt_ccc_cfg *cfg;

		/* Check if attribute is a CCC */
		if (attr->write != bt_gatt_attr_write_ccc) {
			continue;
		}

		ccc = attr->user_data;

		/* Check if there is a cfg for the peer */
		cfg = find_ccc_cfg(conn, ccc);
		if (cfg) {
			memset(cfg, 0, sizeof(*cfg));
		}

		continue;
	}
}

/**
 * @brief Initialize GATT server
 *
 */
void bt_gatt_init(void)
{
	if (!atomic_cas(&gatt_inited, 0, 1)) {
		return;
	}

	bt_gatt_service_init();
	bt_gatt_cb_init();

	for (int i = 0; i < ARRAY_SIZE(gatt_event_queue); i++) {
		gatt_event_queue[i].next = &gatt_event_queue[i + 1];
	}
	gatt_event_queue[ARRAY_SIZE(gatt_event_queue) - 1].next =
		gatt_event_queue;
}

uint16_t bt_gatt_get_mtu(struct bt_conn *conn)
{
	return conn_mtu[conn->handle];
}

/**
 * @brief BT GATT attribute iteration handler
 *
 */
static uint8_t gatt_foreach_iter(const struct bt_gatt_attr *attr,
				 uint16_t handle, uint16_t start_handle,
				 uint16_t end_handle,
				 const struct bt_uuid *uuid,
				 const void *attr_data, uint16_t *num_matches,
				 bt_gatt_attr_func_t func, void *user_data)
{
	uint8_t result;

	/* Stop if over the requested range */
	if (handle > end_handle) {
		return BT_GATT_ITER_STOP;
	}

	/* Check if attribute handle is within range */
	if (handle < start_handle) {
		return BT_GATT_ITER_CONTINUE;
	}

	/* Match attribute UUID if set */
	if (uuid && bt_uuid_cmp(uuid, attr->uuid)) {
		return BT_GATT_ITER_CONTINUE;
	}

	/* Match attribute user_data if set */
	if (attr_data && attr_data != attr->user_data) {
		return BT_GATT_ITER_CONTINUE;
	}

	*num_matches -= 1;
	result = func(attr, handle, user_data);

	if (!*num_matches) {
		return BT_GATT_ITER_STOP;
	}

	return result;
}

void bt_gatt_foreach_attr_type(uint16_t start_handle, uint16_t end_handle,
			       const struct bt_uuid *uuid,
			       const void *attr_data, uint16_t num_matches,
			       bt_gatt_attr_func_t func, void *user_data)
{
	size_t i;

	if (!num_matches) {
		num_matches = UINT16_MAX;
	}

	uint16_t handle = 1;

	STRUCT_SECTION_FOREACH(bt_gatt_service_static, static_svc)
	{
		/* Skip ahead if start is not within service handles */
		if (bt_gatt_attr_get_handle(
			    &static_svc->attrs[static_svc->attr_count -
					       1]) < start_handle) {
			continue;
		} else {
			handle = bt_gatt_attr_get_handle(
				static_svc->attrs);
		}

		for (i = 0; i < static_svc->attr_count; i++, handle++) {
			if (gatt_foreach_iter(
				    &static_svc->attrs[i], handle,
				    start_handle, end_handle, uuid,
				    attr_data, &num_matches, func,
				    user_data) == BT_GATT_ITER_STOP) {
				return;
			}
		}
	}
}

static uint8_t find_next(const struct bt_gatt_attr *attr, uint16_t handle,
			 void *user_data)
{
	struct bt_gatt_attr **next = user_data;

	*next = (struct bt_gatt_attr *)attr;

	return BT_GATT_ITER_STOP;
}

struct bt_gatt_attr *bt_gatt_attr_next(const struct bt_gatt_attr *attr)
{
	struct bt_gatt_attr *next = NULL;
	uint16_t handle = bt_gatt_attr_get_handle(attr);

	bt_gatt_foreach_attr(handle + 1, handle + 1, find_next, &next);

	return next;
}

uint16_t bt_gatt_attr_get_handle(const struct bt_gatt_attr *attr)
{
	if (attr == NULL) {
		return 0;
	}
	for (uint16_t i = 0; i <= RSI_BLE_MAX_NBR_ATT_REC; i++) {
		if (att_handle_table[i].attr != NULL &&
		    att_handle_table[i].attr == attr) {
			return att_handle_table[i].handle;
		}
	}
	return 0;
}

uint16_t bt_gatt_attr_value_handle(const struct bt_gatt_attr *attr)
{
	uint16_t handle = 0;

	if (attr != NULL && bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CHRC) == 0) {
		struct bt_gatt_chrc *chrc = attr->user_data;

		handle = chrc->value_handle;
		if (handle == 0) {
			/* Fall back to Zephyr value handle policy */
			handle = bt_gatt_attr_get_handle(attr) + 1U;
		}
	}

	return handle;
}

/**
 * @brief GATT Event Processor
 *
 */
void bt_gatt_process(void)
{
	k_sem_take(&gatt_evt_queue_sem, K_FOREVER);
	struct rsi_event_s *current_event = &gatt_event_queue[gatt_event_ptr];
	const struct bt_gatt_attr *curr_att;
	k_sem_give(&gatt_evt_queue_sem);

	while (current_event->event_type) {
		switch (current_event->event_type) {
		case RSI_EVT_READ: {
			curr_att = rsi_attr_handle_table_search(
				current_event->r.handle);
			if (curr_att == NULL) {
				rsi_ble_att_error_response(
					current_event->ew.dev_addr,
					current_event->r.handle, 0x0a,
					BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
				break;
			}
			if (!(curr_att->perm & BT_GATT_PERM_READ)) {
				rsi_ble_att_error_response(
					current_event->r.dev_addr,
					current_event->r.handle,
					(current_event->r.type ? 0x0c : 0x0a),
					BT_ATT_ERR_READ_NOT_PERMITTED);
				break;
			}
			uint8_t buf[RSI_DEV_ATT_LEN] = {0};

			struct bt_conn *selected_conn;

			bt_addr_le_t addr;

			memcpy(addr.a.val, current_event->r.dev_addr, 6);

			selected_conn = bt_conn_lookup_addr_le(0, &addr);

			if (!selected_conn) {
				BT_WARN("GATT read request: Unable to find "
					"connection");
				break;
			}

			ssize_t read_size;

			if (curr_att->read != NULL) {
				read_size = curr_att->read(
					selected_conn, curr_att, buf,
					sizeof(buf), current_event->r.offset);
				if ((long)read_size < 0L) {
					BT_ERR("Read Error %d\n", read_size);
				}
			} else {
				read_size = 0;
			}
			int32_t status;

			if (read_size < 0) {
				uint8_t err = -read_size;

				if (!current_event->r.type) {
					status = rsi_ble_att_error_response(
						current_event->r.dev_addr,
						current_event->r.handle, 0x0a,
						err);
				} else {
					status = rsi_ble_att_error_response(
						current_event->r.dev_addr,
						current_event->r.handle, 0x0c,
						err);
				}
			} else {
				uint16_t read_len = read_size;

				status = rsi_ble_gatt_read_response(
					current_event->r.dev_addr,
					current_event->r.type,
					current_event->r.handle,
					current_event->r.offset, read_len, buf);
			}

			if (status) {
				BT_ERR("Read Response Status: %d\n", status);
			}
			bt_conn_unref(selected_conn);
			break;
		}
		case RSI_EVT_WRITE: {
			bool response_required = true;
			ssize_t return_val;
			uint16_t handle =
				rsi_bytes2R_to_uint16(current_event->w.handle);
			curr_att = rsi_attr_handle_table_search(handle);
			const struct bt_gatt_attr *chrc_desc_att =
				rsi_attr_handle_table_search(handle - 1);
			if (chrc_desc_att && !bt_uuid_cmp(chrc_desc_att->uuid,
							  BT_UUID_GATT_CHRC)) {
				struct bt_gatt_chrc *chrc =
					chrc_desc_att->user_data;
				response_required =
					!(chrc->properties &
					  BT_GATT_CHRC_WRITE_WITHOUT_RESP);
			}
			if (curr_att == NULL) {
				rsi_ble_att_error_response(
					current_event->w.dev_addr, handle, 0x12,
					BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
				break;
			}
			if (!(curr_att->perm & BT_GATT_PERM_WRITE)) {
				rsi_ble_att_error_response(
					current_event->w.dev_addr, handle, 0x12,
					BT_ATT_ERR_WRITE_NOT_PERMITTED);
				break;
			}
			struct bt_conn *selected_conn;

			bt_addr_le_t addr;

			memcpy(addr.a.val, current_event->w.dev_addr, 6);

			selected_conn = bt_conn_lookup_addr_le(0, &addr);

			if (!selected_conn) {
				BT_WARN("GATT write request: Unable to find "
					"connection");
				break;
			}

			if (curr_att->write != NULL) {
				return_val = curr_att->write(
					selected_conn, curr_att,
					current_event->w.att_value,
					current_event->w.length, 0,
					response_required
						? 0
						: BT_GATT_WRITE_FLAG_CMD);
				if (return_val < 0) {
					BT_ERR("Write Error %d\n", return_val);
					rsi_ble_att_error_response(
						current_event->w.dev_addr,
						handle, 0x12, -return_val);
				} else {
					if (response_required) {
						rsi_ble_gatt_write_response(
							current_event->w
								.dev_addr,
							0);
					}
				}
			} else {
				if (response_required) {
					rsi_ble_gatt_write_response(
						current_event->w.dev_addr, 0);
				}
			}

			bt_conn_unref(selected_conn);
			break;
		}
		case RSI_EVT_MTU: {
			rsi_ble_mtu_exchange_event(current_event->addr.val,
						   BT_ATT_MTU);
			break;
		}
		case RSI_EVT_PREP_WRITE: {
#if CONFIG_BT_ATT_PREPARE_COUNT > 0
			ssize_t return_val;
			uint16_t handle = rsi_bytes2R_to_uint16(
				current_event->pw->handle);
			if (current_event->pw == NULL) {
				rsi_ble_att_error_response(
					current_event->pw->dev_addr, handle,
					0x16, BT_ATT_ERR_PREPARE_QUEUE_FULL);
				break;
			}
			curr_att = rsi_attr_handle_table_search(handle);
			if (curr_att == NULL) {
				rsi_ble_att_error_response(
					current_event->pw->dev_addr, handle,
					0x16, BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
				prepared_writes_count--;
				break;
			}
			if (!(curr_att->perm & BT_GATT_PERM_WRITE)) {
				rsi_ble_att_error_response(
					current_event->pw->dev_addr, handle,
					0x16, BT_ATT_ERR_WRITE_NOT_PERMITTED);
				prepared_writes_count--;
				break;
			} else if (!(curr_att->perm &
				     BT_GATT_PERM_PREPARE_WRITE)) {
				rsi_ble_gatt_prepare_write_response(
					current_event->pw->dev_addr, handle,
					rsi_bytes2R_to_uint16(
						current_event->pw->offset),
					current_event->pw->length,
					current_event->pw->att_value);
				break;
			}
			struct bt_conn *selected_conn;

			bt_addr_le_t addr;

			memcpy(addr.a.val, current_event->pw->dev_addr, 6);

			selected_conn = bt_conn_lookup_addr_le(0, &addr);

			if (!selected_conn) {
				BT_WARN("GATT prepare write request: Unable to "
					"find connection");
				break;
			}

			if (curr_att->write != NULL) {
				return_val = curr_att->write(
					selected_conn, curr_att,
					current_event->pw->att_value,
					current_event->pw->length,
					rsi_bytes2R_to_uint16(
						current_event->pw->offset),
					BT_GATT_WRITE_FLAG_PREPARE);
				if (return_val < 0) {
					BT_ERR("Prepare Write Error %ld\n",
					       return_val);
					rsi_ble_att_error_response(
						current_event->pw->dev_addr,
						handle, 0x16, -return_val);
					prepared_writes_count--;
				} else {
					rsi_ble_gatt_prepare_write_response(
						current_event->pw->dev_addr,
						handle,
						rsi_bytes2R_to_uint16(
							current_event->pw
								->offset),
						current_event->pw->length,
						current_event->pw->att_value);
				}
			} else {
				rsi_ble_gatt_prepare_write_response(
					current_event->pw->dev_addr, handle,
					rsi_bytes2R_to_uint16(
						current_event->pw->offset),
					current_event->pw->length,
					current_event->pw->att_value);
			}

			bt_conn_unref(selected_conn);
#else
			rsi_ble_att_error_response(current_event->pw.addr.val,
						   current_event->pw.handle,
						   0x16,
						   BT_ATT_ERR_NOT_SUPPORTED);
#endif
		}
		case RSI_EVT_EXEC_WRITE: {
#if CONFIG_BT_ATT_PREPARE_COUNT > 0
			ssize_t return_val;
			int count = 0;

			rsi_ble_req_prepare_write_t
				*pw_conn[CONFIG_BT_ATT_PREPARE_COUNT] = {0};
			rsi_ble_req_prepare_write_t *pw_curr = NULL;

			for (int i = prepared_writes_count - 1; i >= 0; i--) {
				if (!memcmp(prepared_writes[i].dev_addr,
					    current_event->ew.dev_addr, 6)) {
					pw_curr = &prepared_writes[i];
					pw_conn[count] = pw_curr;
					count++;
				}
			}
			if (pw_curr == NULL) {
				rsi_ble_att_error_response(
					current_event->ew.dev_addr, 0, 0x18,
					BT_ATT_ERR_WRITE_NOT_PERMITTED);
				break;
			}
			struct bt_conn *selected_conn;

			while (count > 0) {
				uint16_t handle =
					rsi_bytes2R_to_uint16(pw_curr->handle);
				curr_att = rsi_attr_handle_table_search(handle);
				if (curr_att == NULL) {
					rsi_ble_att_error_response(
						current_event->ew.dev_addr,
						handle, 0x18,
						BT_ATT_ERR_ATTRIBUTE_NOT_FOUND);
					break;
				}
				if (!(curr_att->perm & BT_GATT_PERM_WRITE)) {
					rsi_ble_att_error_response(
						current_event->ew.dev_addr,
						handle, 0x18,
						BT_ATT_ERR_WRITE_NOT_PERMITTED);
					break;
				}

				bt_addr_le_t addr;

				memcpy(addr.a.val, current_event->ew.dev_addr,
				       6);

				selected_conn =
					bt_conn_lookup_addr_le(0, &addr);

				if (!selected_conn) {
					BT_WARN("GATT execute write request: "
						"Unable to find connection");
					break;
				}

				if (curr_att->write != NULL) {
					return_val = curr_att->write(
						selected_conn, curr_att,
						pw_curr->att_value,
						pw_curr->length,
						rsi_bytes2R_to_uint16(
							pw_curr->offset),
						0);
					if (return_val < 0) {
						BT_ERR("Execute Write Error "
						       "%ld\n",
						       return_val);
						rsi_ble_att_error_response(
							pw_curr->dev_addr,
							handle, 0x18,
							-return_val);
					}
				}
				count--;
				if (count > 0) {
					pw_curr = pw_conn[count - 1];
				} else {
					rsi_ble_gatt_write_response(
						pw_curr->dev_addr, 1);
				}
			}

			if (selected_conn) {
				bt_conn_unref(selected_conn);
			}
#else
			rsi_ble_att_error_response(current_event->ew.dev_addr,
						   last_pw_handle, 0x18,
						   BT_ATT_ERR_NOT_SUPPORTED);
#endif
		}
		default:
			break;
		}
		k_sem_take(&gatt_evt_queue_sem, K_FOREVER);
		current_event->event_type = RSI_EVT_NONE;
		gatt_event_ptr--;
		gatt_event_ptr %= ARRAY_SIZE(gatt_event_queue);
		current_event = &gatt_event_queue[gatt_event_ptr];
		k_sem_give(&gatt_evt_queue_sem);
	}
}

struct notify_data {
	const struct bt_gatt_attr *attr;
	uint16_t handle;
	int err;
	uint16_t type;
	union {
		struct bt_gatt_notify_params *nfy_params;
		struct bt_gatt_indicate_params *ind_params;
	};
};

/**
 * @brief Sends notification signal to a device
 *
 * @param conn Device connection object
 * @param handle Attribute to notify
 * @param params Notification parameters
 * @return status
 */
static int gatt_notify(struct bt_conn *conn, uint16_t handle,
		       struct bt_gatt_notify_params *params)
{
	int err = 0;

	do {
		if (err == -31) {
			k_usleep(conn->le.interval_min * 1250);
		}
		err = rsi_ble_notify_value(conn->le.dst.a.val, handle,
					   params->len,
					   (uint8_t *)params->data);
	} while (err == -31);
	return err;
}

/**
 * @brief Sends indication signal to a device
 *
 * @param conn Device connection object
 * @param handle Attribute to notify
 * @param params Indication parameters
 * @return status
 */
static int gatt_indicate(struct bt_conn *conn, uint16_t handle,
			 struct bt_gatt_indicate_params *params)
{
	int err = 0;

	do {
		if (err == -31) {
			k_usleep(conn->le.interval_min * 1250);
		}
		err = rsi_ble_indicate_value(conn->le.dst.a.val, handle,
					     params->len,
					     (uint8_t *)params->data);
	} while (err == -31);
	return err;
}

/**
 * @brief Callback for sending notifications
 *
 * @param attr Attribute to notifiy
 * @param handle Attribute handler
 * @param user_data Data to send
 * @return status
 */
static uint8_t notify_cb(const struct bt_gatt_attr *attr, uint16_t handle,
			 void *user_data)
{
	struct notify_data *data = user_data;
	struct _bt_gatt_ccc *ccc;
	size_t i;

	/* Check attribute user_data must be of type struct _bt_gatt_ccc */
	if (attr->write != bt_gatt_attr_write_ccc) {
		return BT_GATT_ITER_CONTINUE;
	}

	ccc = attr->user_data;

	/* Notify all peers configured */
	for (i = 0; i < ARRAY_SIZE(ccc->cfg); i++) {
		struct bt_gatt_ccc_cfg *cfg = &ccc->cfg[i];
		struct bt_conn *conn;
		int err;

		/* Check if config value matches data type since consolidated
		 * value may be for a different peer.
		 */
		if (cfg->value != data->type) {
			continue;
		}

		conn = bt_conn_lookup_addr_le(cfg->id, &cfg->peer);

		if (!conn) {
			continue;
		}

		if (conn->state != BT_CONN_CONNECTED) {
			bt_conn_unref(conn);
			continue;
		}

		/* Confirm match if cfg is managed by application */
		if (ccc->cfg_match && !ccc->cfg_match(conn, attr)) {
			bt_conn_unref(conn);
			continue;
		}

		/* Use the Characteristic Value handle discovered since the
		 * Client Characteristic Configuration descriptor may occur
		 * in any position within the characteristic definition after
		 * the Characteristic Value.
		 */
		if (data->type == BT_GATT_CCC_INDICATE) {
			err = gatt_indicate(conn, data->handle,
					    data->ind_params);
			if (err == 0) {
				data->ind_params->_ref++;
			}
		} else {
			err = gatt_notify(conn, data->handle, data->nfy_params);
		}

		bt_conn_unref(conn);

		if (err < 0) {
			return BT_GATT_ITER_STOP;
		}

		data->err = 0;
	}

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t match_uuid(const struct bt_gatt_attr *attr, uint16_t handle,
			  void *user_data)
{
	struct notify_data *data = user_data;

	data->attr = attr;
	data->handle = bt_gatt_attr_get_handle(data->attr);

	return BT_GATT_ITER_STOP;
}

static bool gatt_find_by_uuid(struct notify_data *found,
			      const struct bt_uuid *uuid)
{
	found->attr = NULL;

	bt_gatt_foreach_attr_type(found->handle, 0xffff, uuid, NULL, 1,
				  match_uuid, found);

	return found->attr ? true : false;
}

int bt_gatt_indicate(struct bt_conn *conn,
		     struct bt_gatt_indicate_params *params)
{
	struct notify_data data;

	__ASSERT(params, "invalid parameters\n");
	__ASSERT(params->attr, "invalid parameters\n");

	if (!atomic_test_bit(bt_dev_flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	data.attr = params->attr;

	if (conn && conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	data.handle = bt_gatt_attr_get_handle(data.attr);
	if (!data.handle) {
		return -ENOENT;
	}

	/* Lookup UUID if it was given */
	if (params->uuid) {
		if (!gatt_find_by_uuid(&data, params->uuid)) {
			return -ENOENT;
		}
	}

	/* Check if attribute is a characteristic then adjust the handle */
	if (!bt_uuid_cmp(data.attr->uuid, BT_UUID_GATT_CHRC)) {
		struct bt_gatt_chrc *chrc = data.attr->user_data;

		if (!(chrc->properties & BT_GATT_CHRC_INDICATE)) {
			return -EINVAL;
		}

		data.handle = bt_gatt_attr_value_handle(data.attr);
	}

	if (conn) {
		params->_ref = 1;
	}

	data.err = -ENOTCONN;
	data.type = BT_GATT_CCC_INDICATE;
	data.ind_params = params;

	params->_ref = 0;
	bt_gatt_foreach_attr_type(data.handle, 0xffff, BT_UUID_GATT_CCC, NULL,
				  1, notify_cb, &data);

	return data.err;
}

int bt_gatt_notify_cb(struct bt_conn *conn,
		      struct bt_gatt_notify_params *params)
{
	struct notify_data data;

	__ASSERT(params, "invalid parameters\n");
	__ASSERT(params->attr, "invalid parameters\n");

	if (!atomic_test_bit(bt_dev_flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	data.attr = params->attr;

	if (conn && !(conn->state == BT_CONN_CONNECTED || conn->state == BT_CONN_CONNECT_ADV)) {
		return -ENOTCONN;
	}

	data.handle = bt_gatt_attr_get_handle(data.attr);
	if (!data.handle) {
		return -ENOENT;
	}

	/* Lookup UUID if it was given */
	if (params->uuid) {
		if (!gatt_find_by_uuid(&data, params->uuid)) {
			return -ENOENT;
		}
	}

	/* Check if attribute is a characteristic then adjust the handle */
	if (!bt_uuid_cmp(data.attr->uuid, BT_UUID_GATT_CHRC)) {
		struct bt_gatt_chrc *chrc = data.attr->user_data;

		if (!(chrc->properties & BT_GATT_CHRC_NOTIFY)) {
			return -EINVAL;
		}

		data.handle = bt_gatt_attr_value_handle(data.attr);
	}

	if (conn) {
		return gatt_notify(conn, data.handle, params);
	}

	data.err = -ENOTCONN;
	data.type = BT_GATT_CCC_NOTIFY;
	data.nfy_params = params;

	bt_gatt_foreach_attr_type(data.handle, 0xffff, BT_UUID_GATT_CCC, NULL,
				  1, notify_cb, &data);

	return data.err;
}

bool bt_gatt_is_subscribed(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, uint16_t ccc_value)
{
	const struct _bt_gatt_ccc *ccc;

	__ASSERT(conn, "invalid parameter\n");
	__ASSERT(attr, "invalid parameter\n");

	if (conn->state != BT_CONN_CONNECTED) {
		return false;
	}

	/* Check if attribute is a characteristic declaration */
	if (!bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CHRC)) {
		struct bt_gatt_chrc *chrc = attr->user_data;

		if (!(chrc->properties &
		      (BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_INDICATE))) {
			/* Characteristic doesn't support subscription */
			return false;
		}

		attr = bt_gatt_attr_next(attr);
	}

	/* Check if attribute is a characteristic value */
	if (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CCC) != 0) {
		attr = bt_gatt_attr_next(attr);
	}

	/* Check if the attribute is the CCC Descriptor */
	if (bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CCC) != 0) {
		return false;
	}

	ccc = attr->user_data;

	/* Check if the connection is subscribed */
	for (size_t i = 0; i < BT_GATT_CCC_MAX; i++) {
		const struct bt_gatt_ccc_cfg *cfg = &ccc->cfg[i];

		if (bt_conn_is_peer_addr_le(conn, cfg->id, &cfg->peer) &&
		    (ccc_value & ccc->cfg[i].value)) {
			return true;
		}
	}

	return false;
}
