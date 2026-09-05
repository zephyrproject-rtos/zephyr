/*
 * Copyright (c) 2026 Qingsong Gou <gouqs@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_jdi

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/jdi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>

#include <register.h>

LOG_MODULE_REGISTER(jdi_sf32lb, CONFIG_JDI_LOG_LEVEL);

/* LCDC Register Offsets */
#define LCDC_SETTING     offsetof(LCD_IF_TypeDef, SETTING)
#define LCDC_LCD_CONF    offsetof(LCD_IF_TypeDef, LCD_CONF)
#define LCDC_LCD_IF_CONF offsetof(LCD_IF_TypeDef, LCD_IF_CONF)
#define LCDC_LCD_SINGLE  offsetof(LCD_IF_TypeDef, LCD_SINGLE)
#define LCDC_LCD_WR      offsetof(LCD_IF_TypeDef, LCD_WR)
#define LCDC_IRQ         offsetof(LCD_IF_TypeDef, IRQ)
#define LCDC_STATUS      offsetof(LCD_IF_TypeDef, STATUS)

/* JDI Parallel Interface Registers */
#define LCDC_JDI_PAR_CONF1   offsetof(LCD_IF_TypeDef, JDI_PAR_CONF1)
#define LCDC_JDI_PAR_CONF2   offsetof(LCD_IF_TypeDef, JDI_PAR_CONF2)
#define LCDC_JDI_PAR_CONF3   offsetof(LCD_IF_TypeDef, JDI_PAR_CONF3)
#define LCDC_JDI_PAR_CONF4   offsetof(LCD_IF_TypeDef, JDI_PAR_CONF4)
#define LCDC_JDI_PAR_CONF5   offsetof(LCD_IF_TypeDef, JDI_PAR_CONF5)
#define LCDC_JDI_PAR_CONF6   offsetof(LCD_IF_TypeDef, JDI_PAR_CONF6)
#define LCDC_JDI_PAR_CONF7   offsetof(LCD_IF_TypeDef, JDI_PAR_CONF7)
#define LCDC_JDI_PAR_CONF8   offsetof(LCD_IF_TypeDef, JDI_PAR_CONF8)
#define LCDC_JDI_PAR_CONF9   offsetof(LCD_IF_TypeDef, JDI_PAR_CONF9)
#define LCDC_JDI_PAR_CONF10  offsetof(LCD_IF_TypeDef, JDI_PAR_CONF10)
#define LCDC_JDI_PAR_CTRL    offsetof(LCD_IF_TypeDef, JDI_PAR_CTRL)
#define LCDC_JDI_PAR_STAT    offsetof(LCD_IF_TypeDef, JDI_PAR_STAT)
#define LCDC_JDI_PAR_EX_CTRL offsetof(LCD_IF_TypeDef, JDI_PAR_EX_CTRL)

/* Canvas Registers */
#define LCDC_CANVAS_BG     offsetof(LCD_IF_TypeDef, CANVAS_BG)
#define LCDC_CANVAS_TL_POS offsetof(LCD_IF_TypeDef, CANVAS_TL_POS)
#define LCDC_CANVAS_BR_POS offsetof(LCD_IF_TypeDef, CANVAS_BR_POS)

/* Layer 0 Registers */
#define LCDC_LAYER0_CONFIG offsetof(LCD_IF_TypeDef, LAYER0_CONFIG)
#define LCDC_LAYER0_TL_POS offsetof(LCD_IF_TypeDef, LAYER0_TL_POS)
#define LCDC_LAYER0_BR_POS offsetof(LCD_IF_TypeDef, LAYER0_BR_POS)
#define LCDC_LAYER0_SRC    offsetof(LCD_IF_TypeDef, LAYER0_SRC)
#define LCDC_LAYER0_FILTER offsetof(LCD_IF_TypeDef, LAYER0_FILTER)
#define LCDC_LAYER0_FILL   offsetof(LCD_IF_TypeDef, LAYER0_FILL)
#define LCDC_LAYER0_DECOMP offsetof(LCD_IF_TypeDef, LAYER0_DECOMP)

/* Transfer completion timeout (ms) */
#define JDI_XFER_TIMEOUT_MS 500

/* LCD interface select values */
#define LCD_INTF_SEL_JDI_SERIAL   4U
#define LCD_INTF_SEL_JDI_PARALLEL 5U

