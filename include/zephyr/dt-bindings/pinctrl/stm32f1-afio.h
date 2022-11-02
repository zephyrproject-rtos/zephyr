/*
 * Copyright (c) 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_STM32_AFIO_H_
#define ZEPHYR_STM32_AFIO_H_

#define STM32_REMAP_REG_MASK    0x1U
#define STM32_REMAP_REG_SHIFT   0U
#define STM32_REMAP_SHIFT_MASK  0x1FU
#define STM32_REMAP_SHIFT_SHIFT 1U
#define STM32_REMAP_MASK_MASK   0x3U
#define STM32_REMAP_MASK_SHIFT  6U
#define STM32_REMAP_VAL_MASK    0x3U
#define STM32_REMAP_VAL_SHIFT   8U

/**
 * @brief STM32F1 Remap configuration bit field.
 *
 * - reg   (0/1)      [ 0 : 0 ]
 * - shift (0..31)    [ 1 : 5 ]
 * - mask  (0x1, 0x3) [ 6 : 7 ]
 * - val   (0..3)     [ 8 : 9 ]
 *
 * @param reg AFIO_MAPRx register (MAPR, MAPR2).
 * @param shift Position within AFIO_MAPRx.
 * @param mask Mask for the AFIO_MAPRx field.
 * @param val Remap value (0, 1, 2 or 3).
 */
#define STM32_REMAP(val, mask, shift, reg)				       \
	((((reg) & STM32_REMAP_REG_MASK) << STM32_REMAP_REG_SHIFT) |	       \
	 (((shift) & STM32_REMAP_SHIFT_MASK) << STM32_REMAP_SHIFT_SHIFT) |     \
	 (((mask) & STM32_REMAP_MASK_MASK) << STM32_REMAP_MASK_SHIFT) |	       \
	 (((val) & STM32_REMAP_VAL_MASK) << STM32_REMAP_VAL_SHIFT))


/* Accessors for remap value */

/**
 * Obtain register field from remap configuration.
 *
 * @param remap Remap bit field value.
 */
#define STM32_REMAP_REG_GET(remap) \
	(((remap) >> STM32_REMAP_REG_SHIFT) & STM32_REMAP_REG_MASK)

/**
 * Obtain position field from remap configuration.
 *
 * @param remap Remap bit field value.
 */
#define STM32_REMAP_SHIFT_GET(remap) \
	(((remap) >> STM32_REMAP_SHIFT_SHIFT) & STM32_REMAP_SHIFT_MASK)

/**
 * Obtain mask field from remap configuration.
 *
 * @param remap Remap bit field value.
 */
#define STM32_REMAP_MASK_GET(remap) \
	(((remap) >> STM32_REMAP_MASK_SHIFT) & STM32_REMAP_MASK_MASK)

/**
 * Obtain value field from remap configuration.
 *
 * @param remap Remap bit field value.
 */
#define STM32_REMAP_VAL_GET(remap) \
	(((remap) >> STM32_REMAP_VAL_SHIFT) & STM32_REMAP_VAL_MASK)


/* Remap values definitions, according to RM0008.pdf */

#define STM32_AFIO_MAPR		0U
#define STM32_AFIO_MAPR2	1U

/** Device not remappable **/
#define NO_REMAP		0

/** SPI1 (no remap) */
#define SPI1_REMAP0		STM32_REMAP(0U, 0x1U, 0U, STM32_AFIO_MAPR)
/** SPI1 (remap) */
#define SPI1_REMAP1		STM32_REMAP(1U, 0x1U, 0U, STM32_AFIO_MAPR)

/** I2C1 (no remap) */
#define I2C1_REMAP0		STM32_REMAP(0U, 0x1U, 1U, STM32_AFIO_MAPR)
/** I2C1 (remap) */
#define I2C1_REMAP1		STM32_REMAP(1U, 0x1U, 1U, STM32_AFIO_MAPR)

