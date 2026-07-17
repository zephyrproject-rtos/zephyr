/*
 * Copyright (c) 2026 Luke Bugbee <lbugbee@protonmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_rc522

#include <zephyr/drivers/nfc.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(rc522);

/* Command & Status Registers */
#define RC522_REG_CMD        (0x01)
#define RC522_REG_COM_ISR_EN (0x02)
#define RC522_REG_DIV_ISR_EN (0x03)
#define RC522_REG_COM_IRQ    (0x04)
#define RC522_REG_DIV_IRQ    (0x05)
#define RC522_REG_ERR        (0x06)
#define RC522_REG_STAT_1     (0x07)
#define RC522_REG_STAT_2     (0x08)
#define RC522_REG_FIFO_DATA  (0x09)
#define RC522_REG_FIFO_LVL   (0x0A)
#define RC522_REG_WTR_LVL    (0x0B)
#define RC522_REG_CTRL       (0x0C)
#define RC522_REG_BIT_FRAME  (0x0D)
#define RC522_REG_BIT_COLL   (0x0E)
#define RC522_REG_MODE       (0x11)
#define RC522_REG_TX_MODE    (0x12)
#define RC522_REG_RX_MODE    (0x13)
#define RC522_REG_TX_CTRL    (0x14)
#define RC522_REG_TX_ASK     (0x15)
#define RC522_REG_TX_SEL     (0x16)
#define RC522_REG_RX_SEL     (0x17)
#define RC522_REG_RX_THRESH  (0x18)
#define RC522_REG_DEMOD      (0x19)
#define RC522_REG_MF_TX      (0x1C)
#define RC522_REG_MF_RX      (0x1D)
#define RC522_REG_SER_SPD    (0x1F)

/* Configuration Registers */
#define RC522_REG_CRC_RES_H  (0x21)
#define RC522_REG_CRC_RES_L  (0x22)
#define RC522_REG_MOD_WID    (0x24)
#define RC522_REG_RF_CFG     (0x26)
#define RC522_REG_GS_N       (0x27)
#define RC522_REG_CWGSP      (0x28)
#define RC522_REG_MOD_GSP    (0x29)
#define RC522_REG_T_MODE     (0x2A)
#define RC522_REG_T_PRESCAL  (0x2B)
#define RC522_REG_T_RELOAD_H (0x2C)
#define RC522_REG_T_RELOAD_L (0x2D)
#define RC522_REG_T_CNT_H    (0x2E)
#define RC522_REG_T_CNT_L    (0x2F)

/* Test Registers */
#define RC522_REG_TEST_SEL_1   (0x31)
#define RC522_REG_TEST_SEL_2   (0x32)
#define RC522_REG_TEST_PIN_EN  (0x33)
#define RC522_REG_TEST_PIN_VAL (0x34)
#define RC522_REG_TEST_BUS     (0x35)
#define RC522_REG_AUTO_TEST    (0x36)
#define RC522_REG_VERSION      (0x37)
#define RC522_REG_TEST_ANALOG  (0x38)
#define RC522_REG_TEST_DAC_1   (0x39)
#define RC522_REG_TEST_DAC_2   (0x3A)
#define RC522_REG_TEST_ADC     (0x3B)

#define RC522_FIFO_LVL_FLUSH_BUF (0x80)
#define RC522_DIV_IRQ_CLR_IRQ    (0x04)
#define RC522_COM_IRQ_CLR_IRQ    (0x7F)

#define SPI_READ            (BIT(7))
#define REG_SPI_OFFSET(reg) ((reg) << 1)

#define RC522_MILLER_MOD_WIDTH (0x26)

/* 25 ms */
#define TIMER_PRESCALER (0x0A9)
#define TIMER_RELOAD    (0x03E8)

#define RX_TIMEOUT_MS (36)

enum rc522_err_reg {
	ERR_REG_PRO = BIT(0),
	ERR_REG_PAR = BIT(1),
	ERR_REG_CRC = BIT(2),
	ERR_REG_COL = BIT(3),
	ERR_REG_OVF = BIT(4),
	ERR_REG_TMP = BIT(6),
	ERR_REG_WR = BIT(7)
};

