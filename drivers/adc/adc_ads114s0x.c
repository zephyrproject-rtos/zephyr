/*
 * Copyright (c) 2023 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER 1
#include "adc_context.h"

LOG_MODULE_REGISTER(ads114s0x, CONFIG_ADC_LOG_LEVEL);

#define ADS114S0X_CLK_FREQ_IN_KHZ			    4096
#define ADS114S0X_RESET_LOW_TIME_IN_CLOCK_CYCLES	    4
#define ADS114S0X_START_SYNC_PULSE_DURATION_IN_CLOCK_CYCLES 4
#define ADS114S0X_SETUP_TIME_IN_CLOCK_CYCLES		    32
#define ADS114S0X_INPUT_SELECTION_AINCOM		    12
#define ADS114S0X_RESOLUTION				    16
#define ADS114S0X_REF_INTERNAL				    2500
#define ADS114S0X_POWER_ON_RESET_TIME_IN_US		    2200

/* Not mentioned in the datasheet, but instead determined experimentally. */
#define ADS114S0X_RESET_DELAY_TIME_SAFETY_MARGIN_IN_US 1000
#define ADS114S0X_RESET_DELAY_TIME_IN_US                                                           \
	(4096 * 1000 / ADS114S0X_CLK_FREQ_IN_KHZ + ADS114S0X_RESET_DELAY_TIME_SAFETY_MARGIN_IN_US)

#define ADS114S0X_RESET_LOW_TIME_IN_US                                                             \
	(ADS114S0X_RESET_LOW_TIME_IN_CLOCK_CYCLES * 1000 / ADS114S0X_CLK_FREQ_IN_KHZ)
#define ADS114S0X_START_SYNC_PULSE_DURATION_IN_US                                                  \
	(ADS114S0X_START_SYNC_PULSE_DURATION_IN_CLOCK_CYCLES * 1000 / ADS114S0X_CLK_FREQ_IN_KHZ)
#define ADS114S0X_SETUP_TIME_IN_US                                                                 \
	(ADS114S0X_SETUP_TIME_IN_CLOCK_CYCLES * 1000 / ADS114S0X_CLK_FREQ_IN_KHZ)

enum ads114s0x_command {
	ADS114S0X_COMMAND_NOP = 0x00,
	ADS114S0X_COMMAND_WAKEUP = 0x02,
	ADS114S0X_COMMAND_POWERDOWN = 0x04,
	ADS114S0X_COMMAND_RESET = 0x06,
	ADS114S0X_COMMAND_START = 0x08,
	ADS114S0X_COMMAND_STOP = 0x0A,
	ADS114S0X_COMMAND_SYOCAL = 0x16,
	ADS114S0X_COMMAND_SYGCAL = 0x17,
	ADS114S0X_COMMAND_SFOCAL = 0x19,
	ADS114S0X_COMMAND_RDATA = 0x12,
	ADS114S0X_COMMAND_RREG = 0x20,
	ADS114S0X_COMMAND_WREG = 0x40,
};

enum ads114s0x_register {
	ADS114S0X_REGISTER_ID = 0x00,
	ADS114S0X_REGISTER_STATUS = 0x01,
	ADS114S0X_REGISTER_INPMUX = 0x02,
	ADS114S0X_REGISTER_PGA = 0x03,
	ADS114S0X_REGISTER_DATARATE = 0x04,
	ADS114S0X_REGISTER_REF = 0x05,
	ADS114S0X_REGISTER_IDACMAG = 0x06,
	ADS114S0X_REGISTER_IDACMUX = 0x07,
	ADS114S0X_REGISTER_VBIAS = 0x08,
	ADS114S0X_REGISTER_SYS = 0x09,
	ADS114S0X_REGISTER_OFCAL0 = 0x0B,
	ADS114S0X_REGISTER_OFCAL1 = 0x0C,
	ADS114S0X_REGISTER_FSCAL0 = 0x0E,
	ADS114S0X_REGISTER_FSCAL1 = 0x0F,
	ADS114S0X_REGISTER_GPIODAT = 0x10,
	ADS114S0X_REGISTER_GPIOCON = 0x11,
};

#define ADS114S0X_REGISTER_GET_VALUE(value, pos, length)                                           \
	FIELD_GET(GENMASK(pos + length - 1, pos), value)
#define ADS114S0X_REGISTER_SET_VALUE(target, value, pos, length)                                   \
	target &= ~GENMASK(pos + length - 1, pos);                                                 \
	target |= FIELD_PREP(GENMASK(pos + length - 1, pos), value)

#define ADS114S0X_REGISTER_ID_DEV_ID_LENGTH 3
#define ADS114S0X_REGISTER_ID_DEV_ID_POS    0
#define ADS114S0X_REGISTER_ID_DEV_ID_GET(value)                                                    \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_ID_DEV_ID_POS,                      \
				     ADS114S0X_REGISTER_ID_DEV_ID_LENGTH)
#define ADS114S0X_REGISTER_ID_DEV_ID_SET(target, value)                                            \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_ID_DEV_ID_POS,              \
				     ADS114S0X_REGISTER_ID_DEV_ID_LENGTH)
#define ADS114S0X_REGISTER_STATUS_FL_POR_LENGTH 1
#define ADS114S0X_REGISTER_STATUS_FL_POR_POS	7
#define ADS114S0X_REGISTER_STATUS_FL_POR_GET(value)                                                \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_STATUS_FL_POR_POS,                  \
				     ADS114S0X_REGISTER_STATUS_FL_POR_LENGTH)
#define ADS114S0X_REGISTER_STATUS_FL_POR_SET(target, value)                                        \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_STATUS_FL_POR_POS,          \
				     ADS114S0X_REGISTER_STATUS_FL_POR_LENGTH)
