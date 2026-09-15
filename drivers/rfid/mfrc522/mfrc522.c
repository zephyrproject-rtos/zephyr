/*
 * Copyright (c) 2026 Luke Bugbee <lbugbee@protonmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_mfrc522

#include <zephyr/drivers/rfid/mfrc522.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mfrc522, CONFIG_RFID_LOG_LEVEL);

/* Command & Status Registers */
#define MFRC522_REG_CMD        0x01
#define MFRC522_REG_COM_ISR_EN 0x02
#define MFRC522_REG_DIV_ISR_EN 0x03
#define MFRC522_REG_COM_IRQ    0x04
#define MFRC522_REG_DIV_IRQ    0x05
#define MFRC522_REG_ERR        0x06
#define MFRC522_REG_STAT_1     0x07
#define MFRC522_REG_STAT_2     0x08
#define MFRC522_REG_FIFO_DATA  0x09
#define MFRC522_REG_FIFO_LVL   0x0A
#define MFRC522_REG_WTR_LVL    0x0B
#define MFRC522_REG_CTRL       0x0C
#define MFRC522_REG_BIT_FRAME  0x0D
#define MFRC522_REG_BIT_COLL   0x0E
#define MFRC522_REG_MODE       0x11
#define MFRC522_REG_TX_MODE    0x12
#define MFRC522_REG_RX_MODE    0x13
#define MFRC522_REG_TX_CTRL    0x14
#define MFRC522_REG_TX_ASK     0x15
#define MFRC522_REG_TX_SEL     0x16
#define MFRC522_REG_RX_SEL     0x17
#define MFRC522_REG_RX_THRESH  0x18
#define MFRC522_REG_DEMOD      0x19
#define MFRC522_REG_MF_TX      0x1C
#define MFRC522_REG_MF_RX      0x1D
#define MFRC522_REG_SER_SPD    0x1F

/* Configuration Registers */
#define MFRC522_REG_CRC_RES_H  0x21
#define MFRC522_REG_CRC_RES_L  0x22
#define MFRC522_REG_MOD_WID    0x24
#define MFRC522_REG_RF_CFG     0x26
#define MFRC522_REG_GS_N       0x27
#define MFRC522_REG_CWGSP      0x28
#define MFRC522_REG_MOD_GSP    0x29
#define MFRC522_REG_T_MODE     0x2A
#define MFRC522_REG_T_PRESCAL  0x2B
#define MFRC522_REG_T_RELOAD_H 0x2C
#define MFRC522_REG_T_RELOAD_L 0x2D
#define MFRC522_REG_T_CNT_H    0x2E
#define MFRC522_REG_T_CNT_L    0x2F

/* Test Registers */
#define MFRC522_REG_TEST_SEL_1   0x31
#define MFRC522_REG_TEST_SEL_2   0x32
#define MFRC522_REG_TEST_PIN_EN  0x33
#define MFRC522_REG_TEST_PIN_VAL 0x34
#define MFRC522_REG_TEST_BUS     0x35
#define MFRC522_REG_AUTO_TEST    0x36
#define MFRC522_REG_VERSION      0x37
#define MFRC522_REG_TEST_ANALOG  0x38
#define MFRC522_REG_TEST_DAC_1   0x39
#define MFRC522_REG_TEST_DAC_2   0x3A
#define MFRC522_REG_TEST_ADC     0x3B

#define MFRC522_FIFO_LVL_FLUSH_BUF 0x80
#define MFRC522_DIV_IRQ_CLR_IRQ    0x04
#define MFRC522_COM_IRQ_CLR_IRQ    0x7F

#define SPI_READ            BIT(7)
#define REG_SPI_OFFSET(reg) ((reg) << 1)

enum reg_err {
	ERR_REG_PRO = BIT(0),
	ERR_REG_PAR = BIT(1),
	ERR_REG_CRC = BIT(2),
	ERR_REG_COL = BIT(3),
	ERR_REG_OVF = BIT(4),
	ERR_REG_TMP = BIT(6),
	ERR_REG_WR = BIT(7)
};

