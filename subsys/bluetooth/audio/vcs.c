/*  Bluetooth VCS */

/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2019-2020 Bose Corporation
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
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
#include <zephyr/bluetooth/audio/vcs.h>

#include "audio_internal.h"
#include "vcs_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_VCS)
#define LOG_MODULE_NAME bt_vcs
#include "common/log.h"

static bool valid_vocs_inst(struct bt_vcs *vcs, struct bt_vocs *vocs)
{
	if (vocs == NULL) {
		return false;
	}

#if defined(CONFIG_BT_VCS)
	for (int i = 0; i < ARRAY_SIZE(vcs->srv.vocs_insts); i++) {
		if (vcs->srv.vocs_insts[i] == vocs) {
			return true;
		}
	}
#endif /* CONFIG_BT_VCS */

	return false;
}

static bool valid_aics_inst(struct bt_vcs *vcs, struct bt_aics *aics)
{
	if (aics == NULL) {
		return false;
	}

#if defined(CONFIG_BT_VCS)
	for (int i = 0; i < ARRAY_SIZE(vcs->srv.aics_insts); i++) {
		if (vcs->srv.aics_insts[i] == aics) {
			return true;
		}
	}
#endif /* CONFIG_BT_VCS */

	return false;
}

#if defined(CONFIG_BT_VCS)

#define VOLUME_DOWN(current_vol) \
	((uint8_t)MAX(0, (int)current_vol - vcs_inst.srv.volume_step))
#define VOLUME_UP(current_vol) \
	((uint8_t)MIN(UINT8_MAX, (int)current_vol + vcs_inst.srv.volume_step))

#define VALID_VCS_OPCODE(opcode) ((opcode) <= BT_VCS_OPCODE_MUTE)

static struct bt_vcs vcs_inst;

static void volume_state_cfg_changed(const struct bt_gatt_attr *attr,
				     uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}

