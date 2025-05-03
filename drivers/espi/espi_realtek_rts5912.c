/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_rts5912_espi

#include <zephyr/kernel.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_rts5912.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(espi, CONFIG_ESPI_LOG_LEVEL);

#include "espi_utils.h"
#include "reg/reg_espi.h"

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "support only one espi compatible node");

struct espi_rts5912_config {
	volatile struct espi_reg *const espi_reg;
	uint32_t espislv_clk_grp;
	uint32_t espislv_clk_idx;
	const struct device *clk_dev;
	const struct pinctrl_dev_config *pcfg;
};

struct espi_rts5912_data {
	sys_slist_t callbacks;
	uint32_t config_data;
#ifdef CONFIG_ESPI_OOB_CHANNEL
	struct k_sem oob_rx_lock;
	struct k_sem oob_tx_lock;
	uint8_t *oob_tx_ptr;
	uint8_t *oob_rx_ptr;
	bool oob_tx_busy;
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	struct k_sem flash_lock;
	uint8_t *maf_ptr;
#endif
};

/*
 * =========================================================================
 * ESPI OOB channel
 * =========================================================================
 */

#ifdef CONFIG_ESPI_OOB_CHANNEL

#define MAX_OOB_TIMEOUT 200UL /* ms */
#define OOB_BUFFER_SIZE 256UL

static int espi_rts5912_send_oob(const struct device *dev, struct espi_oob_packet *pckt)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	int ret;

	if (!(espi_reg->EOCFG & ESPI_EOCFG_CHRDY)) {
		LOG_ERR("%s: OOB channel isn't ready", __func__);
		return -EIO;
	}

	if (espi_data->oob_tx_busy) {
		LOG_ERR("%s: OOB channel is busy", __func__);
		return -EIO;
	}

	if (pckt->len > OOB_BUFFER_SIZE) {
		LOG_ERR("%s: OOB Tx have no insufficient space", __func__);
		return -EINVAL;
	}

	for (int i = 0; i < pckt->len; i++) {
		espi_data->oob_tx_ptr[i] = pckt->buf[i];
	}

	espi_reg->EOTXLEN = pckt->len - 1;
	espi_reg->EOTXCTRL = ESPI_EOTXCTRL_TXSTR;

	espi_data->oob_tx_busy = true;

	/* Wait until ISR or timeout */
	ret = k_sem_take(&espi_data->oob_tx_lock, K_MSEC(MAX_OOB_TIMEOUT));
	if (ret == -EAGAIN) {
		return -ETIMEDOUT;
	}

	espi_data->oob_tx_busy = false;

	return 0;
}

static int espi_rts5912_receive_oob(const struct device *dev, struct espi_oob_packet *pckt)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	uint32_t rx_len;

	if (!(espi_reg->EOCFG & ESPI_EOCFG_CHRDY)) {
		LOG_ERR("%s: OOB channel isn't ready", __func__);
		return -EIO;
	}

	if (espi_reg->EOSTS & ESPI_EOSTS_RXPND) {
		LOG_ERR("OOB Receive Pending");
		return -EIO;
	}

#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	/* Wait until ISR or timeout */
	int ret = k_sem_take(&espi_data->oob_rx_lock, K_MSEC(MAX_OOB_TIMEOUT));

	if (ret == -EAGAIN) {
		LOG_ERR("OOB Rx Timeout");
		return -ETIMEDOUT;
	}
#endif

	/* Check if buffer passed to driver can fit the received buffer */
	rx_len = espi_reg->EORXLEN;

	if (rx_len > pckt->len) {
		LOG_ERR("space rcvd %d vs %d", rx_len, pckt->len);
		return -EIO;
	}

	pckt->len = rx_len;

	for (int i = 0; i < rx_len; i++) {
		pckt->buf[i] = espi_data->oob_rx_ptr[i];
	}

	return 0;
}

