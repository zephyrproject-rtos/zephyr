/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_spi

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_CLOCK_CONTROL_IFX_PERI_CLOCK_V2
#include <zephyr/drivers/clock_control/clock_control_ifx.h>
#else
#include <infineon_kconfig.h>
#include <zephyr/drivers/clock_control/clock_control_ifx_cat1.h>
#endif

#ifdef CONFIG_SPI_INFINEON_DMA
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/mux.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_infineon_scb, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"
#include "spi_rtio.h"

#define IFX_SCB_CTRL           0x000U
#define IFX_SCB_CTRL_OVS       GENMASK(3, 0)
#define IFX_SCB_CTRL_BYTE_MODE BIT(11)
#define IFX_SCB_CTRL_MEM_WIDTH GENMASK(15, 14)
#define IFX_SCB_CTRL_MODE      GENMASK(25, 24)
#define IFX_SCB_CTRL_ENABLED   BIT(31)
#define IFX_SCB_CTRL_MODE_SPI  1U

#define IFX_SCB_SPI_CTRL                  0x020U
#define IFX_SCB_SPI_CTRL_SSEL_CONTINUOUS  BIT(0)
#define IFX_SCB_SPI_CTRL_SELECT_PRECEDE   BIT(1)
#define IFX_SCB_SPI_CTRL_CPHA             BIT(2)
#define IFX_SCB_SPI_CTRL_CPOL             BIT(3)
#define IFX_SCB_SPI_CTRL_LATE_MISO_SAMPLE     BIT(4)
#define IFX_SCB_SPI_CTRL_SCLK_CONTINUOUS      BIT(5)
#define IFX_SCB_SPI_CTRL_SSEL_POLARITY(n)     BIT(8U + (n))
#define IFX_SCB_SPI_CTRL_SSEL_SETUP_DEL       BIT(12)
#define IFX_SCB_SPI_CTRL_SSEL_HOLD_DEL        BIT(13)
#define IFX_SCB_SPI_CTRL_SSEL_INTER_FRAME_DEL BIT(14)
#define IFX_SCB_SPI_CTRL_SSEL_DELAYS          GENMASK(14, 12)
#define IFX_SCB_SPI_CTRL_LOOPBACK             BIT(16)
#define IFX_SCB_SPI_CTRL_MODE                 GENMASK(25, 24)
#define IFX_SCB_SPI_CTRL_SSEL                 GENMASK(27, 26)
#define IFX_SCB_SPI_CTRL_MASTER_MODE          BIT(31)
#define IFX_SCB_SPI_CTRL_MODE_MOTOROLA        0U
#define IFX_SCB_SPI_CTRL_MODE_TI              1U

#define IFX_SCB_SPI_SUB_MODE_FORMAT      GENMASK(1, 0)
#define IFX_SCB_SPI_SUB_MODE_PRECEDE     BIT(2)
#define IFX_SCB_SPI_SUB_MODE_TI_PRECEDES 5U

#define IFX_SCB_SPI_STATUS          0x024U
#define IFX_SCB_SPI_STATUS_BUS_BUSY BIT(0)

#define IFX_SCB_TX_CTRL          0x200U
#define IFX_SCB_RX_CTRL          0x300U
#define IFX_SCB_FRAME_DATA_WIDTH GENMASK(4, 0)
#define IFX_SCB_FRAME_MSB_FIRST  BIT(8)
#define IFX_SCB_FRAME_MASK       (IFX_SCB_FRAME_DATA_WIDTH | IFX_SCB_FRAME_MSB_FIRST)
#define IFX_SCB_RX_CTRL_MEDIAN   BIT(9)

#define IFX_SCB_TX_FIFO_CTRL     0x204U
#define IFX_SCB_RX_FIFO_CTRL     0x304U
#define IFX_SCB_FIFO_CTRL_CLEAR  BIT(16)
#define IFX_SCB_FIFO_CTRL_FREEZE BIT(17)

#define IFX_SCB_TX_FIFO_STATUS   0x208U
#define IFX_SCB_RX_FIFO_STATUS   0x308U
#define IFX_SCB_FIFO_STATUS_USED GENMASK(7, 0)

#define IFX_SCB_TX_FIFO_WR 0x240U
#define IFX_SCB_RX_FIFO_RD 0x340U

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

#define IFX_SCB_SPI_OVS_MIN  4U
#define IFX_SCB_SPI_OVS_MAX  16U
#define IFX_SCB_SPI_SSEL_MAX 3U

#define IFX_SCB_SPI_PERIPH_DELAY_PS 36660U

#define IFX_SCB_SPI_DMA_MAX_BYTES 256U

#define IFX_SCB_SPI_M0S8 DT_HAS_COMPAT_STATUS_OKAY(infineon_spi_m0s8)

#define IFX_SCB_SPI_DFS(op)                                                                        \
	((SPI_WORD_SIZE_GET(op) <= 8U) ? 1U : ((SPI_WORD_SIZE_GET(op) <= 16U) ? 2U : 4U))

#ifdef CONFIG_SPI_INFINEON_DMA
struct ifx_scb_spi_dma {
	const struct device *dev;
	uint32_t channel;
	const struct device *trig_dev;
	const struct mux_state *trig_state;
};
#endif

struct ifx_scb_spi_config {
	mem_addr_t base;
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_CLOCK_CONTROL_IFX_PERI_CLOCK_V2
	const struct device *clk_dev;
	struct ifx_clk_peri clk;
#else
	struct ifx_cat1_clock clk;
	en_clk_dst_t clk_dst;
#endif
	uint8_t fifo_depth;
	uint8_t sub_mode;
	bool input_filter;
	uint32_t spi_ctrl;
	void (*irq_config)(void);
#ifdef CONFIG_SPI_INFINEON_DMA
	struct ifx_scb_spi_dma dma_tx;
	struct ifx_scb_spi_dma dma_rx;
#endif
};

struct ifx_scb_spi_data {
	struct spi_context ctx;
	struct k_spinlock lock;
	size_t tx_left;
	size_t rx_left;
};

static void ifx_scb_spi_fifo_clear(mem_addr_t base)
{
	sys_set_bits(base + IFX_SCB_TX_FIFO_CTRL, IFX_SCB_FIFO_CTRL_CLEAR);
	sys_clear_bits(base + IFX_SCB_TX_FIFO_CTRL, IFX_SCB_FIFO_CTRL_CLEAR);
	sys_set_bits(base + IFX_SCB_RX_FIFO_CTRL, IFX_SCB_FIFO_CTRL_CLEAR);
	sys_clear_bits(base + IFX_SCB_RX_FIFO_CTRL, IFX_SCB_FIFO_CTRL_CLEAR);
}

