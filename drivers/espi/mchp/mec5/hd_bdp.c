/*
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* BIOS Debug Port capture I/O 0x80 and alias
 * BDP can capture a one, two, and four byte I/O write cycles to a configurable
 * x86 I/O address range plus a one byte I/O write capture of an alias I/O
 * address.
 */
#define DT_DRV_COMPAT microchip_mec5_bdp

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
#include <mec_bdp_api.h>
#include <mec_espi_api.h>
#include <mec_pcr_api.h>

LOG_MODULE_REGISTER(espi_bdp, CONFIG_ESPI_LOG_LEVEL);

#define MEC5_BDP_FIFO_INTR_THRH_LVL CONFIG_ESPI_MEC5_BDP_FIFO_THRESHOLD_LEVEL

struct mec5_bdp_devcfg {
	struct bdp_regs *regs;
	const struct device *parent;
	uint16_t host_io_base;
	uint16_t host_io_alias;
	uint8_t alias_byte_lane;
	uint8_t ldn;
	uint8_t ldn_alias;
	void (*irq_config_func)(void);
};

struct mec5_bdp_data {
	uint32_t isr_count;
	uint32_t hwstatus;
	struct espi_ld_host_addr ha;
#ifdef CONFIG_ESPI_MEC5_BDP_CALLBACK
	mchp_espi_pc_bdp_callback_t cb;
	void *cb_data;
	struct host_io_data cb_hiod;
#endif
	uint16_t capdata[MEC5_BDP_FIFO_INTR_THRH_LVL];
};

#ifdef CONFIG_ESPI_MEC5_BDP_CALLBACK
static int mec5_bdp_set_callback(const struct device *dev,
				 mchp_espi_pc_bdp_callback_t callback,
				 void *user_data)
{
	struct mec5_bdp_data *const data = dev->data;
	unsigned int key = irq_lock();

	data->cb = callback;
	data->cb_data = user_data;

	irq_unlock(key);

	return 0;
}
#endif

static int mec5_bdp_intr_enable(const struct device *dev, int intr_en)
{
	const struct mec5_bdp_devcfg *const devcfg = dev->config;
	struct bdp_regs *const regs = devcfg->regs;
	uint8_t ien = (intr_en != 0) ? 1 : 0;

	mec_bdp_intr_en(regs, ien);

	return 0;
}

static int mec5_bdp_has_data(const struct device *dev)
{
	const struct mec5_bdp_devcfg *const devcfg = dev->config;
	struct bdp_regs *const regs = devcfg->regs;

	if (mec_bdp_fifo_not_empty(regs)) {
		return 1;
	}

	return 0;
}

static int mec5_bdp_get_data(const struct device *dev, struct host_io_data *data)
{
	const struct mec5_bdp_devcfg *const devcfg = dev->config;
	struct bdp_regs *const regs = devcfg->regs;
	struct mec_bdp_io capio = {0};

	int ret = mec_bdp_get_host_io(regs, &capio);

	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	data->data = capio.data;
	data->start_byte_lane = (capio.flags & MEC_BDP_IO_LANE_MSK) >> MEC_BDP_IO_LANE_POS;
	data->size = (capio.flags & MEC_BDP_IO_SIZE_MSK) >> MEC_BDP_IO_SIZE_POS;

	return 0;
}

/*
 * No Overrun:
 *   Read 16-byte entries from FIFO and process each one (invoke callback)
 * Overrun:
 *   Read 16-byte entries from FIFO and process each one (invoke callback)
 *   Does callback have a way to pass overrun status indicating data was lost?
 *
 * Each 16-byte entry indicates the I/O cycles size and byte lane.
 * If the size encoding is 0x3 (invalid) then we should discard the data.
 * Also can we get into a situation where the Host is writing I/O so fast
 * we can't empty the FIFO fast enough? We do not want to get stuck in this ISR.
 * The FIFO has 32 entries. We should put number of FIFO entries in DT.
 */
static void mec5_bdp_isr(const struct device *dev)
{
	const struct mec5_bdp_devcfg *const devcfg = dev->config;
	struct bdp_regs *const regs = devcfg->regs;
	struct mec5_bdp_data *data = dev->data;
	int ret = 0;
	struct mec_bdp_io mio = {0};
#ifdef CONFIG_ESPI_MEC5_BDP_CALLBACK
	struct host_io_data *hiod = &data->cb_hiod;
#else
	struct espi_event evt = {
		.evt_type = ESPI_BUS_PERIPHERAL_NOTIFICATION,
		.evt_details = ESPI_PERIPHERAL_DEBUG_PORT80,
		.evt_data = ESPI_PERIPHERAL_NODATA,
	};
#endif
	data->isr_count++;

	LOG_DBG("ISR: BDP: cnt=%u", data->isr_count);

	for (uint8_t n = 0; n < MEC_BDP_FIFO_MAX_ENTRIES; n++) {
		ret = mec_bdp_get_host_io(regs, &mio);
		if (ret != MEC_RET_OK) { /* disable interrupt and exit */
			mec_bdp_intr_en(regs, 0);
			return;
		}

		if (!mio.flags) { /* No more data */
			break;
		}

#ifdef CONFIG_ESPI_MEC5_BDP_CALLBACK
		if (data->cb) {
			hiod->data = mio.data;
			hiod->start_byte_lane =
				(mio.flags & MEC_BDP_IO_LANE_MSK) >> MEC_BDP_IO_LANE_POS;
			hiod->size = (mio.flags & MEC_BDP_IO_SIZE_MSK) >> MEC_BDP_IO_SIZE_POS;
			data->cb(dev, hiod, data->cb_data);
		}
#else
		evt.evt_details |= ((uint32_t)mio.flags << 16);
		evt.evt_data = mio.data;
		espi_mec5_send_callbacks(devcfg->parent, evt);
#endif
	}

	mec_bdp_girq_status_clr(regs);
}