static void espi_oob_tx_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	uint32_t status = espi_reg->EOSTS;

	if (status & ESPI_EOSTS_TXDONE) {
		k_sem_give(&espi_data->oob_tx_lock);
		espi_reg->EOSTS = ESPI_EOSTS_TXDONE;
	}
}

static void espi_oob_rx_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	uint32_t status = espi_reg->EOSTS;

#ifdef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	struct espi_event evt = {
		.evt_type = ESPI_BUS_EVENT_OOB_RECEIVED, .evt_details = 0, .evt_data = 0};
#endif

	if (status & ESPI_EOSTS_RXDONE) {
#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
		k_sem_give(&espi_data->oob_rx_lock);
#else
		k_busy_wait(250);
		evt.evt_details = espi_reg->EORXLEN;
		espi_send_callbacks(&espi_data->callbacks, dev, evt);
#endif
		espi_reg->EOSTS = ESPI_EOSTS_RXDONE;
	}
}

static void espi_oob_chg_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;

	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;

	struct espi_event evt = {.evt_type = ESPI_BUS_EVENT_CHANNEL_READY,
				 .evt_details = ESPI_CHANNEL_OOB,
				 .evt_data = 0};

	uint32_t status = espi_reg->EOSTS;
	uint32_t config = espi_reg->EOCFG;

	if (status & ESPI_EOSTS_CFGENCHG) {
		evt.evt_data = config & ESPI_EVCFG_CHEN ? 1 : 0;
		espi_send_callbacks(&espi_data->callbacks, dev, evt);
		espi_reg->EOSTS = ESPI_EOSTS_CFGENCHG;
	}
}

static uint8_t oob_tx_buffer[OOB_BUFFER_SIZE] __aligned(4);
static uint8_t oob_rx_buffer[OOB_BUFFER_SIZE] __aligned(4);

static int espi_oob_ch_setup(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	espi_data->oob_tx_busy = false;

	espi_data->oob_tx_ptr = oob_tx_buffer;
	if (espi_data->oob_tx_ptr == NULL) {
		LOG_ERR("Failed to allocate OOB Tx buffer");
		return -ENOMEM;
	}

	espi_data->oob_rx_ptr = oob_rx_buffer;
	if (espi_data->oob_tx_ptr == NULL) {
		LOG_ERR("Failed to allocate OOB Rx buffer");
		return -ENOMEM;
	}

	espi_reg->EOTXBUF = (uint32_t)espi_data->oob_tx_ptr;
	espi_reg->EORXBUF = (uint32_t)espi_data->oob_rx_ptr;

	espi_reg->EOTXINTEN = ESPI_EOTXINTEN_TXEN;
	espi_reg->EORXINTEN = (ESPI_EORXINTEN_RXEN | ESPI_EORXINTEN_CHENCHG);

	k_sem_init(&espi_data->oob_tx_lock, 0, 1);
#ifndef CONFIG_ESPI_OOB_CHANNEL_RX_ASYNC
	k_sem_init(&espi_data->oob_rx_lock, 0, 1);
#endif

	NVIC_ClearPendingIRQ(DT_IRQ_BY_NAME(DT_DRV_INST(0), oob_tx, irq));
	NVIC_ClearPendingIRQ(DT_IRQ_BY_NAME(DT_DRV_INST(0), oob_rx, irq));
	NVIC_ClearPendingIRQ(DT_IRQ_BY_NAME(DT_DRV_INST(0), oob_chg, irq));

	/* Tx */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), oob_tx, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), oob_tx, priority), espi_oob_tx_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(0), oob_tx, irq));

	/* Rx */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), oob_rx, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), oob_rx, priority), espi_oob_rx_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(0), oob_rx, irq));

	/* Chg */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), oob_chg, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), oob_chg, priority), espi_oob_chg_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(0), oob_chg, irq));

	return 0;
}

#endif /* CONFIG_ESPI_OOB_CHANNEL */

