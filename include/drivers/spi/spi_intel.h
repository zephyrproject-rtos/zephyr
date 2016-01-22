/* spi_intel.h - Intel's SPI controller driver utilities */

/*
 * Copyright (c) 2015 Intel Corporation.
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

#ifndef __SPI_INTEL_H__
#define __SPI_INTEL_H__

#ifdef __cplusplus
extern "C" {
#endif

/* SPI Maximum supported system frequencies */
#define SPI_MAX_CLK_FREQ_25MHZ     ((0x800000 << 8))
#define SPI_MAX_CLK_FREQ_20MHz     ((0x666666 << 8) | 1)
#define SPI_MAX_CLK_FREQ_166667KHZ ((0x800000 << 8) | 2)
#define SPI_MAX_CLK_FREQ_13333KHZ  ((0x666666 << 8) | 2)
#define SPI_MAX_CLK_FREQ_12500KHZ  ((0x200000 << 8))
#define SPI_MAX_CLK_FREQ_10MHZ     ((0x800000 << 8) | 4)
#define SPI_MAX_CLK_FREQ_8MHZ      ((0x666666 << 8) | 4)
#define SPI_MAX_CLK_FREQ_6250HZ    ((0x400000 << 8) | 3)
#define SPI_MAX_CLK_FREQ_5MHZ      ((0x400000 << 8) | 4)
#define SPI_MAX_CLK_FREQ_4MHZ      ((0x666666 << 8) | 9)
#define SPI_MAX_CLK_FREQ_3125KHZ   ((0x80000 << 8))
#define SPI_MAX_CLK_FREQ_2500KHZ   ((0x400000 << 8) | 9)
#define SPI_MAX_CLK_FREQ_2MHZ      ((0x666666 << 8) | 19)
#define SPI_MAX_CLK_FREQ_1563KHZ   ((0x40000 << 8))
#define SPI_MAX_CLK_FREQ_1250KHZ   ((0x200000 << 8) | 9)
#define SPI_MAX_CLK_FREQ_1MHZ      ((0x400000 << 8) | 24)
#define SPI_MAX_CLK_FREQ_800KHZ    ((0x666666 << 8) | 49)
#define SPI_MAX_CLK_FREQ_781KHZ    ((0x20000 << 8))
#define SPI_MAX_CLK_FREQ_625KHZ    ((0x200000 << 8) | 19)
#define SPI_MAX_CLK_FREQ_500KHZ    ((0x400000 << 8) | 49)
#define SPI_MAX_CLK_FREQ_400KHZ    ((0x666666 << 8) | 99)
#define SPI_MAX_CLK_FREQ_390KHZ    ((0x10000 << 8))
#define SPI_MAX_CLK_FREQ_250KHZ    ((0x400000 << 8) | 99)
#define SPI_MAX_CLK_FREQ_200KHZ    ((0x666666 << 8) | 199)
#define SPI_MAX_CLK_FREQ_195KHZ    ((0x8000 << 8))
#define SPI_MAX_CLK_FREQ_125KHZ    ((0x100000 << 8) | 49)
#define SPI_MAX_CLK_FREQ_100KHZ    ((0x200000 << 8) | 124)
#define SPI_MAX_CLK_FREQ_50KHZ     ((0x100000 << 8) | 124)
#define SPI_MAX_CLK_FREQ_20KHZ     ((0x80000 << 8) | 124)
#define SPI_MAX_CLK_FREQ_10KHZ     ((0x20000 << 8) | 77)
#define SPI_MAX_CLK_FREQ_5KHZ      ((0x20000 << 8) | 154)
#define SPI_MAX_CLK_FREQ_1KHZ      ((0x8000 << 8) | 194)
#ifdef __cplusplus
}
#endif

#endif /* __SPI_INTEL_H__ */
