/*  Bluetooth VCP */

/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2019-2020 Bose Corporation
 * Copyright (c) 2020-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/audio/aics.h>
#include <zephyr/bluetooth/audio/vcp.h>
#include <zephyr/bluetooth/audio/vocs.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys_clock.h>

#include "audio_internal.h"
#include "vcp_internal.h"

#define LOG_LEVEL CONFIG_BT_VCP_VOL_REND_LOG_LEVEL

LOG_MODULE_REGISTER(bt_vcp_vol_rend);

#define VOLUME_DOWN(current_vol) \
	((uint8_t)MAX(0, (int)current_vol - vol_rend.volume_step))
#define VOLUME_UP(current_vol) \
	((uint8_t)MIN(UINT8_MAX, (int)current_vol + vol_rend.volume_step))

#define VALID_VCP_OPCODE(opcode) ((opcode) <= BT_VCP_OPCODE_MUTE)

enum vol_rend_notify {
	NOTIFY_STATE,
	NOTIFY_FLAGS,
	NOTIFY_NUM,
};

struct bt_vcp_vol_rend {
	struct vcs_state state;
	uint8_t flags;
	struct bt_vcp_vol_rend_cb *cb;
	uint8_t volume_step;

	struct bt_gatt_service *service_p;
	struct bt_vocs *vocs_insts[CONFIG_BT_VCP_VOL_REND_VOCS_INSTANCE_COUNT];
	struct bt_aics *aics_insts[CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT];

	ATOMIC_DEFINE(notify, NOTIFY_NUM);
	struct k_work_delayable notify_work;
};

static struct bt_vcp_vol_rend vol_rend;

static void volume_state_cfg_changed(const struct bt_gatt_attr *attr,
				     uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static ssize_t read_vol_state(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	LOG_DBG("Volume %u, mute %u, counter %u",
		vol_rend.state.volume, vol_rend.state.mute,
		vol_rend.state.change_counter);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &vol_rend.state, sizeof(vol_rend.state));
}

static const char *vol_rend_notify_str(enum vol_rend_notify notify)
{
	switch (notify) {
	case NOTIFY_STATE:
		return "state";
	case NOTIFY_FLAGS:
		return "flags";
	default:
		return "unknown";
	}
}

static void notify_work_reschedule(struct bt_vcp_vol_rend *inst, enum vol_rend_notify notify,
				   k_timeout_t delay)
{
	int err;

	atomic_set_bit(inst->notify, notify);

	err = k_work_reschedule(&inst->notify_work, delay);
	if (err < 0) {
		LOG_ERR("Failed to reschedule %s notification err %d", vol_rend_notify_str(notify),
			err);
	} else if (!K_TIMEOUT_EQ(delay, K_NO_WAIT)) {
		LOG_DBG("%s notification scheduled in %dms", vol_rend_notify_str(notify),
			k_ticks_to_ms_floor32(k_work_delayable_remaining_get(&inst->notify_work)));
	}
}

static void notify(struct bt_vcp_vol_rend *inst, enum vol_rend_notify notify,
		   const struct bt_uuid *uuid, const void *data, uint16_t len)
{
	int err;

	err = bt_gatt_notify_uuid(NULL, uuid, inst->service_p->attrs, data, len);
	if (err == -ENOMEM) {
		notify_work_reschedule(inst, notify, K_USEC(BT_AUDIO_NOTIFY_RETRY_DELAY_US));
	} else if (err < 0 && err != -ENOTCONN) {
		LOG_ERR("Notify %s err %d", vol_rend_notify_str(notify), err);
	}
}

static void notify_work_handler(struct k_work *work)
{
	struct k_work_delayable *d_work = k_work_delayable_from_work(work);
	struct bt_vcp_vol_rend *inst = CONTAINER_OF(d_work, struct bt_vcp_vol_rend, notify_work);

	if (atomic_test_and_clear_bit(inst->notify, NOTIFY_STATE)) {
		notify(inst, NOTIFY_STATE, BT_UUID_VCS_STATE, &inst->state, sizeof(inst->state));
	}

	if (IS_ENABLED(CONFIG_BT_VCP_VOL_REND_VOL_FLAGS_NOTIFIABLE) &&
	    atomic_test_and_clear_bit(inst->notify, NOTIFY_FLAGS)) {
		notify(inst, NOTIFY_FLAGS, BT_UUID_VCS_FLAGS, &inst->flags, sizeof(inst->flags));
	}
}

