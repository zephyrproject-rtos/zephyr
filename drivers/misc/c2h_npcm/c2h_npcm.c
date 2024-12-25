/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcm_c2h

/*
 * NPCM series enables the Core to access the Host module registers
 * (i.e., Host Configuration, UART, Keyboard/Mouse interface, Power Management
 * Channels 1, 2, 3 and 4, SHM and MSWC), using the Host Modules Internal
 * Bus (HMIB = SIB).
 *
 * The terms "C2H" and "SIB" will be used interchangeably in this driver.
 */

#include <assert.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(c2h_npcm, CONFIG_LOG_DEFAULT_LEVEL);

#define NPCM_C2H_TRANSACTION_TIMEOUT_US 200

struct c2h_npcm_config {
	struct c2h_reg *const inst_c2h;
};

/* host core-to-host interface local functions */
static void host_c2h_wait_write_done(const struct device *dev)
{
	struct c2h_npcm_config *cfg = (struct c2h_npcm_config *)dev->config;
	struct c2h_reg *const inst_c2h = cfg->inst_c2h;
	uint32_t elapsed_cycles;
	uint32_t start_cycles = k_cycle_get_32();
	uint32_t max_wait_cycles =
			k_us_to_cyc_ceil32(NPCM_C2H_TRANSACTION_TIMEOUT_US);

	while (IS_BIT_SET(inst_c2h->SIBCTRL, NPCM_SIBCTRL_CSWR)) {
		elapsed_cycles = k_cycle_get_32() - start_cycles;
		if (elapsed_cycles > max_wait_cycles) {
			LOG_ERR("c2h write transaction expired!");
			break;
		}
	}
}

static void host_c2h_wait_read_done(const struct device *dev)
{
	struct c2h_npcm_config *cfg = (struct c2h_npcm_config *)dev->config;
	struct c2h_reg *const inst_c2h = cfg->inst_c2h;
	uint32_t elapsed_cycles;
	uint32_t start_cycles = k_cycle_get_32();
	uint32_t max_wait_cycles =
			k_us_to_cyc_ceil32(NPCM_C2H_TRANSACTION_TIMEOUT_US);

	while (IS_BIT_SET(inst_c2h->SIBCTRL, NPCM_SIBCTRL_CSRD)) {
		elapsed_cycles = k_cycle_get_32() - start_cycles;
		if (elapsed_cycles > max_wait_cycles) {
			LOG_ERR("c2h read transaction expired!");
			break;
		}
	}
}

void __c2h_config_reg_access(const struct device *dev, SIB_DEVICE_T device,
		uint16_t offset)
{
	struct c2h_npcm_config *cfg = (struct c2h_npcm_config *)dev->config;
	struct c2h_reg *const inst_c2h = cfg->inst_c2h;

	/* Enable Core-to-Host access module */
	inst_c2h->CRSMAE |= device;

	/* Verify core-to-host modules is not in progress */
	host_c2h_wait_read_done(dev);
	host_c2h_wait_write_done(dev);

	/*
	 * Specifying the in-direct IO address which A0 = offset indicates the
	 * index register is accessed. Then write index address directly and
	 * it starts a write transaction to host sub-module on LPC/eSPI bus.
	 */
	inst_c2h->IHIOA = offset;
}

void __c2h_write_reg(const struct device *dev, SIB_DEVICE_T device,
		uint16_t offset, uint8_t value)
{
	struct c2h_npcm_config *cfg = (struct c2h_npcm_config *)dev->config;
	struct c2h_reg *const inst_c2h = cfg->inst_c2h;

	__c2h_config_reg_access(dev, device, offset);

	/*
	 * Specifying the in-direct IO address which A0 = 1 indicates the data
	 * register is accessed. Then write data directly and it starts a write
	 * transaction to host sub-module on LPC/eSPI bus.
	 */
	inst_c2h->IHD = value;
	host_c2h_wait_write_done(dev);
}

