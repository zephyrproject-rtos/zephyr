/*
 * Copyright (c) 2024 Semtech Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_LORA_LBM_LR11XX_BINDINGS_DEF_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_LORA_LBM_LR11XX_BINDINGS_DEF_H_

#define LR11XX_DIO5 (1 << 0)
#define LR11XX_DIO6 (1 << 1)
#define LR11XX_DIO7 (1 << 2)
#define LR11XX_DIO8 (1 << 3)
#define LR11XX_DIO9 (1 << 4)

/* Aliases from the datasheet */
#define LR11XX_SYSTEM_RFSW0_HIGH LR11XX_DIO5
#define LR11XX_SYSTEM_RFSW1_HIGH LR11XX_DIO6
#define LR11XX_SYSTEM_RFSW2_HIGH LR11XX_DIO7
#define LR11XX_SYSTEM_RFSW3_HIGH LR11XX_DIO8

/* Only the low frequency low power path is placed. */
#define LR11XX_TX_PATH_LF_LP    0
/* Only the low frequency high power power path is placed. */
#define LR11XX_TX_PATH_LF_HP    1
/* Both the low frequency low power and low frequency high power paths are placed. */
#define LR11XX_TX_PATH_LF_LP_HP 2

#define LR11XX_TCXO_SUPPLY_1_6V 0x00 /* Supply voltage = 1.6v */
#define LR11XX_TCXO_SUPPLY_1_7V 0x01 /* Supply voltage = 1.7v */
#define LR11XX_TCXO_SUPPLY_1_8V 0x02 /* Supply voltage = 1.8v */
#define LR11XX_TCXO_SUPPLY_2_2V 0x03 /* Supply voltage = 2.2v */
#define LR11XX_TCXO_SUPPLY_2_4V 0x04 /* Supply voltage = 2.4v */
#define LR11XX_TCXO_SUPPLY_2_7V 0x05 /* Supply voltage = 2.7v */
#define LR11XX_TCXO_SUPPLY_3_0V 0x06 /* Supply voltage = 3.0v */
#define LR11XX_TCXO_SUPPLY_3_3V 0x07 /* Supply voltage = 3.3v */

#define LR11XX_LFCLK_RC   0x00 /* Use 32.768kHz RC oscillator */
#define LR11XX_LFCLK_XTAL 0x01 /* Use 32.768kHz crystal oscillator */
#define LR11XX_LFCLK_EXT  0x02 /* Use externally provided 32.768kHz signal on DIO11 pin */

#define LR11XX_REG_MODE_LDO  0x00 /* Do not switch on the DC-DC converter in any mode */
#define LR11XX_REG_MODE_DCDC 0x01 /* Automatically switch on the DC-DC converter when required */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_LORA_LBM_LR11XX_BINDINGS_DEF_H_*/