static ssize_t read_vol_state(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, void *buf,
			      uint16_t len, uint16_t offset)
{
	BT_DBG("Volume %u, mute %u, counter %u",
	       vcs_inst.srv.state.volume, vcs_inst.srv.state.mute,
	       vcs_inst.srv.state.change_counter);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &vcs_inst.srv.state, sizeof(vcs_inst.srv.state));
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
	if (!VALID_VCS_OPCODE(cp_val->cp.opcode)) {
		BT_DBG("Invalid opcode %u", cp_val->cp.opcode);
		return BT_GATT_ERR(BT_VCS_ERR_OP_NOT_SUPPORTED);
	}

	if ((len < sizeof(struct vcs_control)) ||
	    (len == sizeof(struct vcs_control_vol) &&
	    cp_val->cp.opcode != BT_VCS_OPCODE_SET_ABS_VOL) ||
	    (len > sizeof(struct vcs_control_vol))) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	opcode = cp_val->cp.opcode;

	BT_DBG("Opcode %u, counter %u", opcode, cp_val->cp.counter);

	if (cp_val->cp.counter != vcs_inst.srv.state.change_counter) {
		return BT_GATT_ERR(BT_VCS_ERR_INVALID_COUNTER);
	}

	switch (opcode) {
	case BT_VCS_OPCODE_REL_VOL_DOWN:
		BT_DBG("Relative Volume Down (0x%x)", opcode);
		if (vcs_inst.srv.state.volume > 0) {
			vcs_inst.srv.state.volume = VOLUME_DOWN(vcs_inst.srv.state.volume);
			notify = true;
		}
		volume_change = true;
		break;
	case BT_VCS_OPCODE_REL_VOL_UP:
		BT_DBG("Relative Volume Up (0x%x)", opcode);
		if (vcs_inst.srv.state.volume != UINT8_MAX) {
			vcs_inst.srv.state.volume = VOLUME_UP(vcs_inst.srv.state.volume);
			notify = true;
		}
		volume_change = true;
		break;
	case BT_VCS_OPCODE_UNMUTE_REL_VOL_DOWN:
		BT_DBG("(Unmute) relative Volume Down (0x%x)", opcode);
		if (vcs_inst.srv.state.volume > 0) {
			vcs_inst.srv.state.volume = VOLUME_DOWN(vcs_inst.srv.state.volume);
			notify = true;
		}
		if (vcs_inst.srv.state.mute) {
			vcs_inst.srv.state.mute = BT_VCS_STATE_UNMUTED;
			notify = true;
		}
		volume_change = true;
		break;
	case BT_VCS_OPCODE_UNMUTE_REL_VOL_UP:
		BT_DBG("(Unmute) relative Volume Up (0x%x)", opcode);
		if (vcs_inst.srv.state.volume != UINT8_MAX) {
			vcs_inst.srv.state.volume = VOLUME_UP(vcs_inst.srv.state.volume);
			notify = true;
		}
		if (vcs_inst.srv.state.mute) {
			vcs_inst.srv.state.mute = BT_VCS_STATE_UNMUTED;
			notify = true;
		}
		volume_change = true;
		break;
	case BT_VCS_OPCODE_SET_ABS_VOL:
		BT_DBG("Set Absolute Volume (0x%x) %u",
		       opcode, vcs_inst.srv.state.volume);
		if (vcs_inst.srv.state.volume != cp_val->volume) {
			vcs_inst.srv.state.volume = cp_val->volume;
			notify = true;
		}
		volume_change = true;
		break;
	case BT_VCS_OPCODE_UNMUTE:
		BT_DBG("Unmute (0x%x)", opcode);
		if (vcs_inst.srv.state.mute) {
			vcs_inst.srv.state.mute = BT_VCS_STATE_UNMUTED;
			notify = true;
		}
		break;
	case BT_VCS_OPCODE_MUTE:
		BT_DBG("Mute (0x%x)", opcode);
		if (vcs_inst.srv.state.mute == BT_VCS_STATE_UNMUTED) {
			vcs_inst.srv.state.mute = BT_VCS_STATE_MUTED;
			notify = true;
		}
		break;
	default:
		BT_DBG("Unknown opcode (0x%x)", opcode);
		return BT_GATT_ERR(BT_VCS_ERR_OP_NOT_SUPPORTED);
	}

	if (notify) {
		vcs_inst.srv.state.change_counter++;
		BT_DBG("New state: volume %u, mute %u, counter %u",
		       vcs_inst.srv.state.volume, vcs_inst.srv.state.mute,
		       vcs_inst.srv.state.change_counter);

		bt_gatt_notify_uuid(NULL, BT_UUID_VCS_STATE,
				    vcs_inst.srv.service_p->attrs,
				    &vcs_inst.srv.state, sizeof(vcs_inst.srv.state));

		if (vcs_inst.srv.cb && vcs_inst.srv.cb->state) {
			vcs_inst.srv.cb->state(&vcs_inst, 0,
					       vcs_inst.srv.state.volume,
					       vcs_inst.srv.state.mute);
		}
	}

	if (volume_change && !vcs_inst.srv.flags) {
		vcs_inst.srv.flags = 1;

		bt_gatt_notify_uuid(NULL, BT_UUID_VCS_FLAGS,
				    vcs_inst.srv.service_p->attrs,
				    &vcs_inst.srv.flags, sizeof(vcs_inst.srv.flags));

		if (vcs_inst.srv.cb && vcs_inst.srv.cb->flags) {
			vcs_inst.srv.cb->flags(&vcs_inst, 0, vcs_inst.srv.flags);
		}
	}
	return len;
}

static void flags_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("value 0x%04x", value);
}

static ssize_t read_flags(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  void *buf, uint16_t len, uint16_t offset)
{
	BT_DBG("0x%02x", vcs_inst.srv.flags);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &vcs_inst.srv.flags,
				 sizeof(vcs_inst.srv.flags));
}

#define DUMMY_INCLUDE(i, _) BT_GATT_INCLUDE_SERVICE(NULL),
#define VOCS_INCLUDES(cnt) LISTIFY(cnt, DUMMY_INCLUDE, ())
#define AICS_INCLUDES(cnt) LISTIFY(cnt, DUMMY_INCLUDE, ())

