/*
 * Copyright (c) 2017 Bobby Noelte
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _PINCTRL_COMMON_H_
#define _PINCTRL_COMMON_H_

/**
 * @brief PINCTRL Pin Configurations
 * @defgroup pinctrl_interface_pin_configurations PINCTRL Pin Configurations
 * @ingroup pinctrl_interface
 * @{
 */

/**
 * @def PINCTRL_CONFIG_BIAS_DISABLE
 * @brief Disable any pin bias.
 */
#define PINCTRL_CONFIG_BIAS_DISABLE             (1<<0)
/**
 * @def PINCTRL_CONFIG_BIAS_HIGH_IMPEDANCE
 * @brief High impedance mode ("third-state", "floating").
 */
#define PINCTRL_CONFIG_BIAS_HIGH_IMPEDANCE	(1<<1)
/**
 * @def PINCTRL_CONFIG_BIAS_BUS_HOLD
 * @brief Latch weakly.
 */
#define PINCTRL_CONFIG_BIAS_BUS_HOLD		(1<<2)
/**
 * @def PINCTRL_CONFIG_BIAS_PULL_UP
 * @brief Pull up the pin.
 */
#define PINCTRL_CONFIG_BIAS_PULL_UP		(1<<3)
/**
 * @def PINCTRL_CONFIG_BIAS_PULL_DOWN
 * @brief Pull down the pin.
 */
#define PINCTRL_CONFIG_BIAS_PULL_DOWN		(1<<4)
/**
 * @def PINCTRL_CONFIG_BIAS_PULL_PIN_DEFAULT
 * @brief Use pin default pull state.
 */
#define PINCTRL_CONFIG_BIAS_PULL_PIN_DEFAULT	(1<<5)
/**
 * @def PINCTRL_CONFIG_DRIVE_PUSH_PULL
 * @brief Drive actively high and low.
 */
#define PINCTRL_CONFIG_DRIVE_PUSH_PULL		(1<<6)
/**
 * @def PINCTRL_CONFIG_DRIVE_OPEN_DRAIN
 * @brief Drive with open drain (open collector).
 */
#define PINCTRL_CONFIG_DRIVE_OPEN_DRAIN		(1<<7)
/**
 * @def PINCTRL_CONFIG_DRIVE_OPEN_SOURCE
 * @brief Drive with open source (open emitter).
 */
#define PINCTRL_CONFIG_DRIVE_OPEN_SOURCE	(1<<8)
/**
 * @def PINCTRL_CONFIG_DRIVE_STRENGTH_DEFAULT
 * @brief Drive with default drive strength.
 */
#define PINCTRL_CONFIG_DRIVE_STRENGTH_DEFAULT	(0<<9)
/**
 * @def PINCTRL_CONFIG_DRIVE_STRENGTH_1
 * @brief Drive with minimum drive strength.
 */
#define PINCTRL_CONFIG_DRIVE_STRENGTH_1		(1<<9)
/**
 * @def PINCTRL_CONFIG_DRIVE_STRENGTH_2
 * @brief Drive with minimum to medium drive strength.
 */
#define PINCTRL_CONFIG_DRIVE_STRENGTH_2		(2<<9)
/**
 * @def PINCTRL_CONFIG_DRIVE_STRENGTH_3
 * @brief Drive with minimum to medium drive strength.
 */
#define PINCTRL_CONFIG_DRIVE_STRENGTH_3		(3<<9)
/**
 * @def PINCTRL_CONFIG_DRIVE_STRENGTH_4
 * @brief Drive with medium drive strength.
 */
#define PINCTRL_CONFIG_DRIVE_STRENGTH_4		(4<<9)
/**
 * @def PINCTRL_CONFIG_DRIVE_STRENGTH_5
 * @brief Drive with medium to maximum drive strength.
 */
#define PINCTRL_CONFIG_DRIVE_STRENGTH_5		(5<<9)
/**
 * @def PINCTRL_CONFIG_DRIVE_STRENGTH_6
 * @brief Drive with medium to maximum drive strength.
 */
#define PINCTRL_CONFIG_DRIVE_STRENGTH_6		(6<<9)
/**
 * @def PINCTRL_CONFIG_DRIVE_STRENGTH_7
 * @brief Drive with maximum drive strength.
 */
#define PINCTRL_CONFIG_DRIVE_STRENGTH_7		(7<<9)
/**
 * @def PINCTRL_CONFIG_INPUT_ENABLE
 * @brief Enable the pins input.
 * @note Does not affect the pin's ability to drive output.
 */
