/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief MCUX SLCD (Segment LCD) auxdisplay driver
 *
 * # Design overview
 *
 * This driver exposes the NXP MCUXpresso SLCD peripheral as a Zephyr
 * @ref auxdisplay_interface.
 *
 * The implementation is split into two layers:
 *
 * 1. **Generic Zephyr auxdisplay layer** (this file)
 *
 *    - Implements the auxdisplay API: display on/off, clear, cursor positioning,
 *      write, and a small panel-specific `custom_command()` interface.
 *    - Maintains a small shadow framebuffer:
 *      - `digits[]` holds per-digit *encoded segment bitmasks*.
 *      - `colon_mask` is a bitmask where `BIT(n)` enables a colon at digit index
 *        `n` (limited to 8 positions, see `MCUX_SLCD_MAX_DIGITS`).
 *      - `icon_mask` is a 32-bit panel-defined bitmask for special symbols/icons.
 *    - Converts the user write stream into the internal digit buffer using the
 *      active panel's `encode_char()` callback.
 *    - Applies updates to hardware through the panel's `apply()` callback.
 *
 * 2. **Panel (glass) mapping layer** (`mcux_slcd_lcd_*.c`)
 *
 *    Different LCD glasses wire segments to SLCD frontplane pins differently.
 *    To keep the core driver generic, glass-specific logic is provided by a
 *    `struct mcux_slcd_panel_api` implementation (see `mcux_slcd_lcd.h`).
 *
 *    The panel layer is responsible for:
 *    - Mapping COM pins to SLCD phases (A..D) via `backplane_setting()`.
 *    - Encoding ASCII characters into an internal 7-seg(+DP) bitmask.
 *    - Translating `digits[]` + `colon_mask` + `icon_mask` into per-frontplane-pin
 *      phase bits and programming them with `SLCD_SetFrontPlaneSegments()`.
 *
 * ## Formatting policy implemented by write()
 *
 * - `':'` is treated as a formatting character and **does not advance the cursor**.
 *   If the active panel allows a colon at the current digit index, the
 *   corresponding `colon_mask` bit is set.
 * - `'.'` is also formatting-only:
 *   - If it appears after at least one digit, DP is attached to the previous digit.
 *   - If it appears at the start, it requests a "leading dot" on the next digit.
 *
 * # Porting guide (adding support for a new glass)
 *
 * To support a new LCD glass wiring, add a new panel implementation similar to:
 * - `mcux_slcd_lcd_od_6010.c`
 * - `mcux_slcd_lcd_s401m16kr.c`
 *
 * ## 1) Implement `struct mcux_slcd_panel_api`
 *
 * Create a new file `mcux_slcd_lcd_<your_glass>.c` that provides:
 * - `backplane_setting(base, com_pins, pin_count)`
 *   - Validate `pin_count` for your duty cycle.
 *   - Call `SLCD_SetBackPlanePhase(base, com_pins[i], kSLCD_PhaseXActivate)`.
 * - `encode_char(ch, allow_dot)`
 *   - Return the driver's internal segment mask: A..G + DP in BIT(0)..BIT(7).
 * - `apply(base, d_pins, digits, colon_mask, icon_mask, cfg)`
 *   - Build a `pin_val[]` array where each element is a phase mask (A..D).
 *   - Call `SLCD_SetFrontPlaneSegments()` for each used frontplane pin.
 *   - `icon_mask` is reserved for panel-specific special symbols/icons.
 *
 * ## 2) Match devicetree to the panel
 *
 * In your devicetree instance of `nxp,slcd`, ensure:
 * - The controller node has a single child node `panel { ... }` which describes
 *   the LCD glass wiring.
 * - The panel backend is selected by matching the panel node's `panel-name`
 *   string to `mcux_slcd_panel_api.name`.
 *   - `panel-name` does not need to be present in DTS if the panel binding
 *     provides a default (recommended, to avoid duplication with `compatible`).
 *   - If present in DTS, `panel-name` overrides the binding default.
 * - `com-pins` contains the SLCD/LCD pin indices (LCD_Pn numbers) used as COM.
 * - `d-pins` contains the frontplane SLCD/LCD pin indices used as segments.
 *   - The number of `d-pins` must be > 0 and <= the selected panel
 *     implementation's `d_pins_count` (allowing smaller wiring variants).
 * - Optional panel properties (e.g. `max-digits`, `dot-pos`, `colon-pos`,
 *   `colon-routing`, `rotate-180`) may be provided in the panel node and are passed
 *   to the panel backend via `struct mcux_slcd_panel_cfg`.
 * - `columns`/`rows` define how the auxdisplay cursor addresses digits.
 *   (The driver writes linearly across `columns * rows` cells, bounded by the
 *   panel's `max_digits` and `MCUX_SLCD_MAX_DIGITS`.)
 *
 * ## 3) Verify runtime behaviour
 *
 * - Confirm COM phase mapping is correct (wrong phase assignments typically
 *   result in ghosting or missing segments).
 * - Confirm dot/colon configuration (dot-pos/colon-pos/colon-routing) matches hardware.
 *
 * @note `colon_mask` is derived from the contents of each `write()` call (it is
 *       reset at the start of a write). `icon_mask` is persistent and may be
 *       updated via auxdisplay `custom_command()`.
 */

#define DT_DRV_COMPAT nxp_slcd

#include <zephyr/device.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/iterable_sections.h>

#include <zephyr/sys/util.h>

#include <errno.h>
#include <string.h>

#include <soc.h>

#include <fsl_clock.h>
#include <fsl_slcd.h>

#include "mcux_slcd_lcd.h"

/* SLCD panel selection is modeled as a single child node named "panel". */
#define MCUX_SLCD_PANEL_NODE(inst) DT_INST_CHILD(inst, panel)

#define MCUX_SLCD_POS_MASK_ELEM(node_id, prop, idx) BIT(DT_PROP_BY_IDX(node_id, prop, idx))

#define MCUX_SLCD_POS_MASK_OR_ZERO(node_id, prop)                                                  \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, prop),						\
		(DT_FOREACH_PROP_ELEM_SEP(node_id, prop, MCUX_SLCD_POS_MASK_ELEM, (|))), (0U))

