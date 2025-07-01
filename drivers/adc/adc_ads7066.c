/*
 * Copyright (c) 2025 GE Healthcare Technologies, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ADC driver for TI ADS7066 16-bit 8-channel SPI ADC
 */

#define DT_DRV_COMPAT ti_ads7066

#include <stdint.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>

LOG_MODULE_REGISTER(adc_ads7066, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

/* ADS7066 Specifications */
#define ADS7066_RESOLUTION            16U
#define ADS7066_MAX_CHANNELS          8U
#define ADS7066_INTERNAL_REFERENCE_MV 2500U

/* 7.3.10.3 Register Read/Write operation */
#define ADS7066_CMD_NOP        0x00 /* 0000 0000b */
#define ADS7066_CMD_REG_READ   0x10 /* 0001 0000b */
#define ADS7066_CMD_REG_WRITE  0x08 /* 0000 1000b */
#define ADS7066_CMD_SET_BITS   0x18 /* 0001 1000b */
#define ADS7066_CMD_CLEAR_BITS 0x20 /* 0010 0000b */

/* 7.4.3 On-the-Fly Mode */
#define ADS7066_OTF_START_BIT 0x80

/* 7.5 ADS7066 Registers */               /* Reset value */
#define ADS7066_REG_SYSTEM_STATUS    0x0  /* = 0x81 */
#define ADS7066_REG_GENERAL_CFG      0x1  /* = 0x00 */
#define ADS7066_REG_DATA_CFG         0x2  /* = 0x00 */
#define ADS7066_REG_OSR_CFG          0x3  /* = 0x00 */
#define ADS7066_REG_OPMODE_CFG       0x4  /* = 0x04 */
#define ADS7066_REG_PIN_CFG          0x5  /* = 0x00 */
#define ADS7066_REG_GPIO_CFG         0x7  /* = 0x00 */
#define ADS7066_REG_GPO_DRIVE_CFG    0x9  /* = 0x00 */
#define ADS7066_REG_GPO_OUTPUT_VALUE 0xB  /* = 0x00 */
#define ADS7066_REG_GPI_VALUE        0xD  /* = 0x00 */
#define ADS7066_REG_SEQUENCE_CFG     0x10 /* = 0x00 */
#define ADS7066_REG_CHANNEL_SEL      0x11 /* = 0x00 */
#define ADS7066_REG_AUTO_SEQ_CH_SEL  0x12 /* = 0x00 */
#define ADS7066_REG_DIAGNOSTICS_KEY  0xBF /* = 0x00 */
#define ADS7066_REG_DIAGNOSTICS_EN   0xC0 /* = 0x00 */
#define ADS7066_REG_BIT_SAMPLE_LSB   0xC1 /* = 0x00 */
#define ADS7066_REG_BIT_SAMPLE_MSB   0xC2 /* = 0x00 */

/* 7.5.2 */
/* SYSTEM_STATUS Register (Address = 0x0) [Reset = 0x81] */
#define ADS7066_STATUS_CRCERR_FUSE BIT(2)
#define ADS7066_STATUS_CRCERR_IN   BIT(1)
#define ADS7066_STATUS_BOR         BIT(0)
/* GENERAL_CFG Register (Address = 0x1) [Reset = 0x00] */
#define ADS7066_CFG_REF_EN         BIT(7)
#define ADS7066_CFG_CRC_EN         BIT(6)
#define ADS7066_CFG_CAL            BIT(1)
#define ADS7066_CFG_RST            BIT(0)
/* DATA CFG Register (Address = 0x2) [Reset = 0x00] */
#define ADS7066_CFG_APPEND_STAT    BIT(5)
#define ADS7066_CFG_APPEND_CHID    BIT(4)
/* SEQUENCE CFG Register (Address = 0x10) [Reset = 0x00] */
#define ADS7066_CFG_SEQ_MODE_AUTO  0x01
#define ADS7066_CFG_SEQ_MODE_OTF   0x02
#define ADS7066_CFG_SEQ_START      BIT(4)

#define ADS7066_SPI_BUF_SIZE           4
#define ADS7066_COMMAND_SIZE           3
#define ADS7066_CONVERSION_RESULT_SIZE 2
#define ADS7066_REGISTER_READ_SIZE     1

/* 6.7 Switching characteristics */
#define ADS7066_TIME_RST 5U /* msec */

#define ADS7066_CALIBRATION_TIMEOUT       10  /* calibration is measured to take ~140 us */
#define ADS7066_CALIBRATION_POLL_INTERVAL 100 /* us */

enum ads7066_mode {
	ADS7066_MODE_MANUAL = 0, /* Register-based channel selection */
	ADS7066_MODE_OTF = 1,    /* Zero latency channel switching */
	ADS7066_MODE_AUTO = 2,   /*
				  * Switch to next input channel automatically after each conversion
				  */
};

struct ads7066_config {
	struct spi_dt_spec bus;
	uint8_t resolution;
	uint8_t channels;
	enum adc_reference reference;
	enum ads7066_mode mode;
};

struct ads7066_data {
	struct adc_context ctx;
	const struct device *dev;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint8_t channel;       /* current channel */
	uint8_t channels_mask; /* channels to sample: BIT(0) = ch0, BIT(1) = ch1, ... */
	bool crc_enabled;
	bool crcerr_in;                /* MOSI CRC error */
	uint8_t auto_reg_channels_sel; /* A cached version of ADS7066_REG_AUTO_SEQ_CH_SEL */

	struct k_sem sem;

#ifdef CONFIG_ADC_ASYNC
	struct k_thread thread;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_ADC_ADS7066_ACQUISITION_THREAD_STACK_SIZE);
#endif
};