#define PINCTRL_CONFIG_INPUT_ENABLE		(1<<12)
/**
 * @def PINCTRL_CONFIG_INPUT_DISABLE
 * @brief Disable the pins input.
 * @note Does not affect the pin's ability to drive output.
 */
#define PINCTRL_CONFIG_INPUT_DISABLE		(1<<13)
/**
 * @def PINCTRL_CONFIG_INPUT_SCHMITT_ENABLE
 * @brief Enable schmitt trigger mode for input.
 */
#define PINCTRL_CONFIG_INPUT_SCHMITT_ENABLE	(1<<14)
/**
 * @def PINCTRL_CONFIG_INPUT_SCHMITT_DISABLE
 * @brief Disable schmitt trigger mode for input.
 */
#define PINCTRL_CONFIG_INPUT_SCHMITT_DISABLE	(1<<15)
/**
 * @def PINCTRL_CONFIG_INPUT_DEBOUNCE_NONE
 * @brief Do not debounce input.
 */
#define PINCTRL_CONFIG_INPUT_DEBOUNCE_NONE	(0<<16)
/**
 * @def PINCTRL_CONFIG_INPUT_DEBOUNCE_SHORT
 * @brief Debounce input with short debounce time.
 */
#define PINCTRL_CONFIG_INPUT_DEBOUNCE_SHORT	(1<<16)
/**
 * @def PINCTRL_CONFIG_INPUT_DEBOUNCE_MEDIUM
 * @brief Debounce input with medium debounce time.
 */
#define PINCTRL_CONFIG_INPUT_DEBOUNCE_MEDIUM	(2<<16)
/**
 * @def PINCTRL_CONFIG_INPUT_DEBOUNCE_LONG
 * @brief Debounce input with long debounce time.
 */
#define PINCTRL_CONFIG_INPUT_DEBOUNCE_LONG	(3<<16)
/**
 * @def PINCTRL_CONFIG_POWER_SOURCE_DEFAULT
 * @brief Select default power source for pin.
 */
#define PINCTRL_CONFIG_POWER_SOURCE_DEFAULT	(0<<18)
/**
 * @def PINCTRL_CONFIG_POWER_SOURCE_1
 * @brief Select power source #1 for pin.
 */
#define PINCTRL_CONFIG_POWER_SOURCE_1		(1<<18)
/**
 * @def PINCTRL_CONFIG_POWER_SOURCE_2
 * @brief Select power source #2 for pin.
 */
#define PINCTRL_CONFIG_POWER_SOURCE_2		(2<<18)
/**
 * @def PINCTRL_CONFIG_POWER_SOURCE_3
 * @brief Select power source #3 for pin.
 */
#define PINCTRL_CONFIG_POWER_SOURCE_3		(3<<18)
/**
 * @def PINCTRL_CONFIG_LOW_POWER_ENABLE
 * @brief Enable low power mode.
 */
#define PINCTRL_CONFIG_LOW_POWER_ENABLE		(1<<20)
/**
 * @def PINCTRL_CONFIG_LOW_POWER_DISABLE
 * @brief Disable low power mode.
 */
#define PINCTRL_CONFIG_LOW_POWER_DISABLE	(1<<21)
/**
 * @def PINCTRL_CONFIG_OUTPUT_ENABLE
 * @brief Enable output on pin.
 *
 * Such as connecting the output buffer to the drive stage.
 *
 * @note Does not set the output drive.
 */
#define PINCTRL_CONFIG_OUTPUT_ENABLE            (1<<22)
/**
 * @def PINCTRL_CONFIG_OUTPUT_DISABLE
 * @brief Disable output on pin.
 *
 * Such as disconnecting the output buffer from the drive stage.
 *
 * @note Does not reset the output drive.
 */
#define PINCTRL_CONFIG_OUTPUT_DISABLE           (1<<23)
/**
 * @def PINCTRL_CONFIG_OUTPUT_LOW
 * @brief Set output to active low.
 *
 * A "1" in the output buffer drives the output to low level.
 */
#define PINCTRL_CONFIG_OUTPUT_LOW		(1<<24)
/**
 * @def PINCTRL_CONFIG_OUTPUT_HIGH
 * @brief Set output to active high.
 *
 * A "1" in the output buffer drives the output to high level.
 */
#define PINCTRL_CONFIG_OUTPUT_HIGH		(1<<25)
/**
 * @def PINCTRL_CONFIG_SLEW_RATE_SLOW
 * @brief Select slow slew rate.
 */
