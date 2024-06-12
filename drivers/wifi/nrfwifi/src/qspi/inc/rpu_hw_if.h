/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Header containing common functions for RPU hardware interaction
 * using QSPI and SPI that can be invoked by shell or the driver.
 */

#ifndef __RPU_HW_IF_H_
#define __RPU_HW_IF_H_

#include <stdio.h>
#include <stdlib.h>
#include <zephyr/drivers/gpio.h>

enum {
	SYSBUS = 0,
	EXT_SYS_BUS,
	PBUS,
	PKTRAM,
	GRAM,
	LMAC_ROM,
	LMAC_RET_RAM,
	LMAC_SRC_RAM,
	UMAC_ROM,
	UMAC_RET_RAM,
	UMAC_SRC_RAM,
	NUM_MEM_BLOCKS
};

extern char blk_name[][15];
extern uint32_t rpu_7002_memmap[][3];

int rpu_read(unsigned int addr, void *data, int len);
int rpu_write(unsigned int addr, const void *data, int len);

int rpu_sleep(void);
int rpu_wakeup(void);
int rpu_sleep_status(void);
void rpu_get_sleep_stats(uint32_t addr, uint32_t *buff, uint32_t wrd_len);
int rpu_irq_config(struct gpio_callback *irq_callback_data, void (*irq_handler)());
int rpu_irq_remove(struct gpio_callback *irq_callback_data);

int rpu_wrsr2(uint8_t data);
int rpu_rdsr2(void);
int rpu_rdsr1(void);
int rpu_clks_on(void);

int rpu_init(void);
int rpu_enable(void);
int rpu_disable(void);

#ifdef CONFIG_NRF70_SR_COEX_RF_SWITCH
int sr_ant_switch(unsigned int ant_switch);
int sr_gpio_remove(void);
int sr_gpio_config(void);
#endif /* CONFIG_NRF70_SR_COEX_RF_SWITCH */
#endif /* __RPU_HW_IF_H_ */
