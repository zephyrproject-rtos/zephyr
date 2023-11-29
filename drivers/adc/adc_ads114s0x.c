/*
 * Copyright (c) 2023 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/ads114s0x.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/adc/ads114s0x_adc.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER 1
#define ADC_CONTEXT_WAIT_FOR_COMPLETION_TIMEOUT                                                    \
	K_MSEC(CONFIG_ADC_ADS114S0X_WAIT_FOR_COMPLETION_TIMEOUT_MS)
#include "adc_context.h"

LOG_MODULE_REGISTER(ads114s0x, CONFIG_ADC_LOG_LEVEL);

#define ADS114S0X_CLK_FREQ_IN_KHZ                           4096
#define ADS114S0X_RESET_LOW_TIME_IN_CLOCK_CYCLES            4
#define ADS114S0X_START_SYNC_PULSE_DURATION_IN_CLOCK_CYCLES 4
#define ADS114S0X_SETUP_TIME_IN_CLOCK_CYCLES                32
#define ADS114S0X_INPUT_SELECTION_AINCOM                    12
#define ADS114S0X_RESOLUTION                                16
#define ADS114S0X_REF_INTERNAL                              2500
#define ADS114S0X_GPIO_MAX                                  3
#define ADS114S0X_POWER_ON_RESET_TIME_IN_US                 2200
#define ADS114S0X_VBIAS_PIN_MAX                             7
#define ADS114S0X_VBIAS_PIN_MIN                             0

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
#define ADS114S0X_REGISTER_STATUS_FL_POR_POS    7
#define ADS114S0X_REGISTER_STATUS_FL_POR_GET(value)                                                \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_STATUS_FL_POR_POS,                  \
				     ADS114S0X_REGISTER_STATUS_FL_POR_LENGTH)
#define ADS114S0X_REGISTER_STATUS_FL_POR_SET(target, value)                                        \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_STATUS_FL_POR_POS,          \
				     ADS114S0X_REGISTER_STATUS_FL_POR_LENGTH)
#define ADS114S0X_REGISTER_STATUS_NOT_RDY_LENGTH 1
#define ADS114S0X_REGISTER_STATUS_NOT_RDY_POS    6
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
#define ADS114S0X_REGISTER_STATUS_FL_REF_L1_POS    1
#define ADS114S0X_REGISTER_STATUS_FL_REF_L1_GET(value)                                             \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_STATUS_FL_REF_L1_POS,               \
				     ADS114S0X_REGISTER_STATUS_FL_REF_L1_LENGTH)
#define ADS114S0X_REGISTER_STATUS_FL_REF_L1_SET(target, value)                                     \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_STATUS_FL_REF_L1_POS,       \
				     ADS114S0X_REGISTER_STATUS_FL_REF_L1_LENGTH)
#define ADS114S0X_REGISTER_STATUS_FL_REF_L0_LENGTH 1
#define ADS114S0X_REGISTER_STATUS_FL_REF_L0_POS    0
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
#define ADS114S0X_REGISTER_PGA_GAIN_POS    0
#define ADS114S0X_REGISTER_PGA_GAIN_GET(value)                                                     \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_PGA_GAIN_POS,                       \
				     ADS114S0X_REGISTER_PGA_GAIN_LENGTH)
#define ADS114S0X_REGISTER_PGA_GAIN_SET(target, value)                                             \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_PGA_GAIN_POS,               \
				     ADS114S0X_REGISTER_PGA_GAIN_LENGTH)
#define ADS114S0X_REGISTER_DATARATE_G_CHOP_LENGTH 1
#define ADS114S0X_REGISTER_DATARATE_G_CHOP_POS    7
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
#define ADS114S0X_REGISTER_DATARATE_MODE_POS    5
#define ADS114S0X_REGISTER_DATARATE_MODE_GET(value)                                                \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_DATARATE_MODE_POS,                  \
				     ADS114S0X_REGISTER_DATARATE_MODE_LENGTH)
#define ADS114S0X_REGISTER_DATARATE_MODE_SET(target, value)                                        \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_DATARATE_MODE_POS,          \
				     ADS114S0X_REGISTER_DATARATE_MODE_LENGTH)
#define ADS114S0X_REGISTER_DATARATE_FILTER_LENGTH 1
#define ADS114S0X_REGISTER_DATARATE_FILTER_POS    4
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
#define ADS114S0X_REGISTER_REF_FL_REF_EN_POS    6
#define ADS114S0X_REGISTER_REF_FL_REF_EN_GET(value)                                                \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_REF_FL_REF_EN_POS,                  \
				     ADS114S0X_REGISTER_REF_FL_REF_EN_LENGTH)
#define ADS114S0X_REGISTER_REF_FL_REF_EN_SET(target, value)                                        \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_REF_FL_REF_EN_POS,          \
				     ADS114S0X_REGISTER_REF_FL_REF_EN_LENGTH)
#define ADS114S0X_REGISTER_REF_NOT_REFP_BUF_LENGTH 1
#define ADS114S0X_REGISTER_REF_NOT_REFP_BUF_POS    5
#define ADS114S0X_REGISTER_REF_NOT_REFP_BUF_GET(value)                                             \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_REF_NOT_REFP_BUF_POS,               \
				     ADS114S0X_REGISTER_REF_NOT_REFP_BUF_LENGTH)
#define ADS114S0X_REGISTER_REF_NOT_REFP_BUF_SET(target, value)                                     \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_REF_NOT_REFP_BUF_POS,       \
				     ADS114S0X_REGISTER_REF_NOT_REFP_BUF_LENGTH)
#define ADS114S0X_REGISTER_REF_NOT_REFN_BUF_LENGTH 1
#define ADS114S0X_REGISTER_REF_NOT_REFN_BUF_POS    4
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
#define ADS114S0X_REGISTER_IDACMAG_FL_RAIL_EN_LENGTH 1
#define ADS114S0X_REGISTER_IDACMAG_FL_RAIL_EN_POS    7
#define ADS114S0X_REGISTER_IDACMAG_FL_RAIL_EN_GET(value)                                           \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_IDACMAG_FL_RAIL_EN_POS,             \
				     ADS114S0X_REGISTER_IDACMAG_FL_RAIL_EN_LENGTH)
#define ADS114S0X_REGISTER_IDACMAG_FL_RAIL_EN_SET(target, value)                                   \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_IDACMAG_FL_RAIL_EN_POS,     \
				     ADS114S0X_REGISTER_IDACMAG_FL_RAIL_EN_LENGTH)
#define ADS114S0X_REGISTER_IDACMAG_PSW_LENGTH 1
#define ADS114S0X_REGISTER_IDACMAG_PSW_POS    6
#define ADS114S0X_REGISTER_IDACMAG_PSW_GET(value)                                                  \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_IDACMAG_PSW_POS,                    \
				     ADS114S0X_REGISTER_IDACMAG_PSW_LENGTH)
#define ADS114S0X_REGISTER_IDACMAG_PSW_SET(target, value)                                          \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_IDACMAG_PSW_POS,            \
				     ADS114S0X_REGISTER_IDACMAG_PSW_LENGTH)
#define ADS114S0X_REGISTER_IDACMAG_IMAG_LENGTH 4
#define ADS114S0X_REGISTER_IDACMAG_IMAG_POS    0
#define ADS114S0X_REGISTER_IDACMAG_IMAG_GET(value)                                                 \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_IDACMAG_IMAG_POS,                   \
				     ADS114S0X_REGISTER_IDACMAG_IMAG_LENGTH)
#define ADS114S0X_REGISTER_IDACMAG_IMAG_SET(target, value)                                         \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_IDACMAG_IMAG_POS,           \
				     ADS114S0X_REGISTER_IDACMAG_IMAG_LENGTH)
#define ADS114S0X_REGISTER_IDACMUX_I2MUX_LENGTH 4
#define ADS114S0X_REGISTER_IDACMUX_I2MUX_POS    4
#define ADS114S0X_REGISTER_IDACMUX_I2MUX_GET(value)                                                \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_IDACMUX_I2MUX_POS,                  \
				     ADS114S0X_REGISTER_IDACMUX_I2MUX_LENGTH)
#define ADS114S0X_REGISTER_IDACMUX_I2MUX_SET(target, value)                                        \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_IDACMUX_I2MUX_POS,          \
				     ADS114S0X_REGISTER_IDACMUX_I2MUX_LENGTH)
#define ADS114S0X_REGISTER_IDACMUX_I1MUX_LENGTH 4
#define ADS114S0X_REGISTER_IDACMUX_I1MUX_POS    0
#define ADS114S0X_REGISTER_IDACMUX_I1MUX_GET(value)                                                \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_IDACMUX_I1MUX_POS,                  \
				     ADS114S0X_REGISTER_IDACMUX_I1MUX_LENGTH)
#define ADS114S0X_REGISTER_IDACMUX_I1MUX_SET(target, value)                                        \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_IDACMUX_I1MUX_POS,          \
				     ADS114S0X_REGISTER_IDACMUX_I1MUX_LENGTH)
#define ADS114S0X_REGISTER_VBIAS_VB_LEVEL_LENGTH 1
#define ADS114S0X_REGISTER_VBIAS_VB_LEVEL_POS    7
#define ADS114S0X_REGISTER_VBIAS_VB_LEVEL_GET(value)                                               \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_VBIAS_VB_LEVEL_POS,                 \
				     ADS114S0X_REGISTER_VBIAS_VB_LEVEL_LENGTH)
#define ADS114S0X_REGISTER_VBIAS_VB_LEVEL_SET(target, value)                                       \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_VBIAS_VB_LEVEL_POS,         \
				     ADS114S0X_REGISTER_VBIAS_VB_LEVEL_LENGTH)
#define ADS114S0X_REGISTER_GPIODAT_DIR_LENGTH 4
#define ADS114S0X_REGISTER_GPIODAT_DIR_POS    4
#define ADS114S0X_REGISTER_GPIODAT_DIR_GET(value)                                                  \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_GPIODAT_DIR_POS,                    \
				     ADS114S0X_REGISTER_GPIODAT_DIR_LENGTH)
#define ADS114S0X_REGISTER_GPIODAT_DIR_SET(target, value)                                          \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_GPIODAT_DIR_POS,            \
				     ADS114S0X_REGISTER_GPIODAT_DIR_LENGTH)
#define ADS114S0X_REGISTER_GPIODAT_DAT_LENGTH 4
#define ADS114S0X_REGISTER_GPIODAT_DAT_POS    0
#define ADS114S0X_REGISTER_GPIODAT_DAT_GET(value)                                                  \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_GPIODAT_DAT_POS,                    \
				     ADS114S0X_REGISTER_GPIODAT_DAT_LENGTH)
#define ADS114S0X_REGISTER_GPIODAT_DAT_SET(target, value)                                          \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_GPIODAT_DAT_POS,            \
				     ADS114S0X_REGISTER_GPIODAT_DAT_LENGTH)
#define ADS114S0X_REGISTER_GPIOCON_CON_LENGTH 4
#define ADS114S0X_REGISTER_GPIOCON_CON_POS    0
#define ADS114S0X_REGISTER_GPIOCON_CON_GET(value)                                                  \
	ADS114S0X_REGISTER_GET_VALUE(value, ADS114S0X_REGISTER_GPIOCON_CON_POS,                    \
				     ADS114S0X_REGISTER_GPIOCON_CON_LENGTH)
#define ADS114S0X_REGISTER_GPIOCON_CON_SET(target, value)                                          \
	ADS114S0X_REGISTER_SET_VALUE(target, value, ADS114S0X_REGISTER_GPIOCON_CON_POS,            \
				     ADS114S0X_REGISTER_GPIOCON_CON_LENGTH)

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
/*
 * - disable PGA output rail flag
 * - low-side power switch
 * - IDAC off
 */
