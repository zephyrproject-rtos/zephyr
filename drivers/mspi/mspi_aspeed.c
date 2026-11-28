/*
 * Copyright (c) 2026 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/cache.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#define SPI_CONFIG       0x000U
#define SPI_CE_CTRL      0x004U
#define SPI_INTR_STATUS  0x008U
#define SPI_CE0_CTRL     0x010U
#define SPI_CE0_DECODE   0x030U
#define SPI_ADDR_CONT    0x054U
#define SPI_DMA_RAM_HIGH 0x07CU
#define SPI_DMA_CTRL     0x080U
#define SPI_DMA_FLASH    0x084U
#define SPI_DMA_RAM      0x088U
#define SPI_DMA_LEN      0x08CU
#define SPI_SOCRST_LOCK  0x1F4U

#define SPI_CONFIG_CE_WRITE(ce) BIT(16U + (ce))
#define SPI_CE_ADDR4(ce)        (BIT(ce) | BIT(4U + (ce)))
#define SPI_DMA_DONE            BIT(11)
#define SPI_DMA_ENABLE          BIT(0)
#define SPI_DMA_MAX_LEN         BIT(25)
#define SPI_DMA_MIN_LEN         128U

#define SPI_CTRL_IO_MODE    GENMASK(31, 28)
#define SPI_CTRL_FREQ_HIGH  GENMASK(27, 24)
#define SPI_CTRL_COMMAND    GENMASK(23, 16)
#define SPI_CTRL_DUMMY_HIGH BIT(14)
#define SPI_CTRL_FREQ_LOW   GENMASK(11, 8)
#define SPI_CTRL_DUMMY_LOW  GENMASK(7, 6)
#define SPI_CTRL_CE_STOP    BIT(2)
#define SPI_CTRL_NORMAL     1U
#define SPI_CTRL_USER       3U

#define MSPI_ASPEED_MAX_CE 3U

/* Per-SoC layout of a controller type */
struct mspi_aspeed_soc {
	uint8_t num_ce;
	/* Decode window used for a chip select whose flash size is not given */
	uint32_t default_ce_size;
	uint32_t decode_align;
	uint32_t (*decode_value)(uint32_t start, uint32_t end);
};

struct mspi_aspeed_config {
	uintptr_t base;
	uintptr_t window;
	size_t window_size;
	const struct mspi_aspeed_soc *soc;
	const struct device *clock;
	clock_control_subsys_t hclk;
	struct reset_dt_spec reset;
	struct mspi_cfg mspi_cfg;
	uint32_t ce_size[MSPI_ASPEED_MAX_CE];
	uint8_t ce_mask;
};

struct mspi_aspeed_ce {
	struct mspi_dev_cfg cfg;
	uint32_t freq_ctrl;
	uint32_t normal_read;
	uint32_t frequency;
	uint32_t decode_start;
	uint32_t decode_size;
	/* SPI004 address mode between transfers, only goes from 3-byte to 4-byte */
	bool addr4;
};

struct mspi_aspeed_data {
	struct k_mutex lock;
	const struct mspi_dev_id *dev_id;
	uint8_t ce;
	struct mspi_aspeed_ce ces[MSPI_ASPEED_MAX_CE];
};

/* AST2700 decode registers: start and end + 1 in 64 KiB units, window relative. */
static uint32_t __maybe_unused ast2700_decode_value(uint32_t start, uint32_t end)
{
	return ((start >> 16) & 0x7fffU) | ((end + 1U) & 0x7fff0000U);
}

static const struct mspi_aspeed_soc __maybe_unused mspi_aspeed_ast2700_fmc = {
	.num_ce = 3U,
	.default_ce_size = 0x4000000U,
	.decode_align = 0x10000U,
	.decode_value = ast2700_decode_value,
};

static const struct mspi_aspeed_soc __maybe_unused mspi_aspeed_ast2700_spi = {
	.num_ce = 2U,
	.default_ce_size = 0x2000000U,
	.decode_align = 0x10000U,
	.decode_value = ast2700_decode_value,
};

