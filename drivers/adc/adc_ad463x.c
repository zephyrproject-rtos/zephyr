/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Driver for Analog Devices AD463x family precision SAR ADCs (offload mode).
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/cache.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>
#include <zephyr/types.h>

#include <zephyr/drivers/adc/ad463x.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/spi_adi_axi_spi_engine.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pwm.h>

#define DT_DRV_COMPAT adi_ad463x

LOG_MODULE_REGISTER(adc_ad463x, CONFIG_ADC_AD463X_LOG_LEVEL);

/* Special 16-bit "config timing" sequence sent right after reset for decoder
 * alignment. 0x2000 → reg_high=0x20 → with READ flag becomes 0xA0; reg_low=0x00.
 */
#define AD463X_CONFIG_TIMING 0x2000

/* Register addresses */
#define AD463X_REG_INTERFACE_CONFIG_A 0x00
#define AD463X_REG_INTERFACE_CONFIG_B 0x01
#define AD463X_REG_DEVICE_CONFIG      0x02
#define AD463X_REG_CHIP_TYPE          0x03
#define AD463X_REG_PRODUCT_ID_L       0x04
#define AD463X_REG_PRODUCT_ID_H       0x05
#define AD463X_REG_CHIP_GRADE         0x06
#define AD463X_REG_SCRATCH_PAD        0x0A
#define AD463X_REG_SPI_REVISION       0x0B
#define AD463X_REG_VENDOR_L           0x0C
#define AD463X_REG_VENDOR_H           0x0D
#define AD463X_REG_STREAM_MODE        0x0E
#define AD463X_REG_EXIT_CFG_MODE      0x14
#define AD463X_REG_AVG                0x15
#define AD463X_REG_OFFSET_BASE        0x16
#define AD463X_REG_GAIN_BASE          0x1C
#define AD463X_REG_MODES              0x20
#define AD463X_REG_OSCILATOR          0x21
#define AD463X_REG_IO                 0x22
#define AD463X_REG_PAT0               0x23
#define AD463X_REG_PAT1               0x24
#define AD463X_REG_PAT2               0x25
#define AD463X_REG_PAT3               0x26
#define AD463X_REG_DIG_DIAG           0x34
#define AD463X_REG_DIG_ERR            0x35

#define AD463X_REG_CHAN_OFFSET(ch, pos) (AD463X_REG_OFFSET_BASE + (3 * (ch)) + (pos))
#define AD463X_REG_CHAN_GAIN(ch, pos)   (AD463X_REG_GAIN_BASE + (2 * (ch)) + (pos))

/* INTERFACE_CONFIG_A */
#define AD463X_CFG_SW_RESET   ((1 << 7) | (1 << 0))
#define AD463X_CFG_SDO_ENABLE (1 << 4)

/* MODES bit fields */
#define AD463X_LANE_MODE_MSK     ((1 << 7) | (1 << 6))
#define AD463X_CLK_MODE_MSK      ((1 << 5) | (1 << 4))
#define AD463X_DDR_MODE_MSK      (1 << 3)
#define AD463X_OUT_DATA_MODE_MSK ((1 << 2) | (1 << 1) | (1 << 0))

/* MODES values */
#define AD463X_SDR_MODE 0x00
#define AD463X_DDR_MODE (1 << 3)

#define AD463X_24_DIFF          0x00
#define AD463X_16_DIFF_8_COM    0x01
#define AD463X_24_DIFF_8_COM    0x02
#define AD463X_30_AVERAGED_DIFF 0x03
#define AD463X_32_PATTERN       0x04

#define AD463X_ONE_LANE_PER_CH   0x00
#define AD463X_TWO_LANES_PER_CH  (1 << 6)
#define AD463X_FOUR_LANES_PER_CH (1 << 7)
#define AD463X_SHARED_TWO_CH     ((1 << 6) | (1 << 7))

#define AD463X_SPI_COMPATIBLE_MODE 0x00
#define AD463X_ECHO_CLOCK_MODE     (1 << 4)
#define AD463X_CLOCK_MASTER_MODE   (1 << 5)

/* EXIT_CFG_MD */
#define AD463X_EXIT_CFG_MODE (1 << 0)

/* DEVICE_CONFIG power modes */
#define AD463X_NORMAL_MODE    0x00
#define AD463X_LOW_POWER_MODE ((1 << 1) | (1 << 0))

/* AVG */
#define AD463X_AVG_FILTER_RESET (1 << 7)

/* IO drive strength */
#define AD463X_DRIVER_STRENGTH_MASK   (1 << 0)
#define AD463X_NORMAL_OUTPUT_STRENGTH 0x00
#define AD463X_DOUBLE_OUTPUT_STRENGTH (1 << 1)

/* SPI framing */
#define AD463X_REG_READ        (1 << 7)
#define AD463X_REG_WRITE       0x00
#define AD463X_REG_READ_DUMMY  0x00
/* 7-bit register address field, high byte of the 16-bit register frame */
#define AD463X_REG_ADDR_HI_MSK 0x7F

/* Datasheet-specified CNV pulse width */
#define AD463X_TRIGGER_PULSE_WIDTH_NS 10

/* Magic value used by the scratchpad ID test */
#define AD463X_SCRATCH_TEST 0xAA

/*
 * Mapping from the binding strings to register-bit values. Kept as
 * lookup tables so adding a new mode never touches the C code beyond
 * the binding's enum list and these tables.
 */