#define ADS114S0X_REGISTER_STATUS_NOT_RDY_LENGTH 1
#define ADS114S0X_REGISTER_STATUS_NOT_RDY_POS	 6
#define ADS114S0X_REGISTER_STATUS_NOT_RDY_GET(value)                                               \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_STATUS_NOT_RDY_POS,                 \
				     ADS114S0X_REGISTER_STATUS_NOT_RDY_LENGTH)
#define ADS114S0X_REGISTER_STATUS_NOT_RDY_SET(target, value)                                       \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_STATUS_NOT_RDY_POS,         \
				     ADS114S0X_REGISTER_STATUS_NOT_RDY_LENGTH)
#define ADS114S0X_REGISTER_STATUS_FL_P_RAILP_LENGTH 1
#define ADS114S0X_REGISTER_STATUS_FL_P_RAILP_POS    5
#define ADS114S0X_REGISTER_STATUS_FL_P_RAILP_GET(value)                                            \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_STATUS_FL_P_RAILP_POS,              \
				     ADS114S0X_REGISTER_STATUS_FL_P_RAILP_LENGTH)
#define ADS114S0X_REGISTER_STATUS_FL_P_RAILP_SET(target, value)                                    \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_STATUS_FL_P_RAILP_POS,      \
				     ADS114S0X_REGISTER_STATUS_FL_P_RAILP_LENGTH)
#define ADS114S0X_REGISTER_STATUS_FL_P_RAILN_LENGTH 1
#define ADS114S0X_REGISTER_STATUS_FL_P_RAILN_POS    4
#define ADS114S0X_REGISTER_STATUS_FL_P_RAILN_GET(value)                                            \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_STATUS_FL_P_RAILN_POS,              \
				     ADS114S0X_REGISTER_STATUS_FL_P_RAILN_LENGTH)
#define ADS114S0X_REGISTER_STATUS_FL_P_RAILN_SET(target, value)                                    \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_STATUS_FL_P_RAILN_POS,      \
				     ADS114S0X_REGISTER_STATUS_FL_P_RAILN_LENGTH)
#define ADS114S0X_REGISTER_STATUS_FL_N_RAILP_LENGTH 1
#define ADS114S0X_REGISTER_STATUS_FL_N_RAILP_POS    3
#define ADS114S0X_REGISTER_STATUS_FL_N_RAILP_GET(value)                                            \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_STATUS_FL_N_RAILP_POS,              \
				     ADS114S0X_REGISTER_STATUS_FL_N_RAILP_LENGTH)
#define ADS114S0X_REGISTER_STATUS_FL_N_RAILP_SET(target, value)                                    \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_STATUS_FL_N_RAILP_POS,      \
				     ADS114S0X_REGISTER_STATUS_FL_N_RAILP_LENGTH)
#define ADS114S0X_REGISTER_STATUS_FL_N_RAILN_LENGTH 1
#define ADS114S0X_REGISTER_STATUS_FL_N_RAILN_POS    2
#define ADS114S0X_REGISTER_STATUS_FL_N_RAILN_GET(value)                                            \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_STATUS_FL_N_RAILN_POS,              \
				     ADS114S0X_REGISTER_STATUS_FL_N_RAILN_LENGTH)
#define ADS114S0X_REGISTER_STATUS_FL_N_RAILN_SET(target, value)                                    \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_STATUS_FL_N_RAILN_POS,      \
				     ADS114S0X_REGISTER_STATUS_FL_N_RAILN_LENGTH)
#define ADS114S0X_REGISTER_STATUS_FL_REF_L1_LENGTH 1
#define ADS114S0X_REGISTER_STATUS_FL_REF_L1_POS	   1
#define ADS114S0X_REGISTER_STATUS_FL_REF_L1_GET(value)                                             \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_STATUS_FL_REF_L1_POS,               \
				     ADS114S0X_REGISTER_STATUS_FL_REF_L1_LENGTH)
#define ADS114S0X_REGISTER_STATUS_FL_REF_L1_SET(target, value)                                     \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_STATUS_FL_REF_L1_POS,       \
				     ADS114S0X_REGISTER_STATUS_FL_REF_L1_LENGTH)
#define ADS114S0X_REGISTER_STATUS_FL_REF_L0_LENGTH 1
#define ADS114S0X_REGISTER_STATUS_FL_REF_L0_POS	   0
#define ADS114S0X_REGISTER_STATUS_FL_REF_L0_GET(value)                                             \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_STATUS_FL_REF_L0_POS,               \
				     ADS114S0X_REGISTER_STATUS_FL_REF_L0_LENGTH)
#define ADS114S0X_REGISTER_STATUS_FL_REF_L0_SET(target, value)                                     \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_STATUS_FL_REF_L0_POS,       \
				     ADS114S0X_REGISTER_STATUS_FL_REF_L0_LENGTH)
#define ADS114S0X_REGISTER_INPMUX_MUXP_LENGTH 4
#define ADS114S0X_REGISTER_INPMUX_MUXP_POS    4
#define ADS114S0X_REGISTER_INPMUX_MUXP_GET(value)                                                  \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_INPMUX_MUXP_POS,                    \
				     ADS114S0X_REGISTER_INPMUX_MUXP_LENGTH)
#define ADS114S0X_REGISTER_INPMUX_MUXP_SET(target, value)                                          \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_INPMUX_MUXP_POS,            \
				     ADS114S0X_REGISTER_INPMUX_MUXP_LENGTH)
