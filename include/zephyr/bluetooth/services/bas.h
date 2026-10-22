/*
 * Copyright (c) 2024 Demant A/S
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_BAS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_BAS_H_

/**
 * @brief Battery Service (BAS)
 * @defgroup bt_bas Battery Service (BAS)
 * @ingroup bluetooth
 * @{
 *
 * [Experimental] Users should note that the APIs can change
 * as a part of ongoing development.
 */

#include <stdint.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Battery Critical Status Characteristic flags.
 *
 * Enumeration for the flags indicating the presence
 * of various fields in the Battery Critical Status characteristic.
 */
enum bt_bas_bcs_flags {
	/**  Battery Critical Status Bit 0: Critical Power State */
	BT_BAS_BCS_BATTERY_CRITICAL_STATE = BIT(0),

	/**  Battery Critical Status Bit 1: Immediate Service Required */
	BT_BAS_BCS_IMMEDIATE_SERVICE_REQUIRED = BIT(1),
};

/**
 * @brief Battery Level Status Characteristic flags.
 *
 * Enumeration for the flags indicating the presence
 * of various fields in the Battery Level Status characteristic.
 */
enum bt_bas_bls_flags {
	/** Bit indicating that the Battery Level Status identifier is present. */
	BT_BAS_BLS_FLAG_IDENTIFIER_PRESENT = BIT(0),

	/** Bit indicating that the Battery Level is present. */
	BT_BAS_BLS_FLAG_BATTERY_LEVEL_PRESENT = BIT(1),

	/** Bit indicating that additional status information is present. */
	BT_BAS_BLS_FLAG_ADDITIONAL_STATUS_PRESENT = BIT(2),
};

/** @brief Battery Present Status
 *
 * Enumeration for the presence of the battery.
 */
enum bt_bas_bls_battery_present {
	/** Battery is not present. */
	BT_BAS_BLS_BATTERY_NOT_PRESENT = 0,

	/** Battery is present. */
	BT_BAS_BLS_BATTERY_PRESENT = 1
};

/** @brief Wired External Power Source Status
 *
 * Enumeration for the status of the wired external power source.
 */
enum bt_bas_bls_wired_power_source {
	/** Wired external power source is not connected. */
	BT_BAS_BLS_WIRED_POWER_NOT_CONNECTED = 0,

	/** Wired external power source is connected. */
	BT_BAS_BLS_WIRED_POWER_CONNECTED = 1,

	/** Wired external power source status is unknown. */
	BT_BAS_BLS_WIRED_POWER_UNKNOWN = 2
};

/** @brief Wireless External Power Source Status
 *
 * Enumeration for the status of the wireless external power source.
 */
enum bt_bas_bls_wireless_power_source {
	/** Wireless external power source is not connected. */
	BT_BAS_BLS_WIRELESS_POWER_NOT_CONNECTED = 0,

	/** Wireless external power source is connected. */
	BT_BAS_BLS_WIRELESS_POWER_CONNECTED = 1,

	/** Wireless external power source status is unknown. */
	BT_BAS_BLS_WIRELESS_POWER_UNKNOWN = 2
};

/** @brief Battery Charge State
 *
 * Enumeration for the charge state of the battery.
 */
enum bt_bas_bls_battery_charge_state {
	/** Battery charge state is unknown. */
	BT_BAS_BLS_CHARGE_STATE_UNKNOWN = 0,

	/** Battery is currently charging. */
	BT_BAS_BLS_CHARGE_STATE_CHARGING = 1,

	/** Battery is discharging actively. */
	BT_BAS_BLS_CHARGE_STATE_DISCHARGING_ACTIVE = 2,

	/** Battery is discharging but inactive. */
	BT_BAS_BLS_CHARGE_STATE_DISCHARGING_INACTIVE = 3
};

/** @brief Battery Charge Level
 *
 * Enumeration for the level of charge in the battery.
 */
enum bt_bas_bls_battery_charge_level {
	/** Battery charge level is unknown. */
	BT_BAS_BLS_CHARGE_LEVEL_UNKNOWN = 0,

	/** Battery charge level is good. */
	BT_BAS_BLS_CHARGE_LEVEL_GOOD = 1,

	/** Battery charge level is low. */
	BT_BAS_BLS_CHARGE_LEVEL_LOW = 2,

	/** Battery charge level is critical. */
	BT_BAS_BLS_CHARGE_LEVEL_CRITICAL = 3
};

/** @brief Battery Charge Type
 *
 * Enumeration for the type of charging applied to the battery.
 */
enum bt_bas_bls_battery_charge_type {
	/** Battery charge type is unknown or not charging. */
	BT_BAS_BLS_CHARGE_TYPE_UNKNOWN = 0,

	/** Battery is charged using constant current. */
	BT_BAS_BLS_CHARGE_TYPE_CONSTANT_CURRENT = 1,

