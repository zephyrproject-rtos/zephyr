/*
 * Copyright (c) 2024 Analog Devices Inc.
 * Copyright (c) 2024 Baylibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max22190_gpio

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gpio_max22190);

#include <zephyr/drivers/gpio/gpio_utils.h>

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "GPIO MAX22190 driver enabled without any devices"
#endif

#define MAX22190_ENABLE  1
#define MAX22190_DISABLE 0

#define MAX22190_READ  0
#define MAX22190_WRITE 1

#define MAX22190_MAX_PKT_SIZE   3
#define MAX22190_CHANNELS       8
#define MAX22190_FAULT2_ENABLES 5

#define MAX22190_WB_REG           0x0
#define MAX22190_DI_REG           0x2
#define MAX22190_FAULT1_REG       0x4
#define MAX22190_FILTER_IN_REG(x) (0x6 + (2 * (x)))
#define MAX22190_CFG_REG          0x18
#define MAX22190_IN_EN_REG        0x1A
#define MAX22190_FAULT2_REG       0x1C
#define MAX22190_FAULT2_EN_REG    0x1E
#define MAX22190_GPO_REG          0x22
#define MAX22190_FAULT1_EN_REG    0x24
#define MAX22190_NOP_REG          0x26

#define MAX22190_CH_STATE_MASK(x) BIT(x)
#define MAX22190_DELAY_MASK       GENMASK(2, 0)
#define MAX22190_FBP_MASK         BIT(3)
#define MAX22190_WBE_MASK         BIT(4)
#define MAX22190_RW_MASK          BIT(7)
#define MAX22190_ADDR_MASK        GENMASK(6, 0)
#define MAX22190_ALARM_MASK       GENMASK(4, 3)
#define MAX22190_POR_MASK         BIT(6)

#define MAX22190_FAULT_MASK(x)   BIT(x)
#define MAX22190_FAULT2_WBE_MASK BIT(4)

#define MAX22190_FAULT2_EN_MASK GENMASK(5, 0)

#define MAX22190_CFG_REFDI_MASK BIT(0)
#define MAX22190_CFG_CLRF_MASK  BIT(3)
#define MAX22190_CFG_24VF_MASK  BIT(4)

#define PRINT_ERR_BIT(bit1, bit2)                                                                  \
	if (bit1 & bit2) {                                                                         \
		LOG_ERR("[%s] %d", #bit1, bit1);                                                   \
	}

#define MAX22190_CLEAN_POR(dev)                                                                    \
	max22190_reg_update(dev, MAX22190_FAULT1_REG, MAX22190_POR_MASK,                           \
			    FIELD_PREP(MAX22190_POR_MASK, 0))

enum max22190_ch_state {
	MAX22190_CH_OFF,
	MAX22190_CH_ON
};

enum max22190_ch_wb_state {
	MAX22190_CH_NO_WB_BREAK,
	MAX22190_CH_WB_COND_DET
};

enum max22190_mode {
	MAX22190_MODE_0,
	MAX22190_MODE_1,
	MAX22190_MODE_2,
	MAX22190_MODE_3,
};

union max22190_fault1 {
	uint8_t reg_raw;
	struct {
		uint8_t max22190_WBG: 1; /* BIT 0 */
		uint8_t max22190_24VM: 1;
		uint8_t max22190_24VL: 1;
		uint8_t max22190_ALRMT1: 1;
		uint8_t max22190_ALRMT2: 1;
		uint8_t max22190_FAULT2: 1;
		uint8_t max22190_POR: 1;
		uint8_t max22190_CRC: 1; /* BIT 7 */
	} reg_bits;
};

union max22190_fault1_en {
	uint8_t reg_raw;
	struct {
		uint8_t max22190_WBGE: 1; /* BIT 0 */
		uint8_t max22190_24VME: 1;
		uint8_t max22190_24VLE: 1;
		uint8_t max22190_ALRMT1E: 1;
		uint8_t max22190_ALRMT2E: 1;
		uint8_t max22190_FAULT2E: 1;
		uint8_t max22190_PORE: 1;
		uint8_t max22190_CRCE: 1; /* BIT 7 */
	} reg_bits;
};

