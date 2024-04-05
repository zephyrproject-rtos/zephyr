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

LOG_MODULE_REGISTER(espi_oob, CONFIG_ESPI_LOG_LEVEL);

/* ---- OOB API invoked from core eSPI driver ---- */

/* NOTE: MEC15xx and MEC172x eSPI driver copies caller's data buffer to
 * a driver data buffer. OOB HW is configured to transmit from driver
 * buffer. This was probably done because the hardware requires 4-byte
 * aligned buffer.
 *
 */
int mec5_espi_oob_upstream(const struct device *dev, struct espi_oob_packet *pckt)
{
	const struct espi_mec5_dev_config *devcfg = dev->config;
	struct espi_io_regs *iob = devcfg->iob;
	struct espi_mec5_data *data = dev->data;
	struct espi_mec5_oob_data *odata = &data->oob_data;
	struct mec_espi_oob_buf oob_buf = {0};
	uint32_t temp = 0;
	int ret = 0;

	LOG_DBG("%s", __func__);

	if (!pckt || (pckt->len > CONFIG_ESPI_OOB_BUFFER_SIZE)) {
		LOG_ERR("Bad OOB packet");
		return -EINVAL;
	}

	temp = mec_espi_oob_en_status(iob);
	if (!(temp & BIT(MEC_ESPI_CHAN_ENABLED_POS))) {
		LOG_ERR("OOB chan disabled");
		return -EIO;
	}

	if (mec_espi_oob_tx_is_busy(iob)) {
		LOG_ERR("OOB TX busy");
		return -EBUSY;
	}

	memcpy(odata->tx_mem, pckt->buf, pckt->len);

	oob_buf.maddr = (uint32_t)pckt->buf;
	oob_buf.len = pckt->len;
	ret = mec_espi_oob_buffer_set(iob, MEC_ESPI_OOB_DIR_UP, &oob_buf);
	if (ret) {
		return -EIO;
	}

	mec_espi_oob_tx_start(iob, odata->tx_tag, 1u);

	ret = k_sem_take(&odata->tx_sync, K_MSEC(MEC5_MAX_OOB_TIMEOUT_MS));
	if (ret == -EAGAIN) {
		return -ETIMEDOUT;
	}

	if (mec_espi_oob_is_error(odata->tx_status, MEC_ESPI_OOB_DIR_UP)) {
		return -EIO;
	}

	return 0;
}

/* NOTE: MEC15xx and MEC172x eSPI driver has an OOB receive buffer
 * meeting HW alignment requirements. We do not want to waste data memory
 * and will use the caller's buffer. If the caller's buffer is not aligned
 * on a 4-byte boundary we will return an error. The other downside to using
 * a buffer supplied by the caller is we can lose packets.
 */
int mec5_espi_oob_downstream(const struct device *dev, struct espi_oob_packet *pckt)
{
	const struct espi_mec5_dev_config *devcfg = dev->config;
	struct espi_io_regs *iob = devcfg->iob;
	struct espi_mec5_data *data = dev->data;
	struct espi_mec5_oob_data *odata = &data->oob_data;
	uint32_t rxlen = 0;
	int ret = 0;

	if (!pckt) {
		return -EINVAL;
	}

	if (mec_espi_oob_is_error(odata->rx_status, MEC_ESPI_OOB_DIR_DN)) {
		return -EIO;
	}

#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
#else
	ret = k_sem_take(&odata->rx_sync, K_MSEC(MEC5_MAX_OOB_TIMEOUT_MS));
	if (ret == -EAGAIN) {
		return -ETIMEDOUT;
	}
#endif

	rxlen = mec_espi_oob_received_len(iob);
	if (rxlen > pckt->len) {
		return -EIO;
	}

	memcpy(pckt->buf, odata->rx_mem, rxlen);
	memset(odata->rx_mem, 0, rxlen);
	pckt->len = rxlen;

	mec_espi_oob_rx_buffer_avail(iob);

	return 0;
}


/* ---- OOB Interrupts ---- */

/* OOB channel is held in reset until ESPI_nRESET de-asserts.
 * The eSPI Host Controller will send OOB Channel enable.
 * This routine is called from the OOB Channel Enable handler.
 * After this routine returns the handler will set OOB Ready to
 * let the Host know the OOB channel is ready for use.
 */
