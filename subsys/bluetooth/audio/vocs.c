/*  Bluetooth VOCS - Volume offset Control Service
 *
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/vocs.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys_clock.h>

#include "audio_internal.h"
#include "vocs_internal.h"

#define LOG_LEVEL CONFIG_BT_VOCS_LOG_LEVEL
LOG_MODULE_REGISTER(bt_vocs);

#define VALID_VOCS_OPCODE(opcode)	((opcode) == BT_VOCS_OPCODE_SET_OFFSET)

#define BT_AUDIO_LOCATION_RFU (~BT_AUDIO_LOCATION_ANY)

#if defined(CONFIG_BT_VOCS)
static void offset_state_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static k_ssize_t read_offset_state(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				 void *buf, uint16_t len, uint16_t offset)
{
	struct bt_vocs_server *inst = BT_AUDIO_CHRC_USER_DATA(attr);

	LOG_DBG("offset %d, counter %u", inst->state.offset, inst->state.change_counter);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &inst->state,
				 sizeof(inst->state));
}

static void location_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static const char *vocs_notify_str(enum bt_vocs_notify notify)
{
	switch (notify) {
	case NOTIFY_STATE:
		return "state";
	case NOTIFY_LOCATION:
		return "location";
	case NOTIFY_OUTPUT_DESC:
		return "output desc";
	default:
		return "unknown";
	}
}

static void notify_work_reschedule(struct bt_vocs_server *inst, enum bt_vocs_notify notify,
				   k_timeout_t delay)
{
	int err;

	atomic_set_bit(inst->notify, notify);

	err = k_work_reschedule(&inst->notify_work, K_NO_WAIT);
	if (err < 0) {
		LOG_ERR("Failed to reschedule %s notification err %d",
			vocs_notify_str(notify), err);
	}
}

static void notify(struct bt_vocs_server *inst, enum bt_vocs_notify notify,
		   const struct bt_uuid *uuid, const void *data, uint16_t len)
{
	int err;

	err = bt_gatt_notify_uuid(NULL, uuid, inst->service_p->attrs, data, len);
	if (err == -ENOMEM) {
		notify_work_reschedule(inst, notify, K_USEC(BT_AUDIO_NOTIFY_RETRY_DELAY_US));
	} else if (err < 0 && err != -ENOTCONN) {
		LOG_ERR("Notify %s err %d", vocs_notify_str(notify), err);
	}
}

static void notify_work_handler(struct k_work *work)
{
	struct k_work_delayable *d_work = k_work_delayable_from_work(work);
	struct bt_vocs_server *inst = CONTAINER_OF(d_work, struct bt_vocs_server, notify_work);

	if (atomic_test_and_clear_bit(inst->notify, NOTIFY_STATE)) {
		notify(inst, NOTIFY_STATE, BT_UUID_VOCS_STATE, &inst->state, sizeof(inst->state));
	}

	if (atomic_test_and_clear_bit(inst->notify, NOTIFY_LOCATION)) {
		notify(inst, NOTIFY_LOCATION, BT_UUID_VOCS_LOCATION, &inst->location,
		       sizeof(inst->location));
	}

	if (atomic_test_and_clear_bit(inst->notify, NOTIFY_OUTPUT_DESC)) {
		notify(inst, NOTIFY_OUTPUT_DESC, BT_UUID_VOCS_DESCRIPTION, &inst->output_desc,
		       strlen(inst->output_desc));
	}
}

static void value_changed(struct bt_vocs_server *inst, enum bt_vocs_notify notify)
{
	notify_work_reschedule(inst, notify, K_NO_WAIT);
}
#else
#define value_changed(...)
#endif /* CONFIG_BT_VOCS */

static k_ssize_t write_location(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	struct bt_vocs_server *inst = BT_AUDIO_CHRC_USER_DATA(attr);
	enum bt_audio_location new_location;

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(inst->location)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	new_location = sys_get_le32(buf);
	if ((new_location & BT_AUDIO_LOCATION_RFU) > 0) {
		LOG_DBG("Invalid location %u", new_location);

		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	if (new_location != inst->location) {
		inst->location = new_location;

		value_changed(inst, NOTIFY_LOCATION);

		if (inst->cb && inst->cb->location) {
			inst->cb->location(&inst->vocs, 0, inst->location);
		}
	}

	return len;
}

#if defined(CONFIG_BT_VOCS)
static k_ssize_t read_location(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     void *buf, uint16_t len, uint16_t offset)
{
	struct bt_vocs_server *inst = BT_AUDIO_CHRC_USER_DATA(attr);

	LOG_DBG("0x%08x", inst->location);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &inst->location,
				 sizeof(inst->location));
}
#endif /* CONFIG_BT_VOCS */

