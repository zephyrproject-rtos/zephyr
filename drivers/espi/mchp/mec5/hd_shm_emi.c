/*
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mec5_shm_emi

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
#include <mec_emi_api.h>

LOG_MODULE_REGISTER(espi_shm_emi, CONFIG_ESPI_LOG_LEVEL);

/* Driver sharing EC SRAM with the Host via an EMI device instance.
 * Features:
 * 1. EMI host facing register can be mapped to Host I/O or memory spaces.
 * 2. EMI supports up to two memory windows.
 * 3. Each memory window supports R/W attributes.
 * 4. Each memory region has read and write sizes.
 * 5. Single byte mailbox for Host and EC synchronization with interrupts to Host and EC.
 * 6. 16 generic software interrupt status bits clearable/maskable by the Host and settable
 *    by the EC.
 * 7. Host visiable Application ID register usable as a simple HW mutex if multiple Host
 *    threads access the same EMI instance.
 */

struct mec5_shm_emi_devcfg {
	struct emi_regs *regs;
	const struct device *parent;
	uint32_t cfg_flags;
	uint32_t host_addr;
	uint8_t host_mem_space;
	uint8_t ldn;
	uint8_t sirq_hev;
	uint8_t sirq_e2h;
	void (*irq_config_func)(void);
};

struct mec5_shm_emi_data {
	uint32_t isr_count;
	uint8_t mb_host_to_ec;
	uint8_t mb_ec_to_host;
	struct mchp_emi_mem_region mr[MEC_EMI_MEM_REGION_NUM];
	mchp_espi_pc_emi_callback_t cb;
	void *cb_data;
};

/* TODO - Does this API only affect device interrupts to the EC or
 * interrupts to the Host?
 */
static int mec5_shm_emi_intr_en(const struct device *dev, uint8_t enable)
{
	const struct mec5_shm_emi_devcfg *devcfg = dev->config;
	struct emi_regs *const regs = devcfg->regs;

	int ret = mec_emi_girq_ctrl(regs, enable);

	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	return 0;
}

/* Called by eSPI driver when Host eSPI controller has de-asserted PLTRST# virtual wire.
 * Also, requires the platform has driven VCC_PWRGD active.
 * EMI runtime and EC-only registers are reset by RESET_SYS not RESET_VCC therefore we
 * do not need configuration on PLTRST# or VCC_PWRGD events.
 */
static int mec5_shm_emi_host_access_en(const struct device *dev, uint8_t enable, uint32_t cfg)
{
	const struct mec5_shm_emi_devcfg *devcfg = dev->config;
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

	sirqcfg = devcfg->ldn | (((uint32_t)devcfg->sirq_hev << ESPI_MEC5_SIRQ_CFG_SLOT_POS)
				 & ESPI_MEC5_SIRQ_CFG_SLOT_MSK);
	ret = espi_mec5_sirq_config(devcfg->parent, sirqcfg);
	if (ret) {
		return ret;
	}

	sirqcfg = devcfg->ldn | (((uint32_t)devcfg->sirq_e2h << ESPI_MEC5_SIRQ_CFG_SLOT_POS)
				 & ESPI_MEC5_SIRQ_CFG_SLOT_MSK);
	ret = espi_mec5_sirq_config(devcfg->parent, sirqcfg);

	return ret;
}

