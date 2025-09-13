/*
 * Copyright (c) 2025-2026 Macronix International Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_instance.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys_clock.h>
#include <stdlib.h>
#include <zephyr/irq.h>
#include <zephyr/sys/byteorder.h>

enum HC_XFER_MODE_TYPE {
	HC_XFER_MODE_IO,
	HC_XFER_MODE_MAP,
	HC_XFER_MODE_DMA,
	MAX_HC_XFER_MODE
};

#define CHIP_SELECT_COUNT        3u
#define SPI_WORD_SIZE            8u
#define SPI_WR_RD_CHUNK_SIZE_MAX 16u
#define MXICY_UEFC_CMD_LENGTH    2
#define MXICY_UEFC_ADDR_LENGTH   4

/* Host Controller Register */
#define HC_CTRL                  0x00
#define HC_CTRL_DATA_ORDER       BIT(25)
#define HC_CTRL_SIO_SHIFTER(x)   (((x)&0x3) << 23)
#define HC_CTRL_SIO_SHIFTER_MASK GENMASK(24, 23)
#define HC_CTRL_CH_SEL_B         BIT(11)
#define HC_CTRL_CH_SEL_A         0
#define HC_CTRL_CH_MASK          BIT(11)
#define HC_CTRL_LUN_SEL(x)       (((x)&0x7) << 8)
#define HC_CTRL_LUN_MASK         HC_CTRL_LUN_SEL(0x7)
#define HC_CTRL_PORT_SEL(x)      (((x)&0xff) << 0)
#define HC_CTRL_PORT_MASK        (HC_CTRL_PORT_SEL(0xff))

#define HC_CMD_LENGTH_MASK  OP_CMD_CNT(0x7)
#define HC_ADDR_LENGTH_MASK OP_ADDR_CNT(0x7)

