/*
 * Copyright (c) 2025 Texas Instruments
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_lcd

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <string.h>

#include "auxdisplay_slcd_config.h"

LOG_MODULE_REGISTER(auxdisplay_mspm0_lcd, CONFIG_AUXDISPLAY_LOG_LEVEL);

#define DEV_CFG(dev)  ((const struct mspm0_lcd_cfg *)(dev)->config)
#define DEV_DATA(dev) ((struct mspm0_lcd_data *)(dev)->data)
#define DEV_REGS(dev) ((struct mspm0_lcd_regs *)DEVICE_MMIO_GET(dev))

struct mspm0_lcd_regs {
	uint32_t RSVD_0[512];
	volatile uint32_t PWREN;
	volatile uint32_t RSTCTL;
	uint32_t RSVD_1[3];
	volatile uint32_t STAT;
	uint32_t RSVD_2[514];
	volatile uint32_t IIDX;
	uint32_t RSVD_3[1];
	volatile uint32_t IMASK;
	uint32_t RSVD_4[1];
	volatile uint32_t RIS;
	uint32_t RSVD_5[1];
	volatile uint32_t MIS;
	uint32_t RSVD_6[1];
	volatile uint32_t ISET;
	uint32_t RSVD_7[1];
	volatile uint32_t ICLR;
	uint32_t RSVD_8[37];
	volatile uint32_t EVT_MODE;
	uint32_t RSVD_9[7];
	volatile uint32_t LCDCTL0;
	uint32_t RSVD_A[1];
	volatile uint32_t LCDBLKCTL;
	volatile uint32_t LCDMEMCTL;
	volatile uint32_t LCDVCTL;
	volatile uint32_t LCDPCTL0;
	volatile uint32_t LCDPCTL1;
	volatile uint32_t LCDPCTL2;
	volatile uint32_t LCDPCTL3;
	uint32_t RSVD_B[1];
	volatile uint32_t LCDCSSEL0;
	volatile uint32_t LCDCSSEL1;
	volatile uint32_t LCDCSSEL2;
	volatile uint32_t LCDCSSEL3;
	uint32_t RSVD_C[2];
	volatile uint8_t LCDM[64];
	volatile uint8_t LCDBM[32];
	uint32_t RSVD_D[791];
	volatile uint32_t LCDVREFCFG;
};

BUILD_ASSERT(sizeof(struct mspm0_lcd_regs) == 0x1E00U,
	     "mspm0_lcd_regs layout mismatch — verify TRM offsets");

/* PWREN */
#define LCD_PWREN_KEY    0x26000000U
#define LCD_PWREN_ENABLE BIT(0)

/* RSTCTL */
#define LCD_RSTCTL_KEY          0xB1000000U
#define LCD_RSTCTL_RESETSTKYCLR BIT(1)
#define LCD_RSTCTL_RESETASSERT  BIT(0)

/* LCDCTL0 */
#define LCD_LCDCTL0_LCDON       BIT(0)
#define LCD_LCDCTL0_LCDLP       BIT(1)
#define LCD_LCDCTL0_LCDSON      BIT(2)
#define LCD_LCDCTL0_LCDMXX_MSK  GENMASK(5, 3)
#define LCD_LCDCTL0_LCDDIVX_MSK GENMASK(15, 11)

/* LCDBLKCTL */
#define LCD_LCDBLKCTL_LCDBLKMODX_MSK GENMASK(1, 0)
#define LCD_LCDBLKCTL_LCDBLKMODX_ALL 2U
#define LCD_LCDBLKCTL_LCDBLKPREX_MSK GENMASK(4, 2)

/* LCDMEMCTL */
#define LCD_LCDMEMCTL_LCDCLRM  BIT(1)
#define LCD_LCDMEMCTL_LCDCLRBM BIT(2)

