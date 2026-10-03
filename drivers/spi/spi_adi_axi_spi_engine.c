/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Driver for the Analog Devices AXI SPI Engine core.
 *
 * Implements the standard Zephyr SPI controller API for register-mode
 * (command-FIFO) transfers. The autonomous offload path used for MS/s
 * conversion streaming is exposed as a vendor extension in
 * <zephyr/drivers/spi/spi_adi_axi_spi_engine.h>.
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/spi_adi_axi_spi_engine.h>

#define DT_DRV_COMPAT adi_axi_spi_engine

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adi_axi_spi_engine);

#include "spi_context.h"
#include "spi_rtio.h"

#define SPI_ENGINE_REG_VERSION			0x00
#define SPI_ENGINE_REG_DATA_WIDTH		0x0C
#define SPI_ENGINE_REG_RESET			0x40
#define SPI_ENGINE_REG_INT_ENABLE		0x80
#define SPI_ENGINE_REG_INT_PENDING		0x84
#define SPI_ENGINE_REG_INT_SOURCE		0x88
#define SPI_ENGINE_REG_SYNC_ID			0xC0
#define SPI_ENGINE_REG_CMD_FIFO_ROOM		0xD0
#define SPI_ENGINE_REG_SDO_FIFO_ROOM		0xD4
#define SPI_ENGINE_REG_SDI_FIFO_LEVEL		0xD8
#define SPI_ENGINE_REG_CMD_FIFO			0xE0
#define SPI_ENGINE_REG_SDO_DATA_FIFO		0xE4
#define SPI_ENGINE_REG_SDI_DATA_FIFO		0xE8

#define SPI_ENGINE_INST_TRANSFER		0x00
#define SPI_ENGINE_INST_ASSERT			0x01
#define SPI_ENGINE_INST_CONFIG			0x02
#define SPI_ENGINE_INST_MISC			0x03

#define SPI_ENGINE_CMD_REG_CLK_DIV		0x00
#define SPI_ENGINE_CMD_REG_CONFIG		0x01
/* CONFIG sub-register 0x02: bits-per-word for TRANSFER instruction. */
#define SPI_ENGINE_CMD_REG_XFER_BITS		0x02
/* CONFIG sub-registers 0x03/0x04: SDI/SDO lane masks for multi-lane builds. */
#define SPI_ENGINE_CMD_REG_SDI_MASK		0x03
#define SPI_ENGINE_CMD_REG_SDO_MASK		0x04

#define SPI_ENGINE_MISC_SYNC			0x00

/* CONFIG sub-register 0x01. CPHA/CPOL are ordered the opposite way round to
 * Zephyr's SPI_MODE_CPOL/SPI_MODE_CPHA, so mode must be remapped bit by bit
 * rather than passed through as a field.
 */
#define SPI_ENGINE_CONFIG_CPHA			BIT(0)
#define SPI_ENGINE_CONFIG_CPOL			BIT(1)
#define SPI_ENGINE_CONFIG_3WIRE			BIT(2)
#define SPI_ENGINE_CONFIG_SDO_IDLE		BIT(3)

/* TRANSFER encodes its word count in arg2 (8 bits), zero-based. */
#define SPI_ENGINE_MAX_XFER_WORDS		256

#define SPI_ENGINE_INSTRUCTION_TRANSFER_W	0x01
#define SPI_ENGINE_INSTRUCTION_TRANSFER_R	0x02
#define SPI_ENGINE_INSTRUCTION_TRANSFER_RW	0x03

#define SPI_ENGINE_REG_OFFLOAD_CTRL(x)		(0x100 + (0x20 * (x)))
#define SPI_ENGINE_REG_OFFLOAD_RESET(x)		(0x108 + (0x20 * (x)))
#define SPI_ENGINE_REG_OFFLOAD_CMD_MEM(x)	(0x110 + (0x20 * (x)))
#define SPI_ENGINE_REG_OFFLOAD_SDO_MEM(x)	(0x114 + (0x20 * (x)))

/* HDL decodes inst from cmd[14:12], CONFIG sub-reg from cmd[10:8].
 * arg1 MUST be 3-bit-masked to avoid SDO_LANE_CONFIG aliasing to CLK_DIV.
 */
#define SPI_ENGINE_CMD(inst, arg1, arg2) \
	((((inst) & 0x07) << 12) | (((arg1) & 0x07) << 8) | ((arg2) & 0xFF))

#define SPI_ENGINE_CMD_TRANSFER(rw, n) \
	SPI_ENGINE_CMD(SPI_ENGINE_INST_TRANSFER, (rw), (n))

