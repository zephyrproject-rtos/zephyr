/* spi_pw.h - Penwell SPI driver definitions */

/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SPI_SPI_PW_H_
#define ZEPHYR_DRIVERS_SPI_SPI_PW_H_

#include "spi_context.h"

/* lpss penwell spi registers */
#define PW_SPI_REG_CTRLR0               0x00
#define PW_SPI_REG_CTRLR1               0x04
#define PW_SPI_REG_SSSR                 0x08
#define PW_SPI_REG_SSDR                 0x10
#define PW_SPI_REG_SSTO                 0x28
#define PW_SPI_REG_SITF                 0x44
#define PW_SPI_REG_SIRF                 0x48

#define PW_SPI_REG_CLKS                 0x200
#define PW_SPI_REG_RESETS               0x204
#define PW_SPI_REG_ACTIVE_LTR           0x210
#define PW_SPI_REG_IDLE_LTR             0x217
#define PW_SPI_REG_TX_BIT_COUNT         0x218
#define PW_SPI_REG_RX_BIT_COUNT         0x21c
#define PW_SPI_REG_DMA_FINISH_DIS       0x220

#define PW_SPI_REG_CS_CTRL              0x224
#define PW_SPI_REG_SW_SCRATCH           0x228
#define PW_SPI_REG_CLK_GATE             0x238
#define PW_SPI_REG_REMAP_ADDR_LO        0x240
#define PW_SPI_REG_REMAP_ADDR_HI        0x244
#define PW_SPI_REG_DEV_IDLE_CTRL        0x24c
#define PW_SPI_REG_DEL_RX_CLK           0x250
#define PW_SPI_REG_CAP                  0x2fc

/* CTRLR0 settings */
#define PW_SPI_CTRLR0_SSE_BIT           BIT(7)
#define PW_SPI_CTRLR0_EDSS_BIT          BIT(20)
#define PW_SPI_CTRLR0_RIM_BIT           BIT(22)
#define PW_SPI_CTRLR0_TIM_BIT           BIT(23)
#define PW_SPI_CTRLR0_MOD_BIT           BIT(31)

#define PW_SPI_CTRLR0_DATA_MASK         (~(0xf << 0))
#define PW_SPI_CTRLR0_EDSS_MASK         (~(0x1 << 20))

/* Data size set bits sscr0[3:0] */
#define PW_SPI_DATA_SIZE_4_BIT          0x3
#define PW_SPI_DATA_SIZE_8_BIT          0x7
#define PW_SPI_DATA_SIZE_16_BIT         0xf
#define PW_SPI_DATA_SIZE_32_BIT         (PW_SPI_CTRLR0_EDSS_BIT | \
					 PW_SPI_DATA_SIZE_16_BIT)
/* Frame format sscr0[5:4] */
#define PW_SPI_FRF_MOTOROLA             (~(0x3 << 4))

/* SSP Baud rate sscr0[19:8] */
#define PW_SPI_BR_2MHZ                  0x31
#define PW_SPI_BR_4MHZ                  0x18
#define PW_SPI_BR_5MHZ                  0x13
#define PW_SPI_BR_10MHZ                 0x9
#define PW_SPI_BR_20MHZ                 0x5
#define PW_SPI_BR_MAX_FRQ               20000000	/* 20 MHz */
/* [19:8] 12 bits */
#define PW_SPI_SCR_MASK                 (BIT_MASK(12) << 8)
#define PW_SPI_SCR_SHIFT                0x8

/* CTRLR1 settings */
#define PW_SPI_CTRL1_RIE_BIT            BIT(0)
#define PW_SPI_CTRL1_TIE_BIT            BIT(1)
#define PW_SPI_CTRL1_LBM_BIT            BIT(2)
#define PW_SPI_CTRL1_SPO_BIT            BIT(3)
#define PW_SPI_CTRL1_SPH_BIT            BIT(4)
#define PW_SPI_CTRL1_IFS_BIT            BIT(16)
#define PW_SPI_CTRL1_TINTE_BIT          BIT(19)
#define PW_SPI_CTRL1_RSRE_BIT           BIT(20)
#define PW_SPI_CTRL1_TSRE_BIT           BIT(21)
#define PW_SPI_CTRL1_TRAIL_BIT          BIT(22)
#define PW_SPI_CTRL1_RWOT_BIT           BIT(23)

/* [4:3] phase & polarity mask */
#define PW_SPI_CTRL1_SPO_SPH_MASK       (BIT_MASK(2) << 3)

/* Status Register */
#define PW_SPI_SSSR_TNF_BIT             BIT(2)
#define PW_SPI_SSSR_RNE_BIT             BIT(3)
#define PW_SPI_SSSR_BSY_BIT             BIT(4)
#define PW_SPI_SSSR_TFS_BIT             BIT(5)
#define PW_SPI_SSSR_RFS_BIT             BIT(6)
#define PW_SPI_SSSR_ROR_BIT             BIT(7)
#define PW_SPI_SSSR_PINT_BIT            BIT(18)
#define PW_SPI_SSSR_TINT_BIT            BIT(19)
#define PW_SPI_SSSR_TUR_BIT             BIT(21)

/* SPI Tx FIFO Higher Water Mark [5:0] */
#define PW_SPI_SITF_HWM_1_ENTRY         0x1
#define PW_SPI_SITF_HWM_4_ENTRY         0x4
#define PW_SPI_SITF_HWM_8_ENTRY         0x8
#define PW_SPI_SITF_HWM_16_ENTRY        0x10
#define PW_SPI_SITF_HWM_32_ENTRY        0x20
#define PW_SPI_SITF_HWM_64_ENTRY        0x40