/* EMI Memory windows 0 and 1.
 * EC SRAM location of each window must be >= 4-byte aligned because address bits[1:0]
 * are forced to 00b by EMI hardware.
 * The specifies the memory window, 4-byte offset, and access size using a 16-bit value
 * written to the Host facing EC-Address register.
 * b[1:0]=access type:
 *	00b = byte access
 *	01b = 16-bit access
 *	10b = 32-bit access
 *	11b = 32-bit auto-increment. HW increments bits[14:2] by 1 after the access
 * b[14:2] = 4-byte word offset where HW adds bits[1:0]=00b to the offset
 * b[15]=0(memory window 0), 1(memory window 1)
 * The Host computes the offset of the byte, half-word, or dword, masks off b[15,1:0],
 * sets b[1:0] to the access type and b[15] to the memory window, and writes this
 * 16-bit value to the EC-Address register.
 * The Host then accesses the 32-bit EC-Data register using an I/O or memory access
 * size matching the size set in the access type field.
 *
 * Each memory window base address in EC SRAM, a 16-bit read address limit, and a 16-bit
 * write address limit. EMI hardware compares bits[14:2] of EC-Address set by the Host to
 * bits[14:2] of each limit register.
 * Read allowed if EC-Address[14:2] < Read-Limit[14:2]
 * Write allowed if EC-Address[14:2] < Write-Limit[14:2]
 *
 * Example 1: 256 byte memory window. Read access all 256 bytes, write access lower 128 bytes.
 * Memory Window Base = Address of 256 byte buffer aligned >= 4-bytes in EC SRAM
 * Memory Window Read Limit = 0x100. Word offsets 0x00 - 0xfc are < 0x100
 * Memory Window Write Limit = 0x80. Word offsets 0x00 - 0x7c are < 0x80
 *
 * Example 2: 256 byte memory window. Read access all 256, write access upper 128 bytes.
 * Hardware is unable to do this with one memory window. Two windows must be used.
 * Memory Window 0 Base = Address of 256 byte buffer aligned >= 4-bytes in EC SRAM
 * Memory Window 0 Read Limit = 0x80 (read access all 128 bytes)
 * Memory Window 0 Write Limit = 0 (no write access)
 * Memory Window 1 Base = Memory Window 0 Base + 128
 * Memory Window 1 Read Limit = 0x80 (read access all 128 bytes)
 * Memory Window 1 Write Limit = 0x80 (write access all 128 bytes)
 * NOTE: This solution requires Host to know two memory windows are in use and
 * set bit[15] in EC-Address appropriately.
 *
 * ec-mem-window0 = <size_in_bytes read-limit, write-limit>
 */
static int mec5_shm_emi_cfg_mr(const struct device *dev, struct mchp_emi_mem_region *mr,
			       uint8_t region_id)
{
	const struct mec5_shm_emi_devcfg *const devcfg = dev->config;
	struct mec5_shm_emi_data *const data = dev->data;
	struct emi_regs *const regs = devcfg->regs;
	uint32_t rwszs = 0;
	int ret = 0;

	if (!mr || (region_id > MEC_EMI_MEM_REGION_1)) {
		return -EINVAL;
	}

	rwszs = MEC_EMI_MEMR_CFG_SIZES(mr->rdsz, mr->wrsz);

	unsigned int key = irq_lock();

	ret = mec_emi_mem_region_config(regs, region_id, (uint32_t)mr->memptr, rwszs);
	if (ret == MEC_RET_OK) {
		data->mr[region_id].memptr = mr->memptr;
		data->mr[region_id].rdsz = mr->rdsz;
		data->mr[region_id].wrsz = mr->wrsz;
	} else {
		ret = -EIO;
	}

	irq_unlock(key);

	return ret;
}

static int mec5_shm_emi_set_callback(const struct device *dev,
				     mchp_espi_pc_emi_callback_t callback,
				     void *user_data)
{
	struct mec5_shm_emi_data *const data = dev->data;
	unsigned int key = irq_lock();

	data->cb = callback;
	data->cb_data = user_data;

	irq_unlock(key);

	return 0;
}