/** USART1 (no remap) */
#define USART1_REMAP0		STM32_REMAP(0U, 0x1U, 2U, STM32_AFIO_MAPR)
/** USART1 (remap) */
#define USART1_REMAP1		STM32_REMAP(1U, 0x1U, 2U, STM32_AFIO_MAPR)

/** USART2 (no remap) */
#define USART2_REMAP0		STM32_REMAP(0U, 0x1U, 3U, STM32_AFIO_MAPR)
/** USART2 (remap) */
#define USART2_REMAP1		STM32_REMAP(1U, 0x1U, 3U, STM32_AFIO_MAPR)

/** USART3 (no remap) */
#define USART3_REMAP0		STM32_REMAP(0U, 0x3U, 4U, STM32_AFIO_MAPR)
/** USART3 (partial remap) */
#define USART3_REMAP1		STM32_REMAP(1U, 0x3U, 4U, STM32_AFIO_MAPR)
/** USART3 (full remap) */
#define USART3_REMAP2		STM32_REMAP(3U, 0x3U, 4U, STM32_AFIO_MAPR)

/** TIM1 (no remap) */
#define TIM1_REMAP0		STM32_REMAP(0U, 0x3U, 6U, STM32_AFIO_MAPR)
/** TIM1 (partial remap) */
#define TIM1_REMAP1		STM32_REMAP(1U, 0x3U, 6U, STM32_AFIO_MAPR)
/** TIM1 (full remap) */
#define TIM1_REMAP2		STM32_REMAP(3U, 0x3U, 6U, STM32_AFIO_MAPR)

/** TIM2 (no remap) */
#define TIM2_REMAP0		STM32_REMAP(0U, 0x3U, 8U, STM32_AFIO_MAPR)
/** TIM2 (partial remap 1) */
#define TIM2_REMAP1		STM32_REMAP(1U, 0x3U, 8U, STM32_AFIO_MAPR)
/** TIM2 (partial remap 2) */
#define TIM2_REMAP2		STM32_REMAP(2U, 0x3U, 8U, STM32_AFIO_MAPR)
/** TIM2 (full remap) */
#define TIM2_REMAP3		STM32_REMAP(3U, 0x3U, 8U, STM32_AFIO_MAPR)

/** TIM3 (no remap) */
#define TIM3_REMAP0		STM32_REMAP(0U, 0x3U, 10U, STM32_AFIO_MAPR)
/** TIM3 (partial remap 1) */
#define TIM3_REMAP1		STM32_REMAP(1U, 0x3U, 10U, STM32_AFIO_MAPR)
/** TIM3 (partial remap 2) */
#define TIM3_REMAP2		STM32_REMAP(2U, 0x3U, 10U, STM32_AFIO_MAPR)
/** TIM3 (full remap) */
#define TIM3_REMAP3		STM32_REMAP(3U, 0x3U, 10U, STM32_AFIO_MAPR)

/** TIM4 (no remap) */
#define TIM4_REMAP0		STM32_REMAP(0U, 0x1U, 12U, STM32_AFIO_MAPR)
/** TIM4 (remap) */
#define TIM4_REMAP1		STM32_REMAP(1U, 0x1U, 12U, STM32_AFIO_MAPR)

/** CAN (no remap) */
#define CAN_REMAP0		STM32_REMAP(0U, 0x3U, 13U, STM32_AFIO_MAPR)
/** CAN (partial remap) */
#define CAN_REMAP1		STM32_REMAP(2U, 0x3U, 13U, STM32_AFIO_MAPR)
/** CAN (full remap) */
#define CAN_REMAP2		STM32_REMAP(3U, 0x3U, 13U, STM32_AFIO_MAPR)

/** CAN1 alias */
#define CAN1_REMAP0		CAN_REMAP0
#define CAN1_REMAP1		CAN_REMAP1
#define CAN1_REMAP2		CAN_REMAP2

