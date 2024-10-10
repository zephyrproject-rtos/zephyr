/*
 * Copyright (c) 2024 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/bas.h>
#include "btp/btp.h"
#include "btp/btp_bas.h"

static uint8_t bas_supported_commands(const void *cmd, uint16_t cmd_len, void *rsp,
				      uint16_t *rsp_len)
{
	struct btp_bas_read_supported_commands_rp *rp = rsp;

	/* octet 0 */
	tester_set_bit(rp->data, BTP_BAS_READ_SUPPORTED_COMMANDS);
	tester_set_bit(rp->data, BTP_BAS_SET_BATTERY_LEVEL);
	tester_set_bit(rp->data, BTP_BAS_BLS_SET_BATTERY_PRESENT);
	tester_set_bit(rp->data, BTP_BAS_BLS_SET_WIRED_POWER_SOURCE);

	/* octet 1 */
	tester_set_bit(rp->data, BTP_BAS_BLS_SET_WIRELESS_POWER_SOURCE);
	tester_set_bit(rp->data, BTP_BAS_BLS_SET_BATTERY_CHARGE_STATE);
	tester_set_bit(rp->data, BTP_BAS_BLS_SET_BATTERY_CHARGE_LEVEL);
	tester_set_bit(rp->data, BTP_BAS_BLS_SET_BATTERY_CHARGE_TYPE);
	tester_set_bit(rp->data, BTP_BAS_BLS_SET_CHARGING_FAULT_REASON);
	tester_set_bit(rp->data, BTP_BAS_BLS_SET_IDENTIFIER);
	tester_set_bit(rp->data, BTP_BAS_BLS_SET_SERVICE_REQUIRED);
	tester_set_bit(rp->data, BTP_BAS_BLS_SET_BATTERY_FAULT);

	*rsp_len = sizeof(*rp) + 2;

	return BTP_STATUS_SUCCESS;
}

static uint8_t btp_bas_set_battery_level(const void *cmd, uint16_t cmd_len, void *rsp,
					 uint16_t *rsp_len)
{
	const struct btp_bas_set_battery_level_cmd *cp =
		(const struct btp_bas_set_battery_level_cmd *)cmd;
	uint8_t level = cp->level;
	int err = bt_bas_set_battery_level(level);

	return err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS;
}

#if defined(CONFIG_BT_BAS_BLS)
static uint8_t btp_bas_bls_set_battery_present(const void *cmd, uint16_t cmd_len, void *rsp,
					       uint16_t *rsp_len)
{
	const struct btp_bas_bls_set_battery_present_cmd *cp =
		(const struct btp_bas_bls_set_battery_present_cmd *)cmd;
	enum bt_bas_bls_battery_present present = cp->present;

	if (present != 0 && present != 1) {
		return BTP_STATUS_FAILED;
	}

	bt_bas_bls_set_battery_present(present);

	return BTP_STATUS_SUCCESS;
}

static uint8_t btp_bas_bls_set_wired_power_source(const void *cmd, uint16_t cmd_len, void *rsp,
						  uint16_t *rsp_len)
{
	const struct btp_bas_bls_set_wired_power_source_cmd *cp =
		(const struct btp_bas_bls_set_wired_power_source_cmd *)cmd;
	enum bt_bas_bls_wired_power_source source = cp->source;
	
	if (source < 0 || source > 2) {
		return BTP_STATUS_FAILED;
	}

	bt_bas_bls_set_wired_external_power_source(source);

	return BTP_STATUS_SUCCESS;
}

static uint8_t btp_bas_bls_set_wireless_power_source(const void *cmd, uint16_t cmd_len, void *rsp,
						     uint16_t *rsp_len)
{
	const struct btp_bas_bls_set_wireless_power_source_cmd *cp =
		(const struct btp_bas_bls_set_wireless_power_source_cmd *)cmd;
	enum bt_bas_bls_wireless_power_source source = cp->source;
	
	if (source < 0 || source > 2) {
		return BTP_STATUS_FAILED;
	}

	bt_bas_bls_set_wireless_external_power_source(source);

	return BTP_STATUS_SUCCESS;
}