struct jdi_sf32lb_config {
	uintptr_t base;
	const struct pinctrl_dev_config *pcfg;
	struct sf32lb_clock_dt_spec clock;
	void (*irq_configure)(void);
};

struct jdi_sf32lb_data {
	struct k_sem lock;
	struct k_sem xfer_done;
};

static void jdi_sf32lb_isr(const struct device *dev)
{
	const struct jdi_sf32lb_config *config = dev->config;
	struct jdi_sf32lb_data *data = dev->data;
	uintptr_t base = config->base;
	uint32_t irq_sts = sys_read32(base + LCDC_IRQ);

	sys_write32(irq_sts, base + LCDC_IRQ);
	/*
	 * Two-phase JDI parallel interrupt handling
	 * (matches HAL_LCDC_IRQHandler in bf0_hal_lcdc.c:5394-5423):
	 *
	 * Phase 1: JDI_PARL fires at INT_LINE_NUM (halfway).
	 * Update INT_LINE_NUM to max_line and disable ENABLE so the
	 * hardware finishes in single-shot mode.
	 * RETURN EARLY; do NOT touch SETTING or process EOF.
	 *
	 * Phase 2: JDI_PARL fires again at max_line (last line sent).
	 * Disable JDI_PARL interrupt (clear JDI_PARL_INTR_MASK).
	 * Clear XRST to reset the JDI parallel interface.
	 *
	 * EOF fires after the whole transfer completes → give semaphore.
	 */
	if (IS_BIT_SET(irq_sts, LCD_IF_IRQ_JDI_PARL_INTR_RAW_STAT_Pos)) {
		uint32_t ctrl = sys_read32(base + LCDC_JDI_PAR_CTRL);
		uint32_t conf1 = sys_read32(base + LCDC_JDI_PAR_CONF1);
		uint32_t int_line = FIELD_GET(LCD_IF_JDI_PAR_CTRL_INT_LINE_NUM_Msk, ctrl);
		uint32_t max_line = FIELD_GET(LCD_IF_JDI_PAR_CONF1_MAX_LINE_Msk, conf1);

		if (int_line != max_line) {
			/*
			 * Phase 1: halfway — re-arm to max_line, switch to
			 * single-shot.  Skip EOF check this iteration.
			 */
			ctrl &= ~LCD_IF_JDI_PAR_CTRL_INT_LINE_NUM_Msk;
			ctrl |= FIELD_PREP(LCD_IF_JDI_PAR_CTRL_INT_LINE_NUM_Msk, max_line);
			ctrl &= ~LCD_IF_JDI_PAR_CTRL_ENABLE;
			sys_write32(ctrl, base + LCDC_JDI_PAR_CTRL);
		} else {
			/* Phase 2: last line sent — disable JDI_PARL irq, reset */
			sys_clear_bits(base + LCDC_SETTING, LCD_IF_SETTING_JDI_PARL_INTR_MASK);
			sys_clear_bits(base + LCDC_JDI_PAR_CTRL, LCD_IF_JDI_PAR_CTRL_XRST);
			sys_set_bits(base + LCDC_JDI_PAR_EX_CTRL, LCD_IF_JDI_PAR_EX_CTRL_CNT_EN);
		}
	} else if (IS_BIT_SET(irq_sts, LCD_IF_IRQ_EOF_STAT_Pos)) {
		/* Transfer fully complete — disable EOF irq, signal completion */
		sys_clear_bits(base + LCDC_SETTING, LCD_IF_SETTING_EOF_MASK);
		k_sem_give(&data->xfer_done);
	}

	if (IS_BIT_SET(irq_sts, LCD_IF_IRQ_ICB_OF_STAT_Pos)) {
		LOG_ERR("LCDC ICB overflow!");
	}
}

static int jdi_sf32lb_xfer_wait(const struct device *dev)
{
	struct jdi_sf32lb_data *data = dev->data;

	return k_sem_take(&data->xfer_done, K_MSEC(JDI_XFER_TIMEOUT_MS));
}