static uint32_t io_mode_ctrl(enum mspi_io_mode mode)
{
	switch (mode) {
	case MSPI_IO_MODE_DUAL_1_1_2:
		return FIELD_PREP(SPI_CTRL_IO_MODE, 2U);
	case MSPI_IO_MODE_DUAL_1_2_2:
		return FIELD_PREP(SPI_CTRL_IO_MODE, 3U);
	case MSPI_IO_MODE_QUAD_1_1_4:
		return FIELD_PREP(SPI_CTRL_IO_MODE, 4U);
	case MSPI_IO_MODE_QUAD_1_4_4:
		return FIELD_PREP(SPI_CTRL_IO_MODE, 5U);
	default:
		return 0U;
	}
}

static uint8_t io_addr_width(enum mspi_io_mode mode)
{
	switch (mode) {
	case MSPI_IO_MODE_DUAL_1_2_2:
		return 2U;
	case MSPI_IO_MODE_QUAD_1_4_4:
		return 4U;
	default:
		return 1U;
	}
}

static uint8_t io_data_width(enum mspi_io_mode mode)
{
	switch (mode) {
	case MSPI_IO_MODE_DUAL_1_1_2:
	case MSPI_IO_MODE_DUAL_1_2_2:
		return 2U;
	case MSPI_IO_MODE_QUAD_1_1_4:
	case MSPI_IO_MODE_QUAD_1_4_4:
		return 4U;
	default:
		return 1U;
	}
}

/* Bus width of the following user-mode accesses */
static uint32_t user_width_ctrl(uint8_t width)
{
	switch (width) {
	case 4U:
		return FIELD_PREP(SPI_CTRL_IO_MODE, 4U);
	case 2U:
		return FIELD_PREP(SPI_CTRL_IO_MODE, 2U);
	default:
		return 0U;
	}
}

/* Dummy cycles are sent as bytes at the address bus width. */
static uint16_t dummy_bytes(enum mspi_io_mode mode, uint16_t cycles)
{
	return cycles * io_addr_width(mode) / 8U;
}

static uint32_t normal_read_ctrl(enum mspi_io_mode mode, uint32_t command, uint16_t dummy)
{
	return io_mode_ctrl(mode) | FIELD_PREP(SPI_CTRL_COMMAND, command) |
	       FIELD_PREP(SPI_CTRL_DUMMY_LOW, dummy & 3U) |
	       FIELD_PREP(SPI_CTRL_DUMMY_HIGH, dummy >> 2) | SPI_CTRL_NORMAL;
}

static void address_mode(const struct mspi_aspeed_config *config, uint8_t ce, bool addr4)
{
	uint32_t val = sys_read32(config->base + SPI_CE_CTRL);

	val = (val & ~SPI_CE_ADDR4(ce)) | (addr4 ? SPI_CE_ADDR4(ce) : 0U);
	sys_write32(val, config->base + SPI_CE_CTRL);
}

/* Keep the chip select in 4-byte mode from now on, also over a SoC reset. */
static void keep_addr4(const struct mspi_aspeed_config *config, struct mspi_aspeed_ce *c,
		       uint8_t ce)
{
	c->addr4 = true;
	sys_set_bits(config->base + SPI_SOCRST_LOCK, SPI_CE_ADDR4(ce) << 4);
}

static int clock_divider(const struct device *dev, uint32_t requested, uint32_t *control,
			 uint32_t *frequency)
{
	const struct mspi_aspeed_config *config = dev->config;
	static const uint8_t low_encoding[] = {15U, 7U, 14U, 6U, 13U, 5U, 12U, 4U,
					       11U, 3U, 10U, 2U, 9U, 1U, 8U, 0U};
	uint32_t hclk;
	uint32_t divisor;
	int ret;

	if (requested == 0U) {
		return -EINVAL;
	}

	ret = clock_control_get_rate(config->clock, config->hclk, &hclk);
	if (ret != 0) {
		return ret;
	}
	if (hclk == 0U) {
		return -EIO;
	}

	divisor = MAX(2U, DIV_ROUND_UP(hclk, (uint64_t)requested));
	if (divisor > 256U) {
		return -ENOTSUP;
	}

	*control = FIELD_PREP(SPI_CTRL_FREQ_HIGH, (divisor - 1U) / 16U) |
		   FIELD_PREP(SPI_CTRL_FREQ_LOW, low_encoding[(divisor - 1U) % 16U]);
	*frequency = hclk / divisor;

	return 0;
}

