/*
 * Copyright (c) 2026 Qingsong Gou <gouqs@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_jdi_parallel

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>

#include <register.h>

LOG_MODULE_REGISTER(sf32lb_jdi_parallel, CONFIG_DISPLAY_LOG_LEVEL);

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

struct sf32lb_jdi_parallel_config {
	uintptr_t base;
	const struct pinctrl_dev_config *pcfg;
	struct sf32lb_clock_dt_spec clock;
	struct gpio_dt_spec vlcd_gpio;
	struct gpio_dt_spec vddp_gpio;
	struct pwm_dt_spec vcom_pwm;
	struct pwm_dt_spec vcom_pwm_inv;
	uint16_t power_seq_delay_ms;
	uint16_t width;
	uint16_t height;
	void (*irq_configure)(void);
	/** Clock frequency in Hz. */
	uint32_t freq;

	/** Column bank head padding in pixels (left side). */
	uint16_t bank_col_head;
	/** Number of valid/visible columns in pixels. */
	uint16_t valid_columns;
	/** Column bank tail padding in pixels (right side). */
	uint16_t bank_col_tail;

	/** Row bank head padding in pixels (top side). */
	uint16_t bank_row_head;
	/** Number of valid/visible rows in pixels. */
	uint16_t valid_rows;
	/** Row bank tail padding in pixels (bottom side). */
	uint16_t bank_row_tail;

	/** ENB (Enable) active start column number. */
	uint16_t enb_start_col;
	/** ENB (Enable) active end column number. */
	uint16_t enb_end_col;

	/** Enable signal polarity invert. 1 = low active, 0 = high active (default). */
	uint8_t enb_pol_invert: 1;
	/** HCK (Horizontal Clock) polarity invert. 1 = low active, 0 = high active (default). */
	uint8_t hck_pol_invert: 1;
	/** HST (Horizontal Start) polarity invert. 1 = low active, 0 = high active (default). */
	uint8_t hst_pol_invert: 1;
	/** VCK (Vertical Clock) polarity invert. 1 = low active, 0 = high active (default). */
	uint8_t vck_pol_invert: 1;
	/** VST (Vertical Start) polarity invert. 1 = low active, 0 = high active (default). */
	uint8_t vst_pol_invert: 1;
	/** Reserved bits for future use. */
	uint8_t reserved: 3;
};

static void sf32lb_jdi_isr(const struct device *dev)
{
	const struct sf32lb_jdi_parallel_config *config = dev->config;
	uintptr_t base = config->base;
	uint32_t irq_sts = sys_read32(base + LCDC_IRQ);

	sys_write32(irq_sts, base + LCDC_IRQ);

	if (IS_BIT_SET(irq_sts, LCD_IF_IRQ_JDI_PARL_INTR_RAW_STAT_Pos)) {
		uint32_t ctrl = sys_read32(base + LCDC_JDI_PAR_CTRL);
		uint32_t conf1 = sys_read32(base + LCDC_JDI_PAR_CONF1);
		uint32_t int_line = FIELD_GET(LCD_IF_JDI_PAR_CTRL_INT_LINE_NUM_Msk, ctrl);
		uint32_t max_line = FIELD_GET(LCD_IF_JDI_PAR_CONF1_MAX_LINE_Msk, conf1);

		if (int_line != max_line) {
			ctrl &= ~LCD_IF_JDI_PAR_CTRL_INT_LINE_NUM_Msk;
			ctrl |= FIELD_PREP(LCD_IF_JDI_PAR_CTRL_INT_LINE_NUM_Msk, max_line);
			ctrl &= ~LCD_IF_JDI_PAR_CTRL_ENABLE;
			sys_write32(ctrl, base + LCDC_JDI_PAR_CTRL);
			return;
		} else {
			sys_clear_bits(base + LCDC_SETTING, LCD_IF_SETTING_JDI_PARL_INTR_MASK);
			sys_clear_bits(base + LCDC_JDI_PAR_CTRL, LCD_IF_JDI_PAR_CTRL_XRST);
			sys_set_bits(base + LCDC_JDI_PAR_EX_CTRL, LCD_IF_JDI_PAR_EX_CTRL_CNT_EN);
		}
	}

	if (IS_BIT_SET(irq_sts, LCD_IF_IRQ_EOF_STAT_Pos)) {
		sys_clear_bits(base + LCDC_SETTING, LCD_IF_SETTING_EOF_MASK);
	}

	if (IS_BIT_SET(irq_sts, LCD_IF_IRQ_ICB_OF_STAT_Pos)) {
		LOG_ERR("LCDC ICB overflow!");
	}
}

