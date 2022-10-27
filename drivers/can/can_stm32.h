/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CAN_STM32_H_
#define ZEPHYR_DRIVERS_CAN_STM32_H_

#include <zephyr/drivers/can.h>

#define CAN_STM32_NUM_FILTER_BANKS (14)
#define CAN_STM32_MAX_FILTER_ID \
	(CONFIG_CAN_MAX_EXT_ID_FILTER + CONFIG_CAN_MAX_STD_ID_FILTER * 2)

#define CAN_STM32_FIRX_STD_IDE_POS   (3U)
#define CAN_STM32_FIRX_STD_RTR_POS   (4U)
#define CAN_STM32_FIRX_STD_ID_POS    (5U)

#define CAN_STM32_FIRX_EXT_IDE_POS    (2U)
#define CAN_STM32_FIRX_EXT_RTR_POS    (1U)
#define CAN_STM32_FIRX_EXT_STD_ID_POS (21U)
#define CAN_STM32_FIRX_EXT_EXT_ID_POS (3U)

struct can_stm32_mailbox {
	can_tx_callback_t tx_callback;
	void *callback_arg;
};

struct can_stm32_data {
	struct k_mutex inst_mutex;
	struct k_sem tx_int_sem;
	struct can_stm32_mailbox mb0;
	struct can_stm32_mailbox mb1;
	struct can_stm32_mailbox mb2;
	can_rx_callback_t rx_cb_std[CONFIG_CAN_MAX_STD_ID_FILTER];
	can_rx_callback_t rx_cb_ext[CONFIG_CAN_MAX_EXT_ID_FILTER];
	void *cb_arg_std[CONFIG_CAN_MAX_STD_ID_FILTER];
	void *cb_arg_ext[CONFIG_CAN_MAX_EXT_ID_FILTER];
	can_state_change_callback_t state_change_cb;
	void *state_change_cb_data;
	enum can_state state;
	bool started;
};

struct can_stm32_config {
	CAN_TypeDef *can;   /*!< CAN Registers*/
	CAN_TypeDef *master_can;   /*!< CAN Registers for shared filter */
	uint32_t bus_speed;
	uint16_t sample_point;
	uint8_t sjw;
	uint8_t prop_ts1;
	uint8_t ts2;
	struct stm32_pclken pclken;
	void (*config_irq)(CAN_TypeDef *can);
	const struct pinctrl_dev_config *pcfg;
	const struct device *phy;
	uint32_t max_bitrate;
};

#endif /* ZEPHYR_DRIVERS_CAN_STM32_H_ */