union max22190_fault2 {
	uint8_t reg_raw;
	struct {
		uint8_t max22190_RFWBS: 1; /* BIT 0 */
		uint8_t max22190_RFWBO: 1;
		uint8_t max22190_RFDIS: 1;
		uint8_t max22190_RFDIO: 1;
		uint8_t max22190_OTSHDN: 1;
		uint8_t max22190_FAULT8CK: 1;
		uint8_t max22190_DUMMY: 2; /* BIT 7 */
	} reg_bits;
};

union max22190_fault2_en {
	uint8_t reg_raw;
	struct {
		uint8_t max22190_RFWBSE: 1; /* BIT 0 */
		uint8_t max22190_RFWBOE: 1;
		uint8_t max22190_RFDISE: 1;
		uint8_t max22190_RFDIOE: 1;
		uint8_t max22190_OTSHDNE: 1;
		uint8_t max22190_FAULT8CKE: 1;
		uint8_t max22190_DUMMY: 2; /* BIT 7 */
	} reg_bits;
};

union max22190_cfg {
	uint8_t reg_raw;
	struct {
		uint8_t max22190_DUMMY1: 3; /* BIT 0 */
		uint8_t max22190_24VF: 1;
		uint8_t max22190_CLRF: 1;
		uint8_t max22190_DUMMY2: 2;
		uint8_t max22190_REFDI_SH_EN: 1; /* BIT 7 */
	} reg_bits;
};

union max22190_filter {
	uint8_t reg_raw;
	struct {
		uint8_t max22190_DELAY: 3; /* BIT 0 */
		uint8_t max22190_FBP: 1;
		uint8_t max22190_WBE: 1;
		uint8_t max22190_DUMMY: 2; /* BIT 7 */
	} reg_bits;
};

enum max22190_delay {
	MAX22190_DELAY_50US,
	MAX22190_DELAY_100US,
	MAX22190_DELAY_400US,
	MAX22190_DELAY_800US,
	MAX22190_DELAY_1800US,
	MAX22190_DELAY_3200US,
	MAX22190_DELAY_12800US,
	MAX22190_DELAY_20000US
};

struct max22190_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec fault_gpio;
	struct gpio_dt_spec ready_gpio;
	struct gpio_dt_spec latch_gpio;
	union max22190_filter filter[8];
	bool crc_en;
	enum max22190_mode mode;
	uint8_t pkt_size;
};

struct max22190_data {
	struct gpio_driver_data common;
	enum max22190_ch_state channels[MAX22190_CHANNELS];
	enum max22190_ch_wb_state wb[MAX22190_CHANNELS];
	union max22190_cfg cfg;
	union max22190_fault1 fault1;
	union max22190_fault1_en fault1_en;
	union max22190_fault2 fault2;
	union max22190_fault2_en fault2_en;
};

/*
 * @brief Compute the CRC5 value for MAX22190
 * @param data - Data array to calculate CRC for.
 * @return CRC result.
 */
static uint8_t max22190_crc(uint8_t *data)
{
	int length = 19;
	uint8_t crc_step, tmp;
	uint8_t crc_init = 0x7;
	uint8_t crc_poly = 0x35;
	int i;

	/*
	 * This is the C custom implementation of CRC function for MAX22190, and
	 * can be found here:
	 * https://www.analog.com/en/design-notes/guidelines-to-implement-crc-algorithm.html
	 */
	uint32_t datainput = (uint32_t)((data[0] << 16) + (data[1] << 8) + data[2]);

	datainput = (datainput & 0xFFFFE0) + crc_init;

	tmp = (uint8_t)((datainput & 0xFC0000) >> 18);

	if ((tmp & 0x20) == 0x20) {
		crc_step = (uint8_t)(tmp ^ crc_poly);
	} else {
		crc_step = tmp;
	}

	for (i = 0; i < length - 1; i++) {
		tmp = (uint8_t)(((crc_step & 0x1F) << 1) +
				((datainput >> (length - 2 - i)) & 0x01));

		if ((tmp & 0x20) == 0x20) {
			crc_step = (uint8_t)(tmp ^ crc_poly);
		} else {
			crc_step = tmp;
		}
	}

	return (uint8_t)(crc_step & 0x1F);
}

