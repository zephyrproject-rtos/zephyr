/*
 * Copyright (c) 2020 Seagate Technology LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_LED_LP50XX_H_
#define ZEPHYR_INCLUDE_DRIVERS_LED_LP50XX_H_

#define LP50XX_COLORS_PER_LED 3
#define LP50XX_MAX_LEDS(chip)                                                                      \
	(chip == LP5036   ? 12                                                                     \
	 : chip == LP5030 ? 10                                                                     \
	 : chip == LP5024 ? 8                                                                      \
	 : chip == LP5018 ? 6                                                                      \
	 : chip == LP5012 ? 4                                                                      \
			  : 3)

/*
 * Channel mapping.
 */

#define LP50XX_NUM_CHANNELS(chip) (chip >= LP5030 ? 0x38 : chip >= LP5018 ? 0x27 : 0x17)

/* Bank channels. */
#define LP50XX_BANK_BRIGHT_CHAN(chip) (chip >= LP5030 ? 0x04 : 0x03)
#define LP50XX_BANK_A_COLOR(chip)     (LP50XX_BANK_BRIGHT_CHAN(chip) + 1)
#define LP50XX_BANK_B_COLOR(chip)     (LP50XX_BANK_BRIGHT_CHAN(chip) + 2)
#define LP50XX_BANK_C_COLOR(chip)     (LP50XX_BANK_BRIGHT_CHAN(chip) + 3)

/* LED brightness channels. */
#define LP50XX_LED_BRIGHT_CHAN_BASE(chip) (chip >= LP5030 ? 0x08 : 0x07)
#define LP50XX_LED_BRIGHT_CHAN(chip, led) (LP50XX_LED_BRIGHT_CHAN_BASE(chip) + led)

/* LED color channels. */
#define LP50XX_LED_COL_CHAN_BASE(chip) (chip >= LP5030 ? 0x14 : chip >= LP5018 ? 0x0F : 0x0B)
#define LP50XX_LED_COL1_CHAN(chip, led)                                                            \
	(LP50XX_LED_COL_CHAN_BASE(chip) + (led * LP50XX_COLORS_PER_LED))
#define LP50XX_LED_COL2_CHAN(chip, led)                                                            \
	(LP50XX_LED_COL_CHAN_BASE(chip) + (led * LP50XX_COLORS_PER_LED) + 1)
#define LP50XX_LED_COL3_CHAN(chip, led)                                                            \
	(LP50XX_LED_COL_CHAN_BASE(chip) + (led * LP50XX_COLORS_PER_LED) + 2)

/* Reset Channel */
#define LP50XX_RESET_CHAN(chip) (chip >= LP5030 ? 0x38 : chip >= LP5018 ? 0x27 : 0x17)

/* Chips supported by this driver. */
enum lp50xx_chips {
	LP5009,
	LP5012,
	LP5018,
	LP5024,
	LP5030,
	LP5036,
};

/**
 * Get the chip from the device
 * @param dev LP50xx device
 * @return    The chip corresponding to the device.
 */
enum lp50xx_chips lp50xx_get_chip(const struct device *dev);

/**
 * Enable bank mode for the given LEDs.
 *
 * @param dev  LP50xx device
 * @param leds Bitfield of the LEDs for which the bank mode is to be activated.
 * @return     0 if successful, negative errno code on failure.
 */
int lp50xx_set_bank_mode(const struct device *dev, uint32_t leds);

/**
 * Set color of LEDs with bank mode enabled.
 *
 * @param dev   LP50xx device
 * @param color Array of colors. It must be ordered following the color mapping of the LED
 *              controller. See the the color_mapping member in struct led_info.
 * @return      0 if successful, negative errno code on failure.
 */
int lp50xx_set_bank_color(const struct device *dev, const uint8_t color[3]);

/**
 * Set brightness of LEDs with bank mode enabled.
 *
 * @param dev   LP50xx device
 * @param value Brightness value to set
 * @return
 */
int lp50xx_set_bank_brightness(const struct device *dev, uint8_t value);

/**
 * Set the Chip_EN bit in the device config.
 *
 * @param dev    LP50xx device
 * @param enable 1 = LP50xx enabled, 0 = LP50xx not enabled.
 * @return       0 if successful, negative errno code on failure.
 */
int lp50xx_set_chip_en(const struct device *dev, uint8_t enable);

/**
 * Set the Log_Scale_EN bit in the device config.
 *
 * @param dev    LP50xx device
 * @param enable 1 = Logarithmic, 0 = Linear scale dimming curve enabled.
 * @return       0 if successful, negative errno code on failure.
 */
int lp50xx_set_log_scale_en(const struct device *dev, uint8_t enable);

/**
 * Set the Power_Save_EN bit in the device config.
 *
 * @param dev    LP50xx device
 * @param enable Automatic power-saving mode 1 = enabled, 0 = not enabled.
 * @return       0 if successful, negative errno code on failure.
 */
int lp50xx_set_power_save_en(const struct device *dev, uint8_t enable);

/**
 * Set the Auto_Incr_EN bit in the device config.
 *
 * @param dev    LP50xx device
 * @param enable Automatic increment mode 1 = enabled, 0 = not enabled.
 * @return       0 if successful, negative errno code on failure.
 */
int lp50xx_set_auto_incr_en(const struct device *dev, uint8_t enable);

/**
 * Set the PWM_Dithering_EN bit in the device config.
 *
 * @param dev    LP50xx device
 * @param enable PWM dithering mode 1 = enabled, 0 = not enabled.
 * @return       0 if successful, negative errno code on failure.
 */
int lp50xx_set_pwm_dithering_en(const struct device *dev, uint8_t enable);

/**
 * Set the Max_Current_Option bit in the device config.
 *
 * @param dev    LP50xx device
 * @param enable Output maximum current 1 = 35mA, 0 = 25mA.
 * @return       0 if successful, negative errno code on failure.
 */
int lp50xx_set_max_current_option(const struct device *dev, uint8_t enable);

/**
 * Set the LED_Global_Off bit in the device config.
 *
 * @param dev    LP50xx device
 * @param enable 1 = Shut down all LEDs, 0 = Normal Operation.
 * @return       0 if successful, negative errno code on failure.
 */
int lp50xx_set_led_global_off(const struct device *dev, uint8_t enable);

#endif /* ZEPHYR_INCLUDE_DRIVERS_LED_LP50XX_H_ */
