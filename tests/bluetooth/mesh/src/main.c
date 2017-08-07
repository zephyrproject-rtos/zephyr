/* main.c - Application main entry point */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <misc/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh.h>

#include "board.h"

#define CID_INTEL 0x0002

static bool has_reg_fault = true;

static struct bt_mesh_cfg cfg_srv = {
	.relay = BT_MESH_RELAY_DISABLED,
	.beacon = BT_MESH_BEACON_DISABLED,
#if defined(CONFIG_BLUETOOTH_MESH_FRIEND)
#if defined(CONFIG_BLUETOOTH_MESH_LOW_POWER)
	.frnd = BT_MESH_FRIEND_DISABLED,
#else
	.frnd = BT_MESH_FRIEND_ENABLED,
#endif
#else
	.frnd = BT_MESH_FRIEND_NOT_SUPPORTED,
#endif
#if defined(CONFIG_BLUETOOTH_MESH_GATT_PROXY)
	.gatt_proxy = BT_MESH_GATT_PROXY_ENABLED,
#else
	.gatt_proxy = BT_MESH_GATT_PROXY_NOT_SUPPORTED,
#endif
	.default_ttl = 7,

	/* 3 transmissions with 20ms interval */
	.net_transmit = BT_MESH_TRANSMIT(2, 20),
	.relay_retransmit = BT_MESH_TRANSMIT(2, 20),
};

static int fault_get_cur(struct bt_mesh_model *model, u8_t *test_id,
			 u16_t *company_id, u8_t *faults, u8_t *fault_count)
{
	u8_t reg_faults[24] = { [0 ... 23] = 0xff };

	printk("fault_get_cur() has_reg_fault %u\n", has_reg_fault);

	*test_id = 0x00;
	*company_id = CID_INTEL;
	memcpy(faults, reg_faults, sizeof(reg_faults));
	*fault_count = sizeof(reg_faults);

	return 0;
}

static int fault_get_reg(struct bt_mesh_model *model, u16_t company_id,
			 u8_t *test_id, u8_t *faults, u8_t *fault_count)
{
	if (company_id != CID_INTEL) {
		return -EINVAL;
	}

	printk("fault_get_reg() has_reg_fault %u\n", has_reg_fault);

	*test_id = 0x00;

	if (has_reg_fault) {
		u8_t reg_faults[24] = { [0 ... 23] = 0xff };

		memcpy(faults, reg_faults, sizeof(reg_faults));
		*fault_count = sizeof(reg_faults);
	} else {
		*fault_count = 0;
	}

	return 0;
}

static int fault_clear(struct bt_mesh_model *model, uint16_t company_id)
{
	if (company_id != CID_INTEL) {
		return -EINVAL;
	}

	has_reg_fault = false;

	return 0;
}

static int fault_test(struct bt_mesh_model *model, uint8_t test_id,
		      uint16_t company_id)
{
	if (company_id != CID_INTEL) {
		return -EINVAL;
	}

	has_reg_fault = true;
	bt_mesh_fault_update(model->elem);

	return 0;
}

static struct bt_mesh_health health_srv = {
	.fault_get_cur = fault_get_cur,
	.fault_get_reg = fault_get_reg,
	.fault_clear = fault_clear,
	.fault_test = fault_test,
};

static struct bt_mesh_model root_models[] = {
	BT_MESH_MODEL_CFG_SRV(&cfg_srv),
	BT_MESH_MODEL_HEALTH_SRV(&health_srv),
};

static void vnd_publish(struct bt_mesh_model *mod)
{
	printk("Vendor publish\n");
}

static struct bt_mesh_model_pub vnd_pub = {
	.func = vnd_publish,
};

static struct bt_mesh_model_pub vnd_pub2;

static const struct bt_mesh_model_op vnd_ops[] = {
	BT_MESH_MODEL_OP_END,
};

static struct bt_mesh_model vnd_models[] = {
	BT_MESH_MODEL_VND(CID_INTEL, 0x1234, vnd_ops, &vnd_pub, NULL),
	BT_MESH_MODEL_VND(CID_INTEL, 0x4321, vnd_ops, &vnd_pub2, NULL),
};

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(0, root_models, vnd_models),
};

static const struct bt_mesh_comp comp = {
	.cid = CID_INTEL,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

#if 0
static int output_number(bt_mesh_output_action action, uint32_t number)
{
	printk("OOB Number: %u\n", number);

	board_output_number(action, number);

	return 0;
}
#endif

#if defined(CONFIG_BLUETOOTH_MESH_LOW_POWER)
static struct k_delayed_work timer;

static void enable_lpn(struct k_work *work)
{
	printk("Enabling LPN\n");

	bt_mesh_lpn_set(true);
}
#endif

static void prov_complete(void)
{
	board_prov_complete();

#if defined(CONFIG_BLUETOOTH_MESH_LOW_POWER)
	k_delayed_work_init(&timer, enable_lpn);
	k_delayed_work_submit(&timer, K_SECONDS(10));
#endif
}

static const u8_t dev_uuid[16] = { 0xdd, 0xdd };

static const struct bt_mesh_prov prov = {
	.uuid = dev_uuid,
#if 0
	.output_size = 4,
	.output_actions = BT_MESH_DISPLAY_NUMBER,
	.output_number = output_number,
#endif
	.complete = prov_complete,
};

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	board_init();

	err = bt_mesh_init(&prov, &comp);
	if (err) {
		printk("Initializing mesh failed (err %d)\n", err);
		return;
	}

	printk("Mesh initialized\n");
}

void main(void)
{
	int err;

	printk("Initializing...\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}
}
