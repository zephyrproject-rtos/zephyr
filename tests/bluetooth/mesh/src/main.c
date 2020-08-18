/* main.c - Application main entry point */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh.h>

#include "board.h"

#define MAX_FAULT 24

static bool has_reg_fault = true;

static int fault_get_cur(struct bt_mesh_model *model, uint8_t *test_id,
			 uint16_t *company_id, uint8_t *faults, uint8_t *fault_count)
{
	uint8_t reg_faults[MAX_FAULT] = { [0 ... (MAX_FAULT - 1)] = 0xff };

	printk("fault_get_cur() has_reg_fault %u\n", has_reg_fault);

	*test_id = 0x00;
	*company_id = BT_COMP_ID_LF;
	memcpy(faults, reg_faults, sizeof(reg_faults));
	*fault_count = sizeof(reg_faults);

	return 0;
}

static int fault_get_reg(struct bt_mesh_model *model, uint16_t company_id,
			 uint8_t *test_id, uint8_t *faults, uint8_t *fault_count)
{
	if (company_id != BT_COMP_ID_LF) {
		return -EINVAL;
	}

	printk("fault_get_reg() has_reg_fault %u\n", has_reg_fault);

	*test_id = 0x00;

	if (has_reg_fault) {
		uint8_t reg_faults[MAX_FAULT] = { [0 ... (MAX_FAULT - 1)] = 0xff };

		memcpy(faults, reg_faults, sizeof(reg_faults));
		*fault_count = sizeof(reg_faults);
	} else {
		*fault_count = 0U;
	}

	return 0;
}

static int fault_clear(struct bt_mesh_model *model, uint16_t company_id)
{
	if (company_id != BT_COMP_ID_LF) {
		return -EINVAL;
	}

	has_reg_fault = false;

	return 0;
}

static int fault_test(struct bt_mesh_model *model, uint8_t test_id,
		      uint16_t company_id)
{
	if (company_id != BT_COMP_ID_LF) {
		return -EINVAL;
	}

	has_reg_fault = true;
	bt_mesh_fault_update(bt_mesh_model_elem(model));

	return 0;
}

static const struct bt_mesh_health_srv_cb health_srv_cb = {
	.fault_get_cur = fault_get_cur,
	.fault_get_reg = fault_get_reg,
	.fault_clear = fault_clear,
	.fault_test = fault_test,
};

static struct bt_mesh_health_srv health_srv = {
	.cb = &health_srv_cb,
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, MAX_FAULT);

static struct bt_mesh_model root_models[] = {
	BT_MESH_MODEL_CFG_SRV,
	BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
};

static int vnd_publish(struct bt_mesh_model *mod)
{
	printk("Vendor publish\n");
	return 0;
}

BT_MESH_MODEL_PUB_DEFINE(vnd_pub, vnd_publish, 4);

BT_MESH_MODEL_PUB_DEFINE(vnd_pub2, NULL, 4);

static const struct bt_mesh_model_op vnd_ops[] = {
	BT_MESH_MODEL_OP_END,
};

static struct bt_mesh_model vnd_models[] = {
	BT_MESH_MODEL_VND(BT_COMP_ID_LF, 0x1234, vnd_ops, &vnd_pub, NULL),
	BT_MESH_MODEL_VND(BT_COMP_ID_LF, 0x4321, vnd_ops, &vnd_pub2, NULL),
};

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(0, root_models, vnd_models),
};

static const struct bt_mesh_comp comp = {
	.cid = BT_COMP_ID_LF,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

#if 0
static int output_number(bt_mesh_output_action_t action, uint32_t number)
{
	printk("OOB Number: %u\n", number);

	board_output_number(action, number);

	return 0;
}
#endif

static void prov_complete(uint16_t net_idx, uint16_t addr)
{
	board_prov_complete();

	if (IS_ENABLED(CONFIG_BT_MESH_IV_UPDATE_TEST)) {
		bt_mesh_iv_update_test(true);
	}
}

static void prov_reset(void)
{
	bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
}

static const uint8_t dev_uuid[16] = { 0xdd, 0xdd };

static const struct bt_mesh_prov prov = {
	.uuid = dev_uuid,
#if 0
	.output_size = 4,
	.output_actions = BT_MESH_DISPLAY_NUMBER,
	.output_number = output_number,
#endif
	.complete = prov_complete,
	.reset = prov_reset,
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

	/* Initialize publication messages with dummy data */
	net_buf_simple_add_le32(vnd_pub.msg, UINT32_MAX);
	net_buf_simple_add_le32(vnd_pub2.msg, UINT32_MAX);

	bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);

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