struct ad463x_part_info {
	const char *part;
	enum ad463x_id id;
	uint8_t real_bits_default; /* used when output_mode == 24-diff */
};

static const struct ad463x_part_info ad463x_parts[] = {
	{"ad4630-24", AD463X_ID_AD4630_24, 24}, {"ad4630-20", AD463X_ID_AD4630_20, 20},
	{"ad4630-16", AD463X_ID_AD4630_16, 16}, {"ad4631-24", AD463X_ID_AD4631_24, 24},
	{"ad4631-20", AD463X_ID_AD4631_20, 20}, {"ad4631-16", AD463X_ID_AD4631_16, 16},
	{"ad4632-24", AD463X_ID_AD4632_24, 24}, {"ad4632-20", AD463X_ID_AD4632_20, 20},
	{"ad4632-16", AD463X_ID_AD4632_16, 16},
};

struct ad463x_config {
	struct spi_dt_spec bus;
	const struct device *dmac;
	const struct device *clkgen;
	const struct device *pwmgen;
	uint32_t dma_channel;
	uint32_t pwmgen_channel;
	uint32_t vref_uv;
	uint32_t clkgen_rate_hz;
	uint32_t sample_rate_hz;
	uint8_t lane_mode;
	uint8_t clock_mode;
	uint8_t output_mode;
	uint8_t data_rate;
	uint8_t avg_frame_len;
	bool skip_reg_config;
	const char *part;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec cnv_gpio;
};

/*
 * Continuous-capture ring used by the ADC API shim. Cyclic DMA fills it
 * at adi,sample-rate-hz; ad463x_adc_read() snapshots the most-recently
 * written sample slot. 16 slots × 8 bytes = 128 B, comfortably above
 * the cache-line size on Cortex-A9 (32 B) so a single uint32_t load
 * never spans two slots being written.
 */
#define AD463X_CONT_RING_SAMPLES 16
#define AD463X_CONT_RING_BYTES   (AD463X_CONT_RING_SAMPLES * 8)

struct ad463x_data {
	const struct ad463x_part_info *info;
	uint8_t capture_data_width; /* bits per word per SDI lane */
	uint8_t bytes_per_sample;   /* total bytes the HDL produces per CNV */
	uint8_t real_bits_precision;
	bool chip_initialized;
	bool continuous_running;
	struct k_mutex lock;
	struct k_sem dma_done; /* signalled from the DMA completion callback */
	uint32_t cont_commands[8];
	uint8_t ring[AD463X_CONT_RING_BYTES] __aligned(32);
};

/*
 * DMA completion callback. The one-shot capture path waits on dma_done; the
 * cyclic ring reports DMA_STATUS_BLOCK on each wrap, which we ignore.
 */
static void ad463x_dma_cb(const struct device *dma_dev, void *user_data, uint32_t channel,
			  int status)
{
	struct ad463x_data *data = user_data;

	ARG_UNUSED(dma_dev);
	ARG_UNUSED(channel);

	if (status == DMA_STATUS_COMPLETE) {
		k_sem_give(&data->dma_done);
	}
}

/*
 * Program the DMAC for a device-to-memory transfer into buf. Cyclic mode arms
 * the continuous ring; one-shot mode fills buf once and signals dma_done.
 */
static int ad463x_dma_arm(const struct device *dev, void *buf, uint32_t bytes, bool cyclic)
{
	const struct ad463x_config *cfg = dev->config;
	struct ad463x_data *data = dev->data;
	struct dma_block_config block = {
		.dest_address = (uint32_t)(uintptr_t)buf,
		.block_size = bytes,
	};
	struct dma_config dma_cfg = {
		.channel_direction = PERIPHERAL_TO_MEMORY,
		.complete_callback_en = 0,
		.cyclic = cyclic,
		.block_count = 1,
		.head_block = &block,
		.dma_callback = ad463x_dma_cb,
		.user_data = data,
	};
	int ret;

	k_sem_reset(&data->dma_done);

	ret = dma_config(cfg->dmac, cfg->dma_channel, &dma_cfg);
	if (ret) {
		return ret;
	}

	return dma_start(cfg->dmac, cfg->dma_channel);
}

/*
 * Encode a 3-byte register frame and push it through the engine's
 * command FIFO. The frame layout matches the AD463x datasheet:
 *   [0] = (read ? AD463X_REG_READ : 0x00) | (addr_hi & AD463X_REG_ADDR_HI_MSK)
 *   [1] = addr_lo
 *   [2] = data       (write)  or  dummy (read; value comes back here)
 *
 * Reg access speed is the spi-max-frequency of the DT SPI child; the 8-bit
 * word size configured on the bus matches the frame format.
 */
static int ad463x_spi_reg_read(const struct device *dev, uint16_t reg, uint8_t *val)
{
	const struct ad463x_config *cfg = dev->config;

	if (val == NULL) {
		return -EINVAL;
	}

	uint8_t tx[3] = {
		AD463X_REG_READ | ((reg >> 8) & AD463X_REG_ADDR_HI_MSK),
		(uint8_t)reg,
		AD463X_REG_READ_DUMMY,
	};
	uint8_t rx[3] = {0};
	const struct spi_buf tx_buf = {.buf = tx, .len = sizeof(tx)};
	const struct spi_buf rx_buf = {.buf = rx, .len = sizeof(rx)};
	const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf_set rx_set = {.buffers = &rx_buf, .count = 1};
	int ret;

	ret = spi_transceive_dt(&cfg->bus, &tx_set, &rx_set);
	if (ret) {
		return ret;
	}
	*val = rx[2];
	return 0;
}

