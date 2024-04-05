/*
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mec5_mailbox

#include <zephyr/kernel.h>
#include <soc.h>
#include <errno.h>
#include <zephyr/arch/common/ffs.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/espi/espi_mchp_mec5.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/espi/mchp-mec5-espi.h>
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
#include <mec_mailbox_api.h>

LOG_MODULE_REGISTER(espi_mailbox, CONFIG_ESPI_LOG_LEVEL);

struct mec5_mbox_devcfg {
	struct mbox_regs *regs;
	const struct device *parent;
	uint32_t host_addr;
	uint8_t host_mem_space;
	uint8_t ldn;
	uint8_t hev_sirq;
	uint8_t hsmi_sirq;
	void (*irq_config_func)(void);
};

struct mec5_mbox_data {
	uint32_t isr_count;
	uint8_t host_to_ec;
#ifdef ESPI_MEC5_MAILBOX_CALLBACK
	mchp_espi_pc_mbox_callback_t cb;
	void *cb_data;
#endif
};

static int mec5_mbox_intr_enable(const struct device *dev, uint8_t enable)
{
	const struct mec5_mbox_devcfg *const devcfg = dev->config;
	struct mbox_regs *const regs = devcfg->regs;

	int ret = mec_mbox_girq_ctrl(regs, enable);

	if (ret != MEC_RET_OK) {
		ret = -EIO;
	}

	return ret;
}

#ifdef ESPI_MEC5_MAILBOX_CALLBACK
static int mec5_mbox_set_callback(const struct device *dev,
				  mchp_espi_pc_mbox_callback_t callback,
				  void *user_data)
{
	struct mec5_mbox_data *data = dev->data;
	unsigned int lock = irq_lock();

	data->cb = callback;
	data->cb_data = user_data;

	irq_unlock(lock);
	return 0;
}
#endif

/* Interrupt to EC generated when Host writes a byte to the Host-to-EC mailbox register
 * at index 0. Host writes data byte to Data register and then 0 to Index register.
 */
static void mec5_mbox_isr(const struct device *dev)
{
	const struct mec5_mbox_devcfg *const devcfg = dev->config;
	struct mbox_regs *const regs = devcfg->regs;
	struct mec5_mbox_data *data = dev->data;

	data->isr_count++;

	mec_mbox_get_host_to_ec(regs, &data->host_to_ec);

	LOG_DBG("ISR: MBOX: h2ec = 0x%02x", data->host_to_ec);

#ifdef ESPI_MEC5_MAILBOX_CALLBACK
	/* TODO */
#else
	struct espi_event evt = {
		.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
		.evt_details = ESPI_PERIPHERAL_HOST_MAILBOX,
		.evt_data = data->host_to_ec,
	};

	espi_mec5_send_callbacks(devcfg->parent, evt);
#endif
}

/* Mailbox peripheral does not implement a logical device activate register.
 * Its runtime registers mapped to Host address space via the eSPI I/O or Memory BAR
 * are held in reset state until VCC_PWRGD is active.
 * Mailbox EC-only registers are reset by RESET_SYS and will retain configuration
 * through VCC_PWRGD and PLTRST# transitions.
 */
static int mec5_mbox_host_access_en(const struct device *dev, uint8_t enable, uint32_t cfg)
{
	const struct mec5_mbox_devcfg *const devcfg = dev->config;
	uint32_t barcfg = devcfg->ldn | BIT(ESPI_MEC5_BAR_CFG_EN_POS);
	uint32_t sirqcfg = devcfg->ldn;
	int ret = 0;

	if (devcfg->host_mem_space) {
		barcfg |= BIT(ESPI_MEC5_BAR_CFG_MEM_BAR_POS);
	}

	ret = espi_mec5_bar_config(devcfg->parent, devcfg->host_addr, barcfg);
	if (ret) {
		return ret;
	}

	sirqcfg = devcfg->ldn | (((uint32_t)devcfg->hev_sirq << ESPI_MEC5_SIRQ_CFG_SLOT_POS)
				 & ESPI_MEC5_SIRQ_CFG_SLOT_MSK);
	ret = espi_mec5_sirq_config(devcfg->parent, sirqcfg);
	if (ret) {
		return ret;
	}

	sirqcfg = devcfg->ldn | (((uint32_t)devcfg->hsmi_sirq << ESPI_MEC5_SIRQ_CFG_SLOT_POS)
				 & ESPI_MEC5_SIRQ_CFG_SLOT_MSK);
	ret = espi_mec5_sirq_config(devcfg->parent, sirqcfg);

	return ret;
}