BUILD_ASSERT(MCUX_SLCD_MAX_DIGITS <= 8,
	     "colon_mask uses 8-bit positions; increase mask width if needed");

LOG_MODULE_REGISTER(auxdisplay_mcux_slcd, CONFIG_AUXDISPLAY_LOG_LEVEL);

struct auxdisplay_mcux_slcd_config {
	LCD_Type *base;
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	struct auxdisplay_capabilities capabilities;
	slcd_duty_cycle_t duty_cycle;
	uint32_t clk_cfg_div;
	slcd_clock_prescaler_t clk_cfg_prescaler;
#if !(defined(FSL_FEATURE_SLCD_LP_CONTROL) && FSL_FEATURE_SLCD_LP_CONTROL)
	slcd_clock_src_t clk_cfg_src;
	slcd_load_adjust_t load_adjust;
#else
	slcd_regulated_voltage_trim_t voltageTrimVLL1;
	slcd_regulated_voltage_trim_t voltageTrimVLL2;
	slcd_sample_hold_t sampleHold;
#endif
	void (*irq_config_func)(const struct device *dev);
	const uint8_t *com_pins;
	const uint8_t *d_pins;
	const struct mcux_slcd_panel_cfg *panel_cfg;
	const char *panel_name;
	slcd_display_mode_t display_mode;
	slcd_lowpower_behavior low_power_behavior;
	uint8_t com_pins_cnt;
	uint8_t d_pins_cnt;
	bool clk_cfg_fastrate;
	bool low_power_wf;
};

struct auxdisplay_mcux_slcd_data {
	int16_t cursor_x;
	int16_t cursor_y;
	uint8_t digits[MCUX_SLCD_MAX_DIGITS];
	uint8_t last_applied_digits[MCUX_SLCD_MAX_DIGITS];
	uint8_t colon_mask;
	uint8_t last_applied_colon_mask;
	uint32_t icon_mask;
	uint32_t last_applied_icon_mask;
	bool enabled;
	bool last_applied_valid;
	const struct mcux_slcd_panel_api *panel;
};

static const struct mcux_slcd_panel_api *mcux_slcd_panel_lookup(const char *name)
{
	if (name == NULL) {
		return NULL;
	}

	STRUCT_SECTION_FOREACH(mcux_slcd_panel_api, entry) {
		if ((entry->name != NULL) && (strcmp(entry->name, name) == 0)) {
			return entry;
		}
	}

	return NULL;
}

#if CONFIG_AUXDISPLAY_LOG_LEVEL >= LOG_LEVEL_DBG
static void mcux_slcd_dump_slcd_cfg(const struct auxdisplay_mcux_slcd_config *cfg,
				    const slcd_config_t *slcd_cfg)
{
#if defined(FSL_FEATURE_SLCD_LP_CONTROL) && FSL_FEATURE_SLCD_LP_CONTROL
	LOG_DBG("Slcd_cfg: displayMode=%u dutyCycle=%u", (unsigned int)slcd_cfg->displayMode,
		(unsigned int)slcd_cfg->dutyCycle);
	LOG_DBG("Slcd_cfg: clkPrescaler=%u", (unsigned int)slcd_cfg->clkPrescaler);
#else
	LOG_DBG("Slcd_cfg: displayMode=%u dutyCycle=%u lowPowerBehavior=%u loadAdjust=%u",
		(unsigned int)slcd_cfg->displayMode, (unsigned int)slcd_cfg->dutyCycle,
		(unsigned int)slcd_cfg->lowPowerBehavior, (unsigned int)slcd_cfg->loadAdjust);
	if (slcd_cfg->clkConfig != NULL) {
		LOG_DBG("Slcd_cfg: clkSource=%u altClkDivider=%u clkPrescaler=%u",
			(unsigned int)slcd_cfg->clkConfig->clkSource,
			(unsigned int)slcd_cfg->clkConfig->altClkDivider,
			(unsigned int)slcd_cfg->clkConfig->clkPrescaler);
#if defined(FSL_FEATURE_SLCD_HAS_FAST_FRAME_RATE) && FSL_FEATURE_SLCD_HAS_FAST_FRAME_RATE
		LOG_DBG("Slcd_cfg: fastFrameRateEnable=%u",
			(unsigned int)slcd_cfg->clkConfig->fastFrameRateEnable);
#endif
	}
#endif

	LOG_DBG("Slcd_cfg: low_en=0x%08x high_en=0x%08x low_bp=0x%08x high_bp=0x%08x",
		slcd_cfg->slcdLowPinEnabled, slcd_cfg->slcdHighPinEnabled,
		slcd_cfg->backPlaneLowPin, slcd_cfg->backPlaneHighPin);
	LOG_DBG("Slcd_cfg: faultConfig=%p", slcd_cfg->faultConfig);

	LOG_DBG("Slcd_cfg: com_pins=");
	for (int i = 0; i < cfg->com_pins_cnt; i++) {
		LOG_DBG("	 %u", (unsigned int)cfg->com_pins[i]);
	}

	LOG_DBG("Slcd_cfg: d_pins=");
	for (int i = 0; i < cfg->d_pins_cnt; i++) {
		LOG_DBG("	 %u", (unsigned int)cfg->d_pins[i]);
	}
}