static int mspi_aspeed_configure(const struct mspi_dt_spec *spec)
{
	const struct mspi_aspeed_config *config = spec->bus->config;
	const struct mspi_cfg *cfg = &spec->config;

	if (cfg->op_mode != MSPI_OP_MODE_CONTROLLER || cfg->duplex != MSPI_HALF_DUPLEX ||
	    cfg->channel_num != 0U || cfg->num_periph > config->soc->num_ce ||
	    cfg->dqs_support || cfg->num_ce_gpios != 0U || cfg->re_init) {
		return -ENOTSUP;
	}

	return 0;
}

static bool io_mode_supported(enum mspi_io_mode mode)
{
	return mode == MSPI_IO_MODE_SINGLE || mode == MSPI_IO_MODE_DUAL_1_1_2 ||
	       mode == MSPI_IO_MODE_DUAL_1_2_2 || mode == MSPI_IO_MODE_QUAD_1_1_4 ||
	       mode == MSPI_IO_MODE_QUAD_1_4_4;
}

static bool dummy_supported(enum mspi_io_mode mode, uint16_t cycles)
{
	return (cycles * io_addr_width(mode)) % 8U == 0U && dummy_bytes(mode, cycles) <= 7U;
}

static int validate_dev_config(const struct mspi_dev_cfg *cfg)
{
	if (!io_mode_supported(cfg->io_mode) || cfg->data_rate != MSPI_DATA_RATE_SINGLE ||
	    cfg->cpp != MSPI_CPP_MODE_0 || cfg->endian != MSPI_XFER_BIG_ENDIAN ||
	    cfg->ce_polarity != MSPI_CE_ACTIVE_LOW || cfg->dqs_enable ||
	    cfg->mem_boundary != 0U || cfg->time_to_break != 0U || cfg->cmd_length > 1U ||
	    (cfg->addr_length != 0U && cfg->addr_length != 3U && cfg->addr_length != 4U) ||
	    cfg->read_cmd > UINT8_MAX || cfg->write_cmd > UINT8_MAX ||
	    !dummy_supported(cfg->io_mode, cfg->rx_dummy) ||
	    !dummy_supported(cfg->io_mode, cfg->tx_dummy)) {
		return -ENOTSUP;
	}

	return 0;
}

static void merge_dev_config(struct mspi_dev_cfg *next, const struct mspi_dev_cfg *cfg,
			     enum mspi_dev_cfg_mask mask)
{
	if ((mask & MSPI_DEVICE_CONFIG_CE_NUM) != 0U) {
		next->ce_num = cfg->ce_num;
	}

	if ((mask & MSPI_DEVICE_CONFIG_FREQUENCY) != 0U) {
		next->freq = cfg->freq;
	}

	if ((mask & MSPI_DEVICE_CONFIG_IO_MODE) != 0U) {
		next->io_mode = cfg->io_mode;
	}

	if ((mask & MSPI_DEVICE_CONFIG_DATA_RATE) != 0U) {
		next->data_rate = cfg->data_rate;
	}

	if ((mask & MSPI_DEVICE_CONFIG_CPP) != 0U) {
		next->cpp = cfg->cpp;
	}

	if ((mask & MSPI_DEVICE_CONFIG_ENDIAN) != 0U) {
		next->endian = cfg->endian;
	}

	if ((mask & MSPI_DEVICE_CONFIG_CE_POL) != 0U) {
		next->ce_polarity = cfg->ce_polarity;
	}

	if ((mask & MSPI_DEVICE_CONFIG_DQS) != 0U) {
		next->dqs_enable = cfg->dqs_enable;
	}