/* LCDVCTL */
#define LCD_LCDVCTL_LCDREFMODE      BIT(0)
#define LCD_LCDVCTL_LCDBIASSEL      BIT(1)
#define LCD_LCDVCTL_LCDINTBIASEN    BIT(2)
#define LCD_LCDVCTL_VLCDSEL_VDD_R33 BIT(3)
#define LCD_LCDVCTL_LCD_HP_LP       BIT(4)
#define LCD_LCDVCTL_LCDSELVDD       BIT(5)
#define LCD_LCDVCTL_LCDREFEN        BIT(6)
#define LCD_LCDVCTL_LCDCPEN         BIT(7)
#define LCD_LCDVCTL_VLCDX_MSK       GENMASK(11, 8)
#define LCD_LCDVCTL_LCDCPFSELX_MSK  GENMASK(15, 12)
#define LCD_LCDVCTL_LCDVBSTEN       BIT(24)

/* LCDVREFCFG */
#define LCD_LCDVREFCFG_ONTIME_MSK GENMASK(1, 0)

/*
 * DT voltage-source enum → LCDVCTL base bits.
 *   0: external-ref-external-divider
 *   1: external-ref-internal-divider
 *   2: avdd-internal-divider
 *   3: charge-pump-external
 *   4: charge-pump-internal-ref
 *   5: avdd-external-divider
 *   6: charge-pump-avdd
 */
static const uint32_t voltage_mode_vctl[] = {
	0,
	LCD_LCDVCTL_LCDINTBIASEN,
	LCD_LCDVCTL_VLCDSEL_VDD_R33 | LCD_LCDVCTL_LCDINTBIASEN,
	LCD_LCDVCTL_LCDCPEN,
	LCD_LCDVCTL_LCDCPEN | LCD_LCDVCTL_LCDREFEN | LCD_LCDVCTL_LCDINTBIASEN,
	LCD_LCDVCTL_LCDSELVDD,
	LCD_LCDVCTL_LCDCPEN | LCD_LCDVCTL_LCDSELVDD,
};

/* DT bias-mode enum: 0=static→0, 1=1/3-bias→0, 2=1/4-bias→LCDBIASSEL */
static const uint32_t bias_vctl[] = {0, 0, LCD_LCDVCTL_LCDBIASSEL};

struct mspm0_lcd_cfg {
	DEVICE_MMIO_ROM;
	const struct pinctrl_dev_config *pincfg;
	uint8_t mux_mode;
	uint8_t freq_divider;
	uint8_t bias_mode_idx;
	uint8_t voltage_mode_idx;
	uint8_t cp_freq_idx;
	uint8_t vref_idx;
	uint8_t vrefcfg_ontime;
	uint8_t blink_rate;
	bool high_power_drive;
	bool ref_mode_switched;
	const uint8_t *com_pins;
	uint8_t num_com_pins;
	const uint8_t *seg_pins;
	uint8_t num_seg_pins;
	struct auxdisplay_panel_config panel_config;
};

struct mspm0_lcd_data {
	DEVICE_MMIO_RAM;
	struct k_mutex lock;
	uint8_t lcdm[64];
	uint8_t com_lcdm[64];
	int16_t cursor_x;
	int16_t cursor_y;
};

static void lcdm_flush(const struct device *dev)
{
	const struct mspm0_lcd_data *data = DEV_DATA(dev);
	struct mspm0_lcd_regs *regs = DEV_REGS(dev);

	for (size_t i = 0; i < sizeof(data->lcdm); i++) {
		regs->LCDM[i] = data->lcdm[i];
	}
}

static void mspm0_lcd_set_indicator_bit(const struct mspm0_lcd_cfg *cfg,
					struct mspm0_lcd_data *data, uint8_t lcd_pin,
					uint8_t com_idx, bool enable)
{
	uint8_t idx, mask;

	if ((cfg->mux_mode <= 3)) {
		idx = lcd_pin / 2;
		mask = (lcd_pin & 1) ? (uint8_t)(BIT(com_idx) << 4) : (uint8_t)BIT(com_idx);
	} else {
		idx = lcd_pin;
		mask = (uint8_t)BIT(com_idx);
	}

	if (enable) {
		data->lcdm[idx] |= mask;
	} else {
		data->lcdm[idx] &= ~mask;
	}
}