#define BIT_FRAME_RX_ALIGN_MASK_SHIFT 4
enum reg_bit_frame {
	BIT_FRAME_TX_LAST_BITS_MASK = BIT_MASK(3),
	BIT_FRAME_RX_ALIGN_MASK = (BIT_MASK(3) << BIT_FRAME_RX_ALIGN_MASK_SHIFT),
	BIT_FRAME_START_SEND = BIT(7)
};

enum reg_ctrl {
	CTRL_REG_RX_LAST_BITS_MASK = BIT_MASK(3),
	CTRL_REG_T_STRT = BIT(6),
	CTRL_REG_T_STOP = BIT(7)
};

enum reg_mode {
	MODE_REG_CRC_PRESET_MASK = BIT_MASK(2),
	MODE_REG_POL_MFIN = BIT(3),
	MODE_REG_TX_WAIT_RF = BIT(5),
	MODE_REG_MSB_FIRST = BIT(7)
};

#define VER_REG_CHIP_TYPE_MASK_SHIFT 4
enum reg_version {
	VER_REG_VER_MASK = (BIT_MASK(4)),
	VER_REG_CHIP_TYPE_MASK = (BIT_MASK(4) << VER_REG_CHIP_TYPE_MASK_SHIFT)
};

#define TX_MODE_SPEED_MASK_SHIFT 4
enum reg_tx_mode {
	TX_MODE_SPEED_MASK = (BIT_MASK(3) << TX_MODE_SPEED_MASK_SHIFT),
	TX_MODE_INV_MOD = BIT(3),
	TX_MODE_CRC_EN = BIT(7)
};

#define RX_MODE_SPEED_MASK_SHIFT 4
enum reg_rx_mode {
	RX_MODE_SPEED_MASK = (BIT_MASK(3) << RX_MODE_SPEED_MASK_SHIFT),
	RX_MODE_INV_MOD = BIT(3),
	RX_MODE_CRC_EN = BIT(7)
};

enum reg_tx_ctrl {
	TX_CTRL_REG_TX1_EN = BIT(0),
	TX_CTRL_REG_TX2_EN = BIT(1),
	TX_CTRL_REG_TX2_CW = BIT(3),
	TX_CTRL_REG_TX1_INV_OFF = BIT(4),
	TX_CTRL_REG_TX2_INV_OFF = BIT(5),
	TX_CTRL_REG_TX1_INV_ON = BIT(6),
	TX_CTRL_REG_TX2_INV_ON = BIT(7)
};

enum reg_tx_ask {
	TX_ASK_REG_FORCE_100 = BIT(6)
};

#define T_MODE_REG_GATED_MASK_SHIFT 5
enum reg_t_mode {
	T_MODE_REG_PRESCAL_HI_MASK = BIT_MASK(4),
	T_MODE_REG_AUTO_RESTART = BIT(4),
	T_MODE_REG_GATED_MASK = (BIT_MASK(2) << T_MODE_REG_GATED_MASK_SHIFT),
	T_MODE_REG_AUTO = BIT(7)
};

enum reg_t_prescaler {
	T_MODE_REG_PRESCAL_LO_MASK = BIT_MASK(8)
};

enum reg_com_irq {
	COM_IRQ_TIMER = BIT(0),
	COM_IRQ_ERR = BIT(1),
	COM_IRQ_LO_ALERT = BIT(2),
	COM_IRQ_HI_ALERT = BIT(3),
	COM_IRQ_IDLE = BIT(4),
	COM_IRQ_RX = BIT(5),
	COM_IRQ_TX = BIT(6),
	COM_IRQ_SET = BIT(7)
};

enum reg_div_irq {
	DIV_IRQ_CRC = BIT(2),
	DIV_IRQ_MFIN = BIT(4),
	DIV_IRQ_SET = BIT(7)
};

enum crc_preset {
	CRC_PRESET_0000 = 0x0,
	CRC_PRESET_6363 = 0x1,
	CRC_PRESET_A671 = 0x2,
	CRC_PRESET_FFFF = 0x3
};

enum rx_align {
	RX_ALIGN_LSB_0 = 0x0,
	RX_ALIGN_LSB_1 = 0x1,
	RX_ALIGN_LSB_7 = 0x7
};