uint8_t __c2h_read_reg(const struct device *dev, SIB_DEVICE_T device,
		uint16_t offset)
{
	struct c2h_npcm_config *cfg = (struct c2h_npcm_config *)dev->config;
	struct c2h_reg *const inst_c2h = cfg->inst_c2h;
	uint8_t data_val;

	__c2h_config_reg_access(dev, device, offset);

	/*
	 * Specifying the in-direct IO address which A0 = 1 indicates the data
	 * register is accessed. Then write data directly and it starts a write
	 * transaction to host sub-module on LPC/eSPI bus.
	 */
	inst_c2h->SIBCTRL |= BIT(NPCM_SIBCTRL_CSRD);
	host_c2h_wait_read_done(dev);
	data_val = inst_c2h->IHD;

	return data_val;
}

void c2h_write_io_cfg_reg(const struct device *dev, uint8_t reg_index, uint8_t reg_data)
{
	struct c2h_npcm_config *cfg = (struct c2h_npcm_config *)dev->config;
	struct c2h_reg *const inst_c2h = cfg->inst_c2h;

	/* Disable interrupts */
	int key = irq_lock();

	/* Lock host access EC configuration registers (0x4E/0x4F) */
	inst_c2h->LKSIOHA |= BIT(NPCM_LKSIOHA_LKCFG);
	/* Enable Core-to-Host access CFG module */
	inst_c2h->CRSMAE |= BIT(NPCM_CRSMAE_CFGAE);

	/* Verify core-to-host modules is not in progress */
	host_c2h_wait_read_done(dev);
	host_c2h_wait_write_done(dev);

	/*
	 * Specifying the in-direct IO address which A0 = 0 indicates the index
	 * register is accessed. Then write index address directly and it starts
	 * a write transaction to host sub-module on LPC/eSPI bus.
	 */
	inst_c2h->IHIOA = 0;
	inst_c2h->IHD = reg_index;
	host_c2h_wait_write_done(dev);

	/*
	 * Specifying the in-direct IO address which A0 = 1 indicates the data
	 * register is accessed. Then write data directly and it starts a write
	 * transaction to host sub-module on LPC/eSPI bus.
	 */
	inst_c2h->IHIOA = 1;
	inst_c2h->IHD = reg_data;
	host_c2h_wait_write_done(dev);

	/* Disable Core-to-Host access CFG module */
	inst_c2h->CRSMAE &= ~BIT(NPCM_CRSMAE_CFGAE);
	/* Unlock host access EC configuration registers (0x4E/0x4F) */
	inst_c2h->LKSIOHA &= ~BIT(NPCM_LKSIOHA_LKCFG);

	/* Enable interrupts */
	irq_unlock(key);
}

uint8_t c2h_read_io_cfg_reg(const struct device *dev, uint8_t reg_index)
{
	struct c2h_npcm_config *cfg = (struct c2h_npcm_config *)dev->config;
	struct c2h_reg *const inst_c2h = cfg->inst_c2h;
	uint8_t data_val;

	/* Disable interrupts */
	int key = irq_lock();

	/* Lock host access EC configuration registers (0x4E/0x4F) */
	inst_c2h->LKSIOHA |= BIT(NPCM_LKSIOHA_LKCFG);
	/* Enable Core-to-Host access CFG module */
	inst_c2h->CRSMAE |= BIT(NPCM_CRSMAE_CFGAE);

	/* Verify core-to-host modules is not in progress */
	host_c2h_wait_read_done(dev);
	host_c2h_wait_write_done(dev);

	/*
	 * Specifying the in-direct IO address which A0 = 0 indicates the index
	 * register is accessed. Then write index address directly and it starts
	 * a write transaction to host sub-module on LPC/eSPI bus.
	 */
	inst_c2h->IHIOA = 0;
	inst_c2h->IHD = reg_index;
	host_c2h_wait_write_done(dev);

	/*
	 * Specifying the in-direct IO address which A0 = 1 indicates the data
	 * register is accessed. Then write CSRD bit in SIBCTRL to issue a read
	 * transaction to host sub-module on LPC/eSPI bus. Once it was done,
	 * read data out from IHD.
	 */
	inst_c2h->IHIOA = 1;
	inst_c2h->SIBCTRL |= BIT(NPCM_SIBCTRL_CSRD);
	host_c2h_wait_read_done(dev);
	data_val = inst_c2h->IHD;

	/* Disable Core-to-Host access CFG module */
	inst_c2h->CRSMAE &= ~BIT(NPCM_CRSMAE_CFGAE);
	/* Unlock host access EC configuration registers (0x4E/0x4F) */
	inst_c2h->LKSIOHA &= ~BIT(NPCM_LKSIOHA_LKCFG);

	/* Enable interrupts */
	irq_unlock(key);

	return data_val;
}