static uint8_t btp_bas_bls_set_battery_charge_state(const void *cmd, uint16_t cmd_len, void *rsp,
						    uint16_t *rsp_len)
{
	const struct btp_bas_bls_set_battery_charge_state_cmd *cp =
		(const struct btp_bas_bls_set_battery_charge_state_cmd *)cmd;
	enum bt_bas_bls_battery_charge_state state = cp->state;
	
	f (state < 0 || state > 3) {
		return BTP_STATUS_FAILED;
	}

	bt_bas_bls_set_battery_charge_state(state);

	return BTP_STATUS_SUCCESS;
}

static uint8_t btp_bas_bls_set_battery_charge_level(const void *cmd, uint16_t cmd_len, void *rsp,
						    uint16_t *rsp_len)
{
	const struct btp_bas_bls_set_battery_charge_level_cmd *cp =
		(const struct btp_bas_bls_set_battery_charge_level_cmd *)cmd;
	enum bt_bas_bls_battery_charge_level level = cp->level;
	
	if (level < 0 || level > 3) {
		return BTP_STATUS_FAILED;
	}

	bt_bas_bls_set_battery_charge_level(level);

	return BTP_STATUS_SUCCESS;
}

static uint8_t btp_bas_bls_set_battery_charge_type(const void *cmd, uint16_t cmd_len, void *rsp,
						   uint16_t *rsp_len)
{
	const struct btp_bas_bls_set_battery_charge_type_cmd *cp =
		(const struct btp_bas_bls_set_battery_charge_type_cmd *)cmd;
	enum bt_bas_bls_battery_charge_type type = cp->type;
	
	if (type < 0 || type > 4) {
		return BTP_STATUS_FAILED;
	}

	bt_bas_bls_set_battery_charge_type(type);

	return BTP_STATUS_SUCCESS;
}

static uint8_t btp_bas_bls_set_charging_fault_reason(const void *cmd, uint16_t cmd_len, void *rsp,
						     uint16_t *rsp_len)
{
	const struct btp_bas_bls_set_charging_fault_reason_cmd *cp =
		(const struct btp_bas_bls_set_charging_fault_reason_cmd *)cmd;
	enum bt_bas_bls_charging_fault_reason reason = cp->reason;
	
	if (reason != BT_BAS_BLS_FAULT_REASON_NONE ||
		reason != BT_BAS_BLS_FAULT_REASON_BATTERY ||
		reason != BT_BAS_BLS_FAULT_REASON_EXTERNAL_POWER ||
		reason != BT_BAS_BLS_FAULT_REASON_OTHER) {
		return BTP_STATUS_FAILED;
	}

	bt_bas_bls_set_charging_fault_reason(reason);

	return BTP_STATUS_SUCCESS;
}