/*
 * Handle '.' and ':' decorators inside write().
 * Caller must hold data->lock.  No lcdm_flush() here — write() does one flush.
 * Returns true if the character was consumed as a decorator.
 */
static bool mspm0_lcd_write_symbol(const struct device *dev, uint8_t ch)
{
	const struct mspm0_lcd_cfg *cfg = DEV_CFG(dev);
	struct mspm0_lcd_data *data = DEV_DATA(dev);
	const uint16_t cols = cfg->panel_config.capabilities.columns;

	if (data->cursor_x == 0) {
		return false;
	}

	uint8_t gap = cfg->panel_config.rotated ? (uint8_t)(cols - 1u - data->cursor_x)
						: (uint8_t)(data->cursor_x - 1u);

	if (ch == ':') {
		if (cfg->panel_config.col_indicators != NULL &&
		    cfg->panel_config.col_indicators[gap] != 0xFFU) {
			uint8_t idx = cfg->panel_config.col_indicators[gap];

			if (cfg->panel_config.indicator_pins != NULL &&
			    cfg->panel_config.indicator_coms != NULL) {
				mspm0_lcd_set_indicator_bit(
					cfg, data, cfg->panel_config.indicator_pins[idx],
					cfg->panel_config.indicator_coms[idx], true);
			}
		} else if (cfg->panel_config.upper_dot_indicators != NULL &&
			   cfg->panel_config.lower_dot_indicators != NULL) {
			uint8_t ui = cfg->panel_config.upper_dot_indicators[gap];
			uint8_t li = cfg->panel_config.lower_dot_indicators[gap];

			if (ui != 0xFFU && cfg->panel_config.indicator_pins != NULL) {
				mspm0_lcd_set_indicator_bit(
					cfg, data, cfg->panel_config.indicator_pins[ui],
					cfg->panel_config.indicator_coms[ui], true);
			}
			if (li != 0xFFU && cfg->panel_config.indicator_pins != NULL) {
				mspm0_lcd_set_indicator_bit(
					cfg, data, cfg->panel_config.indicator_pins[li],
					cfg->panel_config.indicator_coms[li], true);
			}
		}
		return true;
	}

	if (ch == '.') {
		const uint8_t *dot = cfg->panel_config.rotated
					     ? cfg->panel_config.upper_dot_indicators
					     : cfg->panel_config.lower_dot_indicators;

		if (dot != NULL && dot[gap] != 0xFFU && cfg->panel_config.indicator_pins != NULL) {
			uint8_t idx = dot[gap];

			mspm0_lcd_set_indicator_bit(cfg, data,
						    cfg->panel_config.indicator_pins[idx],
						    cfg->panel_config.indicator_coms[idx], true);
		}
		return true;
	}

	return false;
}

