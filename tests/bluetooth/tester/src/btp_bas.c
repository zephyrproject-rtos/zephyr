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
#if defined(CONFIG_BT_BAS_BLS)
	tester_set_bit(rp->data, BTP_BAS_BLS_SET_BATTERY_PRESENT);
	tester_set_bit(rp->data, BTP_BAS_BLS_SET_WIRED_POWER_SOURCE);
	tester_set_bit(rp->data, BTP_BAS_BLS_SET_WIRELESS_POWER_SOURCE);
	tester_set_bit(rp->data, BTP_BAS_BLS_SET_BATTERY_CHARGE_STATE);
	tester_set_bit(rp->data, BTP_BAS_BLS_SET_BATTERY_CHARGE_LEVEL);
	/* octet 1 */
	tester_set_bit(rp->data, BTP_BAS_BLS_SET_BATTERY_CHARGE_TYPE);
	tester_set_bit(rp->data, BTP_BAS_BLS_SET_CHARGING_FAULT_REASON);
#if defined(CONFIG_BT_BAS_BLS_IDENTIFIER_PRESENT)
	tester_set_bit(rp->data, BTP_BAS_BLS_SET_IDENTIFIER);
#endif /* CONFIG_BT_BAS_BLS_IDENTIFIER_PRESENT */
	tester_set_bit(rp->data, BTP_BAS_BLS_SET_SERVICE_REQUIRED);
	tester_set_bit(rp->data, BTP_BAS_BLS_SET_BATTERY_FAULT);
#endif /* CONFIG_BT_BAS_BLS */

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

	if (cp->present != BT_BAS_BLS_BATTERY_NOT_PRESENT &&
	    cp->present != BT_BAS_BLS_BATTERY_PRESENT) {
		return BTP_STATUS_FAILED;
	}

	/*
	 * BTP Values:
	 *	Battery is not present.
	 *	BTP_BAS_BLS_BATTERY_NOT_PRESENT = 0,
	 *
	 *	Battery is present.
	 *	BTP_BAS_BLS_BATTERY_PRESENT = 1
	 */
	enum bt_bas_bls_battery_present present = cp->present;

	bt_bas_bls_set_battery_present(present);

	return BTP_STATUS_SUCCESS;
}

static uint8_t btp_bas_bls_set_wired_power_source(const void *cmd, uint16_t cmd_len, void *rsp,
						  uint16_t *rsp_len)
{
	const struct btp_bas_bls_set_wired_power_source_cmd *cp =
		(const struct btp_bas_bls_set_wired_power_source_cmd *)cmd;

	if (!IN_RANGE(cp->source, BT_BAS_BLS_WIRED_POWER_NOT_CONNECTED,
		      BT_BAS_BLS_WIRED_POWER_UNKNOWN)) {
		return BTP_STATUS_FAILED;
	}

	/*
	 * BTP Values:
	 * Wired external power source is not connected.
	 * BTP_BAS_BLS_WIRED_POWER_NOT_CONNECTED = 0,
	 *
	 * Wired external power source is connected.
	 * BTP_BAS_BLS_WIRED_POWER_CONNECTED = 1,
	 *
	 * Wired external power source status is unknown.
	 * BTP_BAS_BLS_WIRED_POWER_UNKNOWN = 2
	 */
	enum bt_bas_bls_wired_power_source source = cp->source;

	bt_bas_bls_set_wired_external_power_source(source);

	return BTP_STATUS_SUCCESS;
}

