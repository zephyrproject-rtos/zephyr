/*
 * Copyright (c) 2024 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BT_BAS_INTERNAL_H_
#define BT_BAS_INTERNAL_H_

#include <sys/types.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/types.h>

/**
 * @brief  Battery level status structure definition.
 */
struct bt_bas_bls {

	/** @brief Flags Field
	 *
	 * The values of this field are defined below.
	 *
	 * - bit 0: Identifier Present
	 *   - Indicates whether the identifier field is present.
	 * - bit 1: Battery Level Present
	 *   - Indicates whether the battery level field is present.
	 * - bit 2: Additional Status Present
	 *   - Indicates whether the additional status field is present.
	 * - bit 3–7: RFU (Reserved for Future Use)
	 *   - Reserved bits for future use; should be set to zero.
	 */
	uint8_t flags;

	/** @brief Power State
	 *
	 * The values of this field are defined below.
	 *
	 * - bit 0: Battery Present
	 *   - 0 = No
	 *   - 1 = Yes
	 * - bit 1–2: Wired External Power Source Connected
	 *   - 0 = No
	 *   - 1 = Yes
	 *   - 2 = Unknown
	 *   - 3 = RFU
	 * - bit 3–4: Wireless External Power Source Connected
	 *   - 0 = No
	 *   - 1 = Yes
	 *   - 2 = Unknown
	 *   - 3 = RFU
	 * - bit 5–6: Battery Charge State
	 *   - 0 = Unknown
	 *   - 1 = Charging
	 *   - 2 = Discharging: Active
	 *   - 3 = Discharging: Inactive
	 * - bit 7–8: Battery Charge Level
	 *   - 0 = Unknown
	 *   - 1 = Good
	 *   - 2 = Low
	 *   - 3 = Critical
	 * - bit 9–11: Charging Type
	 *   - 0 = Unknown or Not Charging
	 *   - 1 = Constant Current
	 *   - 2 = Constant Voltage
	 *   - 3 = Trickle
	 *   - 4 = Float
	 *   - 5–7 = RFU
	 * - bit 12–14: Charging Fault Reason
	 *   - Bit 12: Battery
	 *   - Bit 13: External Power source
	 *   - Bit 14: Other
	 * - bit 15: RFU
	 */
	uint16_t power_state;

#if defined(CONFIG_BT_BAS_BLS_IDENTIFIER_PRESENT)
	/** Identifier for the battery, range 0x0000 to 0xFFFF.*/
	uint16_t identifier;
#endif /* CONFIG_BT_BAS_BLS_IDENTIFIER_PRESENT */

#if defined(CONFIG_BT_BAS_BLS_BATTERY_LEVEL_PRESENT)
	/** Current battery level */
	uint8_t battery_level;
#endif /* CONFIG_BT_BAS_BLS_BATTERY_LEVEL_PRESENT */

#if defined(CONFIG_BT_BAS_BLS_ADDITIONAL_STATUS_PRESENT)

	/** @brief Additional Status
	 *
	 * The values of this field are defined below.
	 *
	 * - bit 0–1: Service Required
	 *   - 0 = False
	 *   - 1 = True
	 *   - 2 = Unknown
	 *   - 3 = RFU
	 * - bit 2: Battery Fault
	 *   - 0 = False or Unknown
	 *   - 1 = Yes
	 * - bit 3–7: RFU
	 */
	uint8_t additional_status;

#endif /* CONFIG_BT_BAS_BLS_ADDITIONAL_STATUS_PRESENT */

} __packed;

/**
 * @brief Initialize the Battery Level Status Module.
 *
 */
void bt_bas_bls_init(void);

/**
 * @brief Read the Battery Critical Status characteristic.
 *
 * @param conn Pointer to the Bluetooth connection object representing the client requesting
 *             the characteristic.
 * @param attr Pointer to the GATT attribute of Battery Critical Status characteristic.
 * @param buf Buffer to store the read value.
 * @param len Length of the buffer.
 * @param offset Offset within the characteristic value to start reading.
 *
 * @return The number of bytes read and sent to the client, or a negative error code on failure.
 */
ssize_t bt_bas_bcs_read_critical_status(struct bt_conn *conn, const struct bt_gatt_attr *attr,
					void *buf, uint16_t len, uint16_t offset);

/**
 * @brief Callback function for BCS Client Characteristic Configuration changes.
 *
 *
 * @param attr  Pointer to the GATT attribute of battery critical status char.
 * @param value The new value of the CCC. This value indicates whether
 *              notifications or indications are enabled or disabled.
 */
void bt_bas_bcs_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);

/**
 * @brief Set the battery level characteristic value.
 *
 * @param battery_level The new battery level value in percent (0-100).
 */
void bt_bas_bls_set_battery_level(uint8_t battery_level);

/**
 * @brief Set the battery critical state flag.
 *
 * @param critical_state The battery critical state to set (true for critical, false otherwise).
 */
void bt_bas_bcs_set_battery_critical_state(bool critical_state);

/**
 * @brief Set the immediate service required flag.
 *
 * @param service_required The immediate service required status to set.
 */
void bt_bas_bcs_set_immediate_service_required(bool service_required);

/**
 * @brief Read the Battery Level Status characteristic.
 *
 * @param conn Pointer to the Bluetooth connection object representing the client requesting
 *             the characteristic.
 * @param attr Pointer to the GATT attribute representing the Battery Level Status characteristic.
 * @param buf Buffer to store the read value.
 * @param len Length of the buffer.
 * @param offset Offset within the characteristic value to start reading.
 *
 * @return The number of bytes read and sent to the client, or a negative error code on failure.
 */
ssize_t bt_bas_bls_read_blvl_status(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				    void *buf, uint16_t len, uint16_t offset);

/**
 * @brief Retrieve the Bluetooth GATT attribute for the BAS service by index.
 *
 * @param index The index of the attribute within the BAS service.
 *
 * @return Pointer to the Bluetooth GATT attribute if the index is valid,
 *         otherwise NULL if the index is out of bounds.
 */
const struct bt_gatt_attr *bt_bas_get_bas_attr(uint16_t index);

#endif /* BT_BAS_INTERNAL_H_ */
