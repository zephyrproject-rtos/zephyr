/*
 * Copyright (c) 2024 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_RTS5817_CONTROL_REG_H_
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_RTS5817_CONTROL_REG_H_

#define SYSCLK_BASE_ADDR            0
#define R_SYS_CLK_CHANGE            (SYSCLK_BASE_ADDR + 0X0000)
#define R_SYS_BUS_CLK_CFG_REG       (SYSCLK_BASE_ADDR + 0X0004)
#define R_SYS_SPI_CACHE_CLK_CFG_REG (SYSCLK_BASE_ADDR + 0X0008)
#define R_SYS_SPI_SSOR_CLK_CFG_REG  (SYSCLK_BASE_ADDR + 0X000C)
#define R_SYS_SPI_SSI_M_CLK_CFG_REG (SYSCLK_BASE_ADDR + 0X0010)
#define R_SYS_SPI_SSI_S_CLK_CFG_REG (SYSCLK_BASE_ADDR + 0X0014)
#define R_SYS_SHA_CLK_CFG_REG       (SYSCLK_BASE_ADDR + 0X0018)
#define R_SYS_AES_CLK_CFG_REG       (SYSCLK_BASE_ADDR + 0X001C)
#define R_SYS_PKE_CLK_CFG_REG       (SYSCLK_BASE_ADDR + 0X0020)
#define R_SYS_I2C_CLK_CFG_REG       (SYSCLK_BASE_ADDR + 0X0024)
#define R_SYS_CK60_CFG_REG          (SYSCLK_BASE_ADDR + 0X0028)
#define R_SYS_UART0_CLK_CFG_REG     (SYSCLK_BASE_ADDR + 0X002C)
#define R_SYS_UART1_CLK_CFG_REG     (SYSCLK_BASE_ADDR + 0X0030)
#define R_SYS_SIE_CLK_CFG_REG       (SYSCLK_BASE_ADDR + 0X0034)
#define R_SYS_PUF_CLK_CFG_REG       (SYSCLK_BASE_ADDR + 0X0038)

/* Bits of R_SYS_CLK_CHANGE (0X0300) */

#define CHANGE_BUS_CLK_PRE_OFFSET 0
#define CHANGE_BUS_CLK_PRE_BITS   1
#define CHANGE_BUS_CLK_PRE_MASK   (((1 << 1) - 1) << 0)
#define CHANGE_BUS_CLK_PRE        (CHANGE_BUS_CLK_PRE_MASK)

#define CHANGE_SPI_CACHE_CLK_OFFSET 1
#define CHANGE_SPI_CACHE_CLK_BITS   1
#define CHANGE_SPI_CACHE_CLK_MASK   (((1 << 1) - 1) << 1)
#define CHANGE_SPI_CACHE_CLK        (CHANGE_SPI_CACHE_CLK_MASK)

#define CHANGE_SPI_SSOR_CLK_OFFSET 2
#define CHANGE_SPI_SSOR_CLK_BITS   1
#define CHANGE_SPI_SSOR_CLK_MASK   (((1 << 1) - 1) << 2)
#define CHANGE_SPI_SSOR_CLK        (CHANGE_SPI_SSOR_CLK_MASK)

#define CHANGE_SPI_SSI_M_CLK_OFFSET 3
#define CHANGE_SPI_SSI_M_CLK_BITS   1
#define CHANGE_SPI_SSI_M_CLK_MASK   (((1 << 1) - 1) << 3)
#define CHANGE_SPI_SSI_M_CLK        (CHANGE_SPI_SSI_M_CLK_MASK)

#define CHANGE_SPI_SSI_S_CLK_OFFSET 4
#define CHANGE_SPI_SSI_S_CLK_BITS   1
#define CHANGE_SPI_SSI_S_CLK_MASK   (((1 << 1) - 1) << 4)
#define CHANGE_SPI_SSI_S_CLK        (CHANGE_SPI_SSI_S_CLK_MASK)

#define CHANGE_SHA_CLK_OFFSET 5
#define CHANGE_SHA_CLK_BITS   1
#define CHANGE_SHA_CLK_MASK   (((1 << 1) - 1) << 5)
#define CHANGE_SHA_CLK        (CHANGE_SHA_CLK_MASK)