static int sf32lb_jdi_config(const struct device *dev)
{
	const struct sf32lb_jdi_parallel_config *config = dev->config;
	uintptr_t base = config->base;
	uint32_t lcdc_clk_Hz;
	uint32_t max_col, max_line;
	uint32_t reg_val;
	int ret;

	reg_val = sys_read32(base + LCDC_LCD_CONF);
	reg_val &= ~(LCD_IF_LCD_CONF_LCD_INTF_SEL_Msk | LCD_IF_LCD_CONF_TARGET_LCD_Msk |
		     LCD_IF_LCD_CONF_LCD_FORMAT_Msk | LCD_IF_LCD_CONF_AHB_FORMAT_Msk);
	reg_val |= FIELD_PREP(LCD_IF_LCD_CONF_LCD_INTF_SEL_Msk, LCD_INTF_SEL_JDI_PARALLEL) |
		   LCD_IF_LCD_CONF_LCD_FORMAT_RGB332 |
		   FIELD_PREP(LCD_IF_LCD_CONF_AHB_FORMAT_Msk, 3);
	sys_write32(reg_val, base + LCDC_LCD_CONF);

	/* Parallel mode - configure timing registers */
	ret = sf32lb_clock_control_get_rate_dt(&config->clock, &lcdc_clk_Hz);
	if (ret < 0) {
		LOG_ERR("Failed to get LCDC clock rate: %d", ret);
		return ret;
	}

	max_col = (config->bank_col_head + config->valid_columns + config->bank_col_tail) / 2;
	max_line = (config->bank_row_head + config->valid_rows + config->bank_row_tail) * 2;

	/* Match HAL timing calculation: hck_tk = ((clk + freq-1) / freq) >> 1 */
	uint32_t hck_tk = ((lcdc_clk_Hz + (config->freq - 1)) / config->freq) >> 1;
	uint32_t hst_tk = hck_tk;
	uint32_t hst_dly_tk = hst_tk;
	uint32_t hck_dly_tk = hck_tk / 2;

	uint32_t vck_tk = hck_tk * max_col;
	uint32_t vst_tk = vck_tk;
	uint32_t vck_dly_tk = vck_tk / 2;

	reg_val = FIELD_PREP(LCD_IF_JDI_PAR_CONF1_MAX_COL_Msk, max_col - 1) |
		  FIELD_PREP(LCD_IF_JDI_PAR_CONF1_MAX_LINE_Msk, max_line - 1);
	sys_write32(reg_val, base + LCDC_JDI_PAR_CONF1);

	reg_val = FIELD_PREP(LCD_IF_JDI_PAR_CONF4_HST_WIDTH_Msk, hst_tk) |
		  FIELD_PREP(LCD_IF_JDI_PAR_CONF4_HCK_WIDTH_Msk, hck_tk);
	sys_write32(reg_val, base + LCDC_JDI_PAR_CONF4);

	reg_val = FIELD_PREP(LCD_IF_JDI_PAR_CONF5_VST_WIDTH_Msk, vst_tk) |
		  FIELD_PREP(LCD_IF_JDI_PAR_CONF5_VCK_WIDTH_Msk, vck_tk);
	sys_write32(reg_val, base + LCDC_JDI_PAR_CONF5);

	reg_val = FIELD_PREP(LCD_IF_JDI_PAR_CONF6_HST_DLY_Msk, hst_dly_tk) |
		  FIELD_PREP(LCD_IF_JDI_PAR_CONF6_VCK_DLY_Msk, vck_dly_tk);
	sys_write32(reg_val, base + LCDC_JDI_PAR_CONF6);

	reg_val = FIELD_PREP(LCD_IF_JDI_PAR_CONF7_HCK_DLY_Msk, hck_dly_tk) |
		  LCD_IF_JDI_PAR_CONF7_DP_MODE;
	sys_write32(reg_val, base + LCDC_JDI_PAR_CONF7);

	reg_val = FIELD_PREP(LCD_IF_JDI_PAR_CONF8_ENB_ST_COL_Msk, config->enb_start_col) |
		  FIELD_PREP(LCD_IF_JDI_PAR_CONF8_ENB_END_COL_Msk, config->enb_end_col);
	sys_write32(reg_val, base + LCDC_JDI_PAR_CONF8);

	reg_val = FIELD_PREP(LCD_IF_JDI_PAR_CTRL_ENBPOL_Msk, config->enb_pol_invert) |
		  FIELD_PREP(LCD_IF_JDI_PAR_CTRL_HCKPOL_Msk, config->hck_pol_invert) |
		  FIELD_PREP(LCD_IF_JDI_PAR_CTRL_HSTPOL_Msk, config->hst_pol_invert) |
		  FIELD_PREP(LCD_IF_JDI_PAR_CTRL_VCKPOL_Msk, config->vck_pol_invert) |
		  FIELD_PREP(LCD_IF_JDI_PAR_CTRL_VSTPOL_Msk, config->vst_pol_invert);
	sys_write32(reg_val, base + LCDC_JDI_PAR_CTRL);

	/* Hold JDI in reset until first transfer */
	sys_clear_bits(base + LCDC_JDI_PAR_CTRL, LCD_IF_JDI_PAR_CTRL_XRST);

	/* Initialize CANVAS - full screen */
	sys_write32(0, base + LCDC_CANVAS_BG);
	sys_write32(0, base + LCDC_CANVAS_TL_POS);
	reg_val = FIELD_PREP(LCD_IF_CANVAS_BR_POS_X1_Msk, max_col - 1) |
		  FIELD_PREP(LCD_IF_CANVAS_BR_POS_Y1_Msk, max_line - 1);
	sys_write32(reg_val, base + LCDC_CANVAS_BR_POS);

	/* Initialize LAYER0 - RGB332, full screen */
	reg_val = LCD_IF_LAYER0_CONFIG_FORMAT_RGB332 |
		  FIELD_PREP(LCD_IF_LAYER0_CONFIG_ACTIVE_Msk, 1) |
		  FIELD_PREP(LCD_IF_LAYER0_CONFIG_PREFETCH_EN_Msk, 1) |
		  FIELD_PREP(LCD_IF_LAYER0_CONFIG_ALPHA_SEL_Msk, 0) |
		  FIELD_PREP(LCD_IF_LAYER0_CONFIG_ALPHA_Msk, 255) |
		  FIELD_PREP(LCD_IF_LAYER0_CONFIG_LINE_FETCH_MODE_Msk, 1) |
		  FIELD_PREP(LCD_IF_LAYER0_CONFIG_WIDTH_Msk, max_col);
	sys_write32(reg_val, base + LCDC_LAYER0_CONFIG);

	reg_val = FIELD_PREP(LCD_IF_LAYER0_BR_POS_X1_Msk, max_col - 1) |
		  FIELD_PREP(LCD_IF_LAYER0_BR_POS_Y1_Msk, max_line - 1);
	sys_write32(reg_val, base + LCDC_LAYER0_BR_POS);
	sys_write32(0, base + LCDC_LAYER0_DECOMP);

	/* Set VMirror to false (matching HAL_LCDC_LayerVMirror(false)) */
	sys_clear_bits(base + LCDC_LAYER0_CONFIG, LCD_IF_LAYER0_CONFIG_V_MIRROR_Msk);

	return 0;
}