static int mspm0_lcd_init(const struct device *dev)
{
	const struct mspm0_lcd_cfg *cfg = DEV_CFG(dev);
	struct mspm0_lcd_data *data = DEV_DATA(dev);
	struct mspm0_lcd_regs *regs;
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	regs = DEV_REGS(dev);

	k_mutex_init(&data->lock);

	regs->RSTCTL = LCD_RSTCTL_KEY | LCD_RSTCTL_RESETSTKYCLR | LCD_RSTCTL_RESETASSERT;
	k_busy_wait(10);

	regs->PWREN = LCD_PWREN_KEY | LCD_PWREN_ENABLE;
	k_busy_wait(1);

	if (cfg->pincfg != NULL) {
		ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			LOG_ERR("pinctrl failed: %d", ret);
			return ret;
		}
	}

	/* Build LCDPCTL and LCDCSSEL bitmaps from COM/SEG pin lists. */
	uint32_t pctl[4] = {0};
	uint32_t cssel[4] = {0};

	for (int i = 0; i < cfg->num_com_pins; i++) {
		uint8_t p = cfg->com_pins[i];

		pctl[p / 16] |= BIT(p % 16);
		cssel[p / 16] |= BIT(p % 16);
	}
	for (int i = 0; i < cfg->num_seg_pins; i++) {
		uint8_t p = cfg->seg_pins[i];

		pctl[p / 16] |= BIT(p % 16);
	}

	regs->LCDPCTL0 = pctl[0];
	regs->LCDPCTL1 = pctl[1];
	regs->LCDPCTL2 = pctl[2];
	regs->LCDPCTL3 = pctl[3];

	regs->LCDCSSEL0 = cssel[0];
	regs->LCDCSSEL1 = cssel[1];
	regs->LCDCSSEL2 = cssel[2];
	regs->LCDCSSEL3 = cssel[3];

	/* LCDCTL0: mux + divider + low-power waveform (always on for 1/3-bias and static).
	 * LCDLP is only valid when LCDBIASSEL=0 (bias_mode_idx != 2); hardware ignores it
	 * otherwise, but we set it unconditionally for those modes as it is always beneficial.
	 */
	uint32_t ctl0 = FIELD_PREP(LCD_LCDCTL0_LCDMXX_MSK, cfg->mux_mode) |
			FIELD_PREP(LCD_LCDCTL0_LCDDIVX_MSK, cfg->freq_divider);

	if (cfg->bias_mode_idx != 2) {
		ctl0 |= LCD_LCDCTL0_LCDLP;
	}
	regs->LCDCTL0 = ctl0;

	/* LCDVCTL: voltage mode + bias + optional modifier bits. */
	uint32_t vctl = voltage_mode_vctl[cfg->voltage_mode_idx] | bias_vctl[cfg->bias_mode_idx];

	if (cfg->high_power_drive) {
		vctl |= LCD_LCDVCTL_LCD_HP_LP;
	}
	if (cfg->ref_mode_switched) {
		vctl |= LCD_LCDVCTL_LCDREFMODE;
	}
	if (IS_ENABLED(CONFIG_AUXDISPLAY_MSPM0_LCD_VOLTAGE_BOOST)) {
		vctl |= LCD_LCDVCTL_LCDVBSTEN;
	}
	if (vctl & LCD_LCDVCTL_LCDCPEN) {
		vctl |= FIELD_PREP(LCD_LCDVCTL_LCDCPFSELX_MSK, cfg->cp_freq_idx);
	}
	if (cfg->voltage_mode_idx == 4) {
		vctl |= FIELD_PREP(LCD_LCDVCTL_VLCDX_MSK, cfg->vref_idx);
	}
	regs->LCDVCTL = vctl;

	/* LCDVREFCFG: only applicable for charge-pump-internal-ref in switched mode. */
	if (cfg->voltage_mode_idx == 4 && cfg->ref_mode_switched) {
		regs->LCDVREFCFG = FIELD_PREP(LCD_LCDVREFCFG_ONTIME_MSK, cfg->vrefcfg_ontime);
	}

	/* LCDBLKCTL: prescaler pre-loaded, blinking mode disabled. */
	regs->LCDBLKCTL = FIELD_PREP(LCD_LCDBLKCTL_LCDBLKPREX_MSK, cfg->blink_rate);

	/* Clear both LCD memories. LCDCLRM/LCDCLRBM self-clear when done; wait
	 * for them before writing display data (a few LFCLK cycles).
	 */
	regs->LCDMEMCTL = LCD_LCDMEMCTL_LCDCLRM | LCD_LCDMEMCTL_LCDCLRBM;
	for (int i = 0;
	     i < 1000 && (regs->LCDMEMCTL & (LCD_LCDMEMCTL_LCDCLRM | LCD_LCDMEMCTL_LCDCLRBM));
	     i++) {
		k_busy_wait(1);
	}

	/*
	 * Build COM self-identification bits in the shadow buffer, then flush
	 * once.  Hardware LCDM was just cleared, so no read-back is needed.
	 *   mux_mode <= 3 (mux 1–4): two pins per byte, nibble addressing.
	 *   otherwise  (mux 5–8): one pin per byte, full byte addressing.
	 */
	memset(data->lcdm, 0, sizeof(data->lcdm));
	for (int i = 0; i < cfg->num_com_pins; i++) {
		uint8_t p = cfg->com_pins[i];
		uint8_t com_val = (uint8_t)BIT(i);

		if ((cfg->mux_mode <= 3)) {
			data->lcdm[p / 2] |= (p & 1) ? (uint8_t)(com_val << 4) : com_val;
		} else {
			data->lcdm[p] = com_val;
		}
	}

	/* Snapshot COM state; used by clear() to restore COM bits. */
	memcpy(data->com_lcdm, data->lcdm, sizeof(data->lcdm));
	lcdm_flush(dev);

	/* Enable LCD with segments on. */
	ctl0 |= LCD_LCDCTL0_LCDSON | LCD_LCDCTL0_LCDON;
	regs->LCDCTL0 = ctl0;

	LOG_INF("MSPM0 LCD init: mux=%d, %d COM, %d SEG, %d pos, %d-seg panel", cfg->mux_mode + 1,
		cfg->num_com_pins, cfg->num_seg_pins, (int)cfg->panel_config.capabilities.columns,
		(int)cfg->panel_config.segment_type);
	return 0;
}

