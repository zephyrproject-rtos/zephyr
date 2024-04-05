/*
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
#include "espi_utils.h"
#include "espi_mchp_mec5_private.h"

/* MEC5 HAL */
#include <device_mec5.h>
#include <mec_retval.h>
#include <mec_pcr_api.h>
#include <mec_espi_api.h>
#include <sys/errno.h>

LOG_MODULE_REGISTER(espi_fc, CONFIG_ESPI_LOG_LEVEL);

/* ---- Flash channel API invoked from core eSPI driver ---- */

static int mec5_espi_fc_op(const struct device *dev, struct espi_flash_packet *pckt,
			   uint8_t op, uint8_t tag)
{
	const struct espi_mec5_dev_config *devcfg = dev->config;
	struct espi_io_regs *ioregs = devcfg->iob;
	struct espi_mec5_data *data = dev->data;
	struct espi_mec5_fc_data *fcd = &data->fc_data;
	uint32_t flags = BIT(MEC_ESPI_FC_XFR_FLAG_START_IEN_POS);
	struct mec_espi_fc_xfr fcx = {  0 };
	int ret = 0;

	if (!mec_espi_fc_is_ready(ioregs)) {
		LOG_ERR("FC not ready");
		return -EIO;
	}

	if (!mec_espi_fc_is_busy(ioregs)) {
		LOG_ERR("FC is busy");
		return -EIO;
	}

	if (!pckt) {
		return -EINVAL;
	}

	if (!IS_ALIGNED(pckt->buf, 4)) {
		LOG_ERR("Invalid buffer alignment");
		return -EINVAL;
	}

	if (pckt->len > CONFIG_ESPI_FLASH_BUFFER_SIZE) {
		LOG_ERR("Invalid size request");
		return -EINVAL;
	}

	/* configure and start FC read or write */
	fcx.buf_addr = (uint32_t)pckt->buf;
	/* note: length ignored by HAL for erase operations */
	fcx.byte_len = pckt->len;
	fcx.flash_addr = pckt->flash_addr;
	fcx.operation = op;
	fcx.tag = tag;

	ret = mec_espi_fc_xfr_start(ioregs, &fcx, flags);
	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	ret = k_sem_take(&fcd->flash_lock, K_MSEC(MEC5_MAX_FC_TIMEOUT_MS));
	if (ret == -EAGAIN) {
		LOG_ERR("%s timeout", __func__);
		return -ETIMEDOUT;
	}

	if (mec_espi_fc_is_error(fcd->fc_status)) {
		mec_espi_fc_status_clr(ioregs, UINT32_MAX);
		LOG_ERR("%s error %x", __func__, fcd->fc_status);
		return -EIO;
	}

	return 0;
}

int mec5_espi_fc_read(const struct device *dev, struct espi_flash_packet *pckt)
{

	return mec5_espi_fc_op(dev, pckt, MEC_ESPI_FC_OP_READ, 0);
}

int mec5_espi_fc_write(const struct device *dev, struct espi_flash_packet *pckt)
{
	return mec5_espi_fc_op(dev, pckt, MEC_ESPI_FC_OP_WRITE, 0);
}

int mec5_espi_fc_erase(const struct device *dev, struct espi_flash_packet *pckt)
{
	/* Let eSPI Host controller choose the smallest supported erase size */
	return mec5_espi_fc_op(dev, pckt, MEC_ESPI_FC_OP_ERASE_S, 0);
}

/* ---- Flash Channel interrupt handler ---- */

/* Called by ISR when eSPI Host Controller sends eSPI flash channel enable = 1 */
static void mec5_espi_fc_init(const struct device *dev)
{
	const struct espi_mec5_dev_config *devcfg = dev->config;
	struct espi_io_regs *ioregs = devcfg->iob;
	struct espi_mec5_data *data = dev->data;
	struct espi_mec5_fc_data *fcd = &data->fc_data;
	uint32_t msk = BIT(MEC_ESPI_FC_INTR_DONE_POS) | BIT(MEC_ESPI_FC_INTR_CHEN_CHG_POS);

	k_sem_init(&fcd->flash_lock, 0, 1);
	mec_espi_fc_intr_ctrl(ioregs, msk, 1u);
}

/* Flash channel ISR:
 * Channel enable change by Host
 * Flash read/write/erase requests by Target to Host
 */
static void mec5_espi_fc_isr(const struct device *dev)
{
	const struct espi_mec5_dev_config *devcfg = dev->config;
	struct espi_io_regs *ioregs = devcfg->iob;
	struct espi_mec5_data *data = dev->data;
	struct espi_mec5_fc_data *fcd = &data->fc_data;
	struct espi_event evt = { .evt_type = ESPI_BUS_EVENT_CHANNEL_READY,
				  .evt_details = ESPI_CHANNEL_FLASH,
				  .evt_data = 0,
				};
	uint32_t status = mec_espi_fc_status(ioregs);

	LOG_DBG("ISR FC: status = 0x%0x", status);

	fcd->fc_status = status;
	mec_espi_fc_status_clr(ioregs, status);

	if (status & BIT(MEC_ESPI_FC_INTR_DONE_POS)) {
		k_sem_give(&fcd->flash_lock);
	}

	/* Flash Channel Enable changed by Host or reset */
	if (status & BIT(MEC_ESPI_FC_INTR_CHEN_CHG_POS)) {
		if (status & BIT(MEC_ESPI_FC_INTR_CHEN_POS)) { /* Host enabled flash channel? */
			/* perform any initialization before indicating ready */
			mec5_espi_fc_init(dev);
			mec_espi_fc_ready_set(ioregs);
			evt.evt_data = 1;
		}
		espi_send_callbacks(&data->callbacks, dev, evt);
	}
}

#define MEC5_ESPI_NODE DT_NODELABEL(espi0)

void mec5_espi_fc_irq_connect(const struct device *espi_dev)
{
	IRQ_CONNECT(DT_IRQ_BY_NAME(MEC5_ESPI_NODE, fc, irq),
		DT_IRQ_BY_NAME(MEC5_ESPI_NODE, fc, priority),
		mec5_espi_fc_isr,
		DEVICE_DT_GET(MEC5_ESPI_NODE), 0);
	irq_enable(DT_IRQ_BY_NAME(MEC5_ESPI_NODE, fc, irq));
	mec_espi_fc_girq_ctrl(1);
}