	if ((mask & MSPI_DEVICE_CONFIG_RX_DUMMY) != 0U) {
		next->rx_dummy = cfg->rx_dummy;
	}

	if ((mask & MSPI_DEVICE_CONFIG_TX_DUMMY) != 0U) {
		next->tx_dummy = cfg->tx_dummy;
	}

	if ((mask & MSPI_DEVICE_CONFIG_READ_CMD) != 0U) {
		next->read_cmd = cfg->read_cmd;
	}

	if ((mask & MSPI_DEVICE_CONFIG_WRITE_CMD) != 0U) {
		next->write_cmd = cfg->write_cmd;
	}

	if ((mask & MSPI_DEVICE_CONFIG_CMD_LEN) != 0U) {
		next->cmd_length = cfg->cmd_length;
	}

	if ((mask & MSPI_DEVICE_CONFIG_ADDR_LEN) != 0U) {
		next->addr_length = cfg->addr_length;
	}

	if ((mask & MSPI_DEVICE_CONFIG_MEM_BOUND) != 0U) {
		next->mem_boundary = cfg->mem_boundary;
	}

	if ((mask & MSPI_DEVICE_CONFIG_BREAK_TIME) != 0U) {
		next->time_to_break = cfg->time_to_break;
	}
}

/* Program the normal-read mode of a chip select from its stored configuration. */
static void apply_ce(const struct mspi_aspeed_config *config, struct mspi_aspeed_data *data,
		     uint8_t ce)
{
	struct mspi_aspeed_ce *c = &data->ces[ce];

	address_mode(config, ce, c->addr4);
	sys_write32(c->normal_read, config->base + SPI_CE0_CTRL + ce * 4U);
}

static int mspi_aspeed_dev_config(const struct device *dev, const struct mspi_dev_id *dev_id,
				  enum mspi_dev_cfg_mask mask, const struct mspi_dev_cfg *cfg)
{
	const struct mspi_aspeed_config *config = dev->config;
	struct mspi_aspeed_data *data = dev->data;
	struct mspi_dev_cfg next;
	struct mspi_aspeed_ce *c;
	uint32_t freq_ctrl;
	uint32_t frequency;
	uint32_t command;
	bool acquired;
	uint8_t ce;
	int ret;

	if (dev_id == NULL || (mask != MSPI_DEVICE_CONFIG_NONE && cfg == NULL)) {
		return -EINVAL;
	}
	ce = dev_id->dev_idx;
	if (ce >= config->soc->num_ce || (config->ce_mask & BIT(ce)) == 0U ||
	    dev_id->ce.port != NULL || ((uint32_t)mask & ~MSPI_DEVICE_CONFIG_ALL) != 0U) {
		return -ENOTSUP;
	}

	ret = k_mutex_lock(&data->lock, K_MSEC(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE));
	if (ret != 0) {
		return -EBUSY;
	}

	acquired = data->dev_id == NULL;
	if (!acquired && data->dev_id != dev_id) {
		ret = -ESTALE;
		goto out;
	}

	c = &data->ces[ce];
	next = c->cfg;
	merge_dev_config(&next, cfg, mask);

	ret = validate_dev_config(&next);
	if (ret != 0) {
		goto out;
	}

	ret = clock_divider(dev, next.freq, &freq_ctrl, &frequency);
	if (ret != 0) {
		goto out;
	}

	command = next.read_cmd;
	if (command == 0U) {
		command = next.addr_length == 4U ? 0x13U : 0x03U;
	}

	if (next.addr_length == 4U) {
		keep_addr4(config, c, ce);
	}
	c->cfg = next;
	c->freq_ctrl = freq_ctrl;
	c->frequency = frequency;
	c->normal_read = freq_ctrl |
			 normal_read_ctrl(next.read_cmd == 0U ? MSPI_IO_MODE_SINGLE : next.io_mode,
					  command,
					  next.read_cmd == 0U ? 0U
							      : dummy_bytes(next.io_mode,
									    next.rx_dummy));
	apply_ce(config, data, ce);
	data->dev_id = dev_id;
	data->ce = ce;

out:
	/* Keep the first acquisition until get_channel_status releases the bus. */
	if (ret != 0 || !acquired) {
		(void)k_mutex_unlock(&data->lock);
	}