#define HC_CTRL_CH_LUN_PORT_MASK (HC_CTRL_CH_MASK | HC_CTRL_LUN_MASK | HC_CTRL_PORT_MASK)
#define HC_CTRL_CH_LUN_PORT(ch, lun, port)                                                         \
	(HC_CTRL_CH_SEL_##ch | HC_CTRL_LUN_SEL(lun) | HC_CTRL_PORT_SEL(port))

/* Normal Interrupt Status Register */
#define INT_STS                   0x04
#define INT_STS_AC_RDY            BIT(28)
#define INT_STS_ERR_INT           BIT(15)
#define INT_STS_DMA_TFR_CMPLT_BIT BIT(7)
#define INT_STS_DMA_INT_BIT       BIT(6)
#define INT_STS_ALL_CLR                                                                            \
	(INT_STS_AC_RDY | INT_STS_ERR_INT | INT_STS_DMA_TFR_CMPLT_BIT | INT_STS_DMA_INT_BIT)

/* Error Interrupt Status Register */
#define ERR_INT_STS       0x08
#define ERR_INT_STS_ECC   BIT(19)
#define ERR_INT_STS_PREAM BIT(18)
#define ERR_INT_STS_CRC   BIT(17)
#define ERR_INT_STS_AC    BIT(16)
#define ERR_INT_STS_ADMA  BIT(9)
#define ERR_INT_STS_ALL_CLR                                                                        \
	(ERR_INT_STS_ECC | ERR_INT_STS_PREAM | ERR_INT_STS_CRC | ERR_INT_STS_AC | ERR_INT_STS_ADMA)

/* Normal Interrupt Status Enable Register */
#define INT_STS_EN                   0x0C
#define INT_STS_EN_AC_RDY            BIT(28)
#define INT_STS_EN_ERR_INT           BIT(15)
#define INT_STS_EN_DMA_TFR_CMPLT_BIT BIT(7)
#define INT_STS_DMA_BIT              BIT(6)
#define INT_STS_EN_ALL_EN                                                                          \
	(INT_STS_EN_AC_RDY | INT_STS_EN_ERR_INT | INT_STS_EN_DMA_TFR_CMPLT_BIT | INT_STS_DMA_BIT)

/* Error Interrupt Status Enable Register */
#define ERR_INT_STS_EN       0x10
#define ERR_INT_STS_EN_ECC   BIT(19)
#define ERR_INT_STS_EN_PREAM BIT(18)
#define ERR_INT_STS_EN_CRC   BIT(17)
#define ERR_INT_STS_EN_AC    BIT(16)
#define ERR_INT_STS_EN_ADMA  BIT(9)
#define ERR_INT_STS_EN_ALL_EN                                                                      \
	(ERR_INT_STS_EN_ECC | ERR_INT_STS_EN_PREAM | ERR_INT_STS_EN_CRC | ERR_INT_STS_EN_AC |      \
	 ERR_INT_STS_EN_ADMA)

/* Normal Interrupt Signal Enable Register */
#define INT_STS_SIG_EN               0x14
#define INT_STS_SIG_EN_AC_RDY        BIT(28)
#define INT_STS_SIG_EN_ERR_INT       BIT(15)
#define INT_STS_SIG_EN_DMA_TFR_CMPLT BIT(7)
#define INT_STS_SIG_EN_DMA_INT       BIT(3)
#define INT_STS_SIG_EN_ALL_EN                                                                      \
	(INT_STS_SIG_EN_AC_RDY | INT_STS_SIG_EN_ERR_INT | INT_STS_SIG_EN_DMA_TFR_CMPLT |           \
	 INT_STS_SIG_EN_DMA_INT)

/* Error Interrupt Signal Enable Register */
#define ERR_INT_STS_SIG_EN       0x18
#define ERR_INT_STS_SIG_EN_ECC   BIT(19)
#define ERR_INT_STS_SIG_EN_PREAM BIT(18)
#define ERR_INT_STS_SIG_EN_CRC   BIT(17)
#define ERR_INT_STS_SIG_EN_AC    BIT(16)
#define ERR_INT_STS_SIG_EN_ADMA  BIT(9)
#define ERR_INT_STS_SIG_EN_ALL_EN                                                                  \
	(ERR_INT_STS_SIG_EN_ECC | ERR_INT_STS_SIG_EN_PREAM | ERR_INT_STS_SIG_EN_CRC |              \
	 ERR_INT_STS_SIG_EN_AC | ERR_INT_STS_SIG_EN_ADMA)

/* Transfer Mode register */
#define TFR_MODE                    0x1C
#define TFR_MODE_DMA_TYPE           BIT(31)
#define TFR_MODE_CMD_CNT            BIT(17)
#define TFR_MODE_DATA_DTR_BIT       BIT(16)
#define TFR_MODE_ADDR_DTR_BIT       BIT(13)
#define TFR_MODE_CMD_DTR_BIT        BIT(10)
#define TFR_MODE_CNT_EN             BIT(1)
#define TFR_MODE_DMA_EN_BIT         BIT(0)
/* share with MAPRD, MAPWR */
#define OP_DMY_CNT(_len, _dtr, _bw) (((_len * (_dtr + 1)) / (8 / (_bw))) << 21)

#define OP_DMY(x)               (((x)&0x3F) << 21)
#define TFR_MODE_DMY_MASK       GENMASK(26, 21)
#define TFR_MODE_DATA_BUSW_MASK GENMASK(15, 14)
#define TFR_MODE_CMD_BUSW_MASK  GENMASK(9, 8)
#define TFR_MODE_ADDR_BUSW_MASK GENMASK(12, 11)
#define TFR_MODE_ADDR_CNT_MASK  GENMASK(19, 18)

#define OP_ADDR_CNT(x)  (((x)&0x7) << 18)
#define OP_CMD_CNT(x)   (((x)-1) << 17)
#define OP_DATA_BUSW(x) (((x)&0x3) << 14)
#define OP_DATA_DTR(x)  (((x)&0x1) << 16)
#define OP_ADDR_BUSW(x) (((x)&0x3) << 11)
#define OP_ADDR_DTR(x)  (((x)&0x1) << 13)
#define OP_CMD_BUSW(x)  (((x)&0x3) << 8)
#define OP_CMD_DTR(x)   (((x)&1) << 10)
#define OP_DD_RD_BIT    BIT(4)

/* Transfer Control Register */
#define TFR_CTRL              0x20
#define TFR_CTRL_DEV_DIS_BIT  BIT(18)
#define TFR_CTRL_IO_END_BIT   BIT(16)
#define TFR_CTRL_DEV_ACT_BIT  BIT(2)
#define TFR_CTRL_HC_ACT_BIT   BIT(1)
#define TFR_CTRL_IO_START_BIT BIT(0)

/* Present State Register */
#define PRES_STS          0x24
#define PRES_STS_RX_NEMPT BIT(18)
#define PRES_STS_TX_NFULL BIT(17)

/* SDMA Transfer Count Register */
#define SDMA_CNT             0x28
#define SDMA_CNT_TFR_BYTE(x) (((x)&0xFFFFFFFF) << 0)

/* SDMA System Address Register */
#define SDMA_ADDR   0x2C
#define SDMA_VAL(x) (((x)&0xFFFFFFFF) << 0)

/* Mapping Base Address Register */
#define BASE_MAP_ADDR          0x38
#define BASE_MAP_ADDR_VALUE(x) (((x)&0xFFFFFFFF) << 0)

/* Clock Control Register */
#define CLK_CTRL            0x4C
#define CLK_CTRL_RX_SS_B(x) (((x)&0x1F) << 21)
#define CLK_CTRL_RX_SS_A(x) (((x)&0x1F) << 16)

/* Capabilities Register */
#define CAP_1              0x58
#define CAP_1_CSB_NUM_MASK 0x1FF
#define CAP_1_CSB_NUM_OFS  0

/* Transmit Data 0~3 Register */
#define TXD_REG 0x70
#define TXD(x)  (TXD_REG + ((x)*4))

/* Receive Data Register */
#define RXD_REG      0x80
#define RXD_VALUE(x) (((x)&0xFFFFFFFF) << 0)

/* Device Present Status Register */
#define DEV_CTRL                 0xC0
#define DEV_CTRL_TYPE(x)         (((x)&0x7) << 29)
#define DEV_CTRL_TYPE_MASK       DEV_CTRL_TYPE(0x7)
#define DEV_CTRL_TYPE_SPI        DEV_CTRL_TYPE(0)
#define DEV_CTRL_SCLK_SEL(x)     (((x)&0xF) << 25)
#define DEV_CTRL_SCLK_SEL_MASK   DEV_CTRL_SCLK_SEL(0xF)
#define DEV_CTRL_SCLK_SEL_DIV(x) (((x >> 1) - 1) << 25)
#define DEV_CTRL_DQS_EN          BIT(5)

/* Mapping Read Control Register */
#define MAP_RD_CTRL              0xC4
#define MAP_RD_CTRL_PREAM_EN     BIT(28)
#define MAP_RD_CTRL_SIO_1X_RD(x) (((x)&0x3) << 6)

/* Linear/Mapping Write Control Register */
#define MAP_WR_CTRL 0xC8

/* Mapping Command Register */
#define MAP_CMD          0xCC
#define MAP_WR_CMD_SHIFT 16

/* Top Mapping Address Register */
#define TOP_MAP_ADDR          0xD0
#define TOP_MAP_ADDR_VALUE(x) (((x)&0xFFFFFFFF) << 0)

/* Sample Point Adjust Register */
#define SAMPLE_ADJ                    0xEC
#define SAMPLE_ADJ_DQS_IDLY_DOPI(x)   (((x)&0xff) << 27)
#define SAMPLE_ADJ_DQS_IDLY_DOPI_MASK SAMPLE_ADJ_DQS_IDLY_DOPI(0xff)
#define SAMPLE_ADJ_DQS_IDLY_SOPI(x)   (((x)&0xff) << 19)
#define SAMPLE_ADJ_DQS_ODLY(x)        (((x)&0xff) << 8)
#define SAMPLE_ADJ_POINT_SEL_DDR(x)   (((x)&0x7) << 3)
#define SAMPLE_ADJ_POINT_SEL_DDR_MASK SAMPLE_ADJ_POINT_SEL_DDR(0x7)
#define SAMPLE_ADJ_POINT_SEL_SDR(x)   (((x)&0x7) << 0)
#define SAMPLE_ADJ_POINT_SEL_SDR_MASK SAMPLE_ADJ_POINT_SEL_SDR(0x7)

/* SIO Input Delay 1 Register */
#define SIO_IDLY_1         0xF0
#define SIO_IDLY_1_SIO3(x) (((x)&0xff) << 24)
#define SIO_IDLY_1_SIO2(x) (((x)&0xff) << 16)
#define SIO_IDLY_1_SIO1(x) (((x)&0xff) << 8)
#define SIO_IDLY_1_SIO0(x) (((x)&0xff) << 0)
#define SIO_IDLY_1_0123(x)                                                                         \
	SIO_IDLY_1_SIO0(x) | SIO_IDLY_1_SIO1(x) | SIO_IDLY_1_SIO2(x) | SIO_IDLY_1_SIO3(x)

/* SIO Input Delay 2 Register */
#define SIO_IDLY_2          0xF4
#define SIO_IDLY_2_SIO4(x)  (((x)&0xff) << 24)
#define SIO_IDLY_2_SIO5(x)  (((x)&0xff) << 16)
#define SIO_IDLY_2_SIO6(x)  (((x)&0xff) << 8)
#define SIO_IDLY_2_SIO7(x)  (((x)&0xff) << 0)
#define IDLY_CODE_VAL(x, v) ((v) << (((x) % 4) * 8))
#define SIO_IDLY_2_4567(x)                                                                         \
	SIO_IDLY_2_SIO4(x) | SIO_IDLY_2_SIO5(x) | SIO_IDLY_2_SIO6(x) | SIO_IDLY_2_SIO7(x)

#define UEFC_BASE_ADDRESS      0x43a00000
#define UEFC_BASE_MAP_ADDR     0x60000000
#define UEFC_MAP_SIZE          0x00800000
#define UEFC_TOP_MAP_ADDR      (UEFC_BASE_MAP_ADDR + UEFC_MAP_SIZE)
#define UEFC_BASE_EXT_DDR_ADDR 0x00000000
#define DIR_IN                 0
#define DIR_OUT                1

#define BASE_ADDR(dev) (mm_reg_t) DEVICE_MMIO_GET(dev)

static uint32_t reg_read(const struct device *dev, uint32_t off)
{
	return sys_read32(BASE_ADDR(dev) + off);
}
static void reg_write(uint32_t data, const struct device *dev, uint32_t off)
{
	sys_write32(data, BASE_ADDR(dev) + off);
}
static void reg_update(const struct device *dev, uint32_t _mask, uint32_t data, uint32_t off)
{
	sys_write32(((data) | (sys_read32(BASE_ADDR(dev) + off) & ~(_mask))),
		    (BASE_ADDR(dev) + off));
}

#define DEFINE_MM_REG_RD(reg, off)                                                                 \
	static inline uint32_t read_##reg(const struct device *dev)                                \
	{                                                                                          \
		return reg_read(dev, off);                                                         \
	}

#define DEFINE_MM_REG_WR(reg, off)                                                                 \
	static inline void write_##reg(const struct device *dev, uint32_t data)                    \
	{                                                                                          \
		reg_write(data, dev, off);                                                         \
	}

#define DEFINE_MM_REG_UPDATE(reg, off)                                                             \
	static inline void update_##reg(const struct device *dev, uint32_t _mask, uint32_t data)   \
	{                                                                                          \
		reg_update(dev, _mask, data, off);                                                 \
	}

/* Default selection: Channel A, lun 0, Port 0 */
#define UEFC_CH_LUN_PORT HC_CTRL_CH_LUN_PORT(A, 0, 0)

#define MXICY_RD32(_reg) (*(volatile uint32_t *)(_reg))

#define MXICY_WR32(_val, _reg) ((*(uint32_t *)((_reg))) = (_val))

#define UPDATE_WRITE(_mask, _value, _reg)                                                          \
	MXICY_WR32(((_value) | (MXICY_RD32(_reg) & ~(_mask))), (_reg))

#define MSPI_MAX_FREQ                 48000000
#define MSPI_MAX_DEVICE               2
#define MSPI_TIMEOUT_US               10000
#define MSPI_DATA_PATTERN             0xffffffff
#define MSPI_LINES_TO_BUSWIDTH(lines) ((lines) == 1 ? 0 : (lines) == 2 ? 1 : (lines) == 4 ? 2 : 3)
#define MSPI_2BYTE_CMD                2
#define MSPI_4BYTE_ADDR               4
#define PWRCTRL_MAX_WAIT_US           5
#define MSPI_BUSY                     BIT(2)
#define CE_PORTS_MAX_LEN              16

struct mspi_mxicy_timing_cfg {
	uint8_t ui8SioShifter;
	uint8_t ui8DQSDdrDelay;
	uint8_t ui8DdrDelay;
	uint8_t ui8SdrDelay;
	uint32_t ui32SioLowDelay;
	uint32_t ui32SioHighDelay;
};

enum mspi_mxicy_timing_param {
	MSPI_MXICY_SET_SIO_SHIFTER = BIT(0),
	MSPI_MXICY_SET_DQS_DDR_DELAY = BIT(1),
	MSPI_MXICY_SET_DDR_DELAY = BIT(2),
	MSPI_MXICY_SET_SDR_DELAY = BIT(3),
	MSPI_MXICY_SET_SIO_LOW_DELAY = BIT(4),
	MSPI_MXICY_SET_SIO_HIGH_DELAY = BIT(5),
};
