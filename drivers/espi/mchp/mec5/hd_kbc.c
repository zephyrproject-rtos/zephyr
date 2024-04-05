/*
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Microchip MEC5 HAL based 8042-KBC (keyboard controller) */

#define DT_DRV_COMPAT microchip_mec5_kbc

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
#include <mec_kbc_api.h>

LOG_MODULE_REGISTER(espi_kbc, CONFIG_ESPI_LOG_LEVEL);

struct mec5_kbc_devcfg {
	struct kbc_regs *regs;
	const struct device *parent;
	uint32_t cfg_flags;
	uint16_t host_addr;
	uint16_t host_addr_p92;
	uint8_t ldn;
	uint8_t ldn_p92;
	uint8_t kirq;
	uint8_t mirq;
	void (*irq_config_func)(void);
};

struct mec5_kbc_data {
	volatile uint32_t isr_count_ibf;
	volatile uint32_t isr_count_obe;
	volatile uint8_t status;
	volatile uint8_t cmd_data;
};

static int mec5_kbc_lpc_request(const struct device *dev,
				enum lpc_peripheral_opcode op,
				uint32_t *data, uint32_t flags)
{
	const struct mec5_kbc_devcfg *const devcfg = dev->config;
	struct kbc_regs *const regs = devcfg->regs;
	int ret = 0;
	uint8_t kbc_hw_sts = 0, msk = 0xffu, pos = 0u;

	/* if opcode requires data check its valid */
	if ((op != E8042_RESUME_IRQ) && (op != E8042_PAUSE_IRQ) && (op != E8042_CLEAR_OBF)) {
		if (!data) {
			return -EINVAL;
		}
	}

	if (!mec_kbc_is_enabled(regs)) {
		return -ENOTSUP;
	}

	switch (op) {
	case E8042_OBF_HAS_CHAR:
	case E8042_IBF_HAS_CHAR:
	case E8042_READ_KB_STS:
		if (op == E8042_OBF_HAS_CHAR) {
			msk = BIT(MEC_KBC_STS_OBF_POS);
			pos = MEC_KBC_STS_OBF_POS;
		} else if (op == E8042_IBF_HAS_CHAR) {
			msk = BIT(MEC_KBC_STS_IBF_POS);
			pos = MEC_KBC_STS_OBF_POS;
		}
		kbc_hw_sts = mec_kbc_status(regs);
		*data = (kbc_hw_sts & msk) >> pos;
		break;
	case E8042_WRITE_KB_CHAR:
		mec_kbc_wr_data(regs, (uint8_t)(*data & 0xffu), MEC_KBC_DATA_KB);
		break;
	case E8042_WRITE_MB_CHAR:
		mec_kbc_wr_data(regs, (uint8_t)(*data & 0xffu), MEC_KBC_DATA_AUX);
		break;
	case E8042_RESUME_IRQ:
		mec_kbc_girq_clr(regs, MEC_KBC_IBF_IRQ);
		mec_kbc_girq_en(regs, MEC_KBC_IBF_IRQ);
		break;
	case E8042_PAUSE_IRQ:
		mec_kbc_girq_dis(regs, MEC_KBC_IBF_IRQ);
		break;
	case E8042_CLEAR_OBF:
		mec_kbc_rd_host_data(regs, MEC_KBC_DATA_HOST);
		break;
	case E8042_SET_FLAG: /* do not modify IBF, OBF, or AUXOBF HW status */
		msk = (uint8_t)(*data & (uint8_t)~(MEC_KBC_STS_OBF | MEC_KBC_STS_IBF
						   | MEC_KBC_STS_AUXOBF));
		mec_kbc_status_set(regs, msk);
		break;
	case E8042_CLEAR_FLAG: /* do not modify IBF, OBF, or AUXOBF HW status */
		msk = (uint8_t)(*data | (MEC_KBC_STS_OBF | MEC_KBC_STS_IBF
					 | MEC_KBC_STS_AUXOBF));
		mec_kbc_status_clear(regs, msk);
		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}

static int mec5_kbc_intr_enable(const struct device *dev, uint8_t enable)
{
	const struct mec5_kbc_devcfg *const devcfg = dev->config;
	struct kbc_regs *const regs = devcfg->regs;

	if (enable) {
		mec_kbc_girq_en(regs, MEC_KBC_IBF_IRQ | MEC_KBC_OBE_IRQ);
	} else {
		mec_kbc_girq_dis(regs, MEC_KBC_IBF_IRQ | MEC_KBC_OBE_IRQ);
	}

	return 0;
}

/* MEC5 8042-KBC can generate two interrupts to the EC: IBF and OBE
 * The input buffer full (IBF) handler behavior is based on Kconfig.
 * if CONFIG_ESPI_PERIPHERAL_KBC_IBF_EVT_DATA
 *   Read KBC hardware received byte to clear IBF interrupt status.
 *   Package received byte and command/data flag in eSPI event structure
 * else
 *   Do not read HW leaving IBF status active
 * endif
 * Invoke callback.
 */
static void mec5_kbc_ibf_isr(struct device *dev)
{
	const struct mec5_kbc_devcfg *const devcfg = dev->config;
	struct kbc_regs *const regs = devcfg->regs;
	struct mec5_kbc_data *data = dev->data;
	struct espi_event evt = {
		.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
		.evt_details = ESPI_PERIPHERAL_8042_KBC,
		.evt_data = ESPI_PERIPHERAL_NODATA,
	};
	uint8_t cmd_data, status;

	data->isr_count_ibf++;

	status = mec_kbc_status(regs);
	cmd_data = mec_kbc_rd_host_data(regs, MEC_KBC_DATA_KB); /* clears IBF status */

	data->status = status;
	data->cmd_data = cmd_data;

#ifdef CONFIG_ESPI_PERIPHERAL_KBC_IBF_EVT_DATA
	struct espi_evt_data_kbc *kbc_evt = (struct espi_evt_data *)&evt.evt_data;

	kbc_evt->type = (status & MEC_KBC_STS_CMD) ? 1 : 0;
	kbc_evt->data = cmd_data;
	kbc_evt->evt = HOST_KBC_EVT_IBF;
#else
	/* evt_data[7:0]=0(data), 1(command). evt_data[15:8]=data/cmd */
	evt.evt_data = cmd_data;
	evt.evt_data <<= 8;
	evt.evt_data |= (status & MEC_KBC_STS_CMD) ? 1 : 0;
#endif
	espi_mec5_send_callbacks(devcfg->parent, evt);
}

/* #ifdef CONFIG_ESPI_PERIPHERAL_KBC_OBE_CBK */
/* The OBE interrupt signal goes active when KBC clears OBF after the Host
 * reads either the data for auxiliary data registers. OBE interrupt will
 * remain active until the EC writes (aux)data. We must disable OBE interrupt
 * when it fires and logic to enable it when the application writes (aux)data.
 */
static void mec5_kbc_obe_isr(struct device *dev)
{
	const struct mec5_kbc_devcfg *const devcfg = dev->config;
	struct kbc_regs *const regs = devcfg->regs;
	struct mec5_kbc_data *data = dev->data;
	struct espi_event evt = {
		.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
		.evt_details = ESPI_PERIPHERAL_8042_KBC,
		.evt_data = ESPI_PERIPHERAL_NODATA,
	};

	data->isr_count_obe++;
	mec_kbc_girq_dis(regs, MEC_KBC_OBE_IRQ);
	mec_kbc_girq_clr(regs, MEC_KBC_OBE_IRQ);

	struct espi_evt_data_kbc *kbc_evt = (struct espi_evt_data_kbc *)&evt.evt_data;

	evt.evt_data = 0;
	kbc_evt->evt = HOST_KBC_EVT_OBE;
	espi_mec5_send_callbacks(devcfg->parent, evt);
}

/* The hd_kbc driver uses two related peripherals, 8042-KBC and Port92h fast KB reset.
 * Each of these peripherals has a logical device activate register reset by RESET_VCC.
 * The eSPI driver will call this API after configuring each peripheral's I/O BAR.
 * RESET_VCC is active if any of the following activate:
 *  RESET_SYS signal active
 *    VTR power rail, nRESET_IN pin, FW triggered chip resets
 *  VCC_PWRGD or VCC_PWRGD2 inactive
 *  PCR PWR_INV bit == 1
 *
 * Enable: eSPI IO BARs and Serial IRQs
 */
static int mec5_kbc_host_access_en(const struct device *dev, uint8_t enable, uint32_t ha_cfg)
{
	const struct mec5_kbc_devcfg *devcfg = dev->config;
	struct kbc_regs *const regs = devcfg->regs;
	uint32_t sirqcfg = devcfg->ldn;
	uint32_t barcfg = devcfg->ldn | BIT(ESPI_MEC5_BAR_CFG_EN_POS);
	int ret = 0;

	ret = espi_mec5_bar_config(devcfg->parent, devcfg->host_addr, barcfg);
	if (ret) {
		return -EIO;
	}

	barcfg = devcfg->ldn_p92 | BIT(ESPI_MEC5_BAR_CFG_EN_POS);
	ret = espi_mec5_bar_config(devcfg->parent, devcfg->host_addr, barcfg);
	if (ret) {
		return -EIO;
	}

	sirqcfg |= (((uint32_t)devcfg->kirq << ESPI_MEC5_SIRQ_CFG_SLOT_POS)
		    & ESPI_MEC5_SIRQ_CFG_SLOT_MSK);
	ret = espi_mec5_sirq_config(devcfg->parent, sirqcfg);
	if (ret) {
		return -EIO;
	}

	sirqcfg |= (((uint32_t)devcfg->mirq << ESPI_MEC5_SIRQ_CFG_SLOT_POS)
		    & ESPI_MEC5_SIRQ_CFG_SLOT_MSK);
	ret = espi_mec5_sirq_config(devcfg->parent, sirqcfg);
	if (ret) {
		return -EIO;
	}

	mec_kbc_activate(regs, enable, MEC_KBC_ACTV_KBC | MEC_KBC_ACTV_P92);

	return 0;
}

static int mec5_kbc_init(const struct device *dev)
{
	const struct mec5_kbc_devcfg *devcfg = dev->config;
	struct kbc_regs *const regs = devcfg->regs;
	struct mec5_kbc_data *data = dev->data;
	uint32_t init_flags = MEC_KBC_PCOBF_EN | MEC_KBC_AUXOBF_EN | MEC_KBC_PORT92_EN;

	data->isr_count_ibf = 0;
	data->isr_count_obe = 0;
	data->status = 0;
	data->cmd_data = 0;

	int ret = mec_kbc_init(regs, init_flags);

	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	if (devcfg->irq_config_func) {
		devcfg->irq_config_func();
		mec_kbc_girq_en(regs, MEC_KBC_IBF_IRQ);
	}

	return 0;
}

static const struct mchp_espi_pc_kbc_driver_api mec5_kbc_driver_api = {
	.host_access_enable = mec5_kbc_host_access_en,
	.intr_enable = mec5_kbc_intr_enable,
	.lpc_request = mec5_kbc_lpc_request,
};

#define MEC5_DT_KBC_NODE(inst) DT_INST(inst, DT_DRV_COMPAT)

#define MEC5_DT_KBC_HA(inst) \
	DT_PROP_BY_PHANDLE_IDX(MEC5_DT_KBC_NODE(inst), host_infos, 0, host_address)
#define MEC5_DT_P92_HA(inst) \
	DT_PROP_BY_PHANDLE_IDX(MEC5_DT_KBC_NODE(inst), host_infos, 1, host_address)

#define MEC5_DT_KBC_LDN(inst) DT_PROP_BY_PHANDLE_IDX(MEC5_DT_KBC_NODE(inst), host_infos, 0, ldn)
#define MEC5_DT_P92_LDN(inst) DT_PROP_BY_PHANDLE_IDX(MEC5_DT_KBC_NODE(inst), host_infos, 1, ldn)

/* return node indentifier pointed to by index, idx in phandles host_infos */
#define MEC5_DT_KBC_HI_NODE(inst) DT_PHANDLE_BY_IDX(MEC5_DT_KBC_NODE(inst), host_infos, 0)

/* KBC can generate two Serial IRQs: keyboard and mouse */
#define MEC5_DT_KBC_KIRQ(inst) DT_PROP_BY_IDX(MEC5_DT_KBC_HI_NODE(inst), sirqs, 0)
#define MEC5_DT_KBC_MIRQ(inst) DT_PROP_BY_IDX(MEC5_DT_KBC_HI_NODE(inst), sirqs, 1)


#define KBC_MEC5_DEVICE(inst)							\
										\
	static struct mec5_kbc_data mec5_kbc_data_##inst;			\
										\
	static void mec5_kbc_irq_config_func_##inst(void)			\
	{									\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, ibf, irq),		\
			    DT_INST_IRQ_BY_NAME(inst, ibf, priority),		\
			    mec5_kbc_ibf_isr,					\
			    DEVICE_DT_INST_GET(inst), 0);			\
		irq_enable(DT_INST_IRQ_BY_NAME(inst, ibf, irq));		\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, obe, irq),		\
			    DT_INST_IRQ_BY_NAME(inst, obe, priority),		\
			    mec5_kbc_obe_isr,					\
			    DEVICE_DT_INST_GET(inst), 0);			\
		irq_enable(DT_INST_IRQ_BY_NAME(inst, obe, irq));		\
	}									\
										\
	static const struct mec5_kbc_devcfg mec5_kbc_dcfg_##inst = {		\
		.regs = (struct kbc_regs *)DT_INST_REG_ADDR(inst),		\
		.parent = DEVICE_DT_GET(DT_INST_PARENT(inst)),			\
		.cfg_flags = 0,							\
		.host_addr = MEC5_DT_KBC_HA(inst),				\
		.host_addr_p92 = MEC5_DT_P92_HA(inst),				\
		.ldn = MEC5_DT_KBC_LDN(inst),					\
		.ldn_p92 = MEC5_DT_P92_LDN(inst),				\
		.kirq = MEC5_DT_KBC_KIRQ(inst),					\
		.mirq = MEC5_DT_KBC_MIRQ(inst),					\
		.irq_config_func = mec5_kbc_irq_config_func_##inst,		\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst, mec5_kbc_init, NULL,			\
			&mec5_kbc_data_##inst,					\
			&mec5_kbc_dcfg_##inst,					\
			POST_KERNEL, CONFIG_ESPI_INIT_PRIORITY,			\
			&mec5_kbc_driver_api);					\

DT_INST_FOREACH_STATUS_OKAY(KBC_MEC5_DEVICE)