static void ifx_scb_spi_mask_all(mem_addr_t base)
{
	sys_write32(0U, base + IFX_SCB_INTR_M + IFX_SCB_INTR_MASK);
	sys_write32(0U, base + IFX_SCB_INTR_S + IFX_SCB_INTR_MASK);
	sys_write32(0U, base + IFX_SCB_INTR_TX + IFX_SCB_INTR_MASK);
	sys_write32(0U, base + IFX_SCB_INTR_RX + IFX_SCB_INTR_MASK);
}

static bool ifx_scb_spi_busy(mem_addr_t base)
{
	return (sys_read32(base + IFX_SCB_INTR_M + IFX_SCB_INTR_MASK) |
		sys_read32(base + IFX_SCB_INTR_S + IFX_SCB_INTR_MASK) |
		sys_read32(base + IFX_SCB_INTR_TX + IFX_SCB_INTR_MASK) |
		sys_read32(base + IFX_SCB_INTR_RX + IFX_SCB_INTR_MASK)) != 0U;
}

static void ifx_scb_spi_halt(const struct device *dev)
{
	const struct ifx_scb_spi_config *config = dev->config;
	struct ifx_scb_spi_data *data = dev->data;

	ifx_scb_spi_mask_all(config->base);
	sys_clear_bits(config->base + IFX_SCB_CTRL, IFX_SCB_CTRL_ENABLED);
	data->ctx.config = NULL;
}

static uint64_t ifx_scb_spi_periph_rate(const struct device *dev, uint32_t hz)
{
	const struct ifx_scb_spi_config *config = dev->config;
	uint64_t half_ps = (hz == 0U) ? UINT64_MAX : ((uint64_t)NSEC_PER_SEC * 500U) / hz;

	if (half_ps <= IFX_SCB_SPI_PERIPH_DELAY_PS) {
		return UINT64_MAX;
	}

	return ((uint64_t)NSEC_PER_SEC * 1000U * (config->input_filter ? 4U : 3U)) /
	       (half_ps - IFX_SCB_SPI_PERIPH_DELAY_PS);
}

#ifdef CONFIG_CLOCK_CONTROL_IFX_PERI_CLOCK_V2
static int ifx_scb_spi_clk_init(const struct device *dev)
{
	const struct ifx_scb_spi_config *config = dev->config;

	return device_is_ready(config->clk_dev) ? 0 : -ENODEV;
}

