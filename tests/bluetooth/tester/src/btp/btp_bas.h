/*
 * Copyright (c) 2024 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BTP_BAS_H_
#define BTP_BAS_H_

#include <stdint.h>
#include <stddef.h>

/* BTP BAS Service Opcodes */
#define BTP_BAS_READ_SUPPORTED_COMMANDS       0x01
#define BTP_BAS_SET_BATTERY_LEVEL             0x02
#define BTP_BAS_BLS_SET_BATTERY_PRESENT       0x03
#define BTP_BAS_BLS_SET_WIRED_POWER_SOURCE    0x04
#define BTP_BAS_BLS_SET_WIRELESS_POWER_SOURCE 0x05
#define BTP_BAS_BLS_SET_BATTERY_CHARGE_STATE  0x06
#define BTP_BAS_BLS_SET_BATTERY_CHARGE_LEVEL  0x07
#define BTP_BAS_BLS_SET_BATTERY_CHARGE_TYPE   0x08
#define BTP_BAS_BLS_SET_CHARGING_FAULT_REASON 0x09
#define BTP_BAS_BLS_SET_IDENTIFIER            0x0A
#define BTP_BAS_BLS_SET_SERVICE_REQUIRED      0x0B
#define BTP_BAS_BLS_SET_BATTERY_FAULT         0x0C

/* Command structures */
struct btp_bas_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

struct btp_bas_set_battery_level_cmd {
	uint8_t level;
} __packed;

struct btp_bas_bls_set_battery_present_cmd {
	uint8_t present;
} __packed;

struct btp_bas_bls_set_wired_power_source_cmd {
	uint8_t source;
} __packed;

struct btp_bas_bls_set_wireless_power_source_cmd {
	uint8_t source;
} __packed;

struct btp_bas_bls_set_battery_charge_state_cmd {
	uint8_t state;
} __packed;

struct btp_bas_bls_set_battery_charge_level_cmd {
	uint8_t level;
} __packed;

struct btp_bas_bls_set_battery_charge_type_cmd {
	uint8_t type;
} __packed;

struct btp_bas_bls_set_charging_fault_reason_cmd {
	uint8_t reason;
} __packed;

struct btp_bas_bls_set_identifier_cmd {
	uint16_t identifier;
} __packed;
struct btp_bas_bls_set_service_required_cmd {
	uint8_t service_required;
} __packed;
struct btp_bas_bls_set_battery_fault_cmd {
	uint8_t battery_fault;
} __packed;

/**
 * @brief Initialize the Battery Service (BAS) tester.
 *
 * @return Status of the initialization. Returns `BTP_STATUS_SUCCESS`
 *         if the initialization is successful. Other values indicate failure.
 */
uint8_t tester_init_bas(void);

/**
 * @brief Unregister the Battery Service (BAS) tester.
 *
 * @return Status of the unregistration. Returns `BTP_STATUS_SUCCESS`
 *         if the unregistration is successful. Other values indicate failure.
 */
uint8_t tester_unregister_bas(void);

#endif /* BTP_BAS_H_ */
