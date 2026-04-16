/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_slcd

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>

#include "auxdisplay_slcd_config.h"
#include "fsl_slcd.h"

LOG_MODULE_REGISTER(auxdisplay_nxp_slcd, CONFIG_AUXDISPLAY_LOG_LEVEL);

struct auxdisplay_nxp_slcd_data {
	int16_t cursor_x;
	int16_t cursor_y;
	enum auxdisplay_orientation orientation;
};

struct auxdisplay_nxp_slcd_config {
	/* Hardware controller fields */
	LCD_Type *base;
	const struct pinctrl_dev_config *pincfg;
	uint32_t duty_cycle;
	uint8_t blink_rate;
	const uint8_t *pin_list;
	const uint8_t *com_list;
	const uint8_t pin_list_len;
	const uint8_t com_list_len;
	/* Shared Panel config fields */
	struct auxdisplay_panel_config panel_config;
};

static int auxdisplay_nxp_slcd_display_on(const struct device *dev)
{
	const struct auxdisplay_nxp_slcd_config *config = dev->config;

	SLCD_StartDisplay(config->base);
	return 0;
}

static int auxdisplay_nxp_slcd_display_off(const struct device *dev)
{
	const struct auxdisplay_nxp_slcd_config *config = dev->config;

	SLCD_StopDisplay(config->base);
	return 0;
}

static int auxdisplay_nxp_slcd_clear(const struct device *dev)
{
	const struct auxdisplay_nxp_slcd_config *config = dev->config;
	struct auxdisplay_nxp_slcd_data *data = dev->data;

	data->cursor_x = 0;
	data->cursor_y = 0;

	for (uint8_t i = 0U; i < config->pin_list_len; i++) {
		SLCD_SetFrontPlaneSegments(config->base, config->pin_list[i], 0);
	}

	return 0;
}

static int auxdisplay_nxp_slcd_cursor_position_set(const struct device *dev,
						    enum auxdisplay_position type,
						    int16_t x, int16_t y)
{
	const struct auxdisplay_nxp_slcd_config *config = dev->config;
	struct auxdisplay_nxp_slcd_data *data = dev->data;

	if (type == AUXDISPLAY_POSITION_RELATIVE) {
		x += data->cursor_x;
		y += data->cursor_y;
	} else if (type == AUXDISPLAY_POSITION_RELATIVE_DIRECTION) {
		return -ENOTSUP;
	}

	if ((x >= config->panel_config.capabilities.columns) ||
		(y >= config->panel_config.capabilities.rows) || (x < 0) || (y < 0)) {
		return -EINVAL;
	}

	data->cursor_x = x;
	data->cursor_y = y;

	return 0;
}

static int auxdisplay_nxp_slcd_cursor_position_get(const struct device *dev,
						    int16_t *x, int16_t *y)
{
	struct auxdisplay_nxp_slcd_data *data = dev->data;

	*x = data->cursor_x;
	*y = data->cursor_y;

	return 0;
}

static int auxdisplay_nxp_slcd_capabilities_get(const struct device *dev,
						 struct auxdisplay_capabilities *capabilities)
{
	const struct auxdisplay_nxp_slcd_config *config = dev->config;

	memcpy(capabilities, &config->panel_config.capabilities,
	       sizeof(struct auxdisplay_capabilities));

	return 0;
}

static int auxdisplay_nxp_slcd_indicator_set(const struct device *dev, uint8_t index, bool enable)
{
	const struct auxdisplay_nxp_slcd_config *config = dev->config;
	uint8_t pin;
	uint8_t com;

	if (index >= config->panel_config.num_indicators) {
		return -EINVAL;
	}

	if (config->panel_config.indicator_pins == NULL ||
	    config->panel_config.indicator_coms == NULL) {
		return -ENOTSUP;
	}

	pin = config->pin_list[config->panel_config.indicator_pins[index]];
	com = config->panel_config.indicator_coms[index];

	SLCD_SetFrontPlaneOnePhase(config->base, pin, com, enable);

	return 0;
}

static bool auxdisplay_nxp_slcd_write_symbol(const struct device *dev, uint8_t ch)
{
	const struct auxdisplay_nxp_slcd_config *config = dev->config;
	const struct auxdisplay_nxp_slcd_data *data = dev->data;
	const uint16_t cols = config->panel_config.capabilities.columns;
	bool rotated = (data->orientation == AUXDISPLAY_ORIENTATION_ROTATED_180);
	uint8_t gap = rotated ? (uint8_t)(cols - 1u - data->cursor_x)
			      : (uint8_t)data->cursor_x - 1u;
	const uint8_t *dot_indicators = rotated
		? config->panel_config.upper_dot_indicators
		: config->panel_config.lower_dot_indicators;

	/* ':' activates the colon/dot indicator(s) at the current column gap.
	 * The display hardware may represent a colon either as a single dedicated
	 * colon indicator or as a pair of upper and lower dot indicators.
	 */
	if (ch == ':') {
		/* Prefer a dedicated colon indicator if one is configured. */
		if (config->panel_config.col_indicators &&
		    config->panel_config.col_indicators[gap] != 0xFF) {
			auxdisplay_nxp_slcd_indicator_set(
				dev, config->panel_config.col_indicators[gap], true);
		} else {
			/* Fall back to enabling both upper and lower dot indicators. */
			if (config->panel_config.upper_dot_indicators &&
			    config->panel_config.lower_dot_indicators &&
			    config->panel_config.upper_dot_indicators[gap] != 0xFF &&
			    config->panel_config.lower_dot_indicators[gap] != 0xFF) {
				auxdisplay_nxp_slcd_indicator_set(
					dev, config->panel_config.upper_dot_indicators[gap], true);
				auxdisplay_nxp_slcd_indicator_set(
					dev, config->panel_config.lower_dot_indicators[gap], true);
			}
		}
		return true;
	} else if (ch == '.') {
		if (dot_indicators && dot_indicators[gap] != 0xFF) {
			auxdisplay_nxp_slcd_indicator_set(dev, dot_indicators[gap], true);
		}
		return true;
	} else {
		return false;
	}
}