#define CHANGE_AES_CLK_OFFSET 6
#define CHANGE_AES_CLK_BITS   1
#define CHANGE_AES_CLK_MASK   (((1 << 1) - 1) << 6)
#define CHANGE_AES_CLK        (CHANGE_AES_CLK_MASK)

#define CHANGE_UART0_CLK_OFFSET 7
#define CHANGE_UART0_CLK_BITS   1
#define CHANGE_UART0_CLK_MASK   (((1 << 1) - 1) << 7)
#define CHANGE_UART0_CLK        (CHANGE_UART0_CLK_MASK)

#define CHANGE_UART1_CLK_OFFSET 8
#define CHANGE_UART1_CLK_BITS   1
#define CHANGE_UART1_CLK_MASK   (((1 << 1) - 1) << 8)
#define CHANGE_UART1_CLK        (CHANGE_UART1_CLK_MASK)

/* Bits of R_SYS_BUS_CLK_CFG_REG (0X0304) */

#define BUS_CLK_SRC_SEL_OFFSET 0
#define BUS_CLK_SRC_SEL_BITS   2
#define BUS_CLK_SRC_SEL_MASK   (((1 << 2) - 1) << 0)
#define BUS_CLK_SRC_SEL        (BUS_CLK_SRC_SEL_MASK)

#define BUS_CLK_DIV_OFFSET 2
#define BUS_CLK_DIV_BITS   4
#define BUS_CLK_DIV_MASK   (((1 << 4) - 1) << 2)
#define BUS_CLK_DIV        (BUS_CLK_DIV_MASK)

#define BUS_CLK_NDIV_OFFSET 8
#define BUS_CLK_NDIV_BITS   2
#define BUS_CLK_NDIV_MASK   (((1 << 2) - 1) << 8)
#define BUS_CLK_NDIV        (BUS_CLK_NDIV_MASK)

#define BUS_CLK_FDIV_OFFSET 16
#define BUS_CLK_FDIV_BITS   6
#define BUS_CLK_FDIV_MASK   (((1 << 6) - 1) << 16)
#define BUS_CLK_FDIV        (BUS_CLK_FDIV_MASK)

#define GE_CLK_EN_OFFSET 25
#define GE_CLK_EN_BITS   1
#define GE_CLK_EN_MASK   (((1 << 1) - 1) << 25)
#define GE_CLK_EN        (GE_CLK_EN_MASK)

/* Bits of R_SYS_SPI_CACHE_CLK_CFG_REG (0X0308) */

#define SPI_CACHE_CLK_SEL_OFFSET 0
#define SPI_CACHE_CLK_SEL_BITS   2
#define SPI_CACHE_CLK_SEL_MASK   (((1 << 2) - 1) << 0)
#define SPI_CACHE_CLK_SEL        (SPI_CACHE_CLK_SEL_MASK)

#define SPI_CACHE_CLK_DIV_OFFSET 2
#define SPI_CACHE_CLK_DIV_BITS   3
#define SPI_CACHE_CLK_DIV_MASK   (((1 << 3) - 1) << 2)
#define SPI_CACHE_CLK_DIV        (SPI_CACHE_CLK_DIV_MASK)

#define SPI_CACHE_CLK_NDIV_OFFSET 8
#define SPI_CACHE_CLK_NDIV_BITS   2
#define SPI_CACHE_CLK_NDIV_MASK   (((1 << 2) - 1) << 8)
#define SPI_CACHE_CLK_NDIV        (SPI_CACHE_CLK_NDIV_MASK)

#define SPI_CACHE_CLK_FDIV_OFFSET 16
#define SPI_CACHE_CLK_FDIV_BITS   6
#define SPI_CACHE_CLK_FDIV_MASK   (((1 << 6) - 1) << 16)
#define SPI_CACHE_CLK_FDIV        (SPI_CACHE_CLK_FDIV_MASK)

#define SPI_CACHE_CLK_EN_OFFSET 24
#define SPI_CACHE_CLK_EN_BITS   1
#define SPI_CACHE_CLK_EN_MASK   (((1 << 1) - 1) << 24)
#define SPI_CACHE_CLK_EN        (SPI_CACHE_CLK_EN_MASK)

