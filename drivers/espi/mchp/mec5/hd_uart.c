/*
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mec5_host_uart

#include <zephyr/kernel.h>
#include <soc.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>
#include "espi_mchp_mec5_private.h"

/* MEC5 HAL */
#include <device_mec5.h>
#include <mec_retval.h>
#include <mec_pcr_api.h>
#include <mec_espi_api.h>
#include <mec_uart_api.h>

LOG_MODULE_REGISTER(espi_host_uart, CONFIG_ESPI_LOG_LEVEL);

struct mec5_host_uart_devcfg {
	struct uart_regs *regs;
	const struct device *parent;
	const struct pinctrl_dev_config *pcfg;
	uint16_t host_io_addr;
	uint8_t ldn;
	uint8_t sirq;
};

struct mec5_host_uart_driver_api {
	int (*host_access_enable)(const struct device *dev, uint8_t enable, uint32_t cfg);
};

/* Called by eSPI parent driver when platform reset has de-asserted.
 * Program eSPI I/O BARs and Serial IRQ
 */
static int mec5_uart_host_access_en(const struct device *dev, uint8_t enable, uint32_t cfg)
{
	const struct mec5_host_uart_devcfg *const devcfg = dev->config;
	struct uart_regs *const regs = devcfg->regs;
	uint32_t sirqcfg = devcfg->ldn;
	uint32_t barcfg = devcfg->ldn | BIT(ESPI_MEC5_BAR_CFG_EN_POS);
	int ret = 0;

	ret = mec_uart_power_on(regs, MEC5_UART_CFG_RESET_HOST);
	if (ret != MEC_RET_OK) {
		LOG_ERR("HAL uart power on failed: (%d)", ret);
		return -EIO;
	}

	ret = espi_mec5_bar_config(devcfg->parent, devcfg->host_io_addr, barcfg);
	if (ret) {
		return ret;
	}

	sirqcfg |= (((uint32_t)devcfg->sirq << ESPI_MEC5_SIRQ_CFG_SLOT_POS)
		    & ESPI_MEC5_SIRQ_CFG_SLOT_MSK);
	ret = espi_mec5_sirq_config(devcfg->parent, sirqcfg);

	return ret;
}

/* We are not supporting the interrupt enable API. Host owns this UART
 * and controls interrupts through the UART registers exposed to it.
 */
static const struct mec5_host_uart_driver_api mec5_host_uart_drv_api = {
	.host_access_enable = mec5_uart_host_access_en,
};

static int mec5_host_uart_init(const struct device *dev)
{
	const struct mec5_host_uart_devcfg *const devcfg = dev->config;

	int ret = pinctrl_apply_state(devcfg->pcfg, PINCTRL_STATE_DEFAULT);

	if (ret) {
		LOG_ERR("pinctrl dflt state (%d)", ret);
		return ret;
	}

	return 0;
}

#define MEC5_DT_UART_NODE(inst) DT_INST(inst, DT_DRV_COMPAT)

#define MEC5_DT_UART_HW_NODE(inst) DT_PHANDLE(MEC5_DT_UART_NODE(inst), hwdev)
#define MEC5_DT_UART_HW_REGS(inst) DT_REG_ADDR(MEC5_DT_UART_HW_NODE(inst))

#define MEC5_DT_UART_HA(inst) \
	DT_PROP_BY_PHANDLE_IDX(MEC5_DT_UART_NODE(inst), host_infos, 0, host_address)

#define MEC5_DT_UART_LDN(inst) \
	DT_PROP_BY_PHANDLE_IDX(MEC5_DT_UART_NODE(inst), host_infos, 0, ldn)

/* return node indentifier of the first entry in host_infos phandles */
#define MEC5_DT_UART_HI_NODE(inst) DT_PHANDLE_BY_IDX(MEC5_DT_UART_NODE(inst), host_infos, 0)

/* UART can generate one Serial IRQ to the Host */
#define MEC5_DT_UART_SIRQ(inst) DT_PROP_BY_IDX(MEC5_DT_UART_HI_NODE(inst), sirqs, 0)

#define MEC5_UART_HOST_DEVICE(inst)							\
	PINCTRL_DT_INST_DEFINE(inst);							\
											\
	static const struct mec5_host_uart_devcfg mec5_host_uart_dcfg_##inst = {	\
		.regs = (struct uart_regs *)MEC5_DT_UART_HW_REGS(inst),			\
		.parent = DEVICE_DT_GET(DT_INST_PARENT(inst)),				\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),				\
		.host_io_addr = MEC5_DT_UART_HA(inst),					\
		.ldn = MEC5_DT_UART_LDN(inst),						\
		.sirq = MEC5_DT_UART_SIRQ(inst),					\
	};										\
	DEVICE_DT_INST_DEFINE(inst, mec5_host_uart_init, NULL, NULL,			\
			&mec5_host_uart_dcfg_##inst,					\
			POST_KERNEL, CONFIG_ESPI_INIT_PRIORITY,				\
			&mec5_host_uart_drv_api);

DT_INST_FOREACH_STATUS_OKAY(MEC5_UART_HOST_DEVICE)