	return ret;
}

static int validate_xfer(const struct mspi_xfer *xfer)
{
	if (xfer->num_packet == 0U || xfer->packets == NULL) {
		return -EINVAL;
	}
	if (xfer->async || xfer->hold_ce || xfer->ce_sw_ctrl.gpio.port != NULL ||
	    xfer->ce_sw_ctrl.delay != 0U || xfer->cmd_length > 1U ||
	    (xfer->addr_length != 0U && xfer->addr_length != 3U && xfer->addr_length != 4U) ||
	    (xfer->xfer_mode != MSPI_PIO && xfer->xfer_mode != MSPI_DMA)) {
		return -ENOTSUP;
	}

	for (uint32_t i = 0U; i < xfer->num_packet; i++) {
		const struct mspi_xfer_packet *packet = &xfer->packets[i];

		if (packet->dir != MSPI_RX && packet->dir != MSPI_TX) {
			return -EINVAL;
		}
		if (packet->num_bytes != 0U && packet->data_buf == NULL) {
			return -EINVAL;
		}
		if (packet->cb_mask != MSPI_BUS_NO_CB ||
		    (xfer->cmd_length == 1U && packet->cmd > UINT8_MAX)) {
			return -ENOTSUP;
		}
		if (xfer->addr_length == 3U && packet->address > 0xFFFFFFU) {
			return -EINVAL;
		}
	}

	return 0;
}

static int64_t transfer_deadline(const struct mspi_aspeed_ce *c, const struct mspi_xfer *xfer)
{
	uint64_t bytes = 0U;
	uint64_t duration;

	for (uint32_t i = 0U; i < xfer->num_packet; i++) {
		const struct mspi_xfer_packet *packet = &xfer->packets[i];

		bytes += (uint64_t)packet->num_bytes + xfer->cmd_length + xfer->addr_length;
		bytes += (packet->dir == MSPI_RX ? xfer->rx_dummy : xfer->tx_dummy) / 8U;
	}

	duration = DIV_ROUND_UP(bytes * 8U * MSEC_PER_SEC, c->frequency);
	duration += CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE;

	return k_uptime_get() + MAX(duration, xfer->timeout);
}

static int transfer_pio(const struct device *dev, const struct mspi_xfer *xfer,
			const struct mspi_xfer_packet *packet, int64_t deadline)
{
	const struct mspi_aspeed_config *config = dev->config;
	struct mspi_aspeed_data *data = dev->data;
	const struct mspi_aspeed_ce *c = &data->ces[data->ce];
	enum mspi_io_mode mode = c->cfg.io_mode;
	uintptr_t ctrl_reg = config->base + SPI_CE0_CTRL + data->ce * 4U;
	uintptr_t window = config->window + c->decode_start;
	uint32_t control = c->freq_ctrl | SPI_CTRL_USER;
	uint16_t dummy = dummy_bytes(mode, packet->dir == MSPI_RX ? xfer->rx_dummy
								   : xfer->tx_dummy);
	uint32_t offset = 0U;
	int ret = 0;

	sys_write32(control | SPI_CTRL_CE_STOP, ctrl_reg);
	sys_write32(control, ctrl_reg);
	(void)sys_read32(ctrl_reg);

	if (xfer->cmd_length != 0U) {
		sys_write8(packet->cmd, window);
	}

	sys_write32(control | user_width_ctrl(io_addr_width(mode)), ctrl_reg);
	for (uint8_t i = xfer->addr_length; i > 0U; i--) {
		sys_write8(packet->address >> ((i - 1U) * 8U), window);
	}
	for (uint16_t i = 0U; i < dummy; i++) {
		sys_write8(0U, window);
	}

	sys_write32(control | user_width_ctrl(io_data_width(mode)), ctrl_reg);
	while (packet->num_bytes - offset >= sizeof(uint32_t)) {
		if ((offset % 256U) == 0U && k_uptime_get() >= deadline) {
			ret = -ETIMEDOUT;
			goto out;
		}
		if (packet->dir == MSPI_RX) {
			sys_put_le32(sys_read32(window), packet->data_buf + offset);
		} else {
			sys_write32(sys_get_le32(packet->data_buf + offset), window);
		}
		offset += sizeof(uint32_t);
	}
	while (offset < packet->num_bytes) {
		if (packet->dir == MSPI_RX) {
			packet->data_buf[offset] = sys_read8(window);
		} else {
			sys_write8(packet->data_buf[offset], window);
		}
		offset++;
	}

out:
	sys_write32(control | SPI_CTRL_CE_STOP, ctrl_reg);

	return ret;
}