/* Bits of R_SYS_SPI_SSOR_CLK_CFG_REG (0X030C) */

#define SPI_SSOR_CLK_SEL_OFFSET 0
#define SPI_SSOR_CLK_SEL_BITS   2
#define SPI_SSOR_CLK_SEL_MASK   (((1 << 2) - 1) << 0)
#define SPI_SSOR_CLK_SEL        (SPI_SSOR_CLK_SEL_MASK)

#define SPI_SSOR_CLK_DIV_OFFSET 2
#define SPI_SSOR_CLK_DIV_BITS   3
#define SPI_SSOR_CLK_DIV_MASK   (((1 << 3) - 1) << 2)
#define SPI_SSOR_CLK_DIV        (SPI_SSOR_CLK_DIV_MASK)

#define SPI_SSOR_CLK_NDIV_OFFSET 8
#define SPI_SSOR_CLK_NDIV_BITS   2
#define SPI_SSOR_CLK_NDIV_MASK   (((1 << 2) - 1) << 8)
#define SPI_SSOR_CLK_NDIV        (SPI_SSOR_CLK_NDIV_MASK)

#define SPI_SSOR_CLK_FDIV_OFFSET 16
#define SPI_SSOR_CLK_FDIV_BITS   6
#define SPI_SSOR_CLK_FDIV_MASK   (((1 << 6) - 1) << 16)
#define SPI_SSOR_CLK_FDIV        (SPI_SSOR_CLK_FDIV_MASK)

#define SPI_SSOR_CLK_EN_OFFSET 24
#define SPI_SSOR_CLK_EN_BITS   1
#define SPI_SSOR_CLK_EN_MASK   (((1 << 1) - 1) << 24)
#define SPI_SSOR_CLK_EN        (SPI_SSOR_CLK_EN_MASK)

/* Bits of R_SYS_SPI_SSI_M_CLK_CFG_REG (0X0310) */

#define SPI_SSI_M_CLK_SEL_OFFSET 0
#define SPI_SSI_M_CLK_SEL_BITS   2
#define SPI_SSI_M_CLK_SEL_MASK   (((1 << 2) - 1) << 0)
#define SPI_SSI_M_CLK_SEL        (SPI_SSI_M_CLK_SEL_MASK)

#define SPI_SSI_M_CLK_DIV_OFFSET 2
#define SPI_SSI_M_CLK_DIV_BITS   3
#define SPI_SSI_M_CLK_DIV_MASK   (((1 << 3) - 1) << 2)
#define SPI_SSI_M_CLK_DIV        (SPI_SSI_M_CLK_DIV_MASK)

#define SPI_SSI_M_CLK_NDIV_OFFSET 8
#define SPI_SSI_M_CLK_NDIV_BITS   2
#define SPI_SSI_M_CLK_NDIV_MASK   (((1 << 2) - 1) << 8)
#define SPI_SSI_M_CLK_NDIV        (SPI_SSI_M_CLK_NDIV_MASK)

#define SPI_SSI_M_CLK_FDIV_OFFSET 16
#define SPI_SSI_M_CLK_FDIV_BITS   6
#define SPI_SSI_M_CLK_FDIV_MASK   (((1 << 6) - 1) << 16)
#define SPI_SSI_M_CLK_FDIV        (SPI_SSI_M_CLK_FDIV_MASK)

#define SPI_SSI_M_CLK_EN_OFFSET 24
#define SPI_SSI_M_CLK_EN_BITS   1
#define SPI_SSI_M_CLK_EN_MASK   (((1 << 1) - 1) << 24)
#define SPI_SSI_M_CLK_EN        (SPI_SSI_M_CLK_EN_MASK)

/* Bits of R_SYS_SPI_SSI_S_CLK_CFG_REG (0X0314) */

#define SPI_SSI_S_CLK_SEL_OFFSET 0
#define SPI_SSI_S_CLK_SEL_BITS   2
#define SPI_SSI_S_CLK_SEL_MASK   (((1 << 2) - 1) << 0)
#define SPI_SSI_S_CLK_SEL        (SPI_SSI_S_CLK_SEL_MASK)

