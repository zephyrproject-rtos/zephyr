/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ADC driver for the LMP90xxx AFE.
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/lmp90xxx.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_lmp90xxx);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

/* LMP90xxx register addresses */
#define LMP90XXX_REG_RESETCN         0x00U
#define LMP90XXX_REG_SPI_HANDSHAKECN 0x01U
#define LMP90XXX_REG_SPI_RESET       0x02U
#define LMP90XXX_REG_SPI_STREAMCN    0x03U
#define LMP90XXX_REG_PWRCN           0x08U
#define LMP90XXX_REG_DATA_ONLY_1     0x09U
#define LMP90XXX_REG_DATA_ONLY_2     0x0AU
#define LMP90XXX_REG_ADC_RESTART     0x0BU
#define LMP90XXX_REG_GPIO_DIRCN      0x0EU
#define LMP90XXX_REG_GPIO_DAT        0x0FU
#define LMP90XXX_REG_BGCALCN         0x10U
#define LMP90XXX_REG_SPI_DRDYBCN     0x11U
#define LMP90XXX_REG_ADC_AUXCN       0x12U
#define LMP90XXX_REG_SPI_CRC_CN      0x13U
#define LMP90XXX_REG_SENDIAG_THLDH   0x14U
#define LMP90XXX_REG_SENDIAG_THLDL   0x15U
#define LMP90XXX_REG_SCALCN          0x17U
#define LMP90XXX_REG_ADC_DONE        0x18U
#define LMP90XXX_REG_SENDIAG_FLAGS   0x19U
#define LMP90XXX_REG_ADC_DOUT        0x1AU
#define LMP90XXX_REG_SPI_CRC_DAT     0x1DU
#define LMP90XXX_REG_CH_STS          0x1EU
#define LMP90XXX_REG_CH_SCAN         0x1FU

/* LMP90xxx channel input and configuration registers */
#define LMP90XXX_REG_CH_INPUTCN(ch) (0x20U + (2 * ch))
#define LMP90XXX_REG_CH_CONFIG(ch)  (0x21U + (2 * ch))

/* LMP90xxx upper (URA) and lower (LRA) register addresses */
#define LMP90XXX_URA(addr) ((addr >> 4U) & GENMASK(2, 0))
#define LMP90XXX_LRA(addr) (addr & GENMASK(3, 0))

/* LMP90xxx instruction byte 1 (INST1) */
#define LMP90XXX_INST1_WAB 0x10U
#define LMP90XXX_INST1_RA  0x90U

/* LMP90xxx instruction byte 2 (INST2) */
#define LMP90XXX_INST2_WB        0U
#define LMP90XXX_INST2_R         BIT(7)
#define LMP90XXX_INST2_SZ_1      (0x0U << 5)
#define LMP90XXX_INST2_SZ_2      (0x1U << 5)
#define LMP90XXX_INST2_SZ_3      (0x2U << 5)
#define LMP90XXX_INST2_SZ_STREAM (0x3U << 5)

/* LMP90xxx register values/commands */
#define LMP90XXX_REG_AND_CNV_RST     0xC3U
#define LMP90XXX_SDO_DRDYB_DRIVER(x) ((x & BIT_MASK(3)) << 1)
#define LMP90XXX_PWRCN(x)            (x & BIT_MASK(2))
#define LMP90XXX_RTD_CUR_SEL(x)      (x & BIT_MASK(4))
#define LMP90XXX_SPI_DRDYB_D6(x)     ((x & BIT(0)) << 7)
#define LMP90XXX_EN_CRC(x)           ((x & BIT(0)) << 4)
#define LMP90XXX_DRDYB_AFT_CRC(x)    ((x & BIT(0)) << 2)
#define LMP90XXX_CH_SCAN_SEL(x)      ((x & BIT_MASK(2)) << 6)
#define LMP90XXX_LAST_CH(x)          ((x & BIT_MASK(3)) << 3)
#define LMP90XXX_FIRST_CH(x)         (x & BIT_MASK(3))
#define LMP90XXX_BURNOUT_EN(x)       ((x & BIT(0)) << 7)
#define LMP90XXX_VREF_SEL(x)         ((x & BIT(0)) << 6)
#define LMP90XXX_VINP(x)             ((x & BIT_MASK(3)) << 3)
#define LMP90XXX_VINN(x)             (x & BIT_MASK(3))
#define LMP90XXX_BGCALN(x)           (x & BIT_MASK(3))
#define LMP90XXX_ODR_SEL(x)          ((x & BIT_MASK(3)) << 4)
#define LMP90XXX_GAIN_SEL(x)         ((x & BIT_MASK(3)) << 1)
#define LMP90XXX_BUF_EN(x)           (x & BIT(0))
#define LMP90XXX_GPIO_DAT_MASK       BIT_MASK(LMP90XXX_GPIO_MAX)

/* Invalid (never used) Upper Register Address */
#define LMP90XXX_INVALID_URA UINT8_MAX