static void value_changed(struct bt_vcp_vol_rend *inst, enum vol_rend_notify notify)
{
	notify_work_reschedule(inst, notify, K_NO_WAIT);
}

static ssize_t write_vcs_control(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 const void *buf, uint16_t len, uint16_t offset,
				 uint8_t flags)
{
	const struct vcs_control_vol *cp_val = buf;
	bool notify = false;
	bool volume_change = false;
	uint8_t opcode;

	if (offset > 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len == 0 || buf == NULL) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	/* Check opcode before length */
	if (!VALID_VCP_OPCODE(cp_val->cp.opcode)) {
		LOG_DBG("Invalid opcode %u", cp_val->cp.opcode);
		return BT_GATT_ERR(BT_VCP_ERR_OP_NOT_SUPPORTED);
	}

	if ((len < sizeof(struct vcs_control)) ||
	    (len == sizeof(struct vcs_control_vol) &&
	    cp_val->cp.opcode != BT_VCP_OPCODE_SET_ABS_VOL) ||
	    (len > sizeof(struct vcs_control_vol))) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	opcode = cp_val->cp.opcode;

	LOG_DBG("Opcode %u, counter %u", opcode, cp_val->cp.counter);

	if (cp_val->cp.counter != vol_rend.state.change_counter) {
		return BT_GATT_ERR(BT_VCP_ERR_INVALID_COUNTER);
	}

	switch (opcode) {
	case BT_VCP_OPCODE_REL_VOL_DOWN:
		LOG_DBG("Relative Volume Down (0x%x)", opcode);
		if (vol_rend.state.volume > 0) {
			vol_rend.state.volume = VOLUME_DOWN(vol_rend.state.volume);
			notify = true;
		}
		volume_change = true;
		break;
	case BT_VCP_OPCODE_REL_VOL_UP:
		LOG_DBG("Relative Volume Up (0x%x)", opcode);
		if (vol_rend.state.volume != UINT8_MAX) {
			vol_rend.state.volume = VOLUME_UP(vol_rend.state.volume);
			notify = true;
		}
		volume_change = true;
		break;
	case BT_VCP_OPCODE_UNMUTE_REL_VOL_DOWN:
		LOG_DBG("(Unmute) relative Volume Down (0x%x)", opcode);
		if (vol_rend.state.volume > 0) {
			vol_rend.state.volume = VOLUME_DOWN(vol_rend.state.volume);
			notify = true;
		}
		if (vol_rend.state.mute) {
			vol_rend.state.mute = BT_VCP_STATE_UNMUTED;
			notify = true;
		}
		volume_change = true;
		break;
	case BT_VCP_OPCODE_UNMUTE_REL_VOL_UP:
		LOG_DBG("(Unmute) relative Volume Up (0x%x)", opcode);
		if (vol_rend.state.volume != UINT8_MAX) {
			vol_rend.state.volume = VOLUME_UP(vol_rend.state.volume);
			notify = true;
		}
		if (vol_rend.state.mute) {
			vol_rend.state.mute = BT_VCP_STATE_UNMUTED;
			notify = true;
		}
		volume_change = true;
		break;
	case BT_VCP_OPCODE_SET_ABS_VOL:
		LOG_DBG("Set Absolute Volume (0x%x): Current volume %u",
			opcode, vol_rend.state.volume);
		if (vol_rend.state.volume != cp_val->volume) {
			vol_rend.state.volume = cp_val->volume;
			notify = true;
		}
		volume_change = true;
		break;
	case BT_VCP_OPCODE_UNMUTE:
		LOG_DBG("Unmute (0x%x)", opcode);
		if (vol_rend.state.mute) {
			vol_rend.state.mute = BT_VCP_STATE_UNMUTED;
			notify = true;
		}
		break;
	case BT_VCP_OPCODE_MUTE:
		LOG_DBG("Mute (0x%x)", opcode);
		if (vol_rend.state.mute == BT_VCP_STATE_UNMUTED) {
			vol_rend.state.mute = BT_VCP_STATE_MUTED;
			notify = true;
		}
		break;
	default:
		LOG_DBG("Unknown opcode (0x%x)", opcode);
		return BT_GATT_ERR(BT_VCP_ERR_OP_NOT_SUPPORTED);
	}

	if (notify) {
		vol_rend.state.change_counter++;
		LOG_DBG("New state: volume %u, mute %u, counter %u",
			vol_rend.state.volume, vol_rend.state.mute,
			vol_rend.state.change_counter);

		value_changed(&vol_rend, NOTIFY_STATE);

		if (vol_rend.cb && vol_rend.cb->state) {
			vol_rend.cb->state(0, vol_rend.state.volume,
					       vol_rend.state.mute);
		}
	}

	if (volume_change && !vol_rend.flags) {
		vol_rend.flags = 1;

		if (IS_ENABLED(CONFIG_BT_VCP_VOL_REND_VOL_FLAGS_NOTIFIABLE)) {
			value_changed(&vol_rend, NOTIFY_FLAGS);
		}

		if (vol_rend.cb && vol_rend.cb->flags) {
			vol_rend.cb->flags(0, vol_rend.flags);
		}
	}
	return len;
}

