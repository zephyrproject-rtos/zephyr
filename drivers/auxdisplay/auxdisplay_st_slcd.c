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

/* This driver is for stm32l476g_discovery REVB or REVC only,
 * thus REVA is not supported.
 * The LCD display is GH08172T.
 */

LOG_MODULE_REGISTER(auxdisplay_st_slcd, CONFIG_AUXDISPLAY_LOG_LEVEL);

/* ST modules to refer for definitions:
 * - /deps/modules/hal/stm32/stm32cube/stm32l4xx/soc/stm32l4xx.h
 * - /deps/modules/hal/stm32/stm32cube/stm32l4xx/soc/stm32l476xx.h
 * - /deps/modules/hal/stm32/stm32cube/stm32l4xx/drivers/include/stm32l4xx_ll_bus.h
 * - /deps/modules/hal/stm32/stm32cube/stm32l4xx/drivers/include/stm32l4xx_hal_lcd.h
 * - /deps/modules/hal/stm32/stm32cube/stm32l4xx/drivers/src/stm32l4xx_hal_lcd.c
 */

/* Supported characters
 *
 * ASCII characters:
 *    - numbers:                0..9
 *    - uppercase letters:      A..Z
 *    - operators:              +  -  *  /
 *    - symbols:                Space (' ': 0x20)  _  (  )
 *    - indicators (pos 1..4):  . (dot)  : (double dot)
 *
 * Not ASCII indicators:
 *    - pos 1..4:               Triple dot (both dot and double dot)
 *    - pos 6:                  4 independent bars
 *
 * Custom characters (see sample auxdisplay_14seg):
 *    - unit prefixes:          d  c  m  u  n
 *    - other symbols:          Degree  Low ring  Full
 *
 *
 * GPIO port to LCD pin to LCD channel, sorted by GPIO port:
 *
 *    GPIO   |   LCD   |  AF11
 *  port pin |   pin   |  channel
 *  ---------|---------|
 *    A06    |  SEG23  |  LCD_SEG3
 *    A07    |  SEG00  |  LCD_SEG4
 *    A08    |  COM00  |  LCD_COM0
 *    A09    |  COM01  |  LCD_COM1
 *    A10    |  COM02  |  LCD_COM2
 *    A15    |  SEG10  |  LCD_SEG17
 *    B00    |  SEG21  |  LCD_SEG5
 *    B01    |  SEG02  |  LCD_SEG6
 *    B04    |  SEG11  |  LCD_SEG8
 *    B05    |  SEG12  |  LCD_SEG9
 *    B09    |  COM03  |  LCD_COM3
 *    B12    |  SEG20  |  LCD_SEG12
 *    B13    |  SEG03  |  LCD_SEG13
 *    B14    |  SEG19  |  LCD_SEG14
 *    B15    |  SEG04  |  LCD_SEG15
 *    C03    |  VLCD   |    *
 *    C04    |  SEG22  |  LCD_SEG22
 *    C05    |  SEG01  |  LCD_SEG23
 *    C06    |  SEG14  |  LCD_SEG24
 *    C07    |  SEG09  |  LCD_SEG25
 *    C08    |  SEG13  |  LCD_SEG26
 *    D08    |  SEG18  |  LCD_SEG28
 *    D09    |  SEG05  |  LCD_SEG29
 *    D10    |  SEG17  |  LCD_SEG30
 *    D11    |  SEG06  |  LCD_SEG31
 *    D12    |  SEG16  |  LCD_SEG32
 *    D13    |  SEG07  |  LCD_SEG33
 *    D14    |  SEG15  |  LCD_SEG34
 *    D15    |  SEG08  |  LCD_SEG35
 *
 *  C03 is not connected to an LCD pin, but has to be activated as AF11 to make
 *  sure the LCD is well driven by the MCU LCD controller.
 *
 *  GLASS LCD MAPPING
 *  The LCD has six 14-segment digits with point/colon and 4 bars:
 *
 *    1       2       3       4       5       6
 *  -----   -----   -----   -----   -----   -----
 *  |\|/| o |\|/| o |\|/| o |\|/| o |\|/|   |\|/|   BAR3
 *  -- --   -- --   -- --   -- --   -- --   -- --   BAR2
 *  |/|\| o |/|\| o |/|\| o |/|\| o |/|\|   |/|\|   BAR1
 *  ----- * ----- * ----- * ----- * -----   -----   BAR0
 *
 *  LCD segment mapping:
 *
 *   Pos 1..6     Pos 1..4                 Pos 6
 *
 *  -----A-----
 *  |\   |   /|        _                  _______
 *  F H  J  K B   COL |_|           BAR3 |_______|  (same as pos 5 COL)
 *  |  \ | /  |                           _______
 *  --G-- --M--        _            BAR2 |_______|  (some as pos 5 DP)
 *  |  / | \  |   COL |_|                 _______
 *  E Q  P  N C                     BAR1 |_______|  (same as pos 6 COL)
 *  |/   |   \|        _                  _______
 *  -----D-----   DP  |_|           BAR0 |_______|  (some as pos 6 DP)
 *
 *  Summary for all positions
 *  LCD pin to LCD crystal, sorted by LCD pin:
 *
 *  | LCD   | LCD | COM3 | COM2 | COM1 | COM0 |
 *  | Pin   | Pin |      |      |      |      |
 *  | name  |     |      |      |      |      |
 *  |       |     |      |      |      |      |
 *  -------------------------------------------
 *  | SEG0  |   1 |  1N  |  1P  |  1D  |  1E  |
 *  | SEG1  |   2 | 1DP  | 1COL |  1C  |  1M  |
 *  | SEG2  |   3 |  2N  |  2P  |  2D  |  2E  |
 *  | SEG3  |   4 | 2DP  | 2COL |  2C  |  2M  |
 *  | SEG4  |   5 |  3N  |  3P  |  3D  |  3E  |
 *  | SEG5  |   6 | 3DP  | 3COL |  3C  |  3M  |
 *  | SEG6  |   7 |  4N  |  4P  |  4D  |  4E  |
 *  | SEG7  |   8 | 4DP  | 4COL |  4C  |  4M  |
 *  | SEG8  |   9 |  5N  |  5P  |  5D  |  5E  |
 *  | SEG9  |  10 | BAR2 | BAR3 |  5C  |  5M  |
 *  | SEG10 |  11 |  6N  |  6P  |  6D  |  6E  |
 *  | SEG11 |  12 | BAR0 | BAR1 |  6C  |  6M  |
 *  | COM3  |  13 | COM3 |      |      |      |
 *  | COM2  |  14 |      | COM2 |      |      |
 *  | COM1  |  15 |      |      | COM1 |      |
 *  | COM0  |  16 |      |      |      | COM0 |
 *  | SEG12 |  17 |  6J  |  6K  |  6A  |  6B  |
 *  | SEG13 |  18 |  6H  |  6Q  |  6F  |  6G  |
 *  | SEG14 |  19 |  5J  |  5K  |  5A  |  5B  |
 *  | SEG15 |  20 |  5H  |  5Q  |  5F  |  5G  |
 *  | SEG16 |  21 |  4J  |  4K  |  4A  |  4B  |
 *  | SEG17 |  22 |  4H  |  4Q  |  4F  |  4G  |
 *  | SEG18 |  23 |  3J  |  3K  |  3A  |  3B  |
 *  | SEG19 |  24 |  3H  |  3Q  |  3F  |  3G  |
 *  | SEG20 |  25 |  2J  |  2K  |  2A  |  2B  |
 *  | SEG21 |  26 |  2H  |  2Q  |  2F  |  2G  |
 *  | SEG22 |  27 |  1J  |  1K  |  1A  |  1B  |
 *  | SEG23 |  28 |  1H  |  1Q  |  1F  |  1G  |
 *
 *  Truly it's more complicated, because each COM is able to drive
 *  more than 32 bits; so, if bits 0-31 are driven by COM0, bits
 *  32-63 (actually, in ST sheets, 38 for segments and 43 for shifts)
 *  are driven by a second register, named COM0_1. This means that
 *  a logical 63 bit set is handled via two 32 bit sets driven by
 *  two registers. This is rapresented in the ST manual by MFU AF11
 *  channel having a name whose number is greater than 31.
 *  This happens for positions 4 and 5 only, for SEG 7, 8, 15 and 16
 *  for channels 33, 35, 34 and 32.
 *
 *    COM   0-31  32-63
 *     0      0     1
 *     1      2     3
 *     2      4     5
 *     3      6     7
 *
 *  | LCD   |  GPIO |  LCD  | LCD |    MCU    | Bit  | RAM      | Bit  |
 *  | Pin   |  port |  Pin  | Pin |    AF11   | posi | regi     | posi |
 *  |       |  pin  |  name | name|  channel  | tion | ster     | tion |
 *  |       |       |       | id  |           |      |          | % 32 |
 *  --------------------------------------------------------------------
 *  |   1   |  A07  | SEG00 | 00  | LCD_SEG4  |   4  | even     |   4  |
 *  |   2   |  C05  | SEG01 | 01  | LCD_SEG23 |  23  | even     |  23  |
 *  |   3   |  B01  | SEG02 | 02  | LCD_SEG6  |   6  | even     |   6  |
 *  |   4   |  B13  | SEG03 | 03  | LCD_SEG13 |  13  | even     |  13  |
 *  |   5   |  B15  | SEG04 | 04  | LCD_SEG15 |  15  | even     |  15  |
 *  |   6   |  D09  | SEG05 | 05  | LCD_SEG29 |  29  | even     |  29  |
 *  |   7   |  D11  | SEG06 | 06  | LCD_SEG31 |  31  | even     |  31  |
 *  |   8   |  D13  | SEG07 | 07  | LCD_SEG33 |  33  | odd      |   1  |
 *  |   9   |  D15  | SEG08 | 08  | LCD_SEG35 |  35  | odd      |   3  |
 *  |  10   |  C07  | SEG09 | 09  | LCD_SEG25 |  25  | even     |  25  |
 *  |  11   |  A15  | SEG10 | 10  | LCD_SEG17 |  17  | even     |  17  |
 *  |  12   |  B04  | SEG11 | 11  | LCD_SEG8  |   8  | even     |   8  |
 *  |  13   |  B09  | COM03 |     | LCD_COM3  |      | Com line |      |
 *  |  14   |  A10  | COM02 |     | LCD_COM2  |      | Com line |      |
 *  |  15   |  A09  | COM01 |     | LCD_COM1  |      | Com line |      |
 *  |  16   |  A08  | COM00 |     | LCD_COM0  |      | Com line |      |
 *  |  17   |  B05  | SEG12 | 12  | LCD_SEG9  |   9  | even     |   9  |
 *  |  18   |  C08  | SEG13 | 13  | LCD_SEG26 |  26  | even     |  26  |
 *  |  19   |  C06  | SEG14 | 14  | LCD_SEG24 |  24  | even     |  24  |
 *  |  20   |  D14  | SEG15 | 15  | LCD_SEG34 |  34  | odd      |   2  |
 *  |  21   |  D12  | SEG16 | 16  | LCD_SEG32 |  32  | odd      |   0  |
 *  |  22   |  D10  | SEG17 | 17  | LCD_SEG30 |  30  | even     |  30  |
 *  |  23   |  D08  | SEG18 | 18  | LCD_SEG28 |  28  | even     |  28  |
 *  |  24   |  B14  | SEG19 | 19  | LCD_SEG14 |  14  | even     |  14  |
 *  |  25   |  B12  | SEG20 | 20  | LCD_SEG12 |  12  | even     |  12  |
 *  |  26   |  B00  | SEG21 | 21  | LCD_SEG5  |   5  | even     |   5  |
 *  |  27   |  C04  | SEG22 | 22  | LCD_SEG22 |  22  | even     |  22  |
 *  |  28   |  A06  | SEG23 | 23  | LCD_SEG3  |   3  | even     |   3  |
 *
 *  Here is the table LCD crystal to LCD segment and com
 *
 *  | LCD   | LCD   | LCD  |    MCU    | RAM
 *  | Cry   | segm  | segm |    AF11   | regi
 *  | stal  | ent   | ent  |  channel  | ster
 *  |       | pin   | COM  |  LCD_SEG  |
 *  -------------------------------------------
 *  |  1A   |   22  |   1  |    22     |   2
 *  |  1B   |   22  |   0  |    22     |   0
 *  |  1C   |    1  |   1  |    23     |   2
 *  |  1D   |    0  |   1  |     4     |   2
 *  |  1E   |    0  |   0  |     4     |   0
 *  |  1F   |   23  |   1  |     3     |   2
 *  |  1G   |   23  |   0  |     3     |   0
 *  |  1H   |   23  |   3  |     3     |   6
 *  |  1J   |   22  |   3  |    22     |   6
 *  |  1K   |   22  |   2  |    22     |   4
 *  |  1M   |    1  |   0  |    23     |   0
 *  |  1N   |    0  |   3  |     4     |   6
 *  |  1P   |    0  |   2  |     4     |   4
 *  |  1Q   |   23  |   2  |     3     |   4
 *  -------------------------------------------
 *  |  2A   |   20  |   1  |    12     |   2
 *  |  2B   |   20  |   0  |    12     |   0
 *  |  2C   |    3  |   1  |    13     |   2
 *  |  2D   |    2  |   1  |     6     |   2
 *  |  2E   |    2  |   0  |     6     |   0
 *  |  2F   |   21  |   1  |     5     |   2
 *  |  2G   |   21  |   0  |     5     |   0
 *  |  2H   |   21  |   3  |     5     |   6
 *  |  2J   |   20  |   3  |    12     |   6
 *  |  2K   |   20  |   2  |    12     |   4
 *  |  2M   |    3  |   0  |    13     |   0
 *  |  2N   |    2  |   3  |     6     |   6
 *  |  2P   |    2  |   2  |     6     |   4
 *  |  2Q   |   21  |   2  |     5     |   4
 *  -------------------------------------------
 *  |  3A   |   18  |   1  |    28     |   2
 *  |  3B   |   18  |   0  |    28     |   0
 *  |  3C   |    5  |   1  |    29     |   2
 *  |  3D   |    4  |   1  |    15     |   2
 *  |  3E   |    4  |   0  |    15     |   0
 *  |  3F   |   19  |   1  |    14     |   2
 *  |  3G   |   19  |   0  |    14     |   0
 *  |  3H   |   19  |   3  |    14     |   6
 *  |  3J   |   18  |   3  |    28     |   6
 *  |  3K   |   18  |   2  |    28     |   4
 *  |  3M   |    5  |   0  |    29     |   0
 *  |  3N   |    4  |   3  |    15     |   6
 *  |  3P   |    4  |   2  |    15     |   4
 *  |  3Q   |   19  |   2  |    14     |   4
 *  -------------------------------------------
 *  |  4A   |   16  |   1  |    32  *  |   3
 *  |  4B   |   16  |   0  |    32  *  |   1
 *  |  4C   |    7  |   1  |    33  *  |   3
 *  |  4D   |    6  |   1  |    31     |   2
 *  |  4E   |    6  |   0  |    31     |   0
 *  |  4F   |   17  |   1  |    30     |   2
 *  |  4G   |   17  |   0  |    30     |   0
 *  |  4H   |   17  |   3  |    30     |   6
 *  |  4J   |   16  |   3  |    32  *  |   7
 *  |  4K   |   16  |   2  |    32  *  |   5
 *  |  4M   |    7  |   0  |    33  *  |   1
 *  |  4N   |    6  |   3  |    31     |   6
 *  |  4P   |    6  |   2  |    31     |   4
 *  |  4Q   |   17  |   2  |    30     |   4
 *  -------------------------------------------
 *  |  5A   |   14  |   1  |    24     |   2
 *  |  5B   |   14  |   0  |    24     |   0
 *  |  5C   |    9  |   1  |    25     |   2
 *  |  5D   |    8  |   1  |    35  *  |   3
 *  |  5E   |    8  |   0  |    35  *  |   1
 *  |  5F   |   15  |   1  |    34  *  |   3
 *  |  5G   |   15  |   0  |    34  *  |   1
 *  |  5H   |   15  |   3  |    34  *  |   7
 *  |  5J   |   14  |   3  |    24     |   6
 *  |  5K   |   14  |   2  |    24     |   4
 *  |  5M   |    9  |   0  |    25     |   0
 *  |  5N   |    8  |   3  |    35  *  |   7
 *  |  5P   |    8  |   2  |    35  *  |   5
 *  |  5Q   |   15  |   2  |    34  *  |   5
 *  -------------------------------------------
 *  |  6A   |   12  |   1  |     9     |   2
 *  |  6B   |   12  |   0  |     9     |   0
 *  |  6C   |   11  |   1  |     8     |   2
 *  |  6D   |   10  |   1  |    17     |   2
 *  |  6E   |   10  |   0  |    17     |   0
 *  |  6F   |   13  |   1  |    26     |   2
 *  |  6G   |   13  |   0  |    26     |   0
 *  |  6H   |   13  |   3  |    26     |   6
 *  |  6J   |   12  |   3  |     9     |   6
 *  |  6K   |   12  |   2  |     9     |   4
 *  |  6M   |   11  |   0  |     8     |   0
 *  |  6N   |   10  |   3  |    17     |   6
 *  |  6P   |   10  |   2  |    17     |   4
 *  |  6Q   |   13  |   2  |    26     |   4
 *  -------------------------------------------
 *  |  1DP  |    1  |   3  |    23     |   6
 *  |  1COL |    1  |   2  |    23     |   4
 *  |  2DP  |    3  |   3  |    13     |   6
 *  |  2COL |    3  |   2  |    13     |   4
 *  |  3DP  |    5  |   3  |    29     |   6
 *  |  3COL |    5  |   2  |    29     |   4
 *  |  4DP  |    7  |   3  |    33  *  |   7
 *  |  4COL |    7  |   2  |    33  *  |   5
 *  -------------------------------------------
 *  |  BAR0 |   11  |   3  |     8     |   6
 *  |  BAR1 |   11  |   2  |     8     |   4
 *  |  BAR2 |    9  |   3  |    25     |   6
 *  |  BAR3 |    9  |   2  |    25     |   4

 */

