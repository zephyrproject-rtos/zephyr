/*
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mec5_acpi_ec_os

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
#include <mec_acpi_ec_api.h>

LOG_MODULE_REGISTER(espi_acpi_ec_os, CONFIG_ESPI_LOG_LEVEL);

struct mec5_aec_os_devcfg {
	struct acpi_ec_regs *regs;
	const struct device *parent;
	uint32_t host_addr;
	uint8_t host_mem_space;
	uint8_t ldn;
	uint8_t sirq_obf;
	uint8_t cfg_flags;
	void (*irq_config_func)(void);
};

struct mec5_aec_os_data {
	uint32_t isr_count;
	uint8_t hwstatus;
	uint8_t oscmd;
	uint8_t rsvd[2];
	uint32_t osdata;
};

/* Implement commands from espi.h
 * Read opcodes
 * EACPI_OBF_HAS_CHAR
 * EACPI_IBF_HAS_CHAR
 * EACPI_READ_STS
 * Write opcodes
 * EACPI_WRITE_CHAR
 * EACPI_WRITE_STS
 *
 * NOTE: We do not implement EACPI_GET_SHARED_MEMORY
 *   (requires CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION=y)
 * New MEC5 eSPI driver architecture requires application
 * to allocate ACPI buffer. If application wishes to expose
 * this buffer to the Host via EMI it calls the respective
 * EMI driver API to configure EMI access.
 *
 */
static int mec5_aec_os_lpc_request(const struct device *dev,
				   enum lpc_peripheral_opcode op,
				   uint32_t *data, uint32_t flags)
{
	const struct mec5_aec_os_devcfg *cfg = dev->config;
	struct acpi_ec_regs *regs = cfg->regs;
	int ret = 0;

	if (!mec_acpi_ec_is_enabled(regs)) {
		return -ENOTSUP;
	}

	if (!data) {
		return -EINVAL;
	}

	switch (op) {
	case EACPI_OBF_HAS_CHAR:
		*data = mec_acpi_ec_status_obf(regs);
		break;
	case EACPI_IBF_HAS_CHAR:
		*data = mec_acpi_ec_status_ibf(regs);
		break;
	case EACPI_READ_STS:
		*data = mec_acpi_ec_status(regs);
		break;
	case EACPI_WRITE_CHAR:
		mec_acpi_ec_e2h_data_wr8(regs, 0, (uint8_t)*data);
		break;
	case EACPI_WRITE_STS:
		mec_acpi_ec_status_set(regs, (uint8_t)*data);
		break;
#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_SHM_REGION
	case EACPI_GET_SHARED_MEMORY:
		*data = espi_mec5_shm_addr_get(cfg->parent, EACPI_GET_SHARED_MEMORY);
		break;
#endif
	default:
		ret = -EINVAL;
	}

	return ret;
}