#if defined(CONFIG_BT_VCP_VOL_REND_VOL_FLAGS_NOTIFIABLE)
static void flags_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}
#endif /* CONFIG_BT_VCP_VOL_REND_VOL_FLAGS_NOTIFIABLE */

static ssize_t read_flags(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  void *buf, uint16_t len, uint16_t offset)
{
	LOG_DBG("0x%02x", vol_rend.flags);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &vol_rend.flags,
				 sizeof(vol_rend.flags));
}

#define DUMMY_INCLUDE(i, _) BT_GATT_INCLUDE_SERVICE(NULL),
#define VOCS_INCLUDES(cnt) LISTIFY(cnt, DUMMY_INCLUDE, ())
#define AICS_INCLUDES(cnt) LISTIFY(cnt, DUMMY_INCLUDE, ())

/* Volume Control Service GATT Attributes */
static struct bt_gatt_attr vcs_attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_VCS),
	VOCS_INCLUDES(CONFIG_BT_VCP_VOL_REND_VOCS_INSTANCE_COUNT)
	AICS_INCLUDES(CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT)
	BT_AUDIO_CHRC(BT_UUID_VCS_STATE,
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
		      BT_GATT_PERM_READ_ENCRYPT,
		      read_vol_state, NULL, NULL),
	BT_AUDIO_CCC(volume_state_cfg_changed),
	BT_AUDIO_CHRC(BT_UUID_VCS_CONTROL,
		      BT_GATT_CHRC_WRITE,
		      BT_GATT_PERM_WRITE_ENCRYPT,
		      NULL, write_vcs_control, NULL),
#if defined(CONFIG_BT_VCP_VOL_REND_VOL_FLAGS_NOTIFIABLE)
	BT_AUDIO_CHRC(BT_UUID_VCS_FLAGS,
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
		      BT_GATT_PERM_READ_ENCRYPT,
		      read_flags, NULL, NULL),
	BT_AUDIO_CCC(flags_cfg_changed)
#else
	BT_AUDIO_CHRC(BT_UUID_VCS_FLAGS,
		      BT_GATT_CHRC_READ,
		      BT_GATT_PERM_READ_ENCRYPT,
		      read_flags, NULL, NULL)
#endif /* CONFIG_BT_VCP_VOL_REND_VOL_FLAGS_NOTIFIABLE */
};

static struct bt_gatt_service vcs_svc;

