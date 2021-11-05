/*
 * Copyright (c) 2021 Jim Shu
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _RISCV_PMP_MEM_CFG_H_
#define _RISCV_PMP_MEM_CFG_H_

#include <sys/util.h>

/*
 * Compute FLASH_BASE and FLASH_SIZE.
 * It's copied from RISC-V linker script.
 */
#if DT_NODE_HAS_COMPAT_STATUS(DT_CHOSEN(zephyr_flash), soc_nv_flash, okay)
# define FLASH_BASE	DT_REG_ADDR(DT_CHOSEN(zephyr_flash))
# define FLASH_SIZE	DT_REG_SIZE(DT_CHOSEN(zephyr_flash))
#elif DT_NODE_HAS_COMPAT_STATUS(DT_CHOSEN(zephyr_flash), jedec_spi_nor, okay)
/* For jedec,spi-nor we expect the spi controller to memory map the flash
 * and for that mapping to be the second register property of the spi
 * controller.
 */
# define SPI_CTRL	DT_PARENT(DT_CHOSEN(zephyr_flash))
# define FLASH_BASE	DT_REG_ADDR_BY_IDX(SPI_CTRL, 1)
# define FLASH_SIZE	DT_REG_SIZE_BY_IDX(SPI_CTRL, 1)
#endif

#ifdef CONFIG_PMP_POWER_OF_TWO_ALIGNMENT
/* To align power-of-2 PMP region size (NAPOT) */

/* Flash Region Definitions */
#if   FLASH_SIZE <= (64 << 10)
#define REGION_FLASH_SIZE KB(64)
#elif FLASH_SIZE <= (128 << 10)
#define REGION_FLASH_SIZE KB(128)
#elif FLASH_SIZE <= (256 << 10)
#define REGION_FLASH_SIZE KB(256)
#elif FLASH_SIZE <= (512 << 10)
#define REGION_FLASH_SIZE KB(512)
#elif FLASH_SIZE <= (1 << 20)
#define REGION_FLASH_SIZE MB(1)
#elif FLASH_SIZE <= (2 << 20)
#define REGION_FLASH_SIZE MB(2)
#elif FLASH_SIZE <= (4 << 20)
#define REGION_FLASH_SIZE MB(4)
#elif FLASH_SIZE <= (8 << 20)
#define REGION_FLASH_SIZE MB(8)
#elif FLASH_SIZE <= (16 << 20)
#define REGION_FLASH_SIZE MB(16)
#elif FLASH_SIZE <= (64 << 20)
#define REGION_FLASH_SIZE MB(64)
#else
#error "Unsupported flash size configuration"
#endif

#else /* CONFIG_PMP_POWER_OF_TWO_ALIGNMENT */
/* To align 4-byte PMP region size (TOR) */

#define REGION_FLASH_SIZE ((FLASH_SIZE + 3) & ~3)

#endif /* CONFIG_PMP_POWER_OF_TWO_ALIGNMENT */

#endif /* _RISCV_PMP_MEM_CFG_H_ */