#define ADS114S0X_REGISTER_IDACMAG_SET_DEFAULTS(target)                                            \
	ADS114S0X_REGISTER_IDACMAG_FL_RAIL_EN_SET(target, 0b0);                                    \
	ADS114S0X_REGISTER_IDACMAG_PSW_SET(target, 0b0);                                           \
	ADS114S0X_REGISTER_IDACMAG_IMAG_SET(target, 0b0000)
/*
 * - disconnect IDAC1
 * - disconnect IDAC2
 */
#define ADS114S0X_REGISTER_IDACMUX_SET_DEFAULTS(target)                                            \
	ADS114S0X_REGISTER_IDACMUX_I1MUX_SET(target, 0b1111);                                      \
	ADS114S0X_REGISTER_IDACMUX_I2MUX_SET(target, 0b1111)

struct ads114s0x_config {
	struct spi_dt_spec bus;
#if CONFIG_ADC_ASYNC
	k_thread_stack_t *stack;
#endif
	const struct gpio_dt_spec gpio_reset;
	const struct gpio_dt_spec gpio_data_ready;
	const struct gpio_dt_spec gpio_start_sync;
	int idac_current;
	uint8_t vbias_level;
};

struct ads114s0x_data {
	struct adc_context ctx;
#if CONFIG_ADC_ASYNC
	struct k_thread thread;
#endif /* CONFIG_ADC_ASYNC */
	struct gpio_callback callback_data_ready;
	struct k_sem data_ready_signal;
	struct k_sem acquire_signal;
	int16_t *buffer;
	int16_t *buffer_ptr;
#if CONFIG_ADC_ADS114S0X_GPIO
	struct k_mutex gpio_lock;
	uint8_t gpio_enabled;   /* one bit per GPIO, 1 = enabled */
	uint8_t gpio_direction; /* one bit per GPIO, 1 = input */
	uint8_t gpio_value;     /* one bit per GPIO, 1 = high */
#endif                          /* CONFIG_ADC_ADS114S0X_GPIO */
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

static int ads114s0x_read_register(const struct device *dev,
				   enum ads114s0x_register register_address, uint8_t *value)
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

