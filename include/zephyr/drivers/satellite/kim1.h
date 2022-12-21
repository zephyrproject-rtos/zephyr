/*
 * Copyright (c) 2022 Kickmaker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SATELLITE_KINEIS_KIM1_H_
#define ZEPHYR_INCLUDE_DRIVERS_SATELLITE_KINEIS_KIM1_H_

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _ID		"ID"
#define _FW		"FW"
#define _SN		"SN"
#define _PWR	"PWR"
#define _FREQ	"ATXFRQ"

#define MDM_RING_BUF_SIZE	CONFIG_SATELLITE_KINEIS_KIM1_MDM_RING_BUF_SIZE
#define MDM_RECV_MAX_BUF	CONFIG_SATELLITE_KINEIS_KIM1_MDM_RX_BUF_COUNT
#define MDM_RECV_BUF_SIZE	CONFIG_SATELLITE_KINEIS_KIM1_MDM_RX_BUF_SIZE

#define KIM1_CMD_TIMEOUT	K_SECONDS(10)
#define KIM1_INIT_TIMEOUT	K_SECONDS(10)
#define KIM1_TX_TIMEOUT		K_SECONDS(10)

#define KIM1_MAX_TX_MESSAGE_SIZE		31U
#define KIM1_MAX_TX_MESSAGE_SIZE_HEXA	KIM1_MAX_TX_MESSAGE_SIZE*2

#define KIM1_TX_FREQ_MAX				401680000U
#define KIM1_TX_FREQ_MIN				399910000U

#define KIM1_ID_MAX_LENGTH				28U
#define KIM1_FW_MAX_LENGTH				20U
#define KIM1_SN_MAX_LENGTH				32U

#define KIM1_ERROR_ID				0
#define KIM1_ERROR_PARAMETER_INDEX	1

/* pin settings */
enum kim_control_pins {
#if DT_INST_NODE_HAS_PROP(0, power_gpios)
	KIM_POWER = 0,
#endif
#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	KIM_RESET,
#endif
#if DT_INST_NODE_HAS_PROP(0, pgood_gpios)
	KIM_PGOOD,
#endif
#if DT_INST_NODE_HAS_PROP(0, tx_status_gpios)
	KIM_TX_STATUS,
#endif
	NUM_PINS,
};

/* available tx power values */
enum kim1_tx_power_enum {
	KIM1_TX_PWR_250,
	KIM1_TX_PWR_500,
	KIM1_TX_PWR_750,
	KIM1_TX_PWR_1000,
	KIM1_TX_PWR_MAX,
};

/* corresponding tx power values */
static const uint16_t kim1_tx_power_value[] = {
	[KIM1_TX_PWR_250] = 250,
	[KIM1_TX_PWR_500] = 500,
	[KIM1_TX_PWR_750] = 750,
	[KIM1_TX_PWR_1000] = 1000,
	[KIM1_TX_PWR_MAX] = 1000
};

/* available modem error values */
enum kim1_error_enum {
	KIM1_ERROR_1 = 1,
	KIM1_ERROR_2,
	KIM1_ERROR_3,
	KIM1_ERROR_4,
	KIM1_ERROR_5,
	KIM1_ERROR_6,
	KIM1_ERROR_7,
	KIM1_ERROR_8,
	KIM1_ERROR_MAX,
};

/* corresponding modem error codes */
static const char * const kim_error_description[] = {
	[KIM1_ERROR_1] = "Parameter formatting issue",
	[KIM1_ERROR_2] = "Parameter out of range",
	[KIM1_ERROR_3] = "Missing parameters",
	[KIM1_ERROR_4] = "Too many parameters",
	[KIM1_ERROR_5] = "Invalid user data length",
	[KIM1_ERROR_6] = "Invalid user data format",
	[KIM1_ERROR_7] = "Incompatible value for parameter",
	[KIM1_ERROR_8] = "Unknown AT command"
};

/**
 * @brief Switch on and check modem is operationnal
 *
 * @return int negative errno, 0 on success
 */
int kineis_switch_on(void);

/**
 * @brief Switch off modem
 *
 * @return int negative errno, 0 on success
 */
int kineis_switch_off(void);

/**
 * @brief Get modem ID
 *
 * @return char* modem ID
 */
char *kineis_get_modem_id(void);

/**
 * @brief Get modem FW version
 *
 * @return char* modem FW version
 */
char *kineis_get_modem_fw_version(void);

/**
 * @brief Get modem serial number
 *
 * @return char* modem serial number
 */
char *kineis_get_modem_serial_number(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SATELLITE_KINEIS_KIM1_H_ */