#define ADS114S0X_REGISTER_INPMUX_MUXN_LENGTH 4
#define ADS114S0X_REGISTER_INPMUX_MUXN_POS    0
#define ADS114S0X_REGISTER_INPMUX_MUXN_GET(value)                                                  \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_INPMUX_MUXN_POS,                    \
				     ADS114S0X_REGISTER_INPMUX_MUXN_LENGTH)
#define ADS114S0X_REGISTER_INPMUX_MUXN_SET(target, value)                                          \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_INPMUX_MUXN_POS,            \
				     ADS114S0X_REGISTER_INPMUX_MUXN_LENGTH)
#define ADS114S0X_REGISTER_PGA_DELAY_LENGTH 3
#define ADS114S0X_REGISTER_PGA_DELAY_POS    5
#define ADS114S0X_REGISTER_PGA_DELAY_GET(value)                                                    \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_PGA_DELAY_POS,                      \
				     ADS114S0X_REGISTER_PGA_DELAY_LENGTH)
#define ADS114S0X_REGISTER_PGA_DELAY_SET(target, value)                                            \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_PGA_DELAY_POS,              \
				     ADS114S0X_REGISTER_PGA_DELAY_LENGTH)
#define ADS114S0X_REGISTER_PGA_PGA_EN_LENGTH 2
#define ADS114S0X_REGISTER_PGA_PGA_EN_POS    3
#define ADS114S0X_REGISTER_PGA_PGA_EN_GET(value)                                                   \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_PGA_PGA_EN_POS,                     \
				     ADS114S0X_REGISTER_PGA_PGA_EN_LENGTH)
#define ADS114S0X_REGISTER_PGA_PGA_EN_SET(target, value)                                           \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_PGA_PGA_EN_POS,             \
				     ADS114S0X_REGISTER_PGA_PGA_EN_LENGTH)
#define ADS114S0X_REGISTER_PGA_GAIN_LENGTH 3
#define ADS114S0X_REGISTER_PGA_GAIN_POS	   0
#define ADS114S0X_REGISTER_PGA_GAIN_GET(value)                                                     \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_PGA_GAIN_POS,                       \
				     ADS114S0X_REGISTER_PGA_GAIN_LENGTH)
#define ADS114S0X_REGISTER_PGA_GAIN_SET(target, value)                                             \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_PGA_GAIN_POS,               \
				     ADS114S0X_REGISTER_PGA_GAIN_LENGTH)
#define ADS114S0X_REGISTER_DATARATE_G_CHOP_LENGTH 1
#define ADS114S0X_REGISTER_DATARATE_G_CHOP_POS	  7
#define ADS114S0X_REGISTER_DATARATE_G_CHOP_GET(value)                                              \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_DATARATE_G_CHOP_POS,                \
				     ADS114S0X_REGISTER_DATARATE_G_CHOP_LENGTH)
#define ADS114S0X_REGISTER_DATARATE_G_CHOP_SET(target, value)                                      \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_DATARATE_G_CHOP_POS,        \
				     ADS114S0X_REGISTER_DATARATE_G_CHOP_LENGTH)
#define ADS114S0X_REGISTER_DATARATE_CLK_LENGTH 1
#define ADS114S0X_REGISTER_DATARATE_CLK_POS    6
#define ADS114S0X_REGISTER_DATARATE_CLK_GET(value)                                                 \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_DATARATE_CLK_POS,                   \
				     ADS114S0X_REGISTER_DATARATE_CLK_LENGTH)
#define ADS114S0X_REGISTER_DATARATE_CLK_SET(target, value)                                         \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_DATARATE_CLK_POS,           \
				     ADS114S0X_REGISTER_DATARATE_CLK_LENGTH)
#define ADS114S0X_REGISTER_DATARATE_MODE_LENGTH 1
#define ADS114S0X_REGISTER_DATARATE_MODE_POS	5
#define ADS114S0X_REGISTER_DATARATE_MODE_GET(value)                                                \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_DATARATE_MODE_POS,                  \
				     ADS114S0X_REGISTER_DATARATE_MODE_LENGTH)
#define ADS114S0X_REGISTER_DATARATE_MODE_SET(target, value)                                        \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_DATARATE_MODE_POS,          \
				     ADS114S0X_REGISTER_DATARATE_MODE_LENGTH)
#define ADS114S0X_REGISTER_DATARATE_FILTER_LENGTH 1
#define ADS114S0X_REGISTER_DATARATE_FILTER_POS	  4
#define ADS114S0X_REGISTER_DATARATE_FILTER_GET(value)                                              \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_DATARATE_FILTER_POS,                \
				     ADS114S0X_REGISTER_DATARATE_FILTER_LENGTH)
#define ADS114S0X_REGISTER_DATARATE_FILTER_SET(target, value)                                      \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_DATARATE_FILTER_POS,        \
				     ADS114S0X_REGISTER_DATARATE_FILTER_LENGTH)
#define ADS114S0X_REGISTER_DATARATE_DR_LENGTH 4
#define ADS114S0X_REGISTER_DATARATE_DR_POS    0
#define ADS114S0X_REGISTER_DATARATE_DR_GET(value)                                                  \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_DATARATE_DR_POS,                    \
				     ADS114S0X_REGISTER_DATARATE_DR_LENGTH)
#define ADS114S0X_REGISTER_DATARATE_DR_SET(target, value)                                          \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_DATARATE_DR_POS,            \
				     ADS114S0X_REGISTER_DATARATE_DR_LENGTH)
#define ADS114S0X_REGISTER_REF_FL_REF_EN_LENGTH 2
#define ADS114S0X_REGISTER_REF_FL_REF_EN_POS	6
#define ADS114S0X_REGISTER_REF_FL_REF_EN_GET(value)                                                \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_REF_FL_REF_EN_POS,                  \
				     ADS114S0X_REGISTER_REF_FL_REF_EN_LENGTH)
