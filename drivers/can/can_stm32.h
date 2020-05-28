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

#define BIT_SEG_LENGTH(cfg) ((cfg)->prop_ts1 + (cfg)->ts2 + 1)

#define CAN_NUMBER_OF_FILTER_BANKS (14)
#define CAN_MAX_NUMBER_OF_FILTERS (CAN_NUMBER_OF_FILTER_BANKS * 4)

#define CAN_FIRX_STD_IDE_POS   (3U)
#define CAN_FIRX_STD_RTR_POS   (4U)
#define CAN_FIRX_STD_ID_POS    (5U)

#define CAN_FIRX_EXT_IDE_POS    (2U)
#define CAN_FIRX_EXT_RTR_POS    (1U)
#define CAN_FIRX_EXT_STD_ID_POS (21U)
#define CAN_FIRX_EXT_EXT_ID_POS (3U)

#define CAN_BANK_IS_EMPTY(usage, bank_nr) (((usage >> ((bank_nr) * 4)) & 0x0F) == 0x0F)
#define CAN_BANK_IN_LIST_MODE(can, bank) ((can)->FM1R & (1U << (bank)))
#define CAN_BANK_IN_32BIT_MODE(can, bank) ((can)->FS1R & (1U << (bank)))
#define CAN_IN_16BIT_LIST_MODE(can, bank) (CAN_BANK_IN_LIST_MODE(can, bank) && \
					   !CAN_BANK_IN_32BIT_MODE(can, bank))
#define CAN_IN_16BIT_MASK_MODE(can, bank) (!CAN_BANK_IN_LIST_MODE(can, bank) &&	\
					   !CAN_BANK_IN_32BIT_MODE(can, bank))
#define CAN_IN_32BIT_LIST_MODE(can, bank) (CAN_BANK_IN_LIST_MODE(can, bank) && \
					   CAN_BANK_IN_32BIT_MODE(can, bank))
#define CAN_IN_32BIT_MASK_MODE(can, bank) (!CAN_BANK_IN_LIST_MODE(can, bank) &&	\
					   CAN_BANK_IN_32BIT_MODE(can, bank))
struct can_mailbox {
	can_tx_callback_t tx_callback;
	void *callback_arg;
	struct k_sem tx_int_sem;
	uint32_t error_flags;
};


/* number = FSCx | FMBx */
enum can_filter_type {
	CAN_FILTER_STANDARD_MASKED = 0,
	CAN_FILTER_STANDARD = 1,
	CAN_FILTER_EXTENDED_MASKED = 2,
	CAN_FILTER_EXTENDED = 3
};

struct can_stm32_data {
	struct k_mutex inst_mutex;
	struct k_sem tx_int_sem;
	struct can_mailbox mb0;
	struct can_mailbox mb1;
	struct can_mailbox mb2;
	uint64_t filter_usage;
	can_rx_callback_t rx_cb[CONFIG_CAN_MAX_FILTER];
	void *cb_arg[CONFIG_CAN_MAX_FILTER];
	can_state_change_isr_t state_change_isr;
};

struct can_stm32_config {
	CAN_TypeDef *can;   /*!< CAN Registers*/
	CAN_TypeDef *master_can;   /*!< CAN Registers for shared filter */
	uint32_t bus_speed;
	uint8_t sjw;
	uint8_t prop_ts1;
	uint8_t ts2;
	struct stm32_pclken pclken;
	void (*config_irq)(CAN_TypeDef *can);
};

#endif /*ZEPHYR_DRIVERS_CAN_STM32_CAN_H_*/