static int sf32lb_jdi_write_data(const struct device *dev, uint16_t x, uint16_t y, uint16_t width,
				 uint16_t height, const uint8_t *data_buf, size_t len)
{
	const struct sf32lb_jdi_parallel_config *config = dev->config;
	uintptr_t base = config->base;
	uint16_t roi_x1 = x + width - 1;
	uint16_t roi_y1 = y + height - 1;
	uint32_t max_line;
	uint32_t start_line, end_line;
	uint32_t start_col, end_col;
	uint32_t reg_val;

	start_line = (config->bank_row_head + y) * 2 + 1;
	end_line = (config->bank_row_head + roi_y1 + 1) * 2;

	start_col = (config->bank_col_head + x) / 2;
	end_col = (config->bank_col_head + roi_x1 + 1) / 2 - 1;
	max_line = (config->bank_row_head + config->valid_rows + config->bank_row_tail) * 2;

	/* Background color (fill outside ROI) */
	sys_write32(0, base + LCDC_CANVAS_BG);

	/* Canvas region */
	reg_val = FIELD_PREP(LCD_IF_CANVAS_TL_POS_X0_Msk, x) |
		  FIELD_PREP(LCD_IF_CANVAS_TL_POS_Y0_Msk, y);
	sys_write32(reg_val, base + LCDC_CANVAS_TL_POS);

	reg_val = FIELD_PREP(LCD_IF_CANVAS_BR_POS_X1_Msk, roi_x1) |
		  FIELD_PREP(LCD_IF_CANVAS_BR_POS_Y1_Msk, y + height * 2 - 1);
	sys_write32(reg_val, base + LCDC_CANVAS_BR_POS);

	reg_val = LCD_IF_LAYER0_CONFIG_FORMAT_RGB332 |
		  FIELD_PREP(LCD_IF_LAYER0_CONFIG_ACTIVE_Msk, 1) |
		  FIELD_PREP(LCD_IF_LAYER0_CONFIG_PREFETCH_EN_Msk, 1) |
		  FIELD_PREP(LCD_IF_LAYER0_CONFIG_ALPHA_SEL_Msk, 0) |
		  FIELD_PREP(LCD_IF_LAYER0_CONFIG_ALPHA_Msk, 255) |
		  FIELD_PREP(LCD_IF_LAYER0_CONFIG_LINE_FETCH_MODE_Msk, 1) |
		  FIELD_PREP(LCD_IF_LAYER0_CONFIG_WIDTH_Msk, width);
	sys_write32(reg_val, base + LCDC_LAYER0_CONFIG);

	/* LAYER0 region */
	reg_val = FIELD_PREP(LCD_IF_LAYER0_TL_POS_X0_Msk, x) |
		  FIELD_PREP(LCD_IF_LAYER0_TL_POS_Y0_Msk, y);
	sys_write32(reg_val, base + LCDC_LAYER0_TL_POS);

	reg_val = FIELD_PREP(LCD_IF_LAYER0_BR_POS_X1_Msk, roi_x1) |
		  FIELD_PREP(LCD_IF_LAYER0_BR_POS_Y1_Msk, y + height * 2 - 1);
	sys_write32(reg_val, base + LCDC_LAYER0_BR_POS);

	reg_val = FIELD_PREP(LCD_IF_LAYER0_SRC_ADDR_Msk, (uint32_t)data_buf);
	sys_write32(reg_val, base + LCDC_LAYER0_SRC);

	/* B4-B6. Clear filter, fill, decompress */
	sys_write32(0, base + LCDC_LAYER0_FILTER);
	sys_write32(0, base + LCDC_LAYER0_FILL);
	sys_write32(0, base + LCDC_LAYER0_DECOMP);

	/* Start/end line numbers */
	reg_val = FIELD_PREP(LCD_IF_JDI_PAR_CONF2_ST_LINE_Msk, start_line) |
		  FIELD_PREP(LCD_IF_JDI_PAR_CONF2_END_LINE_Msk, end_line);
	sys_write32(reg_val, base + LCDC_JDI_PAR_CONF2);

	/* Start/end column numbers */
	reg_val = FIELD_PREP(LCD_IF_JDI_PAR_CONF3_ST_COL_Msk, start_col) |
		  FIELD_PREP(LCD_IF_JDI_PAR_CONF3_END_COL_Msk, end_col);
	sys_write32(reg_val, base + LCDC_JDI_PAR_CONF3);

	/* ENB active line range (1 line wider than data lines) */
	reg_val = FIELD_PREP(LCD_IF_JDI_PAR_CONF9_ENB_ST_LINE_Msk, start_line + 1) |
		  FIELD_PREP(LCD_IF_JDI_PAR_CONF9_ENB_END_LINE_Msk, end_line + 1);
	sys_write32(reg_val, base + LCDC_JDI_PAR_CONF9);

	/* HCK active line range */
	reg_val = FIELD_PREP(LCD_IF_JDI_PAR_CONF10_HC_ST_LINE_Msk, start_line) |
		  FIELD_PREP(LCD_IF_JDI_PAR_CONF10_HC_END_LINE_Msk, end_line + 2);
	sys_write32(reg_val, base + LCDC_JDI_PAR_CONF10);

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

	LOG_DBG("JDI write: (%u,%u) %ux%u", x, y, width, height);

	return 0;
}

