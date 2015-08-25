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

#include <spi.h>

#ifdef CONFIG_PCI
#include <pci/pci.h>
#include <pci/pci_mgr.h>
#endif /* CONFIG_PCI */

#ifdef __cplusplus
extern "C" {
#endif

extern void spi_intel_isr(void *data);

int spi_intel_init(struct device *dev);
typedef void (*spi_intel_config_t)(struct device *dev);

struct spi_intel_config {
	uint32_t regs;
	uint32_t irq;
#ifdef CONFIG_PCI
	struct pci_dev_info pci_dev;
#endif /* CONFIG_PCI */
	spi_intel_config_t config_func;
};

struct spi_intel_data {
	uint32_t sscr0;
	uint32_t sscr1;
	spi_callback callback;
	uint8_t *tx_buf;
	uint32_t tx_buf_len;
	uint8_t *rx_buf;
	uint32_t rx_buf_len;
	uint32_t t_len;
};

/* SPI Maximum supported system frequencies */
#define SPI_MAX_CLK_FREQ_25MHZ     ((800000 << 8))
#define SPI_MAX_CLK_FREQ_20MHz     ((666666 << 8) | 1)
#define SPI_MAX_CLK_FREQ_166667KHZ ((800000 << 8) | 2)
#define SPI_MAX_CLK_FREQ_13333KHZ  ((666666 << 8) | 2)
#define SPI_MAX_CLK_FREQ_12500KHZ  ((200000 << 8))
#define SPI_MAX_CLK_FREQ_10MHZ     ((800000 << 8) | 4)
#define SPI_MAX_CLK_FREQ_8MHZ      ((666666 << 8) | 4)
#define SPI_MAX_CLK_FREQ_6250HZ    ((400000 << 8) | 3)
#define SPI_MAX_CLK_FREQ_5MHZ      ((400000 << 8) | 4)
#define SPI_MAX_CLK_FREQ_4MHZ      ((666666 << 8) | 9)
#define SPI_MAX_CLK_FREQ_3125KHZ   ((80000 << 8))
#define SPI_MAX_CLK_FREQ_2500KHZ   ((400000 << 8) | 9)
#define SPI_MAX_CLK_FREQ_2MHZ      ((666666 << 8) | 19)
#define SPI_MAX_CLK_FREQ_1563KHZ   ((40000 << 8))
#define SPI_MAX_CLK_FREQ_1250KHZ   ((200000 << 8) | 9)
#define SPI_MAX_CLK_FREQ_1MHZ      ((400000 << 8) | 24)
#define SPI_MAX_CLK_FREQ_800KHZ    ((666666 << 8) | 49)
#define SPI_MAX_CLK_FREQ_781KHZ    ((20000 << 8))
#define SPI_MAX_CLK_FREQ_625KHZ    ((200000 << 8) | 19)
#define SPI_MAX_CLK_FREQ_500KHZ    ((400000 << 8) | 49)
#define SPI_MAX_CLK_FREQ_400KHZ    ((666666 << 8) | 99)
#define SPI_MAX_CLK_FREQ_390KHZ    ((10000 << 8))
#define SPI_MAX_CLK_FREQ_250KHZ    ((400000 << 8) | 99)
#define SPI_MAX_CLK_FREQ_200KHZ    ((666666 << 8) | 199)
#define SPI_MAX_CLK_FREQ_195KHZ    ((8000 << 8))
#define SPI_MAX_CLK_FREQ_125KHZ    ((100000 << 8) | 49)
#define SPI_MAX_CLK_FREQ_100KHZ    ((200000 << 8) | 124)
#define SPI_MAX_CLK_FREQ_50KHZ     ((100000 << 8) | 124)
#define SPI_MAX_CLK_FREQ_20KHZ     ((80000 << 8) | 124)
#define SPI_MAX_CLK_FREQ_10KHZ     ((20000 << 8) | 77)
#define SPI_MAX_CLK_FREQ_5KHZ      ((20000 << 8) | 154)
#define SPI_MAX_CLK_FREQ_1KHZ      ((8000 << 8) | 194)

#ifdef __cplusplus
}
#endif

#endif /* __INTEL_SPI_H__ */