	buffer_tx[0] = ((uint8_t)ADS114S0X_COMMAND_RREG) | ((uint8_t)register_address);
	/* read one register */
	buffer_tx[1] = 0x00;

	int result = spi_transceive_dt(&config->bus, &tx, &rx);

	if (result != 0) {
		LOG_ERR("%s: spi_transceive failed with error %i", dev->name, result);
		return result;
	}

	*value = buffer_rx[2];
	LOG_DBG("%s: read from register 0x%02X value 0x%02X", dev->name, register_address, *value);

	return 0;
}

static int ads114s0x_write_register(const struct device *dev,
				    enum ads114s0x_register register_address, uint8_t value)
{
	const struct ads114s0x_config *config = dev->config;
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

	LOG_DBG("%s: writing to register 0x%02X value 0x%02X", dev->name, register_address, value);
	int result = spi_write_dt(&config->bus, &tx);

	if (result != 0) {
		LOG_ERR("%s: spi_write failed with error %i", dev->name, result);
		return result;
	}

	return 0;
}

static int ads114s0x_write_multiple_registers(const struct device *dev,
					      enum ads114s0x_register *register_addresses,
					      uint8_t *values, size_t count)
{
	const struct ads114s0x_config *config = dev->config;
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
		LOG_WRN("%s: ignoring the command to write 0 registers", dev->name);
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

	int result = spi_write_dt(&config->bus, &tx);

	if (result != 0) {
		LOG_ERR("%s: spi_write failed with error %i", dev->name, result);
		return result;
	}

	return 0;
}

