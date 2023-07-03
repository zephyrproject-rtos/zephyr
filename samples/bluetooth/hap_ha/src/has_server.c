/** @file
 *  @brief Bluetooth Hearing Access Service (HAS) Server role.
 *
 *  Copyright (c) 2022 Codecoup
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/audio/has.h>

#define PRESET_INDEX_UNIVERSAL  1
#define PRESET_INDEX_OUTDOOR    5
#define PRESET_INDEX_NOISY_ENV  8
#define PRESET_INDEX_OFFICE     22

static int select_cb(uint8_t index, bool sync)
{
	printk("%s %u sync %d", __func__, index, sync);

	return 0;
}

static void name_changed_cb(uint8_t index, const char *name)
{
	printk("%s %u name %s", __func__, index, name);
}

static const struct bt_has_preset_ops ops = {
	.select = select_cb,
	.name_changed = name_changed_cb,
};

int has_server_preset_init(void)
{
	int err;

	struct bt_has_preset_register_param param[] = {
		{
			.index = PRESET_INDEX_UNIVERSAL,
			.properties = BT_HAS_PROP_AVAILABLE | BT_HAS_PROP_WRITABLE,
			.name = "Universal",
			.ops = &ops,
		},
		{
			.index = PRESET_INDEX_OUTDOOR,
			.properties = BT_HAS_PROP_AVAILABLE | BT_HAS_PROP_WRITABLE,
			.name = "Outdoor",
			.ops = &ops,
		},
		{
			.index = PRESET_INDEX_NOISY_ENV,
			.properties = BT_HAS_PROP_AVAILABLE | BT_HAS_PROP_WRITABLE,
			.name = "Noisy environment",
			.ops = &ops,
		},
		{
			.index = PRESET_INDEX_OFFICE,
			.properties = BT_HAS_PROP_AVAILABLE | BT_HAS_PROP_WRITABLE,
			.name = "Office",
			.ops = &ops,
		},
	};

	for (size_t i = 0; i < ARRAY_SIZE(param); i++) {
		err = bt_has_preset_register(&param[i]);
		if (err != 0) {
			return err;
		}
	}

	return 0;
}

static struct bt_has_features_param features = {
	.type = BT_HAS_HEARING_AID_TYPE_MONAURAL,
	.preset_sync_support = false,
	.independent_presets = false
};

int has_server_init(void)
{
	int err;

	if (IS_ENABLED(CONFIG_HAP_HA_HEARING_AID_BINAURAL)) {
		features.type = BT_HAS_HEARING_AID_TYPE_BINAURAL;
	} else if (IS_ENABLED(CONFIG_HAP_HA_HEARING_AID_BANDED)) {
		features.type = BT_HAS_HEARING_AID_TYPE_BANDED;
	}

	err = bt_has_register(&features);
	if (err) {
		return err;
	}

	if (IS_ENABLED(CONFIG_BT_HAS_PRESET_SUPPORT)) {
		return has_server_preset_init();
	}

	return 0;
}
