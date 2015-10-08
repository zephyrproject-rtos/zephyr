/* intel_spi.h - Intel's SPI controller driver utilities */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __INTEL_SPI_H__
#define __INTEL_SPI_H__

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

#endif /* __INTEL_SPI_H__ */