static bool use_dma(const struct mspi_aspeed_ce *c, const struct mspi_xfer *xfer,
		    const struct mspi_xfer_packet *packet)
{
	return xfer->xfer_mode == MSPI_DMA && packet->dir == MSPI_RX &&
	       packet->num_bytes > SPI_DMA_MIN_LEN && packet->num_bytes <= SPI_DMA_MAX_LEN &&
	       IS_ALIGNED(packet->address, 4U) && IS_ALIGNED(packet->data_buf, 4U) &&
	       xfer->cmd_length == 1U && (xfer->addr_length == 3U || xfer->addr_length == 4U) &&
	       packet->address < c->decode_size &&
	       packet->num_bytes <= c->decode_size - packet->address;
}

static int transfer_dma(const struct device *dev, const struct mspi_xfer *xfer,
			const struct mspi_xfer_packet *packet, int64_t deadline)
{
	const struct mspi_aspeed_config *config = dev->config;
	struct mspi_aspeed_data *data = dev->data;
	const struct mspi_aspeed_ce *c = &data->ces[data->ce];
	uint64_t address = ast27xx_soc_virt_addr_to_phy_addr((uintptr_t)packet->data_buf);
	uint32_t control = c->freq_ctrl |
			   normal_read_ctrl(c->cfg.io_mode, packet->cmd,
					    dummy_bytes(c->cfg.io_mode, xfer->rx_dummy));
	int ret = 0;

	address_mode(config, data->ce, xfer->addr_length == 4U);
	sys_write32(control, config->base + SPI_CE0_CTRL + data->ce * 4U);
	sys_write32(0U, config->base + SPI_DMA_CTRL);
	sys_write32(address >> 32, config->base + SPI_DMA_RAM_HIGH);
	/* The DMA flash address is the offset in the combined decode space. */
	sys_write32(c->decode_start + packet->address, config->base + SPI_DMA_FLASH);
	sys_write32((uint32_t)address, config->base + SPI_DMA_RAM);
	sys_write32(packet->num_bytes - 1U, config->base + SPI_DMA_LEN);
	sys_write32(SPI_DMA_ENABLE, config->base + SPI_DMA_CTRL);

	while ((sys_read32(config->base + SPI_INTR_STATUS) & SPI_DMA_DONE) == 0U) {
		if (k_uptime_get() >= deadline) {
			ret = -ETIMEDOUT;
			break;
		}
		k_busy_wait(1U);
	}

	sys_write32(0U, config->base + SPI_DMA_CTRL);
	sys_cache_data_invd_range(packet->data_buf, packet->num_bytes);

	return ret;
}