static int ads114s0x_send_command(const struct device *dev, enum ads114s0x_command command)
{
	const struct ads114s0x_config *config = dev->config;
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

	LOG_DBG("%s: sending command 0x%02X", dev->name, command);
	int result = spi_write_dt(&config->bus, &tx);

	if (result != 0) {
		LOG_ERR("%s: spi_write failed with error %i", dev->name, result);
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
	uint8_t idac_magnitude = 0;
	uint8_t idac_mux = 0;
	uint8_t pin_selections[4];
	uint8_t vbias = 0;
	size_t pin_selections_size;
	int result;
	enum ads114s0x_register register_addresses[7];
	uint8_t values[ARRAY_SIZE(register_addresses)];
	uint16_t acquisition_time_value = ADC_ACQ_TIME_VALUE(channel_cfg->acquisition_time);
	uint16_t acquisition_time_unit = ADC_ACQ_TIME_UNIT(channel_cfg->acquisition_time);

	ADS114S0X_REGISTER_INPMUX_SET_DEFAULTS(gain);
	ADS114S0X_REGISTER_REF_SET_DEFAULTS(reference_control);
	ADS114S0X_REGISTER_DATARATE_SET_DEFAULTS(data_rate);
	ADS114S0X_REGISTER_PGA_SET_DEFAULTS(gain);
	ADS114S0X_REGISTER_IDACMAG_SET_DEFAULTS(idac_magnitude);
	ADS114S0X_REGISTER_IDACMUX_SET_DEFAULTS(idac_mux);

	if (channel_cfg->channel_id != 0) {
		LOG_ERR("%s: only one channel is supported", dev->name);
		return -EINVAL;
	}

	/* The ADS114 uses samples per seconds units with the lowest being 2.5SPS
	 * and with acquisition_time only having 14b for time, this will not fit
	 * within here for microsecond units. Use Tick units and allow the user to
	 * specify the ODR directly.
	 */
	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT &&
	    acquisition_time_unit != ADC_ACQ_TIME_TICKS) {
		LOG_ERR("%s: invalid acquisition time %i", dev->name,
			channel_cfg->acquisition_time);
		return -EINVAL;
	}

	if (channel_cfg->acquisition_time == ADC_ACQ_TIME_DEFAULT) {
		ADS114S0X_REGISTER_DATARATE_DR_SET(data_rate, ADS114S0X_CONFIG_DR_20);
	} else {
		ADS114S0X_REGISTER_DATARATE_DR_SET(data_rate, acquisition_time_value);
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
		LOG_ERR("%s: reference %i is not supported", dev->name, channel_cfg->reference);
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		LOG_DBG("%s: configuring channel for a differential measurement from the pins (p, "
			"n) (%i, %i)",
			dev->name, channel_cfg->input_positive, channel_cfg->input_negative);
		if (channel_cfg->input_positive >= ADS114S0X_INPUT_SELECTION_AINCOM) {
			LOG_ERR("%s: positive channel input %i is invalid", dev->name,
				channel_cfg->input_positive);
			return -EINVAL;
		}

		if (channel_cfg->input_negative >= ADS114S0X_INPUT_SELECTION_AINCOM) {
			LOG_ERR("%s: negative channel input %i is invalid", dev->name,
				channel_cfg->input_negative);
			return -EINVAL;
		}

		if (channel_cfg->input_positive == channel_cfg->input_negative) {
			LOG_ERR("%s: negative and positive channel inputs must be different",
				dev->name);
			return -EINVAL;
		}

		ADS114S0X_REGISTER_INPMUX_MUXP_SET(input_mux, channel_cfg->input_positive);
		ADS114S0X_REGISTER_INPMUX_MUXN_SET(input_mux, channel_cfg->input_negative);
		pin_selections[0] = channel_cfg->input_positive;
		pin_selections[1] = channel_cfg->input_negative;
	} else {
		LOG_DBG("%s: configuring channel for single ended measurement from input %i",
			dev->name, channel_cfg->input_positive);
		if (channel_cfg->input_positive >= ADS114S0X_INPUT_SELECTION_AINCOM) {
			LOG_ERR("%s: channel input %i is invalid", dev->name,
				channel_cfg->input_positive);
			return -EINVAL;
		}

		ADS114S0X_REGISTER_INPMUX_MUXP_SET(input_mux, channel_cfg->input_positive);
		ADS114S0X_REGISTER_INPMUX_MUXN_SET(input_mux, ADS114S0X_INPUT_SELECTION_AINCOM);
		pin_selections[0] = channel_cfg->input_positive;
		pin_selections[1] = ADS114S0X_INPUT_SELECTION_AINCOM;
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
		LOG_ERR("%s: gain value %i not supported", dev->name, channel_cfg->gain);
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		/* enable gain */
		ADS114S0X_REGISTER_PGA_PGA_EN_SET(gain, 0b01);
	}

	switch (config->idac_current) {
	case 0:
		ADS114S0X_REGISTER_IDACMAG_IMAG_SET(idac_magnitude, 0b0000);
		break;
	case 10:
		ADS114S0X_REGISTER_IDACMAG_IMAG_SET(idac_magnitude, 0b0001);
		break;
	case 50:
		ADS114S0X_REGISTER_IDACMAG_IMAG_SET(idac_magnitude, 0b0010);
		break;
	case 100:
		ADS114S0X_REGISTER_IDACMAG_IMAG_SET(idac_magnitude, 0b0011);
		break;
	case 250:
		ADS114S0X_REGISTER_IDACMAG_IMAG_SET(idac_magnitude, 0b0100);
		break;
	case 500:
		ADS114S0X_REGISTER_IDACMAG_IMAG_SET(idac_magnitude, 0b0101);
		break;
	case 750:
		ADS114S0X_REGISTER_IDACMAG_IMAG_SET(idac_magnitude, 0b0110);
		break;
	case 1000:
		ADS114S0X_REGISTER_IDACMAG_IMAG_SET(idac_magnitude, 0b0111);
		break;
	case 1500:
		ADS114S0X_REGISTER_IDACMAG_IMAG_SET(idac_magnitude, 0b1000);
		break;
	case 2000:
		ADS114S0X_REGISTER_IDACMAG_IMAG_SET(idac_magnitude, 0b1001);
		break;
	default:
		LOG_ERR("%s: IDAC magnitude %i not supported", dev->name, config->idac_current);
		return -EINVAL;
	}

	if (channel_cfg->current_source_pin_set) {
		LOG_DBG("%s: current source pin set to %i and %i", dev->name,
			channel_cfg->current_source_pin[0], channel_cfg->current_source_pin[1]);
		if (channel_cfg->current_source_pin[0] > 0b1111) {
			LOG_ERR("%s: invalid selection %i for I1MUX", dev->name,
				channel_cfg->current_source_pin[0]);
			return -EINVAL;
		}

		if (channel_cfg->current_source_pin[1] > 0b1111) {
			LOG_ERR("%s: invalid selection %i for I2MUX", dev->name,
				channel_cfg->current_source_pin[1]);
			return -EINVAL;
		}

		ADS114S0X_REGISTER_IDACMUX_I1MUX_SET(idac_mux, channel_cfg->current_source_pin[0]);
		ADS114S0X_REGISTER_IDACMUX_I2MUX_SET(idac_mux, channel_cfg->current_source_pin[1]);
		pin_selections[2] = channel_cfg->current_source_pin[0];
		pin_selections[3] = channel_cfg->current_source_pin[1];
		pin_selections_size = 4;
	} else {
		LOG_DBG("%s: current source pins not set", dev->name);
		pin_selections_size = 2;
	}

	for (size_t i = 0; i < pin_selections_size; ++i) {
		if (pin_selections[i] > ADS114S0X_INPUT_SELECTION_AINCOM) {
			continue;
		}

		for (size_t j = i + 1; j < pin_selections_size; ++j) {
			if (pin_selections[j] > ADS114S0X_INPUT_SELECTION_AINCOM) {
				continue;
			}

			if (pin_selections[i] == pin_selections[j]) {
				LOG_ERR("%s: pins for inputs and current sources must be different",
					dev->name);
				return -EINVAL;
			}
		}
	}

	ADS114S0X_REGISTER_VBIAS_VB_LEVEL_SET(vbias, config->vbias_level);

	if ((channel_cfg->vbias_pins &
	     ~GENMASK(ADS114S0X_VBIAS_PIN_MAX, ADS114S0X_VBIAS_PIN_MIN)) != 0) {
		LOG_ERR("%s: invalid VBIAS pin selection 0x%08X", dev->name,
			channel_cfg->vbias_pins);
		return -EINVAL;
	}

	vbias |= channel_cfg->vbias_pins;

	register_addresses[0] = ADS114S0X_REGISTER_INPMUX;
	register_addresses[1] = ADS114S0X_REGISTER_PGA;
	register_addresses[2] = ADS114S0X_REGISTER_DATARATE;
	register_addresses[3] = ADS114S0X_REGISTER_REF;
	register_addresses[4] = ADS114S0X_REGISTER_IDACMAG;
	register_addresses[5] = ADS114S0X_REGISTER_IDACMUX;
	register_addresses[6] = ADS114S0X_REGISTER_VBIAS;
	BUILD_ASSERT(ARRAY_SIZE(register_addresses) == 7);
	values[0] = input_mux;
	values[1] = gain;
	values[2] = data_rate;
	values[3] = reference_control;
	values[4] = idac_magnitude;
	values[5] = idac_mux;
	values[6] = vbias;
	BUILD_ASSERT(ARRAY_SIZE(values) == 7);

	result = ads114s0x_write_multiple_registers(dev, register_addresses, values,
						    ARRAY_SIZE(values));

	if (result != 0) {
		LOG_ERR("%s: unable to configure registers", dev->name);
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
		LOG_ERR("%s: invalid resolution", dev->name);
		return -EINVAL;
	}

	if (sequence->channels != BIT(0)) {
		LOG_ERR("%s: invalid channel", dev->name);
		return -EINVAL;
	}

	if (sequence->oversampling) {
		LOG_ERR("%s: oversampling is not supported", dev->name);
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
		LOG_ERR("%s: sequence validation failed", dev->name);
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
		result = ads114s0x_send_command(dev, ADS114S0X_COMMAND_START);
		if (result != 0) {
			LOG_ERR("%s: unable to send START/SYNC command", dev->name);
			return result;
		}
	} else {
		result = gpio_pin_set_dt(&config->gpio_start_sync, 1);

		if (result != 0) {
			LOG_ERR("%s: unable to start ADC operation", dev->name);
			return result;
		}

		k_sleep(K_USEC(ADS114S0X_START_SYNC_PULSE_DURATION_IN_US +
			       ADS114S0X_SETUP_TIME_IN_US));

		result = gpio_pin_set_dt(&config->gpio_start_sync, 0);

		if (result != 0) {
			LOG_ERR("%s: unable to start ADC operation", dev->name);
			return result;
		}
	}

	return 0;
}