static int ad463x_spi_reg_write(const struct device *dev, uint16_t reg, uint8_t val)
{
	const struct ad463x_config *cfg = dev->config;
	uint8_t tx[3] = {
		((reg >> 8) & AD463X_REG_ADDR_HI_MSK),
		(uint8_t)reg,
		val,
	};
	const struct spi_buf tx_buf = {.buf = tx, .len = sizeof(tx)};
	const struct spi_buf_set tx_set = {.buffers = &tx_buf, .count = 1};

	return spi_transceive_dt(&cfg->bus, &tx_set, NULL);
}

static int ad463x_spi_reg_write_masked(const struct device *dev, uint16_t reg, uint8_t mask,
				       uint8_t val)
{
	uint8_t cur;
	int ret;

	ret = ad463x_spi_reg_read(dev, reg, &cur);
	if (ret) {
		return ret;
	}
	cur &= ~mask;
	cur |= (val & mask);
	return ad463x_spi_reg_write(dev, reg, cur);
}

static int ad463x_set_avg_frame_len(const struct device *dev, uint8_t mode)
{
	int ret;

	if (mode == 0) {
		ret = ad463x_spi_reg_write_masked(dev, AD463X_REG_MODES, AD463X_OUT_DATA_MODE_MSK,
						  AD463X_24_DIFF_8_COM);
		if (ret) {
			return ret;
		}
		return ad463x_spi_reg_write(dev, AD463X_REG_AVG, AD463X_AVG_FILTER_RESET);
	}

	if (mode > 16) {
		return -EINVAL;
	}
	ret = ad463x_spi_reg_write_masked(dev, AD463X_REG_MODES, AD463X_OUT_DATA_MODE_MSK,
					  AD463X_30_AVERAGED_DIFF);
	if (ret) {
		return ret;
	}
	return ad463x_spi_reg_write(dev, AD463X_REG_AVG, mode);
}

/*
 * Compute capture_data_width / bytes_per_sample / real_bits from the
 * configured lane / output / data-rate.
 *
 * Per channel, sample_width = 32 if output_mode > 16_DIFF_8_COM else 24.
 * Capture data width per lane = sample_width / lanes_per_ch.
 * In SHARED_TWO_CH the two channels are time-multiplexed on a single SDI
 * lane, so capture width is 2*sample_width.
 *
 * In DDR mode the hardware halves SCLK requirements per bit, so the
 * capture XFER_BITS is halved.
 *
 * bytes_per_sample is what the HDL produces per CNV — both channels
 * combined: 2*sample_width/8 = sample_width/4 (= 6 for 24-bit modes,
 * 8 for 32-bit modes).
 */
static int ad463x_compute_widths(const struct device *dev)
{
	const struct ad463x_config *cfg = dev->config;
	struct ad463x_data *data = dev->data;
	uint8_t sample_width;

	if (cfg->output_mode > AD463X_16_DIFF_8_COM) {
		sample_width = 32;
	} else {
		sample_width = 24;
	}

	switch (cfg->lane_mode) {
	case AD463X_ONE_LANE_PER_CH:
		data->capture_data_width = sample_width;
		break;
	case AD463X_TWO_LANES_PER_CH:
		data->capture_data_width = sample_width / 2;
		break;
	case AD463X_FOUR_LANES_PER_CH:
		data->capture_data_width = sample_width / 4;
		break;
	case AD463X_SHARED_TWO_CH:
		data->capture_data_width = sample_width * 2;
		break;
	default:
		return -EINVAL;
	}

	if (cfg->data_rate == AD463X_DDR_MODE) {
		data->capture_data_width /= 2;
	}

	switch (cfg->output_mode) {
	case AD463X_24_DIFF:
		data->real_bits_precision = data->info->real_bits_default;
		break;
	case AD463X_16_DIFF_8_COM:
		data->real_bits_precision = 16;
		break;
	case AD463X_24_DIFF_8_COM:
		data->real_bits_precision = 24;
		break;
	case AD463X_30_AVERAGED_DIFF:
		data->real_bits_precision = 30;
		break;
	case AD463X_32_PATTERN:
		data->real_bits_precision = 32;
		break;
	default:
		return -EINVAL;
	}

	/* Both channels combined per CNV. */
	data->bytes_per_sample = sample_width / 4;
	return 0;
}

static int ad463x_apply_capture_config(const struct device *dev)
{
	const struct ad463x_config *cfg = dev->config;
	int ret;

	ret = ad463x_spi_reg_write_masked(dev, AD463X_REG_MODES, AD463X_LANE_MODE_MSK,
					  cfg->lane_mode);
	if (ret) {
		LOG_ERR("MODES.lane_mode write failed (%d)", ret);
		return ret;
	}
	ret = ad463x_spi_reg_write_masked(dev, AD463X_REG_MODES, AD463X_CLK_MODE_MSK,
					  cfg->clock_mode);
	if (ret) {
		LOG_ERR("MODES.clock_mode write failed (%d)", ret);
		return ret;
	}
	ret = ad463x_spi_reg_write_masked(dev, AD463X_REG_MODES, AD463X_DDR_MODE_MSK,
					  cfg->data_rate);
	if (ret) {
		LOG_ERR("MODES.data_rate write failed (%d)", ret);
		return ret;
	}
	ret = ad463x_spi_reg_write_masked(dev, AD463X_REG_MODES, AD463X_OUT_DATA_MODE_MSK,
					  cfg->output_mode);
	if (ret) {
		LOG_ERR("MODES.output_mode write failed (%d)", ret);
		return ret;
	}

	if (cfg->avg_frame_len > 0) {
		ret = ad463x_set_avg_frame_len(dev, cfg->avg_frame_len);
		if (ret) {
			LOG_ERR("avg_frame_len=%u write failed (%d)", cfg->avg_frame_len, ret);
			return ret;
		}
		LOG_INF("averaging enabled: %u conversions → 30-bit output", cfg->avg_frame_len);
	}

	return 0;
}