/*
 * =========================================================================
 * ESPI flash channel
 * =========================================================================
 */

#ifdef CONFIG_ESPI_FLASH_CHANNEL

#define MAX_FLASH_TIMEOUT 1000UL
#define MAF_BUFFER_SIZE   512UL

enum {
	MAF_TR_READ = 0,
	MAF_TR_WRITE = 1,
	MAF_TR_ERASE = 2,
};

static int espi_rts5912_flash_read(const struct device *dev, struct espi_flash_packet *pckt)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	int ret;
	uint32_t ctrl;

	if (!(espi_reg->EFCONF & ESPI_EFCONF_CHEN)) {
		LOG_ERR("Flash channel is disabled");
		return -EIO;
	}

	if (pckt->len > MAF_BUFFER_SIZE) {
		LOG_ERR("Invalid size request");
		return -EINVAL;
	}

	if (espi_reg->EMCTRL & ESPI_EMCTRL_START) {
		LOG_ERR("Channel still busy");
		return -EBUSY;
	}

	ctrl = (MAF_TR_READ << ESPI_EMCTRL_MDSEL_Pos) | ESPI_EMCTRL_START;

	espi_reg->EMADR = pckt->flash_addr;
	espi_reg->EMTRLEN = pckt->len;
	espi_reg->EMCTRL = ctrl;

	/* Wait until ISR or timeout */
	ret = k_sem_take(&espi_data->flash_lock, K_MSEC(MAX_FLASH_TIMEOUT));
	if (ret == -EAGAIN) {
		LOG_ERR("%s timeout", __func__);
		return -ETIMEDOUT;
	}

	for (int i = 0; i < pckt->len; i++) {
		pckt->buf[i] = espi_data->maf_ptr[i];
	}

	return 0;
}

static int espi_rts5912_flash_write(const struct device *dev, struct espi_flash_packet *pckt)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	int ret;
	uint32_t ctrl;

	if (!(espi_reg->EFCONF & ESPI_EFCONF_CHEN)) {
		LOG_ERR("Flash channel is disabled");
		return -EIO;
	}

	if (pckt->len > MAF_BUFFER_SIZE) {
		LOG_ERR("Packet length is too big");
		return -EINVAL;
	}

	if (espi_reg->EMCTRL & ESPI_EMCTRL_START) {
		LOG_ERR("Channel still busy");
		return -EBUSY;
	}

	for (int i = 0; i < pckt->len; i++) {
		espi_data->maf_ptr[i] = pckt->buf[i];
	}

	ctrl = (MAF_TR_WRITE << ESPI_EMCTRL_MDSEL_Pos) | ESPI_EMCTRL_START;

	espi_reg->EMADR = pckt->flash_addr;
	espi_reg->EMTRLEN = pckt->len;
	espi_reg->EMCTRL = ctrl;

	/* Wait until ISR or timeout */
	ret = k_sem_take(&espi_data->flash_lock, K_MSEC(MAX_FLASH_TIMEOUT));
	if (ret == -EAGAIN) {
		LOG_ERR("%s timeout", __func__);
		return -ETIMEDOUT;
	}

	return 0;
}

static int espi_rts5912_flash_erase(const struct device *dev, struct espi_flash_packet *pckt)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	int ret;
	uint32_t ctrl;

	if (!(espi_reg->EFCONF & ESPI_EFCONF_CHEN)) {
		LOG_ERR("Flash channel is disabled");
		return -EIO;
	}

	if (espi_reg->EMCTRL & ESPI_EMCTRL_START) {
		LOG_ERR("Channel still busy");
		return -EBUSY;
	}

	ctrl = (MAF_TR_ERASE << ESPI_EMCTRL_MDSEL_Pos) | ESPI_EMCTRL_START;

	espi_reg->EMADR = pckt->flash_addr;
	espi_reg->EMTRLEN = pckt->len;
	espi_reg->EMCTRL = ctrl;

	/* Wait until ISR or timeout */
	ret = k_sem_take(&espi_data->flash_lock, K_MSEC(MAX_FLASH_TIMEOUT));
	if (ret == -EAGAIN) {
		LOG_ERR("%s timeout", __func__);
		return -ETIMEDOUT;
	}

	return 0;
}