#define ADS114S0X_REGISTER_REF_FL_REF_EN_SET(target, value)                                        \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_REF_FL_REF_EN_POS,          \
				     ADS114S0X_REGISTER_REF_FL_REF_EN_LENGTH)
#define ADS114S0X_REGISTER_REF_NOT_REFP_BUF_LENGTH 1
#define ADS114S0X_REGISTER_REF_NOT_REFP_BUF_POS	   5
#define ADS114S0X_REGISTER_REF_NOT_REFP_BUF_GET(value)                                             \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_REF_NOT_REFP_BUF_POS,               \
				     ADS114S0X_REGISTER_REF_NOT_REFP_BUF_LENGTH)
#define ADS114S0X_REGISTER_REF_NOT_REFP_BUF_SET(target, value)                                     \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_REF_NOT_REFP_BUF_POS,       \
				     ADS114S0X_REGISTER_REF_NOT_REFP_BUF_LENGTH)
#define ADS114S0X_REGISTER_REF_NOT_REFN_BUF_LENGTH 1
#define ADS114S0X_REGISTER_REF_NOT_REFN_BUF_POS	   4
#define ADS114S0X_REGISTER_REF_NOT_REFN_BUF_GET(value)                                             \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_REF_NOT_REFN_BUF_POS,               \
				     ADS114S0X_REGISTER_REF_NOT_REFN_BUF_LENGTH)
#define ADS114S0X_REGISTER_REF_NOT_REFN_BUF_SET(target, value)                                     \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_REF_NOT_REFN_BUF_POS,       \
				     ADS114S0X_REGISTER_REF_NOT_REFN_BUF_LENGTH)
#define ADS114S0X_REGISTER_REF_REFSEL_LENGTH 2
#define ADS114S0X_REGISTER_REF_REFSEL_POS    2
#define ADS114S0X_REGISTER_REF_REFSEL_GET(value)                                                   \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_REF_REFSEL_POS,                     \
				     ADS114S0X_REGISTER_REF_REFSEL_LENGTH)
#define ADS114S0X_REGISTER_REF_REFSEL_SET(target, value)                                           \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_REF_REFSEL_POS,             \
				     ADS114S0X_REGISTER_REF_REFSEL_LENGTH)
#define ADS114S0X_REGISTER_REF_REFCON_LENGTH 2
#define ADS114S0X_REGISTER_REF_REFCON_POS    0
#define ADS114S0X_REGISTER_REF_REFCON_GET(value)                                                   \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_REF_REFCON_POS,                     \
				     ADS114S0X_REGISTER_REF_REFCON_LENGTH)
#define ADS114S0X_REGISTER_REF_REFCON_SET(target, value)                                           \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_REF_REFCON_POS,             \
				     ADS114S0X_REGISTER_REF_REFCON_LENGTH)

/*
 * - AIN0 as positive input
 * - AIN1 as negative input
 */
#define ADS114S0X_REGISTER_INPMUX_SET_DEFAULTS(target)                                             \
	ADS114S0X_REGISTER_INPMUX_MUXP_SET(target, 0b0000);                                        \
	ADS114S0X_REGISTER_INPMUX_MUXN_SET(target, 0b0001)
/*
 * - disable reference monitor
 * - enable positive reference buffer
 * - disable negative reference buffer
 * - use internal reference
 * - enable internal voltage reference
 */
#define ADS114S0X_REGISTER_REF_SET_DEFAULTS(target)                                                \
	ADS114S0X_REGISTER_REF_FL_REF_EN_SET(target, 0b00);                                        \
	ADS114S0X_REGISTER_REF_NOT_REFP_BUF_SET(target, 0b0);                                      \
	ADS114S0X_REGISTER_REF_NOT_REFN_BUF_SET(target, 0b1);                                      \
	ADS114S0X_REGISTER_REF_REFSEL_SET(target, 0b10);                                           \
	ADS114S0X_REGISTER_REF_REFCON_SET(target, 0b01)
/*
 * - disable global chop
 * - use internal oscillator
 * - single shot conversion mode
 * - low latency filter
 * - 20 samples per second
 */
#define ADS114S0X_REGISTER_DATARATE_SET_DEFAULTS(target)                                           \
	ADS114S0X_REGISTER_DATARATE_G_CHOP_SET(target, 0b0);                                       \
	ADS114S0X_REGISTER_DATARATE_CLK_SET(target, 0b0);                                          \
	ADS114S0X_REGISTER_DATARATE_MODE_SET(target, 0b1);                                         \
	ADS114S0X_REGISTER_DATARATE_FILTER_SET(target, 0b1);                                       \
	ADS114S0X_REGISTER_DATARATE_DR_SET(target, 0b0100)
/*
 * - delay of 14*t_mod
 * - disable gain
 * - gain 1
 */
#define ADS114S0X_REGISTER_PGA_SET_DEFAULTS(target)                                                \
	ADS114S0X_REGISTER_PGA_DELAY_SET(target, 0b000);                                           \
	ADS114S0X_REGISTER_PGA_PGA_EN_SET(target, 0b00);                                           \
	ADS114S0X_REGISTER_PGA_GAIN_SET(target, 0b000)

struct ads114s0x_config {
	struct spi_dt_spec bus;
#if CONFIG_ADC_ASYNC
	k_thread_stack_t *stack;
#endif
	const struct gpio_dt_spec gpio_reset;
	const struct gpio_dt_spec gpio_data_ready;
	const struct gpio_dt_spec gpio_start_sync;
};