#if defined(CONFIG_BT_BAS_BLS_IDENTIFIER_PRESENT)
static uint8_t btp_bas_bls_set_identifier(const void *cmd, uint16_t cmd_len, void *rsp,
					  uint16_t *rsp_len)
{
	const struct btp_bas_bls_set_identifier_cmd *cp =
		(const struct btp_bas_bls_set_identifier_cmd *)cmd;
	uint16_t identifier = sys_le16_to_cpu(cp->identifier);

	bt_bas_bls_set_identifier(identifier);

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_BAS_BLS_IDENTIFIER_PRESENT */

#if defined(CONFIG_BT_BAS_BLS_ADDITIONAL_STATUS_PRESENT)
static uint8_t btp_bas_bls_set_service_required(const void *cmd, uint16_t cmd_len, void *rsp,
						uint16_t *rsp_len)
{
	const struct btp_bas_bls_set_service_required_cmd *cp =
		(const struct btp_bas_bls_set_service_required_cmd *)cmd;
	enum bt_bas_bls_service_required value = cp->service_required;
	
	if (value < 0 || value > 2) {
		return BTP_STATUS_FAILED;
	}

	bt_bas_bls_set_service_required(value);

	return BTP_STATUS_SUCCESS;
}

static uint8_t btp_bas_bls_set_battery_fault(const void *cmd, uint16_t cmd_len, void *rsp,
					     uint16_t *rsp_len)
{
	const struct btp_bas_bls_set_battery_fault_cmd *cp =
		(const struct btp_bas_bls_set_battery_fault_cmd *)cmd;
	enum bt_bas_bls_battery_fault value = cp->battery_fault;
	
	if (value != 0 || value != 1) {
		return BTP_STATUS_FAILED;
	}

	bt_bas_bls_set_battery_fault(value);

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_BAS_BLS_ADDITIONAL_STATUS_PRESENT */

#endif /* CONFIG_BT_BAS_BLS */

static const struct btp_handler handlers[] = {
	{
		.opcode = BTP_BAS_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX,
		.expect_len = 0,
		.func = bas_supported_commands,
	},
	{
		.opcode = BTP_BAS_SET_BATTERY_LEVEL,
		.index = BTP_INDEX,
		.expect_len = sizeof(struct btp_bas_set_battery_level_cmd),
		.func = btp_bas_set_battery_level,
	},
#if defined(CONFIG_BT_BAS_BLS)
	{
		.opcode = BTP_BAS_BLS_SET_BATTERY_PRESENT,
		.index = BTP_INDEX,
		.expect_len = sizeof(struct btp_bas_bls_set_battery_present_cmd),
		.func = btp_bas_bls_set_battery_present,
	},
	{
		.opcode = BTP_BAS_BLS_SET_WIRED_POWER_SOURCE,
		.index = BTP_INDEX,
		.expect_len = sizeof(struct btp_bas_bls_set_wired_power_source_cmd),
		.func = btp_bas_bls_set_wired_power_source,
	},
	{
		.opcode = BTP_BAS_BLS_SET_WIRELESS_POWER_SOURCE,
		.index = BTP_INDEX,
		.expect_len = sizeof(struct btp_bas_bls_set_wireless_power_source_cmd),
		.func = btp_bas_bls_set_wireless_power_source,
	},
	{
		.opcode = BTP_BAS_BLS_SET_BATTERY_CHARGE_STATE,
		.index = BTP_INDEX,
		.expect_len = sizeof(struct btp_bas_bls_set_battery_charge_state_cmd),
		.func = btp_bas_bls_set_battery_charge_state,
	},
	{
		.opcode = BTP_BAS_BLS_SET_BATTERY_CHARGE_LEVEL,
		.index = BTP_INDEX,
		.expect_len = sizeof(struct btp_bas_bls_set_battery_charge_level_cmd),
		.func = btp_bas_bls_set_battery_charge_level,
	},
	{
		.opcode = BTP_BAS_BLS_SET_BATTERY_CHARGE_TYPE,
		.index = BTP_INDEX,
		.expect_len = sizeof(struct btp_bas_bls_set_battery_charge_type_cmd),
		.func = btp_bas_bls_set_battery_charge_type,
	},
	{
		.opcode = BTP_BAS_BLS_SET_CHARGING_FAULT_REASON,
		.index = BTP_INDEX,
		.expect_len = sizeof(struct btp_bas_bls_set_charging_fault_reason_cmd),
		.func = btp_bas_bls_set_charging_fault_reason,
	},
#if defined(CONFIG_BT_BAS_BLS_IDENTIFIER_PRESENT)
	{
		.opcode = BTP_BAS_BLS_SET_IDENTIFIER,
		.index = BTP_INDEX,
		.expect_len = sizeof(struct btp_bas_bls_set_identifier_cmd),
		.func = btp_bas_bls_set_identifier,
	},
#endif /* CONFIG_BT_BAS_BLS_IDENTIFIER_PRESENT */
#if defined(CONFIG_BT_BAS_BLS_ADDITIONAL_STATUS_PRESENT)
	{
		.opcode = BTP_BAS_BLS_SET_SERVICE_REQUIRED,
		.index = BTP_INDEX,
		.expect_len = sizeof(struct btp_bas_bls_set_service_required_cmd),
		.func = btp_bas_bls_set_service_required,
	},
	{
		.opcode = BTP_BAS_BLS_SET_BATTERY_FAULT,
		.index = BTP_INDEX,
		.expect_len = sizeof(struct btp_bas_bls_set_battery_fault_cmd),
		.func = btp_bas_bls_set_battery_fault,
	},
#endif /* CONFIG_BT_BAS_BLS_ADDITIONAL_STATUS_PRESENT */
#endif /* CONFIG_BT_BAS_BLS */
};

uint8_t tester_init_bas(void)
{
	tester_register_command_handlers(BTP_SERVICE_ID_BAS, handlers, ARRAY_SIZE(handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_bas(void)
{
	return BTP_STATUS_SUCCESS;
}