static int ad463x_exit_reg_cfg_mode(const struct device *dev)
{
	return ad463x_spi_reg_write(dev, AD463X_REG_EXIT_CFG_MODE, AD463X_EXIT_CFG_MODE);
}

int ad463x_init_chip(const struct device *dev)
{
	const struct ad463x_config *cfg = dev->config;
	uint8_t scratch;
	int ret;

	ret = clock_control_set_rate(cfg->clkgen, CLOCK_CONTROL_SUBSYS_ALL,
				     (clock_control_subsys_rate_t)(uintptr_t)cfg->clkgen_rate_hz);
	if (ret) {
		LOG_ERR("clkgen_set_rate(%u) failed (%d)", cfg->clkgen_rate_hz, ret);
		return ret;
	}

	if (cfg->reset_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret) {
			LOG_WRN("reset-gpios configure failed (%d)", ret);
		} else {
			gpio_pin_set_dt(&cfg->reset_gpio, 1);
			k_msleep(10);
			gpio_pin_set_dt(&cfg->reset_gpio, 0);
			k_msleep(10);
		}
	}

	if (cfg->cnv_gpio.port != NULL) {
		ret = gpio_pin_configure_dt(&cfg->cnv_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret) {
			LOG_WRN("cnv-gpios configure failed (%d)", ret);
		}
	}

	if (cfg->skip_reg_config) {
		LOG_INF("adi,skip-reg-config: trusting power-up defaults");
		return 0;
	}

	/* AD463X_CONFIG_TIMING read aligns the chip's SPI decoder after reset. */
	(void)ad463x_spi_reg_read(dev, AD463X_CONFIG_TIMING, &scratch);

	ret = ad463x_spi_reg_write(dev, AD463X_REG_SCRATCH_PAD, AD463X_SCRATCH_TEST);
	if (ret) {
		LOG_ERR("scratchpad write failed (%d)", ret);
		return ret;
	}
	ret = ad463x_spi_reg_read(dev, AD463X_REG_SCRATCH_PAD, &scratch);
	if (ret) {
		LOG_ERR("scratchpad read failed (%d)", ret);
		return ret;
	}
	if (scratch != AD463X_SCRATCH_TEST) {
		LOG_ERR("scratchpad read 0x%02x, expected 0x%02x — chip not responding", scratch,
			AD463X_SCRATCH_TEST);
		return -ENODEV;
	}

	return ad463x_apply_capture_config(dev);
}

/*
 * Lazy chip bring-up shared by the one-shot block path and the cyclic ring
 * path. Idempotent: after the first successful call the chip is out of reset
 * and out of register-config mode, and EXIT_CFG_MODE has been issued — past
 * that point the chip only speaks conversion data on CNV, so this must not
 * be re-run without a fresh reset. ad463x_init() deliberately leaves bring-up
 * to the first capture so the application controls reset/CNV timing.
 */
static int ad463x_ensure_chip_ready(const struct device *dev)
{
	struct ad463x_data *data = dev->data;
	int ret;

	if (data->chip_initialized) {
		return 0;
	}

	ret = ad463x_init_chip(dev);
	if (ret) {
		LOG_ERR("init_chip failed (%d)", ret);
		return ret;
	}
	ret = ad463x_exit_reg_cfg_mode(dev);
	if (ret) {
		LOG_ERR("exit_reg_cfg_mode failed (%d)", ret);
		return ret;
	}
	data->chip_initialized = true;
	return 0;
}

/*
 * Offload-mode read.
 *
 *   1. Build the offload program: switch XFER_BITS / CLK_DIV to the
 *      capture configuration, assert CS_LOW, do one READ_N_WORDS(1)
 *      transfer, deassert CS_HIGH. No SYNC — the offload module replays
 *      the program verbatim on every PWMGEN trigger.
 *   2. Arm DMAC for samples * bytes_per_sample bytes.
 *   3. Configure + enable PWMGEN.
 *   4. Enable engine offload (must be ready before first CNV).
 *   5. Wait for DMA completion. Tear everything down.
 *   6. SHARED_TWO_CH post-process: the HDL emits big-endian words on a
 *      single lane; convert in place.
 */