enum ads7066_status_flag_options {
	STATUS_IF_CRC,
	STATUS_ALWAYS,
	STATUS_NEVER,
};

struct ads7066_cmd_options {
	enum ads7066_status_flag_options status_flags;
	bool ignore_status_flags;
	bool ignore_crc;
};

struct ads7066_cmd {
	uint8_t tx_buf[ADS7066_SPI_BUF_SIZE];
	uint8_t tx_len;
	uint8_t rx_buf[ADS7066_SPI_BUF_SIZE];
	uint8_t rx_len;

	struct ads7066_cmd_options options;
};

static int ads7066_spi_transceive(const struct device *dev, struct ads7066_cmd *cmd)
{
	const struct ads7066_config *config = dev->config;
	struct ads7066_data *data = dev->data;
	uint8_t tx_len = cmd->tx_len;
	uint8_t rx_len = cmd->rx_len;
	bool status_flags;
	int err;

	switch (cmd->options.status_flags) {
	case STATUS_IF_CRC:
		status_flags = data->crc_enabled;
		break;
	case STATUS_ALWAYS:
		status_flags = true;
		break;
	case STATUS_NEVER:
		status_flags = false;
		break;
	default:
		LOG_ERR("invalid status flags option");
		return -EINVAL;
	}

	if (status_flags) {
		rx_len++;
	}

	uint8_t frame_len = data->crc_enabled ? 4 : MAX(tx_len, rx_len);
	const struct spi_buf spi_tx_buf[] = {{
		.buf = cmd->tx_buf,
		.len = frame_len,
	}};
	const struct spi_buf_set tx = {
		.buffers = spi_tx_buf,
		.count = ARRAY_SIZE(spi_tx_buf),
	};

	const struct spi_buf spi_rx_buf[] = {{
		.buf = cmd->rx_buf,
		.len = frame_len,
	}};
	const struct spi_buf_set rx = {
		.buffers = spi_rx_buf,
		.count = ARRAY_SIZE(spi_rx_buf),
	};

#ifdef CONFIG_ADC_ADS7066_CRC
	if (data->crc_enabled) {
		cmd->tx_buf[3] = crc8_ccitt(0xFF, cmd->tx_buf, 3);
	}
#endif

	err = spi_transceive_dt(&config->bus, &tx, &rx);
	if (err) {
		LOG_ERR("spi_transceive failed (%d)", err);
		return err;
	}

	/* Only check for status and CRC errors if we know for sure they are enabled on the ADC. */
#ifdef CONFIG_ADC_ADS7066_CRC
	if (data->crc_enabled && !cmd->options.ignore_crc) {
		uint8_t crc = cmd->rx_buf[rx_len];
		uint8_t crc_calc = crc8_ccitt(0xFF, cmd->rx_buf, rx_len);

		if (crc_calc != crc) {
			LOG_ERR("crc mismatch 0x%02x != 0x%02x", crc, crc_calc);
			return -EBADMSG;
		}
	};
#endif /* CONFIG_ADC_ADS7066_CRC */

	/* Check status flags */
	if (status_flags && !cmd->options.ignore_status_flags) {
		uint8_t status_byte = cmd->rx_buf[2];

		/* 7.3.9.1 Status Flags
		 * Checks that bit 7 is set and no other bits than 7 and 5 are set.
		 */
		if ((status_byte & 0x80) == 0x00 ||
		    (status_byte & ~(0x80 | (ADS7066_STATUS_CRCERR_IN << 4))) != 0) {
			LOG_ERR("invalid status byte 0x%02x", status_byte);
			LOG_HEXDUMP_DBG(cmd->tx_buf, frame_len, "tx");
			LOG_HEXDUMP_DBG(cmd->rx_buf, frame_len, "rx");

			return -EBADMSG;
		}

		if (((status_byte & 0xF0) >> 4) & ADS7066_STATUS_CRCERR_IN) {
			LOG_HEXDUMP_DBG(cmd->tx_buf, frame_len, "tx");
			LOG_HEXDUMP_DBG(cmd->rx_buf, frame_len, "rx");
			LOG_WRN("input CRC error");
			data->crcerr_in = true;
		}
	}

	k_busy_wait(CONFIG_ADC_ADS7066_CONVERSION_BUSY_WAIT);

	return 0;
}