static int ads114s0x_wait_data_ready(const struct device *dev)
{
	struct ads114s0x_data *data = dev->data;

	return k_sem_take(&data->data_ready_signal, ADC_CONTEXT_WAIT_FOR_COMPLETION_TIMEOUT);
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
		LOG_ERR("%s: spi_transceive failed with error %i", dev->name, result);
		return result;
	}

	*buffer = sys_get_be16(buffer_rx + 1);
	LOG_DBG("%s: read ADC sample %i", dev->name, *buffer);

	return 0;
}

static int ads114s0x_adc_perform_read(const struct device *dev)
{
	int result;
	struct ads114s0x_data *data = dev->data;

	k_sem_take(&data->acquire_signal, K_FOREVER);
	k_sem_reset(&data->data_ready_signal);

	result = ads114s0x_send_start_read(dev);
	if (result != 0) {
		LOG_ERR("%s: unable to start ADC conversion", dev->name);
		adc_context_complete(&data->ctx, result);
		return result;
	}

	result = ads114s0x_wait_data_ready(dev);
	if (result != 0) {
		LOG_ERR("%s: waiting for data to be ready failed", dev->name);
		adc_context_complete(&data->ctx, result);
		return result;
	}

	result = ads114s0x_read_sample(dev, data->buffer);
	if (result != 0) {
		LOG_ERR("%s: reading sample failed", dev->name);
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
static void ads114s0x_acquisition_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;
	while (true) {
		ads114s0x_adc_perform_read(dev);
	}
}
#endif

#ifdef CONFIG_ADC_ADS114S0X_GPIO
static int ads114s0x_gpio_write_config(const struct device *dev)
{
	struct ads114s0x_data *data = dev->data;
	enum ads114s0x_register register_addresses[2];
	uint8_t register_values[ARRAY_SIZE(register_addresses)];
	uint8_t gpio_dat = 0;
	uint8_t gpio_con = 0;

	ADS114S0X_REGISTER_GPIOCON_CON_SET(gpio_con, data->gpio_enabled);
	ADS114S0X_REGISTER_GPIODAT_DAT_SET(gpio_dat, data->gpio_value);
	ADS114S0X_REGISTER_GPIODAT_DIR_SET(gpio_dat, data->gpio_direction);

	register_values[0] = gpio_dat;
	register_values[1] = gpio_con;
	register_addresses[0] = ADS114S0X_REGISTER_GPIODAT;
	register_addresses[1] = ADS114S0X_REGISTER_GPIOCON;
	return ads114s0x_write_multiple_registers(dev, register_addresses, register_values,
						  ARRAY_SIZE(register_values));
}

static int ads114s0x_gpio_write_value(const struct device *dev)
{
	struct ads114s0x_data *data = dev->data;
	uint8_t gpio_dat = 0;

	ADS114S0X_REGISTER_GPIODAT_DAT_SET(gpio_dat, data->gpio_value);
	ADS114S0X_REGISTER_GPIODAT_DIR_SET(gpio_dat, data->gpio_direction);

	return ads114s0x_write_register(dev, ADS114S0X_REGISTER_GPIODAT, gpio_dat);
}

int ads114s0x_gpio_set_output(const struct device *dev, uint8_t pin, bool initial_value)
{
	struct ads114s0x_data *data = dev->data;
	int result = 0;

	if (pin > ADS114S0X_GPIO_MAX) {
		LOG_ERR("%s: invalid pin %i", dev->name, pin);
		return -EINVAL;
	}

	k_mutex_lock(&data->gpio_lock, K_FOREVER);

	data->gpio_enabled |= BIT(pin);
	data->gpio_direction &= ~BIT(pin);

	if (initial_value) {
		data->gpio_value |= BIT(pin);
	} else {
		data->gpio_value &= ~BIT(pin);
	}

	result = ads114s0x_gpio_write_config(dev);

	k_mutex_unlock(&data->gpio_lock);

	return result;
}

int ads114s0x_gpio_set_input(const struct device *dev, uint8_t pin)
{
	struct ads114s0x_data *data = dev->data;
	int result = 0;

	if (pin > ADS114S0X_GPIO_MAX) {
		LOG_ERR("%s: invalid pin %i", dev->name, pin);
		return -EINVAL;
	}

	k_mutex_lock(&data->gpio_lock, K_FOREVER);

	data->gpio_enabled |= BIT(pin);
	data->gpio_direction |= BIT(pin);
	data->gpio_value &= ~BIT(pin);

	result = ads114s0x_gpio_write_config(dev);

	k_mutex_unlock(&data->gpio_lock);

	return result;
}

int ads114s0x_gpio_deconfigure(const struct device *dev, uint8_t pin)
{
	struct ads114s0x_data *data = dev->data;
	int result = 0;

	if (pin > ADS114S0X_GPIO_MAX) {
		LOG_ERR("%s: invalid pin %i", dev->name, pin);
		return -EINVAL;
	}

	k_mutex_lock(&data->gpio_lock, K_FOREVER);

	data->gpio_enabled &= ~BIT(pin);
	data->gpio_direction |= BIT(pin);
	data->gpio_value &= ~BIT(pin);

	result = ads114s0x_gpio_write_config(dev);

	k_mutex_unlock(&data->gpio_lock);

	return result;
}

int ads114s0x_gpio_set_pin_value(const struct device *dev, uint8_t pin, bool value)
{
	struct ads114s0x_data *data = dev->data;
	int result = 0;

	if (pin > ADS114S0X_GPIO_MAX) {
		LOG_ERR("%s: invalid pin %i", dev->name, pin);
		return -EINVAL;
	}

	k_mutex_lock(&data->gpio_lock, K_FOREVER);

	if ((BIT(pin) & data->gpio_enabled) == 0) {
		LOG_ERR("%s: gpio pin %i not configured", dev->name, pin);
		result = -EINVAL;
	} else if ((BIT(pin) & data->gpio_direction) != 0) {
		LOG_ERR("%s: gpio pin %i not configured as output", dev->name, pin);
		result = -EINVAL;
	} else {
		data->gpio_value |= BIT(pin);

		result = ads114s0x_gpio_write_value(dev);
	}

	k_mutex_unlock(&data->gpio_lock);

	return result;
}

int ads114s0x_gpio_get_pin_value(const struct device *dev, uint8_t pin, bool *value)
{
	struct ads114s0x_data *data = dev->data;
	int result = 0;
	uint8_t gpio_dat;

	if (pin > ADS114S0X_GPIO_MAX) {
		LOG_ERR("%s: invalid pin %i", dev->name, pin);
		return -EINVAL;
	}

	k_mutex_lock(&data->gpio_lock, K_FOREVER);

	if ((BIT(pin) & data->gpio_enabled) == 0) {
		LOG_ERR("%s: gpio pin %i not configured", dev->name, pin);
		result = -EINVAL;
	} else if ((BIT(pin) & data->gpio_direction) == 0) {
		LOG_ERR("%s: gpio pin %i not configured as input", dev->name, pin);
		result = -EINVAL;
	} else {
		result = ads114s0x_read_register(dev, ADS114S0X_REGISTER_GPIODAT, &gpio_dat);
		data->gpio_value = ADS114S0X_REGISTER_GPIODAT_DAT_GET(gpio_dat);
		*value = (BIT(pin) & data->gpio_value) != 0;
	}

	k_mutex_unlock(&data->gpio_lock);

	return result;
}

int ads114s0x_gpio_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	struct ads114s0x_data *data = dev->data;
	int result = 0;
	uint8_t gpio_dat;

	k_mutex_lock(&data->gpio_lock, K_FOREVER);

	result = ads114s0x_read_register(dev, ADS114S0X_REGISTER_GPIODAT, &gpio_dat);
	data->gpio_value = ADS114S0X_REGISTER_GPIODAT_DAT_GET(gpio_dat);
	*value = data->gpio_value;

	k_mutex_unlock(&data->gpio_lock);

	return result;
}