/** ETH (no remap) */
#define ETH_REMAP0		STM32_REMAP(0U, 0x1U, 21U, STM32_AFIO_MAPR)
/** ETH (remap) */
#define ETH_REMAP1		STM32_REMAP(1U, 0x1U, 21U, STM32_AFIO_MAPR)

/** CAN2 (no remap) */
#define CAN2_REMAP0		STM32_REMAP(0U, 0x1U, 22U, STM32_AFIO_MAPR)
/** CAN2 (remap) */
#define CAN2_REMAP1		STM32_REMAP(1U, 0x1U, 22U, STM32_AFIO_MAPR)

/** SPI3 (no remap) */
#define SPI3_REMAP0		STM32_REMAP(0U, 0x1U, 28U, STM32_AFIO_MAPR)
/** SPI3 (remap) */
#define SPI3_REMAP1		STM32_REMAP(1U, 0x1U, 28U, STM32_AFIO_MAPR)

/** I2S3 (SPI3) (no remap) */
#define I2S3_REMAP0		SPI3_REMAP0
/** I2S3 (SPI3) (remap) */
#define I2S3_REMAP1		SPI3_REMAP1

/** TIM9 (no remap) */
#define TIM9_REMAP0		STM32_REMAP(0U, 0x1U, 5U, STM32_AFIO_MAPR2)
/** TIM9 (remap) */
#define TIM9_REMAP1		STM32_REMAP(1U, 0x1U, 5U, STM32_AFIO_MAPR2)

/** TIM10 (no remap) */
#define TIM10_REMAP0		STM32_REMAP(0U, 0x1U, 6U, STM32_AFIO_MAPR2)
/** TIM10 (remap) */
#define TIM10_REMAP1		STM32_REMAP(1U, 0x1U, 6U, STM32_AFIO_MAPR2)

/** TIM11 (no remap) */
#define TIM11_REMAP0		STM32_REMAP(0U, 0x1U, 7U, STM32_AFIO_MAPR2)
/** TIM11 (remap) */
#define TIM11_REMAP1		STM32_REMAP(1U, 0x1U, 7U, STM32_AFIO_MAPR2)

/** TIM13 (no remap) */
#define TIM13_REMAP0		STM32_REMAP(0U, 0x1U, 8U, STM32_AFIO_MAPR2)
/** TIM13 (remap) */
#define TIM13_REMAP1		STM32_REMAP(1U, 0x1U, 8U, STM32_AFIO_MAPR2)

/** TIM14 (no remap) */
#define TIM14_REMAP0		STM32_REMAP(0U, 0x1U, 9U, STM32_AFIO_MAPR2)
/** TIM14 (remap) */
#define TIM14_REMAP1		STM32_REMAP(1U, 0x1U, 9U, STM32_AFIO_MAPR2)

/** TIM15 (no remap) */
#define TIM15_REMAP0		STM32_REMAP(0U, 0x1U, 0U, STM32_AFIO_MAPR2)
/** TIM15 (remap) */
#define TIM15_REMAP1		STM32_REMAP(1U, 0x1U, 0U, STM32_AFIO_MAPR2)

/** TIM16 (no remap) */
#define TIM16_REMAP0		STM32_REMAP(0U, 0x1U, 1U, STM32_AFIO_MAPR2)
/** TIM16 (remap) */
#define TIM16_REMAP1		STM32_REMAP(1U, 0x1U, 1U, STM32_AFIO_MAPR2)

/** TIM17 (no remap) */
#define TIM17_REMAP0		STM32_REMAP(0U, 0x1U, 2U, STM32_AFIO_MAPR2)
/** TIM17 (remap) */
#define TIM17_REMAP1		STM32_REMAP(1U, 0x1U, 2U, STM32_AFIO_MAPR2)

#endif	/* ZEPHYR_STM32_AFIO_H_ */
