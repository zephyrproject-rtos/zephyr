/*
 * Copyright (c) 2024 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/bluetooth/gatt.h>
#include "bas_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(bas, CONFIG_BT_BAS_LOG_LEVEL);

/* The battery level status of a battery. */
static struct bt_bas_bls bls;

#define BT_BAS_IDX_BATT_LVL_STATUS_CHAR_VAL 6

/* Notify/Indicate all connections */
static struct bt_gatt_indicate_params ind_params;

/*
 * Bitfield structure: Power State
 *
 * - Bits 0: Battery Present
 * - Bits 1–2: Wired External Power Source Connected
 * - Bits 3–4: Wireless External Power Source Connected
 * - Bits 5–6: Battery Charge State
 * - Bits 7–8: Battery Charge Level
 * - Bits 9–11: Charging Type
 * - Bits 12–14: Charging Fault Reason
 * - Bit 15: RFU
 *
 * For detailed specification, refer to:
 * https://bitbucket.org/bluetooth-SIG/public/src/main/gss/
 * org.bluetooth.characteristic.battery_level_status.yaml
 */

#define BATTERY_SHIFT              0
#define WIRED_POWER_SHIFT          1
#define WIRELESS_POWER_SHIFT       3
#define BATTERY_CHARGE_STATE_SHIFT 5
#define BATTERY_CHARGE_LEVEL_SHIFT 7
#define BATTERY_CHARGE_TYPE_SHIFT  9
#define CHARGING_FAULT_SHIFT       12

#define BATTERY_MASK              (BIT_MASK(1) << BATTERY_SHIFT)
#define WIRED_POWER_MASK          (BIT_MASK(2) << WIRED_POWER_SHIFT)
#define WIRELESS_POWER_MASK       (BIT_MASK(2) << WIRELESS_POWER_SHIFT)
#define BATTERY_CHARGE_STATE_MASK (BIT_MASK(2) << BATTERY_CHARGE_STATE_SHIFT)
#define BATTERY_CHARGE_LEVEL_MASK (BIT_MASK(2) << BATTERY_CHARGE_LEVEL_SHIFT)
#define BATTERY_CHARGE_TYPE_MASK  (BIT_MASK(3) << BATTERY_CHARGE_TYPE_SHIFT)
#define CHARGING_FAULT_MASK       (BIT_MASK(3) << CHARGING_FAULT_SHIFT)

/*
 * Bitfield structure: Additional Status
 *
 * - Bits 0–1: Service Required
 * - Bit 2: Battery Fault
 * - Bits 3–7: Reserved
 */
#define SERVICE_REQUIRED_SHIFT 0
#define BATTERY_FAULT_SHIFT    2

#define SERVICE_REQUIRED_MASK (BIT_MASK(2) << SERVICE_REQUIRED_SHIFT)
#define BATTERY_FAULT_MASK    (BIT_MASK(1) << BATTERY_FAULT_SHIFT)

void bt_bas_bls_init(void)
{
	LOG_DBG("Initialise BAS Battery Level Status Module");

	bls.flags = 0;
	bls.power_state = 0;

#if defined(CONFIG_BT_BAS_BLS_IDENTIFIER_PRESENT)
	/* Set identifier flag */
	bls.flags |= BT_BAS_BLS_FLAG_IDENTIFIER_PRESENT;
	bls.identifier = 0;
#endif /* CONFIG_BT_BAS_BLS_IDENTIFIER_PRESENT */

#if defined(CONFIG_BT_BAS_BLS_BATTERY_LEVEL_PRESENT)
	/* Set battery level flag */
	bls.flags |= BT_BAS_BLS_FLAG_BATTERY_LEVEL_PRESENT;
	bls.battery_level = bt_bas_get_battery_level();
#endif /* CONFIG_BT_BAS_BLS_BATTERY_LEVEL_PRESENT */

#if defined(CONFIG_BT_BAS_BLS_ADDITIONAL_STATUS_PRESENT)
	/* Set additional status flag */
	bls.flags |= BT_BAS_BLS_FLAG_ADDITIONAL_STATUS_PRESENT;
	bls.additional_status = 0;
#endif /* CONFIG_BT_BAS_BLS_ADDITIONAL_STATUS_PRESENT */
}

static void indicate_cb(struct bt_conn *conn, struct bt_gatt_indicate_params *params, uint8_t err)
{
	if (err != 0) {
		LOG_DBG("Indication failed with error %d\n", err);
	} else {
		LOG_DBG("Indication sent successfully\n");
	}
}

static void bt_bas_bls_update_battery_level_status(void)
{
	int err;
	const struct bt_gatt_attr *attr = bt_bas_get_bas_attr(BT_BAS_IDX_BATT_LVL_STATUS_CHAR_VAL);

	if (attr) {
		const struct bt_bas_bls le_battery_level_status = {
			.flags = bls.flags,
			.power_state = sys_cpu_to_le16(bls.power_state),
#if defined(CONFIG_BT_BAS_BLS_IDENTIFIER_PRESENT)
			.identifier = sys_cpu_to_le16(bls.identifier),
#endif
#if defined(CONFIG_BT_BAS_BLS_BATTERY_LEVEL_PRESENT)
			.battery_level = bls.battery_level,
#endif
#if defined(CONFIG_BT_BAS_BLS_ADDITIONAL_STATUS_PRESENT)
			.additional_status = bls.additional_status,
#endif
		};

		/* Indicate all connections */
		ind_params.attr = attr;
		ind_params.data = &le_battery_level_status;
		ind_params.len = sizeof(le_battery_level_status);
		ind_params.func = indicate_cb;
		err = bt_gatt_indicate(NULL, &ind_params);
		if (err) {
			LOG_DBG("Failed to send ind to all connections (err %d)\n", err);
		}

		/* Notify all connections */
		err = bt_gatt_notify(NULL, attr, &le_battery_level_status,
				     sizeof(le_battery_level_status));
		if (err) {
			LOG_DBG("Failed to send ntf to all connections (err %d)\n", err);
		}
	}
}