static int sf32lb_jdi_config(const struct device *dev, const struct jdi_config *jdi_cfg)
{
	const struct jdi_sf32lb_config *config = dev->config;
	uintptr_t base = config->base;
	uint32_t lcdc_clk_Hz;
	uint32_t max_col, max_line;
	uint32_t reg_val;
	uint32_t lcd_conf;
	int ret;

	lcd_conf = sys_read32(base + LCDC_LCD_CONF);
	lcd_conf &= ~(LCD_IF_LCD_CONF_LCD_INTF_SEL_Msk | LCD_IF_LCD_CONF_TARGET_LCD_Msk |
		      LCD_IF_LCD_CONF_LCD_FORMAT_Msk | LCD_IF_LCD_CONF_AHB_FORMAT_Msk);
	lcd_conf |= FIELD_PREP(LCD_IF_LCD_CONF_LCD_INTF_SEL_Msk, LCD_INTF_SEL_JDI_PARALLEL) |
		    LCD_IF_LCD_CONF_LCD_FORMAT_RGB332 |
		    FIELD_PREP(LCD_IF_LCD_CONF_AHB_FORMAT_Msk, 3);
	sys_write32(lcd_conf, base + LCDC_LCD_CONF);

	/* Parallel mode - configure timing registers */
	ret = sf32lb_clock_control_get_rate_dt(&config->clock, &lcdc_clk_Hz);
	if (ret < 0) {
		LOG_ERR("Failed to get LCDC clock rate: %d", ret);
		return ret;
	}

	max_col = (jdi_cfg->bank_col_head + jdi_cfg->valid_columns + jdi_cfg->bank_col_tail) / 2;
	max_line = (jdi_cfg->bank_row_head + jdi_cfg->valid_rows + jdi_cfg->bank_row_tail) * 2;

	/* Match HAL timing calculation: hck_tk = ((clk + freq-1) / freq) >> 1 */
	uint32_t hck_tk = ((lcdc_clk_Hz + (jdi_cfg->freq - 1)) / jdi_cfg->freq) >> 1;
	uint32_t hst_tk = hck_tk;
	uint32_t hst_dly_tk = hst_tk;
	uint32_t hck_dly_tk = hck_tk / 2;

	uint32_t vck_tk = hck_tk * max_col;
	uint32_t vst_tk = vck_tk;
	uint32_t vck_dly_tk = vck_tk / 2;

	sys_write32(FIELD_PREP(LCD_IF_JDI_PAR_CONF1_MAX_COL_Msk, max_col - 1) |
			    FIELD_PREP(LCD_IF_JDI_PAR_CONF1_MAX_LINE_Msk, max_line - 1),
		    base + LCDC_JDI_PAR_CONF1);

	sys_write32(FIELD_PREP(LCD_IF_JDI_PAR_CONF4_HST_WIDTH_Msk, hst_tk) |
			    FIELD_PREP(LCD_IF_JDI_PAR_CONF4_HCK_WIDTH_Msk, hck_tk),
		    base + LCDC_JDI_PAR_CONF4);

	sys_write32(FIELD_PREP(LCD_IF_JDI_PAR_CONF5_VST_WIDTH_Msk, vst_tk) |
			    FIELD_PREP(LCD_IF_JDI_PAR_CONF5_VCK_WIDTH_Msk, vck_tk),
		    base + LCDC_JDI_PAR_CONF5);

	sys_write32(FIELD_PREP(LCD_IF_JDI_PAR_CONF6_HST_DLY_Msk, hst_dly_tk) |
			    FIELD_PREP(LCD_IF_JDI_PAR_CONF6_VCK_DLY_Msk, vck_dly_tk),
		    base + LCDC_JDI_PAR_CONF6);

	sys_write32(FIELD_PREP(LCD_IF_JDI_PAR_CONF7_HCK_DLY_Msk, hck_dly_tk) |
			    LCD_IF_JDI_PAR_CONF7_DP_MODE,
		    base + LCDC_JDI_PAR_CONF7);

	sys_write32(FIELD_PREP(LCD_IF_JDI_PAR_CONF8_ENB_ST_COL_Msk, jdi_cfg->enb_start_col) |
			    FIELD_PREP(LCD_IF_JDI_PAR_CONF8_ENB_END_COL_Msk, jdi_cfg->enb_end_col),
		    base + LCDC_JDI_PAR_CONF8);

	sys_write32(FIELD_PREP(LCD_IF_JDI_PAR_CTRL_ENBPOL_Msk, jdi_cfg->enb_pol_invert) |
			    FIELD_PREP(LCD_IF_JDI_PAR_CTRL_HCKPOL_Msk, jdi_cfg->hck_pol_invert) |
			    FIELD_PREP(LCD_IF_JDI_PAR_CTRL_HSTPOL_Msk, jdi_cfg->hst_pol_invert) |
			    FIELD_PREP(LCD_IF_JDI_PAR_CTRL_VCKPOL_Msk, jdi_cfg->vck_pol_invert) |
			    FIELD_PREP(LCD_IF_JDI_PAR_CTRL_VSTPOL_Msk, jdi_cfg->vst_pol_invert),
		    base + LCDC_JDI_PAR_CTRL);

	/* Hold JDI in reset until first transfer */
	sys_clear_bits(base + LCDC_JDI_PAR_CTRL, LCD_IF_JDI_PAR_CTRL_XRST);

	/* Initialize CANVAS - full screen */
	sys_write32(0, base + LCDC_CANVAS_BG);
	sys_write32(0, base + LCDC_CANVAS_TL_POS);
	sys_write32(FIELD_PREP(LCD_IF_CANVAS_BR_POS_X1_Msk, max_col - 1) |
			    FIELD_PREP(LCD_IF_CANVAS_BR_POS_Y1_Msk, max_line - 1),
		    base + LCDC_CANVAS_BR_POS);

	/* Initialize LAYER0 - RGB332, full screen */
	reg_val = LCD_IF_LAYER0_CONFIG_FORMAT_RGB332 |
		  FIELD_PREP(LCD_IF_LAYER0_CONFIG_ACTIVE_Msk, 1) |
		  FIELD_PREP(LCD_IF_LAYER0_CONFIG_PREFETCH_EN_Msk, 1) |
		  FIELD_PREP(LCD_IF_LAYER0_CONFIG_ALPHA_SEL_Msk, 0) |
		  FIELD_PREP(LCD_IF_LAYER0_CONFIG_ALPHA_Msk, 255) |
		  FIELD_PREP(LCD_IF_LAYER0_CONFIG_LINE_FETCH_MODE_Msk, 1) |
		  FIELD_PREP(LCD_IF_LAYER0_CONFIG_WIDTH_Msk, max_col);
	sys_write32(reg_val, base + LCDC_LAYER0_CONFIG);
	sys_write32(0, base + LCDC_LAYER0_TL_POS);
	sys_write32(FIELD_PREP(LCD_IF_LAYER0_BR_POS_X1_Msk, max_col - 1) |
			    FIELD_PREP(LCD_IF_LAYER0_BR_POS_Y1_Msk, max_line - 1),
		    base + LCDC_LAYER0_BR_POS);
	sys_write32(0, base + LCDC_LAYER0_DECOMP);

	/* Set VMirror to false (matching HAL_LCDC_LayerVMirror(false)) */
	sys_clear_bits(base + LCDC_LAYER0_CONFIG, LCD_IF_LAYER0_CONFIG_V_MIRROR_Msk);

	return 0;
}

