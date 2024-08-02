/*
 * Copyright (c) 2019 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DEVICE_POWER_H
#define __DEVICE_POWER_H

#ifndef _ASMLANGUAGE

#define DEBUG_SLEEP
#define DEBUG_DEEP_SLEEP_CLK_REQ

/*
 * Disable UART deep sleep save/restore. If a UART TX FIFO has data on deep
 * sleep entry it will de-assert its CLK_REQ once TX FIFO empties.  If the
 * UART has TX empty interrupt enabled then the system will wake.
 */
/* #define DEEP_SLEEP_UART_SAVE_RESTORE */

/* Comment out to use JTAG without interruptions.
 * Beware this blocks PLL going off, hence should be enabled
 * for power consumption measurements
 * Note: To attach JTAG for any debug need to be performed with breakpoint
 * prior to deep sleep entry.
 */
/* #define DEEP_SLEEP_JTAG */

/*
 * Enabling this would take a snapshot from clock requested values,
 * these can be dump on exit to identify which HW block is blocking.
 */
#define DEEP_SLEEP_CLK_REQ_DUMP

/*
 * Some peripherals if enabled always assert their CLK_REQ bits.
 * For example, any peripheral with a clock generator such as
 * timers, counters, UART, etc. We save the enables for these
 * peripherals, disable them, and restore the enabled state upon
 * wake.
 */
#define DEEP_SLEEP_PERIPH_SAVE_RESTORE

/* Power optimization if certain HW blocks are not used.
 * These are not part of any Zephyr subsystem.
 *  #define DEEP_SLEEP_PERIPH_SAVE_RESTORE_EXTENDED
 */
#define DEEP_SLEEP_PERIPH_SAVE_RESTORE_EXTENDED

#define NUM_DS_TIMER_ENTRIES 6

struct ds_timer_info {
	uintptr_t addr;
	uint32_t stop_mask;
	uint32_t restore_mask;
};

struct ds_peci_info {
	uint32_t peci_ctrl;
	uint32_t peci_dis;
};

struct ds_dev_info {
	uint32_t ecs[2];
	uint32_t timers[NUM_DS_TIMER_ENTRIES];
#ifdef DEEP_SLEEP_UART_SAVE_RESTORE
	uint8_t uart_info[MCHP_UART_INSTANCES];
#endif
	/* Analog power */
#ifdef CONFIG_PECI
	struct ds_peci_info peci_info;
#endif
#ifdef CONFIG_I2C
	uint32_t smb_info[MCHP_I2C_SMB_INSTANCES];
#endif
	uint32_t slwclk_info;
	uint8_t tfdp_en;
	uint8_t comp_en;
};

void soc_deep_sleep_periph_save(void);
void soc_deep_sleep_periph_restore(void);
void soc_deep_sleep_non_wake_en(void);
void soc_deep_sleep_non_wake_dis(void);
void soc_deep_sleep_wake_en(void);
void soc_deep_sleep_wake_dis(void);

#ifdef DEBUG_DEEP_SLEEP_CLK_REQ
void soc_debug_sleep_clk_req(void);
#endif

#endif /* _ASMLANGUAGE */
#endif /* __DEVICE_POWER_H */