#define SPI_SSI_S_CLK_DIV_OFFSET 2
#define SPI_SSI_S_CLK_DIV_BITS   3
#define SPI_SSI_S_CLK_DIV_MASK   (((1 << 3) - 1) << 2)
#define SPI_SSI_S_CLK_DIV        (SPI_SSI_S_CLK_DIV_MASK)

#define SPI_SSI_S_CLK_NDIV_OFFSET 8
#define SPI_SSI_S_CLK_NDIV_BITS   2
#define SPI_SSI_S_CLK_NDIV_MASK   (((1 << 2) - 1) << 8)
#define SPI_SSI_S_CLK_NDIV        (SPI_SSI_S_CLK_NDIV_MASK)

#define SPI_SSI_S_CLK_FDIV_OFFSET 16
#define SPI_SSI_S_CLK_FDIV_BITS   6
#define SPI_SSI_S_CLK_FDIV_MASK   (((1 << 6) - 1) << 16)
#define SPI_SSI_S_CLK_FDIV        (SPI_SSI_S_CLK_FDIV_MASK)

#define SPI_SSI_S_CLK_EN_OFFSET 24
#define SPI_SSI_S_CLK_EN_BITS   1
#define SPI_SSI_S_CLK_EN_MASK   (((1 << 1) - 1) << 24)
#define SPI_SSI_S_CLK_EN        (SPI_SSI_S_CLK_EN_MASK)

/* Bits of R_SYS_SHA_CLK_CFG_REG (0X0318) */

#define SHA_CLK_SEL_OFFSET 0
#define SHA_CLK_SEL_BITS   2
#define SHA_CLK_SEL_MASK   (((1 << 2) - 1) << 0)
#define SHA_CLK_SEL        (SHA_CLK_SEL_MASK)

#define SHA_CLK_DIV_OFFSET 2
#define SHA_CLK_DIV_BITS   3
#define SHA_CLK_DIV_MASK   (((1 << 3) - 1) << 2)
#define SHA_CLK_DIV        (SHA_CLK_DIV_MASK)

#define SHA_CLK_NDIV_OFFSET 8
#define SHA_CLK_NDIV_BITS   2
#define SHA_CLK_NDIV_MASK   (((1 << 2) - 1) << 8)
#define SHA_CLK_NDIV        (SHA_CLK_NDIV_MASK)

#define SHA_CLK_FDIV_OFFSET 16
#define SHA_CLK_FDIV_BITS   6
#define SHA_CLK_FDIV_MASK   (((1 << 6) - 1) << 16)
#define SHA_CLK_FDIV        (SHA_CLK_FDIV_MASK)

#define SHA_CLK_EN_OFFSET 24
#define SHA_CLK_EN_BITS   1
#define SHA_CLK_EN_MASK   (((1 << 1) - 1) << 24)
#define SHA_CLK_EN        (SHA_CLK_EN_MASK)

/* Bits of R_SYS_AES_CLK_CFG_REG (0X031C) */

#define AES_CLK_SEL_OFFSET 0
#define AES_CLK_SEL_BITS   2
#define AES_CLK_SEL_MASK   (((1 << 2) - 1) << 0)
#define AES_CLK_SEL        (AES_CLK_SEL_MASK)

#define AES_CLK_DIV_OFFSET 2
#define AES_CLK_DIV_BITS   3
#define AES_CLK_DIV_MASK   (((1 << 3) - 1) << 2)
#define AES_CLK_DIV        (AES_CLK_DIV_MASK)

#define AES_CLK_NDIV_OFFSET 8
#define AES_CLK_NDIV_BITS   2
#define AES_CLK_NDIV_MASK   (((1 << 2) - 1) << 8)
#define AES_CLK_NDIV        (AES_CLK_NDIV_MASK)

#define AES_CLK_FDIV_OFFSET 16
#define AES_CLK_FDIV_BITS   6
#define AES_CLK_FDIV_MASK   (((1 << 6) - 1) << 16)
#define AES_CLK_FDIV        (AES_CLK_FDIV_MASK)

#define AES_CLK_EN_OFFSET 24
#define AES_CLK_EN_BITS   1
#define AES_CLK_EN_MASK   (((1 << 1) - 1) << 24)
#define AES_CLK_EN        (AES_CLK_EN_MASK)