/* Host write to command or data registers sets IBF and generates an
 * interrupt to the EC. EC reading data clears IBF.
 * Is it true an EC read of cmd or data clears IBF status?
 * Data sheet indicates EC copy of status register all bits are R/W.
 * command and data processing!
 * ACPI specification v6.5
 * 0x80 = Read byte from buffer
 *        OS writes command register followed by write of address byte to data register
 * 0x81 = write byte to buffer
 *        OS writes command register
 *        OS writes address data register
 *        OS writes data to data register
 * 0x82 = Burst mode enable
 * 0x83 = Burst mode disable
 * 0x84 = query
 * !!!!!!!!!! WARNING !!!!!!!!!!
 * If built with CONFIG_ESPI_PERIPHERAL_ACPI_EC_IBF_EVT_DATA not set
 * the ISR does NOT clear IBF. The application callback must then
 * know how to clear ACPI EC HW status using the MEC5 HAL. This means
 * the application must know which ACPI_EC instance is used!
 * We recommand always enabling CONFIG_ESPI_PERIPHERAL_ACPI_EC_IBF_EVT_DATA=y
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */
static void mec5_aec_os_ibf_isr(const struct device *dev)
{
	const struct mec5_aec_os_devcfg *cfg = dev->config;
	struct mec5_aec_os_data *data = dev->data;
	struct acpi_ec_regs *regs = cfg->regs;
	struct espi_event evt = {
		.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
		.evt_details = ESPI_PERIPHERAL_HOST_IO,
		.evt_data = ESPI_PERIPHERAL_NODATA,
	};
#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_EC_IBF_EVT_DATA
	struct espi_evt_data_acpi *acpi_evt =
			(struct espi_evt_data_acpi *)&evt.evt_data;
	uint32_t cmd_data = 0;
#endif
	uint8_t status = mec_acpi_ec_status(regs);

	data->isr_count++;

	LOG_DBG("ISR: IBF ACPI_EC at 0x%0x status = 0x%0x", (uint32_t)regs, status);

#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_EC_IBF_EVT_DATA
	data->hwstatus = status;
	if (status & MEC_ACPI_EC_STS_IBF) {
#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_EC_IBF_HANDSHAKE_UD0
		mec_acpi_ec_status_set(regs, MEC_ACPI_EC_STS_UD0A);
#endif
#ifdef CONFIG_ESPI_PERIPHERAL_ACPI_EC_IBF_HANDSHAKE_UD1
		mec_acpi_ec_status_set(regs, MEC_ACPI_EC_STS_UD1A);
#endif
		/* 32-bit read of data register insures IBF is cleared
		 * for 1-byte or 4-byte mode.
		 */
		cmd_data = mec_acpi_ec_host_to_ec_data_rd32(regs);
		acpi_evt->data = (uint8_t)cmd_data;
		mec_acpi_ec_girq_clr(regs, MEC_ACPI_EC_IBF_IRQ);

		LOG_DBG("ISR: ACPI_EC at 0x%0x cmd_data = 0x%0x", (uint32_t)regs, cmd_data);
		if (status & MEC_ACPI_EC_STS_CMD) {
			data->oscmd = (uint8_t)(cmd_data & 0xffu);
			acpi_evt->type = 0;
		} else { /* Host wrote to data */
			data->osdata = cmd_data;
			acpi_evt->type = 1u;
			/* TODO: Is this safe? Where is the documentation struct espi_evt_data_acpi
			 * type field?
			 */
			if (mec_acpi_ec_is_4byte_mode(regs)) {
				acpi_evt->type |= BIT(7);
			}
		}
	}
#endif
	espi_mec5_send_callbacks(cfg->parent, evt);
}

/* Called by eSPI parent driver when platform reset de-asserts.
 * ACPI_EC peripheral registers reset by "RESET_SYS"
 * RESET_SYS is active if any of the following activate:
 *  RESET_VTR: VTR power rail up/down
 *  nRESET_IN pin asserted
 *  Watch Dog Timer reset generated
 *  PCR System Reset register Soft-Sys reset set by firmware
 *  Cortex-M4 SYSRESETREQ signal active
 * ACPI_EC configuration in driver init should be stable across eSPI PLTRST#
 * and VCC_RESET.
 * eSPI BARs and SerialIRQ are reset by eSPI PLTRST# active. We must reprogram
 * these eSPI registers for this device.
 */
static int mec5_aec_os_host_access_en(const struct device *dev, uint8_t enable, uint32_t cfg)
{
	const struct mec5_aec_os_devcfg *devcfg = dev->config;
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

	sirqcfg |= (((uint32_t)devcfg->sirq_obf << ESPI_MEC5_SIRQ_CFG_SLOT_POS)
		    & ESPI_MEC5_SIRQ_CFG_SLOT_MSK);
	ret = espi_mec5_sirq_config(devcfg->parent, sirqcfg);

	return ret;
}

/* TODO - What about OBE when Host reads data from EC-to-Host register */
static int mec5_aec_os_intr_enable(const struct device *dev, uint8_t enable)
{
	const struct mec5_aec_os_devcfg *cfg = dev->config;
	struct acpi_ec_regs *regs = cfg->regs;
	int ret = 0;

	if (enable) {
		ret = mec_acpi_ec_girq_en(regs, MEC_ACPI_EC_IBF_IRQ);
	} else {
		ret = mec_acpi_ec_girq_dis(regs, MEC_ACPI_EC_IBF_IRQ);
	}

	if (ret) {
		ret = -EIO;
	}

	return ret;
}

/* API
 * First API must be host_access_enable
 * configure - Possibly not needed.
 * lpc_request - eSPI parent driver calls this passing EACPI opcodes only
 * for the ACPI_EC instance obtained via DT chosen espi,os-acpi
 */
static const struct mchp_espi_pc_aec_driver_api mec5_aec_os_driver_api = {
	.host_access_enable = mec5_aec_os_host_access_en,
	.intr_enable = mec5_aec_os_intr_enable,
	.lpc_request = mec5_aec_os_lpc_request,
};