#define SPI_ENGINE_CMD_ASSERT(delay, cs) \
	SPI_ENGINE_CMD(SPI_ENGINE_INST_ASSERT, (delay), (cs))

#define SPI_ENGINE_CMD_CONFIG(reg, val) \
	SPI_ENGINE_CMD(SPI_ENGINE_INST_CONFIG, (reg), (val))

#define SPI_ENGINE_CMD_SYNC(id) \
	SPI_ENGINE_CMD(SPI_ENGINE_INST_MISC, SPI_ENGINE_MISC_SYNC, (id))

/* CS assert pattern: bit == 0 asserts (active low); 0xFF deasserts all. */
#define SPI_ENGINE_CS_DEASSERT			SPI_ENGINE_CMD_ASSERT(0x03, 0xFF)
#define SPI_ENGINE_CS_ASSERT(pattern)		SPI_ENGINE_CMD_ASSERT(0x03, (pattern))

struct axi_spi_engine_config {
	DEVICE_MMIO_ROM;
	uint32_t ref_clock_hz;
	uint32_t data_width;	/* default word size when spi_config leaves it 0 */
	uint32_t num_cs;
};

struct axi_spi_engine_data {
	DEVICE_MMIO_RAM;
	struct spi_context ctx;
	uint32_t version;
	uint32_t max_data_width;
	uint32_t num_sdi;	/* number of SDI lanes synthesized in HDL */
	uint32_t sync_id;	/* SYNC command tag, incremented per transfer */
};

static inline uint32_t spi_eng_read(const struct device *dev, uint32_t reg)
{
	return sys_read32(DEVICE_MMIO_GET(dev) + reg);
}

static inline void spi_eng_write(const struct device *dev, uint32_t reg,
				 uint32_t val)
{
	sys_write32(val, DEVICE_MMIO_GET(dev) + reg);
}

static int spi_eng_write_cmd(const struct device *dev, uint32_t cmd)
{
	uint32_t timeout = 10000;

	while (spi_eng_read(dev, SPI_ENGINE_REG_CMD_FIFO_ROOM) == 0) {
		if (--timeout == 0) {
			LOG_ERR("cmd FIFO full timeout");
			return -ETIMEDOUT;
		}
		k_busy_wait(1);
	}

	spi_eng_write(dev, SPI_ENGINE_REG_CMD_FIFO, cmd);
	return 0;
}

/*
 * Run one command-FIFO transfer for the current spi_context buffers.
 *
 * The core loads a program then executes it: push the command sequence
 * (CLK_DIV, XFER_BITS, lane masks, CS assert, TRANSFER, CS deassert, SYNC),
 * stream all SDO words, poll SYNC_ID, then drain the SDI FIFO into the rx
 * buffer.
 */