static int sf32lb_jdi_write_cmd(const struct device *dev, uint8_t cmd)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cmd);

	return -ENOTSUP;
}

static int sf32lb_jdi_write_data(const struct device *dev, uint16_t x, uint16_t y, uint16_t width,
				 uint16_t height, const uint8_t *data_buf, size_t len,
				 const struct jdi_config *jdi_cfg)
{
	const struct jdi_sf32lb_config *config = dev->config;
	uintptr_t base = config->base;
	uint16_t roi_x1 = x + width - 1;
	uint16_t roi_y1 = y + height - 1;
	uint32_t max_line;
	uint32_t start_line, end_line;
	uint32_t start_col, end_col;
	uint32_t reg_val;

	start_line = (jdi_cfg->bank_row_head + y) * 2 + 1;
	end_line = (jdi_cfg->bank_row_head + roi_y1 + 1) * 2;

	start_col = (jdi_cfg->bank_col_head + x) / 2;
	end_col = (jdi_cfg->bank_col_head + roi_x1 + 1) / 2 - 1;
	max_line = (jdi_cfg->bank_row_head + jdi_cfg->valid_rows + jdi_cfg->bank_row_tail) * 2;

	/* Background color (fill outside ROI) */
	sys_write32(0, base + LCDC_CANVAS_BG);

	/* Canvas region */
	sys_write32(FIELD_PREP(LCD_IF_CANVAS_TL_POS_X0_Msk, x) |
			    FIELD_PREP(LCD_IF_CANVAS_TL_POS_Y0_Msk, y),
		    base + LCDC_CANVAS_TL_POS);
	sys_write32(FIELD_PREP(LCD_IF_CANVAS_BR_POS_X1_Msk, roi_x1) |
			    FIELD_PREP(LCD_IF_CANVAS_BR_POS_Y1_Msk, roi_y1),
		    base + LCDC_CANVAS_BR_POS);

	reg_val = LCD_IF_LAYER0_CONFIG_FORMAT_RGB332 |
		  FIELD_PREP(LCD_IF_LAYER0_CONFIG_ACTIVE_Msk, 1) |
		  FIELD_PREP(LCD_IF_LAYER0_CONFIG_PREFETCH_EN_Msk, 1) |
		  FIELD_PREP(LCD_IF_LAYER0_CONFIG_ALPHA_SEL_Msk, 0) |
		  FIELD_PREP(LCD_IF_LAYER0_CONFIG_ALPHA_Msk, 255) |
		  FIELD_PREP(LCD_IF_LAYER0_CONFIG_LINE_FETCH_MODE_Msk, 1) |
		  FIELD_PREP(LCD_IF_LAYER0_CONFIG_WIDTH_Msk, width);
	sys_write32(reg_val, base + LCDC_LAYER0_CONFIG);

	/* LAYER0 region */
	sys_write32(FIELD_PREP(LCD_IF_LAYER0_TL_POS_X0_Msk, x) |
			    FIELD_PREP(LCD_IF_LAYER0_TL_POS_Y0_Msk, y),
		    base + LCDC_LAYER0_TL_POS);
	sys_write32(FIELD_PREP(LCD_IF_LAYER0_BR_POS_X1_Msk, roi_x1) |
			    FIELD_PREP(LCD_IF_LAYER0_BR_POS_Y1_Msk, roi_y1),
		    base + LCDC_LAYER0_BR_POS);

	sys_write32(FIELD_PREP(LCD_IF_LAYER0_SRC_ADDR_Msk, (uint32_t)data_buf),
		    base + LCDC_LAYER0_SRC);

	/* B4-B6. Clear filter, fill, decompress */
	sys_write32(0, base + LCDC_LAYER0_FILTER);
	sys_write32(0, base + LCDC_LAYER0_FILL);
	sys_write32(0, base + LCDC_LAYER0_DECOMP);

	/* Start/end line numbers */
	sys_write32(FIELD_PREP(LCD_IF_JDI_PAR_CONF2_ST_LINE_Msk, start_line) |
			    FIELD_PREP(LCD_IF_JDI_PAR_CONF2_END_LINE_Msk, end_line),
		    base + LCDC_JDI_PAR_CONF2);

	/* Start/end column numbers */
	sys_write32(FIELD_PREP(LCD_IF_JDI_PAR_CONF3_ST_COL_Msk, start_col) |
			    FIELD_PREP(LCD_IF_JDI_PAR_CONF3_END_COL_Msk, end_col),
		    base + LCDC_JDI_PAR_CONF3);

	/* ENB active line range (1 line wider than data lines) */
	sys_write32(FIELD_PREP(LCD_IF_JDI_PAR_CONF9_ENB_ST_LINE_Msk, start_line + 1) |
			    FIELD_PREP(LCD_IF_JDI_PAR_CONF9_ENB_END_LINE_Msk, end_line + 1),
		    base + LCDC_JDI_PAR_CONF9);

	/* HCK active line range */
	sys_write32(FIELD_PREP(LCD_IF_JDI_PAR_CONF10_HC_ST_LINE_Msk, start_line) |
			    FIELD_PREP(LCD_IF_JDI_PAR_CONF10_HC_END_LINE_Msk, end_line + 2),
		    base + LCDC_JDI_PAR_CONF10);

	/* Interrupt line number = max_line/2 - 1 (triggers at halfway) */
	reg_val = sys_read32(base + LCDC_JDI_PAR_CTRL);
	reg_val &= ~LCD_IF_JDI_PAR_CTRL_INT_LINE_NUM_Msk;
	reg_val |= FIELD_PREP(LCD_IF_JDI_PAR_CTRL_INT_LINE_NUM_Msk, (max_line / 2) - 1);
	sys_write32(reg_val, base + LCDC_JDI_PAR_CTRL);

	/* Enable interrupts (set mask bits to enable) */
	sys_set_bits(base + LCDC_SETTING,
		     LCD_IF_SETTING_JDI_PARL_INTR_MASK | LCD_IF_SETTING_EOF_MASK);

	/* Release reset -> wait 35us */
	sys_set_bits(base + LCDC_JDI_PAR_CTRL, LCD_IF_JDI_PAR_CTRL_XRST);
	k_busy_wait(35);

	/* Start DMA transfer */
	sys_set_bits(base + LCDC_JDI_PAR_CTRL, LCD_IF_JDI_PAR_CTRL_ENABLE);

	LOG_DBG("JDI write: (%u,%u) %ux%u conf2=0x%08x conf3=0x%08x", x, y, width, height,
		sys_read32(base + LCDC_JDI_PAR_CONF2), sys_read32(base + LCDC_JDI_PAR_CONF3));

	return 0;
}

