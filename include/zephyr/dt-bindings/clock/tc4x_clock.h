/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Device tree clock identifiers for the Infineon AURIX TC4x family.
 *
 * Each CLOCK_F<x> macro is a stable numeric handle for one of the SoC's
 * Clock Distribution Unit (CDU) output domains. Devicetree clocks-cells
 * use these identifiers to bind a peripheral to its source domain; the
 * AURIX clock-control driver translates them back into the matching SCU
 * CCUCON / CLKSEL field at runtime. The TC4x set is a superset of TC3x
 * and adds the CPU peripheral bus (fCPB), Embedded GTM (fEGTM) and the
 * low-speed Ethernet (fLETH) domains.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_TC4X_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_TC4X_CLOCK_H_

/** Standby / reset / input clock domain (fSRI). */
#define CLOCK_FSRI     0x0
/** System peripheral bus clock domain (fSPB). */
#define CLOCK_FSPB     0x1
/** CPU peripheral bus clock domain (fCPB). */
#define CLOCK_FCPB     0x2
/** Generic Timer Module clock domain (fGTM). */
#define CLOCK_FGTM     0x3
/** Embedded Generic Timer Module clock domain (fEGTM). */
#define CLOCK_FEGTM    0x4
/** System Timer clock domain (fSTM). */
#define CLOCK_FSTM     0x5
/** MSC (Microsecond Channel) clock domain (fMSC). */
#define CLOCK_FMSC     0x6
/** Gigabit Ethernet clock domain (fGETH). */
#define CLOCK_FGETH    0x7
/** Low-speed Ethernet clock domain (fLETH). */
#define CLOCK_FLETH    0x8
/** High-speed MultiCAN clock domain (fMCANH). */
#define CLOCK_FMCANH   0x9
/** MultiCAN clock domain (fMCAN). */
#define CLOCK_FMCAN    0xA
/** ASC/LIN fast clock domain (fASCLINF). */
#define CLOCK_FASCLINF 0xB
/** ASC/LIN slow clock domain (fASCLINS). */
#define CLOCK_FASCLINS 0xC
/** Queued SPI clock domain (fQSPI). */
#define CLOCK_FQSPI    0xD
/** ADC clock domain (fADC). */
#define CLOCK_FADC     0xE
/** I2C clock domain (fI2C). */
#define CLOCK_FI2C     0xF

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_TC4X_CLOCK_H_ */
