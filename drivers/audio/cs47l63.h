/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 *
 * CS47L63 register definitions
 *
 * Based on Cirrus Logic CS47L63 datasheet and mcu-drivers (Apache-2.0).
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_CS47L63_H_
#define ZEPHYR_DRIVERS_AUDIO_CS47L63_H_

/* Device identification */
#define CS47L63_DEVID                        0x000000
#define CS47L63_REVID                        0x000004
#define CS47L63_OTPID                        0x000010
#define CS47L63_SFT_RESET                    0x000020
#define CS47L63_SOFT_RESET_VAL               0x5A000000

/* GPIO registers */
#define CS47L63_GPIO1_CTRL1                  0x000C08
#define CS47L63_GPIO2_CTRL1                  0x000C0C
#define CS47L63_GPIO3_CTRL1                  0x000C10
#define CS47L63_GPIO4_CTRL1                  0x000C14
#define CS47L63_GPIO5_CTRL1                  0x000C18
#define CS47L63_GPIO6_CTRL1                  0x000C1C
#define CS47L63_GPIO7_CTRL1                  0x000C20
#define CS47L63_GPIO8_CTRL1                  0x000C24
#define CS47L63_GPIO9_CTRL1                  0x000C28
#define CS47L63_GPIO10_CTRL1                 0x000C2C

/* Clock registers */
#define CS47L63_SYSTEM_CLOCK1                0x001404
#define CS47L63_SAMPLE_RATE1                 0x001420
#define CS47L63_SAMPLE_RATE2                 0x001424
#define CS47L63_SAMPLE_RATE3                 0x001428
#define CS47L63_SAMPLE_RATE4                 0x00142C
#define CS47L63_ASYNC_CLOCK1                 0x001460

/* FLL registers */
#define CS47L63_FLL1_CONTROL1                0x001C00
#define CS47L63_FLL1_CONTROL2                0x001C04
#define CS47L63_FLL1_CONTROL3                0x001C08
#define CS47L63_FLL1_GPIO_CLOCK              0x001CA0

/* Bias / power registers */
#define CS47L63_LDO2_CTRL1                   0x002408
#define CS47L63_MICBIAS_CTRL1                0x002410
#define CS47L63_MICBIAS_CTRL5                0x002418

/* Input registers */
#define CS47L63_INPUT_CONTROL                0x004000
#define CS47L63_INPUT_CONTROL3               0x004014
#define CS47L63_INPUT1_CONTROL1              0x004020
#define CS47L63_IN1L_CONTROL2                0x004028
#define CS47L63_IN1R_CONTROL2                0x004048
#define CS47L63_INPUT2_CONTROL1              0x004060
#define CS47L63_IN2L_CONTROL1                0x004064
#define CS47L63_IN2L_CONTROL2                0x004068
#define CS47L63_IN2R_CONTROL1                0x004084
#define CS47L63_IN2R_CONTROL2                0x004088

/* Output registers */
#define CS47L63_OUTPUT_ENABLE_1              0x004804
#define CS47L63_OUT1L_VOLUME_1               0x004818

/* ASP1 (Audio Serial Port) registers */
#define CS47L63_ASP1_ENABLES1                0x006000
#define CS47L63_ASP1_CONTROL1                0x006004
#define CS47L63_ASP1_CONTROL2                0x006008
#define CS47L63_ASP1_CONTROL3                0x00600C
#define CS47L63_ASP1_DATA_CONTROL1           0x006030
#define CS47L63_ASP1_DATA_CONTROL5           0x006040

/* Mixer / routing registers */
#define CS47L63_OUT1L_INPUT1                 0x008100
#define CS47L63_OUT1L_INPUT2                 0x008104
#define CS47L63_ASP1TX1_INPUT1               0x008200
#define CS47L63_ASP1TX2_INPUT1               0x008210

/* IRQ registers */
#define CS47L63_IRQ1_EINT_1                  0x018004
#define CS47L63_IRQ1_EINT_2                  0x018008
#define CS47L63_IRQ1_STS_6                   0x018054
#define CS47L63_IRQ1_MASK_1                  0x01800C

/* Expected device ID */
#define CS47L63_DEVID_VAL                    0x47A63

/* Volume constants */
#define CS47L63_MAX_VOLUME_REG_VAL           0x80
#define CS47L63_OUT_VOLUME_DEFAULT           0x62
#define CS47L63_VOLUME_UPDATE_BIT            BIT(9)

/* SPI padding */
#define CS47L63_SPI_PAD_LEN                  4

/* Sample rate codes (register values for SAMPLE_RATE1) */
#define CS47L63_SAMPLE_RATE_8K               0x0011
#define CS47L63_SAMPLE_RATE_16K              0x0012
#define CS47L63_SAMPLE_RATE_24K              0x0002
#define CS47L63_SAMPLE_RATE_32K              0x0013
#define CS47L63_SAMPLE_RATE_48K              0x0003
#define CS47L63_SAMPLE_RATE_96K              0x0004
#define CS47L63_SAMPLE_RATE_192K             0x0005

#endif /* ZEPHYR_DRIVERS_AUDIO_CS47L63_H_ */
