/*
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2020 Nordic Semiconductor ASA
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
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/aics.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys_clock.h>

#include "aics_internal.h"
#include "audio_internal.h"

#define LOG_LEVEL CONFIG_BT_AICS_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_aics);

#define VALID_AICS_OPCODE(opcode)                                              \
	((opcode) >= BT_AICS_OPCODE_SET_GAIN && (opcode) <= BT_AICS_OPCODE_SET_AUTO)

#define AICS_CP_LEN                 0x02
#define AICS_CP_SET_GAIN_LEN        0x03


static ssize_t write_description(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 const void *buf, uint16_t len, uint16_t offset,
				 uint8_t flags);

static ssize_t write_aics_control(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  const void *buf, uint16_t len,
				  uint16_t offset, uint8_t flags);

#if defined(CONFIG_BT_AICS)
static void aics_state_cfg_changed(const struct bt_gatt_attr *attr,
				   uint16_t value);
static ssize_t read_aics_state(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr,
			       void *buf, uint16_t len, uint16_t offset);
static ssize_t read_aics_gain_settings(struct bt_conn *conn,
				       const struct bt_gatt_attr *attr,
				       void *buf, uint16_t len,
				       uint16_t offset);
static ssize_t read_type(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset);
static void aics_input_status_cfg_changed(const struct bt_gatt_attr *attr,
					  uint16_t value);
static ssize_t read_input_status(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 void *buf, uint16_t len, uint16_t offset);
static void aics_description_cfg_changed(const struct bt_gatt_attr *attr,
					 uint16_t value);
static ssize_t read_description(struct bt_conn *conn,
				const struct bt_gatt_attr *attr, void *buf,
				uint16_t len, uint16_t offset);

#define BT_AICS_SERVICE_DEFINITION(_aics) {                                    \
	BT_GATT_SECONDARY_SERVICE(BT_UUID_AICS),                               \
	BT_AUDIO_CHRC(BT_UUID_AICS_STATE,                                      \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,                 \
		      BT_GATT_PERM_READ_ENCRYPT,                               \
		      read_aics_state, NULL, &_aics),                          \
	BT_AUDIO_CCC(aics_state_cfg_changed),                                  \
	BT_AUDIO_CHRC(BT_UUID_AICS_GAIN_SETTINGS,                              \
		      BT_GATT_CHRC_READ,                                       \
		      BT_GATT_PERM_READ_ENCRYPT,                               \
		      read_aics_gain_settings, NULL, &_aics),                  \
	BT_AUDIO_CHRC(BT_UUID_AICS_INPUT_TYPE,                                 \
		      BT_GATT_CHRC_READ,                                       \
		      BT_GATT_PERM_READ_ENCRYPT,                               \
		      read_type, NULL, &_aics),                                \
	BT_AUDIO_CHRC(BT_UUID_AICS_INPUT_STATUS,                               \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,                 \
		      BT_GATT_PERM_READ_ENCRYPT,                               \
		      read_input_status, NULL, &_aics),                        \
	BT_AUDIO_CCC(aics_input_status_cfg_changed),                           \
	BT_AUDIO_CHRC(BT_UUID_AICS_CONTROL,                                    \
		      BT_GATT_CHRC_WRITE,                                      \
		      BT_GATT_PERM_WRITE_ENCRYPT,                              \
		      NULL, write_aics_control, &_aics),                       \
	BT_AUDIO_CHRC(BT_UUID_AICS_DESCRIPTION,                                \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,                 \
		      BT_GATT_PERM_READ_ENCRYPT,                               \
		      read_description, write_description, &_aics),            \
	BT_AUDIO_CCC(aics_description_cfg_changed)                             \
	}


static struct bt_aics aics_insts[CONFIG_BT_AICS_MAX_INSTANCE_COUNT];
static uint32_t instance_cnt;
BT_GATT_SERVICE_INSTANCE_DEFINE(aics_service_list, aics_insts,
				CONFIG_BT_AICS_MAX_INSTANCE_COUNT,
				BT_AICS_SERVICE_DEFINITION);

static void aics_state_cfg_changed(const struct bt_gatt_attr *attr,
				   uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static ssize_t read_aics_state(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, void *buf,
			       uint16_t len, uint16_t offset)
{
	struct bt_aics *inst = BT_AUDIO_CHRC_USER_DATA(attr);

	LOG_DBG("gain %d, mute %u, gain_mode %u, counter %u", inst->srv.state.gain,
		inst->srv.state.mute, inst->srv.state.gain_mode, inst->srv.state.change_counter);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &inst->srv.state,
				 sizeof(inst->srv.state));
}

static ssize_t read_aics_gain_settings(struct bt_conn *conn,
				       const struct bt_gatt_attr *attr,
				       void *buf, uint16_t len, uint16_t offset)
{
	struct bt_aics *inst = BT_AUDIO_CHRC_USER_DATA(attr);

	LOG_DBG("units %u, min %d, max %d", inst->srv.gain_settings.units,
		inst->srv.gain_settings.minimum, inst->srv.gain_settings.maximum);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &inst->srv.gain_settings,
				 sizeof(inst->srv.gain_settings));
}

static ssize_t read_type(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	struct bt_aics *inst = BT_AUDIO_CHRC_USER_DATA(attr);

	LOG_DBG("%u", inst->srv.type);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &inst->srv.type,
				 sizeof(inst->srv.type));
}

static void aics_input_status_cfg_changed(const struct bt_gatt_attr *attr,
					  uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static ssize_t read_input_status(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr, void *buf,
				 uint16_t len, uint16_t offset)
{
	struct bt_aics *inst = BT_AUDIO_CHRC_USER_DATA(attr);

	LOG_DBG("%u", inst->srv.status);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &inst->srv.status,
				 sizeof(inst->srv.status));
}

static const char *aics_notify_str(enum bt_aics_notify notify)
{
	switch (notify) {
	case AICS_NOTIFY_STATE:
		return "state";
	case AICS_NOTIFY_DESCRIPTION:
		return "description";
	case AICS_NOTIFY_STATUS:
		return "status";
	default:
		return "unknown";
	}
}

static void notify_work_reschedule(struct bt_aics *inst, enum bt_aics_notify notify,
				   k_timeout_t delay)
{
	int err;

	atomic_set_bit(inst->srv.notify, notify);

	err = k_work_reschedule(&inst->srv.notify_work, K_NO_WAIT);
	if (err < 0) {
		LOG_ERR("Failed to reschedule %s notification err %d",
			aics_notify_str(notify), err);
	}
}

static void notify(struct bt_aics *inst, enum bt_aics_notify notify, const struct bt_uuid *uuid,
		   const void *data, uint16_t len)
{
	int err;

	err = bt_gatt_notify_uuid(NULL, uuid, inst->srv.service_p->attrs, data, len);
	if (err == -ENOMEM) {
		notify_work_reschedule(inst, notify, K_USEC(BT_AUDIO_NOTIFY_RETRY_DELAY_US));
	} else if (err < 0 && err != -ENOTCONN) {
		LOG_ERR("Notify %s err %d", aics_notify_str(notify), err);
	}
}

static void notify_work_handler(struct k_work *work)
{
	struct k_work_delayable *d_work = k_work_delayable_from_work(work);
	struct bt_aics *inst = CONTAINER_OF(d_work, struct bt_aics, srv.notify_work);

	if (atomic_test_and_clear_bit(inst->srv.notify, AICS_NOTIFY_STATE)) {
		notify(inst, AICS_NOTIFY_STATE, BT_UUID_AICS_STATE, &inst->srv.state,
		       sizeof(inst->srv.state));
	}

	if (atomic_test_and_clear_bit(inst->srv.notify, AICS_NOTIFY_DESCRIPTION)) {
		notify(inst, AICS_NOTIFY_DESCRIPTION, BT_UUID_AICS_DESCRIPTION,
		       &inst->srv.description, strlen(inst->srv.description));
	}

	if (atomic_test_and_clear_bit(inst->srv.notify, AICS_NOTIFY_STATUS)) {
		notify(inst, AICS_NOTIFY_STATUS, BT_UUID_AICS_INPUT_STATUS, &inst->srv.status,
		       sizeof(inst->srv.status));
	}
}

static void value_changed(struct bt_aics *inst, enum bt_aics_notify notify)
{
	notify_work_reschedule(inst, notify, K_NO_WAIT);
}
#else
#define value_changed(...)
#endif /* CONFIG_BT_AICS */