static void mec5_espi_oob_init(const struct device *dev)
{
	const struct espi_mec5_dev_config *devcfg = dev->config;
	struct espi_io_regs *iob = devcfg->iob;
	struct espi_mec5_data *data = dev->data;
	struct espi_mec5_oob_data *oob_data = &data->oob_data;
	struct mec_espi_oob_buf oob_buf = {0};
	uint32_t ien = 0;
	int ret = 0;

	oob_buf.maddr = (uint32_t)&oob_data->tx_mem[0];
	oob_buf.len = CONFIG_ESPI_OOB_BUFFER_SIZE;
	ret = mec_espi_oob_buffer_set(iob, MEC_ESPI_OOB_DIR_UP, &oob_buf);
	if (ret) {
		LOG_ERR("OOB TX buffer init error (%d)", ret);
	}

	oob_buf.maddr = (uint32_t)&oob_data->rx_mem[0];
	oob_buf.len = CONFIG_ESPI_OOB_BUFFER_SIZE;
	ret = mec_espi_oob_buffer_set(iob, MEC_ESPI_OOB_DIR_DN, &oob_buf);
	if (ret) {
		LOG_ERR("OOB RX buffer init error (%d)", ret);
	}

	ien = BIT(MEC_ESPI_OOB_UP_INTR_DONE_POS) | BIT(MEC_ESPI_OOB_DN_INTR_DONE_POS);
	mec_espi_oob_intr_ctrl(iob, ien, 1u);

	if (!ret) {
		mec_espi_oob_rx_buffer_avail(iob);
	}
}

/* OOB channel upstream data transfer (Target to Host) and OOB channel enable change */
static void mec5_espi_oob_up_isr(const struct device *dev)
{
	const struct espi_mec5_dev_config *devcfg = dev->config;
	struct espi_io_regs *iob = devcfg->iob;
	struct espi_mec5_data *data = dev->data;
	struct espi_mec5_oob_data *oob_data = &data->oob_data;
	struct espi_event evt;
	int chan_en_change;
	uint32_t status;

	status = mec_espi_oob_status(iob, MEC_ESPI_OOB_DIR_UP);

	LOG_DBG("ISR OOB Up: 0x%0x", status);

	oob_data->tx_status = status;
	if (mec_espi_oob_is_done(status, MEC_ESPI_OOB_DIR_UP)) {
		mec_espi_oob_status_clr_all(iob, MEC_ESPI_OOB_DIR_DN);
		k_sem_give(&oob_data->tx_sync);
	}

	chan_en_change = mec_espi_oob_up_is_chan_event(status);
	if (chan_en_change) {
		mec_espi_oob_status_clr_chen_change(iob);
		if (chan_en_change > 0) { /* channel was enabled? */
			mec5_espi_oob_init(dev);
			mec_espi_oob_ready_set(iob);
			evt.evt_data = 1;
		} else {
			evt.evt_data = 0;
		}
		evt.evt_type = ESPI_BUS_EVENT_CHANNEL_READY;
		evt.evt_details = ESPI_CHANNEL_OOB;
		espi_send_callbacks(&data->callbacks, dev, evt);
	}
}

/* OOB channel downstream data transfer (Host to Target) */
static void mec5_espi_oob_dn_isr(const struct device *dev)
{
	const struct espi_mec5_dev_config *devcfg = dev->config;
	struct espi_io_regs *iob = devcfg->iob;
	struct espi_mec5_data *data = dev->data;
	struct espi_mec5_oob_data *oob_data = &data->oob_data;
#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	struct espi_event evt = { .evt_type = ESPI_BUS_EVENT_OOB_RECEIVED,
				  .evt_details = 0,
				  .evt_data = 0 };
#endif
	uint32_t status = mec_espi_oob_status(iob, MEC_ESPI_OOB_DIR_DN);

	oob_data->rx_status = status;
	LOG_DBG("ISR OOB Dn: status = 0x%0x", status);

	if (mec_espi_oob_is_done(status, MEC_ESPI_OOB_DIR_DN)) {
		mec_espi_oob_status_clr_all(iob, MEC_ESPI_OOB_DIR_DN);
#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
		evt.evt_details = mec_espi_oob_received_len(iob);
		espi_send_callbacks(&data->callbacks, dev, evt);
#else
		k_sem_give(&oob_data->rx_sync);
#endif
	}

}

#define MEC5_ESPI_NODE DT_NODELABEL(espi0)

void mec5_espi_oob_irq_connect(const struct device *espi_dev)
{
	IRQ_CONNECT(DT_IRQ_BY_NAME(MEC5_ESPI_NODE, oob_up, irq),
		DT_IRQ_BY_NAME(MEC5_ESPI_NODE, oob_up, priority),
		mec5_espi_oob_up_isr,
		DEVICE_DT_GET(MEC5_ESPI_NODE), 0);
	irq_enable(DT_IRQ_BY_NAME(MEC5_ESPI_NODE, oob_up, irq));
	IRQ_CONNECT(DT_IRQ_BY_NAME(MEC5_ESPI_NODE, oob_dn, irq),
		DT_IRQ_BY_NAME(MEC5_ESPI_NODE, oob_dn, priority),
		mec5_espi_oob_dn_isr,
		DEVICE_DT_GET(MEC5_ESPI_NODE), 0);
	irq_enable(DT_IRQ_BY_NAME(MEC5_ESPI_NODE, oob_dn, irq));
	mec_espi_oob_girq_ctrl(1, MEC_ESPI_OOB_DIR_UP | MEC_ESPI_OOB_DIR_DN);
}