static uint8_t btp_bas_bls_set_wireless_power_source(const void *cmd, uint16_t cmd_len, void *rsp,
						     uint16_t *rsp_len)
{
	const struct btp_bas_bls_set_wireless_power_source_cmd *cp =
		(const struct btp_bas_bls_set_wireless_power_source_cmd *)cmd;

	if (!IN_RANGE(cp->source, BT_BAS_BLS_WIRELESS_POWER_NOT_CONNECTED,
		      BT_BAS_BLS_WIRELESS_POWER_UNKNOWN)) {
		return BTP_STATUS_FAILED;
	}

	/*
	 * BTP Values:
	 * Wireless external power source is not connected.
	 * BTP_BAS_BLS_WIRELESS_POWER_NOT_CONNECTED = 0,
	 *
	 * Wireless external power source is connected.
	 * BTP_BAS_BLS_WIRELESS_POWER_CONNECTED = 1,
	 *
	 * Wireless external power source status is unknown.
	 * BTP_BAS_BLS_WIRELESS_POWER_UNKNOWN = 2
	 */
	enum bt_bas_bls_wireless_power_source source = cp->source;

	bt_bas_bls_set_wireless_external_power_source(source);

	return BTP_STATUS_SUCCESS;
}

static uint8_t btp_bas_bls_set_battery_charge_state(const void *cmd, uint16_t cmd_len, void *rsp,
						    uint16_t *rsp_len)
{
	const struct btp_bas_bls_set_battery_charge_state_cmd *cp =
		(const struct btp_bas_bls_set_battery_charge_state_cmd *)cmd;

	if (!IN_RANGE(cp->state, BT_BAS_BLS_CHARGE_STATE_UNKNOWN,
		      BT_BAS_BLS_CHARGE_STATE_DISCHARGING_INACTIVE)) {
		return BTP_STATUS_FAILED;
	}

	/*
	 * BTP Values:
	 * Battery charge state is unknown.
	 * BTP_BAS_BLS_CHARGE_STATE_UNKNOWN = 0,
	 *
	 * Battery is currently charging.
	 * BTP_BAS_BLS_CHARGE_STATE_CHARGING = 1,
	 *
	 * Battery is discharging actively.
	 * BTP_BAS_BLS_CHARGE_STATE_DISCHARGING_ACTIVE = 2,
	 *
	 * Battery is discharging but inactive.
	 * BTP_BAS_BLS_CHARGE_STATE_DISCHARGING_INACTIVE = 3
	 */
	enum bt_bas_bls_battery_charge_state state = cp->state;

	bt_bas_bls_set_battery_charge_state(state);

	return BTP_STATUS_SUCCESS;
}

static uint8_t btp_bas_bls_set_battery_charge_level(const void *cmd, uint16_t cmd_len, void *rsp,
						    uint16_t *rsp_len)
{
	const struct btp_bas_bls_set_battery_charge_level_cmd *cp =
		(const struct btp_bas_bls_set_battery_charge_level_cmd *)cmd;

	if (!IN_RANGE(cp->level, BT_BAS_BLS_CHARGE_LEVEL_UNKNOWN,
		      BT_BAS_BLS_CHARGE_LEVEL_CRITICAL)) {
		return BTP_STATUS_FAILED;
	}

	/*
	 * BTP Values:
	 * Battery charge level is unknown.
	 * BTP_BAS_BLS_CHARGE_LEVEL_UNKNOWN = 0,
	 *
	 * Battery charge level is good.
	 * BTP_BAS_BLS_CHARGE_LEVEL_GOOD = 1,
	 *
	 * Battery charge level is low.
	 * BTP_BAS_BLS_CHARGE_LEVEL_LOW = 2,
	 *
	 * Battery charge level is critical.
	 * BTP_BAS_BLS_CHARGE_LEVEL_CRITICAL = 3
	 */
	enum bt_bas_bls_battery_charge_level level = cp->level;

	bt_bas_bls_set_battery_charge_level(level);

	return BTP_STATUS_SUCCESS;
}