static int spi_eng_do_transfer(const struct device *dev,
			       const struct spi_config *config)
{
	const struct axi_spi_engine_config *cfg = dev->config;
	struct axi_spi_engine_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint8_t word_size;
	uint32_t bytes_per_word;
	uint32_t tx_bytes;
	uint32_t rx_bytes;
	uint32_t words;
	uint32_t clk_div;
	uint32_t my_sync_id;
	uint32_t poll_timeout;
	uint8_t cs_pattern;
	uint32_t xfer_cmd;
	uint8_t cfg_reg;
	bool have_tx;
	bool have_rx;
	int ret;

	word_size = SPI_WORD_SIZE_GET(config->operation);
	if (word_size == 0) {
		word_size = cfg->data_width;
	}
	bytes_per_word = (word_size + 7) / 8;

	tx_bytes = spi_context_total_tx_len(ctx);
	rx_bytes = spi_context_total_rx_len(ctx);
	have_tx = (tx_bytes > 0);
	have_rx = (rx_bytes > 0);

	words = DIV_ROUND_UP(MAX(tx_bytes, rx_bytes), bytes_per_word);
	if (words == 0) {
		return 0;
	}
	if (words > SPI_ENGINE_MAX_XFER_WORDS) {
		LOG_ERR("transfer of %u words exceeds %u-word limit", words,
			SPI_ENGINE_MAX_XFER_WORDS);
		return -ENOTSUP;
	}

	/* SCLK = ref_clk / (2 * (clk_div + 1)), so clk_div is rounded up:
	 * spi_config.frequency is a maximum, and flooring would overshoot it
	 * (ref=160MHz, freq=30MHz would floor to clk_div=1 => 40MHz).
	 */
	if (config->frequency == 0 ||
	    config->frequency >= cfg->ref_clock_hz / 2) {
		clk_div = 0;
	} else {
		clk_div = DIV_ROUND_UP(cfg->ref_clock_hz,
				       2 * config->frequency) - 1;
		if (clk_div > 255) {
			/* Clamping here would run SCLK faster than requested. */
			LOG_ERR("frequency %u Hz too low for ref_clk %u Hz",
				config->frequency, cfg->ref_clock_hz);
			return -EINVAL;
		}
	}

	cs_pattern = (uint8_t)~BIT(config->slave);

	if (have_tx && have_rx) {
		xfer_cmd = SPI_ENGINE_CMD_TRANSFER(
			SPI_ENGINE_INSTRUCTION_TRANSFER_RW, words - 1);
	} else if (have_tx) {
		xfer_cmd = SPI_ENGINE_CMD_TRANSFER(
			SPI_ENGINE_INSTRUCTION_TRANSFER_W, words - 1);
	} else {
		xfer_cmd = SPI_ENGINE_CMD_TRANSFER(
			SPI_ENGINE_INSTRUCTION_TRANSFER_R, words - 1);
	}

	/* Disable offload module before command-FIFO transfers. */
	spi_eng_write(dev, SPI_ENGINE_REG_OFFLOAD_CTRL(0), 0);

	my_sync_id = data->sync_id;

	ret = spi_eng_write_cmd(dev,
		SPI_ENGINE_CMD_CONFIG(SPI_ENGINE_CMD_REG_CLK_DIV, clk_div));
	if (ret) {
		return ret;
	}
	ret = spi_eng_write_cmd(dev,
		SPI_ENGINE_CMD_CONFIG(SPI_ENGINE_CMD_REG_XFER_BITS, word_size));
	if (ret) {
		return ret;
	}
	/* Multi-lane builds require explicit lane masks; pin to primary lane. */
	if (data->num_sdi > 1) {
		ret = spi_eng_write_cmd(dev,
			SPI_ENGINE_CMD_CONFIG(SPI_ENGINE_CMD_REG_SDI_MASK, 0x01));
		if (ret) {
			return ret;
		}
		ret = spi_eng_write_cmd(dev,
			SPI_ENGINE_CMD_CONFIG(SPI_ENGINE_CMD_REG_SDO_MASK, 0x01));
		if (ret) {
			return ret;
		}
	}
	cfg_reg = 0;
	if (config->operation & SPI_MODE_CPOL) {
		cfg_reg |= SPI_ENGINE_CONFIG_CPOL;
	}
	if (config->operation & SPI_MODE_CPHA) {
		cfg_reg |= SPI_ENGINE_CONFIG_CPHA;
	}
	ret = spi_eng_write_cmd(dev,
		SPI_ENGINE_CMD_CONFIG(SPI_ENGINE_CMD_REG_CONFIG, cfg_reg));
	if (ret) {
		return ret;
	}
	ret = spi_eng_write_cmd(dev, SPI_ENGINE_CS_DEASSERT);
	if (ret) {
		return ret;
	}
	ret = spi_eng_write_cmd(dev, SPI_ENGINE_CS_ASSERT(cs_pattern));
	if (ret) {
		return ret;
	}
	ret = spi_eng_write_cmd(dev, xfer_cmd);
	if (ret) {
		return ret;
	}
	ret = spi_eng_write_cmd(dev, SPI_ENGINE_CS_DEASSERT);
	if (ret) {
		return ret;
	}
	ret = spi_eng_write_cmd(dev, SPI_ENGINE_CMD_SYNC(my_sync_id & 0xFF));
	if (ret) {
		return ret;
	}

	/* Push SDO words packed MSB-first, sourcing bytes from the tx bufs. */
	if (have_tx) {
		for (uint32_t i = 0; i < words; i++) {
			uint32_t word = 0;
			uint32_t timeout = 10000;

			for (uint32_t b = 0; b < bytes_per_word; b++) {
				uint8_t byte = 0;

				if (spi_context_tx_buf_on(ctx)) {
					byte = *ctx->tx_buf;
					spi_context_update_tx(ctx, 1, 1);
				}
				word |= (uint32_t)byte <<
					(word_size - (b + 1) * 8);
			}

			while (spi_eng_read(dev,
				SPI_ENGINE_REG_SDO_FIFO_ROOM) == 0) {
				if (--timeout == 0) {
					return -ETIMEDOUT;
				}
				k_busy_wait(1);
			}
			spi_eng_write(dev, SPI_ENGINE_REG_SDO_DATA_FIFO, word);
		}
	}

	/* Poll SYNC_ID until our id is seen. */
	poll_timeout = 100000;
	while (spi_eng_read(dev, SPI_ENGINE_REG_SYNC_ID) !=
	       (my_sync_id & 0xFF)) {
		if (--poll_timeout == 0) {
			LOG_ERR("sync_id 0x%02x not seen — engine stalled",
				my_sync_id & 0xFF);
			return -ETIMEDOUT;
		}
		k_busy_wait(1);
	}

	data->sync_id = (my_sync_id + 1) & 0xFF;
	if (data->sync_id == 0) {
		data->sync_id = 1;
	}

	/* Pop and unpack SDI words MSB-first into the rx bufs. */
	if (have_rx) {
		for (uint32_t i = 0; i < words; i++) {
			uint32_t timeout = 10000;
			uint32_t word;

			while (spi_eng_read(dev,
				SPI_ENGINE_REG_SDI_FIFO_LEVEL) == 0) {
				if (--timeout == 0) {
					return -ETIMEDOUT;
				}
				k_busy_wait(1);
			}
			word = spi_eng_read(dev, SPI_ENGINE_REG_SDI_DATA_FIFO);

			for (uint32_t b = 0; b < bytes_per_word; b++) {
				uint8_t byte = (word >>
					(word_size - (b + 1) * 8)) & 0xFF;

				if (spi_context_rx_buf_on(ctx)) {
					*ctx->rx_buf = byte;
					spi_context_update_rx(ctx, 1, 1);
				}
			}
		}
	}

	return 0;
}