/* Maximum number of ADC channels */
#define LMP90XXX_MAX_CHANNELS 7

/* Maximum number of ADC inputs */
#define LMP90XXX_MAX_INPUTS 8

/* Default Output Data Rate (ODR) is 214.65 SPS */
#define LMP90XXX_DEFAULT_ODR 7

/* Macro for checking if Data Ready Bar IRQ is in use */
#define LMP90XXX_HAS_DRDYB(config) (config->drdyb.port != NULL)

struct lmp90xxx_config {
	struct spi_dt_spec bus;
	struct gpio_dt_spec drdyb;
	uint8_t rtd_current;
	uint8_t resolution;
	uint8_t channels;
};

struct lmp90xxx_data {
	struct adc_context ctx;
	const struct device *dev;
	struct gpio_callback drdyb_cb;
	struct k_mutex ura_lock;
	uint8_t ura;
	int32_t *buffer;
	int32_t *repeat_buffer;
	uint32_t channels;
	bool calibrate;
	uint8_t channel_odr[LMP90XXX_MAX_CHANNELS];
#ifdef CONFIG_ADC_LMP90XXX_GPIO
	struct k_mutex gpio_lock;
	uint8_t gpio_dircn;
	uint8_t gpio_dat;
#endif /* CONFIG_ADC_LMP90XXX_GPIO */
	struct k_thread thread;
	struct k_sem acq_sem;
	struct k_sem drdyb_sem;

	K_KERNEL_STACK_MEMBER(stack,
			CONFIG_ADC_LMP90XXX_ACQUISITION_THREAD_STACK_SIZE);
};

/*
 * Approximated LMP90xxx acquisition times in milliseconds. These are
 * used for the initial delay when polling for data ready.
 */
static const int32_t lmp90xxx_odr_delay_tbl[8] = {
	596, /* 13.42/8 = 1.6775 SPS */
	298, /* 13.42/4 = 3.355 SPS */
	149, /* 13.42/2 = 6.71 SPS */
	75,  /* 13.42 SPS */
	37,  /* 214.65/8 = 26.83125 SPS */
	19,  /* 214.65/4 = 53.6625 SPS */
	9,   /* 214.65/2 = 107.325 SPS */
	5,   /* 214.65 SPS (default) */
};

static inline uint8_t lmp90xxx_inst2_sz(size_t len)
{
	if (len == 1) {
		return LMP90XXX_INST2_SZ_1;
	} else if (len == 2) {
		return LMP90XXX_INST2_SZ_2;
	} else if (len == 3) {
		return LMP90XXX_INST2_SZ_3;
	} else {
		return LMP90XXX_INST2_SZ_STREAM;
	}
}

static int lmp90xxx_read_reg(const struct device *dev, uint8_t addr,
			     uint8_t *dptr,
			     size_t len)
{
	const struct lmp90xxx_config *config = dev->config;
	struct lmp90xxx_data *data = dev->data;
	uint8_t ura = LMP90XXX_URA(addr);
	uint8_t inst1_uab[2] = { LMP90XXX_INST1_WAB, ura };
	uint8_t inst2 = LMP90XXX_INST2_R | LMP90XXX_LRA(addr);
	struct spi_buf tx_buf[2];
	struct spi_buf rx_buf[2];
	struct spi_buf_set tx;
	struct spi_buf_set rx;
	int dummy = 0;
	int i = 0;
	int err;

	if (len == 0) {
		LOG_ERR("attempt to read 0 bytes from register 0x%02x", addr);
		return -EINVAL;
	}

	if (k_is_in_isr()) {
		/* Prevent SPI transactions from an ISR */
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&data->ura_lock, K_FOREVER);

	if (ura != data->ura) {
		/* Instruction Byte 1 + Upper Address Byte */
		tx_buf[i].buf = inst1_uab;
		tx_buf[i].len = sizeof(inst1_uab);
		dummy += sizeof(inst1_uab);
		i++;
	}

	/* Instruction Byte 2 */
	inst2 |= lmp90xxx_inst2_sz(len);
	tx_buf[i].buf = &inst2;
	tx_buf[i].len = sizeof(inst2);
	dummy += sizeof(inst2);
	i++;

	/* Dummy RX Bytes */
	rx_buf[0].buf = NULL;
	rx_buf[0].len = dummy;

	/* Data Byte(s) */
	rx_buf[1].buf = dptr;
	rx_buf[1].len = len;

	tx.buffers = tx_buf;
	tx.count = i;
	rx.buffers = rx_buf;
	rx.count = 2;

	err = spi_transceive_dt(&config->bus, &tx, &rx);
	if (!err) {
		data->ura = ura;
	} else {
		/* Force INST1 + UAB on next access */
		data->ura = LMP90XXX_INVALID_URA;
	}

	k_mutex_unlock(&data->ura_lock);

	return err;
}

static int lmp90xxx_read_reg8(const struct device *dev, uint8_t addr,
			      uint8_t *val)
{
	return lmp90xxx_read_reg(dev, addr, val, sizeof(*val));
}

