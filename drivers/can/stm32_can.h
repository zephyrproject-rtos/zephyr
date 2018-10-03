/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_CAN_STM32_CAN_H_
#define ZEPHYR_DRIVERS_CAN_STM32_CAN_H_

#include <can.h>

#define DEV_DATA(dev) ((struct can_stm32_data *const)(dev)->driver_data)
#define DEV_CFG(dev) \
	((const struct can_stm32_config *const)(dev)->config->config_info)

#define BIT_SEG_LENGTH(cfg) ((cfg)->prop_bs1 + (cfg)->bs2 + 1)

#define CAN_NUMBER_OF_FILTER_BANKS (14)
#define CAN_MAX_NUMBER_OF_FILTES (CAN_NUMBER_OF_FILTER_BANKS * 4)

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
	struct k_sem tx_int_sem;
	u32_t error_flags;
};

enum can_filter_type {
	CAN_FILTER_STANDARD,
	CAN_FILTER_STANDARD_MASKED,
	CAN_FILTER_EXTENDED,
	CAN_FILTER_EXTENDED_MASKED
};

struct can_stm32_data {
	struct k_mutex tx_mutex;
	struct k_mutex set_filter_mutex;
	struct k_sem tx_int_sem;
	struct can_mailbox mb0;
	struct can_mailbox mb1;
	struct can_mailbox mb2;
	u64_t filter_usage;
	u64_t response_type;
	void *rx_response[CONFIG_CAN_MAX_FILTER];
};

struct can_stm32_config {
	CAN_TypeDef *can;   /*!< CAN Registers*/
	u32_t bus_speed;
	u8_t swj;
	u8_t prop_bs1;
	u8_t bs2;
	struct stm32_pclken pclken;
	void (*config_irq)(CAN_TypeDef *can);
};

#endif /*ZEPHYR_DRIVERS_CAN_STM32_CAN_H_*/