static void mspm0_lcd_write_pattern(const struct mspm0_lcd_cfg *cfg, struct mspm0_lcd_data *data,
				    uint16_t pattern, uint16_t position)
{
	const uint8_t seg_type = cfg->panel_config.segment_type;
	const uint8_t *seg_pins = cfg->panel_config.segment_pins;
	const uint8_t *seg_coms = cfg->panel_config.segment_coms;

	for (uint8_t seg = 0; seg < seg_type; seg++) {
		uint8_t pin = seg_pins[position * seg_type + seg];
		uint8_t com = seg_coms[position * seg_type + seg];
		uint8_t idx, mask;

		if ((cfg->mux_mode <= 3)) {
			idx = pin / 2;
			mask = (pin & 1) ? (uint8_t)(BIT(com) << 4) : (uint8_t)BIT(com);
		} else {
			idx = pin;
			mask = (uint8_t)BIT(com);
		}

		if (pattern & BIT(seg)) {
			data->lcdm[idx] |= mask;
		} else {
			data->lcdm[idx] &= ~mask;
		}
	}
}

static int mspm0_lcd_write(const struct device *dev, const uint8_t *text, uint16_t len)
{
	const struct mspm0_lcd_cfg *cfg = DEV_CFG(dev);
	struct mspm0_lcd_data *data = DEV_DATA(dev);
	const uint16_t cols = cfg->panel_config.capabilities.columns;
	const uint16_t rows = cfg->panel_config.capabilities.rows;

	if (cols == 0) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	for (uint16_t i = 0; i < len; i++) {
		uint8_t ch = text[i];

		if (i != 0 && mspm0_lcd_write_symbol(dev, ch)) {
			continue;
		}

		if (data->cursor_x >= (int16_t)cols) {
			break;
		}

		uint16_t pattern = slcd_char_to_pattern(&cfg->panel_config, ch);
		uint16_t position = (uint16_t)(data->cursor_y * cols + data->cursor_x);

		mspm0_lcd_write_pattern(cfg, data, pattern, position);

		/* Advance cursor with row wrap. */
		data->cursor_x++;
		if (data->cursor_x >= (int16_t)cols && data->cursor_y < (int16_t)(rows - 1)) {
			data->cursor_x = 0;
			data->cursor_y++;
		}
	}

	lcdm_flush(dev);
	k_mutex_unlock(&data->lock);
	return 0;
}

static int mspm0_lcd_clear(const struct device *dev)
{
	struct mspm0_lcd_data *data = DEV_DATA(dev);

	k_mutex_lock(&data->lock, K_FOREVER);
	memcpy(data->lcdm, data->com_lcdm, sizeof(data->lcdm));
	data->cursor_x = 0;
	data->cursor_y = 0;
	lcdm_flush(dev);
	k_mutex_unlock(&data->lock);
	return 0;
}

