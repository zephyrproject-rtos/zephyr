/*
 * Copyright (c) 2026 Siemens AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/ads131m08.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(adc_ads131m08, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

/* Register and bit definitions from TI ADS131M08 datasheet */

#define ADS131M08_ADC_CHANNEL_0 0U
#define ADS131M08_ADC_CHANNEL_1 1U
#define ADS131M08_ADC_CHANNEL_2 2U
#define ADS131M08_ADC_CHANNEL_3 3U
#define ADS131M08_ADC_CHANNEL_4 4U
#define ADS131M08_ADC_CHANNEL_5 5U
#define ADS131M08_ADC_CHANNEL_6 6U
#define ADS131M08_ADC_CHANNEL_7 7U
#define ADS131M08_ADC_CHANNELS  8U
#define ADS131M08_REF_INTERNAL  1200U
#define ADS131M08_RESOLUTION    24U

/* Device settings registers */
#define ADS131M08_ID_REG     0x00
#define ADS131M08_STATUS_REG 0x01

/* Global settings registers */
#define ADS131M08_MODE_REG       0x02
#define ADS131M08_CLOCK_REG      0x03
#define ADS131M08_GAIN_REG_0TO3  0x04
#define ADS131M08_GAIN_REG_4TO7  0x05
#define ADS131M08_CFG_REG        0x06
#define ADS131M08_THRESH_MSB_REG 0x07
#define ADS131M08_THRESH_LSB_REG 0x08

/* Channel 0 settings registers */
#define ADS131M08_CH0_CFG_REG      0x09
#define ADS131M08_CH0_OCAL_MSB_REG 0x0A
#define ADS131M08_CH0_OCAL_LSB_REG 0x0B
#define ADS131M08_CH0_GCAL_MSB_REG 0x0C
#define ADS131M08_CH0_GCAL_LSB_REG 0x0D

/* Channel 1 settings registers */
#define ADS131M08_CH1_CFG_REG      0x0E
#define ADS131M08_CH1_OCAL_MSB_REG 0x0F
#define ADS131M08_CH1_OCAL_LSB_REG 0x10
#define ADS131M08_CH1_GCAL_MSB_REG 0x11
#define ADS131M08_CH1_GCAL_LSB_REG 0x12

/* Channel 2 settings registers */
#define ADS131M08_CH2_CFG_REG      0x13
#define ADS131M08_CH2_OCAL_MSB_REG 0x14
#define ADS131M08_CH2_OCAL_LSB_REG 0x15
#define ADS131M08_CH2_GCAL_MSB_REG 0x16
#define ADS131M08_CH2_GCAL_LSB_REG 0x17

/* Channel 3 settings registers */
#define ADS131M08_CH3_CFG_REG      0x18
#define ADS131M08_CH3_OCAL_MSB_REG 0x19
#define ADS131M08_CH3_OCAL_LSB_REG 0x1A
#define ADS131M08_CH3_GCAL_MSB_REG 0x1B
#define ADS131M08_CH3_GCAL_LSB_REG 0x1C

/* Channel 4 settings registers */
#define ADS131M08_CH4_CFG_REG      0x1D
#define ADS131M08_CH4_OCAL_MSB_REG 0x1E
#define ADS131M08_CH4_OCAL_LSB_REG 0x1F
#define ADS131M08_CH4_GCAL_MSB_REG 0x20
#define ADS131M08_CH4_GCAL_LSB_REG 0x21

/* Channel 5 settings registers */
#define ADS131M08_CH5_CFG_REG      0x22
#define ADS131M08_CH5_OCAL_MSB_REG 0x23
#define ADS131M08_CH5_OCAL_LSB_REG 0x24
#define ADS131M08_CH5_GCAL_MSB_REG 0x25
#define ADS131M08_CH5_GCAL_LSB_REG 0x26

/* Channel 6 settings registers */
#define ADS131M08_CH6_CFG_REG      0x27
#define ADS131M08_CH6_OCAL_MSB_REG 0x28
#define ADS131M08_CH6_OCAL_LSB_REG 0x29
#define ADS131M08_CH6_GCAL_MSB_REG 0x2A
#define ADS131M08_CH6_GCAL_LSB_REG 0x2B

/* Channel 7 settings registers */
#define ADS131M08_CH7_CFG_REG      0x2C
#define ADS131M08_CH7_OCAL_MSB_REG 0x2D
#define ADS131M08_CH7_OCAL_LSB_REG 0x2E
#define ADS131M08_CH7_GCAL_MSB_REG 0x2F
#define ADS131M08_CH7_GCAL_LSB_REG 0x30

/* Register Map CRC Registers */
#define ADS131M08_REGMAP_CRC_REG 0x3E

/* ADS131M08 commands */
#define ADS131M08_NULL_CMD    0x0000
#define ADS131M08_RESET_CMD   0x0011
#define ADS131M08_STANDBY_CMD 0x0022
#define ADS131M08_WAKEUP_CMD  0x0033
#define ADS131M08_LOCK_CMD    0x0555
#define ADS131M08_UNLOCK_CMD  0x0655
#define ADS131M08_RREG_CMD    0xA000
#define ADS131M08_WREG_CMD    0x6000

#define ADS131M08_RESET_RSP      0xFF28
#define ADS131M08_RESET_RSP_INCL 0x0011

/* MODE register */
#define ADS131M08_MODE_DEFAULT         0x0510
#define ADS131M08_WLENGTH_MASK         GENMASK(9, 8)
#define ADS131M08_WLENGTH_16BIT        0
#define ADS131M08_WLENGTH_24BIT        BIT(8)
#define ADS131M08_WLENGTH_32BIT_PADDED BIT(9)
#define ADS131M08_WLENGTH_32BIT_SIGNED (BIT(9) | BIT(8))

/* GAIN register */
#define ADS131M08_GAIN_MASK GENMASK(2, 0)

#define ADS131M08_GAIN_1   0
#define ADS131M08_GAIN_2   1
#define ADS131M08_GAIN_4   2
#define ADS131M08_GAIN_8   3
#define ADS131M08_GAIN_16  4
#define ADS131M08_GAIN_32  5
#define ADS131M08_GAIN_64  6
#define ADS131M08_GAIN_128 7

#define ADS131M08_XTAL_DISABLE BIT(7)

#define ADS131M08_CHANNEL0_ENABLE BIT(8)
#define ADS131M08_CHANNEL1_ENABLE BIT(9)
#define ADS131M08_CHANNEL2_ENABLE BIT(10)
#define ADS131M08_CHANNEL3_ENABLE BIT(11)
#define ADS131M08_CHANNEL4_ENABLE BIT(12)
#define ADS131M08_CHANNEL5_ENABLE BIT(13)
#define ADS131M08_CHANNEL6_ENABLE BIT(14)
#define ADS131M08_CHANNEL7_ENABLE BIT(15)

/* OSR register */
#define ADS131M08_OSR_MASK  GENMASK(4, 2)
#define ADS131M08_OSR_128   0x00
#define ADS131M08_OSR_256   BIT(2)
#define ADS131M08_OSR_512   BIT(3)
#define ADS131M08_OSR_1024  BIT(3) | BIT(2)
#define ADS131M08_OSR_2048  BIT(4)
#define ADS131M08_OSR_4096  BIT(4) | BIT(2)
#define ADS131M08_OSR_8192  BIT(4) | BIT(3)
#define ADS131M08_OSR_16384 GENMASK(4, 2)

