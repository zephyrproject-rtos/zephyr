/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_CAN_STM32_CAN_H_
#define ZEPHYR_DRIVERS_CAN_STM32_CAN_H_

#include <drivers/can.h>

#define DEV_DATA(dev) ((struct can_stm32_data *const)(dev)->data)
#define DEV_CFG(dev) \
	((const struct can_stm32_config *const)(dev)->config)

/*
 * The STM32 HAL has no define for the number of banks, but it
 * does have a struct CAN_TypeDef with an array member that has the
 * length of the number of filters (28 on the F4).
 * We use this array to calculate the number of filter banks the
 * CPU supports
 */
#define CAN_STM32_FILTER_BANK_COUNT \
	(sizeof(((const CAN_TypeDef *)0)->sFilterRegister) \
	 / sizeof(CAN_FilterRegister_TypeDef))

/* Every bank can hold 4 std ids, 2 ext ids, 2 std masks, or 1 ext mask
 * So the max number of filters is the bank count times 4
 */
#define CAN_STM32_FILTER_COUNT (CAN_STM32_FILTER_BANK_COUNT * 4U)

#define CAN_STM32_SHARED_PORT_COUNT 2U
#define CAN_STM32_FIFO_COUNT 2U
#define CAN_STM32_FILTER_TYPE_COUNT 4U

#define BIT_SEG_LENGTH(cfg) ((cfg)->prop_ts1 + (cfg)->ts2 + 1U)

#define CAN_FIRX_STD_IDE_POS   (3U)
#define CAN_FIRX_STD_RTR_POS   (4U)
#define CAN_FIRX_STD_ID_POS    (5U)

#define CAN_FIRX_EXT_IDE_POS    (2U)
#define CAN_FIRX_EXT_RTR_POS    (1U)
#define CAN_FIRX_EXT_STD_ID_POS (21U)
#define CAN_FIRX_EXT_EXT_ID_POS (3U)

struct can_mailbox {
	can_tx_callback_t tx_callback;
	void *callback_arg;
	struct k_sem tx_int_sem;
	uint32_t error_flags;
};

struct can_stm32_filter_data {
	const struct device *can_dev[CAN_STM32_SHARED_PORT_COUNT];
	uint8_t filter_map[CAN_STM32_FILTER_COUNT];
	uint8_t map_offset[CAN_STM32_SHARED_PORT_COUNT][CAN_STM32_FIFO_COUNT];
};

/* number = FSCx | FMBx */
enum can_filter_type {
	CAN_FILTER_STANDARD_MASKED = 0U,
	CAN_FILTER_STANDARD = 1U,
	CAN_FILTER_EXTENDED_MASKED = 2U,
	CAN_FILTER_EXTENDED = 3U
};

struct can_stm32_filter {
	can_rx_callback_t rx_cb;
	void *cb_arg;

	uint32_t filter_id;
	uint32_t filter_mask;

	uint8_t fifo_nr:1;
	uint8_t filter_type:2;
};

struct can_stm32_data {
	struct k_mutex inst_mutex;
	struct k_sem tx_int_sem;
	struct can_mailbox mb0;
	struct can_mailbox mb1;
	struct can_mailbox mb2;
	struct can_stm32_filter filters[CONFIG_CAN_MAX_FILTER];
	can_state_change_isr_t state_change_isr;
};

struct can_stm32_config {
	CAN_TypeDef *can;   /*!< CAN Registers*/
	CAN_TypeDef *master_can; /*!< CAN Registers for shared filter */
	struct can_stm32_filter_data *shared_filter_data;
	uint32_t bus_speed;
	uint8_t port_nr;
	uint8_t sjw;
	uint8_t prop_ts1;
	uint8_t ts2;
	struct stm32_pclken pclken;
	void (*config_irq)(CAN_TypeDef *can);
};

#endif /*ZEPHYR_DRIVERS_CAN_STM32_CAN_H_*/
