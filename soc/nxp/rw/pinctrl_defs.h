/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_NXP_IMX_RW6XX_PINCTRL_DEFS_H_
#define ZEPHYR_SOC_ARM_NXP_IMX_RW6XX_PINCTRL_DEFS_H_

/* Internal macros to pack and extract pin configuration data. */
/* GPIO configuration packing macros */
#define IOMUX_OFFSET_ENABLE(offset, enable, shift) \
	((((offset) << 1) | (enable & 0x1)) << shift)
#define IOMUX_SCTIMER_OUT_CLR(offset, enable) \
	IOMUX_OFFSET_ENABLE(offset, enable, 0)
#define IOMUX_SCTIMER_IN_CLR(offset, enable) \
	IOMUX_OFFSET_ENABLE(offset, enable, 4)
#define IOMUX_CTIMER_CLR(offset, enable)\
	IOMUX_OFFSET_ENABLE(offset, enable, 8)
#define IOMUX_FSEL_CLR(mask) ((mask) << 13)
#define IOMUX_FLEXCOMM_CLR(idx, mask) \
	(((mask) << 45) | ((idx) << 56))

/* GPIO configuration extraction macros */
#define IOMUX_GET_SCTIMER_OUT_CLR_ENABLE(mux) ((mux) & 0x1)
#define IOMUX_GET_SCTIMER_OUT_CLR_OFFSET(mux) (((mux) >> 1) & 0x7)
#define IOMUX_GET_SCTIMER_IN_CLR_ENABLE(mux) (((mux) >> 4) & 0x1)
#define IOMUX_GET_SCTIMER_IN_CLR_OFFSET(mux) (((mux) >> 5) & 0x7)
#define IOMUX_GET_CTIMER_CLR_ENABLE(mux) (((mux) >> 8) & 0x1ULL)
#define IOMUX_GET_CTIMER_CLR_OFFSET(mux) (((mux) >> 9) & 0xFULL)
#define IOMUX_GET_FSEL_CLR_MASK(mux) (((mux) >> 13) & 0xFFFFFFFFULL)
#define IOMUX_GET_FLEXCOMM_CLR_MASK(mux) \
	(((mux) >> 45) & 0x7FFULL)
#define IOMUX_GET_FLEXCOMM_CLR_IDX(mux) \
	(((mux) >> 56) & 0xF)

/* Pin mux type and gpio offset macros */
#define IOMUX_GPIO_IDX(x) ((x) & 0x7F)
#define IOMUX_TYPE(x) (((x) & 0xF) << 7)
#define IOMUX_GET_GPIO_IDX(mux) ((mux) & 0x7F)
#define IOMUX_GET_TYPE(mux) (((mux) >> 7) & 0xF)

/* Flexcomm specific macros */
#define IOMUX_FLEXCOMM_IDX(x) (((x) & 0xF) << 11)
#define IOMUX_FLEXCOMM_BIT(x) (((x) & 0xF) << 15)
#define IOMUX_GET_FLEXCOMM_IDX(mux) (((mux) >> 11) & 0xF)
#define IOMUX_GET_FLEXCOMM_BIT(mux) (((mux) >> 15) & 0xF)

/* Function select specific macros */
#define IOMUX_FSEL_BIT(mux) (((mux) & 0x1F) << 11)
#define IOMUX_GET_FSEL_BIT(mux) (((mux) >> 11) & 0x1F)

/* CTimer specific macros */
#define IOMUX_CTIMER_BIT(x) (((x) & 0xF) << 11)
#define IOMUX_GET_CTIMER_BIT(mux) (((mux) >> 11) & 0xF)

/* SCtimer specific macros */
#define IOMUX_SCTIMER_BIT(x) (((x) & 0xF) << 11)
#define IOMUX_GET_SCTIMER_BIT(mux) (((mux) >> 11) & 0xF)


/* Mux Types */
#define IOMUX_FLEXCOMM 0x0
#define IOMUX_FSEL 0x1
#define IOMUX_CTIMER_IN 0x2
#define IOMUX_CTIMER_OUT 0x3
#define IOMUX_SCTIMER_IN 0x4
#define IOMUX_SCTIMER_OUT 0x5
#define IOMUX_GPIO 0x6
#define IOMUX_SGPIO 0x7
#define IOMUX_AON 0x8