#define VALUE_128_EXPONENT   7
#define VALUE_256_EXPONENT   8
#define VALUE_512_EXPONENT   9
#define VALUE_1024_EXPONENT  10
#define VALUE_2048_EXPONENT  11
#define VALUE_4096_EXPONENT  12
#define VALUE_8192_EXPONENT  13
#define VALUE_16384_EXPONENT 14

#define ADS131M08_GC_MODE_MASK  BIT(8)
#define ADS131M08_GC_DELAY_MASK GENMASK(12, 9)
#define ADS131M08_PWR_HR        BIT(1) | BIT(0)
#define ADS131M08_PWR_LP        BIT(0)

#define ADS131M08_RESET_DELAY 2 /* Min 2048 clock cycles, ~1ms at 2 MHz */

/* CFG register */
#define ADS131M08_GLOBAL_CHOP_EN BIT(8)
#define ADS131M08_GLOBAL_CHOP_DIS 0U

/* CLOCK register */
#define ADS131M08_DISABLE_ADC 0x0002

#define ADS131M08_DEVICE_ID (0x20U | ADS131M08_ADC_CHANNELS)

/* Status word + channel data + CRC */
#define ADS131M08_WORDS_PER_FRAME (1U + ADS131M08_ADC_CHANNELS + 1U)

#define ADS131M08_FIFO_DEPTH             2U
#define ADS131M08_WORDLENGTH_OP          4U /* 32-bit mode */
#define ADS131M08_WORDLENGTH_AFTER_RESET 3U /* 24-bit after reset */
#define ADS131M08_FRAMELENGTH            (ADS131M08_WORDLENGTH_OP * ADS131M08_WORDS_PER_FRAME)

/** ADS131M08 device functional modes */
enum ads131m08_functional_mode {
	ADS131M08_CONTINUOUS_CONVERSION_FM = 0,
	ADS131M08_GLOBAL_CHOP_FM = 1,
	ADS131M08_STANDBY_FM = 2,
	ADS131M08_CURRENT_DETECT_FM = 3,
	ADS131M08_RESET_FM = 4,
};

struct adc_ads131m08_config {
	enum ads131m08_functional_mode active_mode;
	const struct spi_dt_spec spi;
#ifdef CONFIG_ADS131M08_TRIGGER
	struct gpio_dt_spec gpio_drdy;
#endif
	struct gpio_dt_spec gpio_reset;
	const struct adc_dt_spec spec;
};

struct adc_ads131m08_data {
	struct adc_context ctx;
	const struct device *dev;
	uint8_t diff;
	uint8_t channels;
	int32_t *buffer;
	int32_t *repeat_buffer;
	enum ads131m08_functional_mode operation_mode;
#if CONFIG_ADS131M08_TRIGGER
	struct gpio_callback gpio_drdy_cb;
	struct k_sem sem_drdy;
#endif /* CONFIG_ADS131M08_TRIGGER */
#ifdef CONFIG_ADS131M08_STREAM
	struct rtio_iodev_sqe *sqe;
	struct rtio *rtio_ctx;
	struct rtio_iodev *iodev;
	uint64_t timestamp;
	bool fifo_needs_drain;                /* True until first double-read completes */
	enum adc_trigger_type active_trigger; /* Current trigger mode from read config */
	uint8_t drdy_pending;                 /* Count of DRDY events for FIFO_FULL mode */
#endif                                        /* CONFIG_ADS131M08_STREAM */
};

static int ads131m08_configure_wlen(const struct device *dev);

#ifdef CONFIG_ADS131M08_STREAM
#include <zephyr/drivers/counter.h>

struct adc_ads131m08_fifo_data {
	uint8_t is_fifo: 1;
	uint8_t diff_mode: 1;
	uint8_t empty: 1;
	uint8_t wlen: 3;
	uint8_t frame_count: 2; /* Number of frames in this buffer (1 or 2) */
	uint16_t vref_mv;
	uint64_t timestamp;
	/* Frame 0 */
	int32_t status;
	int32_t sample_be[8];
	int32_t crc;
	/* Frame 1 (only valid when frame_count == 2) */
	int32_t status_1;
	int32_t sample_1_be[8];
	int32_t crc_1;
} __packed;

static void ads131m08_stream_irq_handler(const struct device *dev, bool double_read);

#endif /* CONFIG_ADS131M08_STREAM */

static int ads131m08_transceive(const struct device *dev, uint8_t *send_buf,
				const size_t send_buf_len, uint8_t *recv_buf,
				const size_t recv_buf_len)
{
	int ret;
	const struct adc_ads131m08_config *cfg = dev->config;
	const struct spi_buf tx_buf = {
		.buf = send_buf,
		.len = send_buf_len,
	};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	struct spi_buf rx_buf = {
		.buf = recv_buf,
		.len = recv_buf_len,
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1,
	};

	ret = spi_transceive_dt(&cfg->spi, &tx, &rx);
	return ret;
}

static int ads131m08_send_command(const struct device *dev, uint16_t cmd)
{
	int ret;
	uint8_t rx_buf[ADS131M08_FRAMELENGTH] = {0};
	uint8_t tx_buf[ADS131M08_FRAMELENGTH] = {0};

	sys_put_be16(cmd, tx_buf);
	ret = ads131m08_transceive(dev, tx_buf, sizeof(tx_buf), rx_buf, sizeof(rx_buf));
	if (ret != 0) {
		return ret;
	}
	return 0;
}

static int ads131m08_standby(const struct device *dev)
{
	return ads131m08_send_command(dev, ADS131M08_STANDBY_CMD);
}

static int ads131m08_wakeup(const struct device *dev)
{
	return ads131m08_send_command(dev, ADS131M08_WAKEUP_CMD);
}

static uint16_t ads131m08_get_wreg(uint8_t addr, uint8_t n_to_write)
{
	const uint8_t n = n_to_write - 1U;

	/* WREG command word: 011a aaaa annn nnnn (address << 7 | n) */
	return (uint16_t)(ADS131M08_WREG_CMD | (addr << 7) | (uint16_t)n);
}

static int ads131m08_reg_write_short(const struct device *dev, uint8_t addr, uint16_t val)
{
	uint16_t temp;
	uint8_t rx_buf[ADS131M08_FRAMELENGTH] = {0};
	uint8_t tx_buf[ADS131M08_FRAMELENGTH] = {0};

	__ASSERT(sizeof(tx_buf) == 40, "tx_buf size calculation error");
	__ASSERT(sizeof(rx_buf) == 40, "rx_buf size calculation error");
	temp = ads131m08_get_wreg(addr, 1);
	sys_put_be16(temp, tx_buf);
	sys_put_be16(val, &tx_buf[ADS131M08_WORDLENGTH_OP]);

	return ads131m08_transceive(dev, tx_buf, sizeof(tx_buf), rx_buf, sizeof(rx_buf));
}

static int ads131m08_reg_read_short(const struct device *dev, uint8_t addr, uint16_t *buf)
{
	int ret;
	uint16_t temp;
	uint8_t rx_buf[ADS131M08_FRAMELENGTH] = {0};
	uint8_t tx_buf[ADS131M08_FRAMELENGTH] = {0};

	__ASSERT(sizeof(tx_buf) == 40, "tx_buf size calculation error");
	__ASSERT(sizeof(rx_buf) == 40, "rx_buf size calculation error");
	temp = (uint16_t)(ADS131M08_RREG_CMD | (addr << 7));
	sys_put_be16(temp, tx_buf);
	ret = ads131m08_transceive(dev, tx_buf, sizeof(tx_buf), rx_buf, sizeof(rx_buf));
	if (ret != 0) {
		return ret;
	}
	memset(tx_buf, 0, sizeof(tx_buf));
	ret = ads131m08_transceive(dev, tx_buf, sizeof(tx_buf), rx_buf, sizeof(rx_buf));
	*buf = sys_get_be16(rx_buf);
	return ret;
}

