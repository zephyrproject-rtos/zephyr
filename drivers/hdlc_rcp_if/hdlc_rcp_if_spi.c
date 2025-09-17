/**
 * Copyright (c) 2025 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <openthread/platform/radio.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/hdlc_rcp_if/hdlc_rcp_if.h>
#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/net/openthread.h>
#include <zephyr/sys/crc.h>

#define DT_DRV_COMPAT spi_hdlc_rcp_if

LOG_MODULE_REGISTER(hdlc_rcp_if_spi, CONFIG_HDLC_RCP_IF_DRIVER_LOG_LEVEL);

#define SPI_HEADER_LEN           5
#define SPI_HEADER_RESET_FLAG    0x80
#define SPI_HEADER_CRC_FLAG      0x40
#define SPI_HEADER_PATTERN_VALUE 0x02
#define SPI_HEADER_PATTERN_MASK  0x03

#define HDLC_BYTE_FLAG   0x7E
#define HDLC_BYTE_ESC    0x7D
#define HDLC_BYTE_XON    0x11
#define HDLC_BYTE_XOFF   0x13
#define HDLC_BYTE_VENDOR 0xF8
#define HDLC_ESC_XOR     0x20

#define FCS_RESET 0xffff
#define FCS_CHECK 0xf0b8

#define BUFFER_SIZE                                                                                \
	(SPI_HEADER_LEN + CONFIG_HDLC_RCP_IF_SPI_MAX_FRAME_SIZE +                                  \
	 CONFIG_HDLC_RCP_IF_SPI_ALIGN_ALLOWANCE)

struct hdlc_rcp_if_spi_config {
	struct spi_dt_spec bus;
	struct gpio_dt_spec int_gpio;
	struct gpio_dt_spec rst_gpio;
	uint16_t reset_time;
	uint16_t reset_delay;
};

struct hdlc_rcp_if_spi_data {
	const struct device *self;
	struct k_work work;

	struct gpio_callback int_gpio_cb;
	hdlc_rx_callback_t rx_cb;
	void *rx_param;

	uint8_t rx_buf[BUFFER_SIZE];
	uint16_t rx_len;
	uint8_t tx_buf[BUFFER_SIZE];
	uint16_t tx_len;
	bool tx_escaped;
	uint16_t tx_fcs;

	bool tx_ready;
	bool frame_sent;
};

BUILD_ASSERT(CONFIG_HDLC_RCP_IF_SPI_SMALL_PACKET_SIZE <=
		     CONFIG_HDLC_RCP_IF_SPI_MAX_FRAME_SIZE - SPI_HEADER_LEN,
	     "HDLC IF SPI small packet size larger than maximum frame size");

static bool hdlc_byte_needs_escape(uint8_t byte)
{
	switch (byte) {
	case HDLC_BYTE_VENDOR:
	case HDLC_BYTE_ESC:
	case HDLC_BYTE_FLAG:
	case HDLC_BYTE_XOFF:
	case HDLC_BYTE_XON:
		return true;
	default:
		break;
	}

	return false;
}

static int hdlc_rcp_if_spi_reset(const struct device *dev)
{
	const struct hdlc_rcp_if_spi_config *cfg = dev->config;

	if (cfg->rst_gpio.port != NULL) {
		int ret;

		if (!gpio_is_ready_dt(&cfg->rst_gpio)) {
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->rst_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}

		k_msleep(cfg->reset_time);

		ret = gpio_pin_set_dt(&cfg->rst_gpio, 0);
		if (ret < 0) {
			return ret;
		}

		k_msleep(cfg->reset_delay);
	}

	return 0;
}

static void hdlc_rcp_if_rx_cb(struct hdlc_rcp_if_spi_data *data, const uint8_t *buf, uint16_t len)
{
	uint8_t esc_buf[] = {HDLC_BYTE_ESC, 0x00};
	uint16_t idx = 0;

	/* Optimize a bit by sending un-escaped chunks at once */
	while (len > 0 && idx < len) {
		if (!hdlc_byte_needs_escape(buf[idx])) {
			idx++;
			continue;
		}

		if (idx > 0) {
			data->rx_cb(buf, idx, data->rx_param);
		}

		esc_buf[1] = buf[idx] ^ HDLC_ESC_XOR;
		data->rx_cb(esc_buf, sizeof(esc_buf), data->rx_param);

		len -= idx + 1;
		buf += idx + 1;
		idx = 0;
	}

	/* Remainder chunk */
	if (len > 0) {
		data->rx_cb(buf, len, data->rx_param);
	}
}