static uint16_t auxdisplay_nxp_slcd_char_to_pattern(
	const struct auxdisplay_nxp_slcd_config *config,
	uint8_t ch, bool rotated)
{
	/* All panels support digits, not all support letters. */
	if (ch >= '0' && ch <= '9') {
		return rotated
			? config->panel_config.digits_rotated_180[ch - '0']
			: config->panel_config.digits[ch - '0'];
	} else if (ch >= 'A' && ch <= 'Z' &&
		   config->panel_config.letters_upper != NULL) {
		return rotated
			? config->panel_config.letters_upper_rotated_180[ch - 'A']
			: config->panel_config.letters_upper[ch - 'A'];
	} else if (ch >= 'a' && ch <= 'z' &&
		   config->panel_config.letters_lower != NULL) {
		return rotated
			? config->panel_config.letters_lower_rotated_180[ch - 'a']
			: config->panel_config.letters_lower[ch - 'a'];
	} else {
		return SLCD_BLANK;
	}
}

static int auxdisplay_nxp_slcd_write_pattern_on_position(const struct device *dev,
							  uint16_t pattern,
							  uint16_t position)
{
	int ret = 0;
	const struct auxdisplay_nxp_slcd_config *config = dev->config;
	uint8_t pin;
	uint8_t com;

	/* Iterate over each segment bit; skip segments that are off. */
	for (uint8_t segment = 0; segment < config->panel_config.segment_type; segment++) {
		/*
		 * Look up the hardware pin and COM for this segment at this position.
		 * The panel config arrays are laid out as [position * 7 + segment].
		 */
		pin = config->pin_list[
			config->panel_config.segment_pins[position * 7 + segment]];
		com = config->panel_config.segment_coms[position * 7 + segment];

		/* Clear the unused segments and set the used ones. */
		if ((pattern & (1U << segment)) == 0) {
			SLCD_SetFrontPlaneOnePhase(config->base, pin, com, false);
		} else {
			SLCD_SetFrontPlaneOnePhase(config->base, pin, com, true);
		}
	}

	return ret;
}

static int auxdisplay_nxp_slcd_write(const struct device *dev, const uint8_t *ch, uint16_t len)
{
	const struct auxdisplay_nxp_slcd_config *config = dev->config;
	struct auxdisplay_nxp_slcd_data *data = dev->data;
	const uint16_t cols = config->panel_config.capabilities.columns;
	bool rotated = (data->orientation == AUXDISPLAY_ORIENTATION_ROTATED_180);
	int ret = 0;
	uint16_t pattern;
	uint8_t position;

	for (uint16_t i = 0; i < len; i++) {
		/*
		 * Symbols only exist between digits, so no need to process the
		 * first character for symbol.
		 */
		if (i != 0 && auxdisplay_nxp_slcd_write_symbol(dev, ch[i])) {
			/* Proceed to next character without advancing the cursor.  */
			continue;
		}

		pattern = auxdisplay_nxp_slcd_char_to_pattern(config, ch[i], rotated);

		position = data->cursor_y * cols +
			   (rotated ? (cols - 1u - data->cursor_x) : data->cursor_x);

		ret = auxdisplay_nxp_slcd_write_pattern_on_position(dev, pattern, position);
		if (ret != 0) {
			break;
		}

		if (data->cursor_x < cols - 1) {
			data->cursor_x++;
		} else if (data->cursor_y < config->panel_config.capabilities.rows - 1) {
			data->cursor_x = 0;
			data->cursor_y++;
		} else {
			break;
		}
	}

	return ret;
}

static int auxdisplay_nxp_slcd_set_orientation(const struct device *dev,
					       enum auxdisplay_orientation orientation)
{
	struct auxdisplay_nxp_slcd_data *data = dev->data;

	data->orientation = orientation;
	return 0;
}

static int auxdisplay_nxp_slcd_blink_set(const struct device *dev, bool enabled)
{
	const struct auxdisplay_nxp_slcd_config *config = dev->config;

	if (enabled) {
		SLCD_StartBlinkMode(config->base, kSLCD_BlankDisplayBlink,
				    (slcd_blink_rate_t)config->blink_rate);
	} else {
		SLCD_StopBlinkMode(config->base);
	}

	return 0;
}