int ads114s0x_gpio_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
				       gpio_port_value_t value)
{
	struct ads114s0x_data *data = dev->data;
	int result = 0;

	k_mutex_lock(&data->gpio_lock, K_FOREVER);

	data->gpio_value = ((data->gpio_value & ~mask) | (mask & value)) & data->gpio_enabled &
			   ~data->gpio_direction;
	result = ads114s0x_gpio_write_value(dev);

	k_mutex_unlock(&data->gpio_lock);

	return result;
}

int ads114s0x_gpio_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	struct ads114s0x_data *data = dev->data;
	int result = 0;

	k_mutex_lock(&data->gpio_lock, K_FOREVER);

	data->gpio_value = (data->gpio_value ^ pins) & data->gpio_enabled & ~data->gpio_direction;
	result = ads114s0x_gpio_write_value(dev);

	k_mutex_unlock(&data->gpio_lock);

	return result;
}

#endif /* CONFIG_ADC_ADS114S0X_GPIO */

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

#ifdef CONFIG_ADC_ADS114S0X_GPIO
	k_mutex_init(&data->gpio_lock);
#endif /* CONFIG_ADC_ADS114S0X_GPIO */

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("%s: SPI device is not ready", dev->name);
		return -ENODEV;
	}

	if (config->gpio_reset.port != NULL) {
		result = gpio_pin_configure_dt(&config->gpio_reset, GPIO_OUTPUT_ACTIVE);
		if (result != 0) {
			LOG_ERR("%s: failed to initialize GPIO for reset", dev->name);
			return result;
		}
	}

	if (config->gpio_start_sync.port != NULL) {
		result = gpio_pin_configure_dt(&config->gpio_start_sync, GPIO_OUTPUT_INACTIVE);
		if (result != 0) {
			LOG_ERR("%s: failed to initialize GPIO for start/sync", dev->name);
			return result;
		}
	}

	result = gpio_pin_configure_dt(&config->gpio_data_ready, GPIO_INPUT);
	if (result != 0) {
		LOG_ERR("%s: failed to initialize GPIO for data ready", dev->name);
		return result;
	}

	result = gpio_pin_interrupt_configure_dt(&config->gpio_data_ready, GPIO_INT_EDGE_TO_ACTIVE);
	if (result != 0) {
		LOG_ERR("%s: failed to configure data ready interrupt", dev->name);
		return -EIO;
	}

	gpio_init_callback(&data->callback_data_ready, ads114s0x_data_ready_handler,
			   BIT(config->gpio_data_ready.pin));
	result = gpio_add_callback(config->gpio_data_ready.port, &data->callback_data_ready);
	if (result != 0) {
		LOG_ERR("%s: failed to add data ready callback", dev->name);
		return -EIO;
	}

