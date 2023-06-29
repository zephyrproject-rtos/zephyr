/*
 * Copyright (c) 2023, ithinx GmbH
 * Copyright (c) 2023, Tonies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USBC_DEVICE_UCPD_STM32_PRIV_H_
#define ZEPHYR_DRIVERS_USBC_DEVICE_UCPD_STM32_PRIV_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/usb_c/usbc_tcpc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

#define TPS_MODE_REG                     0x03
#define TPS_MODE_REG_SIZE                4
#define TPS_TYPE_REG                     0x04
#define TPS_TYPE_REG_SIZE                4
#define TPS_CUSTUSE_REG                  0x06
#define TPS_CUSTUSE_REG_SIZE             8
#define TPS_CMD1_REG                     0x08
#define TPS_CMD1_REG_SIZE                4
#define TPS_DATA_REG                     0x09
#define TPS_DATA_REG_SIZE                64
#define TPS_DEVICE_CAP_REG               0x0D
#define TPS_DEVICE_CAP_REG_SIZE          1
#define TPS_VERSION_REG                  0x0F
#define TPS_VERSION_REG_SIZE             4
#define TPS_INT_EVENT1_REG               0x14
#define TPS_INT_EVENT1_REG_SIZE          11
#define TPS_INT_MASK1_REG                0x16
#define TPS_INT_MASK1_REG_SIZE           11
#define TPS_INT_CLEAR1_REG               0x18
#define TPS_INT_CLEAR1_REG_SIZE          11
#define TPS_STATUS_REG                   0x1A
#define TPS_STATUS_REG_SIZE              5
#define TPS_POWER_PATH_STATUS_REG        0x26
#define TPS_POWER_PATH_STATUS_REG_SIZE   5
#define TPS_PORT_CONTROL_REG             0x29
#define TPS_PORT_CONTROL_REG_SIZE        4
#define TPS_BOOT_STATUS_REG              0x2D
#define TPS_BOOT_STATUS_REG_SIZE         5
#define TPS_BUILD_DESCRIPTION            0x2E
#define TPS_BUILD_DESCRIPTION_SIZE       49
#define TPS_DEVICE_INFO_REG              0x2F
#define TPS_DEVICE_INFO_REG_SIZE         40
#define TPS_RX_SOURCE_CAPS_REG           0X30
#define TPS_RX_SOURCE_CAPS_REG_SIZE      29
#define TPS_RX_SINK_CAPS_REG             0X31
#define TPS_RX_SINK_CAPS_REG_SIZE        29
#define TPS_TX_SOURCE_CAPS_REG           0x32
#define TPS_TX_SOURCE_CAPS_REG_SIZE      31
#define TPS_TX_SINK_CAPS_REG             0x33
#define TPS_TX_SINK_CAPS_REG_SIZE        29
#define TPS_ACTIVE_CONTRACT_PDO_REG      0x34
#define TPS_ACTIVE_CONTRACT_PDO_REG_SIZE 6
#define TPS_ACTIVE_CONTRACT_RDO_REG      0x35
#define TPS_ACTIVE_CONTRACT_RDO_REG_SIZE 4
#define TPS_POWER_STATUS_REG             0x3F
#define TPS_POWER_STATUS_REG_SIZE        2
#define TPS_PD_STATUS_REG                0x40
#define TPS_PD_STATUS_REG_SIZE           4
#define TPS_TYPEC_STATE_REG              0x69
#define TPS_TYPEC_STATE_REG_SIZE         4
#define TPS_GPIO_STATUS_REG              0x72
#define TPS_GPIO_STATUS_REG_SIZE         8
#define TPS_CAP_POWER_ROLE_M             0x03
#define TPS_CAP_USBPD_CAP_M              0x04
#define TPS_CAP_I2CMLEVEL_VOLT_M         0x80

enum tps25750_event_bits {
	/* Byte 11: Patch Status (common to all slave ports) */
	EVENT_BIT_I2C_MASTER_NACKED = 82,
	EVENT_BIT_READY_FOR_PATCH = 81,
	EVENT_BIT_PATCH_LOADED = 80,
	/* Bytes 9-10: */
	EVENT_BIT_TX_MEM_BUFFER_EMPTY = 64,
	EVENT_BIT_ERROR_UNABLE_TO_SOURCE = 46,
	EVENT_BIT_PLUG_EARLY_NOTIFICATION = 43,
	EVENT_BIT_SNK_TRANSITION_COMPLETE = 42,
	EVENT_BIT_ERROR_MESSAGE_DATA = 39,
	EVENT_BIT_ERROR_PROTOCOL_ERROR = 38,
	EVENT_BIT_ERROR_MISSING_GET_CAP_MESSAGE = 36,
	EVENT_BIT_ERROR_POWER_EVENT_OCCURRED = 35,
	EVENT_BIT_ERROR_CAN_PROVIDE_VOLTAGE_OR_CURRENT_LATER = 34,
	EVENT_BIT_ERROR_CANNOT_PROVIDE_VOLTAGE_OR_CURRENT = 33,
	EVENT_BIT_ERROR_DEVICE_INCOMPATIBLE = 32,
	/* Bytes 1-4: */
	EVENT_BIT_CMD_COMPLETE = 30,
	EVENT_BIT_PD_STATUS_UPDATE = 27,
	EVENT_BIT_STATUS_UPDATE = 26,
	EVENT_BIT_POWER_STATUS_UPDATE = 24,
	EVENT_BIT_P_PSWITCH_CHANGED = 23,
	EVENT_BIT_USB_HOST_PRESENT_NO_LONGER = 21,
	EVENT_BIT_USB_HOST_PRESENT = 20,
	EVENT_BIT_DR_SWAP_REQUESTED = 18,
	EVENT_BIT_PR_SWAP_REQUESTED = 17,
	EVENT_BIT_SOURCE_CAP_MSG_RCVD = 14,
	EVENT_BIT_NEW_CONTRACT_AS_PROV = 13,
	EVENT_BIT_NEW_CONTRACT_AS_CONS = 12,
	EVENT_BIT_DR_SWAP_COMPLETE = 5,
	EVENT_BIT_PR_SWAP_COMPLETE = 4,
	EVENT_BIT_PLUG_INSERT_OR_REMOVAL = 3,
	EVENT_BIT_PD_HARD_RESET = 1,
};

/**
 * @brief Alert handler information
 */
struct alert_info {
	/* Runtime device structure */
	const struct device *dev;
	/* Application supplied data that's passed to the
	 * application's alert handler callback
	 **/
	void *data;
	/* Application's alert handler callback */
	tcpc_alert_handler_cb_t handler;
	/*
	 * Kernel worker used to call the application's
	 * alert handler callback
	 */
	struct k_work work;
};

/**
 * @brief Driver config
 */
struct tcpc_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec irq_gpio;
};

/**
 * @brief Driver data
 */
struct tcpc_data {
	struct alert_info alert_info;
	struct gpio_callback gpio_cb;
};

#endif /* ZEPHYR_DRIVERS_USBC_DEVICE_UCPD_STM32_PRIV_H_ */
