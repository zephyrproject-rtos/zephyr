/*
 * Xilinx Processor System MIO / EMIO GPIO controller driver
 *
 * Driver private data declarations, GPIO bank module
 *
 * Copyright (c) 2022, Weidmueller Interface GmbH & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ZEPHYR_DRIVERS_GPIO_GPIO_XLNX_PS_BANK_H_
#define _ZEPHYR_DRIVERS_GPIO_GPIO_XLNX_PS_BANK_H_

/*
 * Register address calculation macros
 * Register address offsets: comp. Zynq-7000 TRM, ug585, chap. B.19
 */
#define GPIO_XLNX_PS_BANK_MASK_DATA_LSW_REG (dev_conf->base_addr\
	+ ((uint32_t)dev_conf->bank_index * 0x8))
#define GPIO_XLNX_PS_BANK_MASK_DATA_MSW_REG ((dev_conf->base_addr + 0x04)\
	+ ((uint32_t)dev_conf->bank_index * 0x8))
#define GPIO_XLNX_PS_BANK_DATA_REG          ((dev_conf->base_addr + 0x40)\
	+ ((uint32_t)dev_conf->bank_index * 0x4))
#define GPIO_XLNX_PS_BANK_DATA_RO_REG       ((dev_conf->base_addr + 0x60)\
	+ ((uint32_t)dev_conf->bank_index * 0x4))
#define GPIO_XLNX_PS_BANK_DIRM_REG          ((dev_conf->base_addr + 0x204)\
	+ ((uint32_t)dev_conf->bank_index * 0x40))
#define GPIO_XLNX_PS_BANK_OEN_REG           ((dev_conf->base_addr + 0x208)\
	+ ((uint32_t)dev_conf->bank_index * 0x40))
#define GPIO_XLNX_PS_BANK_INT_MASK_REG      ((dev_conf->base_addr + 0x20C)\
	+ ((uint32_t)dev_conf->bank_index * 0x40))
#define GPIO_XLNX_PS_BANK_INT_EN_REG        ((dev_conf->base_addr + 0x210)\
	+ ((uint32_t)dev_conf->bank_index * 0x40))
#define GPIO_XLNX_PS_BANK_INT_DIS_REG       ((dev_conf->base_addr + 0x214)\
	+ ((uint32_t)dev_conf->bank_index * 0x40))
#define GPIO_XLNX_PS_BANK_INT_STAT_REG      ((dev_conf->base_addr + 0x218)\
	+ ((uint32_t)dev_conf->bank_index * 0x40))
#define GPIO_XLNX_PS_BANK_INT_TYPE_REG      ((dev_conf->base_addr + 0x21C)\
	+ ((uint32_t)dev_conf->bank_index * 0x40))
#define GPIO_XLNX_PS_BANK_INT_POLARITY_REG  ((dev_conf->base_addr + 0x220)\
	+ ((uint32_t)dev_conf->bank_index * 0x40))
#define GPIO_XLNX_PS_BANK_INT_ANY_REG       ((dev_conf->base_addr + 0x224)\
	+ ((uint32_t)dev_conf->bank_index * 0x40))

/**
 * @brief Run-time modifiable device data structure.
 *
 * This struct contains all data of a PS MIO / EMIO GPIO bank
 * which is modifiable at run-time, such as the configuration
 * parameters and current values of each individual pin belonging
 * to the respective bank.
 */
struct gpio_xlnx_ps_bank_dev_data {
	struct gpio_driver_data common;
	sys_slist_t callbacks;
};

/**
 * @brief Constant device configuration data structure.
 *
 * This struct contains all data of a PS MIO / EMIO GPIO bank
 * which is required for proper operation (such as base memory
 * addresses etc.) which don't have to and therefore cannot be
 * modified at run-time.
 */
struct gpio_xlnx_ps_bank_dev_cfg {
	struct gpio_driver_config common;

	uint32_t base_addr;
	uint8_t bank_index;
};

#endif /* _ZEPHYR_DRIVERS_GPIO_GPIO_XLNX_PS_BANK_H_ */
