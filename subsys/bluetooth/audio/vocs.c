/*  Bluetooth VOCS - Volume offset Control Service
 *
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/byteorder.h>
#include <sys/check.h>

#include <device.h>
#include <init.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/audio/vocs.h>

#include "vocs_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_VOCS)
#define LOG_MODULE_NAME bt_vocs
#include "common/log.h"

#define VALID_VOCS_OPCODE(opcode)	((opcode) == BT_VOCS_OPCODE_SET_OFFSET)

#if defined(CONFIG_BT_VOCS)
static void offset_state_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}

static ssize_t read_offset_state(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				 void *buf, uint16_t len, uint16_t offset)
{
	struct bt_vocs *inst = attr->user_data;

	BT_DBG("offset %d, counter %u", inst->srv.state.offset, inst->srv.state.change_counter);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &inst->srv.state,
				 sizeof(inst->srv.state));
}

static void location_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}

#endif /* CONFIG_BT_VOCS */

static ssize_t write_location(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	struct bt_vocs *inst = attr->user_data;
	uint32_t old_location = inst->srv.location;

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(inst->srv.location)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	memcpy(&inst->srv.location, buf, len);
	BT_DBG("%02x", inst->srv.location);

	if (old_location != inst->srv.location) {
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
	struct bt_vocs *inst = attr->user_data;

	BT_DBG("0x%08x", inst->srv.location);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &inst->srv.location,
				 sizeof(inst->srv.location));
}
#endif /* CONFIG_BT_VOCS */

static ssize_t write_vocs_control(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				  const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	struct bt_vocs *inst = attr->user_data;
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
		BT_DBG("Invalid opcode %u", cp->opcode);
		return BT_GATT_ERR(BT_VOCS_ERR_OP_NOT_SUPPORTED);
	}

	if (len != sizeof(struct bt_vocs_control)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	BT_DBG("Opcode %u, counter %u", cp->opcode, cp->counter);


	if (cp->counter != inst->srv.state.change_counter) {
		return BT_GATT_ERR(BT_VOCS_ERR_INVALID_COUNTER);
	}

	switch (cp->opcode) {
	case BT_VOCS_OPCODE_SET_OFFSET:
		BT_DBG("Set offset %d", cp->offset);
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
		BT_DBG("New state: offset %d, counter %u",
		       inst->srv.state.offset, inst->srv.state.change_counter);
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
	BT_DBG("value 0x%04x", value);
}
#endif /* CONFIG_BT_VOCS */

static ssize_t write_output_desc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				 const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	struct bt_vocs *inst = attr->user_data;

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len >= sizeof(inst->srv.output_desc)) {
		BT_DBG("Output desc was clipped from length %u to %zu",
		       len, sizeof(inst->srv.output_desc) - 1);
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

	BT_DBG("%s", log_strdup(inst->srv.output_desc));

	return len;
}

#if defined(CONFIG_BT_VOCS)
static ssize_t read_output_desc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				void *buf, uint16_t len, uint16_t offset)
{
	struct bt_vocs *inst = attr->user_data;

	BT_DBG("%s", log_strdup(inst->srv.output_desc));
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &inst->srv.output_desc,
				 strlen(inst->srv.output_desc));
}