static int ifx_scb_spi_clk_set(const struct device *dev, uint32_t hz, bool peripheral,
			       uint32_t *rate, uint32_t *ovs)
{
	const struct ifx_scb_spi_config *config = dev->config;
	uint64_t request = peripheral ? ifx_scb_spi_periph_rate(dev, hz)
				      : (uint64_t)hz * IFX_SCB_SPI_OVS_MIN;
	int ret;

	if ((hz == 0U) || (request > UINT32_MAX)) {
		return -ENOTSUP;
	}

	*rate = (uint32_t)request;
	ret = clock_control_set_rate(config->clk_dev, (clock_control_subsys_t)&config->clk,
				     (clock_control_subsys_rate_t)rate);
	if (ret == 0) {
		ret = clock_control_get_rate(config->clk_dev, (clock_control_subsys_t)&config->clk,
					     rate);
	}

	if ((ret != 0) || (*rate < request)) {
		return -ENOTSUP;
	}

	*ovs = peripheral ? IFX_SCB_SPI_OVS_MIN : DIV_ROUND_UP(*rate, hz);

	return (*ovs > IFX_SCB_SPI_OVS_MAX) ? -ENOTSUP : 0;
}
#else
static int ifx_scb_spi_clk_init(const struct device *dev)
{
	const struct ifx_scb_spi_config *config = dev->config;

	if (ifx_cat1_utils_peri_pclk_assign_divider(config->clk_dst, &config->clk) !=
	    CY_RSLT_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static uint32_t ifx_scb_spi_clk_src(const struct device *dev)
{
#ifdef CONFIG_SOC_FAMILY_INFINEON_PSOC4
	ARG_UNUSED(dev);

	return Cy_SysClk_ClkHfGetFrequency();
#else
	const struct ifx_scb_spi_config *config = dev->config;
	uint32_t freq = ifx_cat1_utils_peri_pclk_get_frequency(config->clk_dst, &config->clk);
	uint32_t div_int;
	uint32_t div_frac = 0U;

	if (ifx_cat1_utils_peri_is_fract_div(&config->clk)) {
		ifx_cat1_utils_peri_pclk_get_frac_divider(config->clk_dst, &config->clk, &div_int,
							  &div_frac);
	} else {
		div_int = ifx_cat1_utils_peri_pclk_get_divider(config->clk_dst, &config->clk);
	}

	return (freq * (div_int + 1U)) + ((freq / 32U) * div_frac);
#endif
}

static uint32_t ifx_scb_spi_clk_div(const struct device *dev, uint32_t src, uint32_t hz,
				    uint32_t *ovs)
{
	const struct ifx_scb_spi_config *config = dev->config;
	uint64_t cycles = DIV_ROUND_UP((uint64_t)src, hz);
	uint64_t best = UINT64_MAX;
	uint32_t divider = 0U;
	uint32_t div_max;

	switch (IFX_CAT1_PERIPHERAL_GROUP_GET_DIVIDER_TYPE(config->clk.block)) {
	case CY_SYSCLK_DIV_8_BIT:
		div_max = BIT(8);
		break;
	case CY_SYSCLK_DIV_24_5_BIT:
		div_max = BIT(24);
		break;
	default:
		div_max = BIT(16);
		break;
	}

	for (uint32_t k = IFX_SCB_SPI_OVS_MAX; k >= IFX_SCB_SPI_OVS_MIN; k--) {
		uint64_t n = DIV_ROUND_UP(cycles, k);

		if (n > div_max) {
			break;
		}

		if (((n * k) < best) ||
		    (((n * k) == best) && ((k % 2U) == 0U) && ((*ovs % 2U) != 0U))) {
			best = n * k;
			divider = (uint32_t)n;
			*ovs = k;
		}
	}

	return divider;
}

static int ifx_scb_spi_clk_set(const struct device *dev, uint32_t hz, bool peripheral,
			       uint32_t *rate, uint32_t *ovs)
{
	const struct ifx_scb_spi_config *config = dev->config;
	uint32_t src = ifx_scb_spi_clk_src(dev);
	uint32_t divider;
	cy_rslt_t status;

	if (src == 0U) {
		return -EIO;
	}

	if (peripheral && (src < ifx_scb_spi_periph_rate(dev, hz))) {
		return -ENOTSUP;
	}

	*ovs = IFX_SCB_SPI_OVS_MIN;
	divider = peripheral ? 1U : ifx_scb_spi_clk_div(dev, src, hz, ovs);
	if (divider == 0U) {
		return -ENOTSUP;
	}

	status = ifx_cat1_utils_peri_pclk_disable_divider(config->clk_dst, &config->clk);
	if (status != CY_RSLT_SUCCESS) {
		return -EIO;
	}

	if (ifx_cat1_utils_peri_is_fract_div(&config->clk)) {
		status = ifx_cat1_utils_peri_pclk_set_frac_divider(config->clk_dst, &config->clk,
								   divider - 1U, 0U);
	} else {
		status = ifx_cat1_utils_peri_pclk_set_divider(config->clk_dst, &config->clk,
							      divider - 1U);
	}

	if (status != CY_RSLT_SUCCESS) {
		return -EIO;
	}

	status = ifx_cat1_utils_peri_pclk_enable_divider(config->clk_dst, &config->clk);
	if (status != CY_RSLT_SUCCESS) {
		return -EIO;
	}

	*rate = src / divider;

	return 0;
}
#endif

static int ifx_scb_spi_check(const struct spi_config *spi_cfg, uint32_t format)
{
	uint32_t op = spi_cfg->operation;
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

	if ((SPI_WORD_SIZE_GET(op) < CONFIG_IFX_SPI_MIN_DATA_WIDTH) ||
	    (SPI_WORD_SIZE_GET(op) > CONFIG_IFX_SPI_MAX_DATA_WIDTH)) {
		LOG_ERR("Word size %u is not supported", SPI_WORD_SIZE_GET(op));
		return -ENOTSUP;
	}

	if (peripheral && ((op & SPI_MODE_LOOP) != 0U)) {
		LOG_ERR("Loopback is not supported in peripheral mode");
		return -ENOTSUP;
	}

	if (peripheral) {
		return 0;
	}

	if ((format == IFX_SCB_SPI_CTRL_MODE_TI) && spi_cs_is_gpio(spi_cfg)) {
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

static uint32_t ifx_scb_spi_cs_delay(uint32_t ns, uint32_t hz, uint32_t quarters, uint32_t bit)
{
	uint64_t need = (uint64_t)ns * hz * 4U;

	if (need <= ((uint64_t)quarters * NSEC_PER_SEC)) {
		return 0U;
	}

	if (need > ((uint64_t)(quarters + 4U) * NSEC_PER_SEC)) {
		LOG_WRN("CS delay of %u ns exceeds %u/4 SCLK periods", ns, quarters + 4U);
	}

	return bit;
}

static int ifx_scb_spi_configure(const struct device *dev, const struct spi_config *spi_cfg)
{
	const struct ifx_scb_spi_config *config = dev->config;
	struct ifx_scb_spi_data *data = dev->data;
	uint32_t op = spi_cfg->operation;
	bool peripheral = (SPI_OP_MODE_GET(op) == SPI_OP_MODE_PERIPHERAL);
	bool native_cs = !peripheral && !spi_cs_is_gpio(spi_cfg);
	uint32_t sub_mode = (((op & SPI_FRAME_FORMAT_TI) != 0U) &&
			     (config->sub_mode == IFX_SCB_SPI_CTRL_MODE_MOTOROLA))
				    ? IFX_SCB_SPI_SUB_MODE_TI_PRECEDES
				    : config->sub_mode;
	uint32_t format = FIELD_GET(IFX_SCB_SPI_SUB_MODE_FORMAT, sub_mode);
	bool motorola = (format == IFX_SCB_SPI_CTRL_MODE_MOTOROLA);
	bool ti = (format == IFX_SCB_SPI_CTRL_MODE_TI);
	bool cpha = ((SPI_MODE_GET(op) & SPI_MODE_CPHA) != 0U);
	uint32_t ssel = native_cs ? spi_cfg->peripheral : 0U;
	uint32_t frame = FIELD_PREP(IFX_SCB_FRAME_DATA_WIDTH, SPI_WORD_SIZE_GET(op) - 1U) |
			 (((op & SPI_TRANSFER_LSB) == 0U) ? IFX_SCB_FRAME_MSB_FIRST : 0U);
	uint32_t ctrl;
	uint32_t spi_ctrl;
	uint32_t rate;
	uint32_t ovs;
	int ret;

	ret = ifx_scb_spi_check(spi_cfg, format);
	if (ret != 0) {
		return ret;
	}

	ifx_scb_spi_halt(dev);

	ret = ifx_scb_spi_clk_set(dev, spi_cfg->frequency, peripheral, &rate, &ovs);
	if (ret != 0) {
		LOG_ERR("Frequency %u Hz is not reachable", spi_cfg->frequency);
		return ret;
	}

	ctrl = FIELD_PREP(IFX_SCB_CTRL_OVS, ovs - 1U) |
	       FIELD_PREP(IFX_SCB_CTRL_MODE, IFX_SCB_CTRL_MODE_SPI);

	if (IFX_SCB_SPI_M0S8) {
		ctrl |= (SPI_WORD_SIZE_GET(op) <= 8U) ? IFX_SCB_CTRL_BYTE_MODE : 0U;
	} else {
		ctrl |= FIELD_PREP(IFX_SCB_CTRL_MEM_WIDTH, IFX_SCB_SPI_DFS(op) / 2U);
	}

	spi_ctrl = FIELD_PREP(IFX_SCB_SPI_CTRL_MODE, format);

	if ((sub_mode & IFX_SCB_SPI_SUB_MODE_PRECEDE) != 0U) {
		spi_ctrl |= IFX_SCB_SPI_CTRL_SELECT_PRECEDE;
	}

	if (ti) {
		spi_ctrl |= IFX_SCB_SPI_CTRL_CPHA;
	}

	if (motorola) {
		spi_ctrl |= cpha ? IFX_SCB_SPI_CTRL_CPHA : 0U;
		spi_ctrl |= ((SPI_MODE_GET(op) & SPI_MODE_CPOL) != 0U) ? IFX_SCB_SPI_CTRL_CPOL : 0U;
	}

	if (!ti && (peripheral || native_cs) && ((op & SPI_CS_ACTIVE_HIGH) != 0U)) {
		spi_ctrl |= IFX_SCB_SPI_CTRL_SSEL_POLARITY(ssel);
	}

	if (!peripheral) {
		spi_ctrl |= IFX_SCB_SPI_CTRL_MASTER_MODE | FIELD_PREP(IFX_SCB_SPI_CTRL_SSEL, ssel) |
			    (config->spi_ctrl & ~IFX_SCB_SPI_CTRL_SSEL_DELAYS);
		spi_ctrl |= ((op & SPI_MODE_LOOP) != 0U) ? IFX_SCB_SPI_CTRL_LOOPBACK : 0U;
	}

	if (!peripheral && motorola) {
		spi_ctrl |= config->spi_ctrl & IFX_SCB_SPI_CTRL_SSEL_DELAYS;
	}

	if (motorola && native_cs) {
		spi_ctrl |= ifx_scb_spi_cs_delay(spi_cfg->cs.setup_ns, rate / ovs, cpha ? 1U : 3U,
						 IFX_SCB_SPI_CTRL_SSEL_SETUP_DEL) |
			    ifx_scb_spi_cs_delay(spi_cfg->cs.hold_ns, rate / ovs, cpha ? 3U : 1U,
						 IFX_SCB_SPI_CTRL_SSEL_HOLD_DEL);
	}

	if (IFX_SCB_SPI_M0S8 && ((spi_ctrl & IFX_SCB_SPI_CTRL_SSEL_DELAYS) != 0U)) {
		LOG_ERR("CS delays are not supported");
		return -ENOTSUP;
	}

	LOG_DBG("SCLK %u Hz (CLK_SCB %u Hz / %u)", rate / ovs, rate, ovs);

	sys_write32(spi_ctrl, config->base + IFX_SCB_SPI_CTRL);
	sys_write32((sys_read32(config->base + IFX_SCB_TX_CTRL) & ~IFX_SCB_FRAME_MASK) | frame,
		    config->base + IFX_SCB_TX_CTRL);
	sys_write32((sys_read32(config->base + IFX_SCB_RX_CTRL) &
		     ~(IFX_SCB_FRAME_MASK | IFX_SCB_RX_CTRL_MEDIAN)) |
			    frame | (config->input_filter ? IFX_SCB_RX_CTRL_MEDIAN : 0U),
		    config->base + IFX_SCB_RX_CTRL);
	sys_write32(0U, config->base + IFX_SCB_TX_FIFO_CTRL);
	sys_write32(0U, config->base + IFX_SCB_RX_FIFO_CTRL);
	sys_write32(ctrl, config->base + IFX_SCB_CTRL);
	sys_write32(ctrl | IFX_SCB_CTRL_ENABLED, config->base + IFX_SCB_CTRL);

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

static void ifx_scb_spi_push(const struct device *dev)
{
	const struct ifx_scb_spi_config *config = dev->config;
	struct ifx_scb_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint8_t dfs = IFX_SCB_SPI_DFS(ctx->config->operation);
	size_t depth = config->fifo_depth / dfs;
	size_t n;

	if (spi_context_is_peripheral(ctx)) {
		n = depth - FIELD_GET(IFX_SCB_FIFO_STATUS_USED,
				      sys_read32(config->base + IFX_SCB_TX_FIFO_STATUS));
	} else {
		n = depth - (data->rx_left - data->tx_left);
	}

	n = MIN(n, data->tx_left);
	data->tx_left -= n;

	for (; n > 0U; n--) {
		sys_write32((ctx->tx_buf != NULL) ? ifx_scb_spi_load(ctx->tx_buf, dfs) : 0U,
			    config->base + IFX_SCB_TX_FIFO_WR);
		spi_context_update_tx(ctx, dfs, 1U);
	}
}

static void ifx_scb_spi_pull(const struct device *dev)
{
	const struct ifx_scb_spi_config *config = dev->config;
	struct ifx_scb_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint8_t dfs = IFX_SCB_SPI_DFS(ctx->config->operation);
	size_t n = FIELD_GET(IFX_SCB_FIFO_STATUS_USED,
			     sys_read32(config->base + IFX_SCB_RX_FIFO_STATUS));

	n = MIN(n, data->rx_left);
	data->rx_left -= n;

	for (; n > 0U; n--) {
		uint32_t val = sys_read32(config->base + IFX_SCB_RX_FIFO_RD);

		if (ctx->rx_buf != NULL) {
			ifx_scb_spi_store(ctx->rx_buf, dfs, val);
		}

		if (spi_context_rx_on(ctx)) {
			spi_context_update_rx(ctx, dfs, 1U);
		}
	}
}

static void ifx_scb_spi_set_rx_level(const struct device *dev)
{
	const struct ifx_scb_spi_config *config = dev->config;
	struct ifx_scb_spi_data *data = dev->data;
	size_t depth = config->fifo_depth / IFX_SCB_SPI_DFS(data->ctx.config->operation);

	sys_write32(MIN(data->rx_left, depth / 2U) - 1U, config->base + IFX_SCB_RX_FIFO_CTRL);
}

static void ifx_scb_spi_quiesce(const struct device *dev)
{
	const struct ifx_scb_spi_config *config = dev->config;

	ifx_scb_spi_mask_all(config->base);

#ifdef CONFIG_SPI_INFINEON_DMA
	if (config->dma_tx.dev != NULL) {
		(void)dma_stop(config->dma_tx.dev, config->dma_tx.channel);
		(void)dma_stop(config->dma_rx.dev, config->dma_rx.channel);
	}
#endif
}

static void ifx_scb_spi_stop(const struct device *dev)
{
	const struct ifx_scb_spi_config *config = dev->config;

	ifx_scb_spi_quiesce(dev);
	ifx_scb_spi_fifo_clear(config->base);
}

static void ifx_scb_spi_complete(const struct device *dev, int status)
{
	struct ifx_scb_spi_data *data = dev->data;

	if (!spi_context_is_peripheral(&data->ctx)) {
		spi_context_cs_control(&data->ctx, false);
	}

	pm_policy_device_power_lock_put(dev);
	spi_context_complete(&data->ctx, dev, status);
}

static bool ifx_scb_spi_finish(const struct device *dev)
{
	const struct ifx_scb_spi_config *config = dev->config;
	struct ifx_scb_spi_data *data = dev->data;

	if (spi_context_is_peripheral(&data->ctx)) {
		ifx_scb_spi_quiesce(dev);
		return true;
	}

	if (spi_cs_is_gpio(data->ctx.config)) {
		ifx_scb_spi_stop(dev);
		return true;
	}

	sys_write32(0U, config->base + IFX_SCB_INTR_RX + IFX_SCB_INTR_MASK);
	sys_write32(IFX_SCB_INTR_M_SPI_DONE, config->base + IFX_SCB_INTR_M);

	if ((sys_read32(config->base + IFX_SCB_SPI_STATUS) & IFX_SCB_SPI_STATUS_BUS_BUSY) != 0U) {
		sys_write32(IFX_SCB_INTR_M_SPI_DONE,
			    config->base + IFX_SCB_INTR_M + IFX_SCB_INTR_MASK);
		return false;
	}

	ifx_scb_spi_stop(dev);

	return true;
}

#ifdef CONFIG_SPI_INFINEON_DMA
static void ifx_scb_spi_dma_callback(const struct device *dma_dev, void *arg, uint32_t channel,
				     int status);

static int ifx_scb_spi_dma_tx(const struct device *dev, size_t frames, dma_callback_t callback)
{
	static uint32_t zero;
	const struct ifx_scb_spi_config *config = dev->config;
	struct ifx_scb_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint8_t dfs = IFX_SCB_SPI_DFS(ctx->config->operation);
	struct dma_block_config blk = {
		.block_size = frames * dfs,
		.source_address = (ctx->tx_buf != NULL) ? (uintptr_t)ctx->tx_buf : (uintptr_t)&zero,
		.source_addr_adj = (ctx->tx_buf != NULL) ? DMA_ADDR_ADJ_INCREMENT
							 : DMA_ADDR_ADJ_NO_CHANGE,
		.dest_address = (uintptr_t)(config->base + IFX_SCB_TX_FIFO_WR),
		.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE,
	};
	struct dma_config cfg = {
		.channel_direction = MEMORY_TO_PERIPHERAL,
		.complete_callback_en = 1U,
		.source_handshake = 1U,
		.source_data_size = dfs,
		.dest_data_size = dfs,
		.source_burst_length = 1U,
		.block_count = 1U,
		.head_block = &blk,
		.user_data = (void *)dev,
		.dma_callback = callback,
	};
	int ret = dma_config(config->dma_tx.dev, config->dma_tx.channel, &cfg);

	if (ret != 0) {
		return ret;
	}

	return dma_start(config->dma_tx.dev, config->dma_tx.channel);
}

static int ifx_scb_spi_dma_rx(const struct device *dev, size_t frames)
{
	static uint32_t sink;
	const struct ifx_scb_spi_config *config = dev->config;
	struct ifx_scb_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint8_t dfs = IFX_SCB_SPI_DFS(ctx->config->operation);
	struct dma_block_config blk = {
		.block_size = frames * dfs,
		.source_address = (uintptr_t)(config->base + IFX_SCB_RX_FIFO_RD),
		.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE,
		.dest_address = (ctx->rx_buf != NULL) ? (uintptr_t)ctx->rx_buf : (uintptr_t)&sink,
		.dest_addr_adj = (ctx->rx_buf != NULL) ? DMA_ADDR_ADJ_INCREMENT
						       : DMA_ADDR_ADJ_NO_CHANGE,
	};
	struct dma_config cfg = {
		.channel_direction = PERIPHERAL_TO_MEMORY,
		.complete_callback_en = 1U,
		.source_data_size = dfs,
		.dest_data_size = dfs,
		.source_burst_length = 1U,
		.block_count = 1U,
		.head_block = &blk,
		.user_data = (void *)dev,
		.dma_callback = ifx_scb_spi_dma_callback,
	};
	int ret = dma_config(config->dma_rx.dev, config->dma_rx.channel, &cfg);

	if (ret != 0) {
		return ret;
	}

	return dma_start(config->dma_rx.dev, config->dma_rx.channel);
}

static size_t ifx_scb_spi_dma_frames(size_t len, size_t left, uint8_t dfs)
{
	return MIN((len != 0U) ? len : left, IFX_SCB_SPI_DMA_MAX_BYTES / dfs);
}

static bool ifx_scb_spi_dma_error(const struct device *dev, int ret, int *status)
{
	if (ret == 0) {
		return false;
	}

	ifx_scb_spi_stop(dev);
	*status = -EIO;

	return true;
}

static int ifx_scb_spi_dma_next(const struct device *dev)
{
	const struct ifx_scb_spi_config *config = dev->config;
	struct ifx_scb_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	bool rx_dma = (ctx->rx_buf != NULL);
	uint8_t dfs = IFX_SCB_SPI_DFS(ctx->config->operation);
	size_t frames = MIN(spi_context_max_continuous_chunk(ctx), IFX_SCB_SPI_DMA_MAX_BYTES / dfs);
	int ret;

	sys_write32(rx_dma ? 0U : IFX_SCB_FIFO_CTRL_FREEZE, config->base + IFX_SCB_RX_FIFO_CTRL);

	if (!rx_dma && (ctx->tx_buf == NULL)) {
		frames = MIN(frames, config->fifo_depth / dfs);

		for (size_t i = 0U; i < frames; i++) {
			sys_write32(0U, config->base + IFX_SCB_TX_FIFO_WR);
		}

		sys_write32(IFX_SCB_INTR_M_SPI_DONE,
			    config->base + IFX_SCB_INTR_M + IFX_SCB_INTR_MASK);
		spi_context_update_tx(ctx, dfs, frames);
		spi_context_update_rx(ctx, dfs, frames);

		return 0;
	}

	if (rx_dma) {
		ret = ifx_scb_spi_dma_rx(dev, frames);
		if (ret != 0) {
			return ret;
		}
	}

	ret = ifx_scb_spi_dma_tx(dev, frames, rx_dma ? NULL : ifx_scb_spi_dma_callback);
	if (ret != 0) {
		return ret;
	}

	spi_context_update_tx(ctx, dfs, frames);
	spi_context_update_rx(ctx, dfs, frames);

	return 0;
}

static bool ifx_scb_spi_dma_advance(const struct device *dev, int *status)
{
	struct ifx_scb_spi_data *data = dev->data;

	if (!spi_context_tx_on(&data->ctx) && !spi_context_rx_on(&data->ctx)) {
		return ifx_scb_spi_finish(dev);
	}

	return ifx_scb_spi_dma_error(dev, ifx_scb_spi_dma_next(dev), status);
}

static bool ifx_scb_spi_dma_stream(const struct device *dev, int *status)
{
	struct ifx_scb_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint8_t dfs = IFX_SCB_SPI_DFS(ctx->config->operation);
	size_t rx_frames = ifx_scb_spi_dma_frames(ctx->rx_len, data->rx_left, dfs);
	size_t tx_frames = ifx_scb_spi_dma_frames(ctx->tx_len, data->tx_left, dfs);
	int ret = ifx_scb_spi_dma_rx(dev, rx_frames);

	if (ret == 0) {
		ret = ifx_scb_spi_dma_tx(dev, tx_frames, ifx_scb_spi_dma_callback);
	}

	return ifx_scb_spi_dma_error(dev, ret, status);
}

static bool ifx_scb_spi_dma_tx_done(const struct device *dev, int *status)
{
	struct ifx_scb_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint8_t dfs = IFX_SCB_SPI_DFS(ctx->config->operation);
	size_t sent = ifx_scb_spi_dma_frames(ctx->tx_len, data->tx_left, dfs);
	size_t next;

	data->tx_left -= sent;
	spi_context_update_tx(ctx, dfs, sent);

	if (data->tx_left == 0U) {
		return false;
	}

	next = ifx_scb_spi_dma_frames(ctx->tx_len, data->tx_left, dfs);

	return ifx_scb_spi_dma_error(dev, ifx_scb_spi_dma_tx(dev, next, ifx_scb_spi_dma_callback),
				     status);
}

static bool ifx_scb_spi_dma_rx_done(const struct device *dev, int *status)
{
	struct ifx_scb_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint8_t dfs = IFX_SCB_SPI_DFS(ctx->config->operation);
	size_t received = ifx_scb_spi_dma_frames(ctx->rx_len, data->rx_left, dfs);
	size_t next;

	data->rx_left -= received;
	if (spi_context_rx_on(ctx)) {
		spi_context_update_rx(ctx, dfs, received);
	}

	if (data->rx_left == 0U) {
		return ifx_scb_spi_finish(dev);
	}

	next = ifx_scb_spi_dma_frames(ctx->rx_len, data->rx_left, dfs);

	return ifx_scb_spi_dma_error(dev, ifx_scb_spi_dma_rx(dev, next), status);
}

static void ifx_scb_spi_dma_callback(const struct device *dma_dev, void *arg, uint32_t channel,
				     int status)
{
	const struct device *dev = arg;
	const struct ifx_scb_spi_config *config = dev->config;
	struct ifx_scb_spi_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	bool tx = (dma_dev == config->dma_tx.dev) && (channel == config->dma_tx.channel);
	bool done = false;
	int ret = 0;

	if (!ifx_scb_spi_busy(config->base)) {
		k_spin_unlock(&data->lock, key);
		return;
	}

	if (status < 0) {
		ifx_scb_spi_stop(dev);
		ret = -EIO;
		done = true;
	} else if (spi_context_is_peripheral(&data->ctx)) {
		done = tx ? ifx_scb_spi_dma_tx_done(dev, &ret) : ifx_scb_spi_dma_rx_done(dev, &ret);
	} else if (tx) {
		sys_write32(IFX_SCB_INTR_M_SPI_DONE,
			    config->base + IFX_SCB_INTR_M + IFX_SCB_INTR_MASK);
	} else {
		done = ifx_scb_spi_dma_advance(dev, &ret);
	}

	k_spin_unlock(&data->lock, key);

	if (done) {
		ifx_scb_spi_complete(dev, ret);
	}
}
#endif

static bool ifx_scb_spi_bus_done(const struct device *dev, int *status)
{
	const struct ifx_scb_spi_config *config = dev->config;

	sys_write32(IFX_SCB_INTR_M_SPI_DONE, config->base + IFX_SCB_INTR_M);

	if ((sys_read32(config->base + IFX_SCB_SPI_STATUS) & IFX_SCB_SPI_STATUS_BUS_BUSY) != 0U) {
		return false;
	}

#ifdef CONFIG_SPI_INFINEON_DMA
	if (sys_read32(config->base + IFX_SCB_INTR_RX + IFX_SCB_INTR_MASK) ==
	    IFX_SCB_INTR_RX_OVERFLOW) {
		sys_write32(0U, config->base + IFX_SCB_INTR_M + IFX_SCB_INTR_MASK);
		return ifx_scb_spi_dma_advance(dev, status);
	}
#endif

	ifx_scb_spi_stop(dev);

	return true;
}

static bool ifx_scb_spi_service(const struct device *dev, int *status)
{
	const struct ifx_scb_spi_config *config = dev->config;
	struct ifx_scb_spi_data *data = dev->data;
	uint32_t rx = sys_read32(config->base + IFX_SCB_INTR_RX + IFX_SCB_INTR_MASKED);
	uint32_t s = sys_read32(config->base + IFX_SCB_INTR_S + IFX_SCB_INTR_MASKED);

	if (!ifx_scb_spi_busy(config->base)) {
		return false;
	}

	if (((rx & IFX_SCB_INTR_RX_OVERFLOW) != 0U) || ((s & IFX_SCB_INTR_S_SPI_BUS_ERROR) != 0U)) {
		LOG_ERR("Transfer error (RX 0x%x, slave 0x%x)", rx, s);
		ifx_scb_spi_stop(dev);
		*status = -EIO;
		return true;
	}

	if ((sys_read32(config->base + IFX_SCB_INTR_M + IFX_SCB_INTR_MASKED) &
	     IFX_SCB_INTR_M_SPI_DONE) != 0U) {
		return ifx_scb_spi_bus_done(dev, status);
	}

	if ((rx & IFX_SCB_INTR_RX_TRIGGER) != 0U) {
		ifx_scb_spi_pull(dev);
		if (data->rx_left == 0U) {
			return ifx_scb_spi_finish(dev);
		}

		ifx_scb_spi_set_rx_level(dev);
		sys_write32(IFX_SCB_INTR_RX_TRIGGER, config->base + IFX_SCB_INTR_RX);
	}

	ifx_scb_spi_push(dev);

	if (spi_context_is_peripheral(&data->ctx)) {
		sys_write32(IFX_SCB_INTR_TX_TRIGGER, config->base + IFX_SCB_INTR_TX);
		sys_write32((data->tx_left > 0U) ? IFX_SCB_INTR_TX_TRIGGER : 0U,
			    config->base + IFX_SCB_INTR_TX + IFX_SCB_INTR_MASK);
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

static void ifx_scb_spi_start(const struct device *dev)
{
	const struct ifx_scb_spi_config *config = dev->config;
	struct ifx_scb_spi_data *data = dev->data;
	size_t depth = config->fifo_depth / IFX_SCB_SPI_DFS(data->ctx.config->operation);
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	ifx_scb_spi_fifo_clear(config->base);
	sys_write32(IFX_SCB_INTR_ALL, config->base + IFX_SCB_INTR_M);
	sys_write32(IFX_SCB_INTR_ALL, config->base + IFX_SCB_INTR_S);
	sys_write32(IFX_SCB_INTR_ALL, config->base + IFX_SCB_INTR_TX);
	sys_write32(IFX_SCB_INTR_ALL, config->base + IFX_SCB_INTR_RX);

	if (spi_context_is_peripheral(&data->ctx)) {
		sys_write32(IFX_SCB_INTR_S_SPI_BUS_ERROR,
			    config->base + IFX_SCB_INTR_S + IFX_SCB_INTR_MASK);
	}

#ifdef CONFIG_SPI_INFINEON_DMA
	if ((config->dma_tx.dev != NULL) && (data->rx_left > depth)) {
		int ret = 0;
		bool done;

		sys_write32(IFX_SCB_INTR_RX_OVERFLOW,
			    config->base + IFX_SCB_INTR_RX + IFX_SCB_INTR_MASK);

		if (spi_context_is_peripheral(&data->ctx)) {
			sys_write32(0U, config->base + IFX_SCB_RX_FIFO_CTRL);
			sys_write32(depth - 1U, config->base + IFX_SCB_TX_FIFO_CTRL);
			done = ifx_scb_spi_dma_stream(dev, &ret);
		} else {
			data->tx_left = 0U;
			data->rx_left = 0U;
			sys_write32(1U, config->base + IFX_SCB_TX_FIFO_CTRL);
			done = ifx_scb_spi_dma_advance(dev, &ret);
		}

		k_spin_unlock(&data->lock, key);

		if (done) {
			ifx_scb_spi_complete(dev, ret);
		}

		return;
	}
#endif

	ifx_scb_spi_push(dev);
	ifx_scb_spi_set_rx_level(dev);

	if (spi_context_is_peripheral(&data->ctx)) {
		sys_write32(depth / 2U, config->base + IFX_SCB_TX_FIFO_CTRL);
		sys_write32((data->tx_left > 0U) ? IFX_SCB_INTR_TX_TRIGGER : 0U,
			    config->base + IFX_SCB_INTR_TX + IFX_SCB_INTR_MASK);
	}

	sys_write32(IFX_SCB_INTR_RX_TRIGGER | IFX_SCB_INTR_RX_OVERFLOW,
		    config->base + IFX_SCB_INTR_RX + IFX_SCB_INTR_MASK);

	k_spin_unlock(&data->lock, key);
}

static void ifx_scb_spi_abort(const struct device *dev)
{
	const struct ifx_scb_spi_config *config = dev->config;
	struct ifx_scb_spi_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	bool active = ifx_scb_spi_busy(config->base);

	if (active) {
		ifx_scb_spi_stop(dev);
	}

	k_spin_unlock(&data->lock, key);

	if (active) {
		ifx_scb_spi_complete(dev, -ETIMEDOUT);
	}

	k_sem_reset(&data->ctx.sync);
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
	uint8_t dfs = IFX_SCB_SPI_DFS(spi_cfg->operation);
	int ret;

	spi_context_lock(ctx, asynchronous, cb, userdata, spi_cfg);

	ret = spi_context_configured(ctx, spi_cfg) ? 0 : ifx_scb_spi_configure(dev, spi_cfg);
	if (ret != 0) {
		spi_context_release(ctx, ret);
		return ret;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, dfs);
	data->tx_left = ifx_scb_spi_frames(tx_bufs, dfs);
	data->rx_left = ifx_scb_spi_frames(rx_bufs, dfs);
	data->tx_left = MAX(data->tx_left, data->rx_left);
	data->rx_left = data->tx_left;

	pm_policy_device_power_lock_get(dev);

	if (!spi_context_is_peripheral(ctx)) {
		spi_context_cs_control(ctx, true);
	}

	if (data->rx_left == 0U) {
		ifx_scb_spi_complete(dev, 0);
	} else {
		ifx_scb_spi_start(dev);
	}

	ret = spi_context_wait_for_completion(ctx);
	if (ret == -ETIMEDOUT) {
		ifx_scb_spi_abort(dev);
	}

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

#ifdef CONFIG_SPI_INFINEON_DMA
static int ifx_scb_spi_dma_route(const struct device *dev)
{
	const struct ifx_scb_spi_config *config = dev->config;
	const struct ifx_scb_spi_dma *streams[] = {&config->dma_tx, &config->dma_rx};
	int ret;

	for (size_t i = 0U; i < ARRAY_SIZE(streams); i++) {
		if ((streams[i]->dev == NULL) || (streams[i]->trig_dev == NULL)) {
			continue;
		}

		if (!device_is_ready(streams[i]->trig_dev)) {
			return -ENODEV;
		}

		ret = mux_state_apply(streams[i]->trig_dev, streams[i]->trig_state);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}
#endif

static int ifx_scb_spi_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct ifx_scb_spi_config *config = dev->config;
	struct ifx_scb_spi_data *data = dev->data;
	k_spinlock_key_t key;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
#ifdef CONFIG_SPI_INFINEON_DMA
		ret = ifx_scb_spi_dma_route(dev);
		if (ret != 0) {
			return ret;
		}
#endif

		return pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	case PM_DEVICE_ACTION_SUSPEND:
		key = k_spin_lock(&data->lock);
		ret = ifx_scb_spi_busy(config->base) ? -EBUSY : 0;
		if (ret == 0) {
			ifx_scb_spi_halt(dev);
		}
		k_spin_unlock(&data->lock, key);

		if (ret != 0) {
			return ret;
		}

		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);

		return (ret == -ENOENT) ? 0 : ret;
	case PM_DEVICE_ACTION_TURN_ON:
		return ifx_scb_spi_clk_init(dev);
	default:
		return -ENOTSUP;
	}
}

static int ifx_scb_spi_init(const struct device *dev)
{
	const struct ifx_scb_spi_config *config = dev->config;
	struct ifx_scb_spi_data *data = dev->data;
	int ret;

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret != 0) {
		return ret;
	}

#ifdef CONFIG_SPI_INFINEON_DMA
	if ((config->dma_tx.dev != NULL) &&
	    (!device_is_ready(config->dma_tx.dev) || !device_is_ready(config->dma_rx.dev))) {
		return -ENODEV;
	}
#endif

	ifx_scb_spi_halt(dev);
	config->irq_config();

	spi_context_unlock_unconditionally(&data->ctx);

	return pm_device_driver_init(dev, ifx_scb_spi_pm_action);
}

#ifdef CONFIG_DEVICE_DEINIT_SUPPORT
static int ifx_scb_spi_deinit(const struct device *dev)
{
	const struct ifx_scb_spi_config *config = dev->config;

	if (ifx_scb_spi_busy(config->base)) {
		return -EBUSY;
	}

	return pm_device_driver_deinit(dev, ifx_scb_spi_pm_action);
}
#endif

#define IFX_SCB_SPI_DIV(n) DT_INST_CLOCKS_CTLR(n)

#ifdef CONFIG_CLOCK_CONTROL_IFX_PERI_CLOCK_V2
#define IFX_SCB_SPI_CLOCK_INIT(n)                                                                  \
	.clk_dev = DEVICE_DT_GET(DT_PARENT(IFX_SCB_SPI_DIV(n))),                                   \
	.clk = IFX_CLK_PERI_DT_INST_SPEC_GET(n),
#else
#define IFX_SCB_SPI_DIV_TYPE(n) DT_PROP(IFX_SCB_SPI_DIV(n), div_type)
#ifdef CONFIG_SOC_FAMILY_INFINEON_EDGE
#define IFX_SCB_SPI_CLOCK_BLOCK(n)                                                                 \
	IFX_CAT1_PERIPHERAL_GROUP_ADJUST(DT_PROP_BY_IDX(IFX_SCB_SPI_DIV(n), peri_group, 0),        \
					 DT_PROP_BY_IDX(IFX_SCB_SPI_DIV(n), peri_group, 1),        \
					 IFX_SCB_SPI_DIV_TYPE(n))
#else
#define IFX_SCB_SPI_CLOCK_BLOCK(n)                                                                 \
	IFX_CAT1_PERIPHERAL_GROUP_ADJUST(DT_PROP_BY_IDX(IFX_SCB_SPI_DIV(n), peri_group, 1),        \
					 IFX_SCB_SPI_DIV_TYPE(n))
#endif
#define IFX_SCB_SPI_CLOCK_INIT(n)                                                                  \
	.clk = {                                                                                   \
		.block = IFX_SCB_SPI_CLOCK_BLOCK(n),                                               \
		.channel = DT_PROP(IFX_SCB_SPI_DIV(n), channel),                                   \
	},                                                                                         \
	.clk_dst = (en_clk_dst_t)DT_INST_PROP(n, clk_dst),
#endif


#ifdef CONFIG_SPI_INFINEON_DMA
#define IFX_SCB_SPI_DMA_CHANNEL(n, dir)                                                            \
	.dma_##dir = {COND_CODE_1(DT_INST_DMAS_HAS_NAME(n, dir),                                   \
				  (.dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, dir)),        \
				   .channel = DT_INST_DMAS_CELL_BY_NAME(n, dir, channel),          \
				   .trig_dev = COND_CODE_1(                                        \
					   DT_INST_PROP_HAS_NAME(n, mux_states, dir),              \
					   (DEVICE_DT_GET(DT_INST_PHANDLE_BY_NAME(n, mux_states,   \
										  dir))),          \
					   (NULL)),                                                \
				   .trig_state = COND_CODE_1(                                      \
					   DT_INST_PROP_HAS_NAME(n, mux_states, dir),              \
					   (MUX_STATE_DT_GET_BY_NAME(DT_DRV_INST(n), dir)),        \
					   (NULL))),                                               \
				  (.dev = NULL))},
#define IFX_SCB_SPI_DMA_TRIG_DEFINE(n, dir)                                                        \
	IF_ENABLED(UTIL_AND(DT_INST_DMAS_HAS_NAME(n, dir),                                         \
			    DT_INST_PROP_HAS_NAME(n, mux_states, dir)),                            \
		   (MUX_STATE_DT_SPEC_DEFINE_BY_NAME(DT_DRV_INST(n), dir);))
#else
#define IFX_SCB_SPI_DMA_CHANNEL(n, dir)
#define IFX_SCB_SPI_DMA_TRIG_DEFINE(n, dir)
#endif

#define IFX_SCB_SPI_CTRL_DT(n)                                                                     \
	((DT_INST_PROP(n, enable_free_run_sclk) ? IFX_SCB_SPI_CTRL_SCLK_CONTINUOUS : 0U) |         \
	 (DT_INST_PROP(n, disable_miso_late_sample) ? 0U : IFX_SCB_SPI_CTRL_LATE_MISO_SAMPLE) |    \
	 (DT_INST_PROP(n, enable_transfer_separation) ? 0U : IFX_SCB_SPI_CTRL_SSEL_CONTINUOUS) |   \
	 (DT_INST_PROP(n, ss_setup_delay) ? IFX_SCB_SPI_CTRL_SSEL_SETUP_DEL : 0U) |                \
	 (DT_INST_PROP(n, ss_hold_delay) ? IFX_SCB_SPI_CTRL_SSEL_HOLD_DEL : 0U) |                  \
	 (DT_INST_PROP(n, ss_inter_frame_delay) ? IFX_SCB_SPI_CTRL_SSEL_INTER_FRAME_DEL : 0U))

#define IFX_SCB_SPI_INIT(n)                                                                        \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	IFX_SCB_SPI_DMA_TRIG_DEFINE(n, tx)                                                         \
	IFX_SCB_SPI_DMA_TRIG_DEFINE(n, rx)                                                         \
                                                                                                   \
	static void ifx_scb_spi_irq_config_##n(void)                                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), ifx_scb_spi_isr,            \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static const struct ifx_scb_spi_config ifx_scb_spi_config_##n = {                          \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		IFX_SCB_SPI_CLOCK_INIT(n)                                                          \
		.fifo_depth = DT_INST_PROP(n, fifo_depth),                                         \
		.sub_mode = DT_INST_PROP(n, sub_mode),                                             \
		.input_filter = DT_INST_PROP(n, enable_input_filter),                              \
		.spi_ctrl = IFX_SCB_SPI_CTRL_DT(n),                                                \
		.irq_config = ifx_scb_spi_irq_config_##n,                                          \
		IFX_SCB_SPI_DMA_CHANNEL(n, tx)                                                     \
		IFX_SCB_SPI_DMA_CHANNEL(n, rx)                                                     \
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