/*
 * @brief Update chan WB state in max22190_data
 *
 * @param dev - MAX22190 device.
 * @param val - value to be set.
 */
static void max22190_update_wb_stat(const struct device *dev, uint8_t val)
{
	struct max22190_data *data = dev->data;

	for (int ch_n = 0; ch_n < 8; ch_n++) {
		data->wb[ch_n] = (val >> ch_n) & 0x1;
	}
}

/*
 * @brief Update chan IN state in max22190_data
 *
 * @param dev - MAX22190 device.
 * @param val - value to be set.
 */
static void max22190_update_in_stat(const struct device *dev, uint8_t val)
{
	struct max22190_data *data = dev->data;

	for (int ch_n = 0; ch_n < 8; ch_n++) {
		data->channels[ch_n] = (val >> ch_n) & 0x1;
	}
}

/*
 * @brief Register write function for MAX22190
 *
 * @param dev - MAX22190 device config.
 * @param addr - Register value to which data is written.
 * @param val - Value which is to be written to requested register.
 * @return 0 in case of success, negative error code otherwise.
 */
static int max22190_reg_transceive(const struct device *dev, uint8_t addr, uint8_t val, uint8_t rw)
{
	uint8_t crc;
	int ret;

	uint8_t local_rx_buff[MAX22190_MAX_PKT_SIZE] = {0};
	uint8_t local_tx_buff[MAX22190_MAX_PKT_SIZE] = {0};

	const struct max22190_config *config = dev->config;

	struct spi_buf tx_buf = {
		.buf = &local_tx_buff,
		.len = config->pkt_size,
	};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	struct spi_buf rx_buf = {
		.buf = &local_rx_buff,
		.len = config->pkt_size,
	};
	const struct spi_buf_set rx = {.buffers = &rx_buf, .count = 1};

	local_tx_buff[0] =
		FIELD_PREP(MAX22190_ADDR_MASK, addr) | FIELD_PREP(MAX22190_RW_MASK, rw & 0x1);
	local_tx_buff[1] = val;

	/* If CRC enabled calculate it */
	if (config->crc_en) {
		local_tx_buff[2] = max22190_crc(&local_tx_buff[0]);
	}

	/* write cmd & read resp at once */
	ret = spi_transceive_dt(&config->spi, &tx, &rx);

	if (ret) {
		LOG_ERR("Err spi_transcieve_dt  [%d]\n", ret);
		return ret;
	}

	/* if CRC enabled check readed */
	if (config->crc_en) {
		crc = max22190_crc(&local_rx_buff[0]);
		if (crc != (local_rx_buff[2] & 0x1F)) {
			LOG_ERR("READ CRC ERR (%d)-(%d)\n", crc, (local_rx_buff[2] & 0x1F));
			return -EINVAL;
		}
	}

	/* always (R/W) get DI reg in first byte */
	max22190_update_in_stat(dev, local_rx_buff[0]);

	/* in case of writing register we get as second byte WB reg */
	if (rw == MAX22190_WRITE) {
		max22190_update_wb_stat(dev, local_rx_buff[1]);
	} else {
		/* in case of READ second byte is value we are looking for */
		ret = local_rx_buff[1];
	}

	return ret;
}

#define max22190_reg_read(dev, addr)       max22190_reg_transceive(dev, addr, 0, MAX22190_READ)
#define max22190_reg_write(dev, addr, val) max22190_reg_transceive(dev, addr, val, MAX22190_WRITE)

/*
 * @brief Register update function for MAX22190
 *
 * @param dev - MAX22190 device.
 * @param addr - Register valueto wich data is updated.
 * @param mask - Corresponding mask to the data that will be updated.
 * @param val - Updated value to be written in the register at update.
 * @return 0 in case of success, negative error code otherwise.
 */
