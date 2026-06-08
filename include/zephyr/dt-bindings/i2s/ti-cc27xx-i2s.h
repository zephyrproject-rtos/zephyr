/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_I2S_TI_CC27XX_I2S_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_I2S_TI_CC27XX_I2S_H_

/**
 * Audio Frequency Clock (AFCLK) source selection
 */
#define TI_CC27XX_I2S_CLK_SRC_CLKREF 0x00 /* 48 MHz crystal reference */
#define TI_CC27XX_I2S_CLK_SRC_CLKHF  0x01 /* 96 MHz high-frequency clock */
#define TI_CC27XX_I2S_CLK_SRC_CLKAF  0x02 /* AFOSC (80/90.3168/98.304 MHz) */

/**
 * Audio Frequency Oscillator (AFOSC) frequencies
 * Only used when afclk-source = TI_CC27XX_I2S_CLK_SRC_CLKAF
 */
#define TI_CC27XX_I2S_AFOSC_FREQ_80MHZ      80000000 /* 80 MHz */
#define TI_CC27XX_I2S_AFOSC_FREQ_90_3168MHZ 90316800 /* 90.3168 MHz for 44.1 kHz family */
#define TI_CC27XX_I2S_AFOSC_FREQ_98_304MHZ  98304000 /* 98.304 MHz for 48 kHz family */

/**
 * I2S serial data pin direction
 * SD0 and SD1 can independently be input (RX), output (TX), or disabled
 */
#define TI_CC27XX_I2S_PIN_DIR_DISABLED 0x00 /* Pin not used */
#define TI_CC27XX_I2S_PIN_DIR_IN       0x01 /* Input (RX - receive from codec) */
#define TI_CC27XX_I2S_PIN_DIR_OUT      0x02 /* Output (TX - transmit to codec) */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_I2S_TI_CC27XX_I2S_H_ */
