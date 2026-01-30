/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Device tree clock identifiers for the Infineon AURIX TC3x family.
 *
 * Each CLOCK_F<x> macro is a stable numeric handle for one of the SoC's
 * Clock Distribution Unit (CDU) output domains. Devicetree clocks-cells
 * use these identifiers to bind a peripheral to its source domain; the
 * AURIX clock-control driver translates them back into the matching SCU
 * CCUCON / CLKSEL field at runtime.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_TC3X_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_TC3X_CLOCK_H_

/** Standby / reset / input clock domain (fSRI). */
#define CLOCK_FSRI     0x0
/** System peripheral bus clock domain (fSPB). */
#define CLOCK_FSPB     0x1
/** Generic Timer Module clock domain (fGTM). */
#define CLOCK_FGTM     0x2
/** System Timer clock domain (fSTM). */
#define CLOCK_FSTM     0x3
/** MSC (Microsecond Channel) clock domain (fMSC). */
#define CLOCK_FMSC     0x4
/** Gigabit Ethernet clock domain (fGETH). */
#define CLOCK_FGETH    0x5
/** ADAS subsystem clock domain (fADAS). */
#define CLOCK_FADAS    0x6
/** High-speed MultiCAN clock domain (fMCANH). */
#define CLOCK_FMCANH   0x7
/** MultiCAN clock domain (fMCAN). */
#define CLOCK_FMCAN    0x8
/** ASC/LIN fast clock domain (fASCLINF). */
#define CLOCK_FASCLINF 0x9
/** ASC/LIN slow clock domain (fASCLINS). */
#define CLOCK_FASCLINS 0xA
/** Queued SPI clock domain (fQSPI). */
#define CLOCK_FQSPI    0xB
/** ADC clock domain (fADC). */
#define CLOCK_FADC     0xC
/** I2C clock domain (fI2C). */
#define CLOCK_FI2C     0xD
/** External Bus Unit clock domain (fEBU). */
#define CLOCK_FEBU     0xE

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_TC3X_CLOCK_H_ */