/* Bits of R_SYS_PKE_CLK_CFG_REG (0X0320) */

#define PKE_CLK_DIV_OFFSET 2
#define PKE_CLK_DIV_BITS   3
#define PKE_CLK_DIV_MASK   (((1 << 3) - 1) << 2)
#define PKE_CLK_DIV        (PKE_CLK_DIV_MASK)

#define PKE_CLK_NDIV_OFFSET 8
#define PKE_CLK_NDIV_BITS   2
#define PKE_CLK_NDIV_MASK   (((1 << 2) - 1) << 8)
#define PKE_CLK_NDIV        (PKE_CLK_NDIV_MASK)

#define PKE_CLK_FDIV_OFFSET 16
#define PKE_CLK_FDIV_BITS   6
#define PKE_CLK_FDIV_MASK   (((1 << 6) - 1) << 16)
#define PKE_CLK_FDIV        (PKE_CLK_FDIV_MASK)

#define PKE_CLK_EN_OFFSET 24
#define PKE_CLK_EN_BITS   1
#define PKE_CLK_EN_MASK   (((1 << 1) - 1) << 24)
#define PKE_CLK_EN        (PKE_CLK_EN_MASK)

/* Bits of R_SYS_I2C_CLK_CFG_REG (0X0324) */

#define I2C_CLK_DIV_OFFSET 2
#define I2C_CLK_DIV_BITS   3
#define I2C_CLK_DIV_MASK   (((1 << 3) - 1) << 2)
#define I2C_CLK_DIV        (I2C_CLK_DIV_MASK)

#define I2C_CLK_NDIV_OFFSET 8
#define I2C_CLK_NDIV_BITS   2
#define I2C_CLK_NDIV_MASK   (((1 << 2) - 1) << 8)
#define I2C_CLK_NDIV        (I2C_CLK_NDIV_MASK)

#define I2C_CLK_FDIV_OFFSET 16
#define I2C_CLK_FDIV_BITS   6
#define I2C_CLK_FDIV_MASK   (((1 << 6) - 1) << 16)
#define I2C_CLK_FDIV        (I2C_CLK_FDIV_MASK)

#define I2C_CLK_EN_OFFSET 24
#define I2C_CLK_EN_BITS   1
#define I2C_CLK_EN_MASK   (((1 << 1) - 1) << 24)
#define I2C_CLK_EN        (I2C_CLK_EN_MASK)

#define I2C0_CLK_EN_OFFSET 25
#define I2C0_CLK_EN_BITS   1
#define I2C0_CLK_EN_MASK   (((1 << 1) - 1) << 25)
#define I2C0_CLK_EN        (I2C0_CLK_EN_MASK)

#define I2C1_CLK_EN_OFFSET 26
#define I2C1_CLK_EN_BITS   1
#define I2C1_CLK_EN_MASK   (((1 << 1) - 1) << 26)
#define I2C1_CLK_EN        (I2C1_CLK_EN_MASK)

/* Bits of R_SYS_CK60_CFG_REG (0X0328) */

#define I2C_S_CLK_EN_OFFSET 24
#define I2C_S_CLK_EN_BITS   1
#define I2C_S_CLK_EN_MASK   (((1 << 1) - 1) << 24)
#define I2C_S_CLK_EN        (I2C_S_CLK_EN_MASK)

#define TRNG_CLK_EN_OFFSET 25
#define TRNG_CLK_EN_BITS   1
#define TRNG_CLK_EN_MASK   (((1 << 1) - 1) << 25)
#define TRNG_CLK_EN        (TRNG_CLK_EN_MASK)

/* Bits of R_SYS_UART0_CLK_CFG_REG (0X032C) */

#define UART0_CLK_SEL_OFFSET 0
#define UART0_CLK_SEL_BITS   2
#define UART0_CLK_SEL_MASK   (((1 << 2) - 1) << 0)
#define UART0_CLK_SEL        (UART0_CLK_SEL_MASK)

#define UART0_CLK_DIV_OFFSET 2
#define UART0_CLK_DIV_BITS   3
#define UART0_CLK_DIV_MASK   (((1 << 3) - 1) << 2)
#define UART0_CLK_DIV        (UART0_CLK_DIV_MASK)