static void espi_maf_tr_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	uint32_t status = espi_reg->EFSTS;

	if (status & ESPI_EFSTS_MAFTXDN) {
		k_sem_give(&espi_data->flash_lock);
		espi_reg->EFSTS = ESPI_EFSTS_MAFTXDN;
	}
}

static void espi_flash_chg_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;

	struct espi_event evt = {.evt_type = ESPI_BUS_EVENT_CHANNEL_READY,
				 .evt_details = ESPI_CHANNEL_FLASH,
				 .evt_data = 0};

	uint32_t status = espi_reg->EFSTS;
	uint32_t config = espi_reg->EFCONF;

	if (status & ESPI_EFSTS_CHENCHG) {
		evt.evt_data = (config & ESPI_EFCONF_CHEN) ? 1 : 0;
		espi_send_callbacks(&espi_data->callbacks, dev, evt);

		espi_reg->EFSTS = ESPI_EFSTS_CHENCHG;
	}
}

static uint8_t flash_channel_buffer[MAF_BUFFER_SIZE] __aligned(4);

static int espi_flash_ch_setup(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *espi_data = dev->data;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;

	espi_data->maf_ptr = flash_channel_buffer;
	if (espi_data->maf_ptr == NULL) {
		LOG_ERR("Failed to allocate MAF buffer");
		return -ENOMEM;
	}

	espi_reg->EMBUF = (uint32_t)espi_data->maf_ptr;
	espi_reg->EMINTEN = ESPI_EMINTEN_CHENCHG | ESPI_EMINTEN_TRDONEEN;

	k_sem_init(&espi_data->flash_lock, 0, 1);

	NVIC_ClearPendingIRQ(DT_IRQ_BY_NAME(DT_DRV_INST(0), maf_tr, irq));
	NVIC_ClearPendingIRQ(DT_IRQ_BY_NAME(DT_DRV_INST(0), flash_chg, irq));

	/* MAF Tr */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), maf_tr, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), maf_tr, priority), espi_maf_tr_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(0), maf_tr, irq));

	/* Chg */
	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), flash_chg, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), flash_chg, priority), espi_flash_chg_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(0), flash_chg, irq));

	return 0;
}

#endif /* CONFIG_ESPI_FLASH_CHANNEL */

/*
 * =========================================================================
 * ESPI common function and API
 * =========================================================================
 */

#define RTS5912_ESPI_MAX_FREQ_20 20
#define RTS5912_ESPI_MAX_FREQ_25 25
#define RTS5912_ESPI_MAX_FREQ_33 33
#define RTS5912_ESPI_MAX_FREQ_50 50
#define RTS5912_ESPI_MAX_FREQ_66 66