#define PINCTRL_CONFIG_SLEW_RATE_SLOW		(0<<26)
/**
 * @def PINCTRL_CONFIG_SLEW_RATE_MEDIUM
 * @brief Select medium slew rate.
 */
#define PINCTRL_CONFIG_SLEW_RATE_MEDIUM		(1<<26)
/**
 * @def PINCTRL_CONFIG_SLEW_RATE_FAST
 * @brief Select fast slew rate.
 */
#define PINCTRL_CONFIG_SLEW_RATE_FAST		(2<<26)
/**
 * @def PINCTRL_CONFIG_SLEW_RATE_HIGH
 * @brief Select high slew rate.
 */
#define PINCTRL_CONFIG_SLEW_RATE_HIGH		(3<<26)
/**
 * @def PINCTRL_CONFIG_SPEED_SLOW
 * @brief Select low toggle speed.
 *
 * @note Slew rate may be unaffected.
 */
#define PINCTRL_CONFIG_SPEED_SLOW		(0<<28)
/**
 * @def PINCTRL_CONFIG_SPEED_MEDIUM
 * @brief Select medium toggle speed.
 *
 * @note Slew rate may be unaffected.
 */
#define PINCTRL_CONFIG_SPEED_MEDIUM		(1<<28)
/**
 * @def PINCTRL_CONFIG_SPEED_FAST
 * @brief Select fast toggle speed.
 *
 * @note Slew rate may be unaffected.
 */
#define PINCTRL_CONFIG_SPEED_FAST		(2<<28)
/**
 * @def PINCTRL_CONFIG_SPEED_HIGH
 * @brief Select high toggle speed.
 *
 * @note Slew rate may be unaffected.
 */
#define PINCTRL_CONFIG_SPEED_HIGH		(3<<28)

/**
 * @def PINCTRL_CONFIG_BIAS_MASK
 * @brief Mask for all PINCTRL_CONFIG_BIAS_xxx values.
 */
#define PINCTRL_CONFIG_BIAS_MASK	(PINCTRL_CONFIG_BIAS_BUS_HOLD\
					| PINCTRL_CONFIG_BIAS_DISABLE\
					| PINCTRL_CONFIG_BIAS_HIGH_IMPEDANCE\
					| PINCTRL_CONFIG_BIAS_PULL_DOWN\
					| PINCTRL_CONFIG_BIAS_PULL_PIN_DEFAULT\
					| PINCTRL_CONFIG_BIAS_PULL_UP)

/**
 * @def PINCTRL_CONFIG_INPUT_MASK
 * @brief Mask for PINCTRL_CONFIG_INPUT_ENABLE/DISABLE values.
 */
#define PINCTRL_CONFIG_INPUT_MASK	(PINCTRL_CONFIG_INPUT_ENABLE\
					| PINCTRL_CONFIG_INPUT_DISABLE)

/**
 * @def PINCTRL_CONFIG_OUTPUT_MASK
 * @brief Mask for PINCTRL_CONFIG_OUTPUT_ENABLE/DISABLE values.
 */
#define PINCTRL_CONFIG_OUTPUT_MASK	(PINCTRL_CONFIG_OUTPUT_ENABLE\
					| PINCTRL_CONFIG_OUTPUT_DISABLE)

/**
 * @def PINCTRL_CONFIG_DRIVE_MASK
 * @brief Mask for PINCTRL_CONFIG_DRIVE_x values.
 */
#define PINCTRL_CONFIG_DRIVE_MASK	(PINCTRL_CONFIG_DRIVE_PUSH_PULL\
					| PINCTRL_CONFIG_DRIVE_OPEN_DRAIN\
					| PINCTRL_CONFIG_DRIVE_OPEN_SOURCE)


/**
 * @def PINCTRL_CONFIG_DRIVE_STRENGTH_MASK
 * @brief Mask for PINCTRL_CONFIG_DRIVE_STRENGTH_x values.
 */
#define PINCTRL_CONFIG_DRIVE_STRENGTH_MASK	PINCTRL_CONFIG_DRIVE_STRENGTH_7

/**@} pinctrl_interface_pin_configurations */

/**
 * @brief PINCTRL Functions
 * @defgroup pinctrl_interface_functions PINCTRL Functions
 * @ingroup pinctrl_interface
 * @{
 */