enum operation_commands {
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

enum bit_rate {
	BIT_RATE_106K = 0x0,
	BIT_RATE_212K = 0x1,
	BIT_RATE_424K = 0x2,
	BIT_RATE_848K = 0x3
};

struct mfrc522_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec rst;
};

struct mfrc522_data {
	uint8_t chip_type;
	uint8_t version;
	enum bit_rate bit_rate;
	uint8_t miller_mod_width;
	uint16_t timer_prescaler;
	uint16_t timer_reload;
	uint32_t rx_timeout_ms;
	struct k_mutex mutex;
};

static int mfrc522_write_reg(const struct device *dev, const uint8_t reg, const uint8_t value)
{
	int ret;
	const struct mfrc522_config *config = dev->config;

	uint8_t write_buf[2] = {REG_SPI_OFFSET(reg), value};
	struct spi_buf tx_buf = {
		.buf = write_buf,
		.len = ARRAY_SIZE(write_buf),
	};
	struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};

	ret = spi_write_dt(&config->spi, &tx);
	if (ret < 0) {
		LOG_ERR("failed spi write (%d)", ret);
		return ret;
	}

	return 0;
}

static int mfrc522_write_reg_multiple(const struct device *dev, const uint8_t reg, uint8_t *values,
				      const size_t len)
{
	int ret;
	const struct mfrc522_config *config = dev->config;

	uint8_t reg_byte = REG_SPI_OFFSET(reg);
	const struct spi_buf tx_bufs[] = {
		{.buf = &reg_byte, .len = 1},
		{.buf = values, .len = len},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs),
	};

	LOG_DBG("writing reg 0x%02X", reg);
	LOG_HEXDUMP_DBG(values, len, "");

	ret = spi_write_dt(&config->spi, &tx);
	if (ret < 0) {
		LOG_ERR("failed spi write (%d)", ret);
		return ret;
	}

	return 0;
}

static int mfrc522_read_reg(const struct device *dev, const uint8_t reg, uint8_t *value)
{
	int ret;
	const struct mfrc522_config *config = dev->config;

	uint8_t reg_buf[2] = {SPI_READ | REG_SPI_OFFSET(reg), 0};
	struct spi_buf tx_buf = {
		.buf = reg_buf,
		.len = ARRAY_SIZE(reg_buf),
	};
	struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};

	uint8_t read_reg_buf[2];
	struct spi_buf rx_buf = {
		.buf = read_reg_buf,
		.len = 2,
	};
	struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1,
	};

	ret = spi_transceive_dt(&config->spi, &tx, &rx);
	if (ret < 0) {
		LOG_ERR("failed spi transceive (%d)", ret);
		return ret;
	}

	*value = read_reg_buf[1];

	LOG_DBG("read reg 0x%02X value 0x%02X", reg, *value);

	return 0;
}

static int mfrc522_read_reg_multiple(const struct device *dev, const uint8_t reg,
				     const size_t count, uint8_t *values)
{
	int ret;

	if (count <= 0) {
		return -EINVAL;
	}

	for (size_t i = 0; i < count; i++) {
		ret = mfrc522_read_reg(dev, reg, values + i);
		if (ret < 0) {
			return ret;
		}
	}

	return count;
}

K_TIMER_DEFINE(rx_timer, NULL, NULL);

