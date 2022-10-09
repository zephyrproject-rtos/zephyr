/*  Bluetooth VCP */

/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2019-2020 Bose Corporation
 * Copyright (c) 2020-2022 Nordic Semiconductor ASA
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
#include <zephyr/bluetooth/audio/vcp.h>

#include "audio_internal.h"
#include "vcp_internal.h"

#define LOG_LEVEL CONFIG_BT_VCP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_vcp);

#define VOLUME_DOWN(current_vol) \
	((uint8_t)MAX(0, (int)current_vol - vcp_inst.srv.volume_step))
#define VOLUME_UP(current_vol) \
	((uint8_t)MIN(UINT8_MAX, (int)current_vol + vcp_inst.srv.volume_step))

#define VALID_VCP_OPCODE(opcode) ((opcode) <= BT_VCP_OPCODE_MUTE)

static struct bt_vcp vcp_inst;

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
		vcp_inst.srv.state.volume, vcp_inst.srv.state.mute,
		vcp_inst.srv.state.change_counter);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &vcp_inst.srv.state, sizeof(vcp_inst.srv.state));
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

	if (cp_val->cp.counter != vcp_inst.srv.state.change_counter) {
		return BT_GATT_ERR(BT_VCP_ERR_INVALID_COUNTER);
	}

	switch (opcode) {
	case BT_VCP_OPCODE_REL_VOL_DOWN:
		LOG_DBG("Relative Volume Down (0x%x)", opcode);
		if (vcp_inst.srv.state.volume > 0) {
			vcp_inst.srv.state.volume = VOLUME_DOWN(vcp_inst.srv.state.volume);
			notify = true;
		}
		volume_change = true;
		break;
	case BT_VCP_OPCODE_REL_VOL_UP:
		LOG_DBG("Relative Volume Up (0x%x)", opcode);
		if (vcp_inst.srv.state.volume != UINT8_MAX) {
			vcp_inst.srv.state.volume = VOLUME_UP(vcp_inst.srv.state.volume);
			notify = true;
		}
		volume_change = true;
		break;
	case BT_VCP_OPCODE_UNMUTE_REL_VOL_DOWN:
		LOG_DBG("(Unmute) relative Volume Down (0x%x)", opcode);
		if (vcp_inst.srv.state.volume > 0) {
			vcp_inst.srv.state.volume = VOLUME_DOWN(vcp_inst.srv.state.volume);
			notify = true;
		}
		if (vcp_inst.srv.state.mute) {
			vcp_inst.srv.state.mute = BT_VCP_STATE_UNMUTED;
			notify = true;
		}
		volume_change = true;
		break;
	case BT_VCP_OPCODE_UNMUTE_REL_VOL_UP:
		LOG_DBG("(Unmute) relative Volume Up (0x%x)", opcode);
		if (vcp_inst.srv.state.volume != UINT8_MAX) {
			vcp_inst.srv.state.volume = VOLUME_UP(vcp_inst.srv.state.volume);
			notify = true;
		}
		if (vcp_inst.srv.state.mute) {
			vcp_inst.srv.state.mute = BT_VCP_STATE_UNMUTED;
			notify = true;
		}
		volume_change = true;
		break;
	case BT_VCP_OPCODE_SET_ABS_VOL:
		LOG_DBG("Set Absolute Volume (0x%x): Current volume %u",
			opcode, vcp_inst.srv.state.volume);
		if (vcp_inst.srv.state.volume != cp_val->volume) {
			vcp_inst.srv.state.volume = cp_val->volume;
			notify = true;
		}
		volume_change = true;
		break;
	case BT_VCP_OPCODE_UNMUTE:
		LOG_DBG("Unmute (0x%x)", opcode);
		if (vcp_inst.srv.state.mute) {
			vcp_inst.srv.state.mute = BT_VCP_STATE_UNMUTED;
			notify = true;
		}
		break;
	case BT_VCP_OPCODE_MUTE:
		LOG_DBG("Mute (0x%x)", opcode);
		if (vcp_inst.srv.state.mute == BT_VCP_STATE_UNMUTED) {
			vcp_inst.srv.state.mute = BT_VCP_STATE_MUTED;
			notify = true;
		}
		break;
	default:
		LOG_DBG("Unknown opcode (0x%x)", opcode);
		return BT_GATT_ERR(BT_VCP_ERR_OP_NOT_SUPPORTED);
	}

	if (notify) {
		vcp_inst.srv.state.change_counter++;
		LOG_DBG("New state: volume %u, mute %u, counter %u",
			vcp_inst.srv.state.volume, vcp_inst.srv.state.mute,
			vcp_inst.srv.state.change_counter);

		bt_gatt_notify_uuid(NULL, BT_UUID_VCS_STATE,
				    vcp_inst.srv.service_p->attrs,
				    &vcp_inst.srv.state, sizeof(vcp_inst.srv.state));

		if (vcp_inst.srv.cb && vcp_inst.srv.cb->state) {
			vcp_inst.srv.cb->state(&vcp_inst, 0,
					       vcp_inst.srv.state.volume,
					       vcp_inst.srv.state.mute);
		}
	}

	if (volume_change && !vcp_inst.srv.flags) {
		vcp_inst.srv.flags = 1;

		bt_gatt_notify_uuid(NULL, BT_UUID_VCS_FLAGS,
				    vcp_inst.srv.service_p->attrs,
				    &vcp_inst.srv.flags, sizeof(vcp_inst.srv.flags));

		if (vcp_inst.srv.cb && vcp_inst.srv.cb->flags) {
			vcp_inst.srv.cb->flags(&vcp_inst, 0, vcp_inst.srv.flags);
		}
	}
	return len;
}

