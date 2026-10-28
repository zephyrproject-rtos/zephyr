/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_scb_spi

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_ifx.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_infineon_scb, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"
#include "spi_rtio.h"

/* SCB registers used in SPI mode */
#define IFX_SCB_CTRL           0x000U
#define IFX_SCB_CTRL_OVS       GENMASK(3, 0)
#define IFX_SCB_CTRL_MEM_WIDTH GENMASK(15, 14)
#define IFX_SCB_CTRL_MODE      GENMASK(25, 24)
#define IFX_SCB_CTRL_ENABLED   BIT(31)
#define IFX_SCB_CTRL_MODE_SPI  1U

#define IFX_SCB_SPI_CTRL                  0x020U
#define IFX_SCB_SPI_CTRL_SSEL_CONTINUOUS  BIT(0)
#define IFX_SCB_SPI_CTRL_SELECT_PRECEDE   BIT(1)
#define IFX_SCB_SPI_CTRL_CPHA             BIT(2)
#define IFX_SCB_SPI_CTRL_CPOL             BIT(3)
#define IFX_SCB_SPI_CTRL_LATE_MISO_SAMPLE BIT(4)
#define IFX_SCB_SPI_CTRL_SSEL_POLARITY(n) BIT(8U + (n))
#define IFX_SCB_SPI_CTRL_SSEL_SETUP_DEL   BIT(12)
#define IFX_SCB_SPI_CTRL_SSEL_HOLD_DEL    BIT(13)
#define IFX_SCB_SPI_CTRL_LOOPBACK         BIT(16)
#define IFX_SCB_SPI_CTRL_MODE             GENMASK(25, 24)
#define IFX_SCB_SPI_CTRL_SSEL             GENMASK(27, 26)
#define IFX_SCB_SPI_CTRL_MASTER_MODE      BIT(31)
#define IFX_SCB_SPI_CTRL_MODE_MOTOROLA    0U
#define IFX_SCB_SPI_CTRL_MODE_TI          1U

#define IFX_SCB_SPI_STATUS          0x024U
#define IFX_SCB_SPI_STATUS_BUS_BUSY BIT(0)

#define IFX_SCB_SPI_TX_CTRL 0x028U
#define IFX_SCB_SPI_RX_CTRL 0x02CU

/* TX_CTRL and RX_CTRL share the frame format fields */
#define IFX_SCB_TX_CTRL         0x200U
#define IFX_SCB_RX_CTRL         0x300U
#define IFX_SCB_CTRL_DATA_WIDTH GENMASK(4, 0)
#define IFX_SCB_CTRL_MSB_FIRST  BIT(8)

#define IFX_SCB_TX_FIFO_CTRL      0x204U
#define IFX_SCB_RX_FIFO_CTRL      0x304U
#define IFX_SCB_FIFO_CTRL_TRIGGER GENMASK(6, 0)
#define IFX_SCB_FIFO_CTRL_CLEAR   BIT(16)

#define IFX_SCB_TX_FIFO_STATUS   0x208U
#define IFX_SCB_RX_FIFO_STATUS   0x308U
#define IFX_SCB_FIFO_STATUS_USED GENMASK(7, 0)

#define IFX_SCB_TX_FIFO_WR 0x240U
#define IFX_SCB_RX_FIFO_RD 0x340U

#define IFX_SCB_INTR_I2C_EC_MASK 0xE88U
#define IFX_SCB_INTR_SPI_EC_MASK 0xEC8U

/* Interrupt groups: request (write 1 to clear) at +0x0, mask at +0x8, masked at +0xC */
#define IFX_SCB_INTR_M      0xF00U
#define IFX_SCB_INTR_S      0xF40U
#define IFX_SCB_INTR_TX     0xF80U
#define IFX_SCB_INTR_RX     0xFC0U
#define IFX_SCB_INTR_MASK   0x8U
#define IFX_SCB_INTR_MASKED 0xCU
#define IFX_SCB_INTR_ALL    GENMASK(31, 0)

#define IFX_SCB_INTR_M_SPI_DONE      BIT(9)
#define IFX_SCB_INTR_S_SPI_BUS_ERROR BIT(11)
#define IFX_SCB_INTR_TX_TRIGGER      BIT(0)
#define IFX_SCB_INTR_RX_TRIGGER      BIT(0)
#define IFX_SCB_INTR_RX_OVERFLOW     BIT(5)

/* The 256-byte SCB memory holds a TX and an RX FIFO of 128 bytes each */
#define IFX_SCB_FIFO_BYTES 128U

#define IFX_SCB_SPI_OVS_MIN  4U
#define IFX_SCB_SPI_OVS_MAX  16U
#define IFX_SCB_SPI_WORD_MIN 4U
#define IFX_SCB_SPI_WORD_MAX 32U
#define IFX_SCB_SPI_SSEL_MAX 3U

/* infineon,peri-div div-type values of the integer dividers */
#define IFX_SCB_SPI_DIV_8  0
#define IFX_SCB_SPI_DIV_16 1

#define IFX_SCB_SPI_SS_GPIO                                                                        \
	(IS_ENABLED(CONFIG_SPI_PERIPHERAL) && DT_ANY_INST_HAS_PROP_STATUS_OKAY(infineon_ss_gpios))