static int mspi_aspeed_transceive(const struct device *dev, const struct mspi_dev_id *dev_id,
				  const struct mspi_xfer *xfer)
{
	const struct mspi_aspeed_config *config = dev->config;
	struct mspi_aspeed_data *data = dev->data;
	uint32_t addr_cont;
	int64_t deadline;
	int ret;

	ret = validate_xfer(xfer);
	if (ret != 0) {
		return ret;
	}
	ret = k_mutex_lock(&data->lock, K_MSEC(xfer->timeout));
	if (ret != 0) {
		return -EBUSY;
	}
	if (dev_id == NULL || dev_id != data->dev_id) {
		ret = -ESTALE;
		goto out;
	}
	if (!dummy_supported(data->ces[data->ce].cfg.io_mode, xfer->rx_dummy) ||
	    !dummy_supported(data->ces[data->ce].cfg.io_mode, xfer->tx_dummy)) {
		ret = -ENOTSUP;
		goto out;
	}

	deadline = transfer_deadline(&data->ces[data->ce], xfer);
	/*
	 * SPI054 holds the CS address-continuity modes and the 4 KB address
	 * boundary control; transfers run with all of them cleared.
	 */
	addr_cont = sys_read32(config->base + SPI_ADDR_CONT);
	sys_write32(0U, config->base + SPI_ADDR_CONT);
	if (xfer->addr_length == 4U) {
		keep_addr4(config, &data->ces[data->ce], data->ce);
	}

	for (uint32_t i = 0U; i < xfer->num_packet; i++) {
		const struct mspi_xfer_packet *packet = &xfer->packets[i];

		if (k_uptime_get() >= deadline) {
			ret = -ETIMEDOUT;
			break;
		}
		if (IS_ENABLED(CONFIG_MSPI_ASPEED_DMA) &&
		    use_dma(&data->ces[data->ce], xfer, packet)) {
			ret = transfer_dma(dev, xfer, packet, deadline);
		} else {
			ret = transfer_pio(dev, xfer, packet, deadline);
		}
		if (ret != 0) {
			break;
		}
	}

	apply_ce(config, data, data->ce);
	sys_write32(addr_cont, config->base + SPI_ADDR_CONT);

out:
	(void)k_mutex_unlock(&data->lock);

	return ret;
}

static int mspi_aspeed_get_channel_status(const struct device *dev, uint8_t channel)
{
	struct mspi_aspeed_data *data = dev->data;
	bool acquired;
	int ret;

	if (channel != 0U) {
		return -ENOTSUP;
	}
	ret = k_mutex_lock(&data->lock, K_NO_WAIT);
	if (ret != 0) {
		return -EBUSY;
	}

	acquired = data->dev_id != NULL;
	data->dev_id = NULL;
	(void)k_mutex_unlock(&data->lock);
	if (acquired) {
		ret = k_mutex_unlock(&data->lock);
	}

	return ret;
}

/*
 * Give each chip select with a flash a decode window of the flash size, one
 * after another from the start of the AHB window.
 */
static int decode_init(const struct device *dev)
{
	const struct mspi_aspeed_config *config = dev->config;
	struct mspi_aspeed_data *data = dev->data;
	uint32_t start = 0U;

	for (uint8_t ce = 0U; ce < config->soc->num_ce; ce++) {
		uint32_t size;

		if ((config->ce_mask & BIT(ce)) == 0U) {
			sys_write32(0U, config->base + SPI_CE0_DECODE + ce * 4U);
			continue;
		}

		size = config->ce_size[ce] != 0U ? config->ce_size[ce]
						 : config->soc->default_ce_size;
		size = ROUND_UP(size, config->soc->decode_align);
		if (size > config->window_size - start) {
			return -EINVAL;
		}

		sys_write32(config->soc->decode_value(start, start + size - 1U),
			    config->base + SPI_CE0_DECODE + ce * 4U);
		sys_set_bits(config->base + SPI_CONFIG, SPI_CONFIG_CE_WRITE(ce));
		data->ces[ce].decode_start = start;
		data->ces[ce].decode_size = size;
		start += size;
	}

	return 0;
}

