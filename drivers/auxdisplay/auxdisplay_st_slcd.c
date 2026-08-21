/*
 * Copyright (c) 2026 Renato Mauro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_slcd

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/logging/log.h>
#include <stm32_ll_bus.h>
#include <zephyr/sys/time_units.h>

#include "auxdisplay_slcd_config.h"

LOG_MODULE_REGISTER(auxdisplay_st_slcd, CONFIG_AUXDISPLAY_LOG_LEVEL);

/* Immutabile driver configuration structure stored in Flash */
struct auxdisplay_st_slcd_config {
	const struct stm32_pclken *pclken;
	const struct device *clk_dev;
	const struct pinctrl_dev_config *pincfg;

	const uint8_t *pin_list;
	const uint8_t pin_list_len;
	const uint8_t com_list_len;
	const uint8_t ram_buffer_size;

	const uint8_t custom_character_slot_count;
	const uint32_t *character_bit_list;
	const uint8_t *character_com_list;
	const uint32_t *indicator_bit_list;
	const uint8_t *indicator_com_list;

	const struct auxdisplay_panel_config *panel_config;
	const int position_count;
};

/* Mutable driver runtime instance data structure stored in RAM */
struct auxdisplay_st_slcd_data {
	LCD_HandleTypeDef hlcd;

	uint16_t cursor_x;
	uint16_t cursor_y;

	uint16_t *custom_character_patterns;
	uint8_t custom_character_slot_used;

	uint32_t *ram_bits_buffers;
	uint32_t *ram_mask_buffers;
};

static int auxdisplay_st_slcd_display_on(const struct device *dev)
{
	struct auxdisplay_st_slcd_data *data = dev->data;

	__HAL_LCD_ENABLE(&data->hlcd);

	return 0;
}

static int auxdisplay_st_slcd_display_off(const struct device *dev)
{
	struct auxdisplay_st_slcd_data *data = dev->data;

	__HAL_LCD_DISABLE(&data->hlcd);

	return 0;
}

static int auxdisplay_st_slcd_cursor_position_set(const struct device *dev,
						  enum auxdisplay_position type, int16_t x,
						  int16_t y)
{
	const struct auxdisplay_st_slcd_config *config = dev->config;
	struct auxdisplay_st_slcd_data *data = dev->data;

	if (type == AUXDISPLAY_POSITION_RELATIVE_DIRECTION) {
		return -ENOTSUP;
	}

	if (type == AUXDISPLAY_POSITION_RELATIVE) {
		x += data->cursor_x;
		y += data->cursor_y;
	}

	if ((x >= config->panel_config->capabilities.columns) ||
	    (y >= config->panel_config->capabilities.rows) || (x < 0) || (y < 0)) {
		return -EINVAL;
	}

	data->cursor_x = x;
	data->cursor_y = y;

	return 0;
}

static int auxdisplay_st_slcd_cursor_position_get(const struct device *dev, int16_t *x, int16_t *y)
{
	struct auxdisplay_st_slcd_data *data = dev->data;

	*x = data->cursor_x;
	*y = data->cursor_y;

	return 0;
}

static int auxdisplay_st_slcd_capabilities_get(const struct device *dev,
					       struct auxdisplay_capabilities *capabilities)
{
	const struct auxdisplay_st_slcd_config *config = dev->config;

	if (!capabilities) {
		return -EINVAL;
	}

	memcpy(capabilities, &config->panel_config->capabilities,
	       sizeof(struct auxdisplay_capabilities));
	return 0;
}

static void st_slcd_clear_ram_buffers(const struct device *dev)
{
	struct auxdisplay_st_slcd_data *data = dev->data;
	const struct auxdisplay_st_slcd_config *config = dev->config;

	memset(data->ram_bits_buffers, 0, config->ram_buffer_size);
	memset(data->ram_mask_buffers, 0, config->ram_buffer_size);
}