static int prepare_vocs_inst(struct bt_vcp_vol_rend_register_param *param)
{
	int err;
	int j;
	int i;

	if (CONFIG_BT_VCP_VOL_REND_VOCS_INSTANCE_COUNT == 0) {
		return 0;
	}

	__ASSERT(param, "NULL param");

	for (j = 0, i = 0; i < ARRAY_SIZE(vcs_attrs); i++) {
		if (bt_uuid_cmp(vcs_attrs[i].uuid, BT_UUID_GATT_INCLUDE) == 0 &&
		    !vcs_attrs[i].user_data) {

			vol_rend.vocs_insts[j] = bt_vocs_free_instance_get();

			if (vol_rend.vocs_insts[j] == NULL) {
				LOG_ERR("Could not get free VOCS instances[%d]",
					j);
				return -ENOMEM;
			}

			err = bt_vocs_register(vol_rend.vocs_insts[j],
					       &param->vocs_param[j]);
			if (err != 0) {
				LOG_DBG("Could not register VOCS instance[%d]: %d",
					j, err);
				return err;
			}

			vcs_attrs[i].user_data = bt_vocs_svc_decl_get(vol_rend.vocs_insts[j]);
			j++;

			if (j == CONFIG_BT_VCP_VOL_REND_VOCS_INSTANCE_COUNT) {
				break;
			}
		}
	}

	__ASSERT(j == CONFIG_BT_VCP_VOL_REND_VOCS_INSTANCE_COUNT,
		 "Invalid VOCS instance count");

	return 0;
}

static int prepare_aics_inst(struct bt_vcp_vol_rend_register_param *param)
{
	int err;
	int j;
	int i;

	if (CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT == 0) {
		return 0;
	}

	__ASSERT(param, "NULL param");

	for (j = 0, i = 0; i < ARRAY_SIZE(vcs_attrs); i++) {
		if (bt_uuid_cmp(vcs_attrs[i].uuid, BT_UUID_GATT_INCLUDE) == 0 &&
		    !vcs_attrs[i].user_data) {
			vol_rend.aics_insts[j] = bt_aics_free_instance_get();

			if (vol_rend.aics_insts[j] == NULL) {
				LOG_ERR("Could not get free AICS instances[%d]",
					j);
				return -ENOMEM;
			}

			err = bt_aics_register(vol_rend.aics_insts[j],
					       &param->aics_param[j]);
			if (err != 0) {
				LOG_DBG("Could not register AICS instance[%d]: %d",
					j, err);
				return err;
			}

			vcs_attrs[i].user_data = bt_aics_svc_decl_get(vol_rend.aics_insts[j]);
			j++;

			LOG_DBG("AICS P %p", vcs_attrs[i].user_data);

			if (j == CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT) {
				break;
			}
		}
	}

	__ASSERT(j == CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT,
		 "Invalid AICS instance count");

	return 0;
}

/****************************** PUBLIC API ******************************/
int bt_vcp_vol_rend_register(struct bt_vcp_vol_rend_register_param *param)
{
	static bool registered;
	int err;

	CHECKIF(param == NULL) {
		LOG_DBG("param is NULL");
		return -EINVAL;
	}

	CHECKIF(param->mute > BT_VCP_STATE_MUTED) {
		LOG_DBG("Invalid mute value: %u", param->mute);
		return -EINVAL;
	}

	CHECKIF(param->step == 0) {
		LOG_DBG("Invalid step value: %u", param->step);
		return -EINVAL;
	}

	if (registered) {
		return -EALREADY;
	}

	vcs_svc = (struct bt_gatt_service)BT_GATT_SERVICE(vcs_attrs);

	if (CONFIG_BT_VCP_VOL_REND_VOCS_INSTANCE_COUNT > 0) {
		err = prepare_vocs_inst(param);

		if (err != 0) {
			return err;
		}
	}

	if (CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT > 0) {
		err = prepare_aics_inst(param);

		if (err != 0) {
			return err;
		}
	}


	vol_rend.state.volume = param->volume;
	vol_rend.state.mute = param->mute;
	vol_rend.volume_step = param->step;
	vol_rend.service_p = &vcs_svc;

	err = bt_gatt_service_register(&vcs_svc);
	if (err != 0) {
		LOG_DBG("VCP service register failed: %d", err);
	}

	vol_rend.cb = param->cb;

	atomic_clear(vol_rend.notify);
	k_work_init_delayable(&vol_rend.notify_work, notify_work_handler);

	registered = true;

	return err;
}