#define BIT_FRAME_RX_ALIGN_MASK_SHIFT 4
enum rc522_bit_frame_reg {
	BIT_FRAME_TX_LAST_BITS_MASK = BIT_MASK(3),
	BIT_FRAME_RX_ALIGN_MASK = (BIT_MASK(3) << BIT_FRAME_RX_ALIGN_MASK_SHIFT),
	BIT_FRAME_START_SEND = BIT(7)
};

enum rc522_ctrl_reg {
	CTRL_REG_RX_LAST_BITS_MASK = BIT_MASK(3),
	CTRL_REG_T_STRT = BIT(6),
	CTRL_REG_T_STOP = BIT(7)
};

enum rc522_mode_reg {
	MODE_REG_CRC_PRESET_MASK = BIT_MASK(2),
	MODE_REG_POL_MFIN = BIT(3),
	MODE_REG_TX_WAIT_RF = BIT(5),
	MODE_REG_MSB_FIRST = BIT(7)
};

enum rc522_crc_preset {
	CRC_PRESET_0000 = 0x0,
	CRC_PRESET_6363 = 0x1,
	CRC_PRESET_A671 = 0x2,
	CRC_PRESET_FFFF = 0x3
};

#define VER_REG_CHIP_TYPE_MASK_SHIFT 4
enum rc522_version_reg {
	VER_REG_VER_MASK = (BIT_MASK(4)),
	VER_REG_CHIP_TYPE_MASK = (BIT_MASK(4) << VER_REG_CHIP_TYPE_MASK_SHIFT)
};

#define TX_MODE_SPEED_MASK_SHIFT 4
enum rc522_tx_mode_reg {
	TX_MODE_SPEED_MASK = (BIT_MASK(3) << TX_MODE_SPEED_MASK_SHIFT),
	TX_MODE_INV_MOD = BIT(3),
	TX_MODE_CRC_EN = BIT(7)
};

#define RX_MODE_SPEED_MASK_SHIFT 4
enum rc522_rx_mode_reg {
	RX_MODE_SPEED_MASK = (BIT_MASK(3) << RX_MODE_SPEED_MASK_SHIFT),
	RX_MODE_INV_MOD = BIT(3),
	RX_MODE_CRC_EN = BIT(7)
};

enum rc522_tx_ctrl_reg {
	TX_CTRL_REG_TX1_EN = BIT(0),
	TX_CTRL_REG_TX2_EN = BIT(1),
	TX_CTRL_REG_TX2_CW = BIT(3),
	TX_CTRL_REG_TX1_INV_OFF = BIT(4),
	TX_CTRL_REG_TX2_INV_OFF = BIT(5),
	TX_CTRL_REG_TX1_INV_ON = BIT(6),
	TX_CTRL_REG_TX2_INV_ON = BIT(7)
};

enum rc522_tx_ask_reg {
	TX_ASK_REG_FORCE_100 = BIT(6)
};

enum rc522_rx_align {
	RX_ALIGN_LSB_0 = 0x0,
	RX_ALIGN_LSB_1 = 0x1,
	RX_ALIGN_LSB_7 = 0x7
};

#define T_MODE_REG_GATED_MASK_SHIFT 5
enum rc522_t_mode_reg {
	T_MODE_REG_PRESCAL_HI_MASK = BIT_MASK(4),
	T_MODE_REG_AUTO_RESTART = BIT(4),
	T_MODE_REG_GATED_MASK = (BIT_MASK(2) << T_MODE_REG_GATED_MASK_SHIFT),
	T_MODE_REG_AUTO = BIT(7)
};

enum rc522_t_prescaler_reg {
	T_MODE_REG_PRESCAL_LO_MASK = BIT_MASK(8)
};

enum rc522_com_irq_reg {
	COM_IRQ_TIMER = BIT(0),
	COM_IRQ_ERR = BIT(1),
	COM_IRQ_LO_ALERT = BIT(2),
	COM_IRQ_HI_ALERT = BIT(3),
	COM_IRQ_IDLE = BIT(4),
	COM_IRQ_RX = BIT(5),
	COM_IRQ_TX = BIT(6),
	COM_IRQ_SET = BIT(7)
};

enum rc522_div_irq_reg {
	DIV_IRQ_CRC = BIT(2),
	DIV_IRQ_MFIN = BIT(4),
	DIV_IRQ_SET = BIT(7)
};