static uint8_t btp_bas_bls_set_battery_charge_type(const void *cmd, uint16_t cmd_len, void *rsp,
						   uint16_t *rsp_len)
{
	const struct btp_bas_bls_set_battery_charge_type_cmd *cp =
		(const struct btp_bas_bls_set_battery_charge_type_cmd *)cmd;

	if (!IN_RANGE(cp->type, BT_BAS_BLS_CHARGE_TYPE_UNKNOWN,
		      BT_BAS_BLS_CHARGE_TYPE_FLOAT)) {
		return BTP_STATUS_FAILED;
	}

	/*
	 * BTP Values:
	 * Battery charge type is unknown or not charging.
	 * BTP_BAS_BLS_CHARGE_TYPE_UNKNOWN = 0,
	 *
	 * Battery is charged using constant current.
	 * BTP_BAS_BLS_CHARGE_TYPE_CONSTANT_CURRENT = 1,
	 *
	 * Battery is charged using constant voltage.
	 * BTP_BAS_BLS_CHARGE_TYPE_CONSTANT_VOLTAGE = 2,
	 *
	 * Battery is charged using trickle charge.
	 * BTP_BAS_BLS_CHARGE_TYPE_TRICKLE = 3,
	 *
	 * Battery is charged using float charge.
	 * BTP_BAS_BLS_CHARGE_TYPE_FLOAT = 4
	 */
	enum bt_bas_bls_battery_charge_type type = cp->type;

	bt_bas_bls_set_battery_charge_type(type);

	return BTP_STATUS_SUCCESS;
}

static uint8_t btp_bas_bls_set_charging_fault_reason(const void *cmd, uint16_t cmd_len, void *rsp,
						     uint16_t *rsp_len)
{
	const struct btp_bas_bls_set_charging_fault_reason_cmd *cp =
		(const struct btp_bas_bls_set_charging_fault_reason_cmd *)cmd;

	if (cp->reason != BT_BAS_BLS_FAULT_REASON_NONE &&
	    cp->reason != BT_BAS_BLS_FAULT_REASON_BATTERY &&
	    cp->reason != BT_BAS_BLS_FAULT_REASON_EXTERNAL_POWER &&
	    cp->reason != BT_BAS_BLS_FAULT_REASON_OTHER) {
		return BTP_STATUS_FAILED;
	}

	/*
	 * BTP Values:
	 * No charging fault.
	 * BTP_BAS_BLS_FAULT_REASON_NONE = 0,
	 *
	 * Charging fault due to battery issue.
	 * BTP_BAS_BLS_FAULT_REASON_BATTERY = BIT(0),
	 *
	 * Charging fault due to external power source issue.
	 * BTP_BAS_BLS_FAULT_REASON_EXTERNAL_POWER = BIT(1),
	 *
	 * Charging fault for other reasons.
	 * BTP_BAS_BLS_FAULT_REASON_OTHER = BIT(2)
	 */
	enum bt_bas_bls_charging_fault_reason reason = cp->reason;

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

	if (!IN_RANGE(cp->service_required, BT_BAS_BLS_SERVICE_REQUIRED_FALSE,
		      BT_BAS_BLS_SERVICE_REQUIRED_UNKNOWN)) {
		return BTP_STATUS_FAILED;
	}

	/*
	 * BTP Values:
	 * Service is not required.
	 * BTP_BAS_BLS_SERVICE_REQUIRED_FALSE = 0,
	 *
	 * Service is required.
	 * BTP_BAS_BLS_SERVICE_REQUIRED_TRUE = 1,
	 *
	 * Service requirement is unknown.
	 * BTP_BAS_BLS_SERVICE_REQUIRED_UNKNOWN = 2
	 */
	enum bt_bas_bls_service_required value = cp->service_required;

	bt_bas_bls_set_service_required(value);

	return BTP_STATUS_SUCCESS;
}