static void hdlc_rcp_if_push_pull_spi(struct k_work *work)
{
	struct hdlc_rcp_if_spi_data *data = CONTAINER_OF(work, struct hdlc_rcp_if_spi_data, work);
	const struct hdlc_rcp_if_spi_config *cfg = data->self->config;
	struct spi_buf tx_frame = {
		.buf = data->tx_buf,
		.len = data->tx_len + SPI_HEADER_LEN,
	};
	const struct spi_buf_set tx_set = {
		.buffers = &tx_frame,
		.count = 1U,
	};
	struct spi_buf rx_frame = {
		.buf = data->rx_buf,
		.len = SPI_HEADER_LEN + CONFIG_HDLC_RCP_IF_SPI_ALIGN_ALLOWANCE,
	};
	const struct spi_buf_set rx_set = {
		.buffers = &rx_frame,
		.count = 1U,
	};
	uint8_t *rx_buf;
	uint16_t peer_max_rx;
	uint16_t fcs;
	int ret;

	data->tx_buf[0] = SPI_HEADER_PATTERN_VALUE;
	if (!data->frame_sent) {
		data->tx_buf[0] |= SPI_HEADER_RESET_FLAG;
	}

	sys_put_le16(0U, &data->tx_buf[1]);
	sys_put_le16(data->tx_len, &data->tx_buf[3]);

	if (data->rx_len > 0U) {
		rx_frame.len += data->rx_len;
		sys_put_le16(data->rx_len, &data->tx_buf[1]);
	} else {
		rx_frame.len += CONFIG_HDLC_RCP_IF_SPI_SMALL_PACKET_SIZE;
		sys_put_le16(CONFIG_HDLC_RCP_IF_SPI_SMALL_PACKET_SIZE, &data->tx_buf[1]);
	}

	LOG_HEXDUMP_DBG(data->tx_buf, SPI_HEADER_LEN, "TX Header");
	LOG_HEXDUMP_DBG(&data->tx_buf[SPI_HEADER_LEN], data->tx_len, "TX Data");

	ret = spi_transceive_dt(&cfg->bus, &tx_set, &rx_set);
	if (ret < 0) {
		LOG_ERR("Failed to push/pull frames (%d)", ret);
		goto end;
	}

	/* Find the real start of the frame */
	rx_buf = data->rx_buf;
	for (uint8_t i = 0U; i < CONFIG_HDLC_RCP_IF_SPI_ALIGN_ALLOWANCE && rx_frame.len > 0U; ++i) {
		if (rx_buf[0] != 0xff) {
			break;
		}
		rx_buf++;
		rx_frame.len--;
	}

	if ((rx_buf[0] & SPI_HEADER_PATTERN_MASK) != SPI_HEADER_PATTERN_VALUE) {
		LOG_HEXDUMP_WRN(rx_buf, SPI_HEADER_LEN, "Invalid header data");
		goto end;
	}

	data->frame_sent = true;

	peer_max_rx = sys_get_le16(&rx_buf[1]);
	data->rx_len = sys_get_le16(&rx_buf[3]);

	if (data->tx_len > peer_max_rx) {
		LOG_WRN("Peer not ready to receive our frame (%u > %u)", data->tx_len, peer_max_rx);
	}

	LOG_HEXDUMP_DBG(rx_buf, SPI_HEADER_LEN, "RX Header");

	if (data->rx_len + SPI_HEADER_LEN > rx_frame.len || data->rx_len == 0U) {
		/* Frame empty or incomplete */
		goto end;
	}

	LOG_HEXDUMP_DBG(&rx_buf[SPI_HEADER_LEN], data->rx_len, "RX Data");

	if (peer_max_rx > CONFIG_HDLC_RCP_IF_SPI_MAX_FRAME_SIZE ||
	    data->rx_len > CONFIG_HDLC_RCP_IF_SPI_MAX_FRAME_SIZE) {
		LOG_HEXDUMP_WRN(rx_buf, SPI_HEADER_LEN, "Invalid accept/data lengths");
		data->rx_len = 0;
		goto end;
	}

	if ((rx_buf[0] & SPI_HEADER_RESET_FLAG) != 0U) {
		LOG_DBG("Peer did reset");
		if (data->rx_cb != NULL) {
			uint8_t rst_buf[] = {HDLC_BYTE_FLAG, 0x13, 0x11, HDLC_BYTE_FLAG};

			data->rx_cb(rst_buf, sizeof(rst_buf), data->rx_param);
		}
	}

	fcs = crc16_ccitt(FCS_RESET, &rx_buf[SPI_HEADER_LEN], data->rx_len);
	fcs ^= FCS_RESET;

	if ((rx_buf[0] & SPI_HEADER_CRC_FLAG) != 0U) {
		if (fcs != sys_get_le16(&rx_buf[SPI_HEADER_LEN + data->rx_len])) {
			LOG_WRN("Invalid CRC");
			data->rx_len = 0U;
			goto end;
		}
	}

	if (data->rx_cb != NULL) {
		uint8_t tmp[2] = {HDLC_BYTE_FLAG, 0x00};

		/* Start HDLC */
		data->rx_cb(tmp, 1, data->rx_param);

		hdlc_rcp_if_rx_cb(data, &rx_buf[SPI_HEADER_LEN], data->rx_len);

		sys_put_le16(fcs, tmp);
		hdlc_rcp_if_rx_cb(data, tmp, sizeof(tmp));

		/* End HDLC */
		tmp[0] = HDLC_BYTE_FLAG;
		data->rx_cb(tmp, 1, data->rx_param);
	}

	data->rx_len = 0;

end:
	data->tx_ready = true;
	data->tx_len = 0;
}

