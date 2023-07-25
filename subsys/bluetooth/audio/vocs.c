/*  Bluetooth VOCS - Volume offset Control Service
 *
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>

#include <zephyr/device.h>
#include <zephyr/init.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/vocs.h>

#include "audio_internal.h"
#include "vocs_internal.h"
#include "zephyr/bluetooth/audio/audio.h"

#define LOG_LEVEL CONFIG_BT_VOCS_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_vocs);

#define VALID_VOCS_OPCODE(opcode)	((opcode) == BT_VOCS_OPCODE_SET_OFFSET)

#define BT_AUDIO_LOCATION_RFU (~BT_AUDIO_LOCATION_ANY)

#if defined(CONFIG_BT_VOCS)
static void offset_state_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static ssize_t read_offset_state(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				 void *buf, uint16_t len, uint16_t offset)
{
	struct bt_vocs *inst = BT_AUDIO_CHRC_USER_DATA(attr);

	LOG_DBG("offset %d, counter %u", inst->srv.state.offset, inst->srv.state.change_counter);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &inst->srv.state,
				 sizeof(inst->srv.state));
}

static void location_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

#endif /* CONFIG_BT_VOCS */

static ssize_t write_location(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	struct bt_vocs *inst = BT_AUDIO_CHRC_USER_DATA(attr);
	enum bt_audio_location new_location;

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(inst->srv.location)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	new_location = sys_get_le32(buf);
	if (new_location == BT_AUDIO_LOCATION_PROHIBITED ||
	    (new_location & BT_AUDIO_LOCATION_RFU) > 0) {
		LOG_DBG("Invalid location %u", new_location);

		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	if (new_location != inst->srv.location) {
		inst->srv.location = new_location;
		(void)bt_gatt_notify_uuid(NULL, BT_UUID_VOCS_LOCATION,
					  inst->srv.service_p->attrs,
					  &inst->srv.location,
					  sizeof(inst->srv.location));

		if (inst->srv.cb && inst->srv.cb->location) {
			inst->srv.cb->location(inst, 0, inst->srv.location);
		}
	}

	return len;
}

#if defined(CONFIG_BT_VOCS)
static ssize_t read_location(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     void *buf, uint16_t len, uint16_t offset)
{
	struct bt_vocs *inst = BT_AUDIO_CHRC_USER_DATA(attr);

	LOG_DBG("0x%08x", inst->srv.location);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &inst->srv.location,
				 sizeof(inst->srv.location));
}
#endif /* CONFIG_BT_VOCS */

static ssize_t write_vocs_control(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				  const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	struct bt_vocs *inst = BT_AUDIO_CHRC_USER_DATA(attr);
	const struct bt_vocs_control *cp = buf;
	bool notify = false;

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (!len || !buf) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	/* Check opcode before length */
	if (!VALID_VOCS_OPCODE(cp->opcode)) {
		LOG_DBG("Invalid opcode %u", cp->opcode);
		return BT_GATT_ERR(BT_VOCS_ERR_OP_NOT_SUPPORTED);
	}

	if (len != sizeof(struct bt_vocs_control)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	LOG_DBG("Opcode %u, counter %u", cp->opcode, cp->counter);


	if (cp->counter != inst->srv.state.change_counter) {
		return BT_GATT_ERR(BT_VOCS_ERR_INVALID_COUNTER);
	}

	switch (cp->opcode) {
	case BT_VOCS_OPCODE_SET_OFFSET:
		LOG_DBG("Set offset %d", cp->offset);
		if (cp->offset > BT_VOCS_MAX_OFFSET || cp->offset < BT_VOCS_MIN_OFFSET) {
			return BT_GATT_ERR(BT_VOCS_ERR_OUT_OF_RANGE);
		}

		if (inst->srv.state.offset != sys_le16_to_cpu(cp->offset)) {
			inst->srv.state.offset = sys_le16_to_cpu(cp->offset);
			notify = true;
		}
		break;
	default:
		return BT_GATT_ERR(BT_VOCS_ERR_OP_NOT_SUPPORTED);
	}

	if (notify) {
		inst->srv.state.change_counter++;
		LOG_DBG("New state: offset %d, counter %u", inst->srv.state.offset,
			inst->srv.state.change_counter);
		(void)bt_gatt_notify_uuid(NULL, BT_UUID_VOCS_STATE,
					  inst->srv.service_p->attrs,
					  &inst->srv.state, sizeof(inst->srv.state));

		if (inst->srv.cb && inst->srv.cb->state) {
			inst->srv.cb->state(inst, 0, inst->srv.state.offset);
		}

	}

	return len;
}

#if defined(CONFIG_BT_VOCS)
static void output_desc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}
#endif /* CONFIG_BT_VOCS */