#define UART0_CLK_NDIV_OFFSET 8
#define UART0_CLK_NDIV_BITS   2
#define UART0_CLK_NDIV_MASK   (((1 << 2) - 1) << 8)
#define UART0_CLK_NDIV        (UART0_CLK_NDIV_MASK)

#define UART0_CLK_FDIV_OFFSET 16
#define UART0_CLK_FDIV_BITS   6
#define UART0_CLK_FDIV_MASK   (((1 << 6) - 1) << 16)
#define UART0_CLK_FDIV        (UART0_CLK_FDIV_MASK)

#define UART0_CLK_EN_OFFSET 24
#define UART0_CLK_EN_BITS   1
#define UART0_CLK_EN_MASK   (((1 << 1) - 1) << 24)
#define UART0_CLK_EN        (UART0_CLK_EN_MASK)

/* Bits of R_SYS_UART1_CLK_CFG_REG (0X0330) */

#define UART1_CLK_SEL_OFFSET 0
#define UART1_CLK_SEL_BITS   2
#define UART1_CLK_SEL_MASK   (((1 << 2) - 1) << 0)
#define UART1_CLK_SEL        (UART1_CLK_SEL_MASK)

#define UART1_CLK_DIV_OFFSET 2
#define UART1_CLK_DIV_BITS   3
#define UART1_CLK_DIV_MASK   (((1 << 3) - 1) << 2)
#define UART1_CLK_DIV        (UART1_CLK_DIV_MASK)

#define UART1_CLK_NDIV_OFFSET 8
#define UART1_CLK_NDIV_BITS   2
#define UART1_CLK_NDIV_MASK   (((1 << 2) - 1) << 8)
#define UART1_CLK_NDIV        (UART1_CLK_NDIV_MASK)

#define UART1_CLK_FDIV_OFFSET 16
#define UART1_CLK_FDIV_BITS   6
#define UART1_CLK_FDIV_MASK   (((1 << 6) - 1) << 16)
#define UART1_CLK_FDIV        (UART1_CLK_FDIV_MASK)

#define UART1_CLK_EN_OFFSET 24
#define UART1_CLK_EN_BITS   1
#define UART1_CLK_EN_MASK   (((1 << 1) - 1) << 24)
#define UART1_CLK_EN        (UART1_CLK_EN_MASK)

/* Bits of R_SYS_SIE_CLK_CFG_REG (0X0334) */

#define SIE_CLK_EN_OFFSET 24
#define SIE_CLK_EN_BITS   1
#define SIE_CLK_EN_MASK   (((1 << 1) - 1) << 24)
#define SIE_CLK_EN        (SIE_CLK_EN_MASK)

/* Bits of R_SYS_PUF_CLK_CFG_REG (0X0338) */

#define PUF_CLK_EN_OFFSET 24
#define PUF_CLK_EN_BITS   1
#define PUF_CLK_EN_MASK   (((1 << 1) - 1) << 24)
#define PUF_CLK_EN        (PUF_CLK_EN_MASK)

/* Parameters definition */

/* Parameters of R_SYS_BUS_CLK_CFG_REG (0x0304) */
#define BUS_CLK_240M  0x0
#define BUS_CLK_160M  0x1
#define BUS_CLK_96M   0x2
#define BUS_CLK_GPLL0 0x3

#define BUS_CLK_DIV_1 0x0
#define BUS_CLK_DIV_2 0x1
#define BUS_CLK_DIV_4 0x2
#define BUS_CLK_DIV_6 0x3

/* Parameters of R_SYS_SPI_CACHE_CLK_CFG_REG (0X0308)*/
#define SPI_CACHE_CLK_240M  0x0
#define SPI_CACHE_CLK_160M  0x1
#define SPI_CACHE_CLK_96M   0x2
#define SPI_CACHE_CLK_GPLL0 0x3

#define SPI_CACHE_CLK_DIV_1 0x0
#define SPI_CACHE_CLK_DIV_2 0x1
#define SPI_CACHE_CLK_DIV_4 0x2
#define SPI_CACHE_CLK_DIV_6 0x3