int bt_vcp_vol_rend_included_get(struct bt_vcp_included *included)
{
	if (included == NULL) {
		return -EINVAL;
	}

	included->vocs_cnt = ARRAY_SIZE(vol_rend.vocs_insts);
	included->vocs = vol_rend.vocs_insts;

	included->aics_cnt = ARRAY_SIZE(vol_rend.aics_insts);
	included->aics = vol_rend.aics_insts;

	return 0;
}

int bt_vcp_vol_rend_set_step(uint8_t volume_step)
{
	if (volume_step > 0) {
		vol_rend.volume_step = volume_step;
		return 0;
	} else {
		return -EINVAL;
	}
}

int bt_vcp_vol_rend_get_state(void)
{
	if (vol_rend.cb && vol_rend.cb->state) {
		vol_rend.cb->state(0, vol_rend.state.volume,
				   vol_rend.state.mute);
	}

	return 0;
}

int bt_vcp_vol_rend_get_flags(void)
{
	if (vol_rend.cb && vol_rend.cb->flags) {
		vol_rend.cb->flags(0, vol_rend.flags);
	}

	return 0;
}

int bt_vcp_vol_rend_vol_down(void)
{
	const struct vcs_control cp = {
		.opcode = BT_VCP_OPCODE_REL_VOL_DOWN,
		.counter = vol_rend.state.change_counter,
	};
	int err;

	err = write_vcs_control(NULL, NULL, &cp, sizeof(cp), 0, 0);

	return err > 0 ? 0 : err;
}

int bt_vcp_vol_rend_vol_up(void)
{
	const struct vcs_control cp = {
		.opcode = BT_VCP_OPCODE_REL_VOL_UP,
		.counter = vol_rend.state.change_counter,
	};
	int err;

	err = write_vcs_control(NULL, NULL, &cp, sizeof(cp), 0, 0);

	return err > 0 ? 0 : err;
}

int bt_vcp_vol_rend_unmute_vol_down(void)
{
	const struct vcs_control cp = {
		.opcode = BT_VCP_OPCODE_UNMUTE_REL_VOL_DOWN,
		.counter = vol_rend.state.change_counter,
	};
	int err;

	err = write_vcs_control(NULL, NULL, &cp, sizeof(cp), 0, 0);

	return err > 0 ? 0 : err;
}

int bt_vcp_vol_rend_unmute_vol_up(void)
{
	const struct vcs_control cp = {
		.opcode = BT_VCP_OPCODE_UNMUTE_REL_VOL_UP,
		.counter = vol_rend.state.change_counter,
	};
	int err;

	err = write_vcs_control(NULL, NULL, &cp, sizeof(cp), 0, 0);

	return err > 0 ? 0 : err;
}

int bt_vcp_vol_rend_set_vol(uint8_t volume)
{
	const struct vcs_control_vol cp = {
		.cp = {
			.opcode = BT_VCP_OPCODE_SET_ABS_VOL,
			.counter = vol_rend.state.change_counter
		},
		.volume = volume
	};
	int err;

	err = write_vcs_control(NULL, NULL, &cp, sizeof(cp), 0, 0);

	return err > 0 ? 0 : err;
}

int bt_vcp_vol_rend_unmute(void)
{
	const struct vcs_control cp = {
		.opcode = BT_VCP_OPCODE_UNMUTE,
		.counter = vol_rend.state.change_counter,
	};
	int err;

	err = write_vcs_control(NULL, NULL, &cp, sizeof(cp), 0, 0);

	return err > 0 ? 0 : err;
}

int bt_vcp_vol_rend_mute(void)
{
	const struct vcs_control cp = {
		.opcode = BT_VCP_OPCODE_MUTE,
		.counter = vol_rend.state.change_counter,
	};
	int err;

	err = write_vcs_control(NULL, NULL, &cp, sizeof(cp), 0, 0);

	return err > 0 ? 0 : err;
}