static int st_slcd_clear_and_update(const struct device *dev)
{
	struct auxdisplay_st_slcd_data *data = dev->data;
	HAL_StatusTypeDef hal_ret;

	/* HAL_LCD_Clear DOES call UpdateRequest internally */
	hal_ret = HAL_LCD_Clear(&data->hlcd);
	if (hal_ret != HAL_OK) {
		LOG_ERR("HAL_LCD_Clear failed (hal ret: %d)", hal_ret);
		return -EIO;
	}

	return 0;
}

static int st_slcd_write_ram(const struct device *dev, uint32_t ram_register, uint32_t mask,
			     uint32_t bits)
{
	struct auxdisplay_st_slcd_data *dev_data = dev->data;
	HAL_StatusTypeDef hal_ret;

	hal_ret = HAL_LCD_Write(&dev_data->hlcd, ram_register, mask, bits);
	if (hal_ret != HAL_OK) {
		LOG_ERR("HAL_LCD_Write failed (hal ret: %d)", hal_ret);
		return -EIO;
	}

	return 0;
}

static int st_slcd_update_request(const struct device *dev)
{
	struct auxdisplay_st_slcd_data *data = dev->data;
	HAL_StatusTypeDef hal_ret;

	hal_ret = HAL_LCD_UpdateDisplayRequest(&data->hlcd);
	if (hal_ret != HAL_OK) {
		LOG_ERR("HAL_LCD_UpdateDisplayRequest failed (hal ret: %d)", hal_ret);
		return -EIO;
	}

	return 0;
}

static int auxdisplay_st_slcd_clear(const struct device *dev)
{
	struct auxdisplay_st_slcd_data *data = dev->data;
	int ret;

	data->cursor_x = 0;
	data->cursor_y = 0;

	ret = st_slcd_clear_and_update(dev);
	if (ret) {
		return ret;
	}

	return 0;
}

static void st_slcd_write_pattern_to_buffer(const struct device *dev, uint16_t pattern,
					    int position)
{
	const struct auxdisplay_st_slcd_config *config = dev->config;
	struct auxdisplay_st_slcd_data *data = dev->data;
	const int line_index = position * config->panel_config->segment_type;
	uint8_t com;
	uint32_t pins;

	/* Iterate over each segment bit; skip segments that are off. */
	for (uint8_t segment = 0; segment < config->panel_config->segment_type; segment++) {

		com = config->character_com_list[line_index + segment];
		pins = config->character_bit_list[line_index + segment];

		/* Clear the unused segments and set the used ones. */
		data->ram_bits_buffers[com] |= ((pattern & (1U << segment)) == 0) ? 0 : pins;
		/* Either way the segment must be updated. */
		data->ram_mask_buffers[com] |= pins;
	}
}

static int auxdisplay_st_slcd_indicator_set(const struct device *dev, uint8_t index, bool enable)
{
	const struct auxdisplay_st_slcd_config *config = dev->config;
	uint32_t pin;
	uint8_t com;
	int ret;

	if (index >= config->panel_config->num_indicators) {
		return -EINVAL;
	}

	if (config->indicator_com_list == NULL || config->indicator_bit_list == NULL) {
		return -ENOTSUP;
	}

	com = config->indicator_com_list[index];
	pin = config->indicator_bit_list[index];

	ret = st_slcd_write_ram(dev, com, ~pin, enable ? pin : 0);
	if (ret) {
		return ret;
	}

	/* Fire a hardware request telling the peripheral controller to push internal RAM
	 * data to the glass.
	 */
	ret = st_slcd_update_request(dev);
	if (ret) {
		return ret;
	}

	return 0;
}