static int ad463x_read_data(const struct device *dev, uint32_t *buf, uint16_t samples)
{
	const struct ad463x_config *cfg = dev->config;
	struct ad463x_data *data = dev->data;
	struct spi_engine_offload_msg msg = {0};
	uint32_t commands[8];
	uint32_t cmd_idx = 0;
	uint32_t total_bytes;
	uint32_t timeout_ms;
	uint32_t period_ns;
	int ret;
	int wait_ret = 0;

	if (buf == NULL || samples == 0) {
		return -EINVAL;
	}

	total_bytes = (uint32_t)samples * data->bytes_per_sample;

	/* clk_div=0 runs the capture SCLK at clkgen_rate/2 (80 MHz at our
	 * 160 MHz ref). At 2 MSPS the 500 ns CNV period must fit 16 SCLK
	 * cycles (200 ns at 80 MHz) after ~265 ns tCONV, which it does.
	 */
	commands[cmd_idx++] = SPI_ENGINE_CMD_BUILD_CONFIG_CLK_DIV(0);
	commands[cmd_idx++] = SPI_ENGINE_CMD_BUILD_CONFIG_XFER_BITS(data->capture_data_width);
	/* CS must remain asserted during the SCLK burst; SDO floats without it
	 * even in echo-clock mode (chip gates SDO driver on CS_N).
	 */
	commands[cmd_idx++] = SPI_ENGINE_CMD_BUILD_CS_LOW;
	commands[cmd_idx++] = SPI_ENGINE_CMD_BUILD_READ_N_WORDS(1);
	commands[cmd_idx++] = SPI_ENGINE_CMD_BUILD_CS_HIGH;

	msg.commands = commands;
	msg.num_commands = cmd_idx;

	/* Flush before DMA to avoid stale dirty cache lines racing the write. */
	sys_cache_data_flush_range(buf, total_bytes);

	ret = spi_engine_offload_load(cfg->bus.bus, &msg);
	if (ret) {
		LOG_ERR("offload_load failed (%d)", ret);
		return ret;
	}

	ret = ad463x_dma_arm(dev, buf, total_bytes, false);
	if (ret) {
		LOG_ERR("dma arm failed (%d)", ret);
		goto out_offload;
	}

	if (cfg->sample_rate_hz == 0) {
		LOG_ERR("adi,sample-rate-hz must be > 0");
		ret = -EINVAL;
		goto out_dmac;
	}
	period_ns = 1000000000U / cfg->sample_rate_hz;

	ret = spi_engine_offload_enable(cfg->bus.bus, true);
	if (ret) {
		LOG_ERR("offload_enable failed (%d)", ret);
		goto out_dmac;
	}

	/*
	 * Arm the CNV train last: pwm_set() commits immediately, so the
	 * offload path must already be enabled to catch the first conversion.
	 *
	 * The cnv_generator core has TWO pwm outputs sharing one period:
	 *   ch0 (pwm_0) -> SPI-engine trigger (drives the readout)
	 *   ch1 (pwm_1) -> ADC CNV pin        (drives the conversion)
	 * BOTH must be programmed to the same period or the readout and the
	 * conversion run at different rates. Programming only ch0 leaves ch1 at
	 * the HDL build-time default (1 MHz); at a requested 2 MHz that makes the
	 * readout fire 2x per conversion and every sample is captured twice.
	 *
	 * The PWMGEN LOAD bit is global — each pwm_set() commits ALL channels'
	 * shadow regs. A prior capture's teardown leaves both period regs at 0, so
	 * programming ch0 (readout) first would fire a LOAD with ch1 (CNV) still
	 * stopped: the readout then re-latches one held conversion for the several
	 * microseconds until the ch1 pwm_set() call starts CNV, producing a run of
	 * identical samples at the start of every capture after the first. Program
	 * the CNV channel (ch1) FIRST so that when ch0's LOAD fires, conversions
	 * are already live.
	 */
	ret = pwm_set(cfg->pwmgen, cfg->pwmgen_channel + 1, period_ns,
		      AD463X_TRIGGER_PULSE_WIDTH_NS, PWM_POLARITY_NORMAL);
	if (ret) {
		LOG_ERR("pwm_set ch1 failed (%d)", ret);
		goto out_offload;
	}
	/* Program the readout-trigger channel (ch0) last. */
	ret = pwm_set(cfg->pwmgen, cfg->pwmgen_channel, period_ns, AD463X_TRIGGER_PULSE_WIDTH_NS,
		      PWM_POLARITY_NORMAL);
	if (ret) {
		LOG_ERR("pwm_set failed (%d)", ret);
		goto out_offload;
	}

	timeout_ms = DIV_ROUND_UP((uint32_t)samples * 1000U, cfg->sample_rate_hz);
	timeout_ms = timeout_ms * 4 + 5;

	if (k_sem_take(&data->dma_done, K_MSEC(timeout_ms)) != 0) {
		wait_ret = -ETIMEDOUT;
		LOG_ERR("DMA wait timed out after %u ms — "
			"DCO not echoing or PWM/offload misaligned",
			timeout_ms);
	}

	/* Disable both CNV-generator outputs (ch0 trigger + ch1 CNV pin). */
	(void)pwm_set(cfg->pwmgen, cfg->pwmgen_channel, 0, 0, PWM_POLARITY_NORMAL);
	(void)pwm_set(cfg->pwmgen, cfg->pwmgen_channel + 1, 0, 0, PWM_POLARITY_NORMAL);
out_offload:
	spi_engine_offload_enable(cfg->bus.bus, false);
out_dmac:
	dma_stop(cfg->dmac, cfg->dma_channel);

	if (ret == 0 && wait_ret) {
		ret = wait_ret;
	}
	if (ret) {
		return ret;
	}

	sys_cache_data_invd_range(buf, total_bytes);

	if (cfg->lane_mode == AD463X_SHARED_TWO_CH) {
		uint32_t words = total_bytes / sizeof(uint32_t);

		/* HDL emits big-endian words on this path; swap to host order. */
		for (uint32_t i = 0; i < words; i++) {
			buf[i] = sys_be32_to_cpu(buf[i]);
		}
	}

	return 0;
}