/* Register image of one SPI configuration */
struct ifx_scb_spi_regs {
	uint32_t ctrl;
	uint32_t spi_ctrl;
	uint32_t data_ctrl;
	uint32_t clk_rate;
};

struct ifx_scb_spi_config {
	uintptr_t base;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clk_dev;
	struct ifx_clk_peri clk_peri;
	const struct device *src_dev;
	struct ifx_clk src_clk;
	uint32_t div_max;
	unsigned int irq;
	void (*irq_config)(void);
#if IFX_SCB_SPI_SS_GPIO
	struct gpio_dt_spec ss_gpio;
#endif
};

struct ifx_scb_spi_data {
	struct spi_context ctx;
	struct k_spinlock lock;
	/* Configuration the hardware holds, valid while configured is true */
	struct spi_config applied;
	struct ifx_scb_spi_regs regs;
	bool configured;
	/* Frames clocked by the transfer, pushed to TX and pulled from RX */
	size_t total;
	size_t written;
	size_t read;
	uint8_t dfs;
	uint8_t depth;
	bool peripheral;
	bool native_cs;
	bool active;
#if IFX_SCB_SPI_SS_GPIO
	const struct device *dev;
	struct gpio_callback ss_cb;
#endif
};

static inline uint32_t ifx_scb_read(const struct device *dev, uint32_t off)
{
	const struct ifx_scb_spi_config *cfg = dev->config;

	return sys_read32(cfg->base + off);
}

static inline void ifx_scb_write(const struct device *dev, uint32_t off, uint32_t val)
{
	const struct ifx_scb_spi_config *cfg = dev->config;

	sys_write32(val, cfg->base + off);
}

static void ifx_scb_spi_fifo_clear(const struct device *dev)
{
	ifx_scb_write(dev, IFX_SCB_TX_FIFO_CTRL, IFX_SCB_FIFO_CTRL_CLEAR);
	ifx_scb_write(dev, IFX_SCB_TX_FIFO_CTRL, 0U);
	ifx_scb_write(dev, IFX_SCB_RX_FIFO_CTRL, IFX_SCB_FIFO_CTRL_CLEAR);
	ifx_scb_write(dev, IFX_SCB_RX_FIFO_CTRL, 0U);
}

static void ifx_scb_spi_mask_all(const struct device *dev)
{
	ifx_scb_write(dev, IFX_SCB_INTR_M + IFX_SCB_INTR_MASK, 0U);
	ifx_scb_write(dev, IFX_SCB_INTR_S + IFX_SCB_INTR_MASK, 0U);
	ifx_scb_write(dev, IFX_SCB_INTR_TX + IFX_SCB_INTR_MASK, 0U);
	ifx_scb_write(dev, IFX_SCB_INTR_RX + IFX_SCB_INTR_MASK, 0U);
}

static void ifx_scb_spi_disable(const struct device *dev)
{
	struct ifx_scb_spi_data *data = dev->data;

	/* Disabling resets the FIFOs and the interrupt requests */
	ifx_scb_spi_mask_all(dev);
	ifx_scb_write(dev, IFX_SCB_CTRL, 0U);
	data->configured = false;
}

/*
 * Among divider and oversampling pairs with the same product, an even factor
 * gives equal SCLK high and low phases, and a larger factor runs CLK_SCB
 * faster, which shortens the one CLK_SCB cycle error of the SELECT timing.
 */
static inline uint32_t ifx_scb_spi_ovs_rank(uint32_t ovs)
{
	return (((ovs % 2U) == 0U) ? IFX_SCB_SPI_OVS_MAX : 0U) + ovs;
}

/*
 * The SPI clock is src / (div * ovs). Pick the smallest product that does not
 * exceed the requested frequency.
 */
static int ifx_scb_spi_clock_pick(uint32_t src, uint32_t hz, uint32_t div_max, uint32_t *div,
				  uint32_t *ovs)
{
	uint64_t prod_min = DIV_ROUND_UP((uint64_t)src, hz);
	uint64_t best = UINT64_MAX;

	for (uint32_t k = IFX_SCB_SPI_OVS_MIN; k <= IFX_SCB_SPI_OVS_MAX; k++) {
		uint64_t n = MAX(DIV_ROUND_UP(prod_min, k), 1U);
		uint64_t prod = n * k;

		if (n > div_max) {
			continue;
		}

		if ((prod < best) ||
		    ((prod == best) && (ifx_scb_spi_ovs_rank(k) > ifx_scb_spi_ovs_rank(*ovs)))) {
			best = prod;
			*div = (uint32_t)n;
			*ovs = k;
		}
	}

	return (best == UINT64_MAX) ? -EINVAL : 0;
}

/*
 * Return true when the SELECT delay of quarter_cycles SCLK quarter periods is
 * shorter than ns, so that the delay one SCLK period longer is needed.
 */
static bool ifx_scb_spi_ssel_long(uint32_t ns, uint32_t hz, uint32_t quarter_cycles)
{
	uint64_t need = (uint64_t)ns * hz * 4U;

	if (need <= ((uint64_t)quarter_cycles * NSEC_PER_SEC)) {
		return false;
	}

	if (need > ((uint64_t)(quarter_cycles + 4U) * NSEC_PER_SEC)) {
		LOG_WRN("CS delay of %u ns exceeds %u/4 SCLK periods", ns, quarter_cycles + 4U);
	}

	return true;
}