	/** Battery is charged using constant voltage. */
	BT_BAS_BLS_CHARGE_TYPE_CONSTANT_VOLTAGE = 2,

	/** Battery is charged using trickle charge. */
	BT_BAS_BLS_CHARGE_TYPE_TRICKLE = 3,

	/** Battery is charged using float charge. */
	BT_BAS_BLS_CHARGE_TYPE_FLOAT = 4
};

/** @brief Charging Fault Reason
 *
 * Enumeration for the reasons of charging faults.
 */
enum bt_bas_bls_charging_fault_reason {
	/** No charging fault. */
	BT_BAS_BLS_FAULT_REASON_NONE = 0,

	/** Charging fault due to battery issue. */
	BT_BAS_BLS_FAULT_REASON_BATTERY = BIT(0),

	/** Charging fault due to external power source issue. */
	BT_BAS_BLS_FAULT_REASON_EXTERNAL_POWER = BIT(1),

	/** Charging fault for other reasons. */
	BT_BAS_BLS_FAULT_REASON_OTHER = BIT(2)
};

/** @brief Service Required Status
 *
 * Enumeration for whether the service is required.
 */
enum bt_bas_bls_service_required {
	/** Service is not required. */
	BT_BAS_BLS_SERVICE_REQUIRED_FALSE = 0,

	/** Service is required. */
	BT_BAS_BLS_SERVICE_REQUIRED_TRUE = 1,

	/** Service requirement is unknown. */
	BT_BAS_BLS_SERVICE_REQUIRED_UNKNOWN = 2
};

/** @brief Battery Fault Status
 *
 * Enumeration for the fault status of the battery.
 */
enum bt_bas_bls_battery_fault {
	/** No battery fault. */
	BT_BAS_BLS_BATTERY_FAULT_NO = 0,

	/** Battery fault present. */
	BT_BAS_BLS_BATTERY_FAULT_YES = 1
};

/** @brief Read battery level value.
 *
 * Read the characteristic value of the battery level
 *
 *  @return The battery level in percent.
 */
uint8_t bt_bas_get_battery_level(void);

/** @brief Update battery level value.
 *
 * Update the characteristic value of the battery level
 * This will send a GATT notification to all current subscribers.
 *
 *  @param level The battery level in percent.
 *
 *  @return Zero in case of success and error code in case of error.
 */
int bt_bas_set_battery_level(uint8_t level);

/**
 * @brief Set the battery present status.
 *
 * @param present The battery present status to set.
 */
void bt_bas_bls_set_battery_present(enum bt_bas_bls_battery_present present);

/**
 * @brief Set the wired external power source status.
 *
 * @param source The wired external power source status to set.
 */
void bt_bas_bls_set_wired_external_power_source(enum bt_bas_bls_wired_power_source source);

/**
 * @brief Set the wireless external power source status.
 *
 * @param source The wireless external power source status to set.
 */
void bt_bas_bls_set_wireless_external_power_source(enum bt_bas_bls_wireless_power_source source);

/**
 * @brief Set the battery charge state.
 *
 * @param state The battery charge state to set.
 */
void bt_bas_bls_set_battery_charge_state(enum bt_bas_bls_battery_charge_state state);

/**
 * @brief Set the battery charge level.
 *
 * @param level The battery charge level to set.
 */
void bt_bas_bls_set_battery_charge_level(enum bt_bas_bls_battery_charge_level level);

/**
 * @brief Set the battery charge type.
 *
 * @param type The battery charge type to set.
 */
void bt_bas_bls_set_battery_charge_type(enum bt_bas_bls_battery_charge_type type);

/**
 * @brief Set the charging fault reason.
 *
 * @param reason The charging fault reason to set.
 */
void bt_bas_bls_set_charging_fault_reason(enum bt_bas_bls_charging_fault_reason reason);

/**
 * @brief Set the identifier of the battery.
 *
 * @kconfig_dep{CONFIG_BT_BAS_BLS_IDENTIFIER_PRESENT}
 *
 * @param identifier Identifier to set.
 */
void bt_bas_bls_set_identifier(uint16_t identifier);

/**
 * @brief Set the service required status.
 *
 * @kconfig_dep{CONFIG_BT_BAS_BLS_ADDITIONAL_STATUS_PRESENT}
 *
 * @param value Service required status to set.
 */
void bt_bas_bls_set_service_required(enum bt_bas_bls_service_required value);

/**
 * @brief Set the battery fault status.
 *
 * @kconfig_dep{CONFIG_BT_BAS_BLS_ADDITIONAL_STATUS_PRESENT}
 *
 * @param value Battery fault status to set.
 */
void bt_bas_bls_set_battery_fault(enum bt_bas_bls_battery_fault value);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_SERVICES_BAS_H_ */