static bool st_slcd_prepare_symbol(const struct device *dev, uint8_t ascii_char, int position)
{
	const struct auxdisplay_st_slcd_config *config = dev->config;
	struct auxdisplay_st_slcd_data *data = dev->data;
	int indicator_type;
	int indicator_index;
	uint32_t pin;
	uint8_t com;

	if (config->panel_config->display_indicators == NULL ||
	    config->indicator_com_list == NULL || config->indicator_bit_list == NULL) {
		return false;
	}

	if (ascii_char == '.') {
		indicator_type = AUXDISPLAY_SLCD_INDICATOR_TYPE_SINGLE_DOT;
	} else if (ascii_char == ':') {
		indicator_type = AUXDISPLAY_SLCD_INDICATOR_TYPE_DOUBLE_DOT;
	} else {
		/* Try to treat it as a regular character on next iteration. */
		return false;
	}

	indicator_index =
		indicator_type * config->panel_config->capabilities.columns + position - 1;
	if (config->panel_config->display_indicators[indicator_index] == 0xFF) {
		/* Nothing to do, anyway return true to consume the character */
		return true;
	}

	com = config->indicator_com_list[config->panel_config->display_indicators[indicator_index]];
	pin = config->indicator_bit_list[config->panel_config->display_indicators[indicator_index]];

	data->ram_bits_buffers[com] |= pin;
	/* Either way the segment must be updated. */
	data->ram_mask_buffers[com] |= pin;

	return true;
}

static bool st_slcd_prepare_custom_character(const struct device *dev, uint8_t ascii_char,
					     int position)
{
	struct auxdisplay_st_slcd_data *data = dev->data;

	if (ascii_char < data->custom_character_slot_used) {
		st_slcd_write_pattern_to_buffer(dev, data->custom_character_patterns[ascii_char],
						position);
		return true;
	}

	return false;
}

static void st_slcd_prepare_character(const struct device *dev, uint8_t ascii_char, int position)
{
	const struct auxdisplay_st_slcd_config *config = dev->config;
	const uint16_t pattern =
		slcd_14seg_convert_ascii_char_to_14seg_pattern(config->panel_config, ascii_char);

	st_slcd_write_pattern_to_buffer(dev, pattern, position);
}

static int st_slcd_write_buffer_to_lcd_ram(const struct device *dev)
{
	const struct auxdisplay_st_slcd_config *config = dev->config;
	struct auxdisplay_st_slcd_data *data = dev->data;
	int ret;

	for (int i = 0; i < config->com_list_len; i++) {
		/* Write to registers only if any the buffer changed */
		if (data->ram_mask_buffers[i]) {
			ret = st_slcd_write_ram(dev, i, ~data->ram_mask_buffers[i],
						data->ram_bits_buffers[i]);
			if (ret) {
				return ret;
			}
		}
	}
	return 0;
}

/* character->data must contain 14 bytes set either to 0x00 or 0xFF,
 * that is as many bytes as the segment-type.
 * character->character_code is set to the slot index, thus from 0 to custom_character_slot_count.
 */
static int auxdisplay_st_slcd_custom_character_set(const struct device *dev,
						   struct auxdisplay_character *character)
{
	const struct auxdisplay_st_slcd_config *config = dev->config;
	struct auxdisplay_st_slcd_data *data = dev->data;
	uint32_t pattern = 0;

	if (data->custom_character_slot_used >= config->custom_character_slot_count) {
		return -ENOMEM;
	}

	for (int i = 0; i < config->panel_config->segment_type; i++) {
		if (character->data[i]) {
			pattern |= 1 << i;
		}
	}

	data->custom_character_patterns[data->custom_character_slot_used] = pattern;
	character->character_code = data->custom_character_slot_used;
	data->custom_character_slot_used++;

	return 0;
}