static void dump_slcd(const struct auxdisplay_mcux_slcd_config *cfg, const uint8_t *com,
		      const uint8_t *d)
{
#if !(defined(FSL_FEATURE_SLCD_LP_CONTROL) && FSL_FEATURE_SLCD_LP_CONTROL)
	LOG_DBG("GCR=0x%08x (LCDEN=%d PADSAFE=%d)\n", cfg->base->GCR,
		(cfg->base->GCR & LCD_GCR_LCDEN_MASK) != 0,
		(cfg->base->GCR & LCD_GCR_PADSAFE_MASK) != 0);
#endif

	for (int i = 0; i < cfg->com_pins_cnt; i++) {
		LOG_DBG("WF[COM%d pin %u]=0x%02x\n", i, com[i], cfg->base->WF8B[com[i]]);
	}
	for (int i = 0; i < cfg->d_pins_cnt; i++) {
		LOG_DBG("WF[D%d pin %u]=0x%02x\n", i, d[i], cfg->base->WF8B[d[i]]);
	}
}
#endif /* #if CONFIG_AUXDISPLAY_LOG_LEVEL >= LOG_LEVEL_DBG */

#if !(defined(FSL_FEATURE_SLCD_LP_CONTROL) && FSL_FEATURE_SLCD_LP_CONTROL)
static int mcux_slcd_alt_div_from_dt(uint32_t div, slcd_alt_clock_div_t *out)
{
	switch (div) {
	case 1:
		*out = kSLCD_AltClkDivFactor1;
		return 0;
	case 64:
		*out = kSLCD_AltClkDivFactor64;
		return 0;
	case 256:
		*out = kSLCD_AltClkDivFactor256;
		return 0;
	case 512:
		*out = kSLCD_AltClkDivFactor512;
		return 0;
	default:
		return -EINVAL;
	}
}
#endif /* #if !(defined(FSL_FEATURE_SLCD_LP_CONTROL) && FSL_FEATURE_SLCD_LP_CONTROL) */

static void auxdisplay_mcux_slcd_isr(const struct device *dev)
{
	const struct auxdisplay_mcux_slcd_config *cfg = dev->config;
	uint32_t status = SLCD_GetInterruptStatus(cfg->base);

	if (status != 0U) {
		SLCD_ClearInterruptStatus(cfg->base, status);
	}
}

static void mcux_slcd_apply(const struct device *dev, bool force)
{
	const struct auxdisplay_mcux_slcd_config *cfg = dev->config;
	struct auxdisplay_mcux_slcd_data *data = dev->data;

	if (!force && data->last_applied_valid &&
	    (memcmp(data->digits, data->last_applied_digits, MCUX_SLCD_MAX_DIGITS) == 0) &&
	    (data->colon_mask == data->last_applied_colon_mask) &&
	    (data->icon_mask == data->last_applied_icon_mask)) {
		return;
	}

	data->panel->apply(cfg->base, cfg->d_pins, data->digits, data->colon_mask, data->icon_mask,
			   cfg->panel_cfg);
	memcpy(data->last_applied_digits, data->digits, MCUX_SLCD_MAX_DIGITS);
	data->last_applied_colon_mask = data->colon_mask;
	data->last_applied_icon_mask = data->icon_mask;
	data->last_applied_valid = true;
}

static int auxdisplay_mcux_slcd_display_on(const struct device *dev)
{
	const struct auxdisplay_mcux_slcd_config *cfg = dev->config;
	struct auxdisplay_mcux_slcd_data *data = dev->data;

	data->enabled = true;
	SLCD_StartDisplay(cfg->base);
	/* Force an apply when enabling the display to ensure frontplane is updated. */
	mcux_slcd_apply(dev, true);
	return 0;
}

static int auxdisplay_mcux_slcd_display_off(const struct device *dev)
{
	const struct auxdisplay_mcux_slcd_config *cfg = dev->config;
	struct auxdisplay_mcux_slcd_data *data = dev->data;

	if (data->enabled) {
		SLCD_StopDisplay(cfg->base);
	}
	data->enabled = false;
	return 0;
}

static int auxdisplay_mcux_slcd_cursor_position_set(const struct device *dev,
						    enum auxdisplay_position type, int16_t x,
						    int16_t y)
{
	const struct auxdisplay_mcux_slcd_config *cfg = dev->config;
	struct auxdisplay_mcux_slcd_data *data = dev->data;
	uint32_t max_digits =
		MIN((uint32_t)data->panel->max_digits, (uint32_t)ARRAY_SIZE(data->digits));
	uint32_t max_cells;
	uint32_t cursor;

	if (type == AUXDISPLAY_POSITION_ABSOLUTE) {
		/* x/y are already absolute. */
	} else if (type == AUXDISPLAY_POSITION_RELATIVE) {
		x += data->cursor_x;
		y += data->cursor_y;
	} else {
		return -ENOTSUP;
	}

	if (x < 0 || y < 0) {
		return -EINVAL;
	}
	if (x >= cfg->capabilities.columns || y >= cfg->capabilities.rows) {
		return -EINVAL;
	}

	max_cells = MIN(max_digits,
			(uint32_t)cfg->capabilities.columns * (uint32_t)cfg->capabilities.rows);
	cursor = (uint32_t)y * (uint32_t)cfg->capabilities.columns + (uint32_t)x;
	if (cursor >= max_cells) {
		return -EINVAL;
	}

	data->cursor_x = x;
	data->cursor_y = y;
	return 0;
}