static int spi_eng_configure(const struct device *dev,
			     const struct spi_config *config)
{
	const struct axi_spi_engine_config *cfg = dev->config;
	struct axi_spi_engine_data *data = dev->data;
	uint8_t word_size;

	if (spi_context_configured(&data->ctx, config)) {
		return 0;
	}

	/* Tested directly rather than via spi_context_is_slave(), which
	 * dereferences ctx->config — still NULL until it is set below.
	 */
	if (config->operation & SPI_OP_MODE_SLAVE) {
		LOG_ERR("slave mode not supported");
		return -ENOTSUP;
	}
	/* The core shifts MSB-first on a dedicated SDO/SDI pair and drives CS
	 * active-low from the ASSERT instruction; none of these are expressible.
	 */
	if (config->operation & (SPI_MODE_LOOP | SPI_TRANSFER_LSB |
				 SPI_HALF_DUPLEX | SPI_FRAME_FORMAT_TI |
				 SPI_CS_ACTIVE_HIGH)) {
		LOG_ERR("unsupported operation flags 0x%x",
			(unsigned int)config->operation);
		return -ENOTSUP;
	}
	if ((config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("only single-line SPI supported");
		return -ENOTSUP;
	}
	if (spi_cs_is_gpio(config)) {
		/* CS is asserted by the ASSERT instruction inside the command
		 * program, so a GPIO CS is not wired up.
		 */
		LOG_ERR("cs-gpios not supported, use the core's hardware CS");
		return -ENOTSUP;
	}

	word_size = SPI_WORD_SIZE_GET(config->operation);
	if (word_size == 0) {
		word_size = cfg->data_width;
	}
	/* The pack/unpack loops shift by (word_size - (b + 1) * 8), which
	 * underflows for sizes that are not a whole number of bytes.
	 */
	if (word_size % 8 != 0) {
		LOG_ERR("word size %u not a multiple of 8", word_size);
		return -ENOTSUP;
	}
	if (word_size > data->max_data_width) {
		LOG_ERR("word size %u exceeds hardware width %u", word_size,
			data->max_data_width);
		return -ENOTSUP;
	}
	if (config->slave >= cfg->num_cs) {
		LOG_ERR("cs %u out of range (num-cs=%u)",
			config->slave, cfg->num_cs);
		return -EINVAL;
	}

	data->ctx.config = config;
	return 0;
}

static int axi_spi_engine_transceive(const struct device *dev,
				     const struct spi_config *config,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs)
{
	struct axi_spi_engine_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;

	spi_context_lock(ctx, false, NULL, NULL, config);

	ret = spi_eng_configure(dev, config);
	if (ret) {
		goto out;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

	ret = spi_eng_do_transfer(dev, config);

out:
	spi_context_release(ctx, ret);
	return ret;
}

static int axi_spi_engine_release(const struct device *dev,
				  const struct spi_config *config)
{
	struct axi_spi_engine_data *data = dev->data;

	ARG_UNUSED(config);
	spi_context_unlock_unconditionally(&data->ctx);
	return 0;
}

/* Offload extension; see <zephyr/drivers/spi/spi_adi_axi_spi_engine.h>. */
int spi_engine_offload_load(const struct device *dev,
			    struct spi_engine_offload_msg *msg)
{
	/* Reset offload engine */
	spi_eng_write(dev, SPI_ENGINE_REG_OFFLOAD_RESET(0), 0x01);
	spi_eng_write(dev, SPI_ENGINE_REG_OFFLOAD_RESET(0), 0x00);

	/* Load commands into offload command memory */
	for (uint32_t i = 0; i < msg->num_commands; i++) {
		spi_eng_write(dev, SPI_ENGINE_REG_OFFLOAD_CMD_MEM(0),
			      msg->commands[i]);
	}

	/* Load TX data if present */
	if (msg->tx_data) {
		for (uint32_t i = 0; i < msg->tx_len; i++) {
			spi_eng_write(dev, SPI_ENGINE_REG_OFFLOAD_SDO_MEM(0),
				      msg->tx_data[i]);
		}
	}

	return 0;
}

int spi_engine_offload_enable(const struct device *dev, bool enable)
{
	spi_eng_write(dev, SPI_ENGINE_REG_OFFLOAD_CTRL(0), enable ? 0x0001 : 0);
	return 0;
}

static int axi_spi_engine_init(const struct device *dev)
{
	const struct axi_spi_engine_config *cfg = dev->config;
	struct axi_spi_engine_data *data = dev->data;
	uint32_t data_width_reg;
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	data->version = spi_eng_read(dev, SPI_ENGINE_REG_VERSION);
	data->sync_id = 1;

	/* Soft reset */
	spi_eng_write(dev, SPI_ENGINE_REG_RESET, 0x01);
	k_busy_wait(100);
	spi_eng_write(dev, SPI_ENGINE_REG_RESET, 0x00);

	/* bits[15:0] = data_width, bits[23:16] = num_of_sdi */
	data_width_reg = spi_eng_read(dev, SPI_ENGINE_REG_DATA_WIDTH);
	data->max_data_width = data_width_reg & 0xFFFF;
	if (data->max_data_width == 0) {
		data->max_data_width = cfg->data_width;
	}
	data->num_sdi = (data_width_reg >> 16) & 0xFF;
	if (data->num_sdi == 0) {
		data->num_sdi = 1;
	}

	/* Disable all interrupts initially */
	spi_eng_write(dev, SPI_ENGINE_REG_INT_ENABLE, 0);
	spi_eng_write(dev, SPI_ENGINE_REG_INT_PENDING, 0xFF);

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		return ret;
	}
	spi_context_unlock_unconditionally(&data->ctx);

	LOG_INF("AXI SPI Engine v%d.%d.%c — ref_clk=%u Hz, "
		"data_width=%u, hw_max_width=%u",
		data->version >> 16,
		(data->version >> 8) & 0xff,
		data->version & 0xff,
		cfg->ref_clock_hz,
		cfg->data_width,
		data->max_data_width);

	return 0;
}

static DEVICE_API(spi, axi_spi_engine_driver_api) = {
	.transceive = axi_spi_engine_transceive,
	.release = axi_spi_engine_release,
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
};

#define AXI_SPI_ENGINE_INIT(n)						\
	static struct axi_spi_engine_data axi_spi_engine_data_##n = {	\
		SPI_CONTEXT_INIT_LOCK(axi_spi_engine_data_##n, ctx),	\
		SPI_CONTEXT_INIT_SYNC(axi_spi_engine_data_##n, ctx),	\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)	\
	};								\
	static const struct axi_spi_engine_config			\
		axi_spi_engine_config_##n = {				\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),			\
		.ref_clock_hz = DT_INST_PROP(n, adi_ref_clock_hz),	\
		.data_width = DT_INST_PROP(n, adi_data_width),		\
		.num_cs = DT_INST_PROP(n, adi_num_cs),			\
	};								\
	SPI_DEVICE_DT_INST_DEFINE(n,					\
			      axi_spi_engine_init,			\
			      NULL,					\
			      &axi_spi_engine_data_##n,			\
			      &axi_spi_engine_config_##n,		\
			      POST_KERNEL,				\
			      CONFIG_SPI_INIT_PRIORITY,			\
			      &axi_spi_engine_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AXI_SPI_ENGINE_INIT)