static void hdlc_rcp_if_spi_isr(const struct device *port, struct gpio_callback *cb,
				gpio_port_pins_t pins)
{
	struct hdlc_rcp_if_spi_data *data =
		CONTAINER_OF(cb, struct hdlc_rcp_if_spi_data, int_gpio_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_work_submit(&data->work);
}

static void hdlc_iface_init(struct net_if *iface)
{
	otExtAddress eui64;

	__ASSERT_NO_MSG(DEVICE_DT_INST_GET(0) == net_if_get_device(iface));

	ieee802154_init(iface);

	otPlatRadioGetIeeeEui64(openthread_get_default_instance(), eui64.m8);
	net_if_set_link_addr(iface, eui64.m8, OT_EXT_ADDRESS_SIZE, NET_LINK_IEEE802154);
}

static int hdlc_register_rx_cb(hdlc_rx_callback_t hdlc_rx_callback, void *param)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	struct hdlc_rcp_if_spi_data *data = dev->data;

	data->rx_cb = hdlc_rx_callback;
	data->rx_param = param;

	return 0;
}

static int hdlc_send(const uint8_t *frame, uint16_t length)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	struct hdlc_rcp_if_spi_data *data = dev->data;

	if (frame == NULL) {
		return -EINVAL;
	}

	if (!data->tx_ready) {
		return -EBUSY;
	}

	for (uint16_t i = 0; i < length; ++i) {
		uint8_t byte = frame[i];

		if (data->tx_len >= BUFFER_SIZE - SPI_HEADER_LEN) {
			data->tx_escaped = false;
			data->tx_len = 0;
			data->tx_fcs = FCS_RESET;
			return -ENOMEM;
		}

		if (byte == HDLC_BYTE_FLAG) {
			if (data->tx_len <= 2) {
				/* Start of frame */
				data->tx_escaped = false;
				data->tx_len = 0;
				data->tx_fcs = FCS_RESET;
				continue;
			}
			if (data->tx_fcs != FCS_CHECK) {
				LOG_ERR("Invalid HDLC CRC 0x%04x for length %u", data->tx_fcs,
					data->tx_len);
				data->tx_escaped = false;
				data->tx_len = 0;
				data->tx_fcs = FCS_RESET;
				continue;
			}

			if (i != length - 1) {
				LOG_WRN("Dropped %d bytes", length - i);
			}

			/* Frame ready (strip CRC length) */
			data->tx_len -= 2U;
			data->tx_ready = false;

			/* Reset for next */
			data->tx_fcs = FCS_RESET;
			data->tx_escaped = false;
			break;
		}

		if (byte == HDLC_BYTE_ESC) {
			data->tx_escaped = true;
			continue;
		}

		if (hdlc_byte_needs_escape(byte)) {
			continue;
		}

		if (data->tx_escaped) {
			byte ^= HDLC_ESC_XOR;
			data->tx_escaped = false;
		}

		data->tx_fcs = crc16_ccitt(data->tx_fcs, &byte, 1);
		data->tx_buf[SPI_HEADER_LEN + data->tx_len++] = byte;
	}

	k_work_submit(&data->work);

	return 0;
}

