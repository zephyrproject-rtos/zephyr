/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * Copyright (c) 2022 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include <sys/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/audio/audio.h>
#include <bluetooth/audio/mics.h>
#include <bluetooth/audio/vcs.h>
#include <sys/byteorder.h>

static struct bt_vcs *vcs;

static void vcs_state_cb(struct bt_vcs *vcs, int err, uint8_t volume,
			 uint8_t mute)
{
	if (err) {
		printk("VCS state get failed (%d)\n", err);
	} else {
		printk("VCS volume %u, mute %u\n", volume, mute);
	}
}

static void vcs_flags_cb(struct bt_vcs *vcs, int err, uint8_t flags)
{
	if (err) {
		printk("VCS flags get failed (%d)\n", err);
	} else {
		printk("VCS flags 0x%02X\n", flags);
	}
}

static struct bt_vcs_cb vcs_cbs = {
	.state = vcs_state_cb,
	.flags = vcs_flags_cb,
};

#if CONFIG_BT_VCS_AICS_INSTANCE_COUNT > 0
static void aics_state_cb(struct bt_aics *inst, int err, int8_t gain,
			  uint8_t mute, uint8_t mode)
{
	if (err) {
		printk("AICS state get failed (%d) for inst %p\n", err, inst);
	} else {
		printk("AICS inst %p state gain %d, mute %u, mode %u\n", inst, gain, mute, mode);
	}
}

static void aics_gain_setting_cb(struct bt_aics *inst, int err, uint8_t units,
				 int8_t minimum, int8_t maximum)
{
	if (err) {
		printk("AICS gain settings get failed (%d) for inst %p\n", err, inst);
	} else {
		printk("AICS inst %p gain settings units %u, min %d, max %d\n", inst, units,
		       minimum, maximum);
	}
}

static void aics_input_type_cb(struct bt_aics *inst, int err,
			       uint8_t input_type)
{
	if (err) {
		printk("AICS input type get failed (%d) for inst %p\n", err, inst);
	} else {
		printk("AICS inst %p input type %u\n", inst, input_type);
	}
}

static void aics_status_cb(struct bt_aics *inst, int err, bool active)
{
	if (err) {
		printk("AICS status get failed (%d) for inst %p\n", err, inst);
	} else {
		printk("AICS inst %p status %s\n", inst, active ? "active" : "inactive");
	}

}
static void aics_description_cb(struct bt_aics *inst, int err,
				char *description)
{
	if (err) {
		printk("AICS description get failed (%d) for inst %p\n", err, inst);
	} else {
		printk("AICS inst %p description %s\n", inst, description);
	}
}

static struct bt_aics_cb aics_cbs = {
	.state = aics_state_cb,
	.gain_setting = aics_gain_setting_cb,
	.type = aics_input_type_cb,
	.status = aics_status_cb,
	.description = aics_description_cb
};
#endif /* CONFIG_BT_VCS_AICS_INSTANCE_COUNT */

#if CONFIG_BT_VCS_VOCS_INSTANCE_COUNT > 0
static void vocs_state_cb(struct bt_vocs *inst, int err, int16_t offset)
{
	if (err) {
		printk("VOCS state get failed (%d) for inst %p\n", err, inst);
	} else {
		printk("VOCS inst %p offset %d\n", inst, offset);
	}
}

static void vocs_location_cb(struct bt_vocs *inst, int err, uint32_t location)
{
	if (err) {
		printk("VOCS location get failed (%d) for inst %p\n", err, inst);
	} else {
		printk("VOCS inst %p location %u\n", inst, location);
	}
}

static void vocs_description_cb(struct bt_vocs *inst, int err,
				char *description)
{
	if (err) {
		printk("VOCS description get failed (%d) for inst %p\n", err, inst);
	} else {
		printk("VOCS inst %p description %s\n", inst, description);
	}
}

static struct bt_vocs_cb vocs_cbs = {
	.state = vocs_state_cb,
	.location = vocs_location_cb,
	.description = vocs_description_cb
};
#endif /* CONFIG_BT_VCS_VOCS_INSTANCE_COUNT */

static int vcs_init(void)
{
	struct bt_vcs_register_param param;

	memset(&param, 0, sizeof(param));

#if CONFIG_BT_VCS_VOCS_INSTANCE_COUNT > 0
	char output_desc[CONFIG_BT_VCS_VOCS_INSTANCE_COUNT][16];

	for (int i = 0; i < ARRAY_SIZE(param.vocs_param); i++) {
		param.vocs_param[i].location_writable = true;
		param.vocs_param[i].desc_writable = true;
		snprintf(output_desc[i], sizeof(output_desc[i]),
			 "Output %d", i + 1);
		param.vocs_param[i].output_desc = output_desc[i];
		param.vocs_param[i].cb = &vocs_cbs;
	}
#endif /* CONFIG_BT_VCS_VOCS_INSTANCE_COUNT */

#if CONFIG_BT_VCS_AICS_INSTANCE_COUNT > 0
	char input_desc[CONFIG_BT_VCS_AICS_INSTANCE_COUNT][16];

	for (int i = 0; i < ARRAY_SIZE(param.aics_param); i++) {
		param.aics_param[i].desc_writable = true;
		snprintf(input_desc[i], sizeof(input_desc[i]),
			 "Input %d", i + 1);
		param.aics_param[i].description = input_desc[i];
		param.aics_param[i].type = BT_AICS_INPUT_TYPE_UNSPECIFIED;
		param.aics_param[i].status = true;
		param.aics_param[i].gain_mode = BT_AICS_MODE_MANUAL;
		param.aics_param[i].units = 1;
		param.aics_param[i].min_gain = -100;
		param.aics_param[i].max_gain = 100;
		param.aics_param[i].cb = &aics_cbs;
	}
#endif /* CONFIG_BT_VCS_AICS_INSTANCE_COUNT */

	param.step = 1;
	param.mute = BT_VCS_STATE_UNMUTED;
	param.volume = 100;

	param.cb = &vcs_cbs;

	return bt_vcs_register(&param, &vcs);
}