struct ads114s0x_data {
	struct adc_context ctx;
#if CONFIG_ADC_ASYNC
	struct k_thread thread;
#endif
	struct gpio_callback callback_data_ready;
	struct k_sem data_ready_signal;
	struct k_sem acquire_signal;
	int16_t *buffer;
	int16_t *buffer_ptr;
};

static void ads114s0x_data_ready_handler(const struct device *dev, struct gpio_callback *gpio_cb,
					 uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pins);

	struct ads114s0x_data *data =
		CONTAINER_OF(gpio_cb, struct ads114s0x_data, callback_data_ready);

	k_sem_give(&data->data_ready_signal);
}

static int ads114s0x_read_register(const struct spi_dt_spec *bus,
				   enum ads114s0x_register register_address, uint8_t *value)
{
	uint8_t buffer_tx[3];
	uint8_t buffer_rx[ARRAY_SIZE(buffer_tx)];
	const struct spi_buf tx_buf[] = {{
		.buf = buffer_tx,
		.len = ARRAY_SIZE(buffer_tx),
	}};
	const struct spi_buf rx_buf[] = {{
		.buf = buffer_rx,
		.len = ARRAY_SIZE(buffer_rx),
	}};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};

	buffer_tx[0] = ((uint8_t)ADS114S0X_COMMAND_RREG) | ((uint8_t)register_address);
	/* read one register */
	buffer_tx[1] = 0x00;

	int result = spi_transceive_dt(bus, &tx, &rx);

	if (result != 0) {
		LOG_ERR("spi_transceive failed with error %i", result);
		return result;
	}

	*value = buffer_rx[2];
	LOG_DBG("read from register 0x%02X value 0x%02X", register_address, *value);

	return 0;
}

static int ads114s0x_write_register(const struct spi_dt_spec *bus,
				    enum ads114s0x_register register_address, uint8_t value)
{
	uint8_t buffer_tx[3];
	const struct spi_buf tx_buf[] = {{
		.buf = buffer_tx,
		.len = ARRAY_SIZE(buffer_tx),
	}};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};

	buffer_tx[0] = ((uint8_t)ADS114S0X_COMMAND_WREG) | ((uint8_t)register_address);
	/* write one register */
	buffer_tx[1] = 0x00;
	buffer_tx[2] = value;

	LOG_DBG("writing to register 0x%02X value 0x%02X", register_address, value);
	int result = spi_write_dt(bus, &tx);

	if (result != 0) {
		LOG_ERR("spi_write failed with error %i", result);
		return result;
	}

	return 0;
}

static int ads114s0x_write_multiple_registers(const struct spi_dt_spec *bus,
					      enum ads114s0x_register *register_addresses,
					      uint8_t *values, size_t count)
{
	uint8_t buffer_tx[2];
	const struct spi_buf tx_buf[] = {
		{
			.buf = buffer_tx,
			.len = ARRAY_SIZE(buffer_tx),
		},
		{
			.buf = values,
			.len = count,
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};

	if (count == 0) {
		LOG_WRN("ignoring the command to write 0 registers");
		return -EINVAL;
	}

	buffer_tx[0] = ((uint8_t)ADS114S0X_COMMAND_WREG) | ((uint8_t)register_addresses[0]);
	buffer_tx[1] = count - 1;

	LOG_HEXDUMP_DBG(register_addresses, count, "writing to registers");
	LOG_HEXDUMP_DBG(values, count, "values");

	/* ensure that the register addresses are in the correct order */
	for (size_t i = 1; i < count; ++i) {
		__ASSERT(register_addresses[i - 1] + 1 == register_addresses[i],
			 "register addresses are not consecutive");
	}

	int result = spi_write_dt(bus, &tx);

	if (result != 0) {
		LOG_ERR("spi_write failed with error %i", result);
		return result;
	}

	return 0;
}

static int ads114s0x_send_command(const struct spi_dt_spec *bus, enum ads114s0x_command command)
{
	uint8_t buffer_tx[1];
	const struct spi_buf tx_buf[] = {{
		.buf = buffer_tx,
		.len = ARRAY_SIZE(buffer_tx),
	}};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};

	buffer_tx[0] = (uint8_t)command;

	LOG_DBG("sending command 0x%02X", command);
	int result = spi_write_dt(bus, &tx);

	if (result != 0) {
		LOG_ERR("spi_write failed with error %i", result);
		return result;
	}

	return 0;
}

