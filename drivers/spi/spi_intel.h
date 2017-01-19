/* spi_intel.h - Intel's SPI driver private definitions */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SPI_INTEL_PRIV_H__
#define __SPI_INTEL_PRIV_H__

#ifdef CONFIG_PCI
#include <pci/pci.h>
#include <pci/pci_mgr.h>
#endif /* CONFIG_PCI */
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*spi_intel_config_t)(void);

struct spi_intel_config {
	uint32_t irq;
	spi_intel_config_t config_func;
#ifdef CONFIG_SPI_CS_GPIO
	char *cs_gpio_name;
	uint32_t cs_gpio_pin;
#endif /* CONFIG_SPI_CS_GPIO */
};

struct spi_intel_data {
	uint32_t regs;
#ifdef CONFIG_PCI
	struct pci_dev_info pci_dev;
#endif /* CONFIG_PCI */
	struct k_sem device_sync_sem;
	uint8_t error;
	uint8_t padding[3];
#ifdef CONFIG_SPI_CS_GPIO
	struct device *cs_gpio_port;
#endif /* CONFIG_SPI_CS_GPIO */
	uint32_t sscr0;
	uint32_t sscr1;
	const uint8_t *tx_buf;
	uint8_t *rx_buf;
	uint32_t t_buf_len;
	uint32_t r_buf_len;
	uint32_t transmitted;
	uint32_t received;
	uint32_t trans_len;
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	uint32_t device_power_state;
#endif
};

/* Registers */
#define INTEL_SPI_REG_SSCR0		(0x00)
#define INTEL_SPI_REG_SSCR1		(0x04)
#define INTEL_SPI_REG_SSSR		(0x08)
#define INTEL_SPI_REG_SSDR		(0x10)
#define INTEL_SPI_REG_DDS_RATE		(0x28)

#define INTEL_SPI_CLK_DIV_MASK		(0x000000ff)
#define INTEL_SPI_DDS_RATE_MASK		(0xffffff00)

/* SSCR0 settings */
#define INTEL_SPI_SSCR0_DSS(__bpw)	((__bpw) - 1)
#define INTEL_SPI_SSCR0_SSE		(0x1 << 7)
#define INTEL_SPI_SSCR0_SSE_BIT		(7)
#define INTEL_SPI_SSCR0_SCR(__msf) \
	((__msf & INTEL_SPI_CLK_DIV_MASK) << 8)

/* SSCR1 settings */
#define INTEL_SPI_SSCR1_TIE_BIT		(1)

#define INTEL_SPI_SSCR1_RIE		(0x1)
#define INTEL_SPI_SSCR1_TIE		(0x1 << 1)
#define INTEL_SPI_SSCR1_LBM		(0x1 << 2)
#define INTEL_SPI_SSCR1_SPO		(0x1 << 3)
#define INTEL_SPI_SSCR1_SPH		(0x1 << 4)
#define INTEL_SPI_SSCR1_TFT_MASK	(0x1f << 6)
#define INTEL_SPI_SSCR1_TFT(__tft) \
	(((__tft) - 1) << 6)
#define INTEL_SPI_SSCR1_RFT_MASK	(0x1f << 11)
#define INTEL_SPI_SSCR1_RFT(__rft) \
	(((__rft) - 1) << 11)
#define INTEL_SPI_SSCR1_EFWR		(0x1 << 16)
#define INTEL_SPI_SSCR1_STRF		(0x1 << 17)

#define INTEL_SPI_SSCR1_TFT_DFLT	(8)
#define INTEL_SPI_SSCR1_RFT_DFLT	(1)

/* SSSR settings */
#define INTEL_SPI_SSSR_TNF		(0x4)
#define INTEL_SPI_SSSR_RNE		(0x8)
#define INTEL_SPI_SSSR_TFS		(0x20)
#define INTEL_SPI_SSSR_RFS		(0x40)
#define INTEL_SPI_SSSR_ROR		(0x80)
#define INTEL_SPI_SSSR_TFL_MASK		(0x1f << 8)
#define INTEL_SPI_SSSR_TFL_EMPTY	(0x00)
#define INTEL_SPI_SSSR_RFL_MASK		(0x1f << 13)
#define INTEL_SPI_SSSR_RFL_EMPTY	(0x1f)

#define INTEL_SPI_SSSR_TFL(__status) \
	((__status & INTEL_SPI_SSSR_TFL_MASK) >> 8)
#define INTEL_SPI_SSSR_RFL(__status) \
	((__status & INTEL_SPI_SSSR_RFL_MASK) >> 13)

#define INTEL_SPI_SSSR_BSY_BIT		(4)
#define INTEL_SPI_SSSR_ROR_BIT		(7)

/* DSS_RATE settings */
#define INTEL_SPI_DSS_RATE(__msf) \
	((__msf & (INTEL_SPI_DDS_RATE_MASK)) >> 8)
#ifdef __cplusplus
}
#endif

#endif /* __SPI_INTEL_PRIV_H__ */
