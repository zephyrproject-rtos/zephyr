/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet: https://www.nxp.com/docs/en/data-sheet/MFRC522.pdf
 */

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "mfrc522.h"

LOG_MODULE_REGISTER(nfc_mfrc522, CONFIG_NFC_DRIVERS_LOG_LEVEL);

#define MFRC522_REG_COMMAND     0x01U
#define MFRC522_REG_COM_IEN     0x02U
#define MFRC522_REG_DIV_IEN     0x03U
#define MFRC522_REG_COM_IRQ     0x04U
#define MFRC522_REG_ERROR       0x06U
#define MFRC522_REG_FIFO_DATA   0x09U
#define MFRC522_REG_FIFO_LEVEL  0x0AU
#define MFRC522_REG_BIT_FRAMING 0x0DU
#define MFRC522_REG_MODE        0x11U
#define MFRC522_REG_TX_MODE     0x12U
#define MFRC522_REG_RX_MODE     0x13U
#define MFRC522_REG_TX_CONTROL  0x14U
#define MFRC522_REG_TX_ASK      0x15U
#define MFRC522_REG_TMODE       0x2AU
#define MFRC522_REG_TPRESCALER  0x2BU
#define MFRC522_REG_TRELOAD_HI  0x2CU
#define MFRC522_REG_TRELOAD_LO  0x2DU
#define MFRC522_REG_VERSION     0x37U

enum mfrc522_command {
	MFRC522_CMD_IDLE = 0x00U,
	MFRC522_CMD_TRANSCEIVE = 0x0CU,
	MFRC522_CMD_SOFT_RESET = 0x0FU,
};

#define MFRC522_BIT_FRAMING_START_SEND 0x80U
#define MFRC522_BIT_FRAMING_LAST_MASK  0x07U

#define MFRC522_MODE_CRC_EN 0x80U

#define MFRC522_FIFO_FLUSH 0x80U

#define MFRC522_TX_RF_EN         0x03U /* Tx1RFEn | Tx2RFEn */
#define MFRC522_CMD_POWER_DOWN   0x10U
#define MFRC522_RESET_TIMEOUT_MS 50U

#define MFRC522_IRQ_RX        0x20U
#define MFRC522_IRQ_ERR       0x02U
#define MFRC522_IRQ_TIMER     0x01U
#define MFRC522_IRQ_ENABLE    (MFRC522_IRQ_RX | MFRC522_IRQ_ERR | MFRC522_IRQ_TIMER)
#define MFRC522_IRQ_CLEAR_ALL 0x7FU /* Set1 = 0 clears the marked flags */
#define MFRC522_IRQ_INV       0x80U /* ComIEnReg: pin follows the inverse of the request */
#define MFRC522_IRQ_PUSH_PULL 0x80U /* DivIEnReg: CMOS pad instead of open drain */

#define MFRC522_ERR_FATAL (0x10U | 0x08U | 0x01U) /* BufferOvfl | CollErr | ProtocolErr */

#define MFRC522_MODE_DEFAULT 0x3DU

#define MFRC522_TX_ASK_FORCE_100 0x40U

#define MFRC522_TMODE_TAUTO     0x80U
#define MFRC522_TPRESCALER_25US 0xA9U
#define MFRC522_TIMER_TICK_US   25U

#define MFRC522_NFCA_BIT_NS        9440U /* 128/fc */
#define MFRC522_NFCA_BITS_PER_BYTE 9U    /* 8 data bits and the parity bit */
#define MFRC522_RX_BYTE_NS         ((MFRC522_NFCA_BIT_NS) * (MFRC522_NFCA_BITS_PER_BYTE))
#define MFRC522_RX_BYTE_US         DIV_ROUND_UP(MFRC522_RX_BYTE_NS, NSEC_PER_USEC)

#define MFRC522_VERSION_MASK    0xF0U
#define MFRC522_VERSION_MFRC522 0x90U