static int ads114s0x_channel_setup(const struct device *dev,
				   const struct adc_channel_cfg *channel_cfg)
{
	const struct ads114s0x_config *config = dev->config;
	uint8_t input_mux = 0;
	uint8_t reference_control = 0;
	uint8_t data_rate = 0;
	uint8_t gain = 0;
	int result;
	enum ads114s0x_register register_addresses[4];
	uint8_t values[ARRAY_SIZE(register_addresses)];

	ADS114S0X_REGISTER_INPMUX_SET_DEFAULTS(gain);
	ADS114S0X_REGISTER_REF_SET_DEFAULTS(reference_control);
	ADS114S0X_REGISTER_DATARATE_SET_DEFAULTS(data_rate);
	ADS114S0X_REGISTER_PGA_SET_DEFAULTS(gain);

	if (channel_cfg->channel_id != 0) {
		LOG_ERR("only one channel is supported");
		return -EINVAL;
	}

	switch (channel_cfg->reference) {
	case ADC_REF_INTERNAL:
		/* disable negative reference buffer */
		ADS114S0X_REGISTER_REF_NOT_REFN_BUF_SET(reference_control, 0b1);
		/* disable positive reference buffer */
		ADS114S0X_REGISTER_REF_NOT_REFP_BUF_SET(reference_control, 0b1);
		/* use internal reference */
		ADS114S0X_REGISTER_REF_REFSEL_SET(reference_control, 0b10);
		break;
	case ADC_REF_EXTERNAL0:
		/* enable negative reference buffer */
		ADS114S0X_REGISTER_REF_NOT_REFN_BUF_SET(reference_control, 0b0);
		/* enable positive reference buffer */
		ADS114S0X_REGISTER_REF_NOT_REFP_BUF_SET(reference_control, 0b0);
		/* use external reference 0*/
		ADS114S0X_REGISTER_REF_REFSEL_SET(reference_control, 0b00);
		break;
	case ADC_REF_EXTERNAL1:
		/* enable negative reference buffer */
		ADS114S0X_REGISTER_REF_NOT_REFN_BUF_SET(reference_control, 0b0);
		/* enable positive reference buffer */
		ADS114S0X_REGISTER_REF_NOT_REFP_BUF_SET(reference_control, 0b0);
		/* use external reference 0*/
		ADS114S0X_REGISTER_REF_REFSEL_SET(reference_control, 0b01);
		break;
	default:
		LOG_ERR("reference %i is not supported", channel_cfg->reference);
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		if (channel_cfg->input_positive >= ADS114S0X_INPUT_SELECTION_AINCOM) {
			LOG_ERR("positive channel input %i is invalid",
				channel_cfg->input_positive);
			return -EINVAL;
		}

		if (channel_cfg->input_negative >= ADS114S0X_INPUT_SELECTION_AINCOM) {
			LOG_ERR("negative channel input %i is invalid",
				channel_cfg->input_negative);
			return -EINVAL;
		}

		if (channel_cfg->input_positive == channel_cfg->input_negative) {
			LOG_ERR("negative and positive channel inputs must be different");
			return -EINVAL;
		}

		ADS114S0X_REGISTER_INPMUX_MUXP_SET(input_mux, channel_cfg->input_positive);
		ADS114S0X_REGISTER_INPMUX_MUXN_SET(input_mux, channel_cfg->input_negative);
	} else {
		if (channel_cfg->input_positive >= ADS114S0X_INPUT_SELECTION_AINCOM) {
			LOG_ERR("channel input %i is invalid", channel_cfg->input_positive);
			return -EINVAL;
		}

		ADS114S0X_REGISTER_INPMUX_MUXP_SET(input_mux, channel_cfg->input_positive);
		ADS114S0X_REGISTER_INPMUX_MUXN_SET(input_mux, ADS114S0X_INPUT_SELECTION_AINCOM);
	}

	switch (channel_cfg->gain) {
	case ADC_GAIN_1:
		/* set gain value */
		ADS114S0X_REGISTER_PGA_GAIN_SET(gain, 0b000);
		break;
	case ADC_GAIN_2:
		ADS114S0X_REGISTER_PGA_GAIN_SET(gain, 0b001);
		break;
	case ADC_GAIN_4:
		ADS114S0X_REGISTER_PGA_GAIN_SET(gain, 0b010);
		break;
	case ADC_GAIN_8:
		ADS114S0X_REGISTER_PGA_GAIN_SET(gain, 0b011);
		break;
	case ADC_GAIN_16:
		ADS114S0X_REGISTER_PGA_GAIN_SET(gain, 0b100);
		break;
	case ADC_GAIN_32:
		ADS114S0X_REGISTER_PGA_GAIN_SET(gain, 0b101);
		break;
	case ADC_GAIN_64:
		ADS114S0X_REGISTER_PGA_GAIN_SET(gain, 0b110);
		break;
	case ADC_GAIN_128:
		ADS114S0X_REGISTER_PGA_GAIN_SET(gain, 0b111);
		break;
	default:
		LOG_ERR("gain value %i not supported", channel_cfg->gain);
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		/* enable gain */
		ADS114S0X_REGISTER_PGA_PGA_EN_SET(gain, 0b01);
	}

	register_addresses[0] = ADS114S0X_REGISTER_INPMUX;
	register_addresses[1] = ADS114S0X_REGISTER_PGA;
	register_addresses[2] = ADS114S0X_REGISTER_DATARATE;
	register_addresses[3] = ADS114S0X_REGISTER_REF;
	BUILD_ASSERT(ARRAY_SIZE(register_addresses) == 4);
	values[0] = input_mux;
	values[1] = gain;
	values[2] = data_rate;
	values[3] = reference_control;
	BUILD_ASSERT(ARRAY_SIZE(values) == 4);

	result = ads114s0x_write_multiple_registers(&config->bus, register_addresses, values,
						    ARRAY_SIZE(values));

	if (result != 0) {
		LOG_ERR("unable to configure registers");
		return result;
	}

	return 0;
}

static int ads114s0x_validate_buffer_size(const struct adc_sequence *sequence)
{
	size_t needed = sizeof(int16_t);

	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

static int ads114s0x_validate_sequence(const struct device *dev,
				       const struct adc_sequence *sequence)
{
	if (sequence->resolution != ADS114S0X_RESOLUTION) {
		LOG_ERR("invalid resolution");
		return -EINVAL;
	}

	if (sequence->channels != BIT(0)) {
		LOG_ERR("invalid channel");
		return -EINVAL;
	}

	if (sequence->oversampling) {
		LOG_ERR("oversampling is not supported");
		return -EINVAL;
	}

	return ads114s0x_validate_buffer_size(sequence);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct ads114s0x_data *data = CONTAINER_OF(ctx, struct ads114s0x_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->buffer_ptr;
	}
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct ads114s0x_data *data = CONTAINER_OF(ctx, struct ads114s0x_data, ctx);

	data->buffer_ptr = data->buffer;
	k_sem_give(&data->acquire_signal);
}

static int ads114s0x_adc_start_read(const struct device *dev, const struct adc_sequence *sequence,
				    bool wait)
{
	int result;
	struct ads114s0x_data *data = dev->data;

	result = ads114s0x_validate_sequence(dev, sequence);

	if (result != 0) {
		LOG_ERR("sequence validation failed");
		return result;
	}

	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);

	if (wait) {
		result = adc_context_wait_for_completion(&data->ctx);
	}

	return result;
}

