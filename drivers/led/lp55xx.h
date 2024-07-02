/*
 * Copyright (c) 2024 Croxel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LP55XX_H
#define LP55XX_H

#define LP5521_ID 5521
#define LP5562_ID 5562

/* LP5521 Registers */
#define LP5521_ENABLE                  0x00
#define LP5521_OP_MODE                 0x01
#define LP5521_R_PWM                   0x02
#define LP5521_G_PWM                   0x03
#define LP5521_B_PWM                   0x04
#define LP5521_R_CURRENT               0x05
#define LP5521_G_CURRENT               0x06
#define LP5521_B_CURRENT               0x07
#define LP5521_CONFIG                  0x08
#define LP5521_R_PC                    0x09
#define LP5521_G_PC                    0x0A
#define LP5521_B_PC                    0x0B
#define LP5521_STATUS                  0x0C
#define LP5521_RESET                   0x0D
#define LP5521_GPO                     0x0E
#define LP5521_PROG_MEM_RED_ENG_BASE   0x10
#define LP5521_PROG_MEM_GREEN_ENG_BASE 0x30
#define LP5521_PROG_MEM_BLUE_ENG_BASE  0x50

/* LP5562 Registers */
#define LP5562_ENABLE             0x00
#define LP5562_OP_MODE            0x01
#define LP5562_B_PWM              0x02
#define LP5562_G_PWM              0x03
#define LP5562_R_PWM              0x04
#define LP5562_B_CURRENT          0x05
#define LP5562_G_CURRENT          0x06
#define LP5562_R_CURRENT          0x07
#define LP5562_CONFIG             0x08
#define LP5562_ENG1_PC            0x09
#define LP5562_ENG2_PC            0x0A
#define LP5562_ENG3_PC            0x0B
#define LP5562_STATUS             0x0C
#define LP5562_RESET              0x0D
#define LP5562_W_PWM              0x0E
#define LP5562_W_CURRENT          0x0F
#define LP5562_PROG_MEM_ENG1_BASE 0x10
#define LP5562_PROG_MEM_ENG2_BASE 0x30
#define LP5562_PROG_MEM_ENG3_BASE 0x50
#define LP5562_LED_MAP            0x70

/* Output current limits in 0.1 mA */
#define LP5562_MIN_CURRENT_SETTING 0
#define LP5562_MAX_CURRENT_SETTING 255

/* Output current limits in 0.1 mA */
#define LP5521_MIN_CURRENT_SETTING 0
#define LP5521_MAX_CURRENT_SETTING 255

#define LP5521_MAX_BLINK_PERIOD 1000
/*
 * The minimum waiting period is 0.49ms with the prescaler set to 0 and one
 * step. We round up to a full millisecond.
 */
#define LP5521_MIN_BLINK_PERIOD 1

/* Brightness limits in percent */
#define LP5521_MIN_BRIGHTNESS 0
#define LP5521_MAX_BRIGHTNESS 100

#define LP5562_MAX_BLINK_PERIOD 1000

#define LP5562_MIN_BLINK_PERIOD 1

/* Brightness limits in percent */
#define LP5562_MIN_BRIGHTNESS 0
#define LP5562_MAX_BRIGHTNESS 100

/* Values for ENABLE register. */
#define LP5562_ENABLE_CHIP_EN (1 << 6)
#define LP5562_ENABLE_LOG_EN  (1 << 7)

/* Values for CONFIG register. */
#define LP5562_CONFIG_EXTERNAL_CLOCK         0x00
#define LP5562_CONFIG_INTERNAL_CLOCK         0x01
#define LP5562_CONFIG_CLOCK_AUTOMATIC_SELECT 0x02
#define LP5562_CONFIG_PWRSAVE_EN             (1 << 5)
/* Enable 558 Hz frequency for PWM. Default is 256. */
#define LP5562_CONFIG_PWM_HW_FREQ_558        (1 << 6)

/* Values for execution engine programs. */
#define LP5562_PROG_COMMAND_SET_PWM                           (1 << 6)
#define LP5562_PROG_COMMAND_RAMP_TIME(prescale, step_time)    (((prescale) << 6) | (step_time))
#define LP5562_PROG_COMMAND_STEP_COUNT(fade_direction, count) (((fade_direction) << 7) | (count))

/* Helper definitions. */
#define LP5562_PROG_MAX_COMMANDS     16
#define LP55XX_MASK                  0x03
#define LP55XX_CHANNEL_MASK(channel) ((LP55XX_MASK) << (channel << 1))

#define LP5521_MAX_LEDS 3
#define LP5562_MAX_LEDS 4

enum lp55xx_led_sources {
	LP5562_SOURCE_PWM,
	LP5562_SOURCE_ENGINE_1,
	LP5562_SOURCE_ENGINE_2,
	LP5562_SOURCE_ENGINE_3,

	LP5562_SOURCE_COUNT,
};

/*
 * Available channels. LP55XX driver currently supports LP5521 and LP5562 which supports 3 LEDs(RGB)
 * and 4 LEDs(RGBW) respectively
 */
enum lp55xx_led_channels {
	LP55XX_CHANNEL_B,
	LP55XX_CHANNEL_G,
	LP55XX_CHANNEL_R,
	LP55XX_CHANNEL_W,

	LP55XX_CHANNEL_COUNT,
};

/* Operational modes of the execution engines. */
enum lp55xx_engine_op_modes {
	LP55XX_OP_MODE_DISABLED = 0x00,
	LP55XX_OP_MODE_LOAD = 0x01,
	LP55XX_OP_MODE_RUN = 0x02,
	LP55XX_OP_MODE_DIRECT_CTRL = 0x03,
};

/* Execution state of the engines. */
enum lp55xx_engine_exec_states {
	LP55XX_ENGINE_MODE_HOLD = 0x00,
	LP55XX_ENGINE_MODE_STEP = 0x01,
	LP55XX_ENGINE_MODE_RUN = 0x02,
	LP55XX_ENGINE_MODE_EXEC = 0x03,
};

/* Fading directions for programs executed by the engines. */
enum lp55xx_engine_fade_dirs {
	LP55XX_FADE_UP = 0x00,
	LP55XX_FADE_DOWN = 0x01,
};

#endif /* LP55XX_H */