static int ads7066_spi_write_cmd_read_conversion(const struct device *dev, const uint8_t opcode,
						 const uint8_t address, const uint8_t value,
						 uint16_t *result,
						 const struct ads7066_cmd_options *options)
{
	struct ads7066_cmd buf = {
		.tx_buf = {opcode, address, value},
		.tx_len = ADS7066_COMMAND_SIZE,
		.rx_buf = {0},
		.rx_len = ADS7066_CONVERSION_RESULT_SIZE,
		.options = options ? *options : (struct ads7066_cmd_options){0},
	};
	int err = ads7066_spi_transceive(dev, &buf);

	if (err) {
		return err;
	}

	if (result) {
		*result = sys_get_be16(buf.rx_buf);
	}

	return 0;
}

static int ads7066_read_conversion(const struct device *dev, uint16_t *result,
				   const struct ads7066_cmd_options *options)
{
	return ads7066_spi_write_cmd_read_conversion(dev, ADS7066_CMD_NOP, 0, 0, result, options);
}

static int ads7066_write_cmd(const struct device *dev, const uint8_t opcode, const uint8_t address,
			     const uint8_t value, const struct ads7066_cmd_options *options)
{
	return ads7066_spi_write_cmd_read_conversion(dev, opcode, address, value, NULL, options);
}

static int ads7066_read_single_register(const struct device *dev, const uint8_t address,
					uint8_t *result, const struct ads7066_cmd_options *options)
{
	int err = ads7066_write_cmd(dev, ADS7066_CMD_REG_READ, address, 0, options);

	if (err) {
		return err;
	}

	/* 7.3.10.3.2 Register Read, Fig. 7-10
	 * The register read output data is read from the second frame.
	 */
	struct ads7066_cmd buf = {
		.tx_buf = {0},
		.tx_len = 0,
		.rx_buf = {0},
		.rx_len = ADS7066_REGISTER_READ_SIZE,
		.options = {
			.status_flags = STATUS_NEVER,
		},
	};
	err = ads7066_spi_transceive(dev, &buf);
	if (err) {
		LOG_ERR("failed to read back register 0x%02x", address);
		return err;
	}

	*result = buf.rx_buf[0];

	return 0;
}

#ifdef CONFIG_ADC_ADS7066_CRC
static int ads7066_clear_crcerr_in(const struct device *dev)
{
	struct ads7066_data *data = dev->data;
	int err;

	LOG_INF("attempting to clear input CRC error");

	err = ads7066_write_cmd(dev, ADS7066_CMD_SET_BITS, ADS7066_REG_SYSTEM_STATUS,
				ADS7066_STATUS_CRCERR_IN, NULL);
	if (err) {
		return err;
	}

	uint8_t reg;

	err = ads7066_read_single_register(dev, ADS7066_REG_SYSTEM_STATUS, &reg, NULL);
	if (err) {
		return err;
	}

	if (reg & ADS7066_STATUS_CRCERR_IN) {
		LOG_ERR("could not clear CRC error!");
		return -EIO;
	}

	data->crcerr_in = false;
	return 0;
}
#endif /* CONFIG_ADC_ADS7066_CRC */

static int ads7066_write_register_read_conversion(const struct device *dev, const uint8_t address,
						  const uint8_t value, uint16_t *result,
						  const struct ads7066_cmd_options *options)
{
	int err;

#ifdef CONFIG_ADC_ADS7066_CRC
	struct ads7066_data *data = dev->data;

	if (data->crcerr_in) {
		err = ads7066_clear_crcerr_in(dev);
		if (err) {
			return err;
		}
	}
#endif

	err = ads7066_spi_write_cmd_read_conversion(dev, ADS7066_CMD_REG_WRITE, address, value,
						    result, options);
	if (err) {
		return err;
	}

	/* Read back the value we just wrote to confirm success. */
	uint8_t reg;

	err = ads7066_read_single_register(dev, address, &reg, options);
	if (err) {
		return err;
	}

	if (reg != value) {
		LOG_ERR("register write failed 0x%02x != 0x%02x", reg, value);
		return -EIO;
	}

	return 0;
}