static int max22190_reg_update(const struct device *dev, uint8_t addr, uint8_t mask, uint8_t val)
{
	int ret;
	uint32_t reg_val = 0;

	ret = max22190_reg_read(dev, addr);

	reg_val = ret;
	reg_val &= ~mask;
	reg_val |= mask & val;

	return max22190_reg_write(dev, addr, reg_val);
}

/*
 * @brief Check FAULT1 and FAULT2
 *
 * @param dev - MAX22190 device
 */
static void max22190_fault_check(const struct device *dev)
{
	struct max22190_data *data = dev->data;

	/* FAULT1 */
	data->fault1.reg_raw = max22190_reg_read(dev, MAX22190_FAULT1_REG);

	if (data->fault1.reg_raw) {
		/* FAULT1_EN */
		data->fault1_en.reg_raw = max22190_reg_read(dev, MAX22190_FAULT1_EN_REG);

		PRINT_ERR_BIT(data->fault1.reg_bits.max22190_CRC,
			      data->fault1_en.reg_bits.max22190_CRCE);
		PRINT_ERR_BIT(data->fault1.reg_bits.max22190_POR,
			      data->fault1_en.reg_bits.max22190_PORE);
		PRINT_ERR_BIT(data->fault1.reg_bits.max22190_FAULT2,
			      data->fault1_en.reg_bits.max22190_FAULT2E);
		PRINT_ERR_BIT(data->fault1.reg_bits.max22190_ALRMT2,
			      data->fault1_en.reg_bits.max22190_ALRMT2E);
		PRINT_ERR_BIT(data->fault1.reg_bits.max22190_ALRMT1,
			      data->fault1_en.reg_bits.max22190_ALRMT1E);
		PRINT_ERR_BIT(data->fault1.reg_bits.max22190_24VL,
			      data->fault1_en.reg_bits.max22190_24VLE);
		PRINT_ERR_BIT(data->fault1.reg_bits.max22190_24VM,
			      data->fault1_en.reg_bits.max22190_24VME);
		PRINT_ERR_BIT(data->fault1.reg_bits.max22190_WBG,
			      data->fault1_en.reg_bits.max22190_WBGE);

		if (data->fault1.reg_bits.max22190_WBG & data->fault1_en.reg_bits.max22190_WBGE) {
			uint8_t wb_val = max22190_reg_read(dev, MAX22190_WB_REG);

			max22190_update_wb_stat(dev, (uint8_t)wb_val);
		}

		if (data->fault1.reg_bits.max22190_FAULT2) {
			/* FAULT2 */
			data->fault2.reg_raw = max22190_reg_read(dev, MAX22190_FAULT2_REG);

			/* FAULT2_EN */
			data->fault2_en.reg_raw = max22190_reg_read(dev, MAX22190_FAULT2_EN_REG);

			PRINT_ERR_BIT(data->fault2.reg_bits.max22190_RFWBS,
				      data->fault2_en.reg_bits.max22190_RFWBSE);
			PRINT_ERR_BIT(data->fault2.reg_bits.max22190_RFWBO,
				      data->fault2_en.reg_bits.max22190_RFWBOE);
			PRINT_ERR_BIT(data->fault2.reg_bits.max22190_RFDIS,
				      data->fault2_en.reg_bits.max22190_RFDISE);
			PRINT_ERR_BIT(data->fault2.reg_bits.max22190_RFDIO,
				      data->fault2_en.reg_bits.max22190_RFDIOE);
			PRINT_ERR_BIT(data->fault2.reg_bits.max22190_OTSHDN,
				      data->fault2_en.reg_bits.max22190_OTSHDNE);
			PRINT_ERR_BIT(data->fault2.reg_bits.max22190_FAULT8CK,
				      data->fault2_en.reg_bits.max22190_FAULT8CKE);
		}
	}
}

static void max22190_state_get(const struct device *dev)
{
	const struct max22190_config *config = dev->config;

	if (gpio_pin_get_dt(&config->fault_gpio)) {
		max22190_fault_check(dev);
	}

	/* We are reading WB reg because on first byte will be clocked out DI reg
	 * on second byte we will ge WB value.
	 */
	uint8_t wb_val = max22190_reg_read(dev, MAX22190_WB_REG);

	max22190_update_wb_stat(dev, (uint8_t)wb_val);
}