#define BT_VOCS_SERVICE_DEFINITION(_vocs) { \
	BT_GATT_SECONDARY_SERVICE(BT_UUID_VOCS), \
	BT_GATT_CHARACTERISTIC(BT_UUID_VOCS_STATE, \
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
			       BT_GATT_PERM_READ_ENCRYPT, \
			       read_offset_state, NULL, &_vocs), \
	BT_GATT_CCC(offset_state_cfg_changed, \
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT), \
	BT_GATT_CHARACTERISTIC(BT_UUID_VOCS_LOCATION, \
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
			       BT_GATT_PERM_READ_ENCRYPT, \
			       read_location, NULL, &_vocs), \
	BT_GATT_CCC(location_cfg_changed, \
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT), \
	BT_GATT_CHARACTERISTIC(BT_UUID_VOCS_CONTROL, \
			       BT_GATT_CHRC_WRITE, \
			       BT_GATT_PERM_WRITE_ENCRYPT, \
			       NULL, write_vocs_control, &_vocs), \
	BT_GATT_CHARACTERISTIC(BT_UUID_VOCS_DESCRIPTION, \
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
			       BT_GATT_PERM_READ_ENCRYPT, \
			       read_output_desc, NULL, &_vocs), \
	BT_GATT_CCC(output_desc_cfg_changed, \
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE_ENCRYPT) \
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
		BT_DBG("Null VOCS pointer");
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
		BT_DBG("Null VOCS pointer");
		return -EINVAL;
	}

	CHECKIF(!param) {
		BT_DBG("NULL params pointer");
		return -EINVAL;
	}

	if (!instances_prepared) {
		prepare_vocs_instances();
		instances_prepared = true;
	}

	CHECKIF(vocs->srv.initialized) {
		BT_DBG("Already initialized VOCS instance");
		return -EALREADY;
	}

	CHECKIF(param->offset > BT_VOCS_MAX_OFFSET || param->offset < BT_VOCS_MIN_OFFSET) {
		BT_DBG("Invalid offset %d", param->offset);
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
		if (IS_ENABLED(CONFIG_BT_DEBUG_VOCS) &&
		    strcmp(vocs->srv.output_desc, param->output_desc)) {
			BT_DBG("Output desc clipped to %s", log_strdup(vocs->srv.output_desc));
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
			attr->write = write_location;
			attr->perm |= BT_GATT_PERM_WRITE_ENCRYPT;
			chrc->properties |= BT_GATT_CHRC_WRITE_WITHOUT_RESP;
		} else if (param->desc_writable &&
			   !bt_uuid_cmp(attr->uuid, BT_UUID_VOCS_DESCRIPTION)) {
			/* Update attr and chrc to be writable */
			chrc = vocs->srv.service_p->attrs[i - 1].user_data;
			attr->write = write_output_desc;
			attr->perm |= BT_GATT_PERM_WRITE_ENCRYPT;
			chrc->properties |= BT_GATT_CHRC_WRITE_WITHOUT_RESP;
		}
	}

	err = bt_gatt_service_register(vocs->srv.service_p);
	if (err) {
		BT_DBG("Could not register VOCS service");
		return err;
	}

	vocs->srv.initialized = true;
	return 0;
}
#endif /* CONFIG_BT_VOCS */

#if defined(CONFIG_BT_VOCS) || defined(CONFIG_BT_VOCS_CLIENT)

int bt_vocs_state_get(struct bt_vocs *inst)
{
	CHECKIF(!inst) {
		BT_DBG("Null VOCS pointer");
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
		BT_DBG("Null VOCS pointer");
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
		BT_DBG("Null VOCS pointer");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VOCS_CLIENT) && inst->client_instance) {
		return bt_vocs_client_location_set(inst, location);
	} else if (IS_ENABLED(CONFIG_BT_VOCS) && !inst->client_instance) {
		struct bt_gatt_attr attr;
		int err;

		attr.user_data = inst;

		err = write_location(NULL, &attr, &location, sizeof(location), 0, 0);

		return err > 0 ? 0 : err;
	}

	return -ENOTSUP;
}

int bt_vocs_state_set(struct bt_vocs *inst, int16_t offset)
{
	CHECKIF(!inst) {
		BT_DBG("Null VOCS pointer");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VOCS_CLIENT) && inst->client_instance) {
		return bt_vocs_client_state_set(inst, offset);
	} else if (IS_ENABLED(CONFIG_BT_VOCS) && !inst->client_instance) {
		struct bt_gatt_attr attr;
		struct bt_vocs_control cp;
		int err;

		cp.opcode = BT_VOCS_OPCODE_SET_OFFSET;
		cp.counter = inst->srv.state.change_counter;
		cp.offset = sys_cpu_to_le16(offset);

		attr.user_data = inst;

		err = write_vocs_control(NULL, &attr, &cp, sizeof(cp), 0, 0);

		return err > 0 ? 0 : err;
	}

	return -ENOTSUP;
}

int bt_vocs_description_get(struct bt_vocs *inst)
{
	CHECKIF(!inst) {
		BT_DBG("Null VOCS pointer");
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
		BT_DBG("Null VOCS pointer");
		return -EINVAL;
	}

	CHECKIF(!description) {
		BT_DBG("Null description pointer");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VOCS_CLIENT) && inst->client_instance) {
		return bt_vocs_client_description_set(inst, description);
	} else if (IS_ENABLED(CONFIG_BT_VOCS) && !inst->client_instance) {
		struct bt_gatt_attr attr;
		int err;

		attr.user_data = inst;

		err = write_output_desc(NULL, &attr, description, strlen(description), 0, 0);
		return err > 0 ? 0 : err;
	}

	return -ENOTSUP;
}

#endif /* CONFIG_BT_VOCS || CONFIG_BT_VOCS_CLIENT */
