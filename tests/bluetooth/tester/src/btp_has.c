/* btp_has.c - Bluetooth HAS Tester */

/*
 * Copyright (c) 2023 Oticon
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/bluetooth/audio/has.h>

#include "btp/btp.h"
#include <zephyr/sys/byteorder.h>
#include <zephyr/arch/common/ffs.h>
#include <stdint.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME bttester_has
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

static uint8_t has_supported_commands(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	struct btp_has_read_supported_commands_rp *rp = rsp;

	/* octet 0 */
	tester_set_bit(rp->data, BTP_HAS_READ_SUPPORTED_COMMANDS);

	*rsp_len = sizeof(*rp) + 1;

	return BTP_STATUS_SUCCESS;
}

static uint8_t has_set_active_index(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_has_set_active_index_cmd *cp = cmd;
	int err = bt_has_preset_active_set(cp->index);

	return (err) ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS;
}

static uint16_t has_presets;
static char	temp_name[BT_HAS_PRESET_NAME_MAX + 1];

static uint8_t has_set_preset_name(const void *cmd, uint16_t cmd_len,
				   void *rsp, uint16_t *rsp_len)
{
	const struct btp_has_set_preset_name_cmd *cp = cmd;
	const uint16_t fixed_size = sizeof(*cp);
	int err = -1;

	if (cmd_len >= fixed_size && cmd_len >= (fixed_size + cp->length)) {
		int name_len = MIN(cp->length, BT_HAS_PRESET_NAME_MAX);

		memcpy(temp_name, cp->name, name_len);
		temp_name[name_len] = '\0';
		err = bt_has_preset_name_change(cp->index, temp_name);
	}
	return (err) ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS;
}

static uint8_t has_remove_preset(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_has_remove_preset_cmd *cp = cmd;
	int err = 0;

	if (cp->index == BT_HAS_PRESET_INDEX_NONE) {
		while (has_presets) {
			uint8_t index = find_lsb_set(has_presets);

			err = bt_has_preset_unregister(index);
			if (err) {
				break;
			}
			has_presets &= ~(1 << (index - 1));
		}
	} else {
		err = bt_has_preset_unregister(cp->index);
		if (!err) {
			has_presets &= ~(1 << (cp->index - 1));
		}
	}
	return (err) ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS;
}

static int has_preset_selected(unsigned char index, bool sync)
{
	return BTP_STATUS_SUCCESS;
}

static const struct bt_has_preset_ops has_preset_ops = {
	has_preset_selected, NULL
};

static uint8_t has_add_preset(const void *cmd, uint16_t cmd_len,
			      void *rsp, uint16_t *rsp_len)
{
	const struct btp_has_add_preset_cmd *cp = cmd;
	const uint16_t fixed_size = sizeof(*cp);
	int err = -1;

	if (cmd_len >= fixed_size && cmd_len >= (fixed_size + cp->length)) {
		int name_len = MIN(cp->length, BT_HAS_PRESET_NAME_MAX);

		memcpy(temp_name, cp->name, name_len);
		temp_name[name_len] = '\0';
		struct bt_has_preset_register_param preset_params = {
			cp->index, cp->props, temp_name, &has_preset_ops
		};
		err = bt_has_preset_register(&preset_params);
		if (!err) {
			has_presets |= 1 << (cp->index - 1);
		}
	}
	return (err) ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS;
}

static uint8_t has_set_properties(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_has_set_properties_cmd *cp = cmd;
	int err = (cp->props & BT_HAS_PROP_AVAILABLE) ?
		bt_has_preset_available(cp->index) :
		bt_has_preset_unavailable(cp->index);

	return (err) ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS;
}

static const struct btp_handler has_handlers[] = {
	{
		.opcode = BTP_HAS_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = has_supported_commands
	},
	{
		.opcode = BTP_HAS_SET_ACTIVE_INDEX,
		.expect_len = sizeof(struct btp_has_set_active_index_cmd),
		.func = has_set_active_index
	},
	{
		.opcode = BTP_HAS_SET_PRESET_NAME,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = has_set_preset_name
	},
	{
		.opcode = BTP_HAS_REMOVE_PRESET,
		.expect_len = sizeof(struct btp_has_remove_preset_cmd),
		.func = has_remove_preset
	},
	{
		.opcode = BTP_HAS_ADD_PRESET,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = has_add_preset
	},
	{
		.opcode = BTP_HAS_SET_PROPERTIES,
		.expect_len = sizeof(struct btp_has_set_properties_cmd),
		.func = has_set_properties
	}
};

static const char *preset_name(uint8_t index)
{
	switch (index) {
	case 0: return "PRESET_0";
	case 1: return "PRESET_1";
	case 2: return "PRESET_2";
	case 3: return "PRESET_3";
	case 4: return "PRESET_4";
	case 5: return "PRESET_5";
	case 6: return "PRESET_6";
	case 7: return "PRESET_7";
	default: return "PRESET_?";
	}
}

#define PRESETS_CONFIGURED 3
#define LAST_PRESET	   MIN(PRESETS_CONFIGURED, CONFIG_BT_HAS_PRESET_COUNT)

uint8_t tester_init_has(void)
{
	tester_register_command_handlers(BTP_SERVICE_ID_HAS, has_handlers,
					 ARRAY_SIZE(has_handlers));

	struct bt_has_features_param params = {
		BT_HAS_HEARING_AID_TYPE_BINAURAL, false, true
	};
	int err = bt_has_register(&params);

	if (!err) {
		for (uint8_t index = 1; index <= LAST_PRESET; index++) {
			enum bt_has_properties properties = (index < PRESETS_CONFIGURED) ?
				BT_HAS_PROP_WRITABLE | BT_HAS_PROP_AVAILABLE :
				BT_HAS_PROP_WRITABLE;
			struct bt_has_preset_register_param preset_params = {
				index, properties, preset_name(index), &has_preset_ops
			};

			err = bt_has_preset_register(&preset_params);
			if (!err) {
				has_presets |= 1 << (index - 1);
			} else {
				break;
			}
		}
	}

	return (err) ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_has(void)
{
	return BTP_STATUS_SUCCESS;
}