ssize_t bt_bas_bls_read_blvl_status(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				    void *buf, uint16_t len, uint16_t offset)
{
	const struct bt_bas_bls le_battery_level_status = {
		.flags = bls.flags,
		.power_state = sys_cpu_to_le16(bls.power_state),
#if defined(CONFIG_BT_BAS_BLS_IDENTIFIER_PRESENT)
		.identifier = sys_cpu_to_le16(bls.identifier),
#endif
#if defined(CONFIG_BT_BAS_BLS_BATTERY_LEVEL_PRESENT)
		.battery_level = bls.battery_level,
#endif
#if defined(CONFIG_BT_BAS_BLS_ADDITIONAL_STATUS_PRESENT)
		.additional_status = bls.additional_status,
#endif
	};

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &le_battery_level_status,
				 sizeof(le_battery_level_status));
}

void bt_bas_bls_set_battery_present(enum bt_bas_bls_battery_present present)
{
	bls.power_state &= ~BATTERY_MASK;
	bls.power_state |= (present << BATTERY_SHIFT) & BATTERY_MASK;
	bt_bas_bls_update_battery_level_status();
}

void bt_bas_bls_set_wired_external_power_source(enum bt_bas_bls_wired_power_source source)
{
	bls.power_state &= ~WIRED_POWER_MASK;
	bls.power_state |= (source << WIRED_POWER_SHIFT) & WIRED_POWER_MASK;
	bt_bas_bls_update_battery_level_status();
}

void bt_bas_bls_set_wireless_external_power_source(enum bt_bas_bls_wireless_power_source source)
{
	bls.power_state &= ~WIRELESS_POWER_MASK;
	bls.power_state |= (source << WIRELESS_POWER_SHIFT) & WIRELESS_POWER_MASK;
	bt_bas_bls_update_battery_level_status();
}

void bt_bas_bls_set_battery_charge_state(enum bt_bas_bls_battery_charge_state state)
{
	bls.power_state &= ~BATTERY_CHARGE_STATE_MASK;
	bls.power_state |= (state << BATTERY_CHARGE_STATE_SHIFT) & BATTERY_CHARGE_STATE_MASK;
	bt_bas_bls_update_battery_level_status();
}

void bt_bas_bls_set_battery_charge_level(enum bt_bas_bls_battery_charge_level level)
{
	bls.power_state &= ~BATTERY_CHARGE_LEVEL_MASK;
	bls.power_state |= (level << BATTERY_CHARGE_LEVEL_SHIFT) & BATTERY_CHARGE_LEVEL_MASK;
	bt_bas_bls_update_battery_level_status();
}

void bt_bas_bls_set_battery_charge_type(enum bt_bas_bls_battery_charge_type type)
{
	bls.power_state &= ~BATTERY_CHARGE_TYPE_MASK;
	bls.power_state |= (type << BATTERY_CHARGE_TYPE_SHIFT) & BATTERY_CHARGE_TYPE_MASK;
	bt_bas_bls_update_battery_level_status();
}

void bt_bas_bls_set_charging_fault_reason(enum bt_bas_bls_charging_fault_reason reason)
{
	bls.power_state &= ~CHARGING_FAULT_MASK;
	bls.power_state |= (reason << CHARGING_FAULT_SHIFT) & CHARGING_FAULT_MASK;
	bt_bas_bls_update_battery_level_status();
}

#if defined(CONFIG_BT_BAS_BLS_BATTERY_LEVEL_PRESENT)
void bt_bas_bls_set_battery_level(uint8_t level)
{
	bls.battery_level = level;
	bt_bas_bls_update_battery_level_status();
}
#endif /* CONFIG_BT_BAS_BLS_BATTERY_LEVEL_PRESENT */

#if defined(CONFIG_BT_BAS_BLS_IDENTIFIER_PRESENT)
void bt_bas_bls_set_identifier(uint16_t identifier)
{
	bls.identifier = sys_cpu_to_le16(identifier);
	bt_bas_bls_update_battery_level_status();
}
#endif /* CONFIG_BT_BAS_BLS_IDENTIFIER_PRESENT */

#if defined(CONFIG_BT_BAS_BLS_ADDITIONAL_STATUS_PRESENT)
void bt_bas_bls_set_service_required(enum bt_bas_bls_service_required value)
{
	bls.additional_status &= ~SERVICE_REQUIRED_MASK;
	bls.additional_status |= (value << SERVICE_REQUIRED_SHIFT) & SERVICE_REQUIRED_MASK;
	bt_bas_bls_update_battery_level_status();
}

void bt_bas_bls_set_battery_fault(enum bt_bas_bls_battery_fault value)
{
	bls.additional_status &= ~BATTERY_FAULT_MASK;
	bls.additional_status |= (value << BATTERY_FAULT_SHIFT) & BATTERY_FAULT_MASK;
	bt_bas_bls_update_battery_level_status();
}
#endif /* CONFIG_BT_BAS_BLS_ADDITIONAL_STATUS_PRESENT */