static int lmp90xxx_write_reg(const struct device *dev, uint8_t addr,
			      uint8_t *dptr,
			      size_t len)
{
	const struct lmp90xxx_config *config = dev->config;
	struct lmp90xxx_data *data = dev->data;
	uint8_t ura = LMP90XXX_URA(addr);
	uint8_t inst1_uab[2] = { LMP90XXX_INST1_WAB, ura };
	uint8_t inst2 = LMP90XXX_INST2_WB | LMP90XXX_LRA(addr);
	struct spi_buf tx_buf[3];
	struct spi_buf_set tx;
	int i = 0;
	int err;

	if (len == 0) {
		LOG_ERR("attempt write 0 bytes to register 0x%02x", addr);
		return -EINVAL;
	}

	if (k_is_in_isr()) {
		/* Prevent SPI transactions from an ISR */
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&data->ura_lock, K_FOREVER);

	if (ura != data->ura) {
		/* Instruction Byte 1 + Upper Address Byte */
		tx_buf[i].buf = inst1_uab;
		tx_buf[i].len = sizeof(inst1_uab);
		i++;
	}

	/* Instruction Byte 2 */
	inst2 |= lmp90xxx_inst2_sz(len);
	tx_buf[i].buf = &inst2;
	tx_buf[i].len = sizeof(inst2);
	i++;

	/* Data Byte(s) */
	tx_buf[i].buf = dptr;
	tx_buf[i].len = len;
	i++;

	tx.buffers = tx_buf;
	tx.count = i;

	err = spi_write_dt(&config->bus, &tx);
	if (!err) {
		data->ura = ura;
	} else {
		/* Force INST1 + UAB on next access */
		data->ura = LMP90XXX_INVALID_URA;
	}

	k_mutex_unlock(&data->ura_lock);

	return err;
}

static int lmp90xxx_write_reg8(const struct device *dev, uint8_t addr,
			       uint8_t val)
{
	return lmp90xxx_write_reg(dev, addr, &val, sizeof(val));
}

static int lmp90xxx_soft_reset(const struct device *dev)
{
	int err;

	err = lmp90xxx_write_reg8(dev, LMP90XXX_REG_RESETCN,
				  LMP90XXX_REG_AND_CNV_RST);
	if (err) {
		return err;
	}

	/* Write to RESETCN twice in order to reset mode as well as registers */
	return lmp90xxx_write_reg8(dev, LMP90XXX_REG_RESETCN,
				   LMP90XXX_REG_AND_CNV_RST);
}

static inline bool lmp90xxx_has_channel(const struct device *dev,
					uint8_t channel)
{
	const struct lmp90xxx_config *config = dev->config;

	if (channel >= config->channels) {
		return false;
	} else {
		return true;
	}
}

static inline bool lmp90xxx_has_input(const struct device *dev, uint8_t input)
{
	const struct lmp90xxx_config *config = dev->config;

	if (input >= LMP90XXX_MAX_INPUTS) {
		return false;
	} else if (config->channels < LMP90XXX_MAX_CHANNELS &&
		   (input >= 3 && input <= 5)) {
		/* This device only has inputs 0, 1, 2, 6, and 7 */
		return false;
	} else {
		return true;
	}
}

static inline int lmp90xxx_acq_time_to_odr(uint16_t acq_time)
{
	uint16_t acq_value;

	if (acq_time == ADC_ACQ_TIME_DEFAULT) {
		return LMP90XXX_DEFAULT_ODR;
	}

	if (ADC_ACQ_TIME_UNIT(acq_time) != ADC_ACQ_TIME_TICKS) {
		return -EINVAL;
	}

	/*
	 * The LMP90xxx supports odd (and very slow) output data
	 * rates. Allow the caller to specify the ODR directly using
	 * ADC_ACQ_TIME_TICKS
	 */
	acq_value = ADC_ACQ_TIME_VALUE(acq_time);
	if (acq_value <= LMP90XXX_DEFAULT_ODR) {
		return acq_value;
	}

	return -EINVAL;
}