static int ifx_scb_spi_check(const struct device *dev, const struct spi_config *spi_cfg)
{
	uint32_t op = spi_cfg->operation;
	uint32_t word = SPI_WORD_SIZE_GET(op);
	bool peripheral = (SPI_OP_MODE_GET(op) == SPI_OP_MODE_PERIPHERAL);

	if (peripheral && !IS_ENABLED(CONFIG_SPI_PERIPHERAL)) {
		LOG_ERR("Peripheral mode support is not enabled");
		return -ENOTSUP;
	}

	if ((op & SPI_HALF_DUPLEX) != 0U) {
		LOG_ERR("Half-duplex is not supported");
		return -ENOTSUP;
	}

	if ((op & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only single line mode is supported");
		return -ENOTSUP;
	}

	if ((word < IFX_SCB_SPI_WORD_MIN) || (word > IFX_SCB_SPI_WORD_MAX)) {
		LOG_ERR("Word size %u is not supported", word);
		return -ENOTSUP;
	}

	if (peripheral) {
		if ((op & SPI_MODE_LOOP) != 0U) {
			LOG_ERR("Loopback is not supported in peripheral mode");
			return -ENOTSUP;
		}

		return 0;
	}

	if (((op & SPI_FRAME_FORMAT_TI) != 0U) && spi_cs_is_gpio(spi_cfg)) {
		LOG_ERR("TI frame format needs the native slave select");
		return -ENOTSUP;
	}

	if (!spi_cs_is_gpio(spi_cfg) && (spi_cfg->peripheral > IFX_SCB_SPI_SSEL_MAX)) {
		LOG_ERR("Slave select %u does not exist", spi_cfg->peripheral);
		return -EINVAL;
	}

	if (spi_cfg->frequency == 0U) {
		LOG_ERR("Frequency must not be zero");
		return -EINVAL;
	}

	return 0;
}

static int ifx_scb_spi_compose(const struct device *dev, const struct spi_config *spi_cfg,
			       struct ifx_scb_spi_regs *r)
{
	const struct ifx_scb_spi_config *cfg = dev->config;
	uint32_t op = spi_cfg->operation;
	uint32_t word = SPI_WORD_SIZE_GET(op);
	uint32_t mem_width = (word <= 8U) ? 0U : ((word <= 16U) ? 1U : 2U);
	bool peripheral = (SPI_OP_MODE_GET(op) == SPI_OP_MODE_PERIPHERAL);
	bool ti = ((op & SPI_FRAME_FORMAT_TI) != 0U);
	bool cpha = ((SPI_MODE_GET(op) & SPI_MODE_CPHA) != 0U);
	bool native_cs = !peripheral && !spi_cs_is_gpio(spi_cfg);
	/* A peripheral listens on slave select 0 */
	uint32_t ssel = native_cs ? spi_cfg->peripheral : 0U;
	uint32_t div = 1U;
	uint32_t ovs = IFX_SCB_SPI_OVS_MIN;
	uint32_t src;
	int ret;

	ret = clock_control_get_rate(cfg->src_dev, (clock_control_subsys_t)&cfg->src_clk, &src);
	if (ret != 0) {
		return ret;
	}

	if (src == 0U) {
		LOG_ERR("SCB source clock is not running");
		return -EIO;
	}

	/*
	 * A peripheral ignores the oversampling factor. CLK_SCB at the full
	 * source rate keeps the CLK_SCB to SCLK ratio the slave logic needs.
	 */
	if (!peripheral) {
		ret = ifx_scb_spi_clock_pick(src, spi_cfg->frequency, cfg->div_max, &div, &ovs);
		if (ret != 0) {
			LOG_ERR("Frequency %u Hz is not reachable", spi_cfg->frequency);
			return ret;
		}
	}

	r->clk_rate = src / div;
	r->ctrl = FIELD_PREP(IFX_SCB_CTRL_OVS, ovs - 1U) |
		  FIELD_PREP(IFX_SCB_CTRL_MEM_WIDTH, mem_width) |
		  FIELD_PREP(IFX_SCB_CTRL_MODE, IFX_SCB_CTRL_MODE_SPI);
	r->data_ctrl = FIELD_PREP(IFX_SCB_CTRL_DATA_WIDTH, word - 1U);
	if ((op & SPI_TRANSFER_LSB) == 0U) {
		r->data_ctrl |= IFX_SCB_CTRL_MSB_FIRST;
	}

	if (ti) {
		/* TI mode runs CPOL 0, CPHA 1; polarity 0 selects a high frame pulse */
		r->spi_ctrl = FIELD_PREP(IFX_SCB_SPI_CTRL_MODE, IFX_SCB_SPI_CTRL_MODE_TI) |
			      IFX_SCB_SPI_CTRL_SELECT_PRECEDE | IFX_SCB_SPI_CTRL_CPHA;
	} else {
		r->spi_ctrl = FIELD_PREP(IFX_SCB_SPI_CTRL_MODE, IFX_SCB_SPI_CTRL_MODE_MOTOROLA);
		if ((SPI_MODE_GET(op) & SPI_MODE_CPOL) != 0U) {
			r->spi_ctrl |= IFX_SCB_SPI_CTRL_CPOL;
		}

		if (cpha) {
			r->spi_ctrl |= IFX_SCB_SPI_CTRL_CPHA;
		}

		if ((peripheral || native_cs) && ((op & SPI_CS_ACTIVE_HIGH) != 0U)) {
			r->spi_ctrl |= IFX_SCB_SPI_CTRL_SSEL_POLARITY(ssel);
		}
	}

	if (peripheral) {
		return 0;
	}

	r->spi_ctrl |= IFX_SCB_SPI_CTRL_MASTER_MODE | IFX_SCB_SPI_CTRL_LATE_MISO_SAMPLE |
		       IFX_SCB_SPI_CTRL_SSEL_CONTINUOUS | FIELD_PREP(IFX_SCB_SPI_CTRL_SSEL, ssel);

	if ((op & SPI_MODE_LOOP) != 0U) {
		r->spi_ctrl |= IFX_SCB_SPI_CTRL_LOOPBACK;
	}

	/*
	 * The SELECT setup and hold delays count from the first and to the last
	 * SCLK edge: 0.75 or 1.75 periods to the sampling edge, 0.25 or 1.25
	 * periods to the driving edge.
	 */
	if (!ti && native_cs) {
		uint32_t hz = src / (div * ovs);

		if (ifx_scb_spi_ssel_long(spi_cfg->cs.setup_ns, hz, cpha ? 1U : 3U)) {
			r->spi_ctrl |= IFX_SCB_SPI_CTRL_SSEL_SETUP_DEL;
		}

		if (ifx_scb_spi_ssel_long(spi_cfg->cs.hold_ns, hz, cpha ? 3U : 1U)) {
			r->spi_ctrl |= IFX_SCB_SPI_CTRL_SSEL_HOLD_DEL;
		}
	}

	LOG_DBG("SCLK %u Hz (CLK_SCB %u Hz / %u)", src / (div * ovs), r->clk_rate, ovs);

	return 0;
}

static int ifx_scb_spi_apply(const struct device *dev, const struct ifx_scb_spi_regs *r)
{
	const struct ifx_scb_spi_config *cfg = dev->config;
	struct ifx_scb_spi_data *data = dev->data;
	struct ifx_scb_spi_regs *cur = &data->regs;
	uint32_t rate = r->clk_rate;
	int ret;

	/*
	 * An idle controller accepts new TX_CTRL, RX_CTRL and SPI_CTRL.SSEL
	 * values without being disabled.
	 */
	if (data->configured && (r->ctrl == cur->ctrl) && (r->clk_rate == cur->clk_rate) &&
	    ((r->spi_ctrl & IFX_SCB_SPI_CTRL_MASTER_MODE) != 0U) &&
	    ((r->spi_ctrl & ~IFX_SCB_SPI_CTRL_SSEL) == (cur->spi_ctrl & ~IFX_SCB_SPI_CTRL_SSEL))) {
		ifx_scb_write(dev, IFX_SCB_SPI_CTRL, r->spi_ctrl);
		ifx_scb_write(dev, IFX_SCB_TX_CTRL, r->data_ctrl);
		ifx_scb_write(dev, IFX_SCB_RX_CTRL, r->data_ctrl);
		*cur = *r;
		return 0;
	}

	ifx_scb_spi_disable(dev);

	if (r->clk_rate != cur->clk_rate) {
		cur->clk_rate = 0U;
		ret = clock_control_set_rate(cfg->clk_dev, (clock_control_subsys_t)&cfg->clk_peri,
					     (clock_control_subsys_rate_t)&rate);
		if (ret != 0) {
			LOG_ERR("CLK_SCB %u Hz rejected (%d)", rate, ret);
			return ret;
		}
	}

	ifx_scb_write(dev, IFX_SCB_SPI_CTRL, r->spi_ctrl);
	ifx_scb_write(dev, IFX_SCB_SPI_TX_CTRL, 0U);
	ifx_scb_write(dev, IFX_SCB_SPI_RX_CTRL, 0U);
	ifx_scb_write(dev, IFX_SCB_TX_CTRL, r->data_ctrl);
	ifx_scb_write(dev, IFX_SCB_RX_CTRL, r->data_ctrl);
	ifx_scb_write(dev, IFX_SCB_TX_FIFO_CTRL, 0U);
	ifx_scb_write(dev, IFX_SCB_RX_FIFO_CTRL, 0U);
	ifx_scb_write(dev, IFX_SCB_INTR_I2C_EC_MASK, 0U);
	ifx_scb_write(dev, IFX_SCB_INTR_SPI_EC_MASK, 0U);
	ifx_scb_write(dev, IFX_SCB_CTRL, r->ctrl);
	ifx_scb_write(dev, IFX_SCB_CTRL, r->ctrl | IFX_SCB_CTRL_ENABLED);

	*cur = *r;
	data->configured = true;

	return 0;
}

static bool ifx_scb_spi_same_config(const struct spi_config *a, const struct spi_config *b)
{
	if ((a->frequency != b->frequency) || (a->operation != b->operation) ||
	    (spi_cs_is_gpio(a) != spi_cs_is_gpio(b))) {
		return false;
	}

	if (spi_cs_is_gpio(a)) {
		return true;
	}

	return (a->peripheral == b->peripheral) && (a->cs.setup_ns == b->cs.setup_ns) &&
	       (a->cs.hold_ns == b->cs.hold_ns);
}

static int ifx_scb_spi_configure(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct ifx_scb_spi_data *data = dev->data;
	struct ifx_scb_spi_regs r;
	uint32_t mem_width;
	int ret;

	if (!data->configured || !ifx_scb_spi_same_config(&data->applied, spi_cfg)) {
		ret = ifx_scb_spi_check(dev, spi_cfg);
		if (ret != 0) {
			return ret;
		}

		ret = ifx_scb_spi_compose(dev, spi_cfg, &r);
		if (ret != 0) {
			return ret;
		}

		ret = ifx_scb_spi_apply(dev, &r);
		if (ret != 0) {
			return ret;
		}

		data->applied = *spi_cfg;
		mem_width = FIELD_GET(IFX_SCB_CTRL_MEM_WIDTH, r.ctrl);
		data->dfs = BIT(mem_width);
		data->depth = IFX_SCB_FIFO_BYTES >> mem_width;
		data->peripheral = (SPI_OP_MODE_GET(spi_cfg->operation) == SPI_OP_MODE_PERIPHERAL);
		data->native_cs = !data->peripheral && !spi_cs_is_gpio(spi_cfg);
	}

	data->ctx.config = spi_cfg;

	return 0;
}

static inline uint32_t ifx_scb_spi_load(const uint8_t *buf, uint8_t dfs)
{
	switch (dfs) {
	case 1U:
		return *buf;
	case 2U:
		return sys_get_le16(buf);
	default:
		return sys_get_le32(buf);
	}
}

static inline void ifx_scb_spi_store(uint8_t *buf, uint8_t dfs, uint32_t val)
{
	switch (dfs) {
	case 1U:
		*buf = (uint8_t)val;
		break;
	case 2U:
		sys_put_le16((uint16_t)val, buf);
		break;
	default:
		sys_put_le32(val, buf);
		break;
	}
}

/* Write up to room frames to the TX FIFO, zeros where no TX buffer is given */
static void ifx_scb_spi_push(const struct device *dev, size_t room)
{
	struct ifx_scb_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	size_t n = MIN(room, data->total - data->written);

	while (n > 0U) {
		size_t run = spi_context_tx_on(ctx) ? MIN(n, ctx->tx_len) : n;

		for (size_t i = 0U; i < run; i++) {
			uint32_t val = 0U;

			if (ctx->tx_buf != NULL) {
				val = ifx_scb_spi_load(ctx->tx_buf + (i * data->dfs), data->dfs);
			}

			ifx_scb_write(dev, IFX_SCB_TX_FIFO_WR, val);
		}

		spi_context_update_tx(ctx, data->dfs, run);
		data->written += run;
		n -= run;
	}
}

/* Read the received frames, dropping those without an RX buffer */
static void ifx_scb_spi_pull(const struct device *dev)
{
	struct ifx_scb_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	size_t used =
		FIELD_GET(IFX_SCB_FIFO_STATUS_USED, ifx_scb_read(dev, IFX_SCB_RX_FIFO_STATUS));
	size_t n = MIN(used, data->total - data->read);

	while (n > 0U) {
		size_t run = spi_context_rx_on(ctx) ? MIN(n, ctx->rx_len) : n;

		for (size_t i = 0U; i < run; i++) {
			uint32_t val = ifx_scb_read(dev, IFX_SCB_RX_FIFO_RD);

			if (ctx->rx_buf != NULL) {
				ifx_scb_spi_store(ctx->rx_buf + (i * data->dfs), data->dfs, val);
			}
		}

		spi_context_update_rx(ctx, data->dfs, run);
		data->read += run;
		n -= run;
	}
}

/* Interrupt when the RX FIFO holds half a FIFO or the last frames */
static void ifx_scb_spi_set_rx_level(const struct device *dev)
{
	struct ifx_scb_spi_data *data = dev->data;
	size_t left = data->total - data->read;

	ifx_scb_write(dev, IFX_SCB_RX_FIFO_CTRL, MIN(left, data->depth / 2U) - 1U);
}

/*
 * A controller clocks a frame for every frame in the TX FIFO and drops frames
 * that arrive at a full RX FIFO. Limiting the frames written but not yet read
 * to the FIFO depth keeps the RX FIFO from overflowing.
 */
static size_t ifx_scb_spi_tx_room(const struct device *dev)
{
	struct ifx_scb_spi_data *data = dev->data;

	if (data->peripheral) {
		return data->depth - FIELD_GET(IFX_SCB_FIFO_STATUS_USED,
					       ifx_scb_read(dev, IFX_SCB_TX_FIFO_STATUS));
	}

	return data->depth - (data->written - data->read);
}

/* Stop the transfer; the caller holds the lock */
static void ifx_scb_spi_stop(const struct device *dev)
{
	struct ifx_scb_spi_data *data = dev->data;

	ifx_scb_spi_mask_all(dev);
	data->active = false;

	/* Drop frames a peripheral prepared but the controller did not clock */
	if (data->peripheral) {
		ifx_scb_spi_fifo_clear(dev);
	}
}

static void ifx_scb_spi_complete(const struct device *dev, int status)
{
	struct ifx_scb_spi_data *data = dev->data;

#if IFX_SCB_SPI_SS_GPIO
	const struct ifx_scb_spi_config *cfg = dev->config;

	if (data->peripheral && (cfg->ss_gpio.port != NULL)) {
		(void)gpio_pin_interrupt_configure_dt(&cfg->ss_gpio, GPIO_INT_DISABLE);
	}
#endif

	if (!data->peripheral) {
		spi_context_cs_control(&data->ctx, false);
	}

	spi_context_complete(&data->ctx, dev, status);
}

/*
 * The last RX frame is read. A GPIO chip select can drop now; the native
 * SELECT drops by itself after its hold delay, which SPI_DONE reports.
 */
static bool ifx_scb_spi_rx_done(const struct device *dev)
{
	struct ifx_scb_spi_data *data = dev->data;

	if (!data->native_cs) {
		ifx_scb_spi_stop(dev);
		return true;
	}

	ifx_scb_write(dev, IFX_SCB_INTR_RX + IFX_SCB_INTR_MASK, 0U);
	ifx_scb_write(dev, IFX_SCB_INTR_M, IFX_SCB_INTR_M_SPI_DONE);
	if ((ifx_scb_read(dev, IFX_SCB_SPI_STATUS) & IFX_SCB_SPI_STATUS_BUS_BUSY) == 0U) {
		ifx_scb_spi_stop(dev);
		return true;
	}

	ifx_scb_write(dev, IFX_SCB_INTR_M + IFX_SCB_INTR_MASK, IFX_SCB_INTR_M_SPI_DONE);

	return false;
}

/* Handle the pending events; return true once the transfer has ended */
static bool ifx_scb_spi_service(const struct device *dev, int *status)
{
	struct ifx_scb_spi_data *data = dev->data;
	uint32_t rx = ifx_scb_read(dev, IFX_SCB_INTR_RX + IFX_SCB_INTR_MASKED);
	uint32_t s = ifx_scb_read(dev, IFX_SCB_INTR_S + IFX_SCB_INTR_MASKED);
	uint32_t m = ifx_scb_read(dev, IFX_SCB_INTR_M + IFX_SCB_INTR_MASKED);

	if (!data->active) {
		return false;
	}

	if (((rx & IFX_SCB_INTR_RX_OVERFLOW) != 0U) || ((s & IFX_SCB_INTR_S_SPI_BUS_ERROR) != 0U)) {
		LOG_ERR("Transfer error (RX 0x%x, slave 0x%x)", rx, s);
		ifx_scb_spi_stop(dev);
		ifx_scb_spi_fifo_clear(dev);
		*status = -EIO;
		return true;
	}

	if ((m & IFX_SCB_INTR_M_SPI_DONE) != 0U) {
		ifx_scb_spi_stop(dev);
		*status = 0;
		return true;
	}

	if ((rx & IFX_SCB_INTR_RX_TRIGGER) != 0U) {
		ifx_scb_spi_pull(dev);
		if (data->read == data->total) {
			*status = 0;
			if (data->peripheral) {
				ifx_scb_spi_stop(dev);
				return true;
			}

			return ifx_scb_spi_rx_done(dev);
		}

		/* Clear after draining, or the trigger condition sets it again at once */
		ifx_scb_spi_set_rx_level(dev);
		ifx_scb_write(dev, IFX_SCB_INTR_RX, IFX_SCB_INTR_RX_TRIGGER);
	}

	ifx_scb_spi_push(dev, ifx_scb_spi_tx_room(dev));

	if (data->peripheral) {
		ifx_scb_write(dev, IFX_SCB_INTR_TX, IFX_SCB_INTR_TX_TRIGGER);
		if (data->written == data->total) {
			ifx_scb_write(dev, IFX_SCB_INTR_TX + IFX_SCB_INTR_MASK, 0U);
		}
	}

	return false;
}

static void ifx_scb_spi_isr(const struct device *dev)
{
	struct ifx_scb_spi_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int status = 0;
	bool done = ifx_scb_spi_service(dev, &status);

	k_spin_unlock(&data->lock, key);

	if (done) {
		ifx_scb_spi_complete(dev, status);
	}
}

#if IFX_SCB_SPI_SS_GPIO
/*
 * The controller deselected the peripheral. Every frame it clocked has
 * reached the RX FIFO, so read what is left and report the frame count.
 */
static void ifx_scb_spi_ss_handler(const struct device *port, struct gpio_callback *cb,
				   gpio_port_pins_t pins)
{
	struct ifx_scb_spi_data *data = CONTAINER_OF(cb, struct ifx_scb_spi_data, ss_cb);
	const struct device *dev = data->dev;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	bool done = data->active;

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	if (done) {
		ifx_scb_spi_pull(dev);
		ifx_scb_spi_stop(dev);
	}

	k_spin_unlock(&data->lock, key);

	if (done) {
		ifx_scb_spi_complete(dev, 0);
	}
}
#endif

static int ifx_scb_spi_start(const struct device *dev)
{
	struct ifx_scb_spi_data *data = dev->data;
	uint32_t rx_mask = IFX_SCB_INTR_RX_TRIGGER | IFX_SCB_INTR_RX_OVERFLOW;
	k_spinlock_key_t key;

#if IFX_SCB_SPI_SS_GPIO
	const struct ifx_scb_spi_config *cfg = dev->config;

	if (data->peripheral && (cfg->ss_gpio.port != NULL)) {
		int ret = gpio_pin_interrupt_configure_dt(&cfg->ss_gpio, GPIO_INT_EDGE_TO_INACTIVE);

		if (ret != 0) {
			return ret;
		}
	}
#endif

	key = k_spin_lock(&data->lock);

	/* Remove what an aborted transfer left behind */
	ifx_scb_spi_fifo_clear(dev);
	ifx_scb_write(dev, IFX_SCB_INTR_M, IFX_SCB_INTR_ALL);
	ifx_scb_write(dev, IFX_SCB_INTR_S, IFX_SCB_INTR_ALL);
	ifx_scb_write(dev, IFX_SCB_INTR_TX, IFX_SCB_INTR_ALL);
	ifx_scb_write(dev, IFX_SCB_INTR_RX, IFX_SCB_INTR_ALL);

	data->written = 0U;
	data->read = 0U;
	data->active = true;

	ifx_scb_spi_push(dev, ifx_scb_spi_tx_room(dev));
	ifx_scb_spi_set_rx_level(dev);

	if (data->peripheral) {
		ifx_scb_write(dev, IFX_SCB_INTR_S + IFX_SCB_INTR_MASK,
			      IFX_SCB_INTR_S_SPI_BUS_ERROR);
		if (data->written < data->total) {
			ifx_scb_write(dev, IFX_SCB_TX_FIFO_CTRL, data->depth / 2U);
			ifx_scb_write(dev, IFX_SCB_INTR_TX + IFX_SCB_INTR_MASK,
				      IFX_SCB_INTR_TX_TRIGGER);
		}
	}

	ifx_scb_write(dev, IFX_SCB_INTR_RX + IFX_SCB_INTR_MASK, rx_mask);

	k_spin_unlock(&data->lock, key);

	return 0;
}

/* Stop a transfer that did not complete in time */
static void ifx_scb_spi_abort(const struct device *dev)
{
	struct ifx_scb_spi_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	ifx_scb_spi_stop(dev);
	ifx_scb_spi_fifo_clear(dev);

	k_spin_unlock(&data->lock, key);

	if (!data->peripheral) {
		spi_context_cs_control(&data->ctx, false);
	}
}

static size_t ifx_scb_spi_frames(const struct spi_buf_set *bufs, uint8_t dfs)
{
	size_t frames = 0U;

	if (bufs == NULL) {
		return 0U;
	}

	for (size_t i = 0U; i < bufs->count; i++) {
		frames += bufs->buffers[i].len / dfs;
	}

	return frames;
}

static int ifx_scb_spi_transceive(const struct device *dev, const struct spi_config *spi_cfg,
				  const struct spi_buf_set *tx_bufs,
				  const struct spi_buf_set *rx_bufs, bool asynchronous,
				  spi_callback_t cb, void *userdata)
{
	struct ifx_scb_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;

	spi_context_lock(ctx, asynchronous, cb, userdata, spi_cfg);

	ret = ifx_scb_spi_configure(dev, spi_cfg);
	if (ret != 0) {
		goto out;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, data->dfs);
	data->total =
		MAX(ifx_scb_spi_frames(tx_bufs, data->dfs), ifx_scb_spi_frames(rx_bufs, data->dfs));

	if (!data->peripheral) {
		spi_context_cs_control(ctx, true);
	}

	if (data->total == 0U) {
		ifx_scb_spi_complete(dev, 0);
	} else {
		ret = ifx_scb_spi_start(dev);
		if (ret != 0) {
			goto out;
		}
	}

	ret = spi_context_wait_for_completion(ctx);
	if (ret == -ETIMEDOUT) {
		ifx_scb_spi_abort(dev);
	}

out:
	spi_context_release(ctx, ret);

	return ret;
}

static int ifx_scb_spi_transceive_sync(const struct device *dev, const struct spi_config *spi_cfg,
				       const struct spi_buf_set *tx_bufs,
				       const struct spi_buf_set *rx_bufs)
{
	return ifx_scb_spi_transceive(dev, spi_cfg, tx_bufs, rx_bufs, false, NULL, NULL);
}

#ifdef CONFIG_SPI_ASYNC
static int ifx_scb_spi_transceive_async(const struct device *dev, const struct spi_config *spi_cfg,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs, spi_callback_t cb,
					void *userdata)
{
	return ifx_scb_spi_transceive(dev, spi_cfg, tx_bufs, rx_bufs, true, cb, userdata);
}
#endif

static int ifx_scb_spi_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct ifx_scb_spi_data *data = dev->data;

	ARG_UNUSED(spi_cfg);

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, ifx_scb_spi_api) = {
	.transceive = ifx_scb_spi_transceive_sync,
#ifdef CONFIG_SPI_ASYNC
	.transceive_async = ifx_scb_spi_transceive_async,
#endif
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = ifx_scb_spi_release,
};