/*
 * Bulk binary buffer capture — the high-throughput path.
 *
 * Fills @p buf with raw little-endian conversion samples straight off the
 * AXI DMAC via the offload path, with no per-sample CPU cost. Intended as
 * the workhorse behind a streaming buffer consumer (e.g. a libiio readbuf
 * callback).
 *
 * @param dev AD463x device.
 * @param buf Destination. Must be 32-byte aligned.
 * @param len Capacity of @p buf in bytes.
 *
 * @return Number of bytes written on success, or a negative errno.
 */
ssize_t ad463x_read_buffer(const struct device *dev, void *buf, size_t len)
{
	struct ad463x_data *data = dev->data;
	size_t max_samples;
	uint16_t samples;
	int ret;

	if (buf == NULL) {
		return -EINVAL;
	}
	if (data->bytes_per_sample == 0) {
		return -EIO;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->continuous_running) {
		LOG_ERR("buffer capture rejected: cyclic ring is active");
		k_mutex_unlock(&data->lock);
		return -EBUSY;
	}

	max_samples = len / data->bytes_per_sample;
	if (max_samples == 0) {
		LOG_ERR("buffer too small: %zu B < one sample (%u B)", len, data->bytes_per_sample);
		k_mutex_unlock(&data->lock);
		return -ENOMEM;
	}

	if (max_samples > UINT16_MAX) {
		max_samples = UINT16_MAX;
	}
	samples = (uint16_t)max_samples;

	ret = ad463x_ensure_chip_ready(dev);
	if (ret == 0) {
		ret = ad463x_read_data(dev, (uint32_t *)buf, samples);
	}

	k_mutex_unlock(&data->lock);

	if (ret) {
		return ret;
	}
	return (ssize_t)samples * data->bytes_per_sample;
}

size_t ad463x_get_frame_size(const struct device *dev)
{
	const struct ad463x_data *data = dev->data;

	return data->bytes_per_sample;
}

uint8_t ad463x_get_real_bits(const struct device *dev)
{
	const struct ad463x_data *data = dev->data;

	return data->real_bits_precision;
}

/*
 * ============================================================================
 * Standard Zephyr ADC API shim (continuous-capture model)
 * ============================================================================
 *
 * The AD463x is fundamentally an offload-mode part — every sample is pushed
 * by the AXI SPI Engine + DMAC at MS/s rates triggered by a PWMGEN CNV pulse.
 * The Zephyr ADC API, by contrast, is a pull-style "give me N samples now"
 * interface. To bridge the two we run the offload+DMAC in cyclic mode into a
 * small ring buffer, lazily armed on the first adc_read(). After that the
 * hardware fills the ring autonomously at adi,sample-rate-hz with zero CPU
 * cost, and adc_read() degenerates to:
 *   1. Invalidate the ring's cache lines (the DMAC bypasses the L1/L2)
 *   2. Load one uint32 per channel from a fixed slot
 *   3. Hand it to the caller
 *
 * adc_read() always reads ring slot 0, not the DMAC's current write
 * position — slot 0's age cycles between 0 and (AD463X_CONT_RING_SAMPLES - 1)
 * conversions depending on where in the wrap the DMAC currently is, bounded
 * by AD463X_CONT_RING_SAMPLES / adi,sample-rate-hz seconds. This is
 * negligible next to adc_read()'s own call overhead (mutex, cache
 * maintenance), so tracking the write position isn't worth the complexity —
 * this shim is the slow-polling path; use ad463x_read_buffer() wherever
 * sample recency or throughput matters.
 */

/*
 * Arm the cyclic capture. Idempotent — second call is a no-op.
 */