static int auxdisplay_mcux_slcd_cursor_position_get(const struct device *dev, int16_t *x,
						    int16_t *y)
{
	struct auxdisplay_mcux_slcd_data *data = dev->data;

	*x = data->cursor_x;
	*y = data->cursor_y;
	return 0;
}

static int auxdisplay_mcux_slcd_capabilities_get(const struct device *dev,
						 struct auxdisplay_capabilities *capabilities)
{
	const struct auxdisplay_mcux_slcd_config *cfg = dev->config;

	memcpy(capabilities, &cfg->capabilities, sizeof(*capabilities));
	return 0;
}

static int auxdisplay_mcux_slcd_clear(const struct device *dev)
{
	struct auxdisplay_mcux_slcd_data *data = dev->data;

	memset(data->digits, 0, sizeof(data->digits));
	data->colon_mask = 0U;
	data->icon_mask = 0U;
	data->cursor_x = 0;
	data->cursor_y = 0;

	mcux_slcd_apply(dev, false);
	return 0;
}

static inline uint32_t mcux_slcd_cursor_calc(const struct auxdisplay_mcux_slcd_data *data,
					     uint32_t columns)
{
	return (uint32_t)data->cursor_y * columns + (uint32_t)data->cursor_x;
}

static inline void mcux_slcd_cursor_clamp(struct auxdisplay_mcux_slcd_data *data, uint32_t *cursor,
					  uint32_t max_cells)
{
	/*
	 * Guard against any out-of-bounds access to digits[]. While cursor is
	 * expected to be bounded by max_cells (which is derived from max_digits),
	 * make the array bound explicit to keep static analyzers happy.
	 */
	if ((*cursor >= max_cells) || (*cursor >= ARRAY_SIZE(data->digits))) {
		data->cursor_x = 0;
		data->cursor_y = 0;
		*cursor = 0U;
	}
}

static inline bool mcux_slcd_handle_colon(uint8_t c, uint32_t cursor,
					  const struct mcux_slcd_panel_cfg *panel_cfg,
					  struct auxdisplay_mcux_slcd_data *data)
{
	if (c != ':') {
		return false;
	}

	if (((panel_cfg->colon_pos_mask & (uint8_t)BIT(cursor)) != 0U) && (cursor < 8U)) {
		data->colon_mask |= (uint8_t)BIT(cursor);
	}

	return true;
}

static inline bool mcux_slcd_handle_dot(uint8_t c, uint32_t cursor,
					const struct mcux_slcd_panel_cfg *panel_cfg,
					struct auxdisplay_mcux_slcd_data *data, bool *allow_dot)
{
	if (c != '.') {
		return false;
	}

	if (cursor == 0U) {
		if ((panel_cfg->dot_pos_mask & BIT(0)) != 0U) {
			*allow_dot = true;
		}
	} else {
		uint32_t prev = cursor - 1U;

		if ((prev < ARRAY_SIZE(data->digits)) &&
		    ((panel_cfg->dot_pos_mask & (uint8_t)BIT(prev)) != 0U)) {
			/* DP is an internal segment bit; reuse encode_char() to obtain its mask. */
			data->digits[prev] |= data->panel->encode_char(' ', true);
		}
	}

	return true;
}

static inline void mcux_slcd_cursor_advance(struct auxdisplay_mcux_slcd_data *data,
					    uint32_t columns, uint32_t rows)
{
	if (((uint32_t)data->cursor_x + 1U) < columns) {
		data->cursor_x++;
		return;
	}

	data->cursor_x = 0;
	if (((uint32_t)data->cursor_y + 1U) < rows) {
		data->cursor_y++;
	} else {
		data->cursor_y = 0;
	}
}

static int auxdisplay_mcux_slcd_write(const struct device *dev, const uint8_t *ch, uint16_t len)
{
	const struct auxdisplay_mcux_slcd_config *cfg = dev->config;
	struct auxdisplay_mcux_slcd_data *data = dev->data;
	uint32_t max_digits =
		MIN((uint32_t)cfg->panel_cfg->max_digits, (uint32_t)ARRAY_SIZE(data->digits));
	uint32_t columns = MIN((uint32_t)cfg->capabilities.columns, max_digits);
	uint32_t rows = cfg->capabilities.rows;
	uint32_t max_cells;
	/* When true, the next encoded digit gets DP set ("leading dot"). */
	bool allow_dot = false;

	/* Colon state is derived from the contents of this write() call. */
	data->colon_mask = 0U;

	if (columns == 0U || rows == 0U) {
		return -EINVAL;
	}

	max_cells = MIN(max_digits, columns * rows);

	for (uint32_t i = 0; i < len; i++) {
		uint32_t cursor = mcux_slcd_cursor_calc(data, columns);

		mcux_slcd_cursor_clamp(data, &cursor, max_cells);

		if (mcux_slcd_handle_colon(ch[i], cursor, cfg->panel_cfg, data)) {
			continue;
		}

		if (mcux_slcd_handle_dot(ch[i], cursor, cfg->panel_cfg, data, &allow_dot)) {
			continue;
		}

		data->digits[cursor] = data->panel->encode_char(ch[i], allow_dot);
		allow_dot = false;

		mcux_slcd_cursor_advance(data, columns, rows);
	}

	mcux_slcd_apply(dev, false);
#if CONFIG_AUXDISPLAY_LOG_LEVEL >= LOG_LEVEL_DBG
	dump_slcd(cfg, cfg->com_pins, cfg->d_pins);
#endif
	return 0;
}

