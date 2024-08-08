/*
 * Copyright (c) 2024 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bas_internal.h"
#include <zephyr/bluetooth/services/bas.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bas, CONFIG_BT_BAS_LOG_LEVEL);

static bt_bas_bls_t *battery_level_status;

/* Power state field
 * Define macros for bit positions and masks
 */
#define BATTERY_SHIFT              0
#define WIRED_POWER_SHIFT          1
#define WIRELESS_POWER_SHIFT       3
#define BATTERY_CHARGE_STATE_SHIFT 5
#define BATTERY_CHARGE_LEVEL_SHIFT 7
#define BATTERY_CHARGE_TYPE_SHIFT  9
#define CHARGING_FAULT_SHIFT       12

#define BATTERY_MASK              0x01
#define WIRED_POWER_MASK          0x03
#define WIRELESS_POWER_MASK       0x03
#define BATTERY_CHARGE_STATE_MASK 0x03
#define BATTERY_CHARGE_LEVEL_MASK 0x03
#define BATTERY_CHARGE_TYPE_MASK  0x07
#define CHARGING_FAULT_MASK       0x07

/* Additional Status field
 * Define macros for bit positions and masks
 */
#define SERVICE_REQUIRED_SHIFT 0
#define BATTERY_FAULT_SHIFT    2

#define SERVICE_REQUIRED_MASK 0x03
#define BATTERY_FAULT_MASK    0x01

static void bas_bls_pack_power_state(void)
{
	uint16_t packed_value = 0;

	packed_value |= (battery_level_status->power_state.battery & BATTERY_MASK) << BATTERY_SHIFT;
	packed_value |=
		(battery_level_status->power_state.wired_external_power_source & WIRED_POWER_MASK)
		<< WIRED_POWER_SHIFT;
	packed_value |= (battery_level_status->power_state.wireless_external_power_source &
			 WIRELESS_POWER_MASK)
			<< WIRELESS_POWER_SHIFT;
	packed_value |=
		(battery_level_status->power_state.battery_charge_state & BATTERY_CHARGE_STATE_MASK)
		<< BATTERY_CHARGE_STATE_SHIFT;
	packed_value |=
		(battery_level_status->power_state.battery_charge_level & BATTERY_CHARGE_LEVEL_MASK)
		<< BATTERY_CHARGE_LEVEL_SHIFT;
	packed_value |=
		(battery_level_status->power_state.battery_charge_type & BATTERY_CHARGE_TYPE_MASK)
		<< BATTERY_CHARGE_TYPE_SHIFT;
	packed_value |=
		(battery_level_status->power_state.charging_fault_reason & CHARGING_FAULT_MASK)
		<< CHARGING_FAULT_SHIFT;

	battery_level_status->power_state.packed_value = packed_value;

	update_battery_level_status();
}

void bt_bas_bls_init(bt_bas_bls_t *bls)
{
	LOG_INF("Initialise BAS Battery Level Status Module");

	battery_level_status = bls;
	if (battery_level_status == NULL) {
		LOG_INF("BAS service not initialised");
		return;
	}

	battery_level_status->flags.identifier = 0;
	battery_level_status->flags.battery_level = 0;
	battery_level_status->flags.additional_status = 0;
	battery_level_status->flags.flags_reserved = 0;

	battery_level_status->power_state.packed_value = 0;

#if defined(CONFIG_BT_BAS_BLS_IDENTIFIER_PRESENT)
	battery_level_status->flags.identifier = 1;
	battery_level_status->identifier = 0;
#endif

#if defined(CONFIG_BT_BAS_BLS_BATTERY_LEVEL_PRESENT)
	battery_level_status->flags.battery_level = 1;
	battery_level_status->battery_level = bt_bas_get_battery_level();
#endif

#if defined(CONFIG_BT_BAS_BLS_ADDITIONAL_STATUS_PRESENT)
	battery_level_status->flags.additional_status = 1;
	battery_level_status->additional_status.packed_value = 0;
#endif
}

void bt_bas_bls_set_battery_present(bt_bas_bls_battery_present_t present)
{
	if (battery_level_status == NULL) {
		return;
	}
	battery_level_status->power_state.battery = present;
	bas_bls_pack_power_state();
}

void bt_bas_bls_set_wired_external_power_source(bt_bas_bls_wired_power_source_t source)
{
	if (battery_level_status == NULL) {
		return;
	}
	battery_level_status->power_state.wired_external_power_source = source;
	bas_bls_pack_power_state();
}

void bt_bas_bls_set_wireless_external_power_source(bt_bas_bls_wireless_power_source_t source)
{
	if (battery_level_status == NULL) {
		return;
	}
	battery_level_status->power_state.wireless_external_power_source = source;
	bas_bls_pack_power_state();
}

void bt_bas_bls_set_battery_charge_state(bt_bas_bls_battery_charge_state_t state)
{
	if (battery_level_status == NULL) {
		return;
	}
	battery_level_status->power_state.battery_charge_state = state;
	bas_bls_pack_power_state();
}

void bt_bas_bls_set_battery_charge_level(bt_bas_bls_battery_charge_level_t level)
{
	if (battery_level_status == NULL) {
		return;
	}
	battery_level_status->power_state.battery_charge_level = level;
	bas_bls_pack_power_state();
}

void bt_bas_bls_set_battery_charge_type(bt_bas_bls_battery_charge_type_t type)
{
	if (battery_level_status == NULL) {
		return;
	}
	battery_level_status->power_state.battery_charge_type = type;
	bas_bls_pack_power_state();
}

void bt_bas_bls_set_charging_fault_reason(bt_bas_bls_charging_fault_reason_t reason)
{
	if (battery_level_status == NULL) {
		return;
	}
	battery_level_status->power_state.charging_fault_reason = reason;
	bas_bls_pack_power_state();
}

#if defined(CONFIG_BT_BAS_BLS_IDENTIFIER_PRESENT)
void bt_bas_bls_set_identifier(uint16_t identifier)
{
	if (battery_level_status == NULL) {
		return;
	}
	battery_level_status->identifier = identifier;
	update_battery_level_status();
}
#endif

#if defined(CONFIG_BT_BAS_BLS_ADDITIONAL_STATUS_PRESENT)

static void bas_bls_pack_additional_status(void)
{
	if (battery_level_status == NULL) {
		return;
	}

	uint8_t packed_value = 0;

	packed_value |=
		(battery_level_status->additional_status.service_required & SERVICE_REQUIRED_MASK)
		<< SERVICE_REQUIRED_SHIFT;
	packed_value |= (battery_level_status->additional_status.battery_fault & BATTERY_FAULT_MASK)
			<< BATTERY_FAULT_SHIFT;

	battery_level_status->additional_status.packed_value = packed_value;
	update_battery_level_status();
}

void bt_bas_bls_set_service_required(bt_bas_bls_service_required_t value)
{
	if (battery_level_status == NULL) {
		return;
	}
	battery_level_status->additional_status.service_required = value;
	bas_bls_pack_additional_status();
}

void bt_bas_bls_set_battery_fault(bt_bas_bls_battery_fault_t value)
{
	if (battery_level_status == NULL) {
		return;
	}
	battery_level_status->additional_status.battery_fault = value;
	bas_bls_pack_additional_status();
}
#endif