#if defined(CONFIG_BT_MICS)
static struct bt_mics *mics;

static void mics_mute_cb(struct bt_mics *mics, int err, uint8_t mute)
{
	if (err != 0) {
		printk("Mute get failed (%d)\n", err);
	} else {
		printk("Mute value %u\n", mute);
	}
}

static struct bt_mics_cb mics_cbs = {
	.mute = mics_mute_cb,
};

#if CONFIG_BT_MICS_AICS_INSTANCE_COUNT > 0
static void mics_aics_state_cb(struct bt_aics *inst, int err, int8_t gain,
			       uint8_t mute, uint8_t mode)
{
	if (err != 0) {
		printk("AICS state get failed (%d) for inst %p\n", err, inst);
	} else {
		printk("AICS inst %p state gain %d, mute %u, mode %u\n", inst, gain, mute, mode);
	}

}
static void mics_aics_gain_setting_cb(struct bt_aics *inst, int err,
				      uint8_t units, int8_t minimum,
				      int8_t maximum)
{
	if (err != 0) {
		printk("AICS gain settings get failed (%d) for inst %p\n", err, inst);
	} else {
		printk("AICS inst %p gain settings units %u, min %d, max %d\n", inst, units,
		       minimum, maximum);
	}

}
static void mics_aics_input_type_cb(struct bt_aics *inst, int err,
				    uint8_t input_type)
{
	if (err != 0) {
		printk("AICS input type get failed (%d) for inst %p\n", err, inst);
	} else {
		printk("AICS inst %p input type %u\n", inst, input_type);
	}

}
static void mics_aics_status_cb(struct bt_aics *inst, int err, bool active)
{
	if (err != 0) {
		printk("AICS status get failed (%d) for inst %p\n", err, inst);
	} else {
		printk("AICS inst %p status %s\n", inst, active ? "active" : "inactive");
	}

}
static void mics_aics_description_cb(struct bt_aics *inst, int err,
				     char *description)
{
	if (err != 0) {
		printk("AICS description get failed (%d) for inst %p\n", err, inst);
	} else {
		printk("AICS inst %p description %s\n", inst, description);
	}
}

static struct bt_aics_cb aics_cb = {
	.state = mics_aics_state_cb,
	.gain_setting = mics_aics_gain_setting_cb,
	.type = mics_aics_input_type_cb,
	.status = mics_aics_status_cb,
	.description = mics_aics_description_cb,
};
#endif /* CONFIG_BT_MICS_AICS_INSTANCE_COUNT > 0 */

static int mics_init(void)
{
	struct bt_mics_register_param mics_param;

	(void)memset(&mics_param, 0, sizeof(mics_param));

#if CONFIG_BT_MICS_AICS_INSTANCE_COUNT > 0
	char input_desc[CONFIG_BT_MICS_AICS_INSTANCE_COUNT][16];

	for (int i = 0; i < ARRAY_SIZE(mics_param.aics_param); i++) {
		mics_param.aics_param[i].desc_writable = true;
		snprintf(input_desc[i], sizeof(input_desc[i]),
			 "Input %d", i + 1);
		mics_param.aics_param[i].description = input_desc[i];
		mics_param.aics_param[i].type = BT_AICS_INPUT_TYPE_UNSPECIFIED;
		mics_param.aics_param[i].status = true;
		mics_param.aics_param[i].gain_mode = BT_AICS_MODE_MANUAL;
		mics_param.aics_param[i].units = 1;
		mics_param.aics_param[i].min_gain = -100;
		mics_param.aics_param[i].max_gain = 100;
		mics_param.aics_param[i].cb = &aics_cb;
	}
#endif /* CONFIG_BT_MICS_AICS_INSTANCE_COUNT > 0 */

	mics_param.cb = &mics_cbs;

	return bt_mics_register(&mics_param, &mics);
}
#endif /* CONFIG_BT_MICS */

int hearing_aid_volume_init(void)
{
        int err;

        err = vcs_init();
	if (err) {
		printk("VCS init failed (err %d)\n", err);
		return err;
	}

	printk("VCS initialized\n");

#if defined(CONFIG_BT_MICS)
	err = mics_init();
	if (err) {
		printk("MICS init failed (err %d)\n", err);
		return err;
	}

	printk("MICS initialized\n");
#endif /* CONFIG_BT_MICS */

        return 0;
}