static int auxdisplay_st_slcd_write(const struct device *dev, const uint8_t *text, uint16_t len)
{
	const struct auxdisplay_st_slcd_config *config = dev->config;
	struct auxdisplay_st_slcd_data *data = dev->data;
	const uint16_t cols = config->panel_config->capabilities.columns;
	int ret;

	st_slcd_clear_ram_buffers(dev);

	/* Loop to update segments sequentially up to the physical maximum string length
	 * restriction.
	 */
	for (int i = 0, position = data->cursor_y * cols + data->cursor_x;
	     i < len && position < config->position_count; i++) {
		const uint8_t one_char = text[i];

		/*
		 * Symbols only exist between digits, so no need to process the
		 * first character for symbol.
		 */
		if (i != 0 && st_slcd_prepare_symbol(dev, one_char, position)) {
			/* Proceed to next character without advancing the cursor.  */
			continue;
		}

		if (st_slcd_prepare_custom_character(dev, one_char, position)) {
			/* Proceed to next character advancing the cursor.  */
		} else {
			st_slcd_prepare_character(dev, one_char, position);
		}
		position++;

		if (data->cursor_x < cols - 1) {
			data->cursor_x++;
		} else if (data->cursor_y < config->panel_config->capabilities.rows - 1) {
			data->cursor_x = 0;
			data->cursor_y++;
		} else {
			/* The cursor keep being on the last available position, this text is
			 * truncated, next write operation will write one character on this
			 * position.
			 */
			break;
		}
	}

	ret = st_slcd_write_buffer_to_lcd_ram(dev);
	if (ret) {
		return ret;
	}

	/* Fire a hardware request telling the peripheral controller to push internal RAM
	 * data to the glass.
	 */
	ret = st_slcd_update_request(dev);
	if (ret) {
		return ret;
	}

	return 0;
}

/* Map implementation functions to the standard public Zephyr auxdisplay API interface. */
static DEVICE_API(auxdisplay, auxdisplay_st_slcd_auxdisplay_api) = {
	.cursor_position_set = auxdisplay_st_slcd_cursor_position_set,
	.cursor_position_get = auxdisplay_st_slcd_cursor_position_get,
	.capabilities_get = auxdisplay_st_slcd_capabilities_get,
	.display_on = auxdisplay_st_slcd_display_on,
	.display_off = auxdisplay_st_slcd_display_off,
	.clear = auxdisplay_st_slcd_clear,
	.write = auxdisplay_st_slcd_write,
	.custom_character_set = auxdisplay_st_slcd_custom_character_set,
	.custom_indicator_set = auxdisplay_st_slcd_indicator_set,
};

/* Core device initialization logic executed automatically by the Zephyr kernel boot sequencer. */
static int auxdisplay_st_slcd_init(const struct device *dev)
{
	const struct auxdisplay_st_slcd_config *config = dev->config;
	struct auxdisplay_st_slcd_data *data = dev->data;
	int ret;
	HAL_StatusTypeDef hal_ret;
	uint8_t segment;
	uint32_t bit;
	uint8_t com;

	/* Check the MCU AF11 channel values */
	for (uint32_t i = 0; i < config->pin_list_len; i++) {
		if (config->pin_list[i] >= 64) {
			LOG_ERR("Invalid pin-list entry: %u", config->pin_list[i]);
			return -EINVAL;
		}
	}

	/* Check the precompiled segment bits (shift) and COM RAM registers */
	for (uint32_t i = 0; i < config->position_count * config->panel_config->segment_type; i++) {
		segment = config->panel_config->segment_pins[i];
		if (segment >= config->pin_list_len) {
			LOG_ERR("Invalid panel segment_pins entry: %u",
				config->panel_config->segment_pins[i]);
			return -EINVAL;
		}
		segment = config->pin_list[segment];

		bit = 1 << (segment % 32);
		com = (config->panel_config->segment_coms[i] * 2) + segment / 32;

		if (bit != config->character_bit_list[i]) {
			LOG_ERR("Invalid character_bit_list entry: index segment data config %u - "
				"%d - %u - %u",
				i, segment, bit, config->character_bit_list[i]);
			return -EINVAL;
		}

		if (com != config->character_com_list[i]) {
			LOG_ERR("Invalid character_com_list entry: index segment data config %u - "
				"%d - %u - %u",
				i, segment, com, config->character_com_list[i]);
			return -EINVAL;
		}
	}

	/* Check the precompiled indicators bits (shift) and COM RAM registers */
	for (uint32_t i = 0; i < config->panel_config->num_indicators; i++) {
		segment = config->panel_config->indicator_pins[i];
		if (segment >= config->pin_list_len) {
			LOG_ERR("Invalid panel indicator_pins entry: %u",
				config->panel_config->indicator_pins[i]);
			return -EINVAL;
		}
		segment = config->pin_list[segment];

		bit = 1 << (segment % 32);
		com = (config->panel_config->indicator_coms[i] * 2) + segment / 32;

		if (bit != config->indicator_bit_list[i]) {
			LOG_ERR("Invalid indicator_bit_list entry: index segment data config %u - "
				"%d - %u - %u",
				i, segment, bit, config->indicator_bit_list[i]);
			return -EINVAL;
		}

		if (com != config->indicator_com_list[i]) {
			LOG_ERR("Invalid indicator_com_list entry: index segment data config %u - "
				"%d - %u - %u",
				i, segment, com, config->indicator_com_list[i]);
			return -EINVAL;
		}
	}

	/* Request clock gating initialization from the generic Zephyr device sub-system tree. */
	if (!device_is_ready(config->clk_dev)) {
		LOG_ERR("Clock Control driver device is not ready");
		return -ENODEV;
	}

	/* Remove the const qualifier in a maintainable way.
	 * Any other way to cast it makes Sonarqube detect a "const drop" issue.
	 */
	void *config_pclken_subsys_ptr = (clock_control_subsys_t)(uintptr_t)config->pclken;

	ret = clock_control_on(config->clk_dev, (clock_control_subsys_t)config_pclken_subsys_ptr);
	if (ret) {
		LOG_ERR("Failed to enable peripheral clock gate (err: %d)", ret);
		return ret;
	}

	/* Enforce the required pin alternate configurations natively via the Pinctrl
	 * manager framework.
	 */
	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("Failed to apply pinctrl default operational states (err: %d)", ret);
		return ret;
	}

	/* Initialize the LCD using the register boundaries configured within Devicetree and piping
	 * them into the ST HAL struct. The hardware parameters are set according to the STM32Cube
	 * peripheral specifications.
	 */
	hal_ret = HAL_LCD_Init(&data->hlcd);
	if (hal_ret != HAL_OK) {
		LOG_ERR("HAL_LCD_Init failed (hal ret: %d)", hal_ret);
		return -EIO;
	}

	/* Trigger hardware core initialization macros to start internal voltage pump generators. */
	__HAL_LCD_ENABLE(&data->hlcd);

	LOG_DBG("GH08172T driver initialized successfully with %d digits", config->position_count);
	return 0;
}