static void flags_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	LOG_DBG("value 0x%04x", value);
}

static ssize_t read_flags(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  void *buf, uint16_t len, uint16_t offset)
{
	LOG_DBG("0x%02x", vcp_inst.srv.flags);
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &vcp_inst.srv.flags,
				 sizeof(vcp_inst.srv.flags));
}

#define DUMMY_INCLUDE(i, _) BT_GATT_INCLUDE_SERVICE(NULL),
#define VOCS_INCLUDES(cnt) LISTIFY(cnt, DUMMY_INCLUDE, ())
#define AICS_INCLUDES(cnt) LISTIFY(cnt, DUMMY_INCLUDE, ())

#define BT_VCS_DEFINITION \
	BT_GATT_PRIMARY_SERVICE(BT_UUID_VCS), \
	VOCS_INCLUDES(CONFIG_BT_VCP_VOCS_INSTANCE_COUNT) \
	AICS_INCLUDES(CONFIG_BT_VCP_AICS_INSTANCE_COUNT) \
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

static struct bt_gatt_attr vcs_attrs[] = { BT_VCS_DEFINITION };
static struct bt_gatt_service vcs_svc;

static int prepare_vocs_inst(struct bt_vcp_register_param *param)
{
	int err;
	int j;
	int i;

	__ASSERT(param, "NULL param");

	for (j = 0, i = 0; i < ARRAY_SIZE(vcs_attrs); i++) {
		if (bt_uuid_cmp(vcs_attrs[i].uuid, BT_UUID_GATT_INCLUDE) == 0 &&
		    !vcs_attrs[i].user_data) {

			vcp_inst.srv.vocs_insts[j] = bt_vocs_free_instance_get();

			if (vcp_inst.srv.vocs_insts[j] == NULL) {
				LOG_ERR("Could not get free VOCS instances[%d]",
					j);
				return -ENOMEM;
			}

			err = bt_vocs_register(vcp_inst.srv.vocs_insts[j],
					       &param->vocs_param[j]);
			if (err != 0) {
				LOG_DBG("Could not register VOCS instance[%d]: %d",
					j, err);
				return err;
			}

			vcs_attrs[i].user_data = bt_vocs_svc_decl_get(vcp_inst.srv.vocs_insts[j]);
			j++;

			if (j == CONFIG_BT_VCP_VOCS_INSTANCE_COUNT) {
				break;
			}
		}
	}

	__ASSERT(j == CONFIG_BT_VCP_VOCS_INSTANCE_COUNT,
		 "Invalid VOCS instance count");

	return 0;
}