int mfrc522_transceive(const struct device *dev, uint8_t *tx_buf, size_t tx_len, uint8_t *rx_buf,
		       size_t rx_size, uint8_t valid_bits)
{
	struct mfrc522_data *data = dev->data;
	bool rx_done = false;
	size_t rx_len = 0;
	uint8_t reg = 0;
	int ret = 0;

	k_mutex_lock(&data->mutex, K_FOREVER);

	/* transmit */
	mfrc522_write_reg(dev, MFRC522_REG_CMD, OP_CMD_IDLE);
	mfrc522_write_reg(dev, MFRC522_REG_COM_IRQ, MFRC522_COM_IRQ_CLR_IRQ);
	mfrc522_write_reg(dev, MFRC522_REG_FIFO_LVL, MFRC522_FIFO_LVL_FLUSH_BUF);
	mfrc522_write_reg_multiple(dev, MFRC522_REG_FIFO_DATA, tx_buf, tx_len);
	mfrc522_write_reg(dev, MFRC522_REG_CMD, OP_CMD_TRANSCEIVE);
	mfrc522_write_reg(dev, MFRC522_REG_BIT_FRAME,
			  (RX_ALIGN_LSB_0 << BIT_FRAME_RX_ALIGN_MASK_SHIFT) |
				  (BIT_FRAME_TX_LAST_BITS_MASK & valid_bits) |
				  BIT_FRAME_START_SEND);

	/* receive */
	k_timer_start(&rx_timer, K_MSEC(data->rx_timeout_ms), K_NO_WAIT);
	do {
		reg = 0;
		mfrc522_read_reg(dev, MFRC522_REG_COM_IRQ, &reg);
		if (reg & COM_IRQ_RX) {
			rx_done = true;
			break;
		}

		if (reg & COM_IRQ_TIMER) {
			LOG_DBG("irq timeout");
			ret = -ETIMEDOUT;
			goto transceive_error;
		}

		k_yield();
	} while (k_timer_status_get(&rx_timer) <= 0 && k_timer_remaining_get(&rx_timer) != 0);

	k_timer_stop(&rx_timer);

	if (!rx_done) {
		LOG_DBG("rx timed out");
		ret = -ETIMEDOUT;
		goto transceive_error;
	}

	reg = 0;
	mfrc522_read_reg(dev, MFRC522_REG_ERR, &reg);
	if (reg & (ERR_REG_PRO | ERR_REG_PAR | ERR_REG_COL | ERR_REG_OVF)) {
		LOG_ERR("rx error (%X)", reg);
		ret = -EBADMSG;
		goto transceive_error;
	}

	reg = 0;
	mfrc522_read_reg(dev, MFRC522_REG_FIFO_LVL, &reg);
	if (reg > rx_size) {
		LOG_ERR("rx fifo overflows rx buf %d > %d", reg, rx_size);
		ret = -ENOBUFS;
		goto transceive_error;
	}

	ret = mfrc522_read_reg_multiple(dev, MFRC522_REG_FIFO_DATA, reg, rx_buf);
	if (ret < 0) {
		LOG_ERR("rx error reading fifo (%d)", ret);
		goto transceive_error;
	}
	rx_len = ret;

	reg = 0;
	mfrc522_read_reg(dev, MFRC522_REG_CTRL, &reg);
	reg &= CTRL_REG_RX_LAST_BITS_MASK;

	if (rx_len == 1 && reg == 4) {
		LOG_ERR("rx nack");
		ret = -EBADMSG;
		goto transceive_error;
	}

	if (reg != 0) {
		LOG_ERR("rx invalid bits (%X)", reg);
		ret = -EBADMSG;
		goto transceive_error;
	}

	k_mutex_unlock(&data->mutex);

	return rx_len;

transceive_error:
	k_mutex_unlock(&data->mutex);

	return ret;
}

static int mfrc522_get_version(const struct device *dev)
{
	struct mfrc522_data *data = dev->data;
	uint8_t reg = 0;

	mfrc522_read_reg(dev, MFRC522_REG_VERSION, &reg);
	data->chip_type = (reg & VER_REG_CHIP_TYPE_MASK) >> VER_REG_CHIP_TYPE_MASK_SHIFT;
	data->version = reg & VER_REG_VER_MASK;

	return 0;
}

/* timeout (secs) = ((prescaler x 2 + 1) x (reload + 1)) / 13.56 MHz */
static int mfrc522_set_timeout(const struct device *dev, uint16_t prescaler, uint16_t reload)
{
	/* prescaler is 12 bits. high is only 4 bits */
	uint8_t prescaler_hi = (prescaler >> 8) & BIT_MASK(4);
	uint8_t prescaler_lo = prescaler & BIT_MASK(8);
	uint8_t reg = 0;

	mfrc522_read_reg(dev, MFRC522_REG_T_MODE, &reg);
	mfrc522_write_reg(dev, MFRC522_REG_T_MODE,
			  reg | (T_MODE_REG_PRESCAL_HI_MASK & prescaler_hi));
	mfrc522_write_reg(dev, MFRC522_REG_T_PRESCAL, prescaler_lo);
	mfrc522_write_reg(dev, MFRC522_REG_T_RELOAD_H, reload >> 8);
	mfrc522_write_reg(dev, MFRC522_REG_T_RELOAD_L, reload & BIT_MASK(8));

	LOG_INF("timeout set to %d us", (((prescaler * 2 + 1) * (reload + 1)) / 13560) * 1000);

	return 0;
}