/* Advanced Devicetree generation macro mapping compile-time definitions directly to C parameters */
#define AUXDISPLAY_ST_SLCD_DEVICE(inst)                                                            \
	SLCD_PANEL_CONFIG(inst)                                                                    \
                                                                                                   \
	static uint16_t auxdisplay_st_slcd_data_custom_character_patterns##inst[DT_INST_PROP(      \
		inst, custom_character_slot_count)];                                               \
                                                                                                   \
	static uint32_t                                                                            \
		auxdisplay_st_slcd_data_ram_bits_buffers##inst[DT_INST_PROP_LEN(inst, com_list)];  \
                                                                                                   \
	static uint32_t                                                                            \
		auxdisplay_st_slcd_data_ram_mask_buffers##inst[DT_INST_PROP_LEN(inst, com_list)];  \
                                                                                                   \
	static struct auxdisplay_st_slcd_data auxdisplay_st_slcd_data_##inst = {                   \
		.hlcd.Instance = (LCD_TypeDef *)DT_INST_REG_ADDR(inst),                            \
		.hlcd.Init.Duty = DT_INST_PROP(inst, lcd_duty),                                    \
		.hlcd.Init.Bias = DT_INST_PROP(inst, lcd_bias),                                    \
		.hlcd.Init.MuxSegment = DT_INST_PROP(inst, lcd_mux_segment),                       \
		.hlcd.Init.VoltageSource = DT_INST_PROP(inst, lcd_voltage_source),                 \
		.hlcd.Init.Prescaler = DT_INST_PROP(inst, lcd_prescaler),                          \
		.hlcd.Init.Divider = DT_INST_PROP(inst, lcd_divider),                              \
		.hlcd.Init.Contrast = DT_INST_PROP(inst, lcd_contrast),                            \
		.hlcd.Init.PulseOnDuration = DT_INST_PROP(inst, lcd_pulse_on_duration),            \
		.hlcd.Init.DeadTime = DT_INST_PROP(inst, lcd_dead_time),                           \
		.hlcd.Init.HighDrive = DT_INST_PROP(inst, lcd_high_drive),                         \
		.hlcd.Init.BlinkMode = DT_INST_PROP(inst, lcd_blink_mode),                         \
		.hlcd.Init.BlinkFrequency = DT_INST_PROP(inst, lcd_blink_frequency),               \
		.custom_character_patterns =                                                       \
			auxdisplay_st_slcd_data_custom_character_patterns##inst,                   \
		.ram_bits_buffers = auxdisplay_st_slcd_data_ram_bits_buffers##inst,                \
		.ram_mask_buffers = auxdisplay_st_slcd_data_ram_mask_buffers##inst,                \
	};                                                                                         \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static const struct stm32_pclken auxdisplay_st_slcd_pclken_##inst[] = {                    \
		STM32_DT_INST_CLOCK_INFO_BY_IDX(inst, 0)};                                         \
                                                                                                   \
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(inst, pin_list),                                        \
		     "ST SLCD instance " #inst " is missing required property pin-list");          \
                                                                                                   \
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(inst, com_list),                                        \
		     "ST SLCD instance " #inst " is missing required property com-list");          \
                                                                                                   \
	static const uint8_t auxdisplay_st_slcd_pin_list_##inst[] = DT_INST_PROP(inst, pin_list);  \
                                                                                                   \
	static const uint32_t auxdisplay_st_slcd_character_bit_list_##inst[] =                     \
		DT_INST_PROP(inst, character_bit_list);                                            \
                                                                                                   \
	static const uint8_t auxdisplay_st_slcd_character_com_list_##inst[] =                      \
		DT_INST_PROP(inst, character_com_list);                                            \
                                                                                                   \
	static const uint32_t auxdisplay_st_slcd_indicator_bit_list_##inst[] =                     \
		DT_INST_PROP(inst, indicator_bit_list);                                            \
                                                                                                   \
	static const uint8_t auxdisplay_st_slcd_indicator_com_list_##inst[] =                      \
		DT_INST_PROP(inst, indicator_com_list);                                            \
                                                                                                   \
	static const struct auxdisplay_st_slcd_config auxdisplay_st_slcd_config_##inst = {         \
		.pclken = auxdisplay_st_slcd_pclken_##inst,                                        \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_IDX(inst, 0)),                     \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
		.pin_list = auxdisplay_st_slcd_pin_list_##inst,                                    \
		.pin_list_len = DT_INST_PROP_LEN(inst, pin_list),                                  \
		.com_list_len = DT_INST_PROP_LEN(inst, com_list),                                  \
		.ram_buffer_size = DT_INST_PROP_LEN(inst, com_list) * sizeof(uint32_t),            \
		.character_bit_list = auxdisplay_st_slcd_character_bit_list_##inst,                \
		.character_com_list = auxdisplay_st_slcd_character_com_list_##inst,                \
		.indicator_bit_list = auxdisplay_st_slcd_indicator_bit_list_##inst,                \
		.indicator_com_list = auxdisplay_st_slcd_indicator_com_list_##inst,                \
		.custom_character_slot_count = DT_INST_PROP(inst, custom_character_slot_count),    \
		.panel_config = &slcd_panel_config_##inst,                                         \
		.position_count = slcd_panel_config_##inst.capabilities.rows *                     \
				  slcd_panel_config_##inst.capabilities.columns,                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, auxdisplay_st_slcd_init, NULL,                                 \
			      &auxdisplay_st_slcd_data_##inst, &auxdisplay_st_slcd_config_##inst,  \
			      POST_KERNEL, CONFIG_AUXDISPLAY_INIT_PRIORITY,                        \
			      &auxdisplay_st_slcd_auxdisplay_api)

/* Parse the active devicetree layout and execute the instantiation macro for matching okay
 * targets.
 */
DT_INST_FOREACH_STATUS_OKAY(AUXDISPLAY_ST_SLCD_DEVICE)