static int auxdisplay_mcux_slcd_custom_command(const struct device *dev,
					       struct auxdisplay_custom_data *cmd)
{
	struct auxdisplay_mcux_slcd_data *data = dev->data;
	uint32_t mask;

	if (cmd == NULL) {
		return -EINVAL;
	}

	/*
	 * Future-facing icon interface:
	 *
	 * icon_mask is panel-specific (BIT(n) meaning defined by the panel backend).
	 *
	 * For now we implement a small, generic convention:
	 * - If cmd->len == 4, cmd->data points to a uint32_t icon mask.
	 * - If cmd->len == 0, cmd->options is used as the icon mask.
	 */
	if ((cmd->len == sizeof(uint32_t)) && (cmd->data != NULL)) {
		memcpy(&mask, cmd->data, sizeof(mask));
		data->icon_mask = mask;
		mcux_slcd_apply(dev, false);
		return 0;
	}

	if (cmd->len == 0U) {
		data->icon_mask = cmd->options;
		mcux_slcd_apply(dev, false);
		return 0;
	}

	return -EINVAL;
}

static DEVICE_API(auxdisplay, auxdisplay_mcux_slcd_api) = {
	.display_on = auxdisplay_mcux_slcd_display_on,
	.display_off = auxdisplay_mcux_slcd_display_off,
	.cursor_position_set = auxdisplay_mcux_slcd_cursor_position_set,
	.cursor_position_get = auxdisplay_mcux_slcd_cursor_position_get,
	.capabilities_get = auxdisplay_mcux_slcd_capabilities_get,
	.clear = auxdisplay_mcux_slcd_clear,
	.write = auxdisplay_mcux_slcd_write,
	.custom_command = auxdisplay_mcux_slcd_custom_command,
};