static int gpio_max22190_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	int err = 0;

	if ((flags & (GPIO_INPUT | GPIO_OUTPUT)) == GPIO_DISCONNECTED) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		return -ENOTSUP;
	}

	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0) {
		return -ENOTSUP;
	}

	if (flags & GPIO_INT_ENABLE) {
		return -ENOTSUP;
	}

	switch (flags & GPIO_DIR_MASK) {
	case GPIO_INPUT:
		LOG_INF("Nothing to do, only INPUT supported");
		break;
	default:
		LOG_ERR("On MAX22190 only input option is available!");
		return -ENOTSUP;
	}

	return err;
}

static void max22190_filter_set(const struct device *dev)
{
	const struct max22190_config *config = dev->config;

	for (int ch_n = 0; ch_n < 8; ch_n++) {
		max22190_reg_write(dev, MAX22190_FILTER_IN_REG(ch_n), config->filter[ch_n].reg_raw);
	}
}

static int max22190_fault_set(const struct device *dev)
{
	const struct max22190_data *data = dev->data;
	int ret = 0;

	ret = max22190_reg_write(dev, MAX22190_FAULT1_EN_REG, data->fault1_en.reg_raw);
	if (ret) {
		goto exit_fault_set;
	}

	ret = max22190_reg_write(dev, MAX22190_FAULT1_REG, data->fault1.reg_raw);
	if (ret) {
		goto exit_fault_set;
	}

	ret = max22190_reg_write(dev, MAX22190_FAULT2_EN_REG, data->fault2_en.reg_raw);
	if (ret) {
		goto exit_fault_set;
	}

	ret = max22190_reg_write(dev, MAX22190_FAULT2_REG, data->fault2.reg_raw);
	if (ret) {
		goto exit_fault_set;
	}

	return ret;

exit_fault_set:
	LOG_ERR("Err spi_transcieve_dt  [%d]\n", ret);
	return ret;
}

static int gpio_max22190_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct max22190_data *data = dev->data;

	max22190_state_get(dev);
	*value = 0;
	for (int ch_n = 0; ch_n < 8; ch_n++) {
		/* IN ch state */
		*value |= data->channels[ch_n] << ch_n;
	}

	return 0;
}

