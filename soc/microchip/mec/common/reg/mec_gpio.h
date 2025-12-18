/*
 * Copyright (c) 2022 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_GPIO_H
#define _MEC_GPIO_H

#include <stdint.h>
#include <stddef.h>
#include <zephyr/sys/util.h>

#define MEC_GPIO_MAX_PORTS     6
#define MEC_GPIO_PINS_PER_PORT 32

/* GPIO Control register 1 */
#define MEC_GPIO_CR1_PUD_POS      0
#define MEC_GPIO_CR1_PUD_MSK      GENMASK(1, 0)
#define MEC_GPIO_CR1_PUD_NONE     0
#define MEC_GPIO_CR1_PUD_PU       1u
#define MEC_GPIO_CR1_PUD_PD       2u
#define MEC_GPIO_CR1_PUD_RPT      3u
#define MEC_GPIO_CR1_PUD_SET(pud) FIELD_PREP(MEC_GPIO_CR1_PUD_MSK, (pud))
#define MEC_GPIO_CR1_PUD_GET(r)   FIELD_GET(MEC_GPIO_CR1_PUD_MSK, (r))

#define MEC_GPIO_CR1_PG_POS     2
#define MEC_GPIO_CR1_PG_MSK     GENMASK(3, 2)
#define MEC_GPIO_CR1_PG_VTR     0
#define MEC_GPIO_CR1_PG_VCC     1u
#define MEC_GPIO_CR1_PG_UNPWRD  2u
#define MEC_GPIO_CR1_PG_SET(pg) FIELD_PREP(MEC_GPIO_CR1_PG_MSK, (pg))
#define MEC_GPIO_CR1_PG_GET(r)  FIELD_GET(MEC_GPIO_CR1_PG_MSK, (r))

#define MEC_GPIO_CR1_IDET_POS       4
#define MEC_GPIO_CR1_IDET_MSK       GENMASK(7, 4)
#define MEC_GPIO_CR1_IDET_LL        0
#define MEC_GPIO_CR1_IDET_LH        1u
#define MEC_GPIO_CR1_IDET_DIS       4u
#define MEC_GPIO_CR1_IDET_RE        0xdu
#define MEC_GPIO_CR1_IDET_FE        0xeu
#define MEC_GPIO_CR1_IDET_BE        0xfu
#define MEC_GPIO_CR1_IDET_SET(idet) FIELD_PREP(MEC_GPIO_CR1_IDET_MSK, (idet))
#define MEC_GPIO_CR1_IDET_GET(r)    FIELD_GET(MEC_GPIO_CR1_IDET_MSK, (r))

#define MEC_GPIO_CR1_OBUF_POS     8
#define MEC_GPIO_CR1_OBUF_MSK     GENMASK(8, 8)
#define MEC_GPIO_CR1_OBUF_PP      0
#define MEC_GPIO_CR1_OBUF_OD      1u
#define MEC_GPIO_CR1_OBUF_SET(ob) FIELD_PREP(MEC_GPIO_CR1_OBUF_MSK, (ob))
#define MEC_GPIO_CR1_OBUF_GET(r)  FIELD_GET(MEC_GPIO_CR1_OBUF_MSK, (r))

#define MEC_GPIO_CR1_DIR_POS    9
#define MEC_GPIO_CR1_DIR_MSK    GENMASK(9, 9)
#define MEC_GPIO_CR1_DIR_IN     0
#define MEC_GPIO_CR1_DIR_OUT    1u
#define MEC_GPIO_CR1_DIR_SET(d) FIELD_PREP(MEC_GPIO_CR1_DIR_MSK, (d))
#define MEC_GPIO_CR1_DIR_GET(r) FIELD_GET(MEC_GPIO_CR1_DIR_MSK, (r))

#define MEC_GPIO_CR1_OCR_POS      10
#define MEC_GPIO_CR1_OCR_MSK      GENMASK(10, 10)
#define MEC_GPIO_CR1_OCR_CR       0
#define MEC_GPIO_CR1_OCR_PR       1u
#define MEC_GPIO_CR1_OCR_SET(ocr) FIELD_PREP(MEC_GPIO_CR1_OCR_MSK, (ocr))
#define MEC_GPIO_CR1_OCR_GET(r)   FIELD_GET(MEC_GPIO_CR1_OCR_MSK, (r))

#define MEC_GPIO_CR1_FPOL_POS    11
#define MEC_GPIO_CR1_FPOL_MSK    GENMASK(11, 11)
#define MEC_GPIO_CR1_FPOL_NORM   0
#define MEC_GPIO_CR1_FPOL_INV    1u
#define MEC_GPIO_CR1_FPOL_SET(p) FIELD_PREP(MEC_GPIO_CR1_FPOL_MSK, (p))
#define MEC_GPIO_CR1_FPOL_GET(r) FIELD_GET(MEC_GPIO_CR1_FPOL_MSK, (r))

