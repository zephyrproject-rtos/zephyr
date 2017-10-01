/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32F10X_GPIO_REGISTERS_H_
#define _STM32F10X_GPIO_REGISTERS_H_

/**
 * @brief
 *
 * Based on reference manual:
 *   STM32F101xx, STM32F102xx, STM32F103xx, STM32F105xx and STM32F107xx
 *   advanced ARM(r)-based 32-bit MCUs
 *
 * Chapter 9: General-purpose and alternate-function I/Os
 *            (GPIOs and AFIOs)
 */

/* 9.2 GPIO registers - each GPIO port controls 16 pins */
struct stm32f10x_gpio {
	u32_t crl;
	u32_t crh;
	u32_t idr;
	u32_t odr;
	u32_t bsrr;
	u32_t brr;
	u32_t lckr;
};

/* 9.4.1 AFIO_EVCR */
union __afio_evcr {
	u32_t val;
	struct {
		u32_t pin :4 __packed;
		u32_t port :3 __packed;
		u32_t evoe :1 __packed;
		u32_t rsvd__8_31 :24 __packed;
	} bit;
};

/* 9.4.2 AFIO_MAPR */
/* TODO: support connectivity line devices */
union __afio_mapr {
	u32_t val;
	struct {
		u32_t spi1_remap :1 __packed;
		u32_t i2c1_remap :1 __packed;
		u32_t usart1_remap :1 __packed;
		u32_t usart2_remap :1 __packed;
		u32_t usart3_remap :2 __packed;
		u32_t tim1_remap :2 __packed;
		u32_t tim2_remap :2 __packed;
		u32_t tim3_remap :2 __packed;
		u32_t tim4_remap :1 __packed;
		u32_t can_remap :2 __packed;
		u32_t pd01_remap :1 __packed;
		u32_t tim5ch4_iremap :1 __packed;
		u32_t adc1_etrginj_remap :1 __packed;
		u32_t adc1_etrgreg_remap :1 __packed;
		u32_t adc2_etrginj_remap :1 __packed;
		u32_t adc2_etrgreg_remap :1 __packed;
		u32_t rsvd__21_23 :3 __packed;
		u32_t swj_cfg :3 __packed;
		u32_t rsvd__27_31 :5 __packed;
	} bit;
};

/* 9.4.{3,4,5,6} AFIO_EXTICRx */
union __afio_exticr {
	u32_t val;
	struct {
		u16_t rsvd__16_31;
		u16_t exti;
	} bit;
};

/* 9.4.7 AFIO_MAPR2 */
union __afio_mapr2 {
	u32_t val;
	struct {
		u32_t rsvd__0_4 :5 __packed;
		u32_t tim9_remap :1 __packed;
		u32_t tim10_remap :1 __packed;
		u32_t tim11_remap :1 __packed;
		u32_t tim13_remap :1 __packed;
		u32_t tim14_remap :1 __packed;
		u32_t fsmc_nadv :1 __packed;
		u32_t rsvd__11_31 :21 __packed;
	} bit;
};

/* 9.4 AFIO registers */
struct stm32f10x_afio {
	union __afio_evcr evcr;
	union __afio_mapr mapr;
	union __afio_exticr exticr1;
	union __afio_exticr exticr2;
	union __afio_exticr exticr3;
	union __afio_exticr exticr4;
	union __afio_mapr2 mapr2;
};

#endif /* _STM32F10X_GPIO_REGISTERS_H_ */