#define BT_VCS_SERVICE_DEFINITION \
	BT_GATT_PRIMARY_SERVICE(BT_UUID_VCS), \
	VOCS_INCLUDES(CONFIG_BT_VCS_VOCS_INSTANCE_COUNT) \
	AICS_INCLUDES(CONFIG_BT_VCS_AICS_INSTANCE_COUNT) \
	BT_AUDIO_CHRC(BT_UUID_VCS_STATE, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_vol_state, NULL, NULL), \
	BT_AUDIO_CCC(volume_state_cfg_changed), \
	BT_AUDIO_CHRC(BT_UUID_VCS_CONTROL, \
		      BT_GATT_CHRC_WRITE, \
		      BT_GATT_PERM_WRITE_ENCRYPT, \
		      NULL, write_vcs_control, NULL), \
	BT_AUDIO_CHRC(BT_UUID_VCS_FLAGS, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      read_flags, NULL, NULL), \
	BT_AUDIO_CCC(flags_cfg_changed)

static struct bt_gatt_attr vcs_attrs[] = { BT_VCS_SERVICE_DEFINITION };
static struct bt_gatt_service vcs_svc;

static int prepare_vocs_inst(struct bt_vcs_register_param *param)
{
	int err;
	int j;
	int i;

	__ASSERT(param, "NULL param");

	for (j = 0, i = 0; i < ARRAY_SIZE(vcs_attrs); i++) {
		if (bt_uuid_cmp(vcs_attrs[i].uuid, BT_UUID_GATT_INCLUDE) == 0 &&
		    !vcs_attrs[i].user_data) {

			vcs_inst.srv.vocs_insts[j] = bt_vocs_free_instance_get();

			if (vcs_inst.srv.vocs_insts[j] == NULL) {
				BT_ERR("Could not get free VOCS instances[%d]",
				       j);
				return -ENOMEM;
			}

			err = bt_vocs_register(vcs_inst.srv.vocs_insts[j],
					       &param->vocs_param[j]);
			if (err != 0) {
				BT_DBG("Could not register VOCS instance[%d]: %d",
				       j, err);
				return err;
			}

			vcs_attrs[i].user_data = bt_vocs_svc_decl_get(vcs_inst.srv.vocs_insts[j]);
			j++;

			if (j == CONFIG_BT_VCS_VOCS_INSTANCE_COUNT) {
				break;
			}
		}
	}

	__ASSERT(j == CONFIG_BT_VCS_VOCS_INSTANCE_COUNT,
		 "Invalid VOCS instance count");

	return 0;
}

static int prepare_aics_inst(struct bt_vcs_register_param *param)
{
	int err;
	int j;
	int i;

	__ASSERT(param, "NULL param");

	for (j = 0, i = 0; i < ARRAY_SIZE(vcs_attrs); i++) {
		if (bt_uuid_cmp(vcs_attrs[i].uuid, BT_UUID_GATT_INCLUDE) == 0 &&
		    !vcs_attrs[i].user_data) {
			vcs_inst.srv.aics_insts[j] = bt_aics_free_instance_get();

			if (vcs_inst.srv.aics_insts[j] == NULL) {
				BT_ERR("Could not get free AICS instances[%d]",
				       j);
				return -ENOMEM;
			}

			err = bt_aics_register(vcs_inst.srv.aics_insts[j],
					       &param->aics_param[j]);
			if (err != 0) {
				BT_DBG("Could not register AICS instance[%d]: %d",
				       j, err);
				return err;
			}

			vcs_attrs[i].user_data = bt_aics_svc_decl_get(vcs_inst.srv.aics_insts[j]);
			j++;

			BT_DBG("AICS P %p", vcs_attrs[i].user_data);

			if (j == CONFIG_BT_VCS_AICS_INSTANCE_COUNT) {
				break;
			}
		}
	}

	__ASSERT(j == CONFIG_BT_VCS_AICS_INSTANCE_COUNT,
		 "Invalid AICS instance count");

	return 0;
}