static int espi_rts5912_configure(const struct device *dev, struct espi_cfg *cfg)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *data = dev->data;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;

	uint32_t gen_conf = 0;
	uint8_t io_mode = 0;

	/* Maximum Frequency Supported */
	switch (cfg->max_freq) {
	case RTS5912_ESPI_MAX_FREQ_20:
		gen_conf |= 0UL << ESPI_ESPICFG_MXFREQSUP_Pos;
		break;
	case RTS5912_ESPI_MAX_FREQ_25:
		gen_conf |= 1UL << ESPI_ESPICFG_MXFREQSUP_Pos;
		break;
	case RTS5912_ESPI_MAX_FREQ_33:
		gen_conf |= 2UL << ESPI_ESPICFG_MXFREQSUP_Pos;
		break;
	case RTS5912_ESPI_MAX_FREQ_50:
		gen_conf |= 3UL << ESPI_ESPICFG_MXFREQSUP_Pos;
		break;
	case RTS5912_ESPI_MAX_FREQ_66:
		gen_conf |= 4UL << ESPI_ESPICFG_MXFREQSUP_Pos;
		break;
	default:
		return -EINVAL;
	}

	/* I/O Mode Supported */
	io_mode = cfg->io_caps >> 1;

	if (io_mode > 3) {
		return -EINVAL;
	}
	gen_conf |= io_mode << ESPI_ESPICFG_IOSUP_Pos;

	/* Channel Supported */
	if (cfg->channel_caps & ESPI_CHANNEL_PERIPHERAL) {
		gen_conf |= BIT(0) << ESPI_ESPICFG_CHSUP_Pos;
	}

	if (cfg->channel_caps & ESPI_CHANNEL_VWIRE) {
		gen_conf |= BIT(1) << ESPI_ESPICFG_CHSUP_Pos;
	}

	if (cfg->channel_caps & ESPI_CHANNEL_OOB) {
		gen_conf |= BIT(2) << ESPI_ESPICFG_CHSUP_Pos;
	}

	if (cfg->channel_caps & ESPI_CHANNEL_FLASH) {
		gen_conf |= BIT(3) << ESPI_ESPICFG_CHSUP_Pos;
	}

	espi_reg->ESPICFG = gen_conf;
	data->config_data = espi_reg->ESPICFG;

	return 0;
}

static bool espi_rts5912_channel_ready(const struct device *dev, enum espi_channel ch)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;

	switch (ch) {
	case ESPI_CHANNEL_PERIPHERAL:
		return espi_reg->EPCFG & ESPI_EPCFG_CHEN ? true : false;
	case ESPI_CHANNEL_VWIRE:
		return espi_reg->EVCFG & ESPI_EVCFG_CHEN ? true : false;
	case ESPI_CHANNEL_OOB:
		return espi_reg->EOCFG & ESPI_EOCFG_CHEN ? true : false;
	case ESPI_CHANNEL_FLASH:
		return espi_reg->EFCONF & ESPI_EFCONF_CHEN ? true : false;
	default:
		return false;
	}
}

static int espi_rts5912_manage_callback(const struct device *dev, struct espi_callback *callback,
					bool set)
{
	struct espi_rts5912_data *data = dev->data;

	return espi_manage_callback(&data->callbacks, callback, set);
}

static DEVICE_API(espi, espi_rts5912_driver_api) = {
	.config = espi_rts5912_configure,
	.get_channel_status = espi_rts5912_channel_ready,
	.manage_callback = espi_rts5912_manage_callback,
#ifdef CONFIG_ESPI_OOB_CHANNEL
	.send_oob = espi_rts5912_send_oob,
	.receive_oob = espi_rts5912_receive_oob,
#endif
#ifdef CONFIG_ESPI_FLASH_CHANNEL
	.flash_read = espi_rts5912_flash_read,
	.flash_write = espi_rts5912_flash_write,
	.flash_erase = espi_rts5912_flash_erase,
#endif
};

static void espi_rst_isr(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct espi_rts5912_data *data = dev->data;

	struct espi_event evt = {.evt_type = ESPI_BUS_RESET, .evt_details = 0, .evt_data = 0};

	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;
	uint32_t status = espi_reg->ERSTCFG;

	espi_reg->ERSTCFG |= ESPI_ERSTCFG_RSTSTS;
	espi_reg->ERSTCFG ^= ESPI_ERSTCFG_RSTPOL;

	if (status & ESPI_ERSTCFG_RSTSTS) {
		if (status & ESPI_ERSTCFG_RSTPOL) {
			/* rst pin high go low trigger interrupt */
			evt.evt_data = 0;
		} else {
			/* rst pin low go high trigger interrupt */
			evt.evt_data = 1;
		}
		espi_send_callbacks(&data->callbacks, dev, evt);
	}
}

