/*
 * Copyright (c) 2017-2018 Antmicro Ltd <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SIFIVE_I2C_H
#define _SIFIVE_I2C_H

/* Register offsets */

#define SPI_REG_SCKDIV          0x00
#define SPI_REG_SCKMODE         0x04
#define SPI_REG_CSID            0x10
#define SPI_REG_CSDEF           0x14
#define SPI_REG_CSMODE          0x18

#define SPI_REG_DCSSCK          0x28
#define SPI_REG_DSCKCS          0x2a
#define SPI_REG_DINTERCS        0x2c
#define SPI_REG_DINTERXFR       0x2e

#define SPI_REG_FMT             0x40
#define SPI_REG_TXFIFO          0x48
#define SPI_REG_RXFIFO          0x4c
#define SPI_REG_TXCTRL          0x50
#define SPI_REG_RXCTRL          0x54

#define SPI_REG_FCTRL           0x60
#define SPI_REG_FFMT            0x64

#define SPI_REG_IE              0x70
#define SPI_REG_IP              0x74

/* Fields */

#define SPI_SCK_POL             0x1
#define SPI_SCK_PHA             0x2

#define SPI_FMT_PROTO(x)        ((x) & 0x3)
#define SPI_FMT_ENDIAN(x)       (((x) & 0x1) << 2)
#define SPI_FMT_DIR(x)          (((x) & 0x1) << 3)
#define SPI_FMT_LEN(x)          (((x) & 0xf) << 16)

/* TXCTRL register */
#define SPI_TXWM(x)             ((x) & 0xffff)
/* RXCTRL register */
#define SPI_RXWM(x)             ((x) & 0xffff)

#define SPI_IP_TXWM             0x1
#define SPI_IP_RXWM             0x2

#define SPI_FCTRL_EN            0x1

#define SPI_INSN_CMD_EN         0x1
#define SPI_INSN_ADDR_LEN(x)    (((x) & 0x7) << 1)
#define SPI_INSN_PAD_CNT(x)     (((x) & 0xf) << 4)
#define SPI_INSN_CMD_PROTO(x)   (((x) & 0x3) << 8)
#define SPI_INSN_ADDR_PROTO(x)  (((x) & 0x3) << 10)
#define SPI_INSN_DATA_PROTO(x)  (((x) & 0x3) << 12)
#define SPI_INSN_CMD_CODE(x)    (((x) & 0xff) << 16)
#define SPI_INSN_PAD_CODE(x)    (((x) & 0xff) << 24)

#define SPI_TXFIFO_FULL  (1 << 31)
#define SPI_RXFIFO_EMPTY (1 << 31)

/* Values */

#define SPI_CSMODE_AUTO         0
#define SPI_CSMODE_HOLD         2
#define SPI_CSMODE_OFF          3

#define SPI_DIR_RX              0
#define SPI_DIR_TX              1

#define SPI_PROTO_S             0
#define SPI_PROTO_D             1
#define SPI_PROTO_Q             2

#define SPI_ENDIAN_MSB          0
#define SPI_ENDIAN_LSB          1

#define SPI1_CSID_SS0		0
#define SPI1_CSID_SS2		2
#define SPI1_CSID_SS3		3

/* IOF masks */
#define SPI1_CTRL_ADDR		0x10024000
#define IOF0_SPI1_MASK		0x000007FC
#define GPIO_CTRL_ADDR		0x10012000
#define SPI11_NUM_SS		(4)
#define IOF_SPI1_SS0		(2u)
#define IOF_SPI1_SS1		(8u)
#define IOF_SPI1_SS2		(9u)
#define IOF_SPI1_SS3		(10u)
#define IOF_SPI1_MOSI		(3u)
#define IOF_SPI1_MISO		(4u)
#define IOF_SPI1_SCK		(5u)
#define IOF_SPI1_DQ0		(3u)
#define IOF_SPI1_DQ1		(4u)
#define IOF_SPI1_DQ2		(6u)
#define IOF_SPI1_DQ3		(7u)

/* gpio defines */
#define GPIO_INPUT_VAL		(0x00)
#define GPIO_INPUT_EN		(0x04)
#define GPIO_OUTPUT_EN		(0x08)
#define GPIO_OUTPUT_VAL		(0x0C)
#define GPIO_PULLUP_EN		(0x10)
#define GPIO_DRIVE		(0x14)
#define GPIO_RISE_IE		(0x18)
#define GPIO_RISE_IP		(0x1C)
#define GPIO_FALL_IE		(0x20)
#define GPIO_FALL_IP		(0x24)
#define GPIO_HIGH_IE		(0x28)
#define GPIO_HIGH_IP		(0x2C)
#define GPIO_LOW_IE		(0x30)
#define GPIO_LOW_IP		(0x34)
#define GPIO_IOF_EN		(0x38)
#define GPIO_IOF_SEL		(0x3C)
#define GPIO_OUTPUT_XOR		(0x40)


#define INT_SPI1_BASE 6
#define _REG32(p, i) (*(volatile uint32_t *) ((p) + (i)))
#define _REG32P(p, i) ((volatile uint32_t *) ((p) + (i)))

#define SPI1_REG(offset) _REG32(SPI1_CTRL_ADDR, offset)
#define GPIO_REG(offset) _REG32(GPIO_CTRL_ADDR, offset)
#endif /* _SIFIVE_SPI_H */