static int ads114s0x_send_start_read(const struct device *dev)
{
	const struct ads114s0x_config *config = dev->config;
	int result;

	if (config->gpio_start_sync.port == 0) {
		result = ads114s0x_send_command(&config->bus, ADS114S0X_COMMAND_START);
		if (result != 0) {
			LOG_ERR("unable to send START/SYNC command");
			return result;
		}
	} else {
		result = gpio_pin_set_dt(&config->gpio_start_sync, 1);

		if (result != 0) {
			LOG_ERR("unable to start ADC operation");
			return result;
		}

		k_sleep(K_USEC(ADS114S0X_START_SYNC_PULSE_DURATION_IN_US +
			       ADS114S0X_SETUP_TIME_IN_US));
		gpio_pin_set_dt(&config->gpio_start_sync, 0);

		if (result != 0) {
			LOG_ERR("unable to start ADC operation");
			return result;
		}
	}

	return 0;
}

static int ads114s0x_wait_data_ready(const struct device *dev)
{
	struct ads114s0x_data *data = dev->data;

	return k_sem_take(&data->data_ready_signal, K_FOREVER);
}

static int ads114s0x_read_sample(const struct device *dev, uint16_t *buffer)
{
	const struct ads114s0x_config *config = dev->config;
	uint8_t buffer_tx[3];
	uint8_t buffer_rx[ARRAY_SIZE(buffer_tx)];
	const struct spi_buf tx_buf[] = {{
		.buf = buffer_tx,
		.len = ARRAY_SIZE(buffer_tx),
	}};
	const struct spi_buf rx_buf[] = {{
		.buf = buffer_rx,
		.len = ARRAY_SIZE(buffer_rx),
	}};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};

	buffer_tx[0] = (uint8_t)ADS114S0X_COMMAND_RDATA;

	int result = spi_transceive_dt(&config->bus, &tx, &rx);

	if (result != 0) {
		LOG_ERR("spi_transceive failed with error %i", result);
		return result;
	}

	*buffer = sys_get_be16(buffer_rx + 1);

	return 0;
}

static int ads114s0x_adc_perform_read(const struct device *dev)
{
	int result;
	struct ads114s0x_data *data = dev->data;

	k_sem_take(&data->acquire_signal, K_FOREVER);

	result = ads114s0x_send_start_read(dev);
	if (result != 0) {
		LOG_ERR("unable to start ADC conversion");
		adc_context_complete(&data->ctx, result);
		return result;
	}

	result = ads114s0x_wait_data_ready(dev);
	if (result != 0) {
		LOG_ERR("waiting for data to be ready failed");
		adc_context_complete(&data->ctx, result);
		return result;
	}

	result = ads114s0x_read_sample(dev, data->buffer);
	if (result != 0) {
		LOG_ERR("reading sample failed");
		adc_context_complete(&data->ctx, result);
		return result;
	}

	data->buffer++;

	adc_context_on_sampling_done(&data->ctx, dev);

	return result;
}

#if CONFIG_ADC_ASYNC
static int ads114s0x_adc_read_async(const struct device *dev, const struct adc_sequence *sequence,
				    struct k_poll_signal *async)
{
	int result;
	struct ads114s0x_data *data = dev->data;

	adc_context_lock(&data->ctx, true, async);
	result = ads114s0x_adc_start_read(dev, sequence, true);
	adc_context_release(&data->ctx, result);

	return result;
}

static int ads114s0x_read(const struct device *dev, const struct adc_sequence *sequence)
{
	int result;
	struct ads114s0x_data *data = dev->data;

	adc_context_lock(&data->ctx, false, NULL);
	result = ads114s0x_adc_start_read(dev, sequence, true);
	adc_context_release(&data->ctx, result);

	return result;
}

#else
static int ads114s0x_read(const struct device *dev, const struct adc_sequence *sequence)
{
	int result;
	struct ads114s0x_data *data = dev->data;

	adc_context_lock(&data->ctx, false, NULL);
	result = ads114s0x_adc_start_read(dev, sequence, false);

	while (result == 0 && k_sem_take(&data->ctx.sync, K_NO_WAIT) != 0) {
		result = ads114s0x_adc_perform_read(dev);
	}

	adc_context_release(&data->ctx, result);
	return result;
}
#endif

#if CONFIG_ADC_ASYNC
static void ads114s0x_acquisition_thread(struct device *dev)
{
	while (true) {
		ads114s0x_adc_perform_read(dev);
	}
}
#endif