static k_ssize_t write_vocs_control(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				  const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	struct bt_vocs_server *inst = BT_AUDIO_CHRC_USER_DATA(attr);
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


	if (cp->counter != inst->state.change_counter) {
		return BT_GATT_ERR(BT_VOCS_ERR_INVALID_COUNTER);
	}

	switch (cp->opcode) {
	case BT_VOCS_OPCODE_SET_OFFSET:
		LOG_DBG("Set offset %d", cp->offset);
		if (cp->offset > BT_VOCS_MAX_OFFSET || cp->offset < BT_VOCS_MIN_OFFSET) {
			return BT_GATT_ERR(BT_VOCS_ERR_OUT_OF_RANGE);
		}

		if (inst->state.offset != sys_le16_to_cpu(cp->offset)) {
			inst->state.offset = sys_le16_to_cpu(cp->offset);
			notify = true;
		}
		break;
	default:
		return BT_GATT_ERR(BT_VOCS_ERR_OP_NOT_SUPPORTED);
	}

	if (notify) {
		inst->state.change_counter++;
		LOG_DBG("New state: offset %d, counter %u", inst->state.offset,
			inst->state.change_counter);

		value_changed(inst, NOTIFY_STATE);

		if (inst->cb && inst->cb->state) {
			inst->cb->state(&inst->vocs, 0, inst->state.offset);
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

static k_ssize_t write_output_desc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				 const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	struct bt_vocs_server *inst = BT_AUDIO_CHRC_USER_DATA(attr);

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len >= sizeof(inst->output_desc)) {
		LOG_DBG("Output desc was clipped from length %u to %zu", len,
			sizeof(inst->output_desc) - 1);
		/* We just clip the string value if it's too long */
		len = (uint16_t)sizeof(inst->output_desc) - 1;
	}

	if (len != strlen(inst->output_desc) || memcmp(buf, inst->output_desc, len)) {
		memcpy(inst->output_desc, buf, len);
		inst->output_desc[len] = '\0';

		value_changed(inst, NOTIFY_OUTPUT_DESC);

		if (inst->cb && inst->cb->description) {
			inst->cb->description(&inst->vocs, 0, inst->output_desc);
		}
	}

	LOG_DBG("%s", inst->output_desc);

	return len;
}

static int vocs_write(struct bt_vocs_server *inst,
		      k_ssize_t (*write)(struct bt_conn *conn,
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
static k_ssize_t read_output_desc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				void *buf, uint16_t len, uint16_t offset)
{
	struct bt_vocs_server *inst = BT_AUDIO_CHRC_USER_DATA(attr);

	LOG_DBG("%s", inst->output_desc);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &inst->output_desc,
				 strlen(inst->output_desc));
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

static struct bt_vocs_server vocs_insts[CONFIG_BT_VOCS_MAX_INSTANCE_COUNT];
BT_GATT_SERVICE_INSTANCE_DEFINE(vocs_service_list, vocs_insts, CONFIG_BT_VOCS_MAX_INSTANCE_COUNT,
				BT_VOCS_SERVICE_DEFINITION);

struct bt_vocs *bt_vocs_free_instance_get(void)
{
	static uint32_t instance_cnt;

	if (instance_cnt >= CONFIG_BT_VOCS_MAX_INSTANCE_COUNT) {
		return NULL;
	}

	return &vocs_insts[instance_cnt++].vocs;
}

void *bt_vocs_svc_decl_get(struct bt_vocs *vocs)
{
	struct bt_vocs_server *inst;

	CHECKIF(!vocs) {
		LOG_DBG("Null VOCS pointer");
		return NULL;
	}

	CHECKIF(vocs->client_instance) {
		LOG_DBG("vocs pointer shall be server instance");
		return NULL;
	}

	inst = CONTAINER_OF(vocs, struct bt_vocs_server, vocs);

	return inst->service_p->attrs;
}

static void prepare_vocs_instances(void)
{
	for (int i = 0; i < ARRAY_SIZE(vocs_insts); i++) {
		vocs_insts[i].service_p = &vocs_service_list[i];
	}
}

int bt_vocs_register(struct bt_vocs *vocs,
		     const struct bt_vocs_register_param *param)
{
	struct bt_vocs_server *inst;
	int err;
	struct bt_gatt_attr *attr;
	struct bt_gatt_chrc *chrc;
	static bool instances_prepared;

	CHECKIF(!vocs) {
		LOG_DBG("Null VOCS pointer");
		return -EINVAL;
	}

	CHECKIF(vocs->client_instance) {
		LOG_DBG("vocs pointer shall be server instance");
		return -EINVAL;
	}

	inst = CONTAINER_OF(vocs, struct bt_vocs_server, vocs);

	CHECKIF(!param) {
		LOG_DBG("NULL params pointer");
		return -EINVAL;
	}

	if (!instances_prepared) {
		prepare_vocs_instances();
		instances_prepared = true;
	}

	CHECKIF(inst->initialized) {
		LOG_DBG("Already initialized VOCS instance");
		return -EALREADY;
	}

	CHECKIF(param->offset > BT_VOCS_MAX_OFFSET || param->offset < BT_VOCS_MIN_OFFSET) {
		LOG_DBG("Invalid offset %d", param->offset);
		return -EINVAL;
	}

	inst->location = param->location;
	inst->state.offset = param->offset;
	inst->cb = param->cb;

	if (param->output_desc) {
		(void)utf8_lcpy(inst->output_desc, param->output_desc,
				sizeof(inst->output_desc));
		if (IS_ENABLED(CONFIG_BT_VOCS_LOG_LEVEL_DBG) &&
		    strcmp(inst->output_desc, param->output_desc)) {
			LOG_DBG("Output desc clipped to %s", inst->output_desc);
		}
	}

	/* Iterate over the attributes in VOCS (starting from i = 1 to skip the service declaration)
	 * to find the BT_UUID_VOCS_DESCRIPTION or BT_UUID_VOCS_LOCATION and update the
	 * characteristic value (at [i]), update with the write permission and callback, and
	 * also update the characteristic declaration (always found at [i - 1]) with the
	 * BT_GATT_CHRC_WRITE_WITHOUT_RESP property.
	 */
	for (int i = 1; i < inst->service_p->attr_count; i++) {
		attr = &inst->service_p->attrs[i];

		if (param->location_writable && !bt_uuid_cmp(attr->uuid, BT_UUID_VOCS_LOCATION)) {
			/* Update attr and chrc to be writable */
			chrc = inst->service_p->attrs[i - 1].user_data;
			attr->perm |= BT_GATT_PERM_WRITE_ENCRYPT;
			chrc->properties |= BT_GATT_CHRC_WRITE_WITHOUT_RESP;
		} else if (param->desc_writable &&
			   !bt_uuid_cmp(attr->uuid, BT_UUID_VOCS_DESCRIPTION)) {
			/* Update attr and chrc to be writable */
			chrc = inst->service_p->attrs[i - 1].user_data;
			attr->perm |= BT_GATT_PERM_WRITE_ENCRYPT;
			chrc->properties |= BT_GATT_CHRC_WRITE_WITHOUT_RESP;
		}
	}

	err = bt_gatt_service_register(inst->service_p);
	if (err) {
		LOG_DBG("Could not register VOCS service");
		return err;
	}

	atomic_clear(inst->notify);
	k_work_init_delayable(&inst->notify_work, notify_work_handler);

	inst->initialized = true;
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
		struct bt_vocs_client *cli = CONTAINER_OF(inst, struct bt_vocs_client, vocs);

		return bt_vocs_client_state_get(cli);
	} else if (IS_ENABLED(CONFIG_BT_VOCS) && !inst->client_instance) {
		struct bt_vocs_server *srv = CONTAINER_OF(inst, struct bt_vocs_server, vocs);

		if (srv->cb && srv->cb->state) {
			srv->cb->state(inst, 0, srv->state.offset);
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
		struct bt_vocs_client *cli = CONTAINER_OF(inst, struct bt_vocs_client, vocs);

		return bt_vocs_client_location_get(cli);
	} else if (IS_ENABLED(CONFIG_BT_VOCS) && !inst->client_instance) {
		struct bt_vocs_server *srv = CONTAINER_OF(inst, struct bt_vocs_server, vocs);

		if (srv->cb && srv->cb->location) {
			srv->cb->location(inst, 0, srv->location);
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
		struct bt_vocs_client *cli = CONTAINER_OF(inst, struct bt_vocs_client, vocs);

		return bt_vocs_client_location_set(cli, location);
	} else if (IS_ENABLED(CONFIG_BT_VOCS) && !inst->client_instance) {
		struct bt_vocs_server *srv = CONTAINER_OF(inst, struct bt_vocs_server, vocs);

		return vocs_write(srv, write_location, &location, sizeof(location));
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
		struct bt_vocs_client *cli = CONTAINER_OF(inst, struct bt_vocs_client, vocs);

		return bt_vocs_client_state_set(cli, offset);
	} else if (IS_ENABLED(CONFIG_BT_VOCS) && !inst->client_instance) {
		struct bt_vocs_server *srv = CONTAINER_OF(inst, struct bt_vocs_server, vocs);
		struct bt_vocs_control cp;

		cp.opcode = BT_VOCS_OPCODE_SET_OFFSET;
		cp.counter = srv->state.change_counter;
		cp.offset = sys_cpu_to_le16(offset);

		return vocs_write(srv, write_vocs_control, &cp, sizeof(cp));
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
		struct bt_vocs_client *cli = CONTAINER_OF(inst, struct bt_vocs_client, vocs);

		return bt_vocs_client_description_get(cli);
	} else if (IS_ENABLED(CONFIG_BT_VOCS) && !inst->client_instance) {
		struct bt_vocs_server *srv = CONTAINER_OF(inst, struct bt_vocs_server, vocs);

		if (srv->cb && srv->cb->description) {
			srv->cb->description(inst, 0, srv->output_desc);
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
		struct bt_vocs_client *cli = CONTAINER_OF(inst, struct bt_vocs_client, vocs);

		return bt_vocs_client_description_set(cli, description);
	} else if (IS_ENABLED(CONFIG_BT_VOCS) && !inst->client_instance) {
		struct bt_vocs_server *srv = CONTAINER_OF(inst, struct bt_vocs_server, vocs);

		return vocs_write(srv, write_output_desc, description, strlen(description));
	}

	return -ENOTSUP;
}