static int sf32lb_jdi_parallel_blanking_on(const struct device *dev)
{
	ARG_UNUSED(dev);

	LOG_DBG("Turning display off");

	return 0;
}

static int sf32lb_jdi_parallel_blanking_off(const struct device *dev)
{
	const struct sf32lb_jdi_parallel_config *config = dev->config;
	int ret;

	LOG_DBG("Turning display on");

	if (config->vlcd_gpio.port) {
		if (device_is_ready(config->vlcd_gpio.port)) {
			gpio_pin_set_dt(&config->vlcd_gpio, 1);
			k_msleep(config->power_seq_delay_ms);
		}
	}

	if (config->vddp_gpio.port) {
		if (device_is_ready(config->vddp_gpio.port)) {
			gpio_pin_set_dt(&config->vddp_gpio, 1);
			k_msleep(config->power_seq_delay_ms);
		}
	}

	/* Start VCOM PWM (50% duty cycle) for JDI memory-in-pixel display */
	if (config->vcom_pwm.dev) {
		if (!pwm_is_ready_dt(&config->vcom_pwm)) {
			LOG_ERR("VCOM PWM device not ready");
			return -ENODEV;
		}
		ret = pwm_set_dt(&config->vcom_pwm, config->vcom_pwm.period,
				 config->vcom_pwm.period / 2U);
		if (ret < 0) {
			LOG_ERR("Failed to set VCOM PWM: %d", ret);
			return ret;
		}
		LOG_DBG("VCOM PWM A started: period=%u ns", config->vcom_pwm.period);
	}

	if (config->vcom_pwm_inv.dev) {
		if (!pwm_is_ready_dt(&config->vcom_pwm_inv)) {
			LOG_ERR("VCOM PWM invert device not ready");
			return -ENODEV;
		}
		ret = pwm_set_dt(&config->vcom_pwm_inv, config->vcom_pwm_inv.period,
				 config->vcom_pwm_inv.period / 2U);
		if (ret < 0) {
			LOG_ERR("Failed to set VCOM PWM invert: %d", ret);
			return ret;
		}
		LOG_DBG("VCOM PWM B (inverted) started: period=%u ns", config->vcom_pwm_inv.period);
	}

	return 0;
}