static ssize_t write_aics_control(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  const void *buf, uint16_t len,
				  uint16_t offset, uint8_t flags)
{
	struct bt_aics *inst = BT_AUDIO_CHRC_USER_DATA(attr);
	const struct bt_aics_gain_control *cp = buf;
	bool notify = false;

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (!len || !buf) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	/* Check opcode before length */
	if (!VALID_AICS_OPCODE(cp->cp.opcode)) {
		LOG_DBG("Invalid opcode %u", cp->cp.opcode);
		return BT_GATT_ERR(BT_AICS_ERR_OP_NOT_SUPPORTED);
	}

	if ((len < AICS_CP_LEN) ||
	    (len == AICS_CP_SET_GAIN_LEN && cp->cp.opcode != BT_AICS_OPCODE_SET_GAIN) ||
	    (len > AICS_CP_SET_GAIN_LEN)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	LOG_DBG("Opcode %u, counter %u", cp->cp.opcode, cp->cp.counter);
	if (cp->cp.counter != inst->srv.state.change_counter) {
		return BT_GATT_ERR(BT_AICS_ERR_INVALID_COUNTER);
	}

	switch (cp->cp.opcode) {
	case BT_AICS_OPCODE_SET_GAIN:
		LOG_DBG("Set gain %d", cp->gain_setting);
		if (cp->gain_setting < inst->srv.gain_settings.minimum ||
		    cp->gain_setting > inst->srv.gain_settings.maximum) {
			return BT_GATT_ERR(BT_AICS_ERR_OUT_OF_RANGE);
		}
		if (BT_AICS_INPUT_MODE_SETTABLE(inst->srv.state.gain_mode) &&
		    inst->srv.state.gain != cp->gain_setting) {
			inst->srv.state.gain = cp->gain_setting;
			notify = true;
		}
		break;
	case BT_AICS_OPCODE_UNMUTE:
		LOG_DBG("Unmute");
		if (inst->srv.state.mute == BT_AICS_STATE_MUTE_DISABLED) {
			return BT_GATT_ERR(BT_AICS_ERR_MUTE_DISABLED);
		}
		if (inst->srv.state.mute != BT_AICS_STATE_UNMUTED) {
			inst->srv.state.mute = BT_AICS_STATE_UNMUTED;
			notify = true;
		}
		break;
	case BT_AICS_OPCODE_MUTE:
		LOG_DBG("Mute");
		if (inst->srv.state.mute == BT_AICS_STATE_MUTE_DISABLED) {
			return BT_GATT_ERR(BT_AICS_ERR_MUTE_DISABLED);
		}
		if (inst->srv.state.mute != BT_AICS_STATE_MUTED) {
			inst->srv.state.mute = BT_AICS_STATE_MUTED;
			notify = true;
		}
		break;
	case BT_AICS_OPCODE_SET_MANUAL:
		LOG_DBG("Set manual mode");
		if (BT_AICS_INPUT_MODE_IMMUTABLE(inst->srv.state.gain_mode)) {
			return BT_GATT_ERR(BT_AICS_ERR_GAIN_MODE_NOT_ALLOWED);
		}
		if (inst->srv.state.gain_mode != BT_AICS_MODE_MANUAL) {
			inst->srv.state.gain_mode = BT_AICS_MODE_MANUAL;
			notify = true;
		}
		break;
	case BT_AICS_OPCODE_SET_AUTO:
		LOG_DBG("Set automatic mode");
		if (BT_AICS_INPUT_MODE_IMMUTABLE(inst->srv.state.gain_mode)) {
			return BT_GATT_ERR(BT_AICS_ERR_GAIN_MODE_NOT_ALLOWED);
		}
		if (inst->srv.state.gain_mode != BT_AICS_MODE_AUTO) {
			inst->srv.state.gain_mode = BT_AICS_MODE_AUTO;
			notify = true;
		}
		break;
	default:
		return BT_GATT_ERR(BT_AICS_ERR_OP_NOT_SUPPORTED);
	}

	if (notify) {
		inst->srv.state.change_counter++;

		LOG_DBG("New state: gain %d, mute %u, gain_mode %u, counter %u",
			inst->srv.state.gain, inst->srv.state.mute, inst->srv.state.gain_mode,
			inst->srv.state.change_counter);

		value_changed(inst, AICS_NOTIFY_STATE);

		if (inst->srv.cb && inst->srv.cb->state) {
			inst->srv.cb->state(inst, 0, inst->srv.state.gain,
					inst->srv.state.mute,
					inst->srv.state.gain_mode);
		} else {
			LOG_DBG("Callback not registered for instance %p", inst);
		}
	}

	return len;
}

#if defined(CONFIG_BT_AICS)
static void aics_description_cfg_changed(const struct bt_gatt_attr *attr,
					 uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}
#endif /* CONFIG_BT_AICS */

static ssize_t write_description(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 const void *buf, uint16_t len, uint16_t offset,
				 uint8_t flags)
{
	struct bt_aics *inst = BT_AUDIO_CHRC_USER_DATA(attr);

	if (len >= sizeof(inst->srv.description)) {
		LOG_DBG("Output desc was clipped from length %u to %zu", len,
			sizeof(inst->srv.description) - 1);
		/* We just clip the string value if it's too long */
		len = (uint16_t)sizeof(inst->srv.description) - 1;
	}

	if (memcmp(buf, inst->srv.description, len)) {
		memcpy(inst->srv.description, buf, len);
		inst->srv.description[len] = '\0';

		value_changed(inst, AICS_NOTIFY_DESCRIPTION);

		if (inst->srv.cb && inst->srv.cb->description) {
			inst->srv.cb->description(inst, 0,
						  inst->srv.description);
		} else {
			LOG_DBG("Callback not registered for instance %p", inst);
		}
	}

	LOG_DBG("%s", inst->srv.description);

	return len;
}

static int aics_write(struct bt_aics *inst,
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

#if defined(CONFIG_BT_AICS)
static ssize_t read_description(struct bt_conn *conn,
				const struct bt_gatt_attr *attr, void *buf,
				uint16_t len, uint16_t offset)
{
	struct bt_aics *inst = BT_AUDIO_CHRC_USER_DATA(attr);

	LOG_DBG("%s", inst->srv.description);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &inst->srv.description, strlen(inst->srv.description));
}

/************************ PUBLIC API ************************/
void *bt_aics_svc_decl_get(struct bt_aics *aics)
{
	CHECKIF(!aics) {
		LOG_DBG("NULL instance");
		return NULL;
	}

	return aics->srv.service_p->attrs;
}

static void prepare_aics_instances(void)
{
	for (int i = 0; i < ARRAY_SIZE(aics_insts); i++) {
		aics_insts[i].srv.service_p = &aics_service_list[i];
	}
}

int bt_aics_register(struct bt_aics *aics, struct bt_aics_register_param *param)
{
	int err;
	static bool instance_prepared;

	CHECKIF(!aics) {
		LOG_DBG("NULL aics pointer");
		return -ENOTCONN;
	}

	CHECKIF(!param) {
		LOG_DBG("NULL param");
		return -EINVAL;
	}

	if (!instance_prepared) {
		prepare_aics_instances();
		instance_prepared = true;
	}

	CHECKIF(aics->srv.initialized) {
		return -EALREADY;
	}

	CHECKIF(param->mute > BT_AICS_STATE_MUTE_DISABLED) {
		LOG_DBG("Invalid AICS mute value: %u", param->mute);
		return -EINVAL;
	}

	CHECKIF(param->gain_mode > BT_AICS_MODE_AUTO) {
		LOG_DBG("Invalid AICS mode value: %u", param->gain_mode);
		return -EINVAL;
	}

	CHECKIF(param->type > BT_AICS_INPUT_TYPE_STREAMING) {
		LOG_DBG("Invalid AICS input type value: %u", param->type);
		return -EINVAL;
	}

	CHECKIF(param->units == 0) {
		LOG_DBG("AICS units value shall not be 0");
		return -EINVAL;
	}

	CHECKIF(!(param->min_gain <= param->max_gain)) {
		LOG_DBG("AICS min gain (%d) shall be lower than or equal to max gain (%d)",
			param->min_gain, param->max_gain);
		return -EINVAL;
	}

	CHECKIF(param->gain < param->min_gain || param->gain > param->max_gain) {
		LOG_DBG("AICS gain (%d) shall be not lower than min gain (%d) "
		       "or higher than max gain (%d)",
		       param->gain, param->min_gain, param->max_gain);
		return -EINVAL;
	}

	aics->srv.state.gain = param->gain;
	aics->srv.state.mute = param->mute;
	aics->srv.state.gain_mode = param->gain_mode;
	aics->srv.gain_settings.units = param->units;
	aics->srv.gain_settings.minimum = param->min_gain;
	aics->srv.gain_settings.maximum = param->max_gain;
	aics->srv.type = param->type;
	aics->srv.status = param->status ? BT_AICS_STATUS_ACTIVE : BT_AICS_STATUS_INACTIVE;
	aics->srv.cb = param->cb;

	atomic_clear(aics->srv.notify);
	k_work_init_delayable(&aics->srv.notify_work, notify_work_handler);

	if (param->description) {
		(void)utf8_lcpy(aics->srv.description, param->description,
				sizeof(aics->srv.description));
		if (IS_ENABLED(CONFIG_BT_AICS_LOG_LEVEL_DBG) &&
		    strcmp(aics->srv.description, param->description)) {
			LOG_DBG("Input desc clipped to %s", aics->srv.description);
		}
	}

	/* Iterate over the attributes in AICS (starting from i = 1 to skip the
	 * service declaration) to find the BT_UUID_AICS_DESCRIPTION and update
	 * the characteristic value (at [i]), update that with the write
	 * permission and callback, and also update the characteristic
	 * declaration (always found at [i - 1]) with the
	 * BT_GATT_CHRC_WRITE_WITHOUT_RESP property.
	 */
	if (param->desc_writable) {
		for (int i = 1; i < aics->srv.service_p->attr_count; i++) {
			struct bt_gatt_attr *attr;

			attr = &aics->srv.service_p->attrs[i];

			if (!bt_uuid_cmp(attr->uuid, BT_UUID_AICS_DESCRIPTION)) {
				/* Update attr and chrc to be writable */
				struct bt_gatt_chrc *chrc;

				chrc = aics->srv.service_p->attrs[i - 1].user_data;
				attr->perm |= BT_GATT_PERM_WRITE_ENCRYPT;
				chrc->properties |= BT_GATT_CHRC_WRITE_WITHOUT_RESP;

				break;
			}
		}
	}

	err = bt_gatt_service_register(aics->srv.service_p);
	if (err) {
		LOG_DBG("Could not register AICS service");
		return err;
	}

	aics->srv.initialized = true;

	return 0;
}

struct bt_aics *bt_aics_free_instance_get(void)
{
	if (instance_cnt >= CONFIG_BT_AICS_MAX_INSTANCE_COUNT) {
		return NULL;
	}

	return (struct bt_aics *)&aics_insts[instance_cnt++];
}

/****************************** PUBLIC API ******************************/
int bt_aics_deactivate(struct bt_aics *inst)
{
	CHECKIF(!inst) {
		LOG_DBG("NULL instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_AICS_CLIENT) && inst->client_instance) {
		return -ENOTSUP;
	}

	if (inst->srv.status == BT_AICS_STATUS_ACTIVE) {
		inst->srv.status = BT_AICS_STATUS_INACTIVE;
		LOG_DBG("Instance %p: Status was set to inactive", inst);

		value_changed(inst, AICS_NOTIFY_STATUS);

		if (inst->srv.cb && inst->srv.cb->status) {
			inst->srv.cb->status(inst, 0, inst->srv.status);
		} else {
			LOG_DBG("Callback not registered for instance %p", inst);
		}
	}

	return 0;
}

int bt_aics_activate(struct bt_aics *inst)
{
	CHECKIF(!inst) {
		LOG_DBG("NULL instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_AICS_CLIENT) && inst->client_instance) {
		return -ENOTSUP;
	}

	if (inst->srv.status == BT_AICS_STATUS_INACTIVE) {
		inst->srv.status = BT_AICS_STATUS_ACTIVE;
		LOG_DBG("Instance %p: Status was set to active", inst);

		value_changed(inst, AICS_NOTIFY_STATUS);

		if (inst->srv.cb && inst->srv.cb->status) {
			inst->srv.cb->status(inst, 0, inst->srv.status);
		} else {
			LOG_DBG("Callback not registered for instance %p", inst);
		}
	}

	return 0;
}

#endif /* CONFIG_BT_AICS */
int bt_aics_gain_set_manual_only(struct bt_aics *inst)
{
	CHECKIF(!inst) {
		LOG_DBG("NULL instance");
		return -EINVAL;
	}

	inst->srv.state.gain_mode = BT_AICS_MODE_MANUAL_ONLY;

	value_changed(inst, AICS_NOTIFY_STATE);

	return 0;
}

int bt_aics_gain_set_auto_only(struct bt_aics *inst)
{
	CHECKIF(!inst) {
		LOG_DBG("NULL instance");
		return -EINVAL;
	}

	inst->srv.state.gain_mode = BT_AICS_MODE_AUTO_ONLY;

	value_changed(inst, AICS_NOTIFY_STATE);

	return 0;
}

int bt_aics_state_get(struct bt_aics *inst)
{
	CHECKIF(!inst) {
		LOG_DBG("NULL instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_AICS_CLIENT) && inst->client_instance) {
		return bt_aics_client_state_get(inst);
	} else if (IS_ENABLED(CONFIG_BT_AICS) && !inst->client_instance) {
		if (inst->srv.cb && inst->srv.cb->state) {
			inst->srv.cb->state(inst, 0, inst->srv.state.gain,
					    inst->srv.state.mute,
					    inst->srv.state.gain_mode);
		} else {
			LOG_DBG("Callback not registered for instance %p", inst);
		}
		return 0;
	}

	return -ENOTSUP;
}

int bt_aics_gain_setting_get(struct bt_aics *inst)
{
	CHECKIF(!inst) {
		LOG_DBG("NULL instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_AICS_CLIENT) && inst->client_instance) {
		return bt_aics_client_gain_setting_get(inst);
	} else if (IS_ENABLED(CONFIG_BT_AICS) && !inst->client_instance) {
		if (inst->srv.cb && inst->srv.cb->gain_setting) {
			inst->srv.cb->gain_setting(inst, 0,
						   inst->srv.gain_settings.units,
						   inst->srv.gain_settings.minimum,
						   inst->srv.gain_settings.maximum);
		} else {
			LOG_DBG("Callback not registered for instance %p", inst);
		}
		return 0;
	}

	return -ENOTSUP;
}

int bt_aics_type_get(struct bt_aics *inst)
{
	CHECKIF(!inst) {
		LOG_DBG("NULL instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_AICS_CLIENT) && inst->client_instance) {
		return bt_aics_client_type_get(inst);
	} else if (IS_ENABLED(CONFIG_BT_AICS) && !inst->client_instance) {
		if (inst->srv.cb && inst->srv.cb->type) {
			inst->srv.cb->type(inst, 0, inst->srv.type);
		} else {
			LOG_DBG("Callback not registered for instance %p", inst);
		}
		return 0;
	}

	return -ENOTSUP;
}

int bt_aics_status_get(struct bt_aics *inst)
{
	CHECKIF(!inst) {
		LOG_DBG("NULL instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_AICS_CLIENT) && inst->client_instance) {
		return bt_aics_client_status_get(inst);
	} else if (IS_ENABLED(CONFIG_BT_AICS) && !inst->client_instance) {
		if (inst->srv.cb && inst->srv.cb->status) {
			inst->srv.cb->status(inst, 0, inst->srv.status);
		} else {
			LOG_DBG("Callback not registered for instance %p", inst);
		}
		return 0;
	}

	return -ENOTSUP;
}

int bt_aics_disable_mute(struct bt_aics *inst)
{
	CHECKIF(!inst) {
		LOG_DBG("NULL instance");
		return -EINVAL;
	}

	inst->srv.state.mute = BT_AICS_STATE_MUTE_DISABLED;

	value_changed(inst, AICS_NOTIFY_STATE);

	return 0;
}

int bt_aics_unmute(struct bt_aics *inst)
{
	CHECKIF(!inst) {
		LOG_DBG("NULL instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_AICS_CLIENT) && inst->client_instance) {
		return bt_aics_client_unmute(inst);
	} else if (IS_ENABLED(CONFIG_BT_AICS) && !inst->client_instance) {
		struct bt_aics_control cp;

		cp.opcode = BT_AICS_OPCODE_UNMUTE;
		cp.counter = inst->srv.state.change_counter;

		return aics_write(inst, write_aics_control, &cp, sizeof(cp));
	}

	return -ENOTSUP;
}

int bt_aics_mute(struct bt_aics *inst)
{
	CHECKIF(!inst) {
		LOG_DBG("NULL instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_AICS_CLIENT) && inst->client_instance) {
		return bt_aics_client_mute(inst);
	} else if (IS_ENABLED(CONFIG_BT_AICS) && !inst->client_instance) {
		struct bt_aics_control cp;

		cp.opcode = BT_AICS_OPCODE_MUTE;
		cp.counter = inst->srv.state.change_counter;

		return aics_write(inst, write_aics_control, &cp, sizeof(cp));
	}

	return -ENOTSUP;
}

int bt_aics_manual_gain_set(struct bt_aics *inst)
{
	CHECKIF(!inst) {
		LOG_DBG("NULL instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_AICS_CLIENT) && inst->client_instance) {
		return bt_aics_client_manual_gain_set(inst);
	} else if (IS_ENABLED(CONFIG_BT_AICS) && !inst->client_instance) {
		struct bt_aics_control cp;

		cp.opcode = BT_AICS_OPCODE_SET_MANUAL;
		cp.counter = inst->srv.state.change_counter;

		return aics_write(inst, write_aics_control, &cp, sizeof(cp));
	}

	return -ENOTSUP;
}

int bt_aics_automatic_gain_set(struct bt_aics *inst)
{
	CHECKIF(!inst) {
		LOG_DBG("NULL instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_AICS_CLIENT) && inst->client_instance) {
		return bt_aics_client_automatic_gain_set(inst);
	} else if (IS_ENABLED(CONFIG_BT_AICS) && !inst->client_instance) {
		struct bt_aics_control cp;

		cp.opcode = BT_AICS_OPCODE_SET_AUTO;
		cp.counter = inst->srv.state.change_counter;

		return aics_write(inst, write_aics_control, &cp, sizeof(cp));
	}

	return -ENOTSUP;
}

int bt_aics_gain_set(struct bt_aics *inst, int8_t gain)
{
	CHECKIF(!inst) {
		LOG_DBG("NULL instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_AICS_CLIENT) && inst->client_instance) {
		return bt_aics_client_gain_set(inst, gain);
	} else if (IS_ENABLED(CONFIG_BT_AICS) && !inst->client_instance) {
		struct bt_aics_gain_control cp;

		cp.cp.opcode = BT_AICS_OPCODE_SET_GAIN;
		cp.cp.counter = inst->srv.state.change_counter;
		cp.gain_setting = gain;

		return aics_write(inst, write_aics_control, &cp, sizeof(cp));
	}

	return -ENOTSUP;
}

int bt_aics_description_get(struct bt_aics *inst)
{
	CHECKIF(!inst) {
		LOG_DBG("NULL instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_AICS_CLIENT) && inst->client_instance) {
		return bt_aics_client_description_get(inst);
	} else if (IS_ENABLED(CONFIG_BT_AICS) && !inst->client_instance) {
		if (inst->srv.cb && inst->srv.cb->description) {
			inst->srv.cb->description(inst, 0,
						  inst->srv.description);
		} else {
			LOG_DBG("Callback not registered for instance %p", inst);
		}
		return 0;
	}

	return -ENOTSUP;
}

int bt_aics_description_set(struct bt_aics *inst, const char *description)
{
	CHECKIF(!inst) {
		LOG_DBG("NULL instance");
		return -EINVAL;
	}

	CHECKIF(!description) {
		LOG_DBG("NULL description");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_AICS_CLIENT) && inst->client_instance) {
		return bt_aics_client_description_set(inst, description);
	} else if (IS_ENABLED(CONFIG_BT_AICS) && !inst->client_instance) {
		return aics_write(inst, write_description, description, strlen(description));
	}

	return -ENOTSUP;
}