static int ads114s0x_init(const struct device *dev)
{
	uint8_t status = 0;
	uint8_t reference_control = 0;
	uint8_t reference_control_read;
	int result;
	const struct ads114s0x_config *config = dev->config;
	struct ads114s0x_data *data = dev->data;

	adc_context_init(&data->ctx);

	k_sem_init(&data->data_ready_signal, 0, 1);
	k_sem_init(&data->acquire_signal, 0, 1);

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI device is not ready");
		return -ENODEV;
	}

	if (config->gpio_reset.port != NULL) {
		result = gpio_pin_configure_dt(&config->gpio_reset, GPIO_OUTPUT_ACTIVE);
		if (result != 0) {
			LOG_ERR("failed to initialize GPIO for reset");
			return result;
		}
	}

	if (config->gpio_start_sync.port != NULL) {
		result = gpio_pin_configure_dt(&config->gpio_start_sync, GPIO_OUTPUT_INACTIVE);
		if (result != 0) {
			LOG_ERR("failed to initialize GPIO for start/sync");
			return result;
		}
	}

	result = gpio_pin_configure_dt(&config->gpio_data_ready, GPIO_INPUT);
	if (result != 0) {
		LOG_ERR("failed to initialize GPIO for data ready");
		return result;
	}

	result = gpio_pin_interrupt_configure_dt(&config->gpio_data_ready, GPIO_INT_EDGE_TO_ACTIVE);
	if (result != 0) {
		LOG_ERR("failed to configure data ready interrupt");
		return -EIO;
	}

	gpio_init_callback(&data->callback_data_ready, ads114s0x_data_ready_handler,
			   BIT(config->gpio_data_ready.pin));
	result = gpio_add_callback(config->gpio_data_ready.port, &data->callback_data_ready);
	if (result != 0) {
		LOG_ERR("failed to add data ready callback");
		return -EIO;
	}

#if CONFIG_ADC_ASYNC
	const k_tid_t tid = k_thread_create(
		&data->thread, config->stack, CONFIG_ADC_ADS114S0X_ACQUISITION_THREAD_STACK_SIZE,
		(k_thread_entry_t)ads114s0x_acquisition_thread, (void *)dev, NULL, NULL,
		CONFIG_ADC_ADS114S0X_ASYNC_THREAD_INIT_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(tid, "adc_ads114s0x");
#endif

	k_busy_wait(ADS114S0X_POWER_ON_RESET_TIME_IN_US);

	if (config->gpio_reset.port == NULL) {
		result = ads114s0x_send_command(&config->bus, ADS114S0X_COMMAND_RESET);
		if (result != 0) {
			LOG_ERR("unable to send RESET command");
			return result;
		}
	} else {
		k_busy_wait(ADS114S0X_RESET_LOW_TIME_IN_US);
		gpio_pin_set_dt(&config->gpio_reset, 0);
	}

	k_busy_wait(ADS114S0X_RESET_DELAY_TIME_IN_US);

	result = ads114s0x_read_register(&config->bus, ADS114S0X_REGISTER_STATUS, &status);
	if (result != 0) {
		LOG_ERR("unable to read status register");
		return result;
	}

	if (ADS114S0X_REGISTER_STATUS_NOT_RDY_GET(status) == 0x01) {
		LOG_ERR("ADS114 is not yet ready");
		return -EBUSY;
	}

	/*
	 * Activate internal voltage reference during initialization to
	 * avoid the necessary setup time for it to settle later on.
	 */
	ADS114S0X_REGISTER_REF_SET_DEFAULTS(reference_control);

	result = ads114s0x_write_register(&config->bus, ADS114S0X_REGISTER_REF, reference_control);
	if (result != 0) {
		LOG_ERR("unable to set default reference control values");
		return result;
	}

	/*
	 * Ensure that the internal voltage reference is active.
	 */
	result = ads114s0x_read_register(&config->bus, ADS114S0X_REGISTER_REF,
					 &reference_control_read);
	if (result != 0) {
		LOG_ERR("unable to read reference control values");
		return result;
	}

	if (reference_control != reference_control_read) {
		LOG_ERR("reference control register is incorrect: 0x%02X", reference_control_read);
		return -EIO;
	}

	adc_context_unlock_unconditionally(&data->ctx);

	return result;
}

static const struct adc_driver_api api = {
	.channel_setup = ads114s0x_channel_setup,
	.read = ads114s0x_read,
	.ref_internal = ADS114S0X_REF_INTERNAL,
#ifdef CONFIG_ADC_ASYNC
	.read_async = ads114s0x_adc_read_async,
#endif
};

BUILD_ASSERT(CONFIG_ADC_INIT_PRIORITY > CONFIG_SPI_INIT_PRIORITY,
	     "CONFIG_ADC_INIT_PRIORITY must be higher than CONFIG_SPI_INIT_PRIORITY");

#define DT_DRV_COMPAT ti_ads114s08

#define ADC_ADS114S0X_INST_DEFINE(n)                                                               \
	IF_ENABLED(                                                                                \
		CONFIG_ADC_ASYNC,                                                                  \
		(static K_KERNEL_STACK_DEFINE(                                                     \
			 thread_stack_##n, CONFIG_ADC_ADS114S0X_ACQUISITION_THREAD_STACK_SIZE);))  \
	static const struct ads114s0x_config config_##n = {                                        \
		.bus = SPI_DT_SPEC_INST_GET(                                                       \
			n, SPI_OP_MODE_MASTER | SPI_MODE_CPHA | SPI_WORD_SET(8), 0),               \
		IF_ENABLED(CONFIG_ADC_ASYNC, (.stack = thread_stack_##n,))                         \
		.gpio_reset = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),                       \
		.gpio_data_ready = GPIO_DT_SPEC_INST_GET(n, drdy_gpios),                           \
		.gpio_start_sync = GPIO_DT_SPEC_INST_GET_OR(n, start_sync_gpios, {0}),             \
	};                                                                                         \
	static struct ads114s0x_data data_##n;                                                     \
	DEVICE_DT_INST_DEFINE(n, ads114s0x_init, NULL, &data_##n, &config_##n, POST_KERNEL,        \
			      CONFIG_ADC_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(ADC_ADS114S0X_INST_DEFINE);