enum rc522_operation_commands {
	OP_CMD_IDLE = 0x0,
	OP_CMD_MEM = 0x1,
	OP_CMD_GEN_RAND_ID = 0x2,
	OP_CMD_CALC_CRC = 0x3,
	OP_CMD_TRANSMIT = 0x4,
	OP_CMD_NO_CMD = 0x7,
	OP_CMD_RECEIVE = 0x8,
	OP_CMD_TRANSCEIVE = 0xC,
	OP_CMD_MF_AUTH = 0xE,
	OP_CMD_SOFT_RST = 0xF
};

enum rc522_bit_rate {
	BIT_RATE_106K = 0x0,
	BIT_RATE_212K = 0x1,
	BIT_RATE_424K = 0x2,
	BIT_RATE_848K = 0x3
};

struct rc522_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec rst;
};

struct rc522_data {
	uint8_t chip_type;
	uint8_t version;
	enum rc522_bit_rate bit_rate;
};

static int rc522_write_reg(const struct device *dev, const uint8_t reg, const uint8_t value)
{
	const struct rc522_config *config = dev->config;
	uint8_t tx_buf[2] = {REG_SPI_OFFSET(reg), value};
	struct spi_buf send_buf = {.buf = tx_buf, .len = ARRAY_SIZE(tx_buf)};
	struct spi_buf_set send_buf_set = {.buffers = &send_buf, .count = 1};
	int ret;

	ret = spi_write_dt(&config->spi, &send_buf_set);
	if (ret != 0) {
		LOG_ERR("failed to write spi (%d)", ret);
		return ret;
	}

	return 0;
}

static int rc522_write_reg_multiple(const struct device *dev, const uint8_t reg, const int length,
				    const uint8_t *values)
{
	const struct rc522_config *config = dev->config;
	uint8_t tx_buf[length + 1];
	int ret;

	tx_buf[0] = REG_SPI_OFFSET(reg);
	memset(tx_buf + 1, 0, length);
	memcpy(tx_buf + 1, values, length);
	struct spi_buf send_buf = {.buf = tx_buf, .len = ARRAY_SIZE(tx_buf)};
	struct spi_buf_set send_buf_set = {.buffers = &send_buf, .count = 1};

	LOG_DBG("writing reg 0x%02X", reg);
	LOG_HEXDUMP_DBG(values, length, "");

	ret = spi_write_dt(&config->spi, &send_buf_set);
	if (ret != 0) {
		LOG_ERR("failed to write spi mult (%d)", ret);
		return ret;
	}

	return 0;
}

static int rc522_read_reg(const struct device *dev, const uint8_t reg, uint8_t *value)
{
	const struct rc522_config *config = dev->config;
	uint8_t tx_buf[2] = {SPI_READ | REG_SPI_OFFSET(reg), 0};
	struct spi_buf send_buf = {.buf = tx_buf, .len = ARRAY_SIZE(tx_buf)};
	struct spi_buf_set send_buf_set = {.buffers = &send_buf, .count = 1};

	uint8_t rx_buf[2];
	struct spi_buf receive_buf = {.buf = rx_buf, .len = 2};
	struct spi_buf_set receive_buf_set = {.buffers = &receive_buf, .count = 1};
	int ret;

	ret = spi_transceive_dt(&config->spi, &send_buf_set, &receive_buf_set);
	if (ret < 0) {
		LOG_ERR("failed to read reg 0x%02X (%d)", reg, ret);
		return ret;
	}

	*value = rx_buf[1];

	LOG_DBG("read reg 0x%02X value 0x%02X", reg, *value);

	return 0;
}

static int rc522_read_reg_multiple(const struct device *dev, const uint8_t reg, size_t count,
				   uint8_t *values)
{
	const struct rc522_config *config = dev->config;

	uint8_t tx_buf[count + 1];
	struct spi_buf send_buf = {tx_buf, count + 1};
	struct spi_buf_set send_buf_set = {&send_buf, 1};

	uint8_t rx_buf[count + 1];
	struct spi_buf receive_buf = {rx_buf, count + 1};
	struct spi_buf_set receive_buf_set = {&receive_buf, 1};
	int ret;

	if (count == 0) {
		return 0;
	}

	memset(tx_buf, SPI_READ | REG_SPI_OFFSET(reg), count);
	tx_buf[count] = 0;

	ret = spi_transceive_dt(&config->spi, &send_buf_set, &receive_buf_set);
	if (ret < 0) {
		LOG_ERR("failed to read reg 0x%02X (%d)", reg, ret);
		return ret;
	}

	memcpy(values, rx_buf + 1, count);

	return count;
}