/* Called by parent eSPI driver when platform reset has de-asserted.
 * We are required to program the BDP eSPI I/O BAR's and set the
 * BDP activate bits.
 */
static int mec5_bdp_host_access_en(const struct device *dev, uint8_t enable, uint32_t ha_cfg)
{
	const struct mec5_bdp_devcfg *const devcfg = dev->config;
	struct bdp_regs *const regs = devcfg->regs;
	uint32_t barcfg = devcfg->ldn | BIT(ESPI_MEC5_BAR_CFG_EN_POS);
	int ret = 0;

	ret = espi_mec5_bar_config(devcfg->parent, devcfg->host_io_base, barcfg);
	if (ret != MEC_RET_OK) {
		LOG_ERR("MEC5 BDP IO BAR: (%d)", ret);
		return -EIO;
	}

	mec_bdp_activate(regs, enable, 0);

	if (devcfg->host_io_alias) {
		barcfg = devcfg->ldn_alias | BIT(ESPI_MEC5_BAR_CFG_EN_POS);
		ret = espi_mec5_bar_config(devcfg->parent, devcfg->host_io_alias, barcfg);
		if (ret != MEC_RET_OK) {
			LOG_ERR("MEC5 BDPA IO BAR: (%d)", ret);
			return -EIO;
		}
		mec_bdp_activate(regs, enable, 1);
	}

	return 0;
}

static const struct mchp_espi_pc_bdp_driver_api mec5_bdp_drv_api = {
	.host_access_enable = mec5_bdp_host_access_en,
	.intr_enable = mec5_bdp_intr_enable,
	.has_data = mec5_bdp_has_data,
	.get_data = mec5_bdp_get_data,
#ifdef CONFIG_ESPI_MEC5_BDP_CALLBACK
	.set_callback = mec5_bdp_set_callback,
#endif
};

/* Called by Zephyr kernel during driver initialization */
static int mec5_bdp_init(const struct device *dev)
{
	const struct mec5_bdp_devcfg *const devcfg = dev->config;
	struct bdp_regs *const regs = devcfg->regs;
	struct mec5_bdp_data *data = dev->data;
	uint32_t cfg_flags = MEC5_BDP_FIFO_INTR_THRH_LVL | BIT(MEC5_BDP_CFG_THRH_IEN_POS);

	data->isr_count = 0;

	if (devcfg->host_io_alias) {
		cfg_flags |= BIT(MEC5_BDP_CFG_ALIAS_EN_POS);
	}

	int ret = mec_bdp_init(regs, cfg_flags);

	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	if (devcfg->irq_config_func) {
		devcfg->irq_config_func();
		mec_bdp_girq_ctrl(regs, 1u);
	}

	return 0;
}

#define MEC5_DT_NODE(inst) DT_INST(inst, DT_DRV_COMPAT)

#define MEC5_DT_BDP_HA(inst) \
	DT_PROP_BY_PHANDLE_IDX(MEC5_DT_NODE(inst), host_infos, 0, host_address)
#define MEC5_DT_BDPA_HA(inst) \
	DT_PROP_BY_PHANDLE_IDX(MEC5_DT_NODE(inst), host_infos, 1, host_address)

#define MEC5_DT_BDP_LDN(inst) DT_PROP_BY_PHANDLE_IDX(MEC5_DT_NODE(inst), host_infos, 0, ldn)
#define MEC5_DT_BDPA_LDN(inst) DT_PROP_BY_PHANDLE_IDX(MEC5_DT_NODE(inst), host_infos, 1, ldn)

#define MEC5_DT_BDPA_ABL(inst) \
	DT_PROP_BY_PHANDLE_IDX(MEC5_DT_NODE(inst), host_infos, 1, bdp_host_alias_byte_lane)

#define MEC5_BDP_DEVICE(inst)							\
										\
	static void mec5_bdp_irq_config_func_##inst(void)			\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(inst),					\
			    DT_INST_IRQ(inst, priority),			\
			    mec5_bdp_isr,					\
			    DEVICE_DT_INST_GET(inst), 0);			\
		irq_enable(DT_INST_IRQN(inst));					\
	}									\
										\
	static struct mec5_bdp_data mec5_bdp_data_##inst;			\
										\
	static const struct mec5_bdp_devcfg mec5_bdp_dcfg_##inst = {		\
		.regs = (struct bdp_regs *)DT_INST_REG_ADDR(inst),		\
		.parent = DEVICE_DT_GET(DT_INST_PARENT(inst)),			\
		.host_io_base = MEC5_DT_BDP_HA(inst),				\
		.host_io_alias = MEC5_DT_BDPA_HA(inst),				\
		.alias_byte_lane = MEC5_DT_BDPA_ABL(inst),			\
		.ldn = MEC5_DT_BDP_LDN(inst),					\
		.ldn_alias = MEC5_DT_BDPA_LDN(inst),				\
		.irq_config_func = mec5_bdp_irq_config_func_##inst,		\
	};									\
	DEVICE_DT_INST_DEFINE(inst, mec5_bdp_init, NULL,			\
			&mec5_bdp_data_##inst,					\
			&mec5_bdp_dcfg_##inst,					\
			POST_KERNEL, CONFIG_ESPI_INIT_PRIORITY,			\
			&mec5_bdp_drv_api);					\
										\

DT_INST_FOREACH_STATUS_OKAY(MEC5_BDP_DEVICE)