static int sf32lb_jdi_read_data(const struct device *dev, uint8_t *data_buf, size_t len)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(data_buf);
	ARG_UNUSED(len);

	return -ENOTSUP;
}

static const struct jdi_bus_api jdi_sf32lb_api = {
	.config = sf32lb_jdi_config,
	.write_cmd = sf32lb_jdi_write_cmd,
	.write_data = sf32lb_jdi_write_data,
	.read_data = sf32lb_jdi_read_data,
};

static int jdi_sf32lb_init(const struct device *dev)
{
	const struct jdi_sf32lb_config *config = dev->config;
	struct jdi_sf32lb_data *data = dev->data;
	int ret;

	LOG_DBG("Initializing JDI device %s", dev->name);

	/* Enable LCDC clock */
	if (!sf32lb_clock_is_ready_dt(&config->clock)) {
		LOG_ERR("LCDC clock not ready");
		return -ENODEV;
	}

	ret = sf32lb_clock_control_on_dt(&config->clock);
	if (ret < 0) {
		LOG_ERR("Failed to enable LCDC clock: %d", ret);
		return ret;
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to configure pins: %d", ret);
		return ret;
	}

	/* Enable auto-gate and release LCD reset */
	sys_set_bits(config->base + LCDC_SETTING, LCD_IF_SETTING_AUTO_GATE_EN);
	sys_set_bits(config->base + LCDC_LCD_IF_CONF, LCD_IF_LCD_IF_CONF_LCD_RSTB);

	k_sem_init(&data->lock, 1, 1);
	k_sem_init(&data->xfer_done, 0, 1);

	config->irq_configure();

	LOG_DBG("JDI device initialized successfully");

	return 0;
}