static int auxdisplay_mcux_slcd_init(const struct device *dev)
{
	const struct auxdisplay_mcux_slcd_config *cfg = dev->config;
	struct auxdisplay_mcux_slcd_data *data = dev->data;
	slcd_config_t slcd_cfg;
	uint32_t low_en = 0U;
	uint32_t high_en = 0U;
	uint32_t low_bp = 0U;
	uint32_t high_bp = 0U;
	int ret;

	data->panel = mcux_slcd_panel_lookup(cfg->panel_name);
	if (data->panel == NULL) {
		LOG_ERR("Unsupported panel '%s'", cfg->panel_name);
		return -EINVAL;
	}

	if (data->panel->max_digits == 0U || data->panel->d_pins_count == 0U) {
		LOG_ERR("Invalid panel '%s'", cfg->panel_name);
		return -EINVAL;
	}

	if ((data->panel->com_pins_count != 0U) &&
	    (cfg->com_pins_cnt != data->panel->com_pins_count)) {
		LOG_ERR("Unsupported com-pins count: %u (expected %u)",
			(unsigned int)cfg->com_pins_cnt, (unsigned int)data->panel->com_pins_count);
		return -EINVAL;
	}

	if ((cfg->d_pins_cnt == 0U) || (cfg->d_pins_cnt > data->panel->d_pins_count)) {
		LOG_ERR("Unsupported d-pins count: %u (expected <= %u)",
			(unsigned int)cfg->d_pins_cnt, (unsigned int)data->panel->d_pins_count);
		return -EINVAL;
	}

	/*
	 * Safety check: the compile-time digit buffer must be large enough for the
	 * selected panel.
	 */
	if (ARRAY_SIZE(data->digits) < data->panel->max_digits) {
		LOG_ERR("Digit buffer too small: %u (panel requires %u).",
			(unsigned int)ARRAY_SIZE(data->digits),
			(unsigned int)data->panel->max_digits);
		return -EINVAL;
	}

	if (cfg->pincfg != NULL) {
		ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
		if (ret != 0) {
			return ret;
		}
	}

	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(cfg->clock_dev, cfg->clock_subsys);
	if (ret != 0) {
		return ret;
	}

	uint32_t rate;
	int r = clock_control_get_rate(cfg->clock_dev, cfg->clock_subsys, &rate);

	LOG_INF("SLCD clock_control_get_rate ret=%d rate=%u", r, rate);

	SLCD_GetDefaultConfig(&slcd_cfg);

	/* 4 backplanes => default is 1/4 duty, but allow DT override */
	slcd_cfg.displayMode = cfg->display_mode;
	slcd_cfg.dutyCycle = cfg->duty_cycle;
	slcd_cfg.lowPowerBehavior = cfg->low_power_behavior;

#if defined(FSL_FEATURE_SLCD_LP_CONTROL) && FSL_FEATURE_SLCD_LP_CONTROL
	/* Low-power variants use a simplified clock model. */
	slcd_cfg.clkPrescaler = cfg->clk_cfg_prescaler;
	slcd_cfg.lowPowerWaveform = cfg->low_power_wf;
	slcd_cfg.sampleHold = cfg->sampleHold;
	slcd_cfg.voltageTrimVLL1 = cfg->voltageTrimVLL1;
	slcd_cfg.voltageTrimVLL2 = cfg->voltageTrimVLL2;
#else
	slcd_alt_clock_div_t alt_div;
	slcd_clock_config_t clk_cfg = {0};

	slcd_cfg.loadAdjust = cfg->load_adjust;
	ret = mcux_slcd_alt_div_from_dt(cfg->clk_cfg_div, &alt_div);
	if (ret != 0) {
		LOG_ERR("Unsupported clk-cfg-div %u", cfg->clk_cfg_div);
		return ret;
	}
	clk_cfg.altClkDivider = alt_div;
	clk_cfg.clkSource = cfg->clk_cfg_src;
#if defined(FSL_FEATURE_SLCD_HAS_FAST_FRAME_RATE) && FSL_FEATURE_SLCD_HAS_FAST_FRAME_RATE
	clk_cfg.fastFrameRateEnable = cfg->clk_cfg_fastrate;
#endif
	clk_cfg.clkPrescaler = cfg->clk_cfg_prescaler;
	slcd_cfg.clkConfig = &clk_cfg;
#endif /* #if defined(FSL_FEATURE_SLCD_LP_CONTROL) && FSL_FEATURE_SLCD_LP_CONTROL */

	/* Enable pins and set which ones are backplanes */
	for (size_t i = 0; i < cfg->com_pins_cnt; i++) {
		uint8_t pin = cfg->com_pins[i];

		if (pin < 32) {
			low_en |= BIT(pin);
			low_bp |= BIT(pin);
		} else {
			high_en |= BIT(pin - 32U);
			high_bp |= BIT(pin - 32U);
		}
	}
	for (size_t i = 0; i < cfg->d_pins_cnt; i++) {
		uint8_t pin = cfg->d_pins[i];

		if (pin < 32) {
			low_en |= BIT(pin);
		} else {
			high_en |= BIT(pin - 32U);
		}
	}

	slcd_cfg.slcdLowPinEnabled = low_en;
	slcd_cfg.slcdHighPinEnabled = high_en;
	slcd_cfg.backPlaneLowPin = low_bp;
	slcd_cfg.backPlaneHighPin = high_bp;
	slcd_cfg.faultConfig = NULL;

#if CONFIG_AUXDISPLAY_LOG_LEVEL >= LOG_LEVEL_DBG
	mcux_slcd_dump_slcd_cfg(cfg, &slcd_cfg);
#endif

	SLCD_Init(cfg->base, &slcd_cfg);

	if (cfg->irq_config_func != NULL) {
		cfg->irq_config_func(dev);
	}

	/* COM0..COM7 are phases A..H */
	ret = data->panel->backplane_setting(cfg->base, cfg->com_pins, cfg->com_pins_cnt);
	if (ret != 0) {
		LOG_ERR("Unsupported com_pins %u", cfg->com_pins_cnt);
		return ret;
	}

	/* Clear frontplanes */
	data->colon_mask = 0U;
	data->icon_mask = 0U;
	memset(data->digits, 0, sizeof(data->digits));
	for (size_t i = 0; i < cfg->d_pins_cnt; i++) {
		SLCD_SetFrontPlaneSegments(cfg->base, cfg->d_pins[i], 0U);
	}

	data->cursor_x = 0;
	data->cursor_y = 0;
	data->last_applied_colon_mask = 0U;
	data->last_applied_icon_mask = 0U;
	data->last_applied_valid = false;
	data->enabled = true;
	SLCD_StartDisplay(cfg->base);

	return 0;
}