K_TIMER_DEFINE(rx_timer, NULL, NULL);

static int rc522_transceive(const struct device *dev, struct nfc_comm *comm)
{
	bool rx_done = false;
	uint8_t reg = 0;
	int ret = 0;

	/* transmit */
	rc522_write_reg(dev, RC522_REG_CMD, OP_CMD_IDLE);
	rc522_write_reg(dev, RC522_REG_COM_IRQ, RC522_COM_IRQ_CLR_IRQ);
	rc522_write_reg(dev, RC522_REG_FIFO_LVL, RC522_FIFO_LVL_FLUSH_BUF);
	rc522_write_reg_multiple(dev, RC522_REG_FIFO_DATA, comm->tx_buf->len, comm->tx_buf->data);
	rc522_write_reg(dev, RC522_REG_CMD, OP_CMD_TRANSCEIVE);
	rc522_write_reg(dev, RC522_REG_BIT_FRAME,
			(RX_ALIGN_LSB_0 << BIT_FRAME_RX_ALIGN_MASK_SHIFT) |
				(BIT_FRAME_TX_LAST_BITS_MASK & comm->valid_bits) |
				BIT_FRAME_START_SEND);

	/* receive */
	k_timer_start(&rx_timer, K_MSEC(RX_TIMEOUT_MS), K_NO_WAIT);
	do {
		reg = 0;
		rc522_read_reg(dev, RC522_REG_COM_IRQ, &reg);
		if (reg & COM_IRQ_RX) {
			rx_done = true;
			break;
		}

		if (reg & COM_IRQ_TIMER) {
			LOG_DBG("irq timeout");
			return -ETIMEDOUT;
		}

		k_yield();
	} while (k_timer_status_get(&rx_timer) <= 0 && k_timer_remaining_get(&rx_timer) != 0);

	k_timer_stop(&rx_timer);

	if (!rx_done) {
		LOG_DBG("rx timed out");
		return -ETIMEDOUT;
	}

	reg = 0;
	rc522_read_reg(dev, RC522_REG_ERR, &reg);
	if (reg & (ERR_REG_PRO | ERR_REG_PAR | ERR_REG_COL | ERR_REG_OVF)) {
		LOG_ERR("rx error (%X)", reg);
		return -EBADMSG;
	}

	reg = 0;
	rc522_read_reg(dev, RC522_REG_FIFO_LVL, &reg);
	if (reg > comm->rx_buf->size) {
		LOG_ERR("rx fifo overflows rx buf %d > %d", reg, comm->rx_buf->size);
		return -ENOBUFS;
	}

	ret = rc522_read_reg_multiple(dev, RC522_REG_FIFO_DATA, reg, comm->rx_buf->data);
	if (ret < 0) {
		LOG_ERR("rx error reading fifo (%d)", ret);
		return ret;
	}

	comm->rx_buf->len = ret;

	reg = 0;
	rc522_read_reg(dev, RC522_REG_CTRL, &reg);
	reg &= CTRL_REG_RX_LAST_BITS_MASK;

	if (comm->rx_buf->len == 1 && reg == 4) {
		LOG_ERR("rx nack");
		return -EBADMSG;
	}

	if (reg != 0) {
		LOG_ERR("rx invalid bits (%X)", reg);
		return -EBADMSG;
	}

	return comm->rx_buf->len;
}

static int rc522_get_version(const struct device *dev)
{
	struct rc522_data *data = dev->data;
	uint8_t reg = 0;

	rc522_read_reg(dev, RC522_REG_VERSION, &reg);
	data->chip_type = (reg & VER_REG_CHIP_TYPE_MASK) >> VER_REG_CHIP_TYPE_MASK_SHIFT;
	data->version = reg & VER_REG_VER_MASK;

	return 0;
}

