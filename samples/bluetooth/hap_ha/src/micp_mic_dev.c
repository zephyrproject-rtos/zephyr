/** @file
 *  @brief Bluetooth Microphone Input Control Profile (MICP) Microphone Device role.
 *
 *  Copyright (c) 2020 Bose Corporation
 *  Copyright (c) 2020-2022 Nordic Semiconductor ASA
 *  Copyright (c) 2022 Codecoup
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <stdlib.h>
#include <stdio.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/micp.h>

static void micp_mic_dev_mute_cb(uint8_t mute)
{
	printk("Mute value %u\n", mute);
}

static struct bt_micp_mic_dev_cb micp_mic_dev_cbs = {
	.mute = micp_mic_dev_mute_cb,
};

#if defined(CONFIG_BT_MICP_MIC_DEV_AICS)
static struct bt_micp_included micp_included;

static void micp_mic_dev_aics_state_cb(struct bt_aics *inst, int err, int8_t gain, uint8_t mute,
				       uint8_t mode)
{
	if (err != 0) {
		printk("AICS state get failed (%d) for inst %p\n", err, inst);
	} else {
		printk("AICS inst %p state gain %d, mute %u, mode %u\n",
		       inst, gain, mute, mode);
	}

}
static void micp_mic_dev_aics_gain_setting_cb(struct bt_aics *inst, int err, uint8_t units,
					      int8_t minimum, int8_t maximum)
{
	if (err != 0) {
		printk("AICS gain settings get failed (%d) for inst %p\n", err, inst);
	} else {
		printk("AICS inst %p gain settings units %u, min %d, max %d\n",
		       inst, units, minimum, maximum);
	}

}
static void micp_mic_dev_aics_input_type_cb(struct bt_aics *inst, int err, uint8_t input_type)
{
	if (err != 0) {
		printk("AICS input type get failed (%d) for inst %p\n", err, inst);
	} else {
		printk("AICS inst %p input type %u\n", inst, input_type);
	}

}
static void micp_mic_dev_aics_status_cb(struct bt_aics *inst, int err, bool active)
{
	if (err != 0) {
		printk("AICS status get failed (%d) for inst %p\n", err, inst);
	} else {
		printk("AICS inst %p status %s\n", inst, active ? "active" : "inactive");
	}

}
static void micp_mic_dev_aics_description_cb(struct bt_aics *inst, int err, char *description)
{
	if (err != 0) {
		printk("AICS description get failed (%d) for inst %p\n", err, inst);
	} else {
		printk("AICS inst %p description %s\n", inst, description);
	}
}

static struct bt_aics_cb aics_cb = {
	.state = micp_mic_dev_aics_state_cb,
	.gain_setting = micp_mic_dev_aics_gain_setting_cb,
	.type = micp_mic_dev_aics_input_type_cb,
	.status = micp_mic_dev_aics_status_cb,
	.description = micp_mic_dev_aics_description_cb,
};
#endif /* CONFIG_BT_MICP_MIC_DEV_AICS */

int micp_mic_dev_init(void)
{
	int err;
	struct bt_micp_mic_dev_register_param micp_param;

	(void)memset(&micp_param, 0, sizeof(micp_param));

#if defined(CONFIG_BT_MICP_MIC_DEV_AICS)
	char input_desc[CONFIG_BT_MICP_MIC_DEV_AICS_INSTANCE_COUNT][16];

	for (int i = 0; i < ARRAY_SIZE(micp_param.aics_param); i++) {
		micp_param.aics_param[i].desc_writable = true;
		snprintf(input_desc[i], sizeof(input_desc[i]), "Input %d", i + 1);
		micp_param.aics_param[i].description = input_desc[i];
		micp_param.aics_param[i].type = BT_AICS_INPUT_TYPE_UNSPECIFIED;
		micp_param.aics_param[i].status = true;
		micp_param.aics_param[i].gain_mode = BT_AICS_MODE_MANUAL;
		micp_param.aics_param[i].units = 1;
		micp_param.aics_param[i].min_gain = -100;
		micp_param.aics_param[i].max_gain = 100;
		micp_param.aics_param[i].cb = &aics_cb;
	}
#endif /* CONFIG_BT_MICP_MIC_DEV_AICS */

	micp_param.cb = &micp_mic_dev_cbs;

	err = bt_micp_mic_dev_register(&micp_param);
	if (err != 0) {
		return err;
	}

#if defined(CONFIG_BT_MICP_MIC_DEV_AICS)
	err = bt_micp_mic_dev_included_get(&micp_included);
	if (err != 0) {
		return err;
	}
#endif /* CONFIG_BT_MICP_MIC_DEV_AICS */

	return 0;
}