/****************************** PUBLIC API ******************************/
int bt_vcs_register(struct bt_vcs_register_param *param, struct bt_vcs **vcs)
{
	static bool registered;
	int err;

	CHECKIF(param == NULL) {
		BT_DBG("param is NULL");
		return -EINVAL;
	}

	CHECKIF(param->mute > BT_VCS_STATE_MUTED) {
		BT_DBG("Invalid mute value: %u", param->mute);
		return -EINVAL;
	}

	CHECKIF(param->step == 0) {
		BT_DBG("Invalid step value: %u", param->step);
		return -EINVAL;
	}

	if (registered) {
		*vcs = &vcs_inst;
		return -EALREADY;
	}

	vcs_svc = (struct bt_gatt_service)BT_GATT_SERVICE(vcs_attrs);

	if (CONFIG_BT_VCS_VOCS_INSTANCE_COUNT > 0) {
		err = prepare_vocs_inst(param);

		if (err != 0) {
			return err;
		}
	}

	if (CONFIG_BT_VCS_AICS_INSTANCE_COUNT > 0) {
		err = prepare_aics_inst(param);

		if (err != 0) {
			return err;
		}
	}


	vcs_inst.srv.state.volume = param->volume;
	vcs_inst.srv.state.mute = param->mute;
	vcs_inst.srv.volume_step = param->step;
	vcs_inst.srv.service_p = &vcs_svc;

	err = bt_gatt_service_register(&vcs_svc);
	if (err != 0) {
		BT_DBG("VCS service register failed: %d", err);
	}

	vcs_inst.srv.cb = param->cb;

	*vcs = &vcs_inst;
	registered = true;

	return err;
}

int bt_vcs_aics_deactivate(struct bt_vcs *vcs, struct bt_aics *inst)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	CHECKIF(inst == NULL) {
		BT_DBG("NULL aics instance");
		return -EINVAL;
	}

	if (!valid_aics_inst(vcs, inst)) {
		return -EINVAL;
	}

	return bt_aics_deactivate(inst);
}

int bt_vcs_aics_activate(struct bt_vcs *vcs, struct bt_aics *inst)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	CHECKIF(inst == NULL) {
		BT_DBG("NULL aics instance");
		return -EINVAL;
	}

	if (!valid_aics_inst(vcs, inst)) {
		return -EINVAL;
	}

	return bt_aics_activate(inst);
}

#endif /* CONFIG_BT_VCS */

int bt_vcs_included_get(struct bt_vcs *vcs, struct bt_vcs_included *included)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}


	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT) && vcs->client_instance) {
		return bt_vcs_client_included_get(vcs, included);
	}

#if defined(CONFIG_BT_VCS)
	if (included == NULL) {
		return -EINVAL;
	}

	included->vocs_cnt = ARRAY_SIZE(vcs->srv.vocs_insts);
	included->vocs = vcs->srv.vocs_insts;

	included->aics_cnt = ARRAY_SIZE(vcs->srv.aics_insts);
	included->aics = vcs->srv.aics_insts;

	return 0;
#else
	return -EOPNOTSUPP;
#endif /* CONFIG_BT_VCS */
}

int bt_vcs_vol_step_set(uint8_t volume_step)
{
#if defined(CONFIG_BT_VCS)
	if (volume_step > 0) {
		vcs_inst.srv.volume_step = volume_step;
		return 0;
	} else {
		return -EINVAL;
	}
#endif /* CONFIG_BT_VCS */

	return -EOPNOTSUPP;
}

int bt_vcs_vol_get(struct bt_vcs *vcs)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT) && vcs->client_instance) {
		return bt_vcs_client_read_vol_state(vcs);
	}

#if defined(CONFIG_BT_VCS)
	if (vcs->srv.cb && vcs->srv.cb->state) {
		vcs->srv.cb->state(vcs, 0, vcs->srv.state.volume,
				   vcs->srv.state.mute);
	}

	return 0;
#else
	return -EOPNOTSUPP;
#endif /* CONFIG_BT_VCS */
}

int bt_vcs_flags_get(struct bt_vcs *vcs)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT) && vcs->client_instance) {
		return bt_vcs_client_read_flags(vcs);
	}

#if defined(CONFIG_BT_VCS)
	if (vcs->srv.cb && vcs->srv.cb->flags) {
		vcs->srv.cb->flags(vcs, 0, vcs->srv.flags);
	}

	return 0;
#else
	return -EOPNOTSUPP;
#endif /* CONFIG_BT_VCS */
}

int bt_vcs_vol_down(struct bt_vcs *vcs)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT) && vcs->client_instance) {
		return bt_vcs_client_vol_down(vcs);
	}

