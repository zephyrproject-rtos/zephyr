/*
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INCLUDE_DISPLAY_DISPLAY_ADI_ADV7535_H_
#define INCLUDE_DISPLAY_DISPLAY_ADI_ADV7535_H_

#include <zephyr/kernel.h>

#define ADV7535_DTS_TEST_PATTERN_DISABLE            -1
#define ADV7535_DTS_TEST_PATTERN_COLOR_BARS         0
#define ADV7535_DTS_TEST_PATTERN_GRAYSCALE_GRADIENT 1

#define ADV7535_I2C_EDID_ADDR_DEFAULT   0x3f
#define ADV7535_I2C_PACKET_ADDR_DEFAULT 0x38
#define ADV7535_I2C_CEC_ADDR_DEFAULT    0x3c
#define ADV7535_I2C_FIXED_ADDR_DEFAULT  0x00

#define ADV7535_REG_EDID_ADDR       0x43
#define ADV7535_REG_PACKET_MEM_ADDR 0x45
#define ADV7535_REG_CEC_ADDR        0xe1
#define ADV7535_REG_FIXED_ADDR      0xf9

#define ADV7535_REG_REVISION 0x00

#define ADV7535_REG_POWER 0x41

#define ADV7535_POWER_DOWN BIT(6) /* 0 = Powered up; 1 = Powered down */

#define ADV7535_REG_ENABLE_0 0x40

#define ADV7535_ENABLE_0_PACKET_SPARE_1 BIT(0)
#define ADV7535_ENABLE_0_PACKET_SPARE_2 BIT(1)
#define ADV7535_ENABLE_0_PACKET_GM      BIT(2)
#define ADV7535_ENABLE_0_PACKET_ISRC    BIT(3)
#define ADV7535_ENABLE_0_PACKET_ACP     BIT(4)
#define ADV7535_ENABLE_0_PACKET_MPEG    BIT(5)
#define ADV7535_ENABLE_0_PACKET_SPD     BIT(6)
#define ADV7535_ENABLE_0_PACKET_GC      BIT(7)

#define ADV7535_REG_ENABLE_1 0x44

#define ADV7535_ENABLE_1_PACKET_MEM_READ_MODE BIT(0)
/* Bits 1 and 2 are reserved and should not be used */
#define ADV7535_ENABLE_1_AUDIO_INFO_FRAME     BIT(3)
#define ADV7535_ENABLE_1_AVI_INFO_FRAME       BIT(4)
#define ADV7535_ENABLE_1_PACKET_AUDIO_SAMPLE  BIT(5)
#define ADV7535_ENABLE_1_PACKET_N_CTS         BIT(6)
/* Bit 7 is reserved and should not be used */

#define ADV7535_REG_CEC_POWER_DOWN 0xe2

#define ADV7535_CEC_POWER_DOWN BIT(0)

#define ADV7535_REG_CEC_INT_ENABLE 0x92
#define ADV7535_REG_INT_ENABLE_0   0x94
#define ADV7535_REG_INT_ENABLE_1   0x95

#define ADV7535_REG_CEC_INT 0x93
#define ADV7535_REG_INT_0   0x96
#define ADV7535_REG_INT_1   0x97

#define ADV7535_INT_0_HPD             BIT(7)
#define ADV7535_INT_0_MONITOR_SENSE   BIT(6)
#define ADV7535_INT_0_VSYNC           BIT(5)
#define ADV7535_INT_0_AUDIO_FIFO_FULL BIT(4)
/* BIT(3) is reserved */
#define ADV7535_INT_0_EDID_READY      BIT(2)
#define ADV7535_INT_0_HDCP_AUTH       BIT(1)
/* BIT(0) is reserved */

#define ADV7535_INT_1_DCC_ERR             BIT(7)
#define ADV7535_INT_1_BKSV                BIT(6)
#define ADV7535_INT_1_TX_READY            BIT(5)
#define ADV7535_INT_1_TX_ARBITRATION_LOST BIT(4)
#define ADV7535_INT_1_TX_RETRY_TIMEOUT    BIT(3)
#define ADV7535_INT_1_RX_READY_3          BIT(2)
#define ADV7535_INT_1_RX_READY_2          BIT(1)
#define ADV7535_INT_1_RX_READY_1          BIT(0)

#define ADV7535_REG_PORT_STATE 0x42

#define ADV7535_HPD_STATE           BIT(6)
#define ADV7535_MONITOR_SENSE_STATE BIT(5)

#define ADV7535_REG_CEC_TEST_PATTERN 0x55

#define ADV7535_TEST_PATTERN_DISABLE            0x00
#define ADV7535_TEST_PATTERN_COLOR_BARS         0x80
#define ADV7535_TEST_PATTERN_GRAYSCALE_GRADIENT 0xA0

#define ADV7535_REG_HDMI_OPTS 0xaf

#define ADV7535_HDMI_OPTS_MODE      BIT(1)
#define ADV7535_HDMI_OPTS_MODE_DVI  0
#define ADV7535_HDMI_OPTS_MODE_HDMI 1

#define ADV7535_REG_TIMING_START 0x28

enum adv7535_reg_map {
	ADV7535_MAIN,
	ADV7535_PACKET,
	ADV7535_EDID,
	ADV7535_CEC
};

struct reg_val_pair {
	uint8_t reg, val;
};

const struct reg_val_pair adv7535_fixed_registers[] = {
	{0x16, 0x20}, {0x9a, 0xe0}, {0xba, 0x70}, {0xde, 0x82}, {0xe4, 0x40}, {0xe5, 0x80},
};

const struct reg_val_pair adv7535_cec_fixed_registers[] = {
	{0x15, 0xd0}, {0x17, 0xd0}, {0x24, 0x20}, {0x57, 0x11}, {0x05, 0xc8},
};

#endif /* INCLUDE_DISPLAY_DISPLAY_ADI_ADV7535_H_ */