#define MEC_GPIO_CR1_MUX_POS    12
#define MEC_GPIO_CR1_MUX_MSK    GENMASK(14, 12)
#define MEC_GPIO_CR1_MUX_GPIO   0
#define MEC_GPIO_CR1_MUX_F1     1u
#define MEC_GPIO_CR1_MUX_F2     2u
#define MEC_GPIO_CR1_MUX_F3     3u
#define MEC_GPIO_CR1_MUX_F4     4u
#define MEC_GPIO_CR1_MUX_F5     5u
#define MEC_GPIO_CR1_MUX_F6     6u
#define MEC_GPIO_CR1_MUX_F7     7u
#define MEC_GPIO_CR1_MUX_SET(m) FIELD_PREP(MEC_GPIO_CR1_MUX_MSK, (m))
#define MEC_GPIO_CR1_MUX_GET(r) FIELD_GET(MEC_GPIO_CR1_MUX_MSK, (r))

#define MEC_GPIO_CR1_INPD_POS      15
#define MEC_GPIO_CR1_INPD_MSK      GENMASK(15, 15)
#define MEC_GPIO_CR1_INPD_EN       0
#define MEC_GPIO_CR1_INPD_DIS      1u
#define MEC_GPIO_CR1_INPD_SET(inp) FIELD_PREP(MEC_GPIO_CR1_INPD_MSK, (inp))
#define MEC_GPIO_CR1_INPD_GET(r)   FIELD_GET(MEC_GPIO_CR1_INPD_MSK, (r))

#define MEC_GPIO_CR1_ODAT_POS     16
#define MEC_GPIO_CR1_ODAT_MSK     GENMASK(16, 16)
#define MEC_GPIO_CR1_ODAT_LO      0
#define MEC_GPIO_CR1_ODAT_HI      1u
#define MEC_GPIO_CR1_ODAT_SET(od) FIELD_PREP(MEC_GPIO_CR1_ODAT_MSK, (od))
#define MEC_GPIO_CR1_ODAT_GET(r)  FIELD_GET(MEC_GPIO_CR1_ODAT_MSK, (r))

#define MEC_GPIO_CR1_INPV_POS       24
#define MEC_GPIO_CR1_INPV_MSK       GENMASK(24, 24)
#define MEC_GPIO_CR1_INPV_LO        0
#define MEC_GPIO_CR1_INPV_HI        1u
#define MEC_GPIO_CR1_INPV_SET(inpv) FIELD_PREP(MEC_GPIO_CR1_INPV_MSK, (inpv))
#define MEC_GPIO_CR1_INPV_GET(r)    FIELD_GET(MEC_GPIO_CR1_INVPV_MSK, (r))

/* GPIO Control register 2 */
#define MEC_GPIO_CR2_SLEW_POS    0
#define MEC_GPIO_CR2_SLEW_MSK    GENMASK(0, 0)
#define MEC_GPIO_CR2_SLEW_FAST   0
#define MEC_GPIO_CR2_SLEW_SLOW   1u
#define MEC_GPIO_CR2_SLEW_SET(s) FIELD_PREP(MEC_GPIO_CR2_SLEW_MSK, (s))
#define MEC_GPIO_CR2_SLEW_GET(r) FIELD_GET(MEC_GPIO_CR2_SLEW_MSK, (r))

#define MEC_GPIO_CR2_DSTR_POS     4
#define MEC_GPIO_CR2_DSTR_MSK     GENMASK(5, 4)
#define MEC_GPIO_CR2_DSTR_1X      0
#define MEC_GPIO_CR2_DSTR_2X      1u
#define MEC_GPIO_CR2_DSTR_4X      2u
#define MEC_GPIO_CR2_DSTR_6X      3u
#define MEC_GPIO_CR2_DSTR_SET(ds) FIELD_PREP(MEC_GPIO_CR2_DSTR_MSK, (ds))
#define MEC_GPIO_CR2_DSTR_GET(r)  FIELD_GET(MEC_GPIO_CR2_DSTR_MSK, (r))

#define MEC_GPIO_CR1_OFS 0
#define MEC_GPIO_CR2_OFS 0x500u

/* gpio_base is the base address of the GPIO peripheral block */
#define MEC_GPIO_CR1_ADDR(gpio_base, pin)                                                          \
	(((uint32_t)(gpio_base) + MEC_GPIO_CR1_OFS) + ((uint32_t)(pin) * 4u))

#define MEC_GPIO_CR2_ADDR(gpio_base, pin)                                                          \
	(((uint32_t)(gpio_base) + MEC_GPIO_CR2_OFS) + ((uint32_t)(pin) * 4u))

#define MCHP_XEC_PINCTRL_REG_IDX(pin) ((pin >> 5) * 32 + (pin & 0x1f))

/** @brief All GPIO register as arrays of registers */
struct gpio_regs {
	volatile uint32_t CTRL[174];
	uint32_t RESERVED[18];
	volatile uint32_t PARIN[6];
	uint32_t RESERVED1[26];
	volatile uint32_t PAROUT[6];
	uint32_t RESERVED2[20];
	volatile uint32_t LOCK[6];
	uint32_t RESERVED3[64];
	volatile uint32_t CTRL2[174];
};

#endif /* #ifndef _MEC_GPIO_H */