static int hdlc_deinit(void)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	struct hdlc_rcp_if_spi_data *data = dev->data;

	data->frame_sent = false;

	return 0;
}

static int hdlc_rcp_if_spi_init(const struct device *dev)
{
	const struct hdlc_rcp_if_spi_config *cfg = dev->config;
	struct hdlc_rcp_if_spi_data *data = dev->data;
	int ret;

	data->self = dev;
	data->tx_ready = true;
	k_work_init(&data->work, hdlc_rcp_if_push_pull_spi);

	if (!spi_is_ready_dt(&cfg->bus)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&cfg->int_gpio)) {
		LOG_ERR("Interrupt GPIO not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure interrupt GPIO pin (%d)", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure interrupt GPIO (%d)", ret);
		return ret;
	}

	gpio_init_callback(&data->int_gpio_cb, hdlc_rcp_if_spi_isr, BIT(cfg->int_gpio.pin));

	ret = gpio_add_callback(cfg->int_gpio.port, &data->int_gpio_cb);
	if (ret < 0) {
		LOG_ERR("Failed to add interrupt GPIO callback (%d)", ret);
		return ret;
	}

	ret = hdlc_rcp_if_spi_reset(dev);
	if (ret < 0) {
		LOG_ERR("Failed to reset HDLC SPI device (%d)", ret);
	}

	return 0;
}

static const struct hdlc_api spi_hdlc_api = {
	.iface_api.init = hdlc_iface_init,
	.register_rx_cb = hdlc_register_rx_cb,
	.send = hdlc_send,
	.deinit = hdlc_deinit,
};

#define L2_CTX_TYPE NET_L2_GET_CTX_TYPE(OPENTHREAD_L2)

#define MTU 1280

static const struct hdlc_rcp_if_spi_config ot_hdlc_rcp_cfg = {
	.bus = SPI_DT_SPEC_INST_GET(0, SPI_OP_MODE_MASTER | SPI_WORD_SET(8),
				    DT_INST_PROP(0, cs_delay)),
	.int_gpio = GPIO_DT_SPEC_INST_GET(0, int_gpios),
	.rst_gpio = GPIO_DT_SPEC_INST_GET_OR(0, reset_gpios, {}),
	.reset_time = DT_INST_PROP(0, reset_assert_time),
	.reset_delay = DT_INST_PROP(0, reset_delay),
};

static struct hdlc_rcp_if_spi_data ot_hdlc_rcp_data;

NET_DEVICE_DT_INST_DEFINE(0, hdlc_rcp_if_spi_init, NULL, &ot_hdlc_rcp_data, &ot_hdlc_rcp_cfg,
			  CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &spi_hdlc_api, OPENTHREAD_L2,
			  NET_L2_GET_CTX_TYPE(OPENTHREAD_L2), MTU);