static int gpio_max22190_init(const struct device *dev)
{
	const struct max22190_config *config = dev->config;
	struct max22190_data *data = dev->data;

	LOG_DBG("GPIO MAX22190 init IN\n");

	int err = 0;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus is not ready\n");
		return -ENODEV;
	}

	/* setup READY gpio - normal low */
	if (!gpio_is_ready_dt(&config->ready_gpio)) {
		LOG_ERR("READY GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->ready_gpio, GPIO_INPUT);
	if (err < 0) {
		LOG_ERR("Failed to configure reset GPIO");
		return err;
	}

	/* setup FAULT gpio - normal high */
	if (!gpio_is_ready_dt(&config->fault_gpio)) {
		LOG_ERR("FAULT GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->fault_gpio, GPIO_INPUT | GPIO_PULL_UP);
	if (err < 0) {
		LOG_ERR("Failed to configure DC GPIO");
		return err;
	}

	/* setup LATCH gpio - normal high */
	if (!gpio_is_ready_dt(&config->latch_gpio)) {
		LOG_ERR("LATCH GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->latch_gpio, GPIO_OUTPUT_INACTIVE);
	if (err < 0) {
		LOG_ERR("Failed to configure busy GPIO");
		return err;
	}

	for (int i = 0; i < 8; i++) {
		LOG_DBG("IN%d WBE [%d] FBP[%d] DELAY[%d]\n", i,
			config->filter[i].reg_bits.max22190_WBE,
			config->filter[i].reg_bits.max22190_FBP,
			config->filter[i].reg_bits.max22190_DELAY);
	}

	LOG_DBG(" > MAX22190 MODE: %x", config->mode);
	LOG_DBG(" > MAX22190 PKT SIZE: %dbits (%dbytes)", config->pkt_size * 8, config->pkt_size);
	LOG_DBG(" > MAX22190 CRC: %s", config->crc_en ? "enable" : "disable");

	data->fault1_en.reg_bits.max22190_WBGE = 1;
	data->fault1_en.reg_bits.max22190_PORE = 1;

	/* Set all FAULT and FAULT_EN regs */
	max22190_fault_set(dev);

	/* Set channels filters */
	max22190_filter_set(dev);

	/* POR bit need to be cleared after start */
	MAX22190_CLEAN_POR(dev);

	LOG_DBG("GPIO MAX22190 init OUT\n");
	return 0;
}

static DEVICE_API(gpio, gpio_max22190_api) = {
	.pin_configure = gpio_max22190_config,
	.port_get_raw = gpio_max22190_port_get_raw,
};

/* Assign appropriate value for FILTER delay*/
#define MAX22190_FILTER_SET_DELAY(delay)                                                           \
	delay == 20000   ? MAX22190_DELAY_20000US                                                  \
	: delay == 12800 ? MAX22190_DELAY_12800US                                                  \
	: delay == 3200  ? MAX22190_DELAY_3200US                                                   \
	: delay == 1600  ? MAX22190_DELAY_1800US                                                   \
	: delay == 800   ? MAX22190_DELAY_800US                                                    \
	: delay == 400   ? MAX22190_DELAY_400US                                                    \
	: delay == 100   ? MAX22190_DELAY_100US                                                    \
	: delay == 50    ? MAX22190_DELAY_50US                                                     \
			 : MAX22190_DELAY_20000US

/* Set FILTERx reg */
#define MAX22190_FILTER_BY_IDX(id, idx)                                                            \
	{                                                                                          \
		.reg_bits.max22190_DELAY =                                                         \
			MAX22190_FILTER_SET_DELAY(DT_INST_PROP_BY_IDX(id, filter_delays, idx)),    \
		.reg_bits.max22190_FBP = DT_INST_PROP_BY_IDX(id, filter_fbps, idx),                \
		.reg_bits.max22190_WBE = DT_INST_PROP_BY_IDX(id, filter_wbes, idx),                \
	}

#define GPIO_MAX22190_DEVICE(id)                                                                   \
	static const struct max22190_config max22190_##id##_cfg = {                                \
		.spi = SPI_DT_SPEC_INST_GET(id, SPI_OP_MODE_MASTER | SPI_WORD_SET(8U), 0U),        \
		.ready_gpio = GPIO_DT_SPEC_INST_GET(id, drdy_gpios),                               \
		.fault_gpio = GPIO_DT_SPEC_INST_GET(id, fault_gpios),                              \
		.latch_gpio = GPIO_DT_SPEC_INST_GET(id, latch_gpios),                              \
		.mode = DT_INST_PROP(id, max22190_mode),                                           \
		.crc_en = !(DT_INST_PROP(id, max22190_mode) & 0x1),                                \
		.pkt_size = !(DT_INST_PROP(id, max22190_mode) & 0x1) ? 3 : 2,                      \
		.filter =                                                                          \
			{                                                                          \
				MAX22190_FILTER_BY_IDX(id, 0),                                     \
				MAX22190_FILTER_BY_IDX(id, 1),                                     \
				MAX22190_FILTER_BY_IDX(id, 2),                                     \
				MAX22190_FILTER_BY_IDX(id, 3),                                     \
				MAX22190_FILTER_BY_IDX(id, 4),                                     \
				MAX22190_FILTER_BY_IDX(id, 5),                                     \
				MAX22190_FILTER_BY_IDX(id, 6),                                     \
				MAX22190_FILTER_BY_IDX(id, 7),                                     \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	static struct max22190_data max22190_##id##_data;                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(id, &gpio_max22190_init, NULL, &max22190_##id##_data,                \
			      &max22190_##id##_cfg, POST_KERNEL,                                   \
			      CONFIG_GPIO_MAX22190_INIT_PRIORITY, &gpio_max22190_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_MAX22190_DEVICE)