/* SPI Tx FIFO Lower Water Mark[13:8] */
#define PW_SPI_SITF_LWM_2_ENTRY         (BIT(0) << 8)
#define PW_SPI_SITF_LWM_3_ENTRY         (BIT(1) << 8)
#define PW_SPI_SITF_LWM_4_ENTRY         ((BIT(1) | BIT(0)) << 8)

/* SPI Tx FIFO Level SITF[21:16] */
#define PW_SPI_SITF_SITFL_MASK          (BIT_MASK(6) << 16)

#define PW_SPI_SITF_SITFL_SHIFT         0x10

/* SPI Rx FIFO water mark */
#define PW_SPI_SIRF_WMRF_1_ENTRY        0x1
#define PW_SPI_SIRF_WMRF_2_ENTRY        0x2
#define PW_SPI_SIRF_WMRF_4_ENTRY        0x4
#define PW_SPI_SITF_WMRF_8_ENTRY        0x8
#define PW_SPI_SITF_WMRF_16_ENTRY       0x10
#define PW_SPI_SITF_WMRF_32_ENTRY       0x20
#define PW_SPI_SITF_WMRF_64_ENTRY       0x40

/* SPI Rx FIFO Level RITF[13:8] */
#define PW_SPI_SIRF_SIRFL_MASK          (BIT_MASK(6) << 8)
#define PW_SPI_SIRF_SIRFL_SHIFT         0x8

/* Threshold default value */
#define PW_SPI_WM_MASK                  BIT_MASK(6)
#define PW_SPI_SITF_LWMTF_SHIFT         0x8
#define PW_SPI_SITF_LOW_WM_DFLT         BIT(PW_SPI_SITF_LWMTF_SHIFT)
#define PW_SPI_SITF_HIGH_WM_DFLT        0x20
#define PW_SPI_SIRF_WM_DFLT             0x28

/* Clocks */
#define PW_SPI_CLKS_EN_BIT              BIT(0)
#define PW_SPI_CLKS_MVAL                BIT(1)
#define PW_SPI_CLKS_NVAL                BIT(16)
#define PW_SPI_CLKS_UPDATE_BIT          BIT(31)

/* mval mask [15:1] */
#define PW_SPI_CLKS_MVAL_MASK           (BIT_MASK(15) << 1)

/* nval mask [30:16] */
#define PW_SPI_CLKS_NVAL_MASK           (BIT_MASK(15) << 16)

/* SPI chip select control */
#define PW_SPI_CS_MODE_BIT              0
#define PW_SPI_CS_STATE_BIT             1
#define PW_SPI_CS0_POL_BIT              12
#define PW_SPI_CS1_POL_BIT              13

/* ssp interrupt error bits */
#define PW_SPI_INTR_ERRORS_MASK         (PW_SPI_SSSR_TUR_BIT | \
					 PW_SPI_SSSR_ROR_BIT | \
					 PW_SPI_SSSR_TINT_BIT)

/* ssp interrupt bits */
#define PW_SPI_INTR_BITS                (PW_SPI_CTRL1_TIE_BIT |	\
					 PW_SPI_CTRL1_RIE_BIT |	\
					 PW_SPI_CTRL1_TINTE_BIT)

#define PW_SPI_INTR_MASK_TX             (~(PW_SPI_CTRL1_TIE_BIT | \
					   PW_SPI_CTRL1_TINTE_BIT))

#define PW_SPI_INTR_MASK_RX             (PW_SPI_CTRL1_RIE_BIT)

/* SSP & DMA reset  */
#define PW_SPI_INST_RESET               0x7

/* Chip select control */
#define PW_SPI_CS_CTRL_SW_MODE          BIT(0)
#define PW_SPI_CS_HIGH                  BIT(1)
#define PW_SPI_CS_LOW                   (~(PW_SPI_CS_HIGH))
#define PW_SPI_CS_CTRL_CS_MASK          0x3
#define PW_SPI_CS_EN_SHIFT              0x8
#define PW_SPI_CS0_SELECT               (~(BIT(PW_SPI_CS_EN_SHIFT)))
#define PW_SPI_CS1_SELECT               BIT(PW_SPI_CS_EN_SHIFT)
#define PW_SPI_CS_CTRL_HW_MODE          (~(PW_SPI_CS_CTRL_SW_MODE))

#define PW_SPI_WIDTH_8BITS              8
#define PW_SPI_FRAME_SIZE_1_BYTE        1
#define PW_SPI_FRAME_SIZE_2_BYTES       2
#define PW_SPI_FRAME_SIZE_4_BYTES       4

#define PW_SPI_CS1_OUTPUT_SELECT	1

enum spi_pw_spo_sph_mode {
	SPI_PW_MODE0 = 0,
	SPI_PW_MODE1,
	SPI_PW_MODE2,
	SPI_PW_MODE3,
};

enum spi_pw_cs_mode {
	CS_HW_MODE = 0,
	CS_SW_MODE,
	CS_GPIO_MODE,
};

struct spi_pw_config {
	uint32_t id;
#ifdef CONFIG_SPI_PW_INTERRUPT
	void (*irq_config)(const struct device *dev);
#endif
	uint32_t clock_freq;
	uint8_t op_modes;
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(pcie)
	struct pcie_dev *pcie;
#endif
};

struct spi_pw_data {
	DEVICE_MMIO_RAM;
	struct spi_context ctx;
	uint8_t dfs;
	uint8_t fifo_diff;
	uint8_t cs_mode;
	uint8_t cs_output;
	uint32_t id;
	uint8_t fifo_depth;
};

#endif /* ZEPHYR_DRIVERS_SPI_SPI_PW_H_ */
