/*
 * Driver for Synopsys DesignWare MAC
 *
 * Copyright (c) 2021 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * STM32H7X specific glue.
 */


#define LOG_MODULE_NAME dwmac_plat
#define LOG_LEVEL CONFIG_ETHERNET_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/* be compatible with the HAL-based driver here */
#define DT_DRV_COMPAT st_stm32_ethernet

#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <ethernet/eth.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>

#include "eth_dwmac_priv.h"

PINCTRL_DT_INST_DEFINE(0);
static const struct pinctrl_dev_config *eth0_pcfg =
	PINCTRL_DT_INST_DEV_CONFIG_GET(0);

static const struct stm32_pclken pclken = {
	.bus = DT_INST_CLOCKS_CELL_BY_NAME(0, stmmaceth, bus),
	.enr = DT_INST_CLOCKS_CELL_BY_NAME(0, stmmaceth, bits),
};
static const struct stm32_pclken pclken_tx = {
	.bus = DT_INST_CLOCKS_CELL_BY_NAME(0, mac_clk_tx, bus),
	.enr = DT_INST_CLOCKS_CELL_BY_NAME(0, mac_clk_tx, bits),
};
static const struct stm32_pclken pclken_rx = {
	.bus = DT_INST_CLOCKS_CELL_BY_NAME(0, mac_clk_rx, bus),
	.enr = DT_INST_CLOCKS_CELL_BY_NAME(0, mac_clk_rx, bits),
};

int dwmac_bus_init(struct dwmac_priv *p)
{
	uint32_t reg_addr, reg_val;
	int ret;

	p->clock = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(p->clock)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	ret  = clock_control_on(p->clock, (clock_control_subsys_t)&pclken);
	ret |= clock_control_on(p->clock, (clock_control_subsys_t)&pclken_tx);
	ret |= clock_control_on(p->clock, (clock_control_subsys_t)&pclken_rx);
	if (ret) {
		LOG_ERR("Failed to enable ethernet clock");
		return -EIO;
	}

	ret = pinctrl_apply_state(eth0_pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Could not configure ethernet pins");
		return ret;
	}

	/* set SYSCFGEN in RCC_APB4ENR */
	reg_addr = DT_REG_ADDR(DT_INST(0, st_stm32h7_rcc)) + 0xf4;
	reg_val = sys_read32(reg_addr);
	sys_write32(reg_val | BIT(1), reg_addr);

	/* set RMII mode in SYSCFG_PMCR */
	reg_addr = 0x58000404;  /* no DT node? */
	reg_val = sys_read32(reg_addr);
	sys_write32(reg_val | 0x03800000, reg_addr);

	p->base_addr = DT_INST_REG_ADDR(0);
	return 0;
}

#if defined(CONFIG_NOCACHE_MEMORY)
#define __desc_mem __nocache __aligned(4)
#else
#error "missing memory attribute for descriptors"
#endif

/* Descriptor rings in uncached memory */
static struct dwmac_dma_desc dwmac_tx_descs[NB_TX_DESCS] __desc_mem;
static struct dwmac_dma_desc dwmac_rx_descs[NB_RX_DESCS] __desc_mem;

void dwmac_platform_init(struct dwmac_priv *p)
{
	p->tx_descs = dwmac_tx_descs;
	p->rx_descs = dwmac_rx_descs;

	/* basic configuration for this platform */
	REG_WRITE(MAC_CONF,
		  MAC_CONF_PS |
		  MAC_CONF_FES |
		  MAC_CONF_DM);
	REG_WRITE(DMA_SYSBUS_MODE,
		  DMA_SYSBUS_MODE_AAL |
		  DMA_SYSBUS_MODE_FB);

	/* set up IRQs (still masked for now) */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), dwmac_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	/* create MAC address */
	gen_random_mac(p->mac_addr, 0x00, 0x80, 0xE1);
}

/* Our private device instance */
static struct dwmac_priv dwmac_instance;

ETH_NET_DEVICE_DT_INST_DEFINE(0,
			      dwmac_probe,
			      NULL,
			      &dwmac_instance,
			      NULL,
			      CONFIG_ETH_INIT_PRIORITY,
			      &dwmac_api,
			      NET_ETH_MTU);