static int ifx_scb_spi_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct ifx_scb_spi_config *cfg = dev->config;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	case PM_DEVICE_ACTION_SUSPEND:
		ifx_scb_spi_disable(dev);
		ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_SLEEP);
		return (ret == -ENOENT) ? 0 : ret;
	default:
		return -ENOTSUP;
	}
}

static int ifx_scb_spi_init(const struct device *dev)
{
	const struct ifx_scb_spi_config *cfg = dev->config;
	struct ifx_scb_spi_data *data = dev->data;
	int ret;

	if (!device_is_ready(cfg->clk_dev) || !device_is_ready(cfg->src_dev)) {
		return -ENODEV;
	}

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret != 0) {
		return ret;
	}

#if IFX_SCB_SPI_SS_GPIO
	if (cfg->ss_gpio.port != NULL) {
		/* The pin stays routed to the SCB; only its edge interrupt is used */
		if (!gpio_is_ready_dt(&cfg->ss_gpio)) {
			return -ENODEV;
		}

		data->dev = dev;
		gpio_init_callback(&data->ss_cb, ifx_scb_spi_ss_handler, BIT(cfg->ss_gpio.pin));
		ret = gpio_add_callback_dt(&cfg->ss_gpio, &data->ss_cb);
		if (ret != 0) {
			return ret;
		}
	}