#if defined(CONFIG_BT_VCS)
	const struct vcs_control cp = {
		.opcode = BT_VCS_OPCODE_REL_VOL_DOWN,
		.counter = vcs->srv.state.change_counter,
	};
	int err = write_vcs_control(NULL, NULL, &cp, sizeof(cp), 0, 0);

	return err > 0 ? 0 : err;
#else
	return -EOPNOTSUPP;
#endif /* CONFIG_BT_VCS */
}

int bt_vcs_vol_up(struct bt_vcs *vcs)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT) && vcs->client_instance) {
		return bt_vcs_client_vol_up(vcs);
	}

#if defined(CONFIG_BT_VCS)
	const struct vcs_control cp = {
		.opcode = BT_VCS_OPCODE_REL_VOL_UP,
		.counter = vcs->srv.state.change_counter,
	};
	int err = write_vcs_control(NULL, NULL, &cp, sizeof(cp), 0, 0);

	return err > 0 ? 0 : err;
#else
	return -EOPNOTSUPP;
#endif /* CONFIG_BT_VCS */
}

int bt_vcs_unmute_vol_down(struct bt_vcs *vcs)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT) && vcs->client_instance) {
		return bt_vcs_client_unmute_vol_down(vcs);
	}

#if defined(CONFIG_BT_VCS)
	const struct vcs_control cp = {
		.opcode = BT_VCS_OPCODE_UNMUTE_REL_VOL_DOWN,
		.counter = vcs->srv.state.change_counter,
	};
	int err = write_vcs_control(NULL, NULL, &cp, sizeof(cp), 0, 0);

	return err > 0 ? 0 : err;
#else
	return -EOPNOTSUPP;
#endif /* CONFIG_BT_VCS */
}

int bt_vcs_unmute_vol_up(struct bt_vcs *vcs)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT) && vcs->client_instance) {
		return bt_vcs_client_unmute_vol_up(vcs);
	}

#if defined(CONFIG_BT_VCS)
	const struct vcs_control cp = {
		.opcode = BT_VCS_OPCODE_UNMUTE_REL_VOL_UP,
		.counter = vcs->srv.state.change_counter,
	};
	int err = write_vcs_control(NULL, NULL, &cp, sizeof(cp), 0, 0);

	return err > 0 ? 0 : err;
#else
	return -EOPNOTSUPP;
#endif /* CONFIG_BT_VCS */
}

int bt_vcs_vol_set(struct bt_vcs *vcs, uint8_t volume)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT) && vcs->client_instance) {
		return bt_vcs_client_set_volume(vcs, volume);
	}

#if defined(CONFIG_BT_VCS)
	const struct vcs_control_vol cp = {
		.cp = {
			.opcode = BT_VCS_OPCODE_SET_ABS_VOL,
			.counter = vcs->srv.state.change_counter
		},
		.volume = volume
	};
	int err = write_vcs_control(NULL, NULL, &cp, sizeof(cp), 0, 0);

	return err > 0 ? 0 : err;
#else
	return -EOPNOTSUPP;
#endif /* CONFIG_BT_VCS */
}

int bt_vcs_unmute(struct bt_vcs *vcs)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT) && vcs->client_instance) {
		return bt_vcs_client_unmute(vcs);
	}

#if defined(CONFIG_BT_VCS)
	const struct vcs_control cp = {
		.opcode = BT_VCS_OPCODE_UNMUTE,
		.counter = vcs->srv.state.change_counter,
	};
	int err = write_vcs_control(NULL, NULL, &cp, sizeof(cp), 0, 0);

	return err > 0 ? 0 : err;
#else
	return -EOPNOTSUPP;
#endif /* CONFIG_BT_VCS */
}

int bt_vcs_mute(struct bt_vcs *vcs)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT) && vcs->client_instance) {
		return bt_vcs_client_mute(vcs);
	}

#if defined(CONFIG_BT_VCS)
	const struct vcs_control cp = {
		.opcode = BT_VCS_OPCODE_MUTE,
		.counter = vcs->srv.state.change_counter,
	};
	int err = write_vcs_control(NULL, NULL, &cp, sizeof(cp), 0, 0);

	return err > 0 ? 0 : err;
#else
	return -EOPNOTSUPP;