static int sf32lb_jdi_parallel_write(const struct device *dev, const uint16_t x, const uint16_t y,
				     const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct sf32lb_jdi_parallel_config *config = dev->config;

	if (x >= config->width || y >= config->height) {
		return -EINVAL;
	}

	return sf32lb_jdi_write_data(dev, x, y, desc->width, desc->height, buf, desc->buf_size);
}

static void sf32lb_jdi_parallel_get_capabilities(const struct device *dev,
						 struct display_capabilities *caps)
{
	const struct sf32lb_jdi_parallel_config *config = dev->config;

	memset(caps, 0, sizeof(*caps));

	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->current_orientation = DISPLAY_ORIENTATION_NORMAL;
	caps->screen_info = SCREEN_INFO_MONO_VTILED;
}

static DEVICE_API(display, sf32lb_jdi_parallel_api) = {
	.blanking_on = sf32lb_jdi_parallel_blanking_on,
	.blanking_off = sf32lb_jdi_parallel_blanking_off,
	.write = sf32lb_jdi_parallel_write,
	.get_capabilities = sf32lb_jdi_parallel_get_capabilities,
};

static int sf32lb_jdi_parallel_init(const struct device *dev)
{
	const struct sf32lb_jdi_parallel_config *config = dev->config;
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

	if (config->vlcd_gpio.port) {
		if (!gpio_is_ready_dt(&config->vlcd_gpio)) {
			LOG_ERR("VLCD GPIO device not ready");
			return -ENODEV;
		}
		gpio_pin_configure_dt(&config->vlcd_gpio, GPIO_OUTPUT_INACTIVE);
	}
	if (config->vddp_gpio.port) {
		if (!gpio_is_ready_dt(&config->vddp_gpio)) {
			LOG_ERR("VDDP GPIO device not ready");
			return -ENODEV;
		}
		gpio_pin_configure_dt(&config->vddp_gpio, GPIO_OUTPUT_INACTIVE);
	}

	/* Enable auto-gate and release LCD reset */
	sys_set_bits(config->base + LCDC_SETTING, LCD_IF_SETTING_AUTO_GATE_EN);
	sys_set_bits(config->base + LCDC_LCD_IF_CONF, LCD_IF_LCD_IF_CONF_LCD_RSTB);

	sf32lb_jdi_config(dev);

	config->irq_configure();

	LOG_DBG("JDI device initialized successfully");

	return 0;
}