static ssize_t write_output_desc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				 const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	struct bt_vocs *inst = BT_AUDIO_CHRC_USER_DATA(attr);

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len >= sizeof(inst->srv.output_desc)) {
		LOG_DBG("Output desc was clipped from length %u to %zu", len,
			sizeof(inst->srv.output_desc) - 1);
		/* We just clip the string value if it's too long */
		len = (uint16_t)sizeof(inst->srv.output_desc) - 1;
	}

	if (len != strlen(inst->srv.output_desc) || memcmp(buf, inst->srv.output_desc, len)) {
		memcpy(inst->srv.output_desc, buf, len);
		inst->srv.output_desc[len] = '\0';

		(void)bt_gatt_notify_uuid(NULL, BT_UUID_VOCS_DESCRIPTION,
					  inst->srv.service_p->attrs,
					  &inst->srv.output_desc,
					  strlen(inst->srv.output_desc));

		if (inst->srv.cb && inst->srv.cb->description) {
			inst->srv.cb->description(inst, 0, inst->srv.output_desc);
		}
	}

	LOG_DBG("%s", inst->srv.output_desc);

	return len;
}

static int vocs_write(struct bt_vocs *inst,
		      ssize_t (*write)(struct bt_conn *conn,
				       const struct bt_gatt_attr *attr,
				       const void *buf, uint16_t len,
				       uint16_t offset, uint8_t flags),
		      const void *buf, uint16_t len)
{
	struct bt_audio_attr_user_data user_data = {
		.user_data = inst,
	};
	struct bt_gatt_attr attr = {
		.user_data = &user_data,
	};
	int err;

	err = write(NULL, &attr, buf, len, 0, 0);
	if (err < 0) {
		return err;
	}

	return 0;
}

#if defined(CONFIG_BT_VOCS)
static ssize_t read_output_desc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				void *buf, uint16_t len, uint16_t offset)
{
	struct bt_vocs *inst = BT_AUDIO_CHRC_USER_DATA(attr);

	LOG_DBG("%s", inst->srv.output_desc);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &inst->srv.output_desc,
				 strlen(inst->srv.output_desc));
}

#define BT_VOCS_SERVICE_DEFINITION(_vocs) { \
	BT_GATT_SECONDARY_SERVICE(BT_UUID_VOCS), \
	BT_AUDIO_CHRC(BT_UUID_VOCS_STATE, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_offset_state, NULL, &_vocs), \
	BT_AUDIO_CCC(offset_state_cfg_changed), \
	BT_AUDIO_CHRC(BT_UUID_VOCS_LOCATION, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_location, write_location, &_vocs), \
	BT_AUDIO_CCC(location_cfg_changed), \
	BT_AUDIO_CHRC(BT_UUID_VOCS_CONTROL, \
		      BT_GATT_CHRC_WRITE, \
		      BT_GATT_PERM_WRITE_ENCRYPT, \
		      NULL, write_vocs_control, &_vocs), \
	BT_AUDIO_CHRC(BT_UUID_VOCS_DESCRIPTION, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_output_desc, write_output_desc, &_vocs), \
	BT_AUDIO_CCC(output_desc_cfg_changed) \
	}