static int mec5_shm_emi_request(const struct device *dev, enum mchp_emi_opcode op, uint32_t *data)
{
	const struct mec5_shm_emi_devcfg *const devcfg = dev->config;
	struct mec5_shm_emi_data *const edata = dev->data;
	struct emi_regs *const regs = devcfg->regs;
	struct mchp_emi_mem_region *mr = NULL;
	int ret = 0;
	uint32_t rwszs = 0;
	uint8_t region = 0;

	switch (op) {
	case MCHP_EMI_OPC_MBOX_EC_IRQ_DIS:
		mec_emi_girq_ctrl(regs, 0);
		break;
	case MCHP_EMI_OPC_MBOX_EC_IRQ_EN:
		mec_emi_girq_ctrl(regs, 1u);
		break;
	case MCHP_EMI_OPC_MR_DIS:
	case MCHP_EMI_OPC_MR_EN:
		if (!data) {
			ret = -EINVAL;
			break;
		}
		region = (uint8_t)(*data & 0xffu);
		mr = &edata->mr[region];
		if (op == MCHP_EMI_OPC_MR_EN) {
			rwszs = MEC_EMI_MEMR_CFG_SIZES(mr->rdsz, mr->wrsz);
		}
		ret = mec_emi_mem_region_config(regs, region, (uint32_t)mr->memptr, rwszs);
		if (ret) {
			ret = -EIO;
		}
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

/* EMI generates an interrupt to the EC when the Host writes the Host-to-EC mailbox
 * register.
 * Future - Invoke callback to application to handle the emi mailbox "command".
 */
static void mec5_shm_emi_isr(const struct device *dev)
{
	const struct mec5_shm_emi_devcfg *const devcfg = dev->config;
	struct mec5_shm_emi_data *const data = dev->data;
	struct emi_regs *const regs = devcfg->regs;
	uint32_t mbval = 0;

	data->isr_count++;

	mbval = mec_emi_mbox_rd(regs, MEC_EMI_HOST_TO_EC_MBOX);
	data->mb_host_to_ec = mbval;
	mec_emi_girq_clr(regs);

	LOG_DBG("ISR: EMI: H2EC = 0x%02x", mbval);

	if (data->cb) {
		data->cb(dev, mbval, data->cb_data);
	}
}

static const struct mchp_espi_pc_emi_driver_api mec5_shm_emi_driver_api = {
	.host_access_enable = mec5_shm_emi_host_access_en,
	.intr_enable = mec5_shm_emi_intr_en,
	.configure_mem_region = mec5_shm_emi_cfg_mr,
	.set_callback = mec5_shm_emi_set_callback,
	.request = mec5_shm_emi_request,
};

static int mec5_shm_emi_init(const struct device *dev)
{
	const struct mec5_shm_emi_devcfg *const devcfg = dev->config;
	struct emi_regs *const regs = devcfg->regs;
	int ret = mec_emi_init(regs, 0);

	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	if (devcfg->irq_config_func) {
		devcfg->irq_config_func();
		mec_emi_girq_ctrl(regs, 1);
	}

	return 0;
}

#define MEC5_DT_EMI_NODE(inst) DT_INST(inst, DT_DRV_COMPAT)

#define MEC5_DT_EMI_HA(inst) \
	DT_PROP_BY_PHANDLE_IDX(MEC5_DT_EMI_NODE(inst), host_infos, 0, host_address)

#define MEC5_DT_EMI_HMS(inst) \
	DT_PROP_BY_PHANDLE_IDX_OR(MEC5_DT_EMI_NODE(inst), host_infos, 0, host_mem_space, 0)

#define MEC5_DT_EMI_LDN(inst) DT_PROP_BY_PHANDLE_IDX(MEC5_DT_EMI_NODE(inst), host_infos, 0, ldn)

/* return node indentifier pointed to by index, idx in phandles host_infos */
#define MEC5_DT_EMI_HI_NODE(inst) DT_PHANDLE_BY_IDX(MEC5_DT_EMI_NODE(inst), host_infos, 0)

#define MEC5_DT_EMI_HEV_SIRQ(inst) DT_PROP_BY_IDX(MEC5_DT_EMI_HI_NODE(inst), sirqs, 0)
#define MEC5_DT_EMI_E2H_SIRQ(inst) DT_PROP_BY_IDX(MEC5_DT_EMI_HI_NODE(inst), sirqs, 1)

#define EMI_SHM_MEC5_DEVICE(inst)							\
	static struct mec5_shm_emi_data mec5_shm_emi_dev_data_##inst;			\
											\
	static void mec5_shm_emi_irq_cfg_##inst(void)					\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(inst),						\
			    DT_INST_IRQ(inst, priority),				\
			    mec5_shm_emi_isr,						\
			    DEVICE_DT_INST_GET(inst), 0);				\
		irq_enable(DT_INST_IRQN(inst));						\
	}										\
											\
	static const struct mec5_shm_emi_devcfg mec5_shm_emi_dcfg##inst = {		\
		.regs = (struct emi_regs *)DT_INST_REG_ADDR(inst),			\
		.parent = DEVICE_DT_GET(DT_INST_PARENT(inst)),				\
		.host_addr = (uint32_t)MEC5_DT_EMI_HA(inst),				\
		.host_mem_space = (uint8_t)MEC5_DT_EMI_HMS(inst),			\
		.ldn = MEC5_DT_EMI_LDN(inst),						\
		.sirq_hev = MEC5_DT_EMI_HEV_SIRQ(inst),					\
		.sirq_e2h = MEC5_DT_EMI_E2H_SIRQ(inst),					\
		.irq_config_func = mec5_shm_emi_irq_cfg_##inst,				\
	};										\
											\
	DEVICE_DT_INST_DEFINE(inst, mec5_shm_emi_init, NULL,				\
			&mec5_shm_emi_dev_data_##inst,					\
			&mec5_shm_emi_dcfg##inst,					\
			POST_KERNEL, CONFIG_ESPI_INIT_PRIORITY,				\
			&mec5_shm_emi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(EMI_SHM_MEC5_DEVICE)