static void espi_bus_reset_setup(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	volatile struct espi_reg *const espi_reg = espi_config->espi_reg;

	espi_reg->ERSTCFG = ESPI_ERSTCFG_RSTINTEN;
	espi_reg->ERSTCFG = ESPI_ERSTCFG_RSTMONEN;

	if (espi_reg->ERSTCFG & ESPI_ERSTCFG_RSTSTS) {
		/* high to low */
		espi_reg->ERSTCFG =
			ESPI_ERSTCFG_RSTMONEN | ESPI_ERSTCFG_RSTPOL | ESPI_ERSTCFG_RSTINTEN;
	} else {
		/* low to high */
		espi_reg->ERSTCFG = ESPI_ERSTCFG_RSTMONEN | ESPI_ERSTCFG_RSTINTEN;
	}

	NVIC_ClearPendingIRQ(DT_IRQ_BY_NAME(DT_DRV_INST(0), bus_rst, irq));

	IRQ_CONNECT(DT_IRQ_BY_NAME(DT_DRV_INST(0), bus_rst, irq),
		    DT_IRQ_BY_NAME(DT_DRV_INST(0), bus_rst, priority), espi_rst_isr,
		    DEVICE_DT_GET(DT_DRV_INST(0)), 0);
	irq_enable(DT_IRQ_BY_NAME(DT_DRV_INST(0), bus_rst, irq));
}

static int espi_rts5912_init(const struct device *dev)
{
	const struct espi_rts5912_config *const espi_config = dev->config;
	struct rts5912_sccon_subsys sccon;

	int rc;

	/* Setup eSPI pins */
	rc = pinctrl_apply_state(espi_config->pcfg, PINCTRL_STATE_DEFAULT);
	if (rc < 0) {
		LOG_ERR("eSPI pinctrl setup failed (%d)", rc);
		return rc;
	}

	if (!device_is_ready(espi_config->clk_dev)) {
		LOG_ERR("eSPI clock not ready");
		return -ENODEV;
	}

	/* Enable eSPI clock */
	sccon.clk_grp = espi_config->espislv_clk_grp;
	sccon.clk_idx = espi_config->espislv_clk_idx;
	rc = clock_control_on(espi_config->clk_dev, (clock_control_subsys_t)&sccon);
	if (rc != 0) {
		LOG_ERR("eSPI clock control on failed");
		goto exit;
	}

	/* Setup eSPI bus reset */
	espi_bus_reset_setup(dev);

#ifdef CONFIG_ESPI_OOB_CHANNEL
	/* Setup eSPI OOB channel */
	rc = espi_oob_ch_setup(dev);
	if (rc != 0) {
		LOG_ERR("eSPI OOB channel setup failed");
		goto exit;
	}
#endif

#ifdef CONFIG_ESPI_FLASH_CHANNEL
	/* Setup eSPI flash channel */
	rc = espi_flash_ch_setup(dev);
	if (rc != 0) {
		LOG_ERR("eSPI flash channel setup failed");
		goto exit;
	}
#endif

exit:
	return rc;
}

PINCTRL_DT_INST_DEFINE(0);

static struct espi_rts5912_data espi_rts5912_data_0;

static const struct espi_rts5912_config espi_rts5912_config = {
	.espi_reg = (volatile struct espi_reg *const)DT_INST_REG_ADDR_BY_NAME(0, espi_target),
	.espislv_clk_grp = DT_CLOCKS_CELL_BY_NAME(DT_DRV_INST(0), espi_target, clk_grp),
	.espislv_clk_idx = DT_CLOCKS_CELL_BY_NAME(DT_DRV_INST(0), espi_target, clk_idx),
	.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

DEVICE_DT_INST_DEFINE(0, &espi_rts5912_init, NULL, &espi_rts5912_data_0, &espi_rts5912_config,
		      PRE_KERNEL_2, CONFIG_ESPI_INIT_PRIORITY, &espi_rts5912_driver_api);