static int mspi_aspeed_init(const struct device *dev)
{
	const struct mspi_aspeed_config *config = dev->config;
	struct mspi_aspeed_data *data = dev->data;
	const struct mspi_dt_spec spec = {.bus = dev, .config = config->mspi_cfg};
	int ret;

	ret = mspi_aspeed_configure(&spec);
	if (ret != 0) {
		return ret;
	}
	if ((config->ce_mask & ~BIT_MASK(config->soc->num_ce)) != 0U) {
		return -EINVAL;
	}
	if (!device_is_ready(config->clock)) {
		return -ENODEV;
	}
	if (config->reset.dev != NULL) {
		if (!device_is_ready(config->reset.dev)) {
			return -ENODEV;
		}
		ret = reset_line_deassert_dt(&config->reset);
		if (ret != 0) {
			return ret;
		}
	}
	ret = k_mutex_init(&data->lock);
	if (ret != 0) {
		return ret;
	}

	for (uint8_t ce = 0U; ce < config->soc->num_ce; ce++) {
		data->ces[ce].cfg.endian = MSPI_XFER_BIG_ENDIAN;
		data->ces[ce].cfg.cmd_length = 1U;
		data->ces[ce].cfg.addr_length = 3U;
		/* Lock a 4-byte mode kept over a warm reset again. */
		if ((sys_read32(config->base + SPI_CE_CTRL) & SPI_CE_ADDR4(ce)) ==
		    SPI_CE_ADDR4(ce)) {
			keep_addr4(config, &data->ces[ce], ce);
		}
	}

	return decode_init(dev);
}

static DEVICE_API(mspi, mspi_aspeed_api) = {
	.config = mspi_aspeed_configure,
	.dev_config = mspi_aspeed_dev_config,
	.get_channel_status = mspi_aspeed_get_channel_status,
	.transceive = mspi_aspeed_transceive,
};

#define MSPI_ASPEED_CE_SIZE(child) [DT_REG_ADDR(child)] = DT_PROP_OR(child, size, 0) / 8U,
#define MSPI_ASPEED_CE_BIT(child)  | BIT(DT_REG_ADDR(child))

#define MSPI_ASPEED_DEFINE(node, soc_data)                                                         \
	BUILD_ASSERT(DT_NUM_REGS(node) == 2U, "controller and AHB window regions are required");  \
	static struct mspi_aspeed_data mspi_aspeed_data_##node;                                    \
	static const struct mspi_aspeed_config mspi_aspeed_config_##node = {                       \
		.base = DT_REG_ADDR_BY_IDX(node, 0),                                               \
		.window = DT_REG_ADDR_BY_IDX(node, 1),                                             \
		.window_size = DT_REG_SIZE_BY_IDX(node, 1),                                        \
		.soc = &(soc_data),                                                                \
		.clock = DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_NAME(node, hclk)),                        \
		.hclk = (clock_control_subsys_t)DT_CLOCKS_CELL_BY_NAME(node, hclk, clkid),         \
		.reset = RESET_DT_SPEC_GET_OR(node, {0}),                                          \
		.mspi_cfg = {                                                                      \
			.op_mode = DT_ENUM_IDX_OR(node, op_mode, MSPI_OP_MODE_CONTROLLER),         \
			.duplex = DT_ENUM_IDX_OR(node, duplex, MSPI_HALF_DUPLEX),                  \
			.dqs_support = DT_PROP(node, dqs_support),                                 \
			.sw_multi_periph = DT_PROP(node, software_multiperipheral),                \
			.num_periph = DT_CHILD_NUM_STATUS_OKAY(node),                              \
			.num_ce_gpios = DT_PROP_LEN_OR(node, ce_gpios, 0),                         \
		},                                                                                 \
		.ce_size = {DT_FOREACH_CHILD_STATUS_OKAY(node, MSPI_ASPEED_CE_SIZE)},              \
		.ce_mask = 0 DT_FOREACH_CHILD_STATUS_OKAY(node, MSPI_ASPEED_CE_BIT),               \
	};                                                                                         \
	DEVICE_DT_DEFINE(node, mspi_aspeed_init, NULL, &mspi_aspeed_data_##node,                   \
			 &mspi_aspeed_config_##node, POST_KERNEL, CONFIG_MSPI_INIT_PRIORITY,       \
			 &mspi_aspeed_api);

DT_FOREACH_STATUS_OKAY_VARGS(aspeed_ast2700_fmc, MSPI_ASPEED_DEFINE, mspi_aspeed_ast2700_fmc)
DT_FOREACH_STATUS_OKAY_VARGS(aspeed_ast2700_spi, MSPI_ASPEED_DEFINE, mspi_aspeed_ast2700_spi)