static int mspm0_lcd_display_on(const struct device *dev)
{
	struct mspm0_lcd_regs *regs = DEV_REGS(dev);

	k_mutex_lock(&DEV_DATA(dev)->lock, K_FOREVER);
	regs->LCDCTL0 |= LCD_LCDCTL0_LCDON | LCD_LCDCTL0_LCDSON;
	k_mutex_unlock(&DEV_DATA(dev)->lock);
	return 0;
}

static int mspm0_lcd_display_off(const struct device *dev)
{
	struct mspm0_lcd_regs *regs = DEV_REGS(dev);

	k_mutex_lock(&DEV_DATA(dev)->lock, K_FOREVER);
	regs->LCDCTL0 &= ~(LCD_LCDCTL0_LCDON | LCD_LCDCTL0_LCDSON);
	k_mutex_unlock(&DEV_DATA(dev)->lock);
	return 0;
}

static int mspm0_lcd_cursor_position_set(const struct device *dev, enum auxdisplay_position type,
					 int16_t x, int16_t y)
{
	const struct mspm0_lcd_cfg *cfg = DEV_CFG(dev);
	struct mspm0_lcd_data *data = DEV_DATA(dev);
	const int16_t max_x = (int16_t)(cfg->panel_config.capabilities.columns - 1);
	const int16_t max_y = (int16_t)(cfg->panel_config.capabilities.rows - 1);
	int ret = 0;

	if (type == AUXDISPLAY_POSITION_RELATIVE_DIRECTION) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (type == AUXDISPLAY_POSITION_RELATIVE) {
		x += data->cursor_x;
		y += data->cursor_y;
	}

	if (x < 0 || x > max_x || y < 0 || y > max_y) {
		ret = -EINVAL;
		goto out;
	}

	data->cursor_x = x;
	data->cursor_y = y;
out:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int mspm0_lcd_cursor_position_get(const struct device *dev, int16_t *x, int16_t *y)
{
	struct mspm0_lcd_data *data = DEV_DATA(dev);

	k_mutex_lock(&data->lock, K_FOREVER);
	*x = data->cursor_x;
	*y = data->cursor_y;
	k_mutex_unlock(&data->lock);
	return 0;
}

static int mspm0_lcd_capabilities_get(const struct device *dev,
				      struct auxdisplay_capabilities *capabilities)
{
	const struct mspm0_lcd_cfg *cfg = DEV_CFG(dev);

	memcpy(capabilities, &cfg->panel_config.capabilities, sizeof(*capabilities));
	return 0;
}

static int mspm0_lcd_custom_indicator_set(const struct device *dev, uint8_t index, bool enable)
{
	const struct mspm0_lcd_cfg *cfg = DEV_CFG(dev);
	struct mspm0_lcd_data *data = DEV_DATA(dev);

	if (index >= cfg->panel_config.num_indicators) {
		return -EINVAL;
	}
	if (cfg->panel_config.indicator_pins == NULL || cfg->panel_config.indicator_coms == NULL) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	mspm0_lcd_set_indicator_bit(cfg, data, cfg->panel_config.indicator_pins[index],
				    cfg->panel_config.indicator_coms[index], enable);
	lcdm_flush(dev);
	k_mutex_unlock(&data->lock);
	return 0;
}

static int mspm0_lcd_blinking_set_enabled(const struct device *dev, bool enabled)
{
	const struct mspm0_lcd_cfg *cfg = DEV_CFG(dev);
	struct mspm0_lcd_regs *regs = DEV_REGS(dev);

	/* Hardware blinking is only supported for mux modes 0-3 (mux_mode <= 3). */
	if (!(cfg->mux_mode <= 3)) {
		return -ENOTSUP;
	}

	k_mutex_lock(&DEV_DATA(dev)->lock, K_FOREVER);
	regs->LCDBLKCTL =
		FIELD_PREP(LCD_LCDBLKCTL_LCDBLKPREX_MSK, cfg->blink_rate) |
		(enabled ? FIELD_PREP(LCD_LCDBLKCTL_LCDBLKMODX_MSK, LCD_LCDBLKCTL_LCDBLKMODX_ALL)
			 : 0U);
	k_mutex_unlock(&DEV_DATA(dev)->lock);
	return 0;
}

static DEVICE_API(auxdisplay, mspm0_lcd_api) = {
	.write = mspm0_lcd_write,
	.clear = mspm0_lcd_clear,
	.display_on = mspm0_lcd_display_on,
	.display_off = mspm0_lcd_display_off,
	.cursor_position_set = mspm0_lcd_cursor_position_set,
	.cursor_position_get = mspm0_lcd_cursor_position_get,
	.capabilities_get = mspm0_lcd_capabilities_get,
	.custom_indicator_set = mspm0_lcd_custom_indicator_set,
	.position_blinking_set_enabled = mspm0_lcd_blinking_set_enabled,
};

#define MSPM0_LCD_PINCTRL_DEFINE(n)                                                                \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, pinctrl_0),			\
		    (PINCTRL_DT_INST_DEFINE(n);), ())