#define MFRC522_DEFAULT_TIMEOUT_US 50000U
#define MFRC522_IRQ_WAIT_MARGIN_US 5000U

static inline int mfrc522_read(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct mfrc522_config *cfg = dev->config;

	return cfg->transport->read_reg(dev, reg, val);
}

static inline int mfrc522_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct mfrc522_config *cfg = dev->config;

	return cfg->transport->write_reg(dev, reg, val);
}

static int mfrc522_modify(const struct device *dev, uint8_t reg, uint8_t clear, uint8_t set)
{
	uint8_t val;
	int ret;

	ret = mfrc522_read(dev, reg, &val);
	if (ret < 0) {
		return ret;
	}

	return mfrc522_write(dev, reg, (val & ~clear) | set);
}

static int mfrc522_set_antenna(const struct device *dev, bool on)
{
	return mfrc522_modify(dev, MFRC522_REG_TX_CONTROL, on ? 0U : MFRC522_TX_RF_EN,
			      on ? MFRC522_TX_RF_EN : 0U);
}

static int mfrc522_set_timeout(const struct device *dev, uint32_t timeout_us)
{
	uint32_t ticks = CLAMP(timeout_us / MFRC522_TIMER_TICK_US, 1U, 0xFFFFU);
	int ret;

	ret = mfrc522_write(dev, MFRC522_REG_TMODE, MFRC522_TMODE_TAUTO);
	if (ret < 0) {
		return ret;
	}
	ret = mfrc522_write(dev, MFRC522_REG_TPRESCALER, MFRC522_TPRESCALER_25US);
	if (ret < 0) {
		return ret;
	}
	ret = mfrc522_write(dev, MFRC522_REG_TRELOAD_HI, (uint8_t)(ticks >> 8));
	if (ret < 0) {
		return ret;
	}

	return mfrc522_write(dev, MFRC522_REG_TRELOAD_LO, (uint8_t)ticks);
}

