/* spi_intel.h - Intel's SPI driver private definitions */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SPI_SPI_INTEL_H_
#define ZEPHYR_DRIVERS_SPI_SPI_INTEL_H_

#include "spi_intel_regs.h"
#include "spi_context.h"

#ifdef CONFIG_PCI
#include <pci/pci.h>
#include <pci/pci_mgr.h>
#endif /* CONFIG_PCI */

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*spi_intel_config_t)(void);

struct spi_intel_config {
	u32_t irq;
	spi_intel_config_t config_func;
};

struct spi_intel_data {
	struct spi_context ctx;
	u32_t regs;
#ifdef CONFIG_PCI
	struct pci_dev_info pci_dev;
#endif /* CONFIG_PCI */
	u32_t sscr0;
	u32_t sscr1;
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	u32_t device_power_state;
#endif
	u8_t dfs;
};

#define DEFINE_MM_REG_READ(__reg, __off, __sz)				\
	static inline u32_t read_##__reg(u32_t addr)			\
	{								\
		return sys_read##__sz(addr + __off);			\
	}
#define DEFINE_MM_REG_WRITE(__reg, __off, __sz)				\
	static inline void write_##__reg(u32_t data, u32_t addr)	\
	{								\
		sys_write##__sz(data, addr + __off);			\
	}

DEFINE_MM_REG_WRITE(sscr0, INTEL_SPI_REG_SSCR0, 32)
DEFINE_MM_REG_WRITE(sscr1, INTEL_SPI_REG_SSCR1, 32)
DEFINE_MM_REG_READ(sssr, INTEL_SPI_REG_SSSR, 32)
DEFINE_MM_REG_READ(ssdr, INTEL_SPI_REG_SSDR, 32)
DEFINE_MM_REG_WRITE(ssdr, INTEL_SPI_REG_SSDR, 32)
DEFINE_MM_REG_WRITE(dds_rate, INTEL_SPI_REG_DDS_RATE, 32)

#define DEFINE_SET_BIT_OP(__reg_bit, __reg_off, __bit)			\
	static inline void set_bit_##__reg_bit(u32_t addr)		\
	{								\
		sys_set_bit(addr + __reg_off, __bit);			\
	}

#define DEFINE_CLEAR_BIT_OP(__reg_bit, __reg_off, __bit)		\
	static inline void clear_bit_##__reg_bit(u32_t addr)		\
	{								\
		sys_clear_bit(addr + __reg_off, __bit);			\
	}

#define DEFINE_TEST_BIT_OP(__reg_bit, __reg_off, __bit)			\
	static inline int test_bit_##__reg_bit(u32_t addr)		\
	{								\
		return sys_test_bit(addr + __reg_off, __bit);		\
	}

DEFINE_SET_BIT_OP(sscr0_sse, INTEL_SPI_REG_SSCR0, INTEL_SPI_SSCR0_SSE_BIT)
DEFINE_CLEAR_BIT_OP(sscr0_sse, INTEL_SPI_REG_SSCR0, INTEL_SPI_SSCR0_SSE_BIT)
DEFINE_TEST_BIT_OP(sscr0_sse, INTEL_SPI_REG_SSCR0, INTEL_SPI_SSCR0_SSE_BIT)
DEFINE_TEST_BIT_OP(sssr_bsy, INTEL_SPI_REG_SSSR, INTEL_SPI_SSSR_BSY_BIT)
DEFINE_CLEAR_BIT_OP(sscr1_tie, INTEL_SPI_REG_SSCR1, INTEL_SPI_SSCR1_TIE_BIT)
DEFINE_TEST_BIT_OP(sscr1_tie, INTEL_SPI_REG_SSCR1, INTEL_SPI_SSCR1_TIE_BIT)
DEFINE_CLEAR_BIT_OP(sssr_ror, INTEL_SPI_REG_SSSR, INTEL_SPI_SSSR_ROR_BIT)

/* 0x38 represents the bits 8, 16 and 32. Knowing that 24 is bits 8 and 16
 * These are the bits were when you divide by 8, you keep the result as it is.
 * For all the other ones, 4 to 7, 9 to 15, etc... you need a +1,
 * since on such division it takes only the result above 0
 */
#define SPI_WS_TO_DFS(__bpw) (((__bpw) & ~0x38) ?		\
			      (((__bpw) / 8) + 1) :		\
			      ((__bpw) / 8))

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_SPI_SPI_INTEL_H_ */