static struct bt_vocs vocs_insts[CONFIG_BT_VOCS_MAX_INSTANCE_COUNT];
BT_GATT_SERVICE_INSTANCE_DEFINE(vocs_service_list, vocs_insts, CONFIG_BT_VOCS_MAX_INSTANCE_COUNT,
				BT_VOCS_SERVICE_DEFINITION);

struct bt_vocs *bt_vocs_free_instance_get(void)
{
	static uint32_t instance_cnt;

	if (instance_cnt >= CONFIG_BT_VOCS_MAX_INSTANCE_COUNT) {
		return NULL;
	}

	return &vocs_insts[instance_cnt++];
}

void *bt_vocs_svc_decl_get(struct bt_vocs *vocs)
{
	CHECKIF(!vocs) {
		LOG_DBG("Null VOCS pointer");
		return NULL;
	}

	return vocs->srv.service_p->attrs;
}

static void prepare_vocs_instances(void)
{
	for (int i = 0; i < ARRAY_SIZE(vocs_insts); i++) {
		vocs_insts[i].srv.service_p = &vocs_service_list[i];
	}
}

int bt_vocs_register(struct bt_vocs *vocs,
		     const struct bt_vocs_register_param *param)
{
	int err;
	struct bt_gatt_attr *attr;
	struct bt_gatt_chrc *chrc;
	static bool instances_prepared;

	CHECKIF(!vocs) {
		LOG_DBG("Null VOCS pointer");
		return -EINVAL;
	}

	CHECKIF(!param) {
		LOG_DBG("NULL params pointer");
		return -EINVAL;
	}

	if (!instances_prepared) {
		prepare_vocs_instances();
		instances_prepared = true;
	}

	CHECKIF(vocs->srv.initialized) {
		LOG_DBG("Already initialized VOCS instance");
		return -EALREADY;
	}

	CHECKIF(param->offset > BT_VOCS_MAX_OFFSET || param->offset < BT_VOCS_MIN_OFFSET) {
		LOG_DBG("Invalid offset %d", param->offset);
		return -EINVAL;
	}

	vocs->srv.location = param->location;
	vocs->srv.state.offset = param->offset;
	vocs->srv.cb = param->cb;

	if (param->output_desc) {
		strncpy(vocs->srv.output_desc, param->output_desc,
			sizeof(vocs->srv.output_desc) - 1);
		/* strncpy may not always null-terminate */
		vocs->srv.output_desc[sizeof(vocs->srv.output_desc) - 1] = '\0';
		if (IS_ENABLED(CONFIG_BT_VOCS_LOG_LEVEL_DBG) &&
		    strcmp(vocs->srv.output_desc, param->output_desc)) {
			LOG_DBG("Output desc clipped to %s", vocs->srv.output_desc);
		}
	}

	/* Iterate over the attributes in VOCS (starting from i = 1 to skip the service declaration)
	 * to find the BT_UUID_VOCS_DESCRIPTION or BT_UUID_VOCS_LOCATION and update the
	 * characteristic value (at [i]), update with the write permission and callback, and
	 * also update the characteristic declaration (always found at [i - 1]) with the
	 * BT_GATT_CHRC_WRITE_WITHOUT_RESP property.
	 */
	for (int i = 1; i < vocs->srv.service_p->attr_count; i++) {
		attr = &vocs->srv.service_p->attrs[i];

		if (param->location_writable && !bt_uuid_cmp(attr->uuid, BT_UUID_VOCS_LOCATION)) {
			/* Update attr and chrc to be writable */
			chrc = vocs->srv.service_p->attrs[i - 1].user_data;
			attr->perm |= BT_GATT_PERM_WRITE_ENCRYPT;
			chrc->properties |= BT_GATT_CHRC_WRITE_WITHOUT_RESP;
		} else if (param->desc_writable &&
			   !bt_uuid_cmp(attr->uuid, BT_UUID_VOCS_DESCRIPTION)) {
			/* Update attr and chrc to be writable */
			chrc = vocs->srv.service_p->attrs[i - 1].user_data;
			attr->perm |= BT_GATT_PERM_WRITE_ENCRYPT;
			chrc->properties |= BT_GATT_CHRC_WRITE_WITHOUT_RESP;
		}
	}

	err = bt_gatt_service_register(vocs->srv.service_p);
	if (err) {
		LOG_DBG("Could not register VOCS service");
		return err;
	}

	vocs->srv.initialized = true;
	return 0;
}
#endif /* CONFIG_BT_VOCS */