/* Pin configuration settings */
#define IOMUX_PAD_PULL(x) (((x) & 0x3) << 19)
#define IOMUX_PAD_SLEW(x) (((x) & 0x3) << 21)
#define IOMUX_PAD_SLEEP_FORCE(en) (((en) & 0x3) << 23)
#define IOMUX_PAD_GET_PULL(mux) (((mux) >> 19) & 0x3)
#define IOMUX_PAD_GET_SLEW(mux) (((mux) >> 21) & 0x3)
#define IOMUX_PAD_GET_SLEEP_FORCE(mux) (((mux) >> 23) & 0x3)
#define IOMUX_PAD_GET_SLEEP_FORCE_VAL(mux) (((mux) >> 23) & 0x1)

/*
 * GPIO mux options. These options are used to clear all alternate
 * pin functions, so the pin controller will use GPIO mode.
 */

#define IOMUX_GPIO_CLR_0 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x418ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x0ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 1ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_1 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x0ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(1ULL, 1ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_2 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x32eULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x0ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_3 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x22eULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x0ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 1ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 1ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_4 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x2dULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x800000ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(1ULL, 1ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(1ULL, 1ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_5 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x430ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x0ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_6 \
	(IOMUX_FLEXCOMM_CLR(0x1ULL, 0x418ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x1000000ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_7 \
	(IOMUX_FLEXCOMM_CLR(0x1ULL, 0xedULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x0ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_8 \
	(IOMUX_FLEXCOMM_CLR(0x1ULL, 0x2eeULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x0ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_9 \
	(IOMUX_FLEXCOMM_CLR(0x1ULL, 0x3eeULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x0ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_10 \
	(IOMUX_FLEXCOMM_CLR(0x1ULL, 0x430ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x0ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_11 \
	(IOMUX_FLEXCOMM_CLR(0x1ULL, 0x40ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x0ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(8ULL, 1ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(8ULL, 1ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_12 \
	(IOMUX_FLEXCOMM_CLR(0x1ULL, 0x80ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x8020ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(2ULL, 1ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_13 \
	(IOMUX_FLEXCOMM_CLR(0x2ULL, 0x3eeULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x0ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(3ULL, 1ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_14 \
	(IOMUX_FLEXCOMM_CLR(0x2ULL, 0x2eeULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x0ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(4ULL, 1ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_15 \
	(IOMUX_FLEXCOMM_CLR(0x2ULL, 0xedULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x8600ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_16 \
	(IOMUX_FLEXCOMM_CLR(0x2ULL, 0x418ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x8600ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_17 \
	(IOMUX_FLEXCOMM_CLR(0x2ULL, 0x430ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x8600ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_18 \
	(IOMUX_FLEXCOMM_CLR(0x2ULL, 0x80ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0xc600ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_19 \
	(IOMUX_FLEXCOMM_CLR(0x3ULL, 0x430ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x8000ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_20 \
	(IOMUX_FLEXCOMM_CLR(0x3ULL, 0x418ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x8000ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_21 \
	(IOMUX_FLEXCOMM_CLR(0x2ULL, 0x40ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x0ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(5ULL, 1ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_22 \
	(IOMUX_FLEXCOMM_CLR(0x3ULL, 0x40ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x4000000ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_23 \
	(IOMUX_FLEXCOMM_CLR(0x3ULL, 0x80ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x4000000ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_24 \
	(IOMUX_FLEXCOMM_CLR(0x3ULL, 0x3eeULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x40000000ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(6ULL, 1ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_25 \
	(IOMUX_FLEXCOMM_CLR(0x3ULL, 0xedULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x10000ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(7ULL, 1ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_26 \
	(IOMUX_FLEXCOMM_CLR(0x3ULL, 0x2eeULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x80000000ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(4ULL, 1ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(4ULL, 1ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_27 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x10000000ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(5ULL, 1ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(5ULL, 1ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_28 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_29 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_30 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_31 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_32 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_33 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_34 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_35 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x8ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(6ULL, 1ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(6ULL, 1ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_36 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x8ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(7ULL, 1ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(7ULL, 1ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_37 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x8ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(8ULL, 1ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_38 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x8ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(9ULL, 1ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_39 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x8ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(10ULL, 1ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_40 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x8ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_41 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x8ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_42 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x800ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_43 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x800ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_44 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x1800ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_45 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x1800ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_46 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x1800ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_47 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x1800ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_48 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x1800ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_49 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x1800ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_50 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x22000ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_51 \
	(IOMUX_FLEXCOMM_CLR(0x6ULL, 0x40ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x40810ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(11ULL, 1ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_52 \
	(IOMUX_FLEXCOMM_CLR(0x6ULL, 0x80ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x80810ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(12ULL, 1ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_53 \
	(IOMUX_FLEXCOMM_CLR(0x6ULL, 0x418ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x100810ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(13ULL, 1ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_54 \
	(IOMUX_FLEXCOMM_CLR(0x6ULL, 0xedULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x200810ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(14ULL, 1ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_55 \
	(IOMUX_FLEXCOMM_CLR(0x6ULL, 0x430ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x400000ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(9ULL, 1ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(9ULL, 1ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_56 \
	(IOMUX_FLEXCOMM_CLR(0x6ULL, 0x2eeULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x8000800ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_57 \
	(IOMUX_FLEXCOMM_CLR(0x6ULL, 0x3eeULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x8000800ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_58 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x2000000ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_59 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x2000000ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_60 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x2000000ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_61 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x20000000ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_62 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x4000000ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_CLR_63 \
	(IOMUX_FLEXCOMM_CLR(0x0ULL, 0x0ULL) | /* Flexcomm bits to clear */ \
	IOMUX_FSEL_CLR(0x4000000ULL) | /* FSEL bits to clear */ \
	IOMUX_CTIMER_CLR(0ULL, 0ULL) | /* CTIMER offset to clear */ \
	IOMUX_SCTIMER_IN_CLR(0ULL, 0ULL) | /* SCTIMER input offset to clear */ \
	IOMUX_SCTIMER_OUT_CLR(0ULL, 0ULL)) /* SCTIMER output offset to clear */

#define IOMUX_GPIO_OPS \
	IOMUX_GPIO_CLR_0, IOMUX_GPIO_CLR_1, IOMUX_GPIO_CLR_2, IOMUX_GPIO_CLR_3, \
	IOMUX_GPIO_CLR_4, IOMUX_GPIO_CLR_5, IOMUX_GPIO_CLR_6, IOMUX_GPIO_CLR_7, \
	IOMUX_GPIO_CLR_8, IOMUX_GPIO_CLR_9, IOMUX_GPIO_CLR_10, IOMUX_GPIO_CLR_11, \
	IOMUX_GPIO_CLR_12, IOMUX_GPIO_CLR_13, IOMUX_GPIO_CLR_14, IOMUX_GPIO_CLR_15, \
	IOMUX_GPIO_CLR_16, IOMUX_GPIO_CLR_17, IOMUX_GPIO_CLR_18, IOMUX_GPIO_CLR_19, \
	IOMUX_GPIO_CLR_20, IOMUX_GPIO_CLR_21, IOMUX_GPIO_CLR_22, IOMUX_GPIO_CLR_23, \
	IOMUX_GPIO_CLR_24, IOMUX_GPIO_CLR_25, IOMUX_GPIO_CLR_26, IOMUX_GPIO_CLR_27, \
	IOMUX_GPIO_CLR_28, IOMUX_GPIO_CLR_29, IOMUX_GPIO_CLR_30, IOMUX_GPIO_CLR_31, \
	IOMUX_GPIO_CLR_32, IOMUX_GPIO_CLR_33, IOMUX_GPIO_CLR_34, IOMUX_GPIO_CLR_35, \
	IOMUX_GPIO_CLR_36, IOMUX_GPIO_CLR_37, IOMUX_GPIO_CLR_38, IOMUX_GPIO_CLR_39, \
	IOMUX_GPIO_CLR_40, IOMUX_GPIO_CLR_41, IOMUX_GPIO_CLR_42, IOMUX_GPIO_CLR_43, \
	IOMUX_GPIO_CLR_44, IOMUX_GPIO_CLR_45, IOMUX_GPIO_CLR_46, IOMUX_GPIO_CLR_47, \
	IOMUX_GPIO_CLR_48, IOMUX_GPIO_CLR_49, IOMUX_GPIO_CLR_50, IOMUX_GPIO_CLR_51, \
	IOMUX_GPIO_CLR_52, IOMUX_GPIO_CLR_53, IOMUX_GPIO_CLR_54, IOMUX_GPIO_CLR_55, \
	IOMUX_GPIO_CLR_56, IOMUX_GPIO_CLR_57, IOMUX_GPIO_CLR_58, IOMUX_GPIO_CLR_59, \
	IOMUX_GPIO_CLR_60, IOMUX_GPIO_CLR_61, IOMUX_GPIO_CLR_62, IOMUX_GPIO_CLR_63

#endif /* ZEPHYR_SOC_ARM_NXP_IMX_RW6XX_PINCTRL_DEFS_H_ */