int mfrc522_enable(const struct device *dev, bool enable)
{
	uint8_t reg = 0;

	LOG_INF("%s rf", enable ? "enabling" : "disabling");

	mfrc522_read_reg(dev, MFRC522_REG_TX_CTRL, &reg);

	if (enable) {
		mfrc522_write_reg(dev, MFRC522_REG_TX_CTRL,
				  reg | (TX_CTRL_REG_TX1_EN | TX_CTRL_REG_TX2_EN));
	} else {
		mfrc522_write_reg(dev, MFRC522_REG_TX_CTRL,
				  reg & ~(TX_CTRL_REG_TX1_EN | TX_CTRL_REG_TX2_EN));
	}

	return 0;
}

static int mfrc522_set_bitrate(const struct device *dev, enum bit_rate bit_rate)
{
	mfrc522_write_reg(dev, MFRC522_REG_TX_MODE, (TX_MODE_SPEED_MASK & bit_rate));
	mfrc522_write_reg(dev, MFRC522_REG_RX_MODE, (RX_MODE_SPEED_MASK & bit_rate));

	return 0;
}

static void mfrc522_reset(const struct device *dev)
{
	const struct mfrc522_config *config = dev->config;

	gpio_pin_set_dt(&config->rst, 1);
	k_usleep(1); /* min 10 ns */
	gpio_pin_set_dt(&config->rst, 0);
	k_usleep(100); /* min 38 us */

	mfrc522_write_reg(dev, MFRC522_REG_CMD, OP_CMD_SOFT_RST);
}

int mfrc522_init(const struct device *dev)
{
	const struct mfrc522_config *config = dev->config;
	struct mfrc522_data *data = dev->data;
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

	k_mutex_init(&data->mutex);

	mfrc522_reset(dev);

	mfrc522_get_version(dev);

	mfrc522_set_bitrate(dev, data->bit_rate);

	mfrc522_write_reg(dev, MFRC522_REG_MOD_WID, data->miller_mod_width);

	mfrc522_write_reg(dev, MFRC522_REG_T_MODE, T_MODE_REG_AUTO);
	mfrc522_set_timeout(dev, data->timer_prescaler, data->timer_reload);

	mfrc522_write_reg(dev, MFRC522_REG_TX_ASK, TX_ASK_REG_FORCE_100);
	mfrc522_write_reg(dev, MFRC522_REG_MODE, MODE_REG_TX_WAIT_RF);

	LOG_INF("chip type 0x%X version %X", data->chip_type, data->version);

	return 0;
}

#define MFRC522_DEVICE(n)                                                                          \
	static struct mfrc522_config mfrc522_config_##n = {                                        \
		.spi = SPI_DT_SPEC_INST_GET(n, SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB |             \
						       SPI_WORD_SET(8) | SPI_LINES_SINGLE),        \
		.rst = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),                              \
	};                                                                                         \
                                                                                                   \
	static struct mfrc522_data mfrc522_data_##n = {                                            \
		.chip_type = 0,                                                                    \
		.version = 0,                                                                      \
		.bit_rate = DT_INST_PROP(n, bit_rate),                                             \
		.timer_prescaler = DT_INST_PROP(n, timer_prescaler),                               \
		.timer_reload = DT_INST_PROP(n, timer_reload),                                     \
		.miller_mod_width = DT_INST_PROP(n, miller_mod_width),                             \
		.rx_timeout_ms = DT_INST_PROP(n, rx_timeout_ms),                                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &mfrc522_init, NULL, &mfrc522_data_##n, &mfrc522_config_##n,      \
			      POST_KERNEL, CONFIG_RFID_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFRC522_DEVICE)