#endif

	cfg->irq_config();
	irq_enable(cfg->irq);

	spi_context_unlock_unconditionally(&data->ctx);

	return pm_device_driver_init(dev, ifx_scb_spi_pm_action);
}

#ifdef CONFIG_DEVICE_DEINIT_SUPPORT
static int ifx_scb_spi_deinit(const struct device *dev)
{
	const struct ifx_scb_spi_config *cfg = dev->config;
	struct ifx_scb_spi_data *data = dev->data;
	int ret;

	if (data->active) {
		return -EBUSY;
	}

	ret = pm_device_driver_deinit(dev, ifx_scb_spi_pm_action);
	if (ret != 0) {
		return ret;
	}

	irq_disable(cfg->irq);

#if IFX_SCB_SPI_SS_GPIO
	if (cfg->ss_gpio.port != NULL) {
		ret = gpio_remove_callback_dt(&cfg->ss_gpio, &data->ss_cb);
	}
#endif

	return ret;
}
#endif

#define IFX_SCB_SPI_DIV(n) DT_INST_CLOCKS_CTLR(n)

#define IFX_SCB_SPI_DIV_MAX(n)                                                                     \
	((DT_PROP(IFX_SCB_SPI_DIV(n), div_type) == IFX_SCB_SPI_DIV_8) ? BIT(8) : BIT(16))