static int ads131m08_reg_update_bits(const struct device *dev, uint8_t addr, uint16_t mask,
				     uint16_t val)
{
	int ret;
	uint16_t reg = 0;

	ret = ads131m08_reg_read_short(dev, addr, &reg);
	if (ret < 0) {
		return ret;
	}

	reg &= ~mask;
	reg |= (val & mask);

	return ads131m08_reg_write_short(dev, addr, reg);
}

static int ads131m08_init_reset(const struct device *dev)
{
	const struct adc_ads131m08_config *cfg = dev->config;
	int ret;
	struct adc_ads131m08_data *drv_data = dev->data;

	if (!gpio_is_ready_dt(&cfg->gpio_reset)) {
		LOG_ERR("GPIO port %s not ready", cfg->gpio_reset.port->name);
		return -EINVAL;
	}

	ret = gpio_pin_configure_dt(&cfg->gpio_reset, GPIO_OUTPUT_INACTIVE);
	if (ret != 0) {
		return ret;
	}

	drv_data->dev = dev;
	return ret;
}

static int ads131m08_sync_gpio(const struct device *dev)
{
	const struct adc_ads131m08_config *cfg = dev->config;
	int ret;

	/* Sync pulse must be shorter than tw(RSL) */
	ret = gpio_pin_set_dt(&cfg->gpio_reset, 1);
	if (ret != 0) {
		return ret;
	}

	k_busy_wait(1);
	ret = gpio_pin_set_dt(&cfg->gpio_reset, 0);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

static int ads131m08_reset_gpio(const struct device *dev)
{
	const struct adc_ads131m08_config *cfg = dev->config;
	int ret;

	/* Reset pulse must be longer than tw(RSL) */
	ret = gpio_pin_set_dt(&cfg->gpio_reset, 1);
	if (ret != 0) {
		return ret;
	}

	k_msleep(ADS131M08_RESET_DELAY);
	ret = gpio_pin_set_dt(&cfg->gpio_reset, 0);
	if (ret != 0) {
		return ret;
	}
	ret = ads131m08_configure_wlen(dev);
	if (ret != 0) {
		return ret;
	}
	return 0;
}

#if CONFIG_ADS131M08_TRIGGER
static void ads131m08_drdy_callback(const struct device *dev, struct gpio_callback *cb,
				    uint32_t pins)
{
	struct adc_ads131m08_data *drv_data =
		CONTAINER_OF(cb, struct adc_ads131m08_data, gpio_drdy_cb);

#ifdef CONFIG_ADS131M08_STREAM
	bool need_double_read;
	const struct adc_read_config *cfg_adc;

	if (drv_data->sqe == NULL) {
		k_sem_give(&drv_data->sem_drdy);
		return;
	}

	cfg_adc = drv_data->sqe->sqe.iodev->data;
	if (!cfg_adc->is_streaming) {
		k_sem_give(&drv_data->sem_drdy);
		return;
	}
	switch (drv_data->active_trigger) {
	case ADC_TRIG_DATA_READY:
		__fallthrough;
	case ADC_TRIG_FIFO_WATERMARK:
		need_double_read = drv_data->fifo_needs_drain;
		ads131m08_stream_irq_handler(drv_data->dev, need_double_read);
		break;
	case ADC_TRIG_FIFO_FULL:
		/* FIFO_FULL mode: wait for 2 samples, then read both */
		drv_data->drdy_pending++;
		if (drv_data->drdy_pending < ADS131M08_FIFO_DEPTH) {
			/* Wait for FIFO to fill */
			return;
		}
		drv_data->drdy_pending = 0;
		need_double_read = true;
		ads131m08_stream_irq_handler(drv_data->dev, need_double_read);
		break;
	default:
		k_sem_give(&drv_data->sem_drdy);
		return;
	}

#else
	k_sem_give(&drv_data->sem_drdy);
#endif /* CONFIG_ADS131M08_STREAM */
}

#endif

static void adc_context_start_sampling(struct adc_context *ctx)
{
	const uint8_t *src_bytes;
	int32_t *dst;
	struct adc_ads131m08_data *data = CONTAINER_OF(ctx, struct adc_ads131m08_data, ctx);
	uint8_t i = 0;
	uint8_t rx_buf[ADS131M08_FRAMELENGTH] = {0};
	uint8_t tx_buf[ADS131M08_FRAMELENGTH] = {0};

	data->channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;
#if CONFIG_ADS131M08_TRIGGER
	/* Wait once per sequence to clear the FIFO */
	if (ctx->sampling_index == 0) {
		k_sem_take(&data->sem_drdy, K_FOREVER);
	}
#endif
	__ASSERT(ADS131M08_WORDLENGTH_OP == 4U, "Word length other than 32bit not supported yet");
	ads131m08_transceive(data->dev, tx_buf, sizeof(tx_buf), rx_buf, sizeof(rx_buf));

	const uint8_t n_ch = POPCOUNT(ctx->sequence.channels);

	dst = ((int32_t *)ctx->sequence.buffer) + ctx->sampling_index * n_ch;
	src_bytes = &rx_buf[ADS131M08_WORDLENGTH_OP];

	for (uint8_t ch = 0; ch < ADS131M08_ADC_CHANNELS; ch++) {
		if ((data->channels & (1 << ch)) != 0) {
			memcpy(&dst[i], src_bytes + ch * ADS131M08_WORDLENGTH_OP,
			       ADS131M08_WORDLENGTH_OP);
			i++;
		}
	}
	__ASSERT(i == n_ch, "Bitmask evaluation error");
	adc_context_on_sampling_done(&data->ctx, data->dev);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_ads131m08_data *data = CONTAINER_OF(ctx, struct adc_ads131m08_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int ads131m08_validate_buffer_size(const struct device *dev,
					  const struct adc_sequence *sequence)
{
	size_t needed;
	uint8_t channels;

	channels = POPCOUNT(sequence->channels);
	needed = channels * sizeof(int32_t);

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

static int ads131m08_enable_global_chop(const struct device *dev)
{
	return ads131m08_reg_update_bits(dev, ADS131M08_CFG_REG, ADS131M08_GC_MODE_MASK,
					 ADS131M08_GLOBAL_CHOP_EN);
}

static int ads131m08_disable_global_chop(const struct device *dev)
{
	return ads131m08_reg_update_bits(dev, ADS131M08_CFG_REG, ADS131M08_GC_MODE_MASK,
					 ADS131M08_GLOBAL_CHOP_DIS);
}

/* Transition to CURRENT_DETECT_FM via standby then sync pulse.
 * On partial failure (standby ok, sync fails), records intermediate STANDBY_FM state.
 */
static int ads131m08_standby_then_sync(const struct device *dev)
{
	struct adc_ads131m08_data *data = dev->data;
	int ret;

	ret = ads131m08_standby(dev);
	if (ret != 0) {
		return ret;
	}
	ret = ads131m08_sync_gpio(dev);
	if (ret != 0) {
		data->operation_mode = ADS131M08_STANDBY_FM;
	}
	return ret;
}

/* Transition from CURRENT_DETECT_FM via reset then standby.
 * On partial failure (reset ok, standby fails), records intermediate RESET_FM state.
 */
static int ads131m08_reset_then_standby(const struct device *dev)
{
	struct adc_ads131m08_data *data = dev->data;
	int ret;

	ret = ads131m08_reset_gpio(dev);
	if (ret != 0) {
		return ret;
	}
	ret = ads131m08_standby(dev);
	if (ret != 0) {
		data->operation_mode = ADS131M08_RESET_FM;
	}
	return ret;
}

/* Transition from CURRENT_DETECT_FM via reset then enable global chop.
 * On partial failure (reset ok, reg update fails), records intermediate RESET_FM state.
 */
static int ads131m08_reset_then_enable_global_chop(const struct device *dev)
{
	struct adc_ads131m08_data *data = dev->data;
	int ret;

	ret = ads131m08_reset_gpio(dev);
	if (ret != 0) {
		return ret;
	}
	ret = ads131m08_enable_global_chop(dev);
	if (ret != 0) {
		data->operation_mode = ADS131M08_RESET_FM;
	}
	return ret;
}

static int ads131m08_transition_from_continuous(const struct device *dev,
						enum ads131m08_functional_mode operation_mode)
{
	switch (operation_mode) {
	case ADS131M08_CONTINUOUS_CONVERSION_FM:
		return 0;
	case ADS131M08_GLOBAL_CHOP_FM:
		return ads131m08_enable_global_chop(dev);
	case ADS131M08_STANDBY_FM:
		return ads131m08_standby(dev);
	case ADS131M08_CURRENT_DETECT_FM:
		return ads131m08_standby_then_sync(dev);
	case ADS131M08_RESET_FM:
		return ads131m08_reset_gpio(dev);
	default:
		LOG_ERR("Invalid operating mode!");
		return -EINVAL;
	}
}

static int ads131m08_transition_from_global_chop(const struct device *dev,
						 enum ads131m08_functional_mode operation_mode)
{
	switch (operation_mode) {
	case ADS131M08_CONTINUOUS_CONVERSION_FM:
		return ads131m08_disable_global_chop(dev);
	case ADS131M08_GLOBAL_CHOP_FM:
		return 0;
	case ADS131M08_STANDBY_FM:
		return ads131m08_standby(dev);
	case ADS131M08_CURRENT_DETECT_FM:
		return ads131m08_standby_then_sync(dev);
	case ADS131M08_RESET_FM:
		return ads131m08_reset_gpio(dev);
	default:
		LOG_ERR("Invalid operating mode!");
		return -EINVAL;
	}
}

static int ads131m08_transition_from_standby(const struct device *dev,
					     enum ads131m08_functional_mode operation_mode)
{
	int ret;

	switch (operation_mode) {
	case ADS131M08_CONTINUOUS_CONVERSION_FM:
		ret = ads131m08_wakeup(dev);
		if (ret != 0) {
			return ret;
		}
		return ads131m08_disable_global_chop(dev);
	case ADS131M08_GLOBAL_CHOP_FM:
		ret = ads131m08_wakeup(dev);
		if (ret != 0) {
			return ret;
		}
		return ads131m08_enable_global_chop(dev);
	case ADS131M08_STANDBY_FM:
		return 0;
	case ADS131M08_CURRENT_DETECT_FM:
		return ads131m08_sync_gpio(dev);
	case ADS131M08_RESET_FM:
		return ads131m08_reset_gpio(dev);
	default:
		LOG_ERR("Invalid operating mode!");
		return -EINVAL;
	}
}

static int ads131m08_transition_from_current_detect(const struct device *dev,
						    enum ads131m08_functional_mode operation_mode)
{
	switch (operation_mode) {
	case ADS131M08_CONTINUOUS_CONVERSION_FM:
		return ads131m08_reset_gpio(dev);
	case ADS131M08_GLOBAL_CHOP_FM:
		return ads131m08_reset_then_enable_global_chop(dev);
	case ADS131M08_STANDBY_FM:
		return ads131m08_reset_then_standby(dev);
	case ADS131M08_CURRENT_DETECT_FM:
		return 0;
	case ADS131M08_RESET_FM:
		return ads131m08_reset_gpio(dev);
	default:
		LOG_ERR("Invalid operating mode!");
		return -EINVAL;
	}
}

static int ads131m08_transition_from_reset(const struct device *dev,
					   enum ads131m08_functional_mode operation_mode)
{
	switch (operation_mode) {
	case ADS131M08_CONTINUOUS_CONVERSION_FM:
		return 0;
	case ADS131M08_GLOBAL_CHOP_FM:
		return ads131m08_enable_global_chop(dev);
	case ADS131M08_STANDBY_FM:
		return ads131m08_standby(dev);
	case ADS131M08_CURRENT_DETECT_FM:
		return ads131m08_standby_then_sync(dev);
	case ADS131M08_RESET_FM:
		return 0;
	default:
		LOG_ERR("Invalid operating mode!");
		return -EINVAL;
	}
}

static int ads131m08_set_operation_mode(const struct device *dev,
					enum ads131m08_functional_mode operation_mode)
{
	struct adc_ads131m08_data *data = dev->data;
	int ret;

	switch (data->operation_mode) {
	case ADS131M08_CONTINUOUS_CONVERSION_FM:
		ret = ads131m08_transition_from_continuous(dev, operation_mode);
		break;
	case ADS131M08_GLOBAL_CHOP_FM:
		ret = ads131m08_transition_from_global_chop(dev, operation_mode);
		break;
	case ADS131M08_STANDBY_FM:
		ret = ads131m08_transition_from_standby(dev, operation_mode);
		break;
	case ADS131M08_CURRENT_DETECT_FM:
		ret = ads131m08_transition_from_current_detect(dev, operation_mode);
		break;
	case ADS131M08_RESET_FM:
		ret = ads131m08_transition_from_reset(dev, operation_mode);
		break;
	default:
		LOG_ERR("Invalid operating mode!");
		return -EINVAL;
	}

	if (ret != 0) {
		return ret;
	}
	data->operation_mode = operation_mode;
	return 0;
}

static int ads131m08_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct adc_ads131m08_config *cfg = dev->config;
	int ret;
	struct adc_ads131m08_data *data = dev->data;

	if (sequence->resolution != cfg->spec.resolution) {
		return -EINVAL;
	}
	if (sequence->oversampling != cfg->spec.oversampling) {
		return -EINVAL;
	}

	ret = ads131m08_validate_buffer_size(dev, sequence);
	if (ret < 0) {
		return ret;
	}

	data->buffer = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);
	return adc_context_wait_for_completion(&data->ctx);
}

static int ads131m08_read_async(const struct device *dev, const struct adc_sequence *sequence,
				struct k_poll_signal *async)
{
	int ret;
	struct adc_ads131m08_data *data = dev->data;

	adc_context_lock(&data->ctx, async ? true : false, async);
	ret = ads131m08_start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}

static int ads131m08_read(const struct device *dev, const struct adc_sequence *sequence)
{
	return ads131m08_read_async(dev, sequence, NULL);
}

static int ads131m08_channel_setup(const struct device *dev,
				   const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_ads131m08_config *cfg = dev->config;
	int ret;
	uint16_t gain_reg;
	uint16_t mask;
	uint16_t val;

	if (channel_cfg->channel_id > 7) {
		LOG_ERR("invalid channel id %d", channel_cfg->channel_id);
		return -EINVAL;
	}
	if (channel_cfg->differential != cfg->spec.channel_cfg.differential) {
		LOG_ERR("invalid mode %d", channel_cfg->differential);
		return -EINVAL;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_DBG("Unsupported Reference Voltage");
		return -ENOTSUP;
	}

	if (!channel_cfg->differential) {
		LOG_ERR("Differential channels only");
		return -EINVAL;
	}

	if (channel_cfg->channel_id < 4) {
		gain_reg = ADS131M08_GAIN_REG_0TO3;
	} else {
		gain_reg = ADS131M08_GAIN_REG_4TO7;
	}
	switch (channel_cfg->gain) {
	case ADC_GAIN_1:
		val = ADS131M08_GAIN_1;
		break;
	case ADC_GAIN_2:
		val = ADS131M08_GAIN_2;
		break;
	case ADC_GAIN_4:
		val = ADS131M08_GAIN_4;
		break;
	case ADC_GAIN_8:
		val = ADS131M08_GAIN_8;
		break;
	case ADC_GAIN_16:
		val = ADS131M08_GAIN_16;
		break;
	case ADC_GAIN_32:
		val = ADS131M08_GAIN_32;
		break;
	case ADC_GAIN_64:
		val = ADS131M08_GAIN_64;
		break;
	case ADC_GAIN_128:
		val = ADS131M08_GAIN_128;
		break;
	default:
		LOG_ERR("Unsupported gain value");
		return -EINVAL;
	}
	/* Shift value and mask to channel position */
	mask = ADS131M08_GAIN_MASK << (channel_cfg->channel_id % 4 * 4);
	val = val << (channel_cfg->channel_id % 4 * 4);

	ret = ads131m08_reg_update_bits(dev, gain_reg, mask, val);
	if (ret != 0) {
		return ret;
	}

	switch (channel_cfg->channel_id) {
	case ADS131M08_ADC_CHANNEL_0:
		mask = ADS131M08_CHANNEL0_ENABLE;
		val = ADS131M08_CHANNEL0_ENABLE;
		break;
	case ADS131M08_ADC_CHANNEL_1:
		mask = ADS131M08_CHANNEL1_ENABLE;
		val = ADS131M08_CHANNEL1_ENABLE;
		break;
	case ADS131M08_ADC_CHANNEL_2:
		mask = ADS131M08_CHANNEL2_ENABLE;
		val = ADS131M08_CHANNEL2_ENABLE;
		break;
	case ADS131M08_ADC_CHANNEL_3:
		mask = ADS131M08_CHANNEL3_ENABLE;
		val = ADS131M08_CHANNEL3_ENABLE;
		break;
	case ADS131M08_ADC_CHANNEL_4:
		mask = ADS131M08_CHANNEL4_ENABLE;
		val = ADS131M08_CHANNEL4_ENABLE;
		break;
	case ADS131M08_ADC_CHANNEL_5:
		mask = ADS131M08_CHANNEL5_ENABLE;
		val = ADS131M08_CHANNEL5_ENABLE;
		break;
	case ADS131M08_ADC_CHANNEL_6:
		mask = ADS131M08_CHANNEL6_ENABLE;
		val = ADS131M08_CHANNEL6_ENABLE;
		break;
	case ADS131M08_ADC_CHANNEL_7:
		mask = ADS131M08_CHANNEL7_ENABLE;
		val = ADS131M08_CHANNEL7_ENABLE;
		break;
	default:
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	ret = ads131m08_reg_update_bits(dev, ADS131M08_CLOCK_REG, mask, val);
	if (ret != 0) {
		return ret;
	}
	return 0;
}

#ifdef CONFIG_ADS131M08_STREAM
static void ads131m08_submit_stream(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct adc_ads131m08_data *data = (struct adc_ads131m08_data *)dev->data;
	const struct adc_read_config *read_config =
		(const struct adc_read_config *)iodev_sqe->sqe.iodev->data;

	/* Determine trigger mode from read config */
	data->active_trigger = ADC_TRIG_DATA_READY; /* Default */
	if (read_config != NULL && read_config->trigger_cnt > 0) {
		for (int i = 0; i < read_config->trigger_cnt; i++) {
			if (read_config->triggers[i].trigger == ADC_TRIG_FIFO_FULL) {
				data->active_trigger = ADC_TRIG_FIFO_FULL;
				break;
			}
		}
	}
	data->drdy_pending = 0;
	data->sqe = iodev_sqe;
}

static void ads131m08_convert_q31(q31_t *out, const uint8_t *buff, uint8_t diff_mode,
				  uint16_t vref_mv, uint8_t adc_shift, uint8_t wlen)
{
	const uint32_t resolution = ADS131M08_RESOLUTION;
	int32_t data_in;

	/* Read 24-bit sample and sign-extend when needed */
	switch (wlen) {
	case (2):
		data_in = (int32_t)(sys_get_be16(buff) << 8); /* LSBs truncated */
		break;
	case (3):
		data_in = (int32_t)(sys_get_be24(buff));
		break;
	case (4):
		data_in = (int32_t)sys_get_be32(buff);
		break;
	default:
		LOG_ERR("Unsupported word length: %d", wlen);
		return;
	}

	if (diff_mode && (data_in & BIT(resolution - 1))) {
		data_in |= (int32_t)~BIT_MASK(resolution);
	}

	/* Scale is the number of codes (excluding sign bit in differential) */
	const uint64_t scale = 1ULL << (resolution - (diff_mode ? 1U : 0U));
	/* Compute Q31 value safely:
	 * out = (2^(31-adc_shift)) * (Vref / scale) * data_in
	 * where Vref is in volts; we use mV and divide by 1000 accordingly.
	 * The operation is ordered to avoid overflow and precision loss.
	 */
	int64_t qscale = (int64_t)(1ULL << (31 - adc_shift));
	const int64_t numerator = (int64_t)data_in * (int64_t)vref_mv * qscale; /* mV scaled */
	const int64_t denominator = (int64_t)scale * 1000; /* convert mV to V */

	if (denominator == 0) {
		*out = 0;
		return;
	}
	const int64_t result = numerator / denominator;

	*out = (q31_t)result;
}

static int ads131m08_decoder_get_frame_count(const uint8_t *buffer, uint32_t channel,
					     uint16_t *frame_count)
{
	const struct adc_ads131m08_fifo_data *enc_data =
		(const struct adc_ads131m08_fifo_data *)buffer;

	if (enc_data->empty) {
		return -ENODATA;
	}

	/* Return actual frame count (1 or 2 for FIFO mode) */
	*frame_count = enc_data->frame_count > 0 ? enc_data->frame_count : 1;

	return 0;
}

static int ads131m08_decoder_decode(const uint8_t *buffer, uint32_t channel, uint32_t *fit,
				    uint16_t max_count, void *data_out)
{
	const struct adc_ads131m08_fifo_data *enc_data =
		(const struct adc_ads131m08_fifo_data *)buffer;
	struct adc_data *data;
	uint16_t frames_to_decode;
	uint32_t frame_idx;

	if (channel >= ADS131M08_ADC_CHANNELS) {
		return -ENOTSUP;
	}
	__ASSERT(enc_data->frame_count <= 2, "Invalid frame count in FIFO data");
	const uint16_t total_frames = enc_data->frame_count > 0 ? enc_data->frame_count : 1;

	if (*fit >= total_frames) {
		return -ENOTSUP;
	}

	data = (struct adc_data *)data_out;
	memset(data, 0, sizeof(struct adc_data));

	if (enc_data->empty) {
		data->header.base_timestamp_ns = 0;
		data->header.reading_count = 0;
		return -ENODATA;
	}

	data->header.base_timestamp_ns = enc_data->timestamp;

	/* 32 is used because input parameter for __builtin_clz func is
	 * unsigned int (32 bits) and func will consider any input value
	 * as 32 bit.
	 */
	__ASSERT(enc_data->vref_mv != 0, "Vref cannot be zero");
	data->shift = 32 - __builtin_clz(enc_data->vref_mv);

	/* Decode remaining frames up to max_count */
	frames_to_decode = MIN(total_frames - *fit, max_count);
	__ASSERT(frames_to_decode <= 2, "Can only decode up to 2 frames from FIFO data");
	data->header.reading_count = frames_to_decode;

	for (uint16_t i = 0; i < frames_to_decode; i++) {
		frame_idx = *fit + i;
		const uint8_t *sample_ptr;

		__ASSERT(frame_idx < 2, "Invalid frame index for FIFO data");
		if (frame_idx == 0) {
			sample_ptr = (const uint8_t *)&enc_data->sample_be[channel];
		} else {
			sample_ptr = (const uint8_t *)&enc_data->sample_1_be[channel];
		}

		data->readings[i].timestamp_delta = 0;
		ads131m08_convert_q31(&data->readings[i].value, sample_ptr, enc_data->diff_mode,
				      enc_data->vref_mv, data->shift, enc_data->wlen);
	}

	*fit += frames_to_decode;

	return 0;
}

/**
 * @brief Callback after first frame SPI read completes
 */
static void ads131m08_process_sample_cb(struct rtio *r, const struct rtio_sqe *sqe, int res,
					void *arg)
{
	ARG_UNUSED(r);
	const struct device *dev = (const struct device *)arg;
	struct adc_ads131m08_data *data = (struct adc_ads131m08_data *)dev->data;
	struct rtio_iodev_sqe *iodev_sqe = (struct rtio_iodev_sqe *)sqe->userdata;

	data->fifo_needs_drain = false;
	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

static void ads131m08_stream_irq_handler(const struct device *dev, bool double_read)
{
	const struct adc_ads131m08_config *cfg = (const struct adc_ads131m08_config *)dev->config;
	enum adc_stream_data_opt data_opt;
	struct adc_ads131m08_data *data = (struct adc_ads131m08_data *)dev->data;
	struct adc_ads131m08_fifo_data *hdr;
	struct adc_read_config *read_config = NULL;
	struct rtio_iodev_sqe *current_sqe = data->sqe;
	struct rtio_sqe *complete_op;
	struct rtio_sqe *read_sample0;
	struct rtio_sqe *read_sample1;
	uint32_t buf_len;
	uint8_t *buf;

	if (current_sqe == NULL) {
		return;
	}

	if (current_sqe->sqe.iodev != NULL) {
		read_config = (struct adc_read_config *)data->sqe->sqe.iodev->data;
	}
	if (read_config == NULL) {
		return;
	}

	data->timestamp = k_ticks_to_ns_floor64(k_uptime_ticks());
	data->sqe = NULL;

	const size_t min_read_size = sizeof(struct adc_ads131m08_fifo_data);

	if (rtio_sqe_rx_buf(current_sqe, min_read_size, min_read_size, &buf, &buf_len) != 0) {
		rtio_iodev_sqe_err(current_sqe, -ENOMEM);
		return;
	}

	__ASSERT(buf_len >= sizeof(struct adc_ads131m08_fifo_data), "Buffer too small");

	/* Read FIFO and call back to rtio with rtio_sqe completion */
	hdr = (struct adc_ads131m08_fifo_data *)buf;
	hdr->is_fifo = 1;
	hdr->wlen = ADS131M08_WORDLENGTH_OP;
	hdr->timestamp = data->timestamp;
	hdr->diff_mode = cfg->spec.channel_cfg.differential;
	hdr->frame_count = double_read ? 2 : 1; /* Signal callback to read 1 or 2 frames */

	if (cfg->spec.channel_cfg.reference == ADC_REF_INTERNAL) {
		hdr->vref_mv = (uint16_t)adc_ref_internal(cfg->spec.dev);
	} else {
		hdr->vref_mv = cfg->spec.vref_mv;
	}

	/* Set empty flag if trigger opt is NOP or DROP */
	if (read_config != NULL && read_config->trigger_cnt != 0) {
		data_opt = read_config->triggers[0].opt;

		for (int i = 1; i < read_config->trigger_cnt; i++) {
			data_opt = MIN(data_opt, read_config->triggers[i].opt);
		}

		if (data_opt == ADC_STREAM_DATA_NOP || data_opt == ADC_STREAM_DATA_DROP) {
			hdr->empty = 1;
		}
	}

	/* Setup RTIO chain: Enqueue one or two SPI transactions, depending on double_read */
	read_sample0 = rtio_sqe_acquire(data->rtio_ctx);

	if (read_sample0 == NULL) {
		rtio_iodev_sqe_err(current_sqe, -ENOMEM);
		return;
	}

	rtio_sqe_prep_read(read_sample0, data->iodev, RTIO_PRIO_NORM, (uint8_t *)&hdr->status,
			   ADS131M08_FRAMELENGTH, current_sqe);
	read_sample0->flags = RTIO_SQE_CHAINED;
	if (double_read) {
		read_sample1 = rtio_sqe_acquire(data->rtio_ctx);
		if (read_sample1 == NULL) {
			rtio_sqe_drop_all(data->rtio_ctx);
			rtio_iodev_sqe_err(current_sqe, -ENOMEM);
			return;
		}
		rtio_sqe_prep_read(read_sample1, data->iodev, RTIO_PRIO_NORM,
				   (uint8_t *)&hdr->status_1, ADS131M08_FRAMELENGTH, current_sqe);
		read_sample1->flags = RTIO_SQE_CHAINED;
	}
	complete_op = rtio_sqe_acquire(data->rtio_ctx);
	if (complete_op == NULL) {
		rtio_sqe_drop_all(data->rtio_ctx);
		rtio_iodev_sqe_err(current_sqe, -ENOMEM);
		return;
	}
	rtio_sqe_prep_callback(complete_op, ads131m08_process_sample_cb, (void *)(uintptr_t)dev,
			       current_sqe);
	rtio_submit(data->rtio_ctx, 0);
}

static bool ads131m08_decoder_has_trigger(const uint8_t *buffer, enum adc_trigger_type trigger)
{
	const struct adc_ads131m08_fifo_data *enc_data =
		(const struct adc_ads131m08_fifo_data *)buffer;

	if (!enc_data->is_fifo) {
		return false;
	}

	switch (trigger) {
	case ADC_TRIG_DATA_READY:
		return true;
	case ADC_TRIG_FIFO_WATERMARK:
		__fallthrough;
	case ADC_TRIG_FIFO_FULL:
		return (enc_data->frame_count >= 2);
	default:
		return false;
	}
}

ADC_DECODER_API_DT_DEFINE() = {
	.get_frame_count = ads131m08_decoder_get_frame_count,
	.decode = ads131m08_decoder_decode,
	.has_trigger = ads131m08_decoder_has_trigger,
};

static int ads131m08_get_decoder(const struct device *dev, const struct adc_decoder_api **api)
{
	ARG_UNUSED(dev);
	*api = &ADC_DECODER_NAME();

	return 0;
}

#endif /* CONFIG_ADS131M08_STREAM */

#if CONFIG_ADS131M08_TRIGGER
static int ads131m08_init_interrupt(const struct device *dev)
{
	const struct adc_ads131m08_config *cfg = dev->config;
	int ret;
	struct adc_ads131m08_data *drv_data = dev->data;

	if (!gpio_is_ready_dt(&cfg->gpio_drdy)) {
		LOG_ERR("GPIO port %s not ready", cfg->gpio_drdy.port->name);
		return -EINVAL;
	}

	ret = gpio_pin_configure_dt(&cfg->gpio_drdy, GPIO_INPUT);
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->gpio_drdy, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		return ret;
	}

	gpio_init_callback(&drv_data->gpio_drdy_cb, ads131m08_drdy_callback,
			   BIT(cfg->gpio_drdy.pin));

	ret = gpio_add_callback(cfg->gpio_drdy.port, &drv_data->gpio_drdy_cb);
	if (ret != 0) {
		LOG_ERR("Failed to set gpio callback for %s:%d (err=%d)", cfg->gpio_drdy.port->name,
			cfg->gpio_drdy.pin, ret);
		return ret;
	}

	return 0;
}
#endif

/**
 * @brief Configure the word length for the ADS131M08 ADC device.
 *
 * After reset the device uses 24-bit word length.
 * This function switches to 32-bit sign-extended mode.
 * @param dev Pointer to the ADS131M08 device structure.
 *
 * @return 0 on success, negative errno code on failure.
 */
static int ads131m08_configure_wlen(const struct device *dev)
{

	int ret;
	uint16_t cmd_word = ads131m08_get_wreg(ADS131M08_MODE_REG, 1);
	uint16_t reg = ADS131M08_MODE_DEFAULT;
	uint8_t tx_buf[ADS131M08_WORDLENGTH_AFTER_RESET * ADS131M08_WORDS_PER_FRAME] = {0};

	__ASSERT(sizeof(tx_buf) == 30, "tx_buf size calculation error");
	__ASSERT(cmd_word == 0x6100, "cmd_word: 0x%04x does not match expected 0x6100", cmd_word);
	sys_put_be16(cmd_word, tx_buf);
	reg &= ~ADS131M08_WLENGTH_MASK;
	reg |= ADS131M08_WLENGTH_32BIT_SIGNED;
	sys_put_be16(reg, &tx_buf[ADS131M08_WORDLENGTH_AFTER_RESET]);
	ret = ads131m08_transceive(dev, tx_buf, sizeof(tx_buf), NULL, 0);

	if (ret != 0) {
		LOG_ERR("Failed to set word length to 32 bits");
		return ret;
	}
	return ret;
}

static int ads131m08_init(const struct device *dev)
{
	const struct adc_ads131m08_config *cfg = dev->config;
	int ret;
	struct adc_ads131m08_data *data = dev->data;
	uint16_t buf = 0;
	uint8_t device_id;
	uint8_t expected_id;

	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR("SPI is not ready");
		return -ENODEV;
	}
	adc_context_init(&data->ctx);
	/* Ensure device pointer and semaphores are initialized before enabling IRQs */
	data->dev = dev;

#if CONFIG_ADS131M08_TRIGGER
	/* sem_drdy is used from the DRDY ISR; initialize it before configuring interrupts */
	k_sem_init(&data->sem_drdy, 0, 1);
	ret = ads131m08_init_interrupt(dev);
	if (ret != 0) {
		LOG_ERR("Failed to initialize interrupt! (err=%d)", ret);
		return ret;
	}
#endif
	ret = ads131m08_init_reset(dev);
	if (ret != 0) {
		LOG_ERR("Failed to initialize reset GPIO");
		return ret;
	}
	data->operation_mode = ADS131M08_CONTINUOUS_CONVERSION_FM;

	/* Reset */
	ret = ads131m08_reset_gpio(dev);
	if (ret < 0) {
		return ret;
	}

	ret = ads131m08_reg_read_short(dev, ADS131M08_ID_REG, &buf);
	if (ret != 0) {
		return ret;
	}

	device_id = (buf >> 8) & 0xFF;
	expected_id = ADS131M08_DEVICE_ID;
	if (device_id != expected_id) {
		LOG_ERR("Device ID mismatch 0x%02x is not equal to expected 0x%02x", device_id,
			(uint8_t)ADS131M08_DEVICE_ID);
		return -ENODEV;
	}

	/* Configure oversampling rate; all channels disabled initially */
	buf = ADS131M08_DISABLE_ADC;
	switch (cfg->spec.oversampling) {
	case VALUE_128_EXPONENT: {
		buf |= ADS131M08_OSR_128;
		break;
	}
	case VALUE_256_EXPONENT: {
		buf |= ADS131M08_OSR_256;
		break;
	}
	case VALUE_512_EXPONENT: {
		buf |= ADS131M08_OSR_512;
		break;
	}
	case VALUE_1024_EXPONENT: {
		buf |= ADS131M08_OSR_1024;
		break;
	}
	case VALUE_2048_EXPONENT: {
		buf |= ADS131M08_OSR_2048;
		break;
	}
	case VALUE_4096_EXPONENT: {
		buf |= ADS131M08_OSR_4096;
		break;
	}
	case VALUE_8192_EXPONENT: {
		buf |= ADS131M08_OSR_8192;
		break;
	}
	case VALUE_16384_EXPONENT: {
		buf |= ADS131M08_OSR_16384;
		break;
	}
	default: {
		LOG_ERR("Unsupported oversampling value");
		return -EINVAL;
	}
	}

	ret = ads131m08_reg_write_short(dev, ADS131M08_CLOCK_REG, buf);
	if (ret != 0) {
		LOG_ERR("Failed to set oversampling rate and disable ADC");
		return ret;
	}
	ret = ads131m08_set_operation_mode(dev, cfg->active_mode);
	if (ret != 0) {
		LOG_ERR("Failed to set operation mode");
		return ret;
	}
#ifdef CONFIG_ADS131M08_STREAM
	/* First execution needs to drain any accumulated FIFO samples */
	data->fifo_needs_drain = true;
#endif

	adc_context_unlock_unconditionally(&data->ctx);
	return ret;
}

int ads131m08_set_calibration(const struct device *dev, const struct ads131m08_calibration *cal)
{
	__ASSERT(ADS131M08_WORDLENGTH_OP == 4U, "Only 32-bit mode supported for calibration");
	int ret;

	/* 5 registers per channel: CFG, OCAL_MSB, OCAL_LSB, GCAL_MSB, GCAL_LSB */
	const uint8_t REGISTERS_PER_CHANNEL = 5U;
	const uint8_t BUFSIZE = ADS131M08_WORDLENGTH_OP + ADS131M08_WORDLENGTH_OP *
								  REGISTERS_PER_CHANNEL *
								  ADS131M08_ADC_CHANNELS;
	uint16_t cfg_reg;
	uint16_t cmd_word;
	uint8_t buf_idx;
	uint8_t send_buf[BUFSIZE];

	__ASSERT(sizeof(send_buf) == 164,
		 "Miscalculation of send buffer size for calibration data");
	memset(send_buf, 0, sizeof(send_buf));

	cmd_word = ads131m08_get_wreg(ADS131M08_CH0_CFG_REG,
				      REGISTERS_PER_CHANNEL * ADS131M08_ADC_CHANNELS);
	sys_put_be16(cmd_word, send_buf);
	buf_idx = ADS131M08_WORDLENGTH_OP;
	for (int ch = 0; ch < ADS131M08_ADC_CHANNELS; ch++) {
		const struct ads131m08_channel_cal *ch_cal = &cal->ch[ch];

		/* CHX_CFG: PHASE[15:6], DCBLK_DIS[2], MUX[1:0] */
		cfg_reg = ((ch_cal->phase & 0x3FF) << 6) | ((ch_cal->dcblk_dis & 0x1) << 2) |
			  (ch_cal->mux & 0x3);
		send_buf[buf_idx++] = (cfg_reg >> 8) & 0xFF;
		send_buf[buf_idx++] = cfg_reg & 0xFF;
		send_buf[buf_idx++] = 0x00;
		send_buf[buf_idx++] = 0x00;

		/* CHX_OCAL_MSB: offset[23:8] */
		send_buf[buf_idx++] = (ch_cal->offset >> 16) & 0xFF;
		send_buf[buf_idx++] = (ch_cal->offset >> 8) & 0xFF;
		send_buf[buf_idx++] = 0x00;
		send_buf[buf_idx++] = 0x00;

		/* CHX_OCAL_LSB: offset[7:0] */
		send_buf[buf_idx++] = ch_cal->offset & 0xFF;
		send_buf[buf_idx++] = 0x00;
		send_buf[buf_idx++] = 0x00;
		send_buf[buf_idx++] = 0x00;

		/* CHX_GCAL_MSB: gain[23:8] */
		send_buf[buf_idx++] = (ch_cal->gain >> 16) & 0xFF;
		send_buf[buf_idx++] = (ch_cal->gain >> 8) & 0xFF;
		send_buf[buf_idx++] = 0x00;
		send_buf[buf_idx++] = 0x00;

		/* CHX_GCAL_LSB: gain[7:0] */
		send_buf[buf_idx++] = ch_cal->gain & 0xFF;
		send_buf[buf_idx++] = 0x00;
		send_buf[buf_idx++] = 0x00;
		send_buf[buf_idx++] = 0x00;
		__ASSERT(buf_idx <= BUFSIZE, "buf_idx: %d - at ch: %d", buf_idx, ch);
		__ASSERT(buf_idx == (ch + 1) * 20 + 4, "buf_idx: %d - at ch: %d", buf_idx, ch);
	}
	__ASSERT(buf_idx == 164, "Final buffer index %d does not match expected size", buf_idx);
	ret = ads131m08_transceive(dev, send_buf, sizeof(send_buf), NULL, 0);
	return ret;
}

int ads131m08_get_calibration(const struct device *dev, struct ads131m08_calibration *cal)
{
	__ASSERT(ADS131M08_WORDLENGTH_OP == 4U, "Only 32-bit mode supported for calibration");
	int ret;
	struct ads131m08_channel_cal *ch_cal;
	uint16_t reg_val;

	/* Channel register base addresses: 5 consecutive registers per channel */
	static const uint8_t ch_base_addr[] = {
		ADS131M08_CH0_CFG_REG, ADS131M08_CH1_CFG_REG, ADS131M08_CH2_CFG_REG,
		ADS131M08_CH3_CFG_REG, ADS131M08_CH4_CFG_REG, ADS131M08_CH5_CFG_REG,
		ADS131M08_CH6_CFG_REG, ADS131M08_CH7_CFG_REG,
	};

	for (int ch = 0; ch < ADS131M08_ADC_CHANNELS; ch++) {
		ch_cal = &cal->ch[ch];
		const uint8_t base = ch_base_addr[ch];

		/* CHX_CFG Register */
		ret = ads131m08_reg_read_short(dev, base, &reg_val);
		if (ret != 0) {
			return ret;
		}
		ch_cal->phase = (int16_t)((reg_val >> 6) & 0x3FF);
		ch_cal->dcblk_dis = (reg_val >> 2) & 0x1;
		ch_cal->mux = reg_val & 0x3;

		/* CHX_OCAL_MSB Register */
		ret = ads131m08_reg_read_short(dev, base + 1, &reg_val);
		if (ret != 0) {
			return ret;
		}
		ch_cal->offset = (int32_t)((uint32_t)reg_val << 8);

		/* CHX_OCAL_LSB Register */
		ret = ads131m08_reg_read_short(dev, base + 2, &reg_val);
		if (ret != 0) {
			return ret;
		}
		ch_cal->offset |= (reg_val >> 8) & 0xFF;
		/* Sign-extend 24-bit to 32-bit */
		if (ch_cal->offset & BIT(23)) {
			ch_cal->offset |= (int32_t)0xFF000000U;
		}

		/* CHX_GCAL_MSB Register */
		ret = ads131m08_reg_read_short(dev, base + 3, &reg_val);
		if (ret != 0) {
			return ret;
		}
		ch_cal->gain = (uint32_t)reg_val << 8;

		/* CHX_GCAL_LSB Register */
		ret = ads131m08_reg_read_short(dev, base + 4, &reg_val);
		if (ret != 0) {
			return ret;
		}
		ch_cal->gain |= (reg_val >> 8) & 0xFF;
	}

	return 0;
}

static DEVICE_API(adc, ads131m08_api) = {
	.channel_setup = ads131m08_channel_setup,
	.read = ads131m08_read,
	.ref_internal = ADS131M08_REF_INTERNAL,
#ifdef CONFIG_ADC_ASYNC
	.read_async = ads131m08_read_async,
#endif
#ifdef CONFIG_ADC_STREAM
	.submit = ads131m08_submit_stream,
	.get_decoder = ads131m08_get_decoder,
#endif
};

#define ADS131M08_SPI_CFG SPI_OP_MODE_MASTER | SPI_MODE_CPHA | SPI_WORD_SET(8)

#define ADS131M08_RTIO_DEFINE(inst)                                                                \
	SPI_DT_IODEV_DEFINE(ads131m08_iodev_##inst, DT_DRV_INST(inst), ADS131M08_SPI_CFG);         \
	RTIO_DEFINE(ads131m08_rtio_ctx_##inst, 16, 16);

/* clang-format off */
#define ADS131M08_INIT(n)								\
	IF_ENABLED(CONFIG_ADS131M08_STREAM, (ADS131M08_RTIO_DEFINE(n)));		\
	static struct adc_ads131m08_data ads131m08_data_##n = {				\
		IF_ENABLED(CONFIG_ADS131M08_STREAM,					\
		(.rtio_ctx = &ads131m08_rtio_ctx_##n,					\
			.iodev = &ads131m08_iodev_##n))};				\
											\
	static const struct adc_ads131m08_config ads131m08_config_##n = {		\
		.spi = SPI_DT_SPEC_INST_GET(n, ADS131M08_SPI_CFG),			\
		.gpio_reset = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),		\
		.spec = ADC_DT_SPEC_STRUCT(DT_INST(n, DT_DRV_COMPAT), 0),		\
		IF_ENABLED(CONFIG_ADS131M08_TRIGGER,					\
			(.gpio_drdy = GPIO_DT_SPEC_INST_GET(n, drdy_gpios),)) };	\
	DEVICE_DT_INST_DEFINE(n, ads131m08_init, NULL, &ads131m08_data_##n,		\
		&ads131m08_config_##n, POST_KERNEL,					\
		CONFIG_ADC_INIT_PRIORITY, &ads131m08_api);
/* clang-format on */

#define DT_DRV_COMPAT ti_ads131m08

DT_INST_FOREACH_STATUS_OKAY(ADS131M08_INIT)