static int prepare_aics_inst(struct bt_vcp_register_param *param)
{
	int err;
	int j;
	int i;

	__ASSERT(param, "NULL param");

	for (j = 0, i = 0; i < ARRAY_SIZE(vcs_attrs); i++) {
		if (bt_uuid_cmp(vcs_attrs[i].uuid, BT_UUID_GATT_INCLUDE) == 0 &&
		    !vcs_attrs[i].user_data) {
			vcp_inst.srv.aics_insts[j] = bt_aics_free_instance_get();

			if (vcp_inst.srv.aics_insts[j] == NULL) {
				LOG_ERR("Could not get free AICS instances[%d]",
					j);
				return -ENOMEM;
			}

			err = bt_aics_register(vcp_inst.srv.aics_insts[j],
					       &param->aics_param[j]);
			if (err != 0) {
				LOG_DBG("Could not register AICS instance[%d]: %d",
					j, err);
				return err;
			}

			vcs_attrs[i].user_data = bt_aics_svc_decl_get(vcp_inst.srv.aics_insts[j]);
			j++;

			LOG_DBG("AICS P %p", vcs_attrs[i].user_data);

			if (j == CONFIG_BT_VCP_AICS_INSTANCE_COUNT) {
				break;
			}
		}
	}

	__ASSERT(j == CONFIG_BT_VCP_AICS_INSTANCE_COUNT,
		 "Invalid AICS instance count");

	return 0;
}

/****************************** PUBLIC API ******************************/
int bt_vcp_register(struct bt_vcp_register_param *param, struct bt_vcp **vcp)
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
		*vcp = &vcp_inst;
		return -EALREADY;
	}

	vcs_svc = (struct bt_gatt_service)BT_GATT_SERVICE(vcs_attrs);

	if (CONFIG_BT_VCP_VOCS_INSTANCE_COUNT > 0) {
		err = prepare_vocs_inst(param);

		if (err != 0) {
			return err;
		}
	}

	if (CONFIG_BT_VCP_AICS_INSTANCE_COUNT > 0) {
		err = prepare_aics_inst(param);

		if (err != 0) {
			return err;
		}
	}


	vcp_inst.srv.state.volume = param->volume;
	vcp_inst.srv.state.mute = param->mute;
	vcp_inst.srv.volume_step = param->step;
	vcp_inst.srv.service_p = &vcs_svc;

	err = bt_gatt_service_register(&vcs_svc);
	if (err != 0) {
		LOG_DBG("VCP service register failed: %d", err);
	}

	vcp_inst.srv.cb = param->cb;

	*vcp = &vcp_inst;
	registered = true;

	return err;
}

int bt_vcp_included_get(struct bt_vcp *vcp, struct bt_vcp_included *included)
{
	CHECKIF(vcp == NULL) {
		LOG_DBG("NULL vcp instance");
		return -EINVAL;
	}

	if (included == NULL) {
		return -EINVAL;
	}

	included->vocs_cnt = ARRAY_SIZE(vcp->srv.vocs_insts);
	included->vocs = vcp->srv.vocs_insts;

	included->aics_cnt = ARRAY_SIZE(vcp->srv.aics_insts);
	included->aics = vcp->srv.aics_insts;

	return 0;
}

int bt_vcp_vol_step_set(uint8_t volume_step)
{
	if (volume_step > 0) {
		vcp_inst.srv.volume_step = volume_step;
		return 0;
	} else {
		return -EINVAL;
	}
}

int bt_vcp_vol_get(struct bt_vcp *vcp)
{
	CHECKIF(vcp == NULL) {
		LOG_DBG("NULL vcp instance");
		return -EINVAL;
	}

	if (vcp->srv.cb && vcp->srv.cb->state) {
		vcp->srv.cb->state(vcp, 0, vcp->srv.state.volume,
				   vcp->srv.state.mute);
	}

	return 0;
}