#if IFX_SCB_SPI_SS_GPIO
#define IFX_SCB_SPI_SS_GPIO_INIT(n) .ss_gpio = GPIO_DT_SPEC_INST_GET_OR(n, infineon_ss_gpios, {0}),
#else
#define IFX_SCB_SPI_SS_GPIO_INIT(n)
#endif

#define IFX_SCB_SPI_INIT(n)                                                                        \
	BUILD_ASSERT((DT_PROP(IFX_SCB_SPI_DIV(n), div_type) == IFX_SCB_SPI_DIV_8) ||               \
		     (DT_PROP(IFX_SCB_SPI_DIV(n), div_type) == IFX_SCB_SPI_DIV_16),                \
		     "SPI needs an 8-bit or 16-bit integer peripheral clock divider");             \
	BUILD_ASSERT(!DT_NODE_HAS_PROP(IFX_SCB_SPI_DIV(n), clock_div),                             \
		     "SPI programs its clock divider and needs one without clock-div");            \
	BUILD_ASSERT(!DT_INST_NODE_HAS_PROP(n, infineon_ss_gpios) || IS_ENABLED(CONFIG_GPIO),      \
		     "infineon,ss-gpios needs CONFIG_GPIO");                                       \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static void ifx_scb_spi_irq_config_##n(void)                                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), ifx_scb_spi_isr,            \
			    DEVICE_DT_INST_GET(n), 0);                                             \
	}                                                                                          \
                                                                                                   \
	static const struct ifx_scb_spi_config ifx_scb_spi_config_##n = {                          \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.clk_dev = DEVICE_DT_GET(DT_PARENT(IFX_SCB_SPI_DIV(n))),                           \
		.clk_peri = IFX_CLK_PERI_DT_INST_SPEC_GET(n),                                      \
		.src_dev = DEVICE_DT_GET(DT_NODELABEL(clocks)),                                    \
		.src_clk =                                                                         \
			{                                                                          \
				.clk = IFX_CLK_HF,                                                 \
				.clk_id = DT_REG_ADDR(                                             \
					DT_CLOCKS_CTLR(DT_PARENT(IFX_SCB_SPI_DIV(n)))),            \
			},                                                                         \
		.div_max = IFX_SCB_SPI_DIV_MAX(n),                                                 \
		.irq = DT_INST_IRQN(n),                                                            \
		.irq_config = ifx_scb_spi_irq_config_##n,                                          \
		IFX_SCB_SPI_SS_GPIO_INIT(n)                                                        \
	};                                                                                         \
                                                                                                   \
	static struct ifx_scb_spi_data ifx_scb_spi_data_##n = {                                    \
		SPI_CONTEXT_INIT_LOCK(ifx_scb_spi_data_##n, ctx),                                  \
		SPI_CONTEXT_INIT_SYNC(ifx_scb_spi_data_##n, ctx),                                  \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)                               \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(n, ifx_scb_spi_pm_action);                                        \
                                                                                                   \
	SPI_DEVICE_DT_INST_DEINIT_DEFINE(n, ifx_scb_spi_init,                                      \
					 COND_CODE_1(CONFIG_DEVICE_DEINIT_SUPPORT,                 \
						     (ifx_scb_spi_deinit), (NULL)),                \
					 PM_DEVICE_DT_INST_GET(n), &ifx_scb_spi_data_##n,          \
					 &ifx_scb_spi_config_##n, POST_KERNEL,                     \
					 CONFIG_SPI_INIT_PRIORITY, &ifx_scb_spi_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_SCB_SPI_INIT)