static int ads7066_write_single_register(const struct device *dev, const uint8_t address,
					 const uint8_t value,
					 const struct ads7066_cmd_options *options)
{
	return ads7066_write_register_read_conversion(dev, address, value, NULL, options);
}

static int ads7066_set_register_bits(const struct device *dev, const uint8_t address, uint8_t mask,
				     const struct ads7066_cmd_options *options)
{
	int err;

#ifdef CONFIG_ADC_ADS7066_CRC
	struct ads7066_data *data = dev->data;

	if (data->crcerr_in) {
		err = ads7066_clear_crcerr_in(dev);
		if (err) {
			return err;
		}
	}
#endif

	err = ads7066_write_cmd(dev, ADS7066_CMD_SET_BITS, address, mask, options);
	if (err) {
		return err;
	}

	uint8_t result;

	err = ads7066_read_single_register(dev, address, &result, options);
	if (err) {
		return err;
	}

	/* Exception */
	if (address == ADS7066_REG_SYSTEM_STATUS) {
		if (mask & ADS7066_STATUS_BOR) {
			WRITE_BIT(mask, 0, 0);
		}

		if (mask & ADS7066_STATUS_CRCERR_IN) {
			WRITE_BIT(mask, 1, 0);
		}
	}

	if ((result & mask) != mask) {
		LOG_ERR("register bit set failed 0x%02x != 0x%02x", result, result | mask);
		return -EIO;
	}

	return 0;
}

__unused static int ads7066_clear_register_bits(const struct device *dev, const uint8_t address,
						const uint8_t mask)
{
	int err;

#ifdef CONFIG_ADC_ADS7066_CRC
	struct ads7066_data *data = dev->data;

	if (data->crcerr_in) {
		err = ads7066_clear_crcerr_in(dev);
		if (err) {
			return err;
		}
	}
#endif

	err = ads7066_write_cmd(dev, ADS7066_CMD_CLEAR_BITS, address, mask, NULL);
	if (err) {
		return err;
	}

	uint8_t result;

	err = ads7066_read_single_register(dev, address, &result, NULL);
	if (err) {
		return err;
	}

	if ((result & mask) != 0) {
		LOG_ERR("register bit clear failed 0x%02x != 0x%02x", result, result & ~mask);
		return -EIO;
	}

	return 0;
}

static int ads7066_reset_registers(const struct device *dev)
{
	int err;

#ifdef CONFIG_ADC_ADS7066_CRC
	struct ads7066_data *data = dev->data;

	/* Attempt to reset with CRC enabled first */
	data->crc_enabled = true;
	struct ads7066_cmd_options options = {
		.ignore_status_flags = true,
		.ignore_crc = true,
	};

	/* Try to reset */
	err = ads7066_write_cmd(dev, ADS7066_CMD_SET_BITS, ADS7066_REG_GENERAL_CFG, ADS7066_CFG_RST,
				&options);
	if (err) {
		return err;
	}

	/* We ignore any errors and try one more time, this time with CRC disabled. */
	data->crc_enabled = false;
#endif
	err = ads7066_write_cmd(dev, ADS7066_CMD_SET_BITS, ADS7066_REG_GENERAL_CFG, ADS7066_CFG_RST,
				NULL);
	if (err) {
		return err;
	}

	k_msleep(ADS7066_TIME_RST);
	return 0;
}

#ifdef CONFIG_ADC_ADS7066_CRC
static int ads7066_enable_crc(const struct device *dev)
{
	struct ads7066_data *data = dev->data;

	struct ads7066_cmd_options options = {
		.status_flags = STATUS_NEVER,
	};

	/* We enable the extra status flags to capture CRC errors from host to device.
	 * This could also be read from ADS7066_STATUS_CRCERR_IN, but that would require a register
	 * read and we would not know when the error occurred.
	 */
	int err = ads7066_set_register_bits(dev, ADS7066_REG_DATA_CFG, ADS7066_CFG_APPEND_STAT,
					    &options);

	if (err) {
		return err;
	}

	options.status_flags = STATUS_ALWAYS;

	err = ads7066_write_cmd(dev, ADS7066_CMD_SET_BITS, ADS7066_REG_GENERAL_CFG,
				ADS7066_CFG_CRC_EN, &options);
	if (err) {
		return err;
	}

	data->crc_enabled = true;

	uint8_t result;

	err = ads7066_read_single_register(dev, ADS7066_REG_GENERAL_CFG, &result, NULL);
	if (err) {
		return err;
	}

	if (!(result & ADS7066_CFG_CRC_EN)) {
		LOG_ERR("failed to enable CRC");
		return -EIO;
	}

	return 0;
}
#endif /* CONFIG_ADC_ADS7066_CRC */