/* Define for scrolling sentences*/
#define GH08172T_SCROLL_SPEED_HIGH   150
#define GH08172T_SCROLL_SPEED_MEDIUM 300
#define GH08172T_SCROLL_SPEED_LOW    450

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

static inline void st_slcd_clear_ram_buffers(const struct device *dev)
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

static inline void st_slcd_write_pattern_to_buffer(const struct device *dev, uint16_t pattern,
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

static inline bool st_slcd_prepare_symbol(const struct device *dev, uint8_t ascii_char,
					  int position)
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
		/* nothing to do, anyway return true to consume the character */
		return true;
	}

	com = config->indicator_com_list[config->panel_config->display_indicators[indicator_index]];
	pin = config->indicator_bit_list[config->panel_config->display_indicators[indicator_index]];

	data->ram_bits_buffers[com] |= pin;
	/* Either way the segment must be updated. */
	data->ram_mask_buffers[com] |= pin;

	return true;
}

static inline bool st_slcd_prepare_custom_character(const struct device *dev, uint8_t ascii_char,
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

static inline void st_slcd_prepare_character(const struct device *dev, uint8_t ascii_char,
					     int position)
{
	const struct auxdisplay_st_slcd_config *config = dev->config;
	const uint16_t pattern =
		slcd_14seg_convert_ascii_char_to_14seg_pattern(config->panel_config, ascii_char);

	st_slcd_write_pattern_to_buffer(dev, pattern, position);
}

static inline int st_slcd_write_buffer_to_lcd_ram(const struct device *dev)
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
 * character->character_code is set to the slot index, thus 0 to custom_character_slot_count.
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

	/* check the MCU AF11 channel values */
	for (uint32_t i = 0; i < config->pin_list_len; i++) {
		if (config->pin_list[i] >= 64) {
			LOG_ERR("Invalid pin-list entry: %u", config->pin_list[i]);
			k_msleep(10);
			return -EINVAL;
		}
	}

	/* check the precompiled segment bits (shift) and COM RAM register */
	for (uint32_t i = 0; i < config->position_count * config->panel_config->segment_type; i++) {
		segment = config->panel_config->segment_pins[i];
		if (segment >= config->pin_list_len) {
			LOG_ERR("Invalid panel segment_pins entry: %u",
				config->panel_config->segment_pins[i]);
			k_msleep(10);
			return -EINVAL;
		}
		segment = config->pin_list[segment];

		bit = 1 << (segment % 32);
		com = (config->panel_config->segment_coms[i] * 2) + segment / 32;

		if (bit != config->character_bit_list[i]) {
			LOG_ERR("Invalid character_bit_list entry: index segment data config %u - "
				"%d - %u - %u",
				i, segment, bit, config->character_bit_list[i]);
			k_msleep(10);
			return -EINVAL;
		}

		if (com != config->character_com_list[i]) {
			LOG_ERR("Invalid character_com_list entry: index segment data config %u - "
				"%d - %u - %u",
				i, segment, com, config->character_com_list[i]);
			k_msleep(10);
			return -EINVAL;
		}
	}

	/* check the precompiled indicators bits (shift) and COM RAM register */
	for (uint32_t i = 0; i < config->panel_config->num_indicators; i++) {
		segment = config->panel_config->indicator_pins[i];
		if (segment >= config->pin_list_len) {
			LOG_ERR("Invalid panel indicator_pins entry: %u",
				config->panel_config->indicator_pins[i]);
			k_msleep(10);
			return -EINVAL;
		}
		segment = config->pin_list[segment];

		bit = 1 << (segment % 32);
		com = (config->panel_config->indicator_coms[i] * 2) + segment / 32;

		if (bit != config->indicator_bit_list[i]) {
			LOG_ERR("Invalid indicator_bit_list entry: index segment data config %u - "
				"%d - %u - %u",
				i, segment, bit, config->indicator_bit_list[i]);
			k_msleep(10);
			return -EINVAL;
		}

		if (com != config->indicator_com_list[i]) {
			LOG_ERR("Invalid indicator_com_list entry: index segment data config %u - "
				"%d - %u - %u",
				i, segment, com, config->indicator_com_list[i]);
			k_msleep(10);
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
	static uint16_t auxdisplay_st_slcd_data_custom_character_patterns##inst[DT_INST_PROP(      \
		inst, custom_character_slot_count)];                                               \
	static uint32_t                                                                            \
		auxdisplay_st_slcd_data_ram_bits_buffers##inst[DT_INST_PROP_LEN(inst, com_list)];  \
	static uint32_t                                                                            \
		auxdisplay_st_slcd_data_ram_mask_buffers##inst[DT_INST_PROP_LEN(inst, com_list)];  \
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
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static const struct stm32_pclken auxdisplay_st_slcd_pclken_##inst[] = {                    \
		STM32_DT_INST_CLOCK_INFO_BY_IDX(inst, 0)};                                         \
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(inst, pin_list),                                        \
		     "ST SLCD instance " #inst " is missing required property pin-list");          \
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(inst, com_list),                                        \
		     "ST SLCD instance " #inst " is missing required property com-list");          \
	static const uint8_t auxdisplay_st_slcd_pin_list_##inst[] = DT_INST_PROP(inst, pin_list);  \
	static const uint32_t auxdisplay_st_slcd_character_bit_list_##inst[] =                     \
		DT_INST_PROP(inst, character_bit_list);                                            \
	static const uint8_t auxdisplay_st_slcd_character_com_list_##inst[] =                      \
		DT_INST_PROP(inst, character_com_list);                                            \
	static const uint32_t auxdisplay_st_slcd_indicator_bit_list_##inst[] =                     \
		DT_INST_PROP(inst, indicator_bit_list);                                            \
	static const uint8_t auxdisplay_st_slcd_indicator_com_list_##inst[] =                      \
		DT_INST_PROP(inst, indicator_com_list);                                            \
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
	DEVICE_DT_INST_DEFINE(inst, auxdisplay_st_slcd_init, NULL,                                 \
			      &auxdisplay_st_slcd_data_##inst, &auxdisplay_st_slcd_config_##inst,  \
			      POST_KERNEL, CONFIG_AUXDISPLAY_INIT_PRIORITY,                        \
			      &auxdisplay_st_slcd_auxdisplay_api)

/* Parse the active devicetree layout and execute the instantiation macro for matching okay
 * targets.
 */
DT_INST_FOREACH_STATUS_OKAY(AUXDISPLAY_ST_SLCD_DEVICE)