static int ad463x_continuous_start(const struct device *dev)
{
	const struct ad463x_config *cfg = dev->config;
	struct ad463x_data *data = dev->data;
	struct spi_engine_offload_msg msg = {0};
	uint32_t cmd_idx = 0;
	uint32_t period_ns;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->continuous_running) {
		k_mutex_unlock(&data->lock);
		return 0;
	}

	if (cfg->sample_rate_hz == 0) {
		LOG_ERR("adi,sample-rate-hz must be > 0");
		k_mutex_unlock(&data->lock);
		return -EINVAL;
	}

	ret = ad463x_ensure_chip_ready(dev);
	if (ret) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	data->cont_commands[cmd_idx++] = SPI_ENGINE_CMD_BUILD_CONFIG_CLK_DIV(0);
	data->cont_commands[cmd_idx++] =
		SPI_ENGINE_CMD_BUILD_CONFIG_XFER_BITS(data->capture_data_width);
	data->cont_commands[cmd_idx++] = SPI_ENGINE_CMD_BUILD_CS_LOW;
	data->cont_commands[cmd_idx++] = SPI_ENGINE_CMD_BUILD_READ_N_WORDS(1);
	data->cont_commands[cmd_idx++] = SPI_ENGINE_CMD_BUILD_CS_HIGH;

	msg.commands = data->cont_commands;
	msg.num_commands = cmd_idx;

	sys_cache_data_flush_range(data->ring, AD463X_CONT_RING_BYTES);

	ret = spi_engine_offload_load(cfg->bus.bus, &msg);
	if (ret) {
		LOG_ERR("offload_load failed (%d)", ret);
		k_mutex_unlock(&data->lock);
		return ret;
	}

	ret = ad463x_dma_arm(dev, data->ring, AD463X_CONT_RING_BYTES, true);
	if (ret) {
		LOG_ERR("dma arm (cyclic) failed (%d)", ret);
		k_mutex_unlock(&data->lock);
		return ret;
	}

	period_ns = 1000000000U / cfg->sample_rate_hz;

	/*
	 * Order matters: enable the SPI Engine offload BEFORE the PWMGEN.
	 * If PWMGEN fires a CNV first, the chip drives a conversion result
	 * onto SDI but no one is clocking SCLK — the data is lost and the
	 * chip+engine drift out of frame for the rest of the session.
	 */
	ret = spi_engine_offload_enable(cfg->bus.bus, true);
	if (ret) {
		LOG_ERR("offload_enable failed (%d)", ret);
		dma_stop(cfg->dmac, cfg->dma_channel);
		k_mutex_unlock(&data->lock);
		return ret;
	}

	/*
	 * pwm_set() commits immediately, so arm the CNV train last.
	 * Program BOTH cnv_generator outputs (ch0=SPI trigger, ch1=ADC CNV pin)
	 * to the same period; leaving ch1 at the HDL default makes the readout
	 * and the conversion run at different rates. See ad463x_read_data().
	 *
	 * Program the CNV channel (ch1) FIRST: the PWMGEN LOAD bit is global, so
	 * arming ch0 before ch1 would start the readout trigger while conversions
	 * are still stopped, and the offload path would re-latch one held sample
	 * for several microseconds until ch1 starts. See ad463x_read_data().
	 */
	ret = pwm_set(cfg->pwmgen, cfg->pwmgen_channel + 1, period_ns,
		      AD463X_TRIGGER_PULSE_WIDTH_NS, PWM_POLARITY_NORMAL);
	if (ret == 0) {
		ret = pwm_set(cfg->pwmgen, cfg->pwmgen_channel, period_ns,
			      AD463X_TRIGGER_PULSE_WIDTH_NS, PWM_POLARITY_NORMAL);
	}
	if (ret) {
		LOG_ERR("pwm_set failed (%d)", ret);
		spi_engine_offload_enable(cfg->bus.bus, false);
		dma_stop(cfg->dmac, cfg->dma_channel);
		k_mutex_unlock(&data->lock);
		return ret;
	}

	data->continuous_running = true;
	k_mutex_unlock(&data->lock);
	LOG_INF("AD463x continuous capture armed at %u Hz", cfg->sample_rate_hz);
	return 0;
}

/* Accepts channel_id 0/1 with external reference and unity gain only. */
static int ad463x_adc_channel_setup(const struct device *dev,
				    const struct adc_channel_cfg *channel_cfg)
{
	ARG_UNUSED(dev);

	if (channel_cfg->channel_id > 1) {
		return -EINVAL;
	}
	if (channel_cfg->gain != ADC_GAIN_1) {
		return -ENOTSUP;
	}
	if (channel_cfg->reference != ADC_REF_EXTERNAL0) {
		return -ENOTSUP;
	}
	return 0;
}

/*
 * Snapshot ring slot 0 — see the staleness note above the shim's header
 * comment for why this is a recent sample, not necessarily the latest one.
 */
static int ad463x_adc_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct ad463x_data *data = dev->data;
	uint32_t channels = sequence->channels;
	uint32_t *out;
	size_t needed;
	uint32_t latest[2];
	const uint32_t *ring32 = (const uint32_t *)data->ring;
	uint16_t reps;
	int ret;
	int n_ch = 0;

	if (sequence->buffer == NULL) {
		return -EINVAL;
	}
	/* Only channels 0 and 1 exist; reject anything else early. */
	if (channels == 0 || (channels & ~0x3u) != 0) {
		return -EINVAL;
	}

	for (uint32_t m = channels; m; m &= (m - 1)) {
		n_ch++;
	}
	reps = 1U + (sequence->options ? sequence->options->extra_samplings : 0);
	needed = (size_t)n_ch * sizeof(uint32_t) * reps;
	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	ret = ad463x_continuous_start(dev);
	if (ret) {
		return ret;
	}

	out = sequence->buffer;
	for (uint16_t r = 0; r < reps; r++) {
		sys_cache_data_invd_range(data->ring, AD463X_CONT_RING_BYTES);
		latest[0] = ring32[0];
		latest[1] = ring32[1];

		if (channels & BIT(0)) {
			*out++ = latest[0];
		}
		if (channels & BIT(1)) {
			*out++ = latest[1];
		}
	}

	return 0;
}

static const struct ad463x_part_info *ad463x_lookup_part(const char *name)
{
	for (size_t i = 0; i < ARRAY_SIZE(ad463x_parts); i++) {
		if (strcmp(ad463x_parts[i].part, name) == 0) {
			return &ad463x_parts[i];
		}
	}
	return NULL;
}