void rtc_write_offset(const struct device *dev, SIB_RTC_OFFSET_Enum offset,
		uint8_t value)
{
	struct c2h_npcm_config *cfg = (struct c2h_npcm_config *)dev->config;
	struct c2h_reg *const inst_c2h = cfg->inst_c2h;

	/* Disable interrupts */
	int key = irq_lock();

	/* Lock host access EC configuration registers (0x4E/0x4F) */
	inst_c2h->LKSIOHA |= BIT(NPCM_LKSIOHA_LKCFG);
	__c2h_write_reg(dev, SIB_DEV_RTC, RTC_INDEX, offset);
	__c2h_write_reg(dev, SIB_DEV_RTC, RTC_DATA, value);

	/* Disable Core-to-Host access 'reg' module */
	inst_c2h->CRSMAE &= (SIB_DEVICE_T)~SIB_DEV_RTC;
	/* Unlock host access EC configuration registers (0x4E/0x4F) */
	inst_c2h->LKSIOHA &= ~BIT(NPCM_LKSIOHA_LKCFG);

	/* Enable interrupts */
	irq_unlock(key);
}

uint8_t rtc_read_offset(const struct device *dev, SIB_RTC_OFFSET_Enum offset)
{
	struct c2h_npcm_config *cfg = (struct c2h_npcm_config *)dev->config;
	struct c2h_reg *const inst_c2h = cfg->inst_c2h;
	uint8_t value;

	/* Disable interrupts */
	int key = irq_lock();

	/* Lock host access EC configuration registers (0x4E/0x4F) */
	inst_c2h->LKSIOHA |= BIT(NPCM_LKSIOHA_LKCFG);
	__c2h_write_reg(dev, SIB_DEV_RTC, RTC_INDEX, offset);
	value = __c2h_read_reg(dev, SIB_DEV_RTC, RTC_DATA);

	/* Disable Core-to-Host access 'reg' module */
	inst_c2h->CRSMAE &= (SIB_DEVICE_T)~SIB_DEV_RTC;
	/* Unlock host access EC configuration registers (0x4E/0x4F) */
	inst_c2h->LKSIOHA &= ~BIT(NPCM_LKSIOHA_LKCFG);

	/* Enable interrupts */
	irq_unlock(key);

	return value;
}

/* C2H driver registration */
#define NPCM_C2H_INIT_FUNC(inst) _CONCAT(c2h_init, inst)
#define NPCM_C2H_INIT_FUNC_DECL(inst) \
	static int c2h_init##inst(const struct device *dev)

/* C2H init function implementation */
#define NPCM_C2H_INIT_FUNC_IMPL(inst)				              \
	static int c2h_init##inst(const struct device *dev)	                      \
	{								              \
		struct c2h_npcm_config *cfg = (struct c2h_npcm_config *)dev->config;  \
		struct c2h_reg *const inst_c2h = cfg->inst_c2h;		              \
									              \
		/* Enable Core-to-Host access module */			              \
		inst_c2h->SIBCTRL |= BIT(NPCM_SIBCTRL_CSAE);		              \
									              \
		return 0;						              \
	}								              \

#define NPCM_C2H_INIT(inst)						                        \
	NPCM_C2H_INIT_FUNC_DECL(inst);								\
									                        \
	static const struct c2h_npcm_config c2h_npcm_config_##inst = {	                        \
		.inst_c2h = (struct c2h_reg *)DT_INST_REG_ADDR(inst),				\
	};								                        \
									                        \
	DEVICE_DT_INST_DEFINE(inst,					                        \
			      NPCM_C2H_INIT_FUNC(inst),						\
			      NULL,					                        \
			      NULL, &c2h_npcm_config_##inst,		                        \
			      PRE_KERNEL_1,				                        \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);                        \
									                        \
									                        \
	NPCM_C2H_INIT_FUNC_IMPL(inst)

DT_INST_FOREACH_STATUS_OKAY(NPCM_C2H_INIT)