#define PINCTRL_FUNCTION_HARDWARE_BASE	0
#define PINCTRL_FUNCTION_HARDWARE_0	(PINCTRL_FUNCTION_HARDWARE_BASE + 0)
#define PINCTRL_FUNCTION_HARDWARE_1	(PINCTRL_FUNCTION_HARDWARE_BASE + 1)
#define PINCTRL_FUNCTION_HARDWARE_2	(PINCTRL_FUNCTION_HARDWARE_BASE + 2)
#define PINCTRL_FUNCTION_HARDWARE_3	(PINCTRL_FUNCTION_HARDWARE_BASE + 3)
#define PINCTRL_FUNCTION_HARDWARE_4	(PINCTRL_FUNCTION_HARDWARE_BASE + 4)
#define PINCTRL_FUNCTION_HARDWARE_5	(PINCTRL_FUNCTION_HARDWARE_BASE + 5)
#define PINCTRL_FUNCTION_HARDWARE_6	(PINCTRL_FUNCTION_HARDWARE_BASE + 6)
#define PINCTRL_FUNCTION_HARDWARE_7	(PINCTRL_FUNCTION_HARDWARE_BASE + 7)
#define PINCTRL_FUNCTION_HARDWARE_8	(PINCTRL_FUNCTION_HARDWARE_BASE + 8)
#define PINCTRL_FUNCTION_HARDWARE_9	(PINCTRL_FUNCTION_HARDWARE_BASE + 9)
#define PINCTRL_FUNCTION_HARDWARE_10	(PINCTRL_FUNCTION_HARDWARE_BASE + 10)
#define PINCTRL_FUNCTION_HARDWARE_11	(PINCTRL_FUNCTION_HARDWARE_BASE + 11)
#define PINCTRL_FUNCTION_HARDWARE_12	(PINCTRL_FUNCTION_HARDWARE_BASE + 12)
#define PINCTRL_FUNCTION_HARDWARE_13	(PINCTRL_FUNCTION_HARDWARE_BASE + 13)
#define PINCTRL_FUNCTION_HARDWARE_14	(PINCTRL_FUNCTION_HARDWARE_BASE + 14)
#define PINCTRL_FUNCTION_HARDWARE_15	(PINCTRL_FUNCTION_HARDWARE_BASE + 15)
#define PINCTRL_FUNCTION_HARDWARE_16	(PINCTRL_FUNCTION_HARDWARE_BASE + 16)
#define PINCTRL_FUNCTION_HARDWARE_17	(PINCTRL_FUNCTION_HARDWARE_BASE + 17)
#define PINCTRL_FUNCTION_HARDWARE_18	(PINCTRL_FUNCTION_HARDWARE_BASE + 18)
#define PINCTRL_FUNCTION_HARDWARE_19	(PINCTRL_FUNCTION_HARDWARE_BASE + 19)
#define PINCTRL_FUNCTION_HARDWARE_20	(PINCTRL_FUNCTION_HARDWARE_BASE + 20)
#define PINCTRL_FUNCTION_HARDWARE_21	(PINCTRL_FUNCTION_HARDWARE_BASE + 21)
#define PINCTRL_FUNCTION_HARDWARE_22	(PINCTRL_FUNCTION_HARDWARE_BASE + 22)
#define PINCTRL_FUNCTION_HARDWARE_23	(PINCTRL_FUNCTION_HARDWARE_BASE + 23)
#define PINCTRL_FUNCTION_HARDWARE_24	(PINCTRL_FUNCTION_HARDWARE_BASE + 24)
#define PINCTRL_FUNCTION_HARDWARE_25	(PINCTRL_FUNCTION_HARDWARE_BASE + 25)
#define PINCTRL_FUNCTION_HARDWARE_26	(PINCTRL_FUNCTION_HARDWARE_BASE + 26)
#define PINCTRL_FUNCTION_HARDWARE_27	(PINCTRL_FUNCTION_HARDWARE_BASE + 27)
#define PINCTRL_FUNCTION_HARDWARE_28	(PINCTRL_FUNCTION_HARDWARE_BASE + 28)
#define PINCTRL_FUNCTION_HARDWARE_29	(PINCTRL_FUNCTION_HARDWARE_BASE + 29)
#define PINCTRL_FUNCTION_HARDWARE_30	(PINCTRL_FUNCTION_HARDWARE_BASE + 30)
#define PINCTRL_FUNCTION_HARDWARE_31	(PINCTRL_FUNCTION_HARDWARE_BASE + 31)
#define PINCTRL_FUNCTION_DEVICE_BASE	32
#define PINCTRL_FUNCTION_DEVICE_0	(PINCTRL_FUNCTION_DEVICE_BASE + 0)

/**@} pinctrl_interface_functions */

#endif /* _PINCTRL_COMMON_H_ */