static void mfrc522_irq_handler(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct mfrc522_data *data = CONTAINER_OF(cb, struct mfrc522_data, irq_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_sem_give(&data->irq_sem);
}

static int mfrc522_wait_irq(const struct device *dev, uint32_t timeout_us, uint8_t *irq)
{
	const struct mfrc522_config *cfg = dev->config;
	struct mfrc522_data *data = dev->data;

	if (cfg->irq_gpio.port != NULL) {
		(void)k_sem_take(&data->irq_sem, K_USEC(timeout_us + MFRC522_IRQ_WAIT_MARGIN_US));
		return mfrc522_read(dev, MFRC522_REG_COM_IRQ, irq);
	}

	k_timepoint_t end = sys_timepoint_calc(K_USEC(timeout_us));

	do {
		int ret = mfrc522_read(dev, MFRC522_REG_COM_IRQ, irq);

		if (ret < 0) {
			return ret;
		}
		if ((*irq & MFRC522_IRQ_ENABLE) != 0U) {
			return 0;
		}
	} while (!sys_timepoint_expired(end));

	return 0;
}

static nfc_proto_t mfrc522_claim(const struct device *dev)
{
	struct mfrc522_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	return NFC_PROTO_ISO14443A;
}

static int mfrc522_release(const struct device *dev)
{
	struct mfrc522_data *data = dev->data;

	k_mutex_unlock(&data->lock);

	return 0;
}

static int mfrc522_load_protocol(const struct device *dev, nfc_proto_t proto, nfc_mode_t mode)
{
	int ret;

	ARG_UNUSED(mode);

	if ((proto & NFC_PROTO_ISO14443A) == 0U) {
		return -ENOTSUP;
	}

	ret = mfrc522_write(dev, MFRC522_REG_MODE, MFRC522_MODE_DEFAULT);
	if (ret < 0) {
		return ret;
	}
	ret = mfrc522_write(dev, MFRC522_REG_TX_MODE, 0x00U);
	if (ret < 0) {
		return ret;
	}

	return mfrc522_write(dev, MFRC522_REG_RX_MODE, 0x00U);
}

static int mfrc522_get_properties(const struct device *dev, struct nfc_property *props,
				  size_t props_len)
{
	struct mfrc522_data *data = dev->data;

	for (size_t i = 0; i < props_len; i++) {
		struct nfc_property *p = &props[i];

		switch (p->type) {
		case NFC_PROP_HW_TX_CRC:
			p->hw_tx_crc = data->hw_tx_crc;
			p->status = 0;
			break;
		case NFC_PROP_HW_RX_CRC:
			p->hw_rx_crc = data->hw_rx_crc;
			p->status = 0;
			break;
		case NFC_PROP_TIMEOUT:
			p->timeout_us = data->timeout_us;
			p->status = 0;
			break;
		default:
			p->status = -ENOTSUP;
			break;
		}
	}

	return 0;
}

static int mfrc522_set_properties(const struct device *dev, struct nfc_property *props,
				  size_t props_len)
{
	struct mfrc522_data *data = dev->data;

	for (size_t i = 0; i < props_len; i++) {
		struct nfc_property *p = &props[i];

		switch (p->type) {
		case NFC_PROP_RF_FIELD:
			p->status = mfrc522_set_antenna(dev, p->rf_on);
			break;
		case NFC_PROP_HW_TX_CRC:
			data->hw_tx_crc = p->hw_tx_crc;
			p->status = 0;
			break;
		case NFC_PROP_HW_RX_CRC:
			data->hw_rx_crc = p->hw_rx_crc;
			p->status = 0;
			break;
		case NFC_PROP_TIMEOUT:
			data->timeout_us = p->timeout_us;
			p->status = 0;
			break;
		case NFC_PROP_MFC_CRYPTO:
			p->status = p->mfc_crypto_on ? -ENOTSUP : 0;
			break;
		default:
			p->status = -ENOTSUP;
			break;
		}
	}

	return 0;
}

static int mfrc522_read_response(const struct device *dev, uint8_t irq, uint8_t *rx_data,
				 uint16_t *rx_len)
{
	uint8_t err;
	uint8_t fifo;
	int ret;

	if ((irq & MFRC522_IRQ_RX) == 0U) {
		return -ETIMEDOUT;
	}

	ret = mfrc522_read(dev, MFRC522_REG_ERROR, &err);
	if (ret < 0) {
		return ret;
	}
	if ((err & MFRC522_ERR_FATAL) != 0U) {
		LOG_DBG("Transceive error 0x%02x", err);
		return -EIO;
	}

	ret = mfrc522_read(dev, MFRC522_REG_FIFO_LEVEL, &fifo);
	if (ret < 0) {
		return ret;
	}
	if (fifo > *rx_len) {
		return -ENOMEM;
	}

	for (uint8_t i = 0; i < fifo; i++) {
		ret = mfrc522_read(dev, MFRC522_REG_FIFO_DATA, &rx_data[i]);
		if (ret < 0) {
			return ret;
		}
	}

	*rx_len = fifo;

	return 0;
}

static int mfrc522_im_transceive(const struct device *dev, const uint8_t *tx_data, uint16_t tx_len,
				 uint8_t tx_last_bits, uint8_t *rx_data, uint16_t *rx_len)
{
	const struct mfrc522_config *cfg = dev->config;
	struct mfrc522_data *data = dev->data;
	uint32_t frame_delay_us = data->timeout_us ? data->timeout_us : MFRC522_DEFAULT_TIMEOUT_US;
	uint32_t timeout_us = frame_delay_us + *rx_len * MFRC522_RX_BYTE_US;
	uint8_t last_bits = tx_last_bits & MFRC522_BIT_FRAMING_LAST_MASK;
	uint8_t irq = 0;
	int ret;

	ret = mfrc522_set_timeout(dev, timeout_us);
	if (ret < 0) {
		return ret;
	}

	ret = mfrc522_write(dev, MFRC522_REG_TX_MODE,
			    data->hw_tx_crc ? MFRC522_MODE_CRC_EN : 0x00U);
	if (ret < 0) {
		return ret;
	}
	ret = mfrc522_write(dev, MFRC522_REG_RX_MODE,
			    data->hw_rx_crc ? MFRC522_MODE_CRC_EN : 0x00U);
	if (ret < 0) {
		return ret;
	}
	ret = mfrc522_write(dev, MFRC522_REG_COMMAND, MFRC522_CMD_IDLE);
	if (ret < 0) {
		return ret;
	}
	ret = mfrc522_write(dev, MFRC522_REG_COM_IRQ, MFRC522_IRQ_CLEAR_ALL);
	if (ret < 0) {
		return ret;
	}
	ret = mfrc522_write(dev, MFRC522_REG_FIFO_LEVEL, MFRC522_FIFO_FLUSH);
	if (ret < 0) {
		return ret;
	}

	for (uint16_t i = 0; i < tx_len; i++) {
		ret = mfrc522_write(dev, MFRC522_REG_FIFO_DATA, tx_data[i]);
		if (ret < 0) {
			return ret;
		}
	}

	if (cfg->irq_gpio.port != NULL) {
		k_sem_reset(&data->irq_sem);
	}

	ret = mfrc522_write(dev, MFRC522_REG_BIT_FRAMING, last_bits);
	if (ret < 0) {
		return ret;
	}
	ret = mfrc522_write(dev, MFRC522_REG_COMMAND, MFRC522_CMD_TRANSCEIVE);
	if (ret < 0) {
		return ret;
	}
	ret = mfrc522_modify(dev, MFRC522_REG_BIT_FRAMING, 0U, MFRC522_BIT_FRAMING_START_SEND);
	if (ret < 0) {
		return ret;
	}

	ret = mfrc522_wait_irq(dev, timeout_us, &irq);
	if (ret < 0) {
		return ret;
	}

	ret = mfrc522_write(dev, MFRC522_REG_BIT_FRAMING, last_bits);
	if (ret < 0) {
		return ret;
	}

	return mfrc522_read_response(dev, irq, rx_data, rx_len);
}

static nfc_proto_t mfrc522_supported_protocols(const struct device *dev)
{
	ARG_UNUSED(dev);

	return NFC_PROTO_ISO14443A;
}

static nfc_mode_t mfrc522_supported_modes(const struct device *dev, nfc_proto_t proto)
{
	ARG_UNUSED(dev);

	if ((proto & NFC_PROTO_ISO14443A) == 0U) {
		return 0;
	}

	return NFC_MODE_INITIATOR | NFC_MODE_TX_106 | NFC_MODE_RX_106;
}

DEVICE_API(nfc, mfrc522_nfc_api) = {
	.claim = mfrc522_claim,
	.release = mfrc522_release,
	.load_protocol = mfrc522_load_protocol,
	.get_properties = mfrc522_get_properties,
	.set_properties = mfrc522_set_properties,
	.im_transceive = mfrc522_im_transceive,
	.supported_protocols = mfrc522_supported_protocols,
	.supported_modes = mfrc522_supported_modes,
};

static int mfrc522_setup_irq(const struct device *dev)
{
	const struct mfrc522_config *cfg = dev->config;
	struct mfrc522_data *data = dev->data;
	int ret;

	if (cfg->irq_gpio.port == NULL) {
		return 0;
	}

	if (!gpio_is_ready_dt(&cfg->irq_gpio)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->irq_gpio, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->irq_cb, mfrc522_irq_handler, BIT(cfg->irq_gpio.pin));
	ret = gpio_add_callback(cfg->irq_gpio.port, &data->irq_cb);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = mfrc522_write(dev, MFRC522_REG_DIV_IEN, MFRC522_IRQ_PUSH_PULL);
	if (ret < 0) {
		return ret;
	}

	return mfrc522_write(dev, MFRC522_REG_COM_IEN, MFRC522_IRQ_INV | MFRC522_IRQ_ENABLE);
}

/*
 * The reset runs until the internal oscillator is stable, which the PowerDown
 * bit reports. Registers written before then are lost.
 */
static int mfrc522_wait_reset(const struct device *dev)
{
	k_timepoint_t deadline = sys_timepoint_calc(K_MSEC(MFRC522_RESET_TIMEOUT_MS));

	do {
		uint8_t cmd;
		int ret = mfrc522_read(dev, MFRC522_REG_COMMAND, &cmd);

		if (ret < 0) {
			return ret;
		}
		if ((cmd & MFRC522_CMD_POWER_DOWN) == 0U) {
			return 0;
		}
		k_sleep(K_MSEC(1));
	} while (!sys_timepoint_expired(deadline));

	return -ETIMEDOUT;
}

int mfrc522_init_common(const struct device *dev)
{
	const struct mfrc522_config *cfg = dev->config;
	struct mfrc522_data *data = dev->data;
	uint8_t version;
	int ret;

	k_mutex_init(&data->lock);
	k_sem_init(&data->irq_sem, 0, 1);
	data->timeout_us = MFRC522_DEFAULT_TIMEOUT_US;

	if (cfg->reset_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}
		k_sleep(K_MSEC(1));
		ret = gpio_pin_set_dt(&cfg->reset_gpio, 0);
		if (ret < 0) {
			return ret;
		}
		k_sleep(K_MSEC(1));
	}

	ret = mfrc522_write(dev, MFRC522_REG_COMMAND, MFRC522_CMD_SOFT_RESET);
	if (ret < 0) {
		LOG_ERR("Soft reset failed (%d)", ret);
		return ret;
	}

	ret = mfrc522_wait_reset(dev);
	if (ret < 0) {
		LOG_ERR("Soft reset did not complete (%d)", ret);
		return ret;
	}

	ret = mfrc522_read(dev, MFRC522_REG_VERSION, &version);
	if (ret < 0) {
		return ret;
	}
	if ((version & MFRC522_VERSION_MASK) != MFRC522_VERSION_MFRC522) {
		LOG_ERR("Unexpected version 0x%02x", version);
		return -ENODEV;
	}
	LOG_INF("MFRC522 version 0x%02x", version);

	ret = mfrc522_set_timeout(dev, MFRC522_DEFAULT_TIMEOUT_US);
	if (ret < 0) {
		return ret;
	}

	ret = mfrc522_write(dev, MFRC522_REG_TX_ASK, MFRC522_TX_ASK_FORCE_100);
	if (ret < 0) {
		return ret;
	}
	ret = mfrc522_write(dev, MFRC522_REG_MODE, MFRC522_MODE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = mfrc522_setup_irq(dev);
	if (ret < 0) {
		LOG_ERR("IRQ setup failed (%d)", ret);
		return ret;
	}

	return mfrc522_set_antenna(dev, true);
}

#define MFRC522_CONFIG(inst)                                                                       \
	{                                                                                          \
		.transport = &mfrc522_spi_transport,                                               \
		.bus = SPI_DT_SPEC_INST_GET(inst, MFRC522_SPI_OPERATION),                          \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                    \
		.irq_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, irq_gpios, {0}),                        \
	}

#define MFRC522_DEFINE(inst)                                                                       \
	static struct mfrc522_data mfrc522_data_##inst;                                            \
	static const struct mfrc522_config mfrc522_config_##inst = MFRC522_CONFIG(inst);           \
	DEVICE_DT_INST_DEFINE(inst, mfrc522_init_common, NULL, &mfrc522_data_##inst,               \
			      &mfrc522_config_##inst, POST_KERNEL, CONFIG_NFC_INIT_PRIORITY,       \
			      &mfrc522_nfc_api);

DT_INST_FOREACH_STATUS_OKAY(MFRC522_DEFINE)