int bt_vcp_flags_get(struct bt_vcp *vcp)
{
	CHECKIF(vcp == NULL) {
		LOG_DBG("NULL vcp instance");
		return -EINVAL;
	}

	if (vcp->srv.cb && vcp->srv.cb->flags) {
		vcp->srv.cb->flags(vcp, 0, vcp->srv.flags);
	}

	return 0;
}

int bt_vcp_vol_down(struct bt_vcp *vcp)
{
	const struct vcs_control cp = {
		.opcode = BT_VCP_OPCODE_REL_VOL_DOWN,
		.counter = vcp->srv.state.change_counter,
	};
	int err;

	CHECKIF(vcp == NULL) {
		LOG_DBG("NULL vcp instance");
		return -EINVAL;
	}

	err = write_vcs_control(NULL, NULL, &cp, sizeof(cp), 0, 0);

	return err > 0 ? 0 : err;
}

int bt_vcp_vol_up(struct bt_vcp *vcp)
{
	const struct vcs_control cp = {
		.opcode = BT_VCP_OPCODE_REL_VOL_UP,
		.counter = vcp->srv.state.change_counter,
	};
	int err;

	CHECKIF(vcp == NULL) {
		LOG_DBG("NULL vcp instance");
		return -EINVAL;
	}

	err = write_vcs_control(NULL, NULL, &cp, sizeof(cp), 0, 0);

	return err > 0 ? 0 : err;
}

int bt_vcp_unmute_vol_down(struct bt_vcp *vcp)
{
	const struct vcs_control cp = {
		.opcode = BT_VCP_OPCODE_UNMUTE_REL_VOL_DOWN,
		.counter = vcp->srv.state.change_counter,
	};
	int err;

	CHECKIF(vcp == NULL) {
		LOG_DBG("NULL vcp instance");
		return -EINVAL;
	}

	err = write_vcs_control(NULL, NULL, &cp, sizeof(cp), 0, 0);

	return err > 0 ? 0 : err;
}

int bt_vcp_unmute_vol_up(struct bt_vcp *vcp)
{
	const struct vcs_control cp = {
		.opcode = BT_VCP_OPCODE_UNMUTE_REL_VOL_UP,
		.counter = vcp->srv.state.change_counter,
	};
	int err;

	CHECKIF(vcp == NULL) {
		LOG_DBG("NULL vcp instance");
		return -EINVAL;
	}

	err = write_vcs_control(NULL, NULL, &cp, sizeof(cp), 0, 0);

	return err > 0 ? 0 : err;
}

int bt_vcp_vol_set(struct bt_vcp *vcp, uint8_t volume)
{

	const struct vcs_control_vol cp = {
		.cp = {
			.opcode = BT_VCP_OPCODE_SET_ABS_VOL,
			.counter = vcp->srv.state.change_counter
		},
		.volume = volume
	};
	int err;

	CHECKIF(vcp == NULL) {
		LOG_DBG("NULL vcp instance");
		return -EINVAL;
	}

	err = write_vcs_control(NULL, NULL, &cp, sizeof(cp), 0, 0);

	return err > 0 ? 0 : err;
}

int bt_vcp_unmute(struct bt_vcp *vcp)
{
	const struct vcs_control cp = {
		.opcode = BT_VCP_OPCODE_UNMUTE,
		.counter = vcp->srv.state.change_counter,
	};
	int err;

	CHECKIF(vcp == NULL) {
		LOG_DBG("NULL vcp instance");
		return -EINVAL;
	}

	err = write_vcs_control(NULL, NULL, &cp, sizeof(cp), 0, 0);

	return err > 0 ? 0 : err;
}

int bt_vcp_mute(struct bt_vcp *vcp)
{
	const struct vcs_control cp = {
		.opcode = BT_VCP_OPCODE_MUTE,
		.counter = vcp->srv.state.change_counter,
	};
	int err;

	CHECKIF(vcp == NULL) {
		LOG_DBG("NULL vcp instance");
		return -EINVAL;
	}

	err = write_vcs_control(NULL, NULL, &cp, sizeof(cp), 0, 0);

	return err > 0 ? 0 : err;
}