static int ads7066_calibrate(const struct device *dev)
{
	/*
	 * For some reason, status flags aren't transmitted properly during calibration and must be
	 * ignored, otherwise we sometimes get spurious input CRC errors.
	 */
	struct ads7066_cmd_options options = {
		.ignore_status_flags = true,
	};

	int err = ads7066_set_register_bits(dev, ADS7066_REG_GENERAL_CFG, ADS7066_CFG_CAL,
					    &options);

	if (err) {
		return err;
	}

	uint8_t count = 0;
	uint8_t reg;

	do {
		err = ads7066_read_single_register(dev, ADS7066_REG_GENERAL_CFG, &reg, &options);
		if (err) {
			return err;
		}

		if (count > ADS7066_CALIBRATION_TIMEOUT) {
			LOG_ERR("calibration timed out");
			return -ETIMEDOUT;
		}

		k_usleep(ADS7066_CALIBRATION_POLL_INTERVAL);
		count++;
	} while (reg & ADS7066_CFG_CAL);

	return 0;
}

static int adc_ads7066_channel_setup(const struct device *dev,
				     const struct adc_channel_cfg *channel_cfg)
{
	const struct ads7066_config *config = dev->config;

	if (channel_cfg->channel_id >= config->channels) {
		LOG_ERR("invalid channel %d", channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("acquisition time is not configurable");
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		LOG_ERR("differential channels are not supported");
		return -ENOTSUP;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("input gain is not supported");
		return -ENOTSUP;
	}

	if (channel_cfg->reference != config->reference) {
		LOG_ERR("all channels must use the same reference (internal or external).");
		return -EINVAL;
	}

	return 0;
}

static int ads7066_validate_buffer_size(const struct device *dev, const struct adc_sequence *seq)
{
	uint8_t channels = 0;
	size_t needed;

	channels = POPCOUNT(seq->channels);

	needed = channels * sizeof(uint16_t);
	if (seq->options) {
		needed *= (1 + seq->options->extra_samplings);
	}

	if (seq->buffer_size < needed) {
		LOG_ERR("buffer size %u is insufficient, need %u bytes", seq->buffer_size, needed);
		return -ENOMEM;
	}

	return 0;
}

static uint8_t ads7066_next_channel(const struct device *dev, uint8_t channel)
{
	struct ads7066_data *data = dev->data;
	uint8_t search_index = channel + 1;

	__ASSERT(search_index <= ADS7066_MAX_CHANNELS, "invalid channel search index %d",
		 search_index);
	uint8_t next_channel = find_lsb_set(data->channels_mask & (0xFF << search_index)) - 1;

	return next_channel == 0xFF ? find_lsb_set(data->channels_mask) - 1 : next_channel;
}

static int ads7066_read_sample_manual(const struct device *dev, uint8_t channel,
				      uint8_t next_channel, uint16_t *result)
{
	struct ads7066_data *data = dev->data;
	int err;

	if (channel != data->channel) {
		/* 7.4.2 Manual Mode, Fig. 7-13
		 * Switch to channel, we can read the first sample in frame N+2.
		 */
		err = ads7066_write_single_register(dev, ADS7066_REG_CHANNEL_SEL, channel, NULL);
		if (err) {
			LOG_ERR("failed to select channel %d", (int)channel);
			return err;
		}
		data->channel = channel;
	}

	/* We send NOP or channel switch to receive the sample from the **last** conversion.
	 * frame N if no channel switch was needed, N+3 otherwise.
	 */

	if (next_channel != channel) {
		err = ads7066_write_register_read_conversion(dev, ADS7066_REG_CHANNEL_SEL,
							     next_channel, result, NULL);
		data->channel = next_channel;
	} else {
		err = ads7066_read_conversion(dev, result, NULL);
	}

	return err;
}

static int ads7066_read_sample_otf(const struct device *dev, uint8_t channel, uint8_t next_channel,
				   uint16_t *result)
{
	struct ads7066_data *data = dev->data;
	int err;

	struct ads7066_cmd buf = {
		.tx_buf = {0},
		.tx_len = 1,
		.rx_buf = {0},
		.rx_len = 2,
	};

	if (channel != data->channel) {
		buf.tx_buf[0] = ADS7066_OTF_START_BIT | (channel << 3);
		err = ads7066_spi_transceive(dev, &buf);
		data->channel = channel;
	}

	if (next_channel != channel) {
		buf.tx_buf[0] = ADS7066_OTF_START_BIT | (next_channel << 3);
	} else {
		buf.tx_buf[0] = 0;
	}

	err = ads7066_spi_transceive(dev, &buf);
	data->channel = next_channel;
	*result = sys_get_be16(buf.rx_buf);

	return err;
}

static int ads7066_read_sample_auto(const struct device *dev, uint8_t channel, uint8_t next_channel,
				    uint16_t *result)
{
	struct ads7066_data *data = dev->data;
	int err;

	if (channel != data->channel) {
		LOG_ERR("wrong channel in auto read! expected %d, got %d", data->channel, channel);
		return -EINVAL;
	}

	err = ads7066_read_conversion(dev, result, NULL);
	data->channel = ads7066_next_channel(dev, channel);

	return err;
}

static int ads7066_read_sample(const struct device *dev, uint8_t channel, uint8_t next_channel,
			       uint16_t *result)
{
	const struct ads7066_config *config = dev->config;
	int err;

	switch (config->mode) {
	case ADS7066_MODE_MANUAL:
		err = ads7066_read_sample_manual(dev, channel, next_channel, result);
		break;
	case ADS7066_MODE_OTF:
		err = ads7066_read_sample_otf(dev, channel, next_channel, result);
		break;
	case ADS7066_MODE_AUTO:
		err = ads7066_read_sample_auto(dev, channel, next_channel, result);
		break;
	default:
		err = -ENOTSUP;
		break;
	}

	return err;
}

static int adc_ads7066_start_read(const struct device *dev, const struct adc_sequence *seq,
				  bool wait)
{
	const struct ads7066_config *config = dev->config;
	struct ads7066_data *data = dev->data;
	int err;

	if (seq->resolution != config->resolution) {
		LOG_ERR("unsupported resolution %u", seq->resolution);
		return -ENOTSUP;
	}

	if (find_msb_set(seq->channels) > config->channels) {
		LOG_ERR("unsupported channels in mask 0x%04x", seq->channels);
		return -ENOTSUP;
	}

	if (seq->calibrate) {
		err = ads7066_calibrate(dev);
		if (err) {
			return err;
		}
	}

	err = ads7066_validate_buffer_size(dev, seq);
	if (err) {
		return err;
	}

	data->buffer = seq->buffer;
	adc_context_start_read(&data->ctx, seq);

	if (data->channel == find_lsb_set(data->channels_mask) - 1) {
		/*
		 * The next conversion result would come from the last transceive which
		 * could have been a long time ago, therefore we must "flush" the previous
		 * conversion in order not to read an old conversion result.
		 */
		err = ads7066_read_conversion(dev, NULL, NULL);
		if (err) {
			return err;
		}

		if (config->mode == ADS7066_MODE_AUTO) {
			data->channel = ads7066_next_channel(dev, data->channel);
		}
	}

	if (wait) {
		return adc_context_wait_for_completion(&data->ctx);
	} else {
		return 0;
	}
}

static int ads7066_setup_auto_sample_read(const struct device *dev)
{
	struct ads7066_data *data = dev->data;
	int err;

	if (data->auto_reg_channels_sel == data->channels_mask) {
		return 0;
	}

	err = ads7066_write_single_register(dev, ADS7066_REG_SEQUENCE_CFG, 0, NULL);
	if (err) {
		LOG_ERR("failed to disable auto sequencing for channel mask reconfiguration");
		return err;
	}

	err = ads7066_write_single_register(dev, ADS7066_REG_AUTO_SEQ_CH_SEL, data->channels_mask,
					    NULL);
	if (err) {
		LOG_ERR("failed to set up channels for auto sequencing");
		return err;
	}

	data->auto_reg_channels_sel = data->channels_mask;
	uint8_t first_channel = find_lsb_set(data->channels_mask) - 1;

	data->channel = ads7066_next_channel(dev, first_channel);

	err = ads7066_write_single_register(dev, ADS7066_REG_SEQUENCE_CFG,
					    ADS7066_CFG_SEQ_MODE_AUTO | ADS7066_CFG_SEQ_START,
					    NULL);
	if (err) {
		LOG_ERR("failed to enable auto sequencing");
		return err;
	}

	return 0;
}

static int ads7066_read_samples(const struct device *dev)
{
	struct ads7066_data *data = dev->data;
	const struct ads7066_config *config = dev->config;
	uint8_t channels_mask_start = data->channels_mask;
	uint8_t total_channels = POPCOUNT(data->channels_mask);
	uint8_t channel = find_lsb_set(data->channels_mask) - 1;
	uint8_t next_channel;
	uint16_t result = 0;
	int err;

	if (config->mode == ADS7066_MODE_AUTO) {
		err = ads7066_setup_auto_sample_read(dev);
		if (err) {
			LOG_ERR("failed to set up auto sample read");
			return err;
		}
		channel = data->channel;
	}

	while (data->channels_mask) {
		next_channel = ads7066_next_channel(dev, channel);

		err = ads7066_read_sample(dev, channel, next_channel, &result);
		if (err) {
			LOG_ERR("failed to read channel %d (%d)", (int)channel, err);
			return err;
		}

		uint8_t channel_idx = POPCOUNT(channels_mask_start & ((1 << channel) - 1));

		data->buffer[channel_idx] = result;
		channel = next_channel;
		WRITE_BIT(data->channels_mask, channel, 0);
	}

	data->buffer += total_channels;

	return 0;
}

#ifdef CONFIG_ADC_ASYNC
static int adc_ads7066_read_async(const struct device *dev, const struct adc_sequence *seq,
				  struct k_poll_signal *async)
{
	struct ads7066_data *data = dev->data;
	int err;

	adc_context_lock(&data->ctx, async ? true : false, async);
	err = adc_ads7066_start_read(dev, seq, async ? false : true);
	adc_context_release(&data->ctx, err);
	return err;
}

static int adc_ads7066_read(const struct device *dev, const struct adc_sequence *seq)
{
	return adc_ads7066_read_async(dev, seq, NULL);
}
#else
static int adc_ads7066_read(const struct device *dev, const struct adc_sequence *seq)
{
	struct ads7066_data *data = dev->data;
	int err;

	adc_context_lock(&data->ctx, false, NULL);

	err = adc_ads7066_start_read(dev, seq, false);
	while (err == 0 && k_sem_take(&data->sem, K_NO_WAIT) == 0) {
		err = ads7066_read_samples(dev);
		if (err) {
			adc_context_complete(&data->ctx, err);
		} else {
			adc_context_on_sampling_done(&data->ctx, dev);
		}
	}

	adc_context_release(&data->ctx, err);

	return err;
}
#endif

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct ads7066_data *data = CONTAINER_OF(ctx, struct ads7066_data, ctx);

	data->channels_mask = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;

	k_sem_give(&data->sem);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct ads7066_data *data = CONTAINER_OF(ctx, struct ads7066_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

#ifdef CONFIG_ADC_ASYNC
static void ads7066_acquisition_thread(void *p1, void *p2, void *p3)
{
	struct ads7066_data *data = p1;
	const struct device *dev = data->dev;
	int err;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_sem_take(&data->sem, K_FOREVER);

		err = ads7066_read_samples(dev);
		if (err) {
			adc_context_complete(&data->ctx, err);
		} else {
			adc_context_on_sampling_done(&data->ctx, data->dev);
		}
	}
}
#endif

static int adc_ads7066_configure(const struct device *dev)
{
	const struct ads7066_config *config = dev->config;
	int err;

	/* Reset registers so we start from a known state. */
	err = ads7066_reset_registers(dev);
	if (err) {
		LOG_ERR("failed to reset registers");
		return err;
	}

	/* Check power-up configuration. */
	uint8_t reg;

	err = ads7066_read_single_register(dev, ADS7066_REG_SYSTEM_STATUS, &reg, NULL);
	if (err) {
		return err;
	}

	if (reg & ADS7066_STATUS_CRCERR_FUSE) {
		LOG_ERR("power-up configuration crc status failed");
		return -EIO;
	}

	if (config->mode == ADS7066_MODE_OTF) {
		err = ads7066_write_single_register(dev, ADS7066_REG_SEQUENCE_CFG,
						    ADS7066_CFG_SEQ_MODE_OTF, NULL);
		if (err) {
			return err;
		}
	}

#ifdef CONFIG_ADC_ADS7066_CRC
	if (config->mode == ADS7066_MODE_OTF) {
		LOG_ERR("On-the-fly mode does not support CRC");
		return -EINVAL;
	}

	err = ads7066_enable_crc(dev);
	if (err) {
		return err;
	}

#endif /* CONFIG_ADC_ADS7066_CRC */

	if (config->reference == ADC_REF_INTERNAL) {
		err = ads7066_set_register_bits(dev, ADS7066_REG_GENERAL_CFG, ADS7066_CFG_REF_EN,
						NULL);
		if (err) {
			return err;
		}
	}

	/* Clear BOR */
	err = ads7066_set_register_bits(dev, ADS7066_REG_SYSTEM_STATUS, ADS7066_STATUS_BOR, NULL);
	if (err) {
		return err;
	}

	err = ads7066_calibrate(dev);
	if (err) {
		return err;
	}

	return 0;
}

static int ads7066_sanity_check(const struct device *dev)
{
	uint16_t conversion;
	int err;

#ifdef CONFIG_ADC_ADS7066_CRC
	err = ads7066_read_conversion(dev, &conversion, NULL);
	return err;
#else
	/* Enable status flags */
	err = ads7066_write_single_register(dev, ADS7066_REG_DATA_CFG, ADS7066_CFG_APPEND_STAT,
					    NULL);
	if (err) {
		return err;
	}

	struct ads7066_cmd_options options = {
		.status_flags = STATUS_ALWAYS,
	};

	err = ads7066_read_conversion(dev, &conversion, &options);
	if (err) {
		return err;
	}

	options.ignore_status_flags = true;

	/* Disable status flags */
	err = ads7066_write_single_register(dev, ADS7066_REG_DATA_CFG, 0, &options);
#endif /* CONFIG_ADC_ADS7066_CRC */

	return err;
}

static int adc_ads7066_init(const struct device *dev)
{
	const struct ads7066_config *config = dev->config;
	struct ads7066_data *data = dev->data;
	int retry = 1;
	int err;

	data->dev = dev;

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	do {
		err = adc_ads7066_configure(dev);
		if (err) {
			LOG_ERR("configuration failed");
			return err;
		}

		/*
		 * It seems that sometimes (rarely) the ADC gets stuck with invalid conversion
		 * results and status flags after calibration. As a mitigation we try to read a
		 * conversion after calibration with status flags enabled and if an error is
		 * detected, we reset the ADC.
		 */
		err = ads7066_sanity_check(dev);
		if (err) {
			if (retry) {
				LOG_WRN("failed sanity check (%d), reset", err);
				retry--;
				continue;
			}

			LOG_ERR("failed sanity check again (%d)", err);
			return err;
		}
	} while (err);

	k_sem_init(&data->sem, 0, 1);

#ifdef CONFIG_ADC_ASYNC
	k_thread_create(&data->thread, data->stack, K_KERNEL_STACK_SIZEOF(data->stack),
			ads7066_acquisition_thread, data, NULL, NULL,
			CONFIG_ADC_ADS7066_ACQUISITION_THREAD_PRIO, 0, K_NO_WAIT);
#endif

	adc_context_unlock_unconditionally(&data->ctx);
	return 0;
}

static DEVICE_API(adc, ads7066_api) = {
	.channel_setup = adc_ads7066_channel_setup,
	.read = adc_ads7066_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_ads7066_read_async,
#endif
	.ref_internal = ADS7066_INTERNAL_REFERENCE_MV,
};

#define ADC_ADS7066_SPI_CFG SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB

#define ADC_ADS7066_INIT(n)                                                                        \
	static const struct ads7066_config ads7066_cfg_##n = {                                     \
		.bus = SPI_DT_SPEC_INST_GET(n, ADC_ADS7066_SPI_CFG, 4),                            \
		.resolution = ADS7066_RESOLUTION,                                                  \
		.channels = ADS7066_MAX_CHANNELS,                                                  \
		.reference = DT_INST_ENUM_IDX_OR(n, reference, 5),                                 \
		.mode = DT_INST_ENUM_IDX_OR(n, mode, 0),                                           \
	};                                                                                         \
                                                                                                   \
	static struct ads7066_data ads7066_data_##n = {                                            \
		ADC_CONTEXT_INIT_TIMER(ads7066_data_##n, ctx),                                     \
		ADC_CONTEXT_INIT_LOCK(ads7066_data_##n, ctx),                                      \
		ADC_CONTEXT_INIT_SYNC(ads7066_data_##n, ctx),                                      \
		.crc_enabled = false,                                                              \
		.crcerr_in = false,								   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, adc_ads7066_init, NULL, &ads7066_data_##n, &ads7066_cfg_##n,      \
			      POST_KERNEL, CONFIG_ADC_ADS7066_INIT_PRIORITY, &ads7066_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_ADS7066_INIT)