static int auxdisplay_nxp_slcd_init(const struct device *dev)
{
	const struct auxdisplay_nxp_slcd_config *config = dev->config;
	slcd_config_t slcd_cfg;
	uint32_t pin_low = 0;
	uint32_t pin_high = 0;
	uint32_t com_low = 0;
	uint32_t com_high = 0;
	int ret;

	for (uint32_t i = 0; i < config->pin_list_len; i++) {
		if (config->pin_list[i] >= 64) {
			LOG_ERR("Invalid pin-list entry: %u", config->pin_list[i]);
			return -EINVAL;
		}

		if (config->pin_list[i] < 32) {
			pin_low |= BIT(config->pin_list[i]);
		} else {
			pin_high |= BIT(config->pin_list[i] - 32);
		}
	}

	for (uint32_t i = 0; i < config->com_list_len; i++) {
		if (config->com_list[i] >= 64) {
			LOG_ERR("Invalid com-list entry: %u", config->com_list[i]);
			return -EINVAL;
		}

		if (config->com_list[i] < 32) {
			com_low |= BIT(config->com_list[i]);
		} else {
			com_high |= BIT(config->com_list[i] - 32);
		}
	}

	if (((pin_low & com_low) != 0U) || ((pin_high & com_high) != 0U)) {
		LOG_ERR("Pins used in both pin-list and com-list");
		return -EINVAL;
	}

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	SLCD_GetDefaultConfig(&slcd_cfg);

	slcd_cfg.dutyCycle = (slcd_duty_cycle_t)config->duty_cycle;
	slcd_cfg.slcdLowPinEnabled = pin_low | com_low;
	slcd_cfg.slcdHighPinEnabled = pin_high | com_high;
	slcd_cfg.backPlaneLowPin = com_low;
	slcd_cfg.backPlaneHighPin = com_high;

	SLCD_Init(config->base, &slcd_cfg);

	for (uint32_t i = 0; i < config->com_list_len && i < 8; i++) {
		SLCD_SetBackPlanePhase(config->base, config->com_list[i],
			(slcd_phase_type_t)(1U << i));
	}

	SLCD_StartDisplay(config->base);

	LOG_INF("NXP SLCD initialized (com_list_len: %u, duty_cycle: %u)",
		config->com_list_len, config->duty_cycle);

	return 0;
}

static DEVICE_API(auxdisplay, auxdisplay_nxp_slcd_api) = {
	.cursor_position_set = auxdisplay_nxp_slcd_cursor_position_set,
	.cursor_position_get = auxdisplay_nxp_slcd_cursor_position_get,
	.capabilities_get = auxdisplay_nxp_slcd_capabilities_get,
	.display_on = auxdisplay_nxp_slcd_display_on,
	.display_off = auxdisplay_nxp_slcd_display_off,
	.clear = auxdisplay_nxp_slcd_clear,
	.write = auxdisplay_nxp_slcd_write,
	.custom_indicator_set = auxdisplay_nxp_slcd_indicator_set,
	.position_blinking_set_enabled = auxdisplay_nxp_slcd_blink_set,
	.set_orientation = auxdisplay_nxp_slcd_set_orientation,
};

#define NXP_SLCD_DEFINE(n)								\
	SLCD_PANEL_CONFIG(n)								\
	PINCTRL_DT_INST_DEFINE(n);							\
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(n, pin_list),				\
		     "NXP SLCD instance " #n " is missing required property pin-list"); \
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(n, com_list),				\
		     "NXP SLCD instance " #n " is missing required property com-list"); \
	static const uint8_t nxp_slcd_pin_list_##n[] =					\
		DT_INST_PROP(n, pin_list);						\
	static const uint8_t nxp_slcd_com_list_##n[] =					\
		DT_INST_PROP(n, com_list);						\
	static struct auxdisplay_nxp_slcd_data nxp_slcd_data_##n;			\
	static const struct auxdisplay_nxp_slcd_config nxp_slcd_config_##n = {		\
		.base = (LCD_Type *)DT_INST_REG_ADDR(n),				\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),				\
		.duty_cycle = (slcd_duty_cycle_t)					\
			(DT_INST_PROP_LEN(n, com_list) - 1U),				\
		.blink_rate = DT_INST_PROP(n, blink_rate),				\
		.pin_list     = nxp_slcd_pin_list_##n,					\
		.pin_list_len = DT_INST_PROP_LEN(n, pin_list),				\
		.com_list     = nxp_slcd_com_list_##n,					\
		.com_list_len = DT_INST_PROP_LEN(n, com_list),				\
		.panel_config = slcd_panel_config_##n,					\
	};										\
	DEVICE_DT_INST_DEFINE(n, &auxdisplay_nxp_slcd_init, NULL,			\
			      &nxp_slcd_data_##n, &nxp_slcd_config_##n,			\
			      POST_KERNEL, CONFIG_AUXDISPLAY_INIT_PRIORITY,		\
			      &auxdisplay_nxp_slcd_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_SLCD_DEFINE)
