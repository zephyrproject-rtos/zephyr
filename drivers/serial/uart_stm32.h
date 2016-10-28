/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @brief Driver for UART port on STM32F10x family processor.
 *
 * Based on reference manual:
 *   STM32F101xx, STM32F102xx, STM32F103xx, STM32F105xx and STM32F107xx
 *   advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 27: Universal synchronous asynchronous receiver
 *             transmitter (USART)
 */

#ifndef _STM32_UART_H_
#define _STM32_UART_H_

/*  27.6.1 Status register (USART_SR) */
union __sr {
	uint32_t val;
	struct {
		uint32_t pe :1 __packed;
		uint32_t fe :1 __packed;
		uint32_t nf :1 __packed;
		uint32_t ore :1 __packed;
		uint32_t idle :1 __packed;
		uint32_t rxne :1 __packed;
		uint32_t tc :1 __packed;
		uint32_t txe :1 __packed;
		uint32_t lbd :1 __packed;
		uint32_t cts :1 __packed;
		uint32_t rsvd__10_15 : 6 __packed;
		uint32_t rsvd__16_31 : 16 __packed;
	} bit;
};

/* 27.6.2 Data register (USART_DR) */
union __dr {
	uint32_t val;
	struct {
		uint32_t dr :8 __packed;
		uint32_t rsvd__9_31 :24 __packed;
	} bit;
};

/* 27.6.3 Baud rate register (USART_BRR) */
union __brr {
	uint32_t val;
	struct {
		uint32_t fraction :4 __packed;
		uint32_t mantissa :12 __packed;
		uint32_t rsvd__16_31 :16 __packed;
	} bit;
};

/* 27.6.4 Control register 1 (USART_CR1) */
union __cr1 {
	uint32_t val;
	struct {
		uint32_t sbk :1 __packed;
		uint32_t rwu :1 __packed;
		uint32_t re :1 __packed;
		uint32_t te :1 __packed;
		uint32_t idleie :1 __packed;
		uint32_t rxneie :1 __packed;
		uint32_t tcie :1 __packed;
		uint32_t txeie :1 __packed;
		uint32_t peie :1 __packed;
		uint32_t ps :1 __packed;
		uint32_t pce :1 __packed;
		uint32_t wake :1 __packed;
		uint32_t m :1 __packed;
		uint32_t ue :1 __packed;
#ifdef CONFIG_SOC_SERIES_SOC32F1X
		uint32_t rsvd__14_15 :2 __packed;
#elif CONFIG_SOC_SERIES_SOC32F4X
		uint32_t rsvd__14 :1 __packed;
		uint32_t over8 :1 __packed;
#endif
		uint32_t rsvd__16_31 :16 __packed;
	} bit;
};

/* 27.6.5 Control register 2 (USART_CR2) */
union __cr2 {
	uint32_t val;
	struct {
		uint32_t addr :4 __packed;
		uint32_t rsvd__4 :1 __packed;
		uint32_t lbdl :1 __packed;
		uint32_t lbdie :1 __packed;
		uint32_t rsvd__7 :1 __packed;
		uint32_t lbcl :1 __packed;
		uint32_t cpha :1 __packed;
		uint32_t cpol :1 __packed;
		uint32_t clken :1 __packed;
		uint32_t stop :2 __packed;
		uint32_t linen :1 __packed;
		uint32_t rsvd__15_31 :17 __packed;
	} bit;
};

/* 27.6.6 Control register 3 (USART_CR3) */
union __cr3 {
	uint32_t val;
	struct {
		uint32_t eie :1 __packed;
		uint32_t iren :1 __packed;
		uint32_t irlp :1 __packed;
		uint32_t hdsel :1 __packed;
		uint32_t nack :1 __packed;
		uint32_t scen :1 __packed;
		uint32_t dmar :1 __packed;
		uint32_t dmat :1 __packed;
		uint32_t rtse :1 __packed;
		uint32_t ctse :1 __packed;
		uint32_t ctsie :1 __packed;
#ifdef CONFIG_SOC_SERIES_SOC32F1X
		uint32_t rsvd__11_31 :21 __packed;
#elif CONFIG_SOC_SERIES_SOC32F4X
		uint32_t onebit :1 __packed;
		uint32_t rsvd__12_31 :20 __packed;
#endif
	} bit;
};

/* 27.6.7 Guard time and prescaler register (USART_GTPR) */
union __gtpr {
	uint32_t val;
	struct {
		uint32_t psc :8 __packed;
		uint32_t gt :8 __packed;
		uint32_t rsvd__16_31 :16 __packed;
	} bit;
};

/* 27.6.8 USART register map */
struct uart_stm32 {
	union __sr sr;
	union __dr dr;
	union __brr brr;
	union __cr1 cr1;
	union __cr2 cr2;
	union __cr3 cr3;
	union __gtpr gtpr;
};

/* device config */
struct uart_stm32_config {
	struct uart_device_config uconf;
	/* clock subsystem driving this peripheral */
#ifdef CONFIG_SOC_SERIES_STM32F1X
	clock_control_subsys_t clock_subsys;
#elif CONFIG_SOC_SERIES_STM32F4X
	struct stm32f4x_pclken pclken;
#endif
};

/* driver data */
struct uart_stm32_data {
	/* current baud rate */
	uint32_t baud_rate;
	/* clock device */
	struct device *clock;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_t user_cb;
#endif
};

#endif	/* _STM32_UART_H_ */