#if CONFIG_ADC_ASYNC
	k_tid_t tid = k_thread_create(&data->thread, config->stack,
				      CONFIG_ADC_ADS114S0X_ACQUISITION_THREAD_STACK_SIZE,
				      ads114s0x_acquisition_thread, (void *)dev, NULL, NULL,
				      CONFIG_ADC_ADS114S0X_ASYNC_THREAD_INIT_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(tid, "adc_ads114s0x");
#endif

	k_busy_wait(ADS114S0X_POWER_ON_RESET_TIME_IN_US);

	if (config->gpio_reset.port == NULL) {
		result = ads114s0x_send_command(dev, ADS114S0X_COMMAND_RESET);
		if (result != 0) {
			LOG_ERR("%s: unable to send RESET command", dev->name);
			return result;
		}
	} else {
		k_busy_wait(ADS114S0X_RESET_LOW_TIME_IN_US);
		gpio_pin_set_dt(&config->gpio_reset, 0);
	}

	k_busy_wait(ADS114S0X_RESET_DELAY_TIME_IN_US);

	result = ads114s0x_read_register(dev, ADS114S0X_REGISTER_STATUS, &status);
	if (result != 0) {
		LOG_ERR("%s: unable to read status register", dev->name);
		return result;
	}

	if (ADS114S0X_REGISTER_STATUS_NOT_RDY_GET(status) == 0x01) {
		LOG_ERR("%s: ADS114 is not yet ready", dev->name);
		return -EBUSY;
	}

	/*
	 * Activate internal voltage reference during initialization to
	 * avoid the necessary setup time for it to settle later on.
	 */
	ADS114S0X_REGISTER_REF_SET_DEFAULTS(reference_control);

	result = ads114s0x_write_register(dev, ADS114S0X_REGISTER_REF, reference_control);
	if (result != 0) {
		LOG_ERR("%s: unable to set default reference control values", dev->name);
		return result;
	}

	/*
	 * Ensure that the internal voltage reference is active.
	 */
	result = ads114s0x_read_register(dev, ADS114S0X_REGISTER_REF, &reference_control_read);
	if (result != 0) {
		LOG_ERR("%s: unable to read reference control values", dev->name);
		return result;
	}

	if (reference_control != reference_control_read) {
		LOG_ERR("%s: reference control register is incorrect: 0x%02X", dev->name,
			reference_control_read);
		return -EIO;
	}

#ifdef CONFIG_ADC_ADS114S0X_GPIO
	data->gpio_enabled = 0x00;
	data->gpio_direction = 0x0F;
	data->gpio_value = 0x00;

	result = ads114s0x_gpio_write_config(dev);

	if (result != 0) {
		LOG_ERR("%s: unable to configure defaults for GPIOs", dev->name);
		return result;
	}
#endif

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
		.idac_current = DT_INST_PROP(n, idac_current),                                     \
		.vbias_level = DT_INST_PROP(n, vbias_level),                                       \
	};                                                                                         \
	static struct ads114s0x_data data_##n;                                                     \
	DEVICE_DT_INST_DEFINE(n, ads114s0x_init, NULL, &data_##n, &config_##n, POST_KERNEL,        \
			      CONFIG_ADC_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(ADC_ADS114S0X_INST_DEFINE);