/* Do we want to support the OBE EC interrupt genered when
 * the Host reads the EC-to-Data register? All we could do is
 * disable the interrupt when it fires and invoke a callback.
 */
static int mec5_aec_os_init(const struct device *dev)
{
	const struct mec5_aec_os_devcfg *cfg = dev->config;
	struct mec5_aec_os_data *data = dev->data;
	struct acpi_ec_regs *regs = cfg->regs;
	uint32_t flags = cfg->cfg_flags | MEC_ACPI_EC_RESET;

	data->isr_count = 0;
	data->hwstatus = 0;
	data->osdata = 0;

	int ret = mec_acpi_ec_init(regs, flags);

	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	if (cfg->irq_config_func) {
		cfg->irq_config_func();
		mec_acpi_ec_girq_en(regs, MEC_ACPI_EC_IBF_IRQ);
	}

	return 0;
}

#define MEC5_DT_AEC_OS_NODE(inst) DT_INST(inst, DT_DRV_COMPAT)

#define MEC5_DT_AEC_OS_HA(inst) \
	DT_PROP_BY_PHANDLE_IDX(MEC5_DT_AEC_OS_NODE(inst), host_infos, 0, host_address)

#define MEC5_DT_AEC_OS_HMS(inst) \
	DT_PROP_BY_PHANDLE_IDX_OR(MEC5_DT_AEC_OS_NODE(inst), host_infos, 0, host_mem_space, 0)

#define MEC5_DT_AEC_OS_LDN(inst) \
	DT_PROP_BY_PHANDLE_IDX(MEC5_DT_AEC_OS_NODE(inst), host_infos, 0, ldn)

/* return node indentifier pointed to by index, idx in phandles host_infos */
#define MEC5_DT_AEC_OS_HI_NODE(inst) \
	DT_PHANDLE_BY_IDX(MEC5_DT_AEC_OS_NODE(inst), host_infos, 0)

#define MEC5_DT_AEC_OS_OBF_SIRQ(inst) \
	DT_PROP_BY_IDX(MEC5_DT_AEC_OS_HI_NODE(inst), sirqs, 0)

#define DT_MEC5_AEC_CFG_FLAGS(inst) \
	(MEC_ACPI_EC_4BYTE_MODE * DT_INST_PROP(inst, four_byte_data_mode))

#define MEC5_AEC_OS_DEVICE(inst)						\
										\
	static void mec5_aec_os_irq_cfg_func_##inst(void);			\
										\
	static struct mec5_aec_os_data mec5_aec_os_data_##inst;			\
										\
	static const struct mec5_aec_os_devcfg mec5_aec_os_dcfg_##inst = {	\
		.regs = (struct acpi_ec_regs *)DT_INST_REG_ADDR(inst),		\
		.parent = DEVICE_DT_GET(DT_INST_PARENT(inst)),			\
		.host_addr = MEC5_DT_AEC_OS_HA(inst),				\
		.host_mem_space = MEC5_DT_AEC_OS_HMS(inst),			\
		.ldn = MEC5_DT_AEC_OS_LDN(inst),				\
		.sirq_obf = MEC5_DT_AEC_OS_OBF_SIRQ(inst),			\
		.cfg_flags = DT_MEC5_AEC_CFG_FLAGS(inst),			\
		.irq_config_func = mec5_aec_os_irq_cfg_func_##inst,		\
	};									\
	DEVICE_DT_INST_DEFINE(inst, mec5_aec_os_init, NULL,			\
			&mec5_aec_os_data_##inst,				\
			&mec5_aec_os_dcfg_##inst,				\
			POST_KERNEL, CONFIG_ESPI_INIT_PRIORITY,			\
			&mec5_aec_os_driver_api);				\
										\
	static void mec5_aec_os_irq_cfg_func_##inst(void)			\
	{									\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, ibf, irq),		\
			    DT_INST_IRQ_BY_NAME(inst, ibf, priority),		\
			    mec5_aec_os_ibf_isr,				\
			    DEVICE_DT_INST_GET(inst), 0);			\
		irq_enable(DT_INST_IRQ_BY_NAME(inst, ibf, irq));		\
	}

DT_INST_FOREACH_STATUS_OKAY(MEC5_AEC_OS_DEVICE)
