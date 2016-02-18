/* spi_k64.h - Freescale K64 SPI controller driver utilities */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __SPI_K64_H__
#define __SPI_K64_H__

/*
 * Device configuration
 *
 * Device-independent configuration:
 * Bits [0 : 11] in the config parameter of the spi_configure() API are defined
 * with the following fields.
 *
 * SCK polarity     [ 0 ]       - SCK inactive state (0 = low, 1 = high)
 * SCK phase        [ 1 ]       - Data captured/changed on which SCK edge:
 *                              -   0 = leading/following edges, respectively
 *                              -   1 = following/leading edges, respectively
 * loop_mode        [ 2 ]       - Not used/Unsupported
 * transfer_mode    [ 3 ]       - First significant bit (0 = MSB, 1 = LSB)
 * word_size        [ 4 : 7 ]   - Size of a data train in bits
 * unused           [ 8 : 11 ]  - Unused word_size field bits
 *
 * Device-specific configuration:
 * Bits [12 : 31] in the config parameter of the spi_configure() API are
 * available, with the following fields defined for this device.
 *
 * PCS0-5 polarity  [ 12 : 17 ] - Periph. Chip Select inactive state, MCR[PCSIS]
 *                              -   (0 = low, 1 = high)
 * Continuous SCK   [ 18 ]      - Continuous serial clocking, MCR[CONT_SCKE]
 *                              -   (0 = disabled, 1 = enabled)
 * Continuous PCS   [ 19 ]      - Continuous selection format, PUSHR[CONT]
 *                              -   (0 = disabled, 1 = enabled)
 *
 * Note that the number of valid PCS signals differs for each
 * K64 SPI module:
 * - SPI0 uses PCS0-5;
 * - SPI1 uses PCS0-3;
 * - SPI2 uses PCS0-1;
 */

/* PCS polarity access macros */

#define SPI_PCS_POL_MASK		(0x3F << 12)
#define SPI_PCS_POL_GET(_in_)	(((_in_) & SPI_PCS_POL_MASK) >> 12)
#define SPI_PCS_POL_SET(_in_)	((_in_) << 12)

/* Continuous SCK access macros */

#define SPI_CONT_SCK_MASK		(0x1 << 18)
#define SPI_CONT_SCK_GET(_in_)	(((_in_) & SPI_CONT_SCK_MASK) >> 18)
#define SPI_CONT_SCK_SET(_in_)	((_in_) << 18)

/* Continuous PCS access macros */

#define SPI_CONT_PCS_MASK		(0x1 << 19)
#define SPI_CONT_PCS_GET(_in_)	(((_in_) & SPI_CONT_PCS_MASK) >> 19)
#define SPI_CONT_PCS_SET(_in_)	((_in_) << 19)

/* K64 SPI word/frame size is limited to 16 bits, represented as: (size - 1) */

#define SPI_K64_WORD_SIZE_MAX	(16)

#endif /* __SPI_K64_H__ */