static int lmp90xxx_adc_channel_setup(const struct device *dev,
				      const struct adc_channel_cfg *channel_cfg)
{
	struct lmp90xxx_data *data = dev->data;
	uint8_t chx_inputcn = LMP90XXX_BURNOUT_EN(0); /* No burnout currents */
	uint8_t chx_config = LMP90XXX_BUF_EN(0);      /* No buffer */
	uint8_t payload[2];
	uint8_t addr;
	int ret;

	switch (channel_cfg->reference) {
	case ADC_REF_EXTERNAL0:
		chx_inputcn |= LMP90XXX_VREF_SEL(0);
		break;
	case ADC_REF_EXTERNAL1:
		chx_inputcn |= LMP90XXX_VREF_SEL(1);
		break;
	default:
		LOG_ERR("unsupported channel reference type '%d'",
			channel_cfg->reference);
		return -ENOTSUP;
	}

	if (!lmp90xxx_has_channel(dev, channel_cfg->channel_id)) {
		LOG_ERR("unsupported channel id '%d'", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (!lmp90xxx_has_input(dev, channel_cfg->input_positive)) {
		LOG_ERR("unsupported positive input '%d'",
			channel_cfg->input_positive);
		return -ENOTSUP;
	}
	chx_inputcn |= LMP90XXX_VINP(channel_cfg->input_positive);

	if (!lmp90xxx_has_input(dev, channel_cfg->input_negative)) {
		LOG_ERR("unsupported negative input '%d'",
			channel_cfg->input_negative);
		return -ENOTSUP;
	}
	chx_inputcn |= LMP90XXX_VINN(channel_cfg->input_negative);

	ret = lmp90xxx_acq_time_to_odr(channel_cfg->acquisition_time);
	if (ret < 0) {
		LOG_ERR("unsupported channel acquisition time 0x%02x",
			channel_cfg->acquisition_time);
		return -ENOTSUP;
	}
	chx_config |= LMP90XXX_ODR_SEL(ret);
	data->channel_odr[channel_cfg->channel_id] = ret;

	switch (channel_cfg->gain) {
	case ADC_GAIN_1:
		chx_config |= LMP90XXX_GAIN_SEL(0);
		break;
	case ADC_GAIN_2:
		chx_config |= LMP90XXX_GAIN_SEL(1);
		break;
	case ADC_GAIN_4:
		chx_config |= LMP90XXX_GAIN_SEL(2);
		break;
	case ADC_GAIN_8:
		chx_config |= LMP90XXX_GAIN_SEL(3);
		break;
	case ADC_GAIN_16:
		chx_config |= LMP90XXX_GAIN_SEL(4);
		break;
	case ADC_GAIN_32:
		chx_config |= LMP90XXX_GAIN_SEL(5);
		break;
	case ADC_GAIN_64:
		chx_config |= LMP90XXX_GAIN_SEL(6);
		break;
	case ADC_GAIN_128:
		chx_config |= LMP90XXX_GAIN_SEL(7);
		break;
	default:
		LOG_ERR("unsupported channel gain '%d'", channel_cfg->gain);
		return -ENOTSUP;
	}

	payload[0] = chx_inputcn;
	payload[1] = chx_config;

	addr = LMP90XXX_REG_CH_INPUTCN(channel_cfg->channel_id);
	ret = lmp90xxx_write_reg(dev, addr, payload, sizeof(payload));
	if (ret) {
		LOG_ERR("failed to configure channel (err %d)", ret);
	}

	return ret;
}

static int lmp90xxx_validate_buffer_size(const struct adc_sequence *sequence)
{
	uint8_t channels = 0;
	size_t needed;
	uint32_t mask;

	for (mask = BIT(LMP90XXX_MAX_CHANNELS - 1); mask != 0; mask >>= 1) {
		if (mask & sequence->channels) {
			channels++;
		}
	}

	needed = channels * sizeof(int32_t);
	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

static int lmp90xxx_adc_start_read(const struct device *dev,
				   const struct adc_sequence *sequence)
{
	const struct lmp90xxx_config *config = dev->config;
	struct lmp90xxx_data *data = dev->data;
	int err;

	if (sequence->resolution != config->resolution) {
		LOG_ERR("unsupported resolution %d", sequence->resolution);
		return -ENOTSUP;
	}

	if (!lmp90xxx_has_channel(dev, find_msb_set(sequence->channels) - 1)) {
		LOG_ERR("unsupported channels in mask: 0x%08x",
			sequence->channels);
		return -ENOTSUP;
	}

	err = lmp90xxx_validate_buffer_size(sequence);
	if (err) {
		LOG_ERR("buffer size too small");
		return err;
	}

	data->buffer = sequence->buffer;
	data->calibrate = sequence->calibrate;
	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int lmp90xxx_adc_read_async(const struct device *dev,
				   const struct adc_sequence *sequence,
				   struct k_poll_signal *async)
{
	struct lmp90xxx_data *data = dev->data;
	int err;

	adc_context_lock(&data->ctx, async ? true : false, async);
	err = lmp90xxx_adc_start_read(dev, sequence);
	adc_context_release(&data->ctx, err);

	return err;
}

static int lmp90xxx_adc_read(const struct device *dev,
			     const struct adc_sequence *sequence)
{
	return lmp90xxx_adc_read_async(dev, sequence, NULL);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct lmp90xxx_data *data =
		CONTAINER_OF(ctx, struct lmp90xxx_data, ctx);

	data->channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;

	k_sem_give(&data->acq_sem);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct lmp90xxx_data *data =
		CONTAINER_OF(ctx, struct lmp90xxx_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int lmp90xxx_adc_read_channel(const struct device *dev,
				     uint8_t channel,
				     int32_t *result)
{
	const struct lmp90xxx_config *config = dev->config;
	struct lmp90xxx_data *data = dev->data;
	uint8_t adc_done;
	uint8_t ch_scan;
	uint8_t buf[4]; /* ADC_DOUT + CRC */
	int32_t delay;
	uint8_t odr;
	int err;

	/* Single channel, single scan mode */
	ch_scan = LMP90XXX_CH_SCAN_SEL(0x1) | LMP90XXX_FIRST_CH(channel) |
		  LMP90XXX_LAST_CH(channel);

	err = lmp90xxx_write_reg8(dev, LMP90XXX_REG_CH_SCAN, ch_scan);
	if (err) {
		LOG_ERR("failed to setup scan channels (err %d)", err);
		return err;
	}

	/* Start scan */
	err = lmp90xxx_write_reg8(dev, LMP90XXX_REG_PWRCN, LMP90XXX_PWRCN(0));
	if (err) {
		LOG_ERR("failed to set active mode (err %d)", err);
		return err;
	}

	if (LMP90XXX_HAS_DRDYB(config)) {
		k_sem_take(&data->drdyb_sem, K_FOREVER);
	} else {
		odr = data->channel_odr[channel];
		delay = lmp90xxx_odr_delay_tbl[odr];
		LOG_DBG("sleeping for %d ms", delay);
		k_msleep(delay);

		/* Poll for data ready */
		do {
			err = lmp90xxx_read_reg8(dev, LMP90XXX_REG_ADC_DONE,
						&adc_done);
			if (err) {
				LOG_ERR("failed to read done (err %d)", err);
				return err;
			}

			if (adc_done == 0xFFU) {
				LOG_DBG("sleeping for 1 ms");
				k_msleep(1);
			} else {
				break;
			}
		} while (true);
	}

	if (IS_ENABLED(CONFIG_ADC_LMP90XXX_CRC)) {
		err = lmp90xxx_read_reg(dev, LMP90XXX_REG_ADC_DOUT, buf,
					sizeof(buf));
	} else {
		err = lmp90xxx_read_reg(dev, LMP90XXX_REG_ADC_DOUT, buf,
					config->resolution / 8);
	}

	if (err) {
		LOG_ERR("failed to read ADC DOUT (err %d)", err);
		return err;
	}

	if (IS_ENABLED(CONFIG_ADC_LMP90XXX_CRC)) {
		uint8_t crc = crc8(buf, 3, 0x31, 0, false) ^ 0xFFU;

		if (buf[3] != crc) {
			LOG_ERR("CRC mismatch (0x%02x vs. 0x%02x)", buf[3],
				crc);
			return err;
		}
	}

	/* Read result, get rid of CRC, and sign extend result */
	*result = (int32_t)sys_get_be32(buf);
	*result >>= (32 - config->resolution);

	return 0;
}

static void lmp90xxx_acquisition_thread(struct lmp90xxx_data *data)
{
	uint8_t bgcalcn = LMP90XXX_BGCALN(0x3); /* Default to BgCalMode3 */
	int32_t result = 0;
	uint8_t channel;
	int err;

	while (true) {
		k_sem_take(&data->acq_sem, K_FOREVER);

		if (data->calibrate) {
			/* Use BgCalMode2 */
			bgcalcn = LMP90XXX_BGCALN(0x2);
		}

		LOG_DBG("using BGCALCN = 0x%02x", bgcalcn);
		err = lmp90xxx_write_reg8(data->dev,
					  LMP90XXX_REG_BGCALCN, bgcalcn);
		if (err) {
			LOG_ERR("failed to setup background calibration "
				"(err %d)", err);
				adc_context_complete(&data->ctx, err);
				break;
		}

		while (data->channels) {
			channel = find_lsb_set(data->channels) - 1;

			LOG_DBG("reading channel %d", channel);

			err = lmp90xxx_adc_read_channel(data->dev,
							channel, &result);
			if (err) {
				adc_context_complete(&data->ctx, err);
				break;
			}

			LOG_DBG("finished channel %d, result = %d", channel,
				result);

			/*
			 * ADC samples are stored as int32_t regardless of the
			 * resolution in order to provide a uniform interface
			 * for the driver.
			 */
			*data->buffer++ = result;
			WRITE_BIT(data->channels, channel, 0);
		}

		adc_context_on_sampling_done(&data->ctx, data->dev);
	}
}

static void lmp90xxx_drdyb_callback(const struct device *port,
				    struct gpio_callback *cb, uint32_t pins)
{
	struct lmp90xxx_data *data =
		CONTAINER_OF(cb, struct lmp90xxx_data, drdyb_cb);

	/* Signal thread that data is now ready */
	k_sem_give(&data->drdyb_sem);
}

#ifdef CONFIG_ADC_LMP90XXX_GPIO
int lmp90xxx_gpio_set_output(const struct device *dev, uint8_t pin)
{
	struct lmp90xxx_data *data = dev->data;
	int err = 0;
	uint8_t tmp;

	if (pin > LMP90XXX_GPIO_MAX) {
		return -EINVAL;
	}

	k_mutex_lock(&data->gpio_lock, K_FOREVER);

	tmp = data->gpio_dircn | BIT(pin);
	if (tmp != data->gpio_dircn) {
		err = lmp90xxx_write_reg8(dev, LMP90XXX_REG_GPIO_DIRCN, tmp);
		if (!err) {
			data->gpio_dircn = tmp;
		}
	}

	k_mutex_unlock(&data->gpio_lock);

	return err;
}

int lmp90xxx_gpio_set_input(const struct device *dev, uint8_t pin)
{
	struct lmp90xxx_data *data = dev->data;
	int err = 0;
	uint8_t tmp;

	if (pin > LMP90XXX_GPIO_MAX) {
		return -EINVAL;
	}

	k_mutex_lock(&data->gpio_lock, K_FOREVER);

	tmp = data->gpio_dircn & ~BIT(pin);
	if (tmp != data->gpio_dircn) {
		err = lmp90xxx_write_reg8(dev, LMP90XXX_REG_GPIO_DIRCN, tmp);
		if (!err) {
			data->gpio_dircn = tmp;
		}
	}

	k_mutex_unlock(&data->gpio_lock);

	return err;
}

int lmp90xxx_gpio_set_pin_value(const struct device *dev, uint8_t pin,
				bool value)
{
	struct lmp90xxx_data *data = dev->data;
	int err = 0;
	uint8_t tmp;

	if (pin > LMP90XXX_GPIO_MAX) {
		return -EINVAL;
	}

	k_mutex_lock(&data->gpio_lock, K_FOREVER);

	tmp = data->gpio_dat;
	WRITE_BIT(tmp, pin, value);

	if (tmp != data->gpio_dat) {
		err = lmp90xxx_write_reg8(dev, LMP90XXX_REG_GPIO_DAT, tmp);
		if (!err) {
			data->gpio_dat = tmp;
		}
	}

	k_mutex_unlock(&data->gpio_lock);

	return err;
}

int lmp90xxx_gpio_get_pin_value(const struct device *dev, uint8_t pin,
				bool *value)
{
	struct lmp90xxx_data *data = dev->data;
	int err = 0;
	uint8_t tmp;

	if (pin > LMP90XXX_GPIO_MAX) {
		return -EINVAL;
	}

	k_mutex_lock(&data->gpio_lock, K_FOREVER);

	err = lmp90xxx_read_reg8(dev, LMP90XXX_REG_GPIO_DAT, &tmp);
	if (!err) {
		*value = tmp & BIT(pin);
	}

	k_mutex_unlock(&data->gpio_lock);

	return err;
}

int lmp90xxx_gpio_port_get_raw(const struct device *dev,
			       gpio_port_value_t *value)
{
	struct lmp90xxx_data *data = dev->data;
	uint8_t tmp;
	int err;

	k_mutex_lock(&data->gpio_lock, K_FOREVER);
	err = lmp90xxx_read_reg8(dev, LMP90XXX_REG_GPIO_DAT, &tmp);
	tmp &= ~(data->gpio_dircn);
	k_mutex_unlock(&data->gpio_lock);

	*value = tmp;

	return err;
}

int lmp90xxx_gpio_port_set_masked_raw(const struct device *dev,
				      gpio_port_pins_t mask,
				      gpio_port_value_t value)
{
	struct lmp90xxx_data *data = dev->data;
	int err = 0;
	uint8_t tmp;

	mask &= LMP90XXX_GPIO_DAT_MASK;

	k_mutex_lock(&data->gpio_lock, K_FOREVER);
	tmp = (data->gpio_dat & ~mask) | (value & mask);
	if (tmp != data->gpio_dat) {
		err = lmp90xxx_write_reg8(dev, LMP90XXX_REG_GPIO_DAT, tmp);
		if (!err) {
			data->gpio_dat = tmp;
		}
	}
	k_mutex_unlock(&data->gpio_lock);

	return err;
}

int lmp90xxx_gpio_port_set_bits_raw(const struct device *dev,
				    gpio_port_pins_t pins)
{
	struct lmp90xxx_data *data = dev->data;
	int err = 0;
	uint8_t tmp;

	tmp = pins & LMP90XXX_GPIO_DAT_MASK;

	k_mutex_lock(&data->gpio_lock, K_FOREVER);
	if (tmp != data->gpio_dat) {
		tmp |= data->gpio_dat;
		err = lmp90xxx_write_reg8(dev, LMP90XXX_REG_GPIO_DAT, tmp);
		if (!err) {
			data->gpio_dat = tmp;
		}
	}
	k_mutex_unlock(&data->gpio_lock);

	return err;
}

int lmp90xxx_gpio_port_clear_bits_raw(const struct device *dev,
				      gpio_port_pins_t pins)
{
	struct lmp90xxx_data *data = dev->data;
	int err = 0;
	uint8_t tmp;

	tmp = pins & LMP90XXX_GPIO_DAT_MASK;

	k_mutex_lock(&data->gpio_lock, K_FOREVER);
	if ((tmp & data->gpio_dat) != 0) {
		tmp = data->gpio_dat & ~tmp;
		err = lmp90xxx_write_reg8(dev, LMP90XXX_REG_GPIO_DAT, tmp);
		if (!err) {
			data->gpio_dat = tmp;
		}
	}
	k_mutex_unlock(&data->gpio_lock);

	return err;
}

int lmp90xxx_gpio_port_toggle_bits(const struct device *dev,
				   gpio_port_pins_t pins)
{
	struct lmp90xxx_data *data = dev->data;
	uint8_t tmp;
	int err;

	tmp = pins & LMP90XXX_GPIO_DAT_MASK;

	k_mutex_lock(&data->gpio_lock, K_FOREVER);
	tmp ^= data->gpio_dat;
	err = lmp90xxx_write_reg8(dev, LMP90XXX_REG_GPIO_DAT, tmp);
	if (!err) {
		data->gpio_dat = tmp;
	}
	k_mutex_unlock(&data->gpio_lock);

	return err;
}

#endif /* CONFIG_ADC_LMP90XXX_GPIO */

static int lmp90xxx_init(const struct device *dev)
{
	const struct lmp90xxx_config *config = dev->config;
	struct lmp90xxx_data *data = dev->data;
	k_tid_t tid;
	int err;

	data->dev = dev;

	k_mutex_init(&data->ura_lock);
	k_sem_init(&data->acq_sem, 0, 1);
	k_sem_init(&data->drdyb_sem, 0, 1);
#ifdef CONFIG_ADC_LMP90XXX_GPIO
	k_mutex_init(&data->gpio_lock);
#endif /* CONFIG_ADC_LMP90XXX_GPIO */

	/* Force INST1 + UAB on first access */
	data->ura = LMP90XXX_INVALID_URA;

	if (!spi_is_ready(&config->bus)) {
		LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	err = lmp90xxx_soft_reset(dev);
	if (err) {
		LOG_ERR("failed to request soft reset (err %d)", err);
		return err;
	}

	err = lmp90xxx_write_reg8(dev, LMP90XXX_REG_SPI_HANDSHAKECN,
				  LMP90XXX_SDO_DRDYB_DRIVER(0x4));
	if (err) {
		LOG_ERR("failed to set SPI handshake control (err %d)",
			err);
		return err;
	}

	if (config->rtd_current) {
		err = lmp90xxx_write_reg8(dev, LMP90XXX_REG_ADC_AUXCN,
				LMP90XXX_RTD_CUR_SEL(config->rtd_current));
		if (err) {
			LOG_ERR("failed to set RTD current (err %d)", err);
			return err;
		}
	}

	if (IS_ENABLED(CONFIG_ADC_LMP90XXX_CRC)) {
		err = lmp90xxx_write_reg8(dev, LMP90XXX_REG_SPI_CRC_CN,
					LMP90XXX_EN_CRC(1) |
					LMP90XXX_DRDYB_AFT_CRC(1));
		if (err) {
			LOG_ERR("failed to enable CRC (err %d)", err);
			return err;
		}
	}

	if (LMP90XXX_HAS_DRDYB(config)) {
		err = gpio_pin_configure_dt(&config->drdyb, GPIO_INPUT);
		if (err) {
			LOG_ERR("failed to configure DRDYB GPIO pin (err %d)",
				err);
			return -EINVAL;
		}

		gpio_init_callback(&data->drdyb_cb, lmp90xxx_drdyb_callback,
				   BIT(config->drdyb.pin));

		err = gpio_add_callback(config->drdyb.port, &data->drdyb_cb);
		if (err) {
			LOG_ERR("failed to add DRDYB callback (err %d)", err);
			return -EINVAL;
		}

		err = lmp90xxx_write_reg8(dev, LMP90XXX_REG_SPI_DRDYBCN,
					  LMP90XXX_SPI_DRDYB_D6(1));
		if (err) {
			LOG_ERR("failed to configure D6 as DRDYB (err %d)",
				err);
			return err;
		}

		err = gpio_pin_interrupt_configure_dt(&config->drdyb,
						      GPIO_INT_EDGE_TO_ACTIVE);
		if (err) {
			LOG_ERR("failed to configure DRDBY interrupt (err %d)",
				err);
			return -EINVAL;
		}
	}

	tid = k_thread_create(&data->thread, data->stack,
			      CONFIG_ADC_LMP90XXX_ACQUISITION_THREAD_STACK_SIZE,
			      (k_thread_entry_t)lmp90xxx_acquisition_thread,
			      data, NULL, NULL,
			      CONFIG_ADC_LMP90XXX_ACQUISITION_THREAD_PRIO,
			      0, K_NO_WAIT);
	k_thread_name_set(tid, "adc_lmp90xxx");

	/* Put device in stand-by to prepare it for single-shot conversion */
	err = lmp90xxx_write_reg8(dev, LMP90XXX_REG_PWRCN, LMP90XXX_PWRCN(0x3));
	if (err) {
		LOG_ERR("failed to request stand-by mode (err %d)", err);
		return err;
	}

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct adc_driver_api lmp90xxx_adc_api = {
	.channel_setup = lmp90xxx_adc_channel_setup,
	.read = lmp90xxx_adc_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = lmp90xxx_adc_read_async,
#endif
};

#define ASSERT_LMP90XXX_CURRENT_VALID(v) \
	BUILD_ASSERT(v == 0 || v == 100 || v == 200 || v == 300 ||	\
		     v == 400 || v == 500 || v == 600 || v == 700 ||	\
		     v == 800 || v == 900 || v == 1000,			\
		     "unsupported RTD current (" #v ")")

#define LMP90XXX_UAMPS_TO_RTD_CUR_SEL(x) (x / 100)

#define DT_INST_LMP90XXX(inst, t) DT_INST(inst, ti_lmp##t)

#define LMP90XXX_INIT(t, n, res, ch) \
	ASSERT_LMP90XXX_CURRENT_VALID(UTIL_AND(	\
		DT_NODE_HAS_PROP(DT_INST_LMP90XXX(n, t), rtd_current), \
		DT_PROP(DT_INST_LMP90XXX(n, t),	rtd_current))); \
	static struct lmp90xxx_data lmp##t##_data_##n = { \
		ADC_CONTEXT_INIT_TIMER(lmp##t##_data_##n, ctx), \
		ADC_CONTEXT_INIT_LOCK(lmp##t##_data_##n, ctx), \
		ADC_CONTEXT_INIT_SYNC(lmp##t##_data_##n, ctx), \
	}; \
	static const struct lmp90xxx_config lmp##t##_config_##n = { \
		.bus = SPI_DT_SPEC_GET(DT_INST_LMP90XXX(n, t), SPI_OP_MODE_MASTER | \
			SPI_TRANSFER_MSB | SPI_WORD_SET(8), 0), \
		.drdyb = GPIO_DT_SPEC_GET_OR(DT_INST_LMP90XXX(n, t), drdyb_gpios, {0}), \
		.rtd_current = LMP90XXX_UAMPS_TO_RTD_CUR_SEL( \
			DT_PROP_OR(DT_INST_LMP90XXX(n, t), rtd_current, 0)), \
		.resolution = res, \
		.channels = ch, \
	}; \
	DEVICE_DT_DEFINE(DT_INST_LMP90XXX(n, t), \
			 &lmp90xxx_init, NULL, \
			 &lmp##t##_data_##n, \
			 &lmp##t##_config_##n, POST_KERNEL, \
			 CONFIG_ADC_INIT_PRIORITY, \
			 &lmp90xxx_adc_api);

/*
 * LMP90077: 16 bit, 2 diff/4 se (4 channels), 0 currents
 */
#define LMP90077_INIT(n) LMP90XXX_INIT(90077, n, 16, 4)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lmp90077
DT_INST_FOREACH_STATUS_OKAY(LMP90077_INIT)

/*
 * LMP90078: 16 bit, 2 diff/4 se (4 channels), 2 currents
 */
#define LMP90078_INIT(n) LMP90XXX_INIT(90078, n, 16, 4)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lmp90078
DT_INST_FOREACH_STATUS_OKAY(LMP90078_INIT)

/*
 * LMP90079: 16 bit, 4 diff/7 se (7 channels), 0 currents, has VIN3-5
 */
#define LMP90079_INIT(n) LMP90XXX_INIT(90079, n, 16, 7)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lmp90079
DT_INST_FOREACH_STATUS_OKAY(LMP90079_INIT)

/*
 * LMP90080: 16 bit, 4 diff/7 se (7 channels), 2 currents, has VIN3-5
 */
#define LMP90080_INIT(n) LMP90XXX_INIT(90080, n, 16, 7)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lmp90080
DT_INST_FOREACH_STATUS_OKAY(LMP90080_INIT)

/*
 * LMP90097: 24 bit, 2 diff/4 se (4 channels), 0 currents
 */
#define LMP90097_INIT(n) LMP90XXX_INIT(90097, n, 24, 4)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lmp90097
DT_INST_FOREACH_STATUS_OKAY(LMP90097_INIT)

/*
 * LMP90098: 24 bit, 2 diff/4 se (4 channels), 2 currents
 */
#define LMP90098_INIT(n) LMP90XXX_INIT(90098, n, 24, 4)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lmp90098
DT_INST_FOREACH_STATUS_OKAY(LMP90098_INIT)

/*
 * LMP90099: 24 bit, 4 diff/7 se (7 channels), 0 currents, has VIN3-5
 */
#define LMP90099_INIT(n) LMP90XXX_INIT(90099, n, 24, 7)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lmp90099
DT_INST_FOREACH_STATUS_OKAY(LMP90099_INIT)

/*
 * LMP90100: 24 bit, 4 diff/7 se (7 channels), 2 currents, has VIN3-5
 */
#define LMP90100_INIT(n) LMP90XXX_INIT(90100, n, 24, 7)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_lmp90100
DT_INST_FOREACH_STATUS_OKAY(LMP90100_INIT)