int bt_vocs_state_get(struct bt_vocs *inst)
{
	CHECKIF(!inst) {
		LOG_DBG("Null VOCS pointer");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VOCS_CLIENT) && inst->client_instance) {
		return bt_vocs_client_state_get(inst);
	} else if (IS_ENABLED(CONFIG_BT_VOCS) && !inst->client_instance) {
		if (inst->srv.cb && inst->srv.cb->state) {
			inst->srv.cb->state(inst, 0, inst->srv.state.offset);
		}
		return 0;
	}

	return -ENOTSUP;
}

int bt_vocs_location_get(struct bt_vocs *inst)
{
	CHECKIF(!inst) {
		LOG_DBG("Null VOCS pointer");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VOCS_CLIENT) && inst->client_instance) {
		return bt_vocs_client_location_get(inst);
	} else if (IS_ENABLED(CONFIG_BT_VOCS) && !inst->client_instance) {
		if (inst->srv.cb && inst->srv.cb->location) {
			inst->srv.cb->location(inst, 0, inst->srv.location);
		}
		return 0;
	}

	return -ENOTSUP;
}

int bt_vocs_location_set(struct bt_vocs *inst, uint32_t location)
{
	CHECKIF(!inst) {
		LOG_DBG("Null VOCS pointer");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VOCS_CLIENT) && inst->client_instance) {
		return bt_vocs_client_location_set(inst, location);
	} else if (IS_ENABLED(CONFIG_BT_VOCS) && !inst->client_instance) {
		return vocs_write(inst, write_location, &location, sizeof(location));
	}

	return -ENOTSUP;
}

int bt_vocs_state_set(struct bt_vocs *inst, int16_t offset)
{
	CHECKIF(!inst) {
		LOG_DBG("Null VOCS pointer");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VOCS_CLIENT) && inst->client_instance) {
		return bt_vocs_client_state_set(inst, offset);
	} else if (IS_ENABLED(CONFIG_BT_VOCS) && !inst->client_instance) {
		struct bt_vocs_control cp;

		cp.opcode = BT_VOCS_OPCODE_SET_OFFSET;
		cp.counter = inst->srv.state.change_counter;
		cp.offset = sys_cpu_to_le16(offset);

		return vocs_write(inst, write_vocs_control, &cp, sizeof(cp));
	}

	return -ENOTSUP;
}

int bt_vocs_description_get(struct bt_vocs *inst)
{
	CHECKIF(!inst) {
		LOG_DBG("Null VOCS pointer");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VOCS_CLIENT) && inst->client_instance) {
		return bt_vocs_client_description_get(inst);
	} else if (IS_ENABLED(CONFIG_BT_VOCS) && !inst->client_instance) {
		if (inst->srv.cb && inst->srv.cb->description) {
			inst->srv.cb->description(inst, 0, inst->srv.output_desc);
		}
		return 0;
	}

	return -ENOTSUP;
}

int bt_vocs_description_set(struct bt_vocs *inst, const char *description)
{
	CHECKIF(!inst) {
		LOG_DBG("Null VOCS pointer");
		return -EINVAL;
	}

	CHECKIF(!description) {
		LOG_DBG("Null description pointer");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VOCS_CLIENT) && inst->client_instance) {
		return bt_vocs_client_description_set(inst, description);
	} else if (IS_ENABLED(CONFIG_BT_VOCS) && !inst->client_instance) {
		return vocs_write(inst, write_output_desc, description, strlen(description));
	}

	return -ENOTSUP;
}