/* Parameters of R_SYS_SPI_SSOR_CLK_CFG_REG (0X030C) */
#define SPI_SSOR_CLK_240M  0x0
#define SPI_SSOR_CLK_96M   0x1
#define SPI_SSOR_CLK_80M   0x2
#define SPI_SSOR_CLK_GPLL0 0x3

#define SPI_SSOR_CLK_DIV_1 0x0
#define SPI_SSOR_CLK_DIV_2 0x1
#define SPI_SSOR_CLK_DIV_4 0x2
#define SPI_SSOR_CLK_DIV_6 0x3

/* Parameters of R_SYS_SPI_SSI_M_CLK_CFG_REG (0X0310) */
#define SPI_SSI_MST_CLK_240M  0x0
#define SPI_SSI_MST_CLK_96M   0x1
#define SPI_SSI_MST_CLK_80M   0x2
#define SPI_SSI_MST_CLK_GPLL0 0x3

#define SPI_SSI_MST_CLK_DIV_1 0x0
#define SPI_SSI_MST_CLK_DIV_2 0x1
#define SPI_SSI_MST_CLK_DIV_4 0x2
#define SPI_SSI_MST_CLK_DIV_6 0x3

/* Parameters of R_SYS_SPI_SSI_S_CLK_CFG_REG (0X0314) */
#define SPI_SSI_SLV_CLK_240M  0x0
#define SPI_SSI_SLV_CLK_96M   0x1
#define SPI_SSI_SLV_CLK_80M   0x2
#define SPI_SSI_SLV_CLK_GPLL0 0x3

#define SPI_SSI_SLV_CLK_DIV_1 0x0
#define SPI_SSI_SLV_CLK_DIV_2 0x1
#define SPI_SSI_SLV_CLK_DIV_4 0x2
#define SPI_SSI_SLV_CLK_DIV_6 0x3

/* Parameters of R_SYS_SHA_CLK_CFG_REG (0X0318) */
#define SHA_CLK_240M  0x0
#define SHA_CLK_160M  0x1
#define SHA_CLK_96M   0x2
#define SHA_CLK_GPLL0 0x3

#define SHA_CLK_DIV_1 0x0
#define SHA_CLK_DIV_2 0x1
#define SHA_CLK_DIV_4 0x2
#define SHA_CLK_DIV_6 0x3

/* Parameters of R_SYS_AES_CLK_CFG_REG (0X031C) */
#define AES_CLK_240M  0x0
#define AES_CLK_160M  0x1
#define AES_CLK_96M   0x2
#define AES_CLK_GPLL0 0x3

#define AES_CLK_DIV_1 0x0
#define AES_CLK_DIV_2 0x1
#define AES_CLK_DIV_4 0x2
#define AES_CLK_DIV_6 0x3

/* Parameters of R_SYS_I2C_CLK_CFG_REG (0X0324) */
#define I2C_CLK_DIV_1 0x0
#define I2C_CLK_DIV_2 0x1
#define I2C_CLK_DIV_4 0x2
#define I2C_CLK_DIV_6 0x3

/* Parameters of R_SYS_UARTx_CLK_CFG_REG (0X032C 0X330) */
#define UART_CLK_96M   0x0
#define UART_CLK_120M  0x1
#define UART_CLK_GPLL0 0x3

#define UART_CLK_DIV_1 0x0
#define UART_CLK_DIV_2 0x1
#define UART_CLK_DIV_4 0x2
#define UART_CLK_DIV_6 0x3

/***********************Definition created by FW**********************/
#define SENSOR_SPI_CLK_SEL_240M   0x00
#define SENSOR_SPI_CLK_SEL_96M    0x01
#define SENSOR_SPI_CLK_SEL_80M    0x02
#define SENSOR_SPI_CLK_SEL_SYSPLL 0x03

#define SSI_SPI_MST_CLK_SEL_240M   0x00
#define SSI_SPI_MST_CLK_SEL_96M    0x01
#define SSI_SPI_MST_CLK_SEL_80M    0x02
#define SSI_SPI_MST_CLK_SEL_SYSPLL 0x03

#endif /* ZEPHYR_DRIVERS_CLOCK_CONTROL_RTS5817_CONTROL_REG_H_ */