#define JDI_SF32LB_IRQ_CONFIGURE(n)                                                                \
	IRQ_CONNECT(DT_IRQN(DT_INST_PARENT(n)), DT_IRQ(DT_INST_PARENT(n), priority),               \
		    jdi_sf32lb_isr, DEVICE_DT_INST_GET(n), 0);                                     \
	irq_enable(DT_IRQN(DT_INST_PARENT(n)));

#define JDI_SF32LB_DEFINE(n)                                                                       \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static void jdi_sf32lb_irq_configure_##n(void)                                             \
	{                                                                                          \
		JDI_SF32LB_IRQ_CONFIGURE(n);                                                       \
	}                                                                                          \
                                                                                                   \
	static const struct jdi_sf32lb_config jdi_sf32lb_config_##n = {                            \
		.base = DT_REG_ADDR(DT_INST_PARENT(n)),                                            \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.clock = SF32LB_CLOCK_DT_INST_PARENT_SPEC_GET(n),                                  \
		.irq_configure = jdi_sf32lb_irq_configure_##n,                                     \
	};                                                                                         \
                                                                                                   \
	static struct jdi_sf32lb_data jdi_sf32lb_data_##n = {};                                    \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, jdi_sf32lb_init, NULL, &jdi_sf32lb_data_##n,                      \
			      &jdi_sf32lb_config_##n, POST_KERNEL, CONFIG_JDI_INIT_PRIORITY,       \
			      &jdi_sf32lb_api);

DT_INST_FOREACH_STATUS_OKAY(JDI_SF32LB_DEFINE)