#endif /* CONFIG_BT_VCS */
}

int bt_vcs_vocs_state_get(struct bt_vcs *vcs, struct bt_vocs *inst)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT_VOCS) &&
	    bt_vcs_client_valid_vocs_inst(vcs, inst)) {
		return bt_vocs_state_get(inst);
	}

	if (IS_ENABLED(CONFIG_BT_VCS_VOCS) && valid_vocs_inst(vcs, inst)) {
		return bt_vocs_state_get(inst);
	}

	return -EOPNOTSUPP;
}

int bt_vcs_vocs_location_get(struct bt_vcs *vcs, struct bt_vocs *inst)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT_VOCS) &&
	    bt_vcs_client_valid_vocs_inst(vcs, inst)) {
		return bt_vocs_location_get(inst);
	}

	if (IS_ENABLED(CONFIG_BT_VCS_VOCS) && valid_vocs_inst(vcs, inst)) {
		return bt_vocs_location_get(inst);
	}

	return -EOPNOTSUPP;
}

int bt_vcs_vocs_location_set(struct bt_vcs *vcs, struct bt_vocs *inst,
			     uint8_t location)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT_VOCS) &&
	    bt_vcs_client_valid_vocs_inst(vcs, inst)) {
		return bt_vocs_location_set(inst, location);
	}

	if (IS_ENABLED(CONFIG_BT_VCS_VOCS) && valid_vocs_inst(vcs, inst)) {
		return bt_vocs_location_set(inst, location);
	}

	return -EOPNOTSUPP;
}

int bt_vcs_vocs_state_set(struct bt_vcs *vcs, struct bt_vocs *inst,
			  int16_t offset)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT_VOCS) &&
	    bt_vcs_client_valid_vocs_inst(vcs, inst)) {
		return bt_vocs_state_set(inst, offset);
	}

	if (IS_ENABLED(CONFIG_BT_VCS_VOCS) && valid_vocs_inst(vcs, inst)) {
		return bt_vocs_state_set(inst, offset);
	}

	return -EOPNOTSUPP;
}

int bt_vcs_vocs_description_get(struct bt_vcs *vcs, struct bt_vocs *inst)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT_VOCS) &&
	    bt_vcs_client_valid_vocs_inst(vcs, inst)) {
		return bt_vocs_description_get(inst);
	}

	if (IS_ENABLED(CONFIG_BT_VCS_VOCS) && valid_vocs_inst(vcs, inst)) {
		return bt_vocs_description_get(inst);
	}

	return -EOPNOTSUPP;
}

int bt_vcs_vocs_description_set(struct bt_vcs *vcs, struct bt_vocs *inst,
				const char *description)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT_VOCS) &&
	    bt_vcs_client_valid_vocs_inst(vcs, inst)) {
		return bt_vocs_description_set(inst, description);
	}

	if (IS_ENABLED(CONFIG_BT_VCS_VOCS) && valid_vocs_inst(vcs, inst)) {
		return bt_vocs_description_set(inst, description);
	}

	return -EOPNOTSUPP;
}

int bt_vcs_aics_state_get(struct bt_vcs *vcs, struct bt_aics *inst)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT_AICS) &&
	    bt_vcs_client_valid_aics_inst(vcs, inst)) {
		return bt_aics_state_get(inst);
	}

	if (IS_ENABLED(CONFIG_BT_VCS_AICS) && valid_aics_inst(vcs, inst)) {
		return bt_aics_state_get(inst);
	}

	return -EOPNOTSUPP;
}

int bt_vcs_aics_gain_setting_get(struct bt_vcs *vcs, struct bt_aics *inst)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT_AICS) &&
	    bt_vcs_client_valid_aics_inst(vcs, inst)) {
		return bt_aics_gain_setting_get(inst);
	}

	if (IS_ENABLED(CONFIG_BT_VCS_AICS) && valid_aics_inst(vcs, inst)) {
		return bt_aics_gain_setting_get(inst);
	}

	return -EOPNOTSUPP;
}