#if defined(FSL_FEATURE_SLCD_LP_CONTROL) && FSL_FEATURE_SLCD_LP_CONTROL
#define AUXDISPLAY_MCUX_SLCD_INIT(n)                                                               \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static void auxdisplay_mcux_slcd_irq_config_##n(const struct device *dev);                 \
	static struct auxdisplay_mcux_slcd_data auxdisplay_mcux_slcd_data_##n;                     \
	BUILD_ASSERT((DT_INST_CHILD_NUM_STATUS_OKAY(n) == 1),                                      \
		     "nxp,slcd requires exactly one child panel node");                            \
	/* panel-name may be omitted in DTS (panel binding provides default). */                   \
	static const uint8_t auxdisplay_mcux_slcd_colon_routing_##n[] =                            \
		 COND_CODE_1(DT_NODE_HAS_PROP(MCUX_SLCD_PANEL_NODE(n), colon_routing),             \
			(DT_PROP(MCUX_SLCD_PANEL_NODE(n), colon_routing)), ({0}));                 \
	static const uint8_t auxdisplay_mcux_slcd_colon_routing_len_##n =                          \
		 (uint8_t)COND_CODE_1(DT_NODE_HAS_PROP(MCUX_SLCD_PANEL_NODE(n), colon_routing),    \
			(DT_PROP_LEN(MCUX_SLCD_PANEL_NODE(n), colon_routing)), (0));               \
	static const uint8_t auxdisplay_mcux_slcd_com_pins_##n[] =                                 \
		DT_PROP(MCUX_SLCD_PANEL_NODE(n), com_pins);                                        \
	static const uint8_t auxdisplay_mcux_slcd_d_pins_##n[] =                                   \
		DT_PROP(MCUX_SLCD_PANEL_NODE(n), d_pins);                                          \
	static const struct mcux_slcd_panel_cfg auxdisplay_mcux_slcd_panel_cfg_##n = {             \
		.com_pins = auxdisplay_mcux_slcd_com_pins_##n,                                     \
		.com_pins_cnt = ARRAY_SIZE(auxdisplay_mcux_slcd_com_pins_##n),                     \
		.d_pins = auxdisplay_mcux_slcd_d_pins_##n,                                         \
		.d_pins_cnt = ARRAY_SIZE(auxdisplay_mcux_slcd_d_pins_##n),                         \
		.max_digits = DT_PROP(MCUX_SLCD_PANEL_NODE(n), max_digits),                        \
		.dot_pos_mask =                                                                    \
			(uint8_t)MCUX_SLCD_POS_MASK_OR_ZERO(MCUX_SLCD_PANEL_NODE(n), dot_pos),     \
		.colon_pos_mask =                                                                  \
			(uint8_t)MCUX_SLCD_POS_MASK_OR_ZERO(MCUX_SLCD_PANEL_NODE(n), colon_pos),   \
		.colon_map = (auxdisplay_mcux_slcd_colon_routing_len_##n > 0U)                     \
				     ? auxdisplay_mcux_slcd_colon_routing_##n                      \
				     : NULL,                                                       \
		.colon_map_len = auxdisplay_mcux_slcd_colon_routing_len_##n,                       \
		.rotate_180 = DT_PROP_OR(MCUX_SLCD_PANEL_NODE(n), rotate_180, 0),                  \
	};                                                                                         \
	static const struct auxdisplay_mcux_slcd_config auxdisplay_mcux_slcd_config_##n = {        \
		.base = (LCD_Type *)DT_INST_REG_ADDR(n),                                           \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),              \
		.capabilities =                                                                    \
			{                                                                          \
				.columns = DT_INST_PROP(n, columns),                               \
				.rows = DT_INST_PROP(n, rows),                                     \
			},                                                                         \
		.display_mode = (slcd_display_mode_t)DT_INST_ENUM_IDX_OR(n, display_mode,          \
									 kSLCD_NormalMode),        \
		.low_power_behavior = (slcd_lowpower_behavior)DT_INST_ENUM_IDX_OR(                 \
			n, low_power_behavior, kSLCD_EnabledInWaitStop),                           \
		.duty_cycle = (slcd_duty_cycle_t)DT_INST_ENUM_IDX_OR(n, slcd_duty_cycle,           \
								     kSLCD_1Div4DutyCycle),        \
		.low_power_wf = DT_INST_PROP_OR(n, low_power_wf, 0),                               \
		.clk_cfg_prescaler =                                                               \
			(slcd_clock_prescaler_t)DT_INST_PROP_OR(n, clk_cfg_prescaler, 0),          \
		.voltageTrimVLL1 = (slcd_regulated_voltage_trim_t)DT_INST_ENUM_IDX_OR(             \
			n, voltage_trim_vll1, kSLCD_VolatgeTrimNo),                                \
		.voltageTrimVLL2 = (slcd_regulated_voltage_trim_t)DT_INST_ENUM_IDX_OR(             \
			n, voltage_trim_vll2, kSLCD_VolatgeTrimNo),                                \
		.sampleHold = (slcd_sample_hold_t)DT_INST_ENUM_IDX_OR(n, sample_hold,              \
								      kSLCD_SampleHoldNone),       \
		.irq_config_func = auxdisplay_mcux_slcd_irq_config_##n,                            \
		.com_pins = auxdisplay_mcux_slcd_com_pins_##n,                                     \
		.com_pins_cnt = ARRAY_SIZE(auxdisplay_mcux_slcd_com_pins_##n),                     \
		.d_pins = auxdisplay_mcux_slcd_d_pins_##n,                                         \
		.d_pins_cnt = ARRAY_SIZE(auxdisplay_mcux_slcd_d_pins_##n),                         \
		.panel_cfg = &auxdisplay_mcux_slcd_panel_cfg_##n,                                  \
		.panel_name = DT_PROP(MCUX_SLCD_PANEL_NODE(n), panel_name),                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, auxdisplay_mcux_slcd_init, NULL, &auxdisplay_mcux_slcd_data_##n,  \
			      &auxdisplay_mcux_slcd_config_##n, POST_KERNEL,                       \
			      CONFIG_AUXDISPLAY_INIT_PRIORITY, &auxdisplay_mcux_slcd_api);         \
	static void auxdisplay_mcux_slcd_irq_config_##n(const struct device *dev)                  \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), auxdisplay_mcux_slcd_isr,   \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}
#else
#define AUXDISPLAY_MCUX_SLCD_INIT(n)                                                               \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static void auxdisplay_mcux_slcd_irq_config_##n(const struct device *dev);                 \
	static struct auxdisplay_mcux_slcd_data auxdisplay_mcux_slcd_data_##n;                     \
	BUILD_ASSERT((DT_INST_CHILD_NUM_STATUS_OKAY(n) == 1),                                      \
		     "nxp,slcd requires exactly one child panel node");                            \
	static const uint8_t auxdisplay_mcux_slcd_colon_routing_##n[] =                            \
			COND_CODE_1(DT_NODE_HAS_PROP(MCUX_SLCD_PANEL_NODE(n), colon_routing),      \
			(DT_PROP(MCUX_SLCD_PANEL_NODE(n), colon_routing)),                         \
			({0}));                                                                    \
	static const uint8_t auxdisplay_mcux_slcd_colon_routing_len_##n =                          \
		 (uint8_t)COND_CODE_1(DT_NODE_HAS_PROP(MCUX_SLCD_PANEL_NODE(n), colon_routing),    \
		 (DT_PROP_LEN(MCUX_SLCD_PANEL_NODE(n), colon_routing)), (0));                      \
	static const uint8_t auxdisplay_mcux_slcd_com_pins_##n[] =                                 \
		DT_PROP(MCUX_SLCD_PANEL_NODE(n), com_pins);                                        \
	static const uint8_t auxdisplay_mcux_slcd_d_pins_##n[] =                                   \
		DT_PROP(MCUX_SLCD_PANEL_NODE(n), d_pins);                                          \
	static const struct mcux_slcd_panel_cfg auxdisplay_mcux_slcd_panel_cfg_##n = {             \
		.com_pins = auxdisplay_mcux_slcd_com_pins_##n,                                     \
		.com_pins_cnt = ARRAY_SIZE(auxdisplay_mcux_slcd_com_pins_##n),                     \
		.d_pins = auxdisplay_mcux_slcd_d_pins_##n,                                         \
		.d_pins_cnt = ARRAY_SIZE(auxdisplay_mcux_slcd_d_pins_##n),                         \
		.max_digits = DT_PROP(MCUX_SLCD_PANEL_NODE(n), max_digits),                        \
		.dot_pos_mask =                                                                    \
			(uint8_t)MCUX_SLCD_POS_MASK_OR_ZERO(MCUX_SLCD_PANEL_NODE(n), dot_pos),     \
		.colon_pos_mask =                                                                  \
			(uint8_t)MCUX_SLCD_POS_MASK_OR_ZERO(MCUX_SLCD_PANEL_NODE(n), colon_pos),   \
		.colon_map = (auxdisplay_mcux_slcd_colon_routing_len_##n > 0U)                     \
				     ? auxdisplay_mcux_slcd_colon_routing_##n                      \
				     : NULL,                                                       \
		.colon_map_len = auxdisplay_mcux_slcd_colon_routing_len_##n,                       \
		.rotate_180 = DT_PROP_OR(MCUX_SLCD_PANEL_NODE(n), rotate_180, 0),                  \
	};                                                                                         \
	static const struct auxdisplay_mcux_slcd_config auxdisplay_mcux_slcd_config_##n = {        \
		.base = (LCD_Type *)DT_INST_REG_ADDR(n),                                           \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),              \
		.capabilities =                                                                    \
			{                                                                          \
				.columns = DT_INST_PROP(n, columns),                               \
				.rows = DT_INST_PROP(n, rows),                                     \
			},                                                                         \
		.display_mode = (slcd_display_mode_t)DT_INST_ENUM_IDX_OR(n, display_mode,          \
									 kSLCD_NormalMode),        \
		.low_power_behavior = (slcd_lowpower_behavior)DT_INST_ENUM_IDX_OR(                 \
			n, low_power_behavior, kSLCD_EnabledInWaitStop),                           \
		.clk_cfg_div = DT_INST_PROP(n, clk_cfg_div),                                       \
		.clk_cfg_prescaler =                                                               \
			(slcd_clock_prescaler_t)DT_INST_PROP_OR(n, clk_cfg_prescaler, 0),          \
		.duty_cycle = (slcd_duty_cycle_t)DT_INST_ENUM_IDX_OR(n, slcd_duty_cycle,           \
								     kSLCD_1Div4DutyCycle),        \
		.load_adjust = (slcd_load_adjust_t)DT_INST_ENUM_IDX_OR(                            \
			n, load_adjust_mode, kSLCD_HighLoadOrSlowestClkSrc),                       \
		.clk_cfg_fastrate = DT_INST_PROP_OR(n, clk_cfg_fastrate, 0),                       \
		.clk_cfg_src = (slcd_clock_src_t)DT_INST_ENUM_IDX_OR(n, clk_cfg_src, 0),           \
		.irq_config_func = auxdisplay_mcux_slcd_irq_config_##n,                            \
		.com_pins = auxdisplay_mcux_slcd_com_pins_##n,                                     \
		.com_pins_cnt = ARRAY_SIZE(auxdisplay_mcux_slcd_com_pins_##n),                     \
		.d_pins = auxdisplay_mcux_slcd_d_pins_##n,                                         \
		.d_pins_cnt = ARRAY_SIZE(auxdisplay_mcux_slcd_d_pins_##n),                         \
		.panel_cfg = &auxdisplay_mcux_slcd_panel_cfg_##n,                                  \
		.panel_name = DT_PROP(MCUX_SLCD_PANEL_NODE(n), panel_name),                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, auxdisplay_mcux_slcd_init, NULL, &auxdisplay_mcux_slcd_data_##n,  \
			      &auxdisplay_mcux_slcd_config_##n, POST_KERNEL,                       \
			      CONFIG_AUXDISPLAY_INIT_PRIORITY, &auxdisplay_mcux_slcd_api);         \
	static void auxdisplay_mcux_slcd_irq_config_##n(const struct device *dev)                  \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), auxdisplay_mcux_slcd_isr,   \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}
#endif

DT_INST_FOREACH_STATUS_OKAY(AUXDISPLAY_MCUX_SLCD_INIT)
