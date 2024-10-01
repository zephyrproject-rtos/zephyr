/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2024 sensry.io
 */

#ifndef GANYMED_SY1XX_PAD_CTRL_H
#define GANYMED_SY1XX_PAD_CTRL_H

#define PAD_CONFIG(pin_offset, SMT, SLEW, PULLUP, PULLDOWN, DRV, PMOD, DIR)                        \
	(((SMT << 7) | (SLEW << 6) | (PULLUP << 5) | (PULLDOWN << 4) | (DRV << 2) | (PMOD << 1) |  \
	  DIR)                                                                                     \
	 << pin_offset)

#define PAD_CONFIG_ADDR (ARCHI_SOC_PERIPHERALS_ADDR + ARCHI_APB_SOC_CTRL_OFFSET)

#define PAD_CONFIG_ADDR_UART (PAD_CONFIG_ADDR + 0x020)
#define PAD_CONFIG_ADDR_SPI  (PAD_CONFIG_ADDR + 0x02c)
#define PAD_CONFIG_ADDR_I2C  (PAD_CONFIG_ADDR + 0x100)
#define PAD_CONFIG_ADDR_MAC  (PAD_CONFIG_ADDR + 0x130)

#define PAD_SMT_DISABLE 0
#define PAD_SMT_ENABLE  1

#define PAD_SLEW_LOW  0
#define PAD_SLEW_HIGH 1

#define PAD_PULLUP_DIS 0
#define PAD_PULLUP_EN  1

#define PAD_PULLDOWN_DIS 0
#define PAD_PULLDOWN_EN  1

#define PAD_DRIVE_2PF  0
#define PAD_DRIVE_4PF  1
#define PAD_DRIVE_8PF  2
#define PAD_DRIVE_16PF 3

#define PAD_PMOD_NORMAL   0
#define PAD_PMOD_TRISTATE 1

#define PAD_DIR_OUTPUT 0
#define PAD_DIR_INPUT  1

#endif /* GANYMED_SY1XX_PAD_CTRL_H */