static uint8_t btp_bas_bls_set_battery_fault(const void *cmd, uint16_t cmd_len, void *rsp,
					     uint16_t *rsp_len)
{
	const struct btp_bas_bls_set_battery_fault_cmd *cp =
		(const struct btp_bas_bls_set_battery_fault_cmd *)cmd;

	if (cp->battery_fault != BT_BAS_BLS_BATTERY_FAULT_NO &&
	    cp->battery_fault != BT_BAS_BLS_BATTERY_FAULT_YES) {
		return BTP_STATUS_FAILED;
	}

	/*
	 * BTP Values:
	 * No battery fault.
	 * BTP_BAS_BLS_BATTERY_FAULT_NO = 0,
	 *
	 * Battery fault present.
	 * BTP_BAS_BLS_BATTERY_FAULT_YES = 1
	 */
	enum bt_bas_bls_battery_fault value = cp->battery_fault;

	bt_bas_bls_set_battery_fault(value);

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_BAS_BLS_ADDITIONAL_STATUS_PRESENT */

#endif /* CONFIG_BT_BAS_BLS */

static const struct btp_handler handlers[] = {
	{
		.opcode = BTP_BAS_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = bas_supported_commands,
	},
	{
		.opcode = BTP_BAS_SET_BATTERY_LEVEL,
		.index = BTP_INDEX_NONE,
		.expect_len = sizeof(struct btp_bas_set_battery_level_cmd),
		.func = btp_bas_set_battery_level,
	},
#if defined(CONFIG_BT_BAS_BLS)
	{
		.opcode = BTP_BAS_BLS_SET_BATTERY_PRESENT,
		.index = BTP_INDEX_NONE,
		.expect_len = sizeof(struct btp_bas_bls_set_battery_present_cmd),
		.func = btp_bas_bls_set_battery_present,
	},
	{
		.opcode = BTP_BAS_BLS_SET_WIRED_POWER_SOURCE,
		.index = BTP_INDEX_NONE,
		.expect_len = sizeof(struct btp_bas_bls_set_wired_power_source_cmd),
		.func = btp_bas_bls_set_wired_power_source,
	},
	{
		.opcode = BTP_BAS_BLS_SET_WIRELESS_POWER_SOURCE,
		.index = BTP_INDEX_NONE,
		.expect_len = sizeof(struct btp_bas_bls_set_wireless_power_source_cmd),
		.func = btp_bas_bls_set_wireless_power_source,
	},
	{
		.opcode = BTP_BAS_BLS_SET_BATTERY_CHARGE_STATE,
		.index = BTP_INDEX_NONE,
		.expect_len = sizeof(struct btp_bas_bls_set_battery_charge_state_cmd),
		.func = btp_bas_bls_set_battery_charge_state,
	},
	{
		.opcode = BTP_BAS_BLS_SET_BATTERY_CHARGE_LEVEL,
		.index = BTP_INDEX_NONE,
		.expect_len = sizeof(struct btp_bas_bls_set_battery_charge_level_cmd),
		.func = btp_bas_bls_set_battery_charge_level,
	},
	{
		.opcode = BTP_BAS_BLS_SET_BATTERY_CHARGE_TYPE,
		.index = BTP_INDEX_NONE,
		.expect_len = sizeof(struct btp_bas_bls_set_battery_charge_type_cmd),
		.func = btp_bas_bls_set_battery_charge_type,
	},
	{
		.opcode = BTP_BAS_BLS_SET_CHARGING_FAULT_REASON,
		.index = BTP_INDEX_NONE,
		.expect_len = sizeof(struct btp_bas_bls_set_charging_fault_reason_cmd),
		.func = btp_bas_bls_set_charging_fault_reason,
	},
#if defined(CONFIG_BT_BAS_BLS_IDENTIFIER_PRESENT)
	{
		.opcode = BTP_BAS_BLS_SET_IDENTIFIER,
		.index = BTP_INDEX_NONE,
		.expect_len = sizeof(struct btp_bas_bls_set_identifier_cmd),
		.func = btp_bas_bls_set_identifier,
	},
#endif /* CONFIG_BT_BAS_BLS_IDENTIFIER_PRESENT */
#if defined(CONFIG_BT_BAS_BLS_ADDITIONAL_STATUS_PRESENT)
	{
		.opcode = BTP_BAS_BLS_SET_SERVICE_REQUIRED,
		.index = BTP_INDEX_NONE,
		.expect_len = sizeof(struct btp_bas_bls_set_service_required_cmd),
		.func = btp_bas_bls_set_service_required,
	},
	{
		.opcode = BTP_BAS_BLS_SET_BATTERY_FAULT,
		.index = BTP_INDEX_NONE,
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