int bt_vcs_aics_type_get(struct bt_vcs *vcs, struct bt_aics *inst)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT_AICS) &&
	    bt_vcs_client_valid_aics_inst(vcs, inst)) {
		return bt_aics_type_get(inst);
	}

	if (IS_ENABLED(CONFIG_BT_VCS_AICS) && valid_aics_inst(vcs, inst)) {
		return bt_aics_type_get(inst);
	}

	return -EOPNOTSUPP;
}

int bt_vcs_aics_status_get(struct bt_vcs *vcs, struct bt_aics *inst)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT_AICS) &&
	    bt_vcs_client_valid_aics_inst(vcs, inst)) {
		return bt_aics_status_get(inst);
	}

	if (IS_ENABLED(CONFIG_BT_VCS_AICS) && valid_aics_inst(vcs, inst)) {
		return bt_aics_status_get(inst);
	}

	return -EOPNOTSUPP;
}

int bt_vcs_aics_unmute(struct bt_vcs *vcs, struct bt_aics *inst)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT_AICS) &&
	    bt_vcs_client_valid_aics_inst(vcs, inst)) {
		return bt_aics_unmute(inst);
	}

	if (IS_ENABLED(CONFIG_BT_VCS_AICS) && valid_aics_inst(vcs, inst)) {
		return bt_aics_unmute(inst);
	}

	return -EOPNOTSUPP;
}

int bt_vcs_aics_mute(struct bt_vcs *vcs, struct bt_aics *inst)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT_AICS) &&
	    bt_vcs_client_valid_aics_inst(vcs, inst)) {
		return bt_aics_mute(inst);
	}

	if (IS_ENABLED(CONFIG_BT_VCS_AICS) && valid_aics_inst(vcs, inst)) {
		return bt_aics_mute(inst);
	}

	return -EOPNOTSUPP;
}

int bt_vcs_aics_manual_gain_set(struct bt_vcs *vcs, struct bt_aics *inst)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT_AICS) &&
	    bt_vcs_client_valid_aics_inst(vcs, inst)) {
		return bt_aics_manual_gain_set(inst);
	}

	if (IS_ENABLED(CONFIG_BT_VCS_AICS) && valid_aics_inst(vcs, inst)) {
		return bt_aics_manual_gain_set(inst);
	}

	return -EOPNOTSUPP;
}

int bt_vcs_aics_automatic_gain_set(struct bt_vcs *vcs, struct bt_aics *inst)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT_AICS) &&
	    bt_vcs_client_valid_aics_inst(vcs, inst)) {
		return bt_aics_automatic_gain_set(inst);
	}

	if (IS_ENABLED(CONFIG_BT_VCS_AICS) && valid_aics_inst(vcs, inst)) {
		return bt_aics_automatic_gain_set(inst);
	}

	return -EOPNOTSUPP;
}

int bt_vcs_aics_gain_set(struct bt_vcs *vcs, struct bt_aics *inst,
			 int8_t gain)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT_AICS) &&
	    bt_vcs_client_valid_aics_inst(vcs, inst)) {
		return bt_aics_gain_set(inst, gain);
	}

	if (IS_ENABLED(CONFIG_BT_VCS_AICS) && valid_aics_inst(vcs, inst)) {
		return bt_aics_gain_set(inst, gain);
	}

	return -EOPNOTSUPP;
}

int bt_vcs_aics_description_get(struct bt_vcs *vcs, struct bt_aics *inst)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT_AICS) &&
	    bt_vcs_client_valid_aics_inst(vcs, inst)) {
		return bt_aics_description_get(inst);
	}

	if (IS_ENABLED(CONFIG_BT_VCS_AICS) && valid_aics_inst(vcs, inst)) {
		return bt_aics_description_get(inst);
	}

	return -EOPNOTSUPP;
}

int bt_vcs_aics_description_set(struct bt_vcs *vcs, struct bt_aics *inst,
				const char *description)
{
	CHECKIF(vcs == NULL) {
		BT_DBG("NULL vcs instance");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_VCS_CLIENT_AICS) &&
	    bt_vcs_client_valid_aics_inst(vcs, inst)) {
		return bt_aics_description_set(inst, description);
	}

	if (IS_ENABLED(CONFIG_BT_VCS_AICS) && valid_aics_inst(vcs, inst)) {
		return bt_aics_description_set(inst, description);
	}

	return -EOPNOTSUPP;
}