static struct mchp_espi_pc_mbox_driver_api mec5_mbox_drv_api = {
	.host_access_enable = mec5_mbox_host_access_en,
	.intr_enable = mec5_mbox_intr_enable,
#ifdef ESPI_MEC5_MAILBOX_CALLBACK
	.set_callback = mec5_mbox_set_callback,
#endif
};

static int mec5_mbox_init(const struct device *dev)
{
	const struct mec5_mbox_devcfg *const devcfg = dev->config;
	struct mbox_regs *const regs = devcfg->regs;
	struct mec5_mbox_data *data = dev->data;
	uint32_t smi_ien_msk = 0;
	uint32_t flags = MEC_MBOX_FLAG_INTR_EN;

	data->isr_count = 0;
	data->host_to_ec = 0;

	int ret = mec_mbox_init(regs, smi_ien_msk, flags);

	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	if (devcfg->irq_config_func) {
		devcfg->irq_config_func();
		mec_mbox_girq_ctrl(regs, 1u);
	}

	return 0;
}

#define MEC5_DT_MBOX_NODE(inst) DT_INST(inst, DT_DRV_COMPAT)

#define MEC5_DT_MBOX_HA(inst) \
	DT_PROP_BY_PHANDLE_IDX(MEC5_DT_MBOX_NODE(inst), host_infos, 0, host_address)

#define MEC5_DT_MBOX_HMS(inst) \
	DT_PROP_BY_PHANDLE_IDX_OR(MEC5_DT_MBOX_NODE(inst), host_infos, 0, host_mem_space, 0)

#define MEC5_DT_MBOX_LDN(inst) \
	DT_PROP_BY_PHANDLE_IDX(MEC5_DT_MBOX_NODE(inst), host_infos, 0, ldn)

/* return node indentifier pointed to by index, idx in phandles host_infos */
#define MEC5_DT_MBOX_HI_NODE(inst) DT_PHANDLE_BY_IDX(MEC5_DT_MBOX_NODE(inst), host_infos, 0)

#define MEC5_DT_MBOX_HEV_SIRQ(inst) DT_PROP_BY_IDX(MEC5_DT_MBOX_HI_NODE(inst), sirqs, 0)
#define MEC5_DT_MBOX_SMI_SIRQ(inst) DT_PROP_BY_IDX(MEC5_DT_MBOX_HI_NODE(inst), sirqs, 1)

#define MEC5_MBOX_DEVICE(inst)							\
										\
	static void mec5_mbox_irq_config_func_##inst(void)			\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(inst),					\
			    DT_INST_IRQ(inst, priority),			\
			    mec5_mbox_isr,					\
			    DEVICE_DT_INST_GET(inst), 0);			\
		irq_enable(DT_INST_IRQN(inst));					\
	}									\
										\
	static struct mec5_mbox_data mec5_mbox_data_##inst;			\
										\
	static const struct mec5_mbox_devcfg mec5_mbox_dcfg_##inst = {		\
		.regs = (struct mbox_regs *)DT_INST_REG_ADDR(inst),		\
		.parent = DEVICE_DT_GET(DT_INST_PARENT(inst)),			\
		.host_addr = MEC5_DT_MBOX_HA(inst),				\
		.host_mem_space = MEC5_DT_MBOX_HMS(inst),			\
		.ldn = MEC5_DT_MBOX_LDN(inst),					\
		.hev_sirq = MEC5_DT_MBOX_HEV_SIRQ(inst),			\
		.hsmi_sirq = MEC5_DT_MBOX_SMI_SIRQ(inst),			\
		.irq_config_func = mec5_mbox_irq_config_func_##inst,		\
	};									\
	DEVICE_DT_INST_DEFINE(inst, mec5_mbox_init, NULL,			\
			&mec5_mbox_data_##inst,					\
			&mec5_mbox_dcfg_##inst,					\
			POST_KERNEL, CONFIG_ESPI_INIT_PRIORITY,			\
			&mec5_mbox_drv_api);					\
										\

DT_INST_FOREACH_STATUS_OKAY(MEC5_MBOX_DEVICE)