static int ad463x_init(const struct device *dev)
{
	const struct ad463x_config *cfg = dev->config;
	struct ad463x_data *data = dev->data;
	int ret;

	k_mutex_init(&data->lock);
	k_sem_init(&data->dma_done, 0, 1);

	if (!spi_is_ready_dt(&cfg->bus)) {
		LOG_ERR("SPI Engine bus is not ready");
		return -ENODEV;
	}
	if (!device_is_ready(cfg->dmac) || !device_is_ready(cfg->clkgen) ||
	    !device_is_ready(cfg->pwmgen)) {
		LOG_ERR("a required AXI sibling device is not ready");
		return -ENODEV;
	}

	if (cfg->clock_mode == AD463X_SPI_COMPATIBLE_MODE && cfg->data_rate == AD463X_DDR_MODE) {
		LOG_ERR("DDR mode invalid with SPI-compatible clock mode");
		return -EINVAL;
	}

	data->info = ad463x_lookup_part(cfg->part);
	if (data->info == NULL) {
		LOG_ERR("unknown adi,part: %s", cfg->part);
		return -EINVAL;
	}

	ret = ad463x_compute_widths(dev);
	if (ret) {
		LOG_ERR("invalid lane/output combination (%d)", ret);
		return ret;
	}

	LOG_INF("AD463x %s — capture_width=%u bits, bytes/sample=%u, "
		"real_bits=%u",
		cfg->part, data->capture_data_width, data->bytes_per_sample,
		data->real_bits_precision);

	return 0;
}

#define AD463X_LANE_MODE(node_id)                                                                  \
	(DT_ENUM_IDX(node_id, adi_lane_mode) == 0   ? AD463X_ONE_LANE_PER_CH                       \
	 : DT_ENUM_IDX(node_id, adi_lane_mode) == 1 ? AD463X_TWO_LANES_PER_CH                      \
	 : DT_ENUM_IDX(node_id, adi_lane_mode) == 2 ? AD463X_FOUR_LANES_PER_CH                     \
						    : AD463X_SHARED_TWO_CH)

#define AD463X_CLOCK_MODE(node_id)                                                                 \
	(DT_ENUM_IDX(node_id, adi_clock_mode) == 0   ? AD463X_SPI_COMPATIBLE_MODE                  \
	 : DT_ENUM_IDX(node_id, adi_clock_mode) == 1 ? AD463X_ECHO_CLOCK_MODE                      \
						     : AD463X_CLOCK_MASTER_MODE)

#define AD463X_OUTPUT_MODE(node_id)                                                                \
	(DT_ENUM_IDX(node_id, adi_output_mode) == 0   ? AD463X_24_DIFF                             \
	 : DT_ENUM_IDX(node_id, adi_output_mode) == 1 ? AD463X_16_DIFF_8_COM                       \
	 : DT_ENUM_IDX(node_id, adi_output_mode) == 2 ? AD463X_24_DIFF_8_COM                       \
	 : DT_ENUM_IDX(node_id, adi_output_mode) == 3 ? AD463X_30_AVERAGED_DIFF                    \
						      : AD463X_32_PATTERN)

#define AD463X_DATA_RATE(node_id)                                                                  \
	(DT_ENUM_IDX(node_id, adi_data_rate) == 0 ? AD463X_SDR_MODE : AD463X_DDR_MODE)

/*
 * Per-instance ADC API. ref_internal is in millivolts and we feed it from
 * the binding's adi,vref-microvolts so adc_raw_to_millivolts() works.
 */
#define AD463X_API(n)                                                                              \
	static DEVICE_API(adc, ad463x_adc_api_##n) = {                                             \
		.channel_setup = ad463x_adc_channel_setup,                                         \
		.read = ad463x_adc_read,                                                           \
		.ref_internal = DT_INST_PROP(n, adi_vref_microvolts) / 1000,                       \
	};

#define AD463X_INIT(n)                                                                             \
	AD463X_API(n)                                                                              \
	static struct ad463x_data ad463x_data_##n;                                                 \
	static const struct ad463x_config ad463x_config_##n = {                                    \
		.bus = SPI_DT_SPEC_INST_GET(n, SPI_OP_MODE_MASTER | SPI_WORD_SET(8) |              \
						       SPI_TRANSFER_MSB),                          \
		.dmac = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, rx)),                           \
		.dma_channel = DT_INST_DMAS_CELL_BY_NAME(n, rx, channel),                          \
		.clkgen = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                   \
		.pwmgen = DEVICE_DT_GET(DT_INST_PWMS_CTLR(n)),                                     \
		.pwmgen_channel = DT_INST_PWMS_CHANNEL(n),                                         \
		.vref_uv = DT_INST_PROP(n, adi_vref_microvolts),                                   \
		.clkgen_rate_hz = DT_INST_PROP(n, adi_axi_clkgen_rate_hz),                         \
		.sample_rate_hz = DT_INST_PROP(n, adi_sample_rate_hz),                             \
		.lane_mode = AD463X_LANE_MODE(DT_DRV_INST(n)),                                     \
		.clock_mode = AD463X_CLOCK_MODE(DT_DRV_INST(n)),                                   \
		.output_mode = AD463X_OUTPUT_MODE(DT_DRV_INST(n)),                                 \
		.data_rate = AD463X_DATA_RATE(DT_DRV_INST(n)),                                     \
		.avg_frame_len = DT_INST_PROP(n, adi_avg_frame_len),                               \
		.skip_reg_config = DT_INST_PROP(n, adi_skip_reg_config),                           \
		.part = DT_INST_PROP(n, adi_part),                                                 \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),                       \
		.cnv_gpio = GPIO_DT_SPEC_INST_GET_OR(n, cnv_gpios, {0}),                           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, ad463x_init, NULL, &ad463x_data_##n, &ad463x_config_##n,          \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                     \
			      &ad463x_adc_api_##n);

DT_INST_FOREACH_STATUS_OKAY(AD463X_INIT)