/* timeout (secs) = ((prescaler x 2 + 1) x (reload + 1)) / 13.56 MHz */
static int rc522_set_timeout(const struct device *dev, uint16_t prescaler, uint16_t reload)
{
	/* prescaler is 12 bits. high is only 4 bits */
	uint8_t prescaler_hi = (prescaler >> 8) & BIT_MASK(4);
	uint8_t prescaler_lo = prescaler & BIT_MASK(8);
	uint8_t reg = 0;

	rc522_read_reg(dev, RC522_REG_T_MODE, &reg);
	rc522_write_reg(dev, RC522_REG_T_MODE, reg | (T_MODE_REG_PRESCAL_HI_MASK & prescaler_hi));
	rc522_write_reg(dev, RC522_REG_T_PRESCAL, prescaler_lo);
	rc522_write_reg(dev, RC522_REG_T_RELOAD_H, reload >> 8);
	rc522_write_reg(dev, RC522_REG_T_RELOAD_L, reload & BIT_MASK(8));

	LOG_INF("timeout set to %d us", (((prescaler * 2 + 1) * (reload + 1)) / 13560) * 1000);

	return 0;
}

static int rc522_antenna_enable(const struct device *dev, bool enable)
{
	uint8_t reg = 0;

	LOG_INF("%s rf", enable ? "enabling" : "disabling");

	rc522_read_reg(dev, RC522_REG_TX_CTRL, &reg);

	if (enable) {
		rc522_write_reg(dev, RC522_REG_TX_CTRL,
				reg | (TX_CTRL_REG_TX1_EN | TX_CTRL_REG_TX2_EN));
	} else {
		rc522_write_reg(dev, RC522_REG_TX_CTRL,
				reg & ~(TX_CTRL_REG_TX1_EN | TX_CTRL_REG_TX2_EN));
	}

	return 0;
}

static int rc522_set_bitrate(const struct device *dev, enum rc522_bit_rate bit_rate)
{
	rc522_write_reg(dev, RC522_REG_TX_MODE, (TX_MODE_SPEED_MASK & bit_rate));
	rc522_write_reg(dev, RC522_REG_RX_MODE, (RX_MODE_SPEED_MASK & bit_rate));

	return 0;
}

static void rc522_reset(const struct device *dev)
{
	const struct rc522_config *config = dev->config;

	gpio_pin_set_dt(&config->rst, 1);
	k_usleep(1); /* min 10 ns */
	gpio_pin_set_dt(&config->rst, 0);
	k_usleep(100); /* min 38 us */

	rc522_write_reg(dev, RC522_REG_CMD, OP_CMD_SOFT_RST);
}

int rc522_init(const struct device *dev)
{
	const struct rc522_config *config = dev->config;
	struct rc522_data *data = dev->data;
	int ret;

	if (!spi_is_ready_dt(&config->spi)) {
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&config->rst)) {
		LOG_ERR("reset gpio is not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->rst, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		LOG_ERR("failed to configure reset gpio (%d)", ret);
		return ret;
	}

	rc522_reset(dev);

	rc522_get_version(dev);

	rc522_set_bitrate(dev, data->bit_rate);

	rc522_write_reg(dev, RC522_REG_MOD_WID, RC522_MILLER_MOD_WIDTH);

	rc522_write_reg(dev, RC522_REG_T_MODE, T_MODE_REG_AUTO);
	rc522_set_timeout(dev, TIMER_PRESCALER, TIMER_RELOAD);

	rc522_write_reg(dev, RC522_REG_TX_ASK, TX_ASK_REG_FORCE_100);
	rc522_write_reg(dev, RC522_REG_MODE, MODE_REG_TX_WAIT_RF);

	LOG_INF("chip type 0x%X version %X", data->chip_type, data->version);

	return 0;
}

static DEVICE_API(nfc, rc522_api) = {
	.enable = rc522_antenna_enable,
	.transceive = rc522_transceive,
};

#define RC522_DEVICE(n)                                                                            \
	static struct rc522_config rc522_config_##n = {                                            \
		.spi = SPI_DT_SPEC_INST_GET(n, SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB |             \
						       SPI_WORD_SET(8) | SPI_LINES_SINGLE),        \
		.rst = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),                              \
	};                                                                                         \
                                                                                                   \
	static struct rc522_data rc522_data_##n = {                                                \
		.chip_type = 0,                                                                    \
		.version = 0,                                                                      \
		.bit_rate = BIT_RATE_106K,                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &rc522_init, NULL, &rc522_data_##n, &rc522_config_##n,            \
			      POST_KERNEL, CONFIG_NFC_INIT_PRIORITY, &rc522_api);

DT_INST_FOREACH_STATUS_OKAY(RC522_DEVICE)