#define MSPM0_LCD_PINCFG(n)                                                                        \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, pinctrl_0),			\
		    (PINCTRL_DT_INST_DEV_CONFIG_GET(n)), (NULL))

#define MSPM0_LCD_INIT(n)                                                                          \
	BUILD_ASSERT(DT_INST_PROP(n, frequency_divider) <= 31U, "frequency-divider must be 0-31"); \
	MSPM0_LCD_PINCTRL_DEFINE(n)                                                                \
	SLCD_PANEL_CONFIG(n)                                                                       \
                                                                                                   \
	static const uint8_t mspm0_lcd_com_pins_##n[] = DT_INST_PROP(n, lcd_com_pins);             \
	static const uint8_t mspm0_lcd_seg_pins_##n[] = DT_INST_PROP(n, lcd_seg_pins);             \
                                                                                                   \
	static const struct mspm0_lcd_cfg mspm0_lcd_cfg_##n = {                                    \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.pincfg = MSPM0_LCD_PINCFG(n),                                                     \
		.mux_mode = DT_INST_PROP(n, mux_mode),                                             \
		.freq_divider = DT_INST_PROP(n, frequency_divider),                                \
		.high_power_drive = DT_INST_PROP(n, high_power_drive),                             \
		.ref_mode_switched = DT_INST_PROP(n, reference_mode_switched),                     \
		.bias_mode_idx = DT_INST_ENUM_IDX(n, bias_mode),                                   \
		.voltage_mode_idx = DT_INST_ENUM_IDX(n, voltage_source),                           \
		.cp_freq_idx = DT_INST_ENUM_IDX(n, charge_pump_frequency),                         \
		.vref_idx = DT_INST_ENUM_IDX_OR(n, internal_reference_voltage, 0),                 \
		.vrefcfg_ontime = DT_INST_ENUM_IDX(n, internal_reference_ontime),                  \
		.blink_rate = DT_INST_PROP(n, blink_rate),                                         \
		.com_pins = mspm0_lcd_com_pins_##n,                                                \
		.num_com_pins = DT_INST_PROP_LEN(n, lcd_com_pins),                                 \
		.seg_pins = mspm0_lcd_seg_pins_##n,                                                \
		.num_seg_pins = DT_INST_PROP_LEN(n, lcd_seg_pins),                                 \
		.panel_config = slcd_panel_config_##n,                                             \
	};                                                                                         \
                                                                                                   \
	static struct mspm0_lcd_data mspm0_lcd_data_##n;                                           \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, mspm0_lcd_init, NULL, &mspm0_lcd_data_##n, &mspm0_lcd_cfg_##n,    \
			      POST_KERNEL, CONFIG_AUXDISPLAY_INIT_PRIORITY, &mspm0_lcd_api);

DT_INST_FOREACH_STATUS_OKAY(MSPM0_LCD_INIT)
