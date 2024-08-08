/*
 * Copyright (c) 2024 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_BAS_BLS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_BAS_BLS_H_

#include <sys/types.h>
#include <zephyr/sys/atomic.h>
#include <stdint.h>
#include <zephyr/types.h>

/* Enums for battery level status fields */
typedef enum {
	BT_BAS_BLS_BATTERY_NOT_PRESENT = 0,
	BT_BAS_BLS_BATTERY_PRESENT = 1
} bt_bas_bls_battery_present_t;

typedef enum {
	BT_BAS_BLS_WIRED_POWER_NOT_CONNECTED = 0,
	BT_BAS_BLS_WIRED_POWER_CONNECTED = 1,
	BT_BAS_BLS_WIRED_POWER_UNKNOWN = 2
} bt_bas_bls_wired_power_source_t;

typedef enum {
	BT_BAS_BLS_WIRELESS_POWER_NOT_CONNECTED = 0,
	BT_BAS_BLS_WIRELESS_POWER_CONNECTED = 1,
	BT_BAS_BLS_WIRELESS_POWER_UNKNOWN = 2
} bt_bas_bls_wireless_power_source_t;

typedef enum {
	BT_BAS_BLS_CHARGE_STATE_UNKNOWN = 0,
	BT_BAS_BLS_CHARGE_STATE_CHARGING = 1,
	BT_BAS_BLS_CHARGE_STATE_DISCHARGING_ACTIVE = 2,
	BT_BAS_BLS_CHARGE_STATE_DISCHARGING_INACTIVE = 3
} bt_bas_bls_battery_charge_state_t;

typedef enum {
	BT_BAS_BLS_CHARGE_LEVEL_UNKNOWN = 0,
	BT_BAS_BLS_CHARGE_LEVEL_GOOD = 1,
	BT_BAS_BLS_CHARGE_LEVEL_LOW = 2,
	BT_BAS_BLS_CHARGE_LEVEL_CRITICAL = 3
} bt_bas_bls_battery_charge_level_t;

typedef enum {
	BT_BAS_BLS_CHARGE_TYPE_UNKNOWN = 0,
	BT_BAS_BLS_CHARGE_TYPE_CONSTANT_CURRENT = 1,
	BT_BAS_BLS_CHARGE_TYPE_CONSTANT_VOLTAGE = 2,
	BT_BAS_BLS_CHARGE_TYPE_TRICKLE = 3,
	BT_BAS_BLS_CHARGE_TYPE_FLOAT = 4
} bt_bas_bls_battery_charge_type_t;

typedef enum {
	BT_BAS_BLS_FAULT_REASON_NONE = 0,
	BT_BAS_BLS_FAULT_REASON_BATTERY = 1,
	BT_BAS_BLS_FAULT_REASON_EXTERNAL_POWER = 2,
	BT_BAS_BLS_FAULT_REASON_OTHER = 4
} bt_bas_bls_charging_fault_reason_t;

typedef enum {
	BT_BAS_BLS_SERVICE_REQUIRED_FALSE = 0,
	BT_BAS_BLS_SERVICE_REQUIRED_TRUE = 1,
	BT_BAS_BLS_SERVICE_REQUIRED_UNKNOWN = 2
} bt_bas_bls_service_required_t;

typedef enum {
	BT_BAS_BLS_BATTERY_FAULT_NO = 0,
	BT_BAS_BLS_BATTERY_FAULT_YES = 1
} bt_bas_bls_battery_fault_t;

/* Battery level status structure definition */
typedef struct {
	struct {
		uint8_t identifier: 1;
		uint8_t battery_level: 1;
		uint8_t additional_status: 1;
		uint8_t flags_reserved: 5;
	} flags;

	union {
		struct {
			uint16_t battery: 1;
			uint16_t wired_external_power_source: 2;
			uint16_t wireless_external_power_source: 2;
			uint16_t battery_charge_state: 2;
			uint16_t battery_charge_level: 2;
			uint16_t battery_charge_type: 3;
			uint16_t charging_fault_reason: 3;
			uint16_t power_state_reserved: 1;
		};
		uint16_t packed_value;
	} power_state;

#if defined(CONFIG_BT_BAS_BLS_IDENTIFIER_PRESENT)
	uint16_t identifier;
#endif /* CONFIG_BT_BAS_BLS_IDENTIFIER_PRESENT */

#if defined(CONFIG_BT_BAS_BLS_BATTERY_LEVEL_PRESENT)
	uint8_t battery_level;
#endif /* CONFIG_BT_BAS_BLS_BATTERY_LEVEL_PRESENT */

#if defined(CONFIG_BT_BAS_BLS_ADDITIONAL_STATUS_PRESENT)
	union {
		struct {
			uint8_t service_required: 2;
			uint8_t battery_fault: 1;
			uint8_t reserved: 5;
		};
		uint8_t packed_value;
	} additional_status;
#endif /* CONFIG_BT_BAS_BLS_ADDITIONAL_STATUS_PRESENT */

} __packed bt_bas_bls_t;

/**
 * @brief Set the battery present status.
 *
 * @param present The battery present status to set.
 */
void bt_bas_bls_set_battery_present(bt_bas_bls_battery_present_t present);

/**
 * @brief Set the wired external power source status.
 *
 * @param source The wired external power source status to set.
 */
void bt_bas_bls_set_wired_external_power_source(bt_bas_bls_wired_power_source_t source);

/**
 * @brief Set the wireless external power source status.
 *
 * @param source The wireless external power source status to set.
 */
void bt_bas_bls_set_wireless_external_power_source(bt_bas_bls_wireless_power_source_t source);

/**
 * @brief Set the battery charge state.
 *
 * @param state The battery charge state to set.
 */
void bt_bas_bls_set_battery_charge_state(bt_bas_bls_battery_charge_state_t state);

/**
 * @brief Set the battery charge level.
 *
 * @param level The battery charge level to set.
 */
void bt_bas_bls_set_battery_charge_level(bt_bas_bls_battery_charge_level_t level);

/**
 * @brief Set the battery charge type.
 *
 * @param type The battery charge type to set.
 */
void bt_bas_bls_set_battery_charge_type(bt_bas_bls_battery_charge_type_t type);

/**
 * @brief Set the charging fault reason.
 *
 * @param reason The charging fault reason to set.
 */
void bt_bas_bls_set_charging_fault_reason(bt_bas_bls_charging_fault_reason_t reason);

#if defined(CONFIG_BT_BAS_BLS_IDENTIFIER_PRESENT)

/**
 * @brief Set the identifier of the battery.
 *
 * @param identifier Identifier to set.
 */
void bt_bas_bls_set_identifier(uint16_t identifier);

#endif /* CONFIG_BT_BAS_BLS_IDENTIFIER_PRESENT */

#if defined(CONFIG_BT_BAS_BLS_ADDITIONAL_STATUS_PRESENT)

/**
 * @brief Set the service required status.
 *
 * @param value Service required status to set.
 */
void bt_bas_bls_set_service_required(bt_bas_bls_service_required_t value);

/**
 * @brief Set the battery fault status.
 *
 * @param value Battery fault status to set.
 */
void bt_bas_bls_set_battery_fault(bt_bas_bls_battery_fault_t value);

#endif /* CONFIG_BT_BAS_BLS_ADDITIONAL_STATUS_PRESENT */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_BAS_BLS_H_ */