#define SF32LB_JDI_PARALLEL_DEFINE(n)                                                              \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static void sf32lb_jdi_irq_configure_##n(void)                                             \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQN(DT_INST_PARENT(n)), DT_IRQ(DT_INST_PARENT(n), priority),       \
			    sf32lb_jdi_isr, DEVICE_DT_INST_GET(n), 0);                             \
		irq_enable(DT_IRQN(DT_INST_PARENT(n)));                                            \
	}                                                                                          \
                                                                                                   \
	static const struct sf32lb_jdi_parallel_config sf32lb_jdi_parallel_config_##n = {          \
		.base = DT_REG_ADDR(DT_INST_PARENT(n)),                                            \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.clock = SF32LB_CLOCK_DT_INST_PARENT_SPEC_GET(n),                                  \
		.irq_configure = sf32lb_jdi_irq_configure_##n,                                     \
		.width = DT_INST_PROP(n, width),                                                   \
		.height = DT_INST_PROP(n, height),                                                 \
		.freq = DT_INST_PROP(n, clock_frequency),                                          \
		.bank_col_head = DT_INST_PROP_OR(n, bank_col_head, 0),                             \
		.valid_columns = DT_INST_PROP_OR(n, valid_columns, 0),                             \
		.bank_col_tail = DT_INST_PROP_OR(n, bank_col_tail, 0),                             \
		.bank_row_head = DT_INST_PROP_OR(n, bank_row_head, 0),                             \
		.valid_rows = DT_INST_PROP_OR(n, valid_rows, 0),                                   \
		.bank_row_tail = DT_INST_PROP_OR(n, bank_row_tail, 0),                             \
		.enb_start_col = DT_INST_PROP_OR(n, enb_start_col, 0),                             \
		.enb_end_col = DT_INST_PROP_OR(n, enb_end_col, 0),                                 \
		.enb_pol_invert = DT_INST_PROP_OR(n, enb_pol_invert, 0),                           \
		.hck_pol_invert = DT_INST_PROP_OR(n, hck_pol_invert, 0),                           \
		.hst_pol_invert = DT_INST_PROP_OR(n, hst_pol_invert, 0),                           \
		.vck_pol_invert = DT_INST_PROP_OR(n, vck_pol_invert, 0),                           \
		.vst_pol_invert = DT_INST_PROP_OR(n, vst_pol_invert, 0),                           \
		.vlcd_gpio = GPIO_DT_SPEC_INST_GET_OR(n, vlcd_gpios, {0}),                         \
		.vddp_gpio = GPIO_DT_SPEC_INST_GET_OR(n, vddp_gpios, {0}),                         \
		.vcom_pwm = PWM_DT_SPEC_INST_GET_BY_IDX(n, 0),                                     \
		.vcom_pwm_inv = PWM_DT_SPEC_INST_GET_BY_IDX_OR(n, 1, {0}),                         \
		.power_seq_delay_ms = DT_INST_PROP_OR(n, power_seq_delay_ms, 11),                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, sf32lb_jdi_parallel_init, NULL, NULL,                             \
			      &sf32lb_jdi_parallel_config_##n, POST_KERNEL,                        \
			      CONFIG_DISPLAY_INIT_PRIORITY, &sf32lb_jdi_parallel_api);

DT_INST_FOREACH_STATUS_OKAY(SF32LB_JDI_PARALLEL_DEFINE)
