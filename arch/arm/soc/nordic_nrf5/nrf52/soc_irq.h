/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
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
 * @file Interrupt numbers for NRF52 family processors.
 *
 * Based on Nordic MDK included header file: nrf52.h
 */


#ifndef _NRF52_SOC_IRQ_H_
#define _NRF52_SOC_IRQ_H_

#define NRF52_IRQ_POWER_CLOCK_IRQn			 0
#define NRF52_IRQ_RADIO_IRQn				 1
#define NRF52_IRQ_UARTE0_UART0_IRQn			 2
#define NRF52_IRQ_SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQn 3
#define NRF52_IRQ_SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQn 4
#define NRF52_IRQ_NFCT_IRQn				 5
#define NRF52_IRQ_GPIOTE_IRQn				 6
#define NRF52_IRQ_SAADC_IRQn				 7
#define NRF52_IRQ_TIMER0_IRQn				 8
#define NRF52_IRQ_TIMER1_IRQn				 9
#define NRF52_IRQ_TIMER2_IRQn				 10
#define NRF52_IRQ_RTC0_IRQn				 11
#define NRF52_IRQ_TEMP_IRQn				 12
#define NRF52_IRQ_RNG_IRQn				 13
#define NRF52_IRQ_ECB_IRQn				 14
#define NRF52_IRQ_CCM_AAR_IRQn				 15
#define NRF52_IRQ_WDT_IRQn				 16
#define NRF52_IRQ_RTC1_IRQn				 17
#define NRF52_IRQ_QDEC_IRQn				 18
#define NRF52_IRQ_COMP_LPCOMP_IRQn			 19
#define NRF52_IRQ_SWI0_EGU0_IRQn			 20
#define NRF52_IRQ_SWI1_EGU1_IRQn			 21
#define NRF52_IRQ_SWI2_EGU2_IRQn			 22
#define NRF52_IRQ_SWI3_EGU3_IRQn			 23
#define NRF52_IRQ_SWI4_EGU4_IRQn			 24
#define NRF52_IRQ_SWI5_EGU5_IRQn			 25
#define NRF52_IRQ_TIMER3_IRQn				 26
#define NRF52_IRQ_TIMER4_IRQn				 27
#define NRF52_IRQ_PWM0_IRQn				 28
#define NRF52_IRQ_PDM_IRQn				 29
#define NRF52_IRQ_MWU_IRQn				 32
#define NRF52_IRQ_PWM1_IRQn				 33
#define NRF52_IRQ_PWM2_IRQn				 34
#define NRF52_IRQ_SPIM2_SPIS2_SPI2_IRQn			 35
#define NRF52_IRQ_RTC2_IRQn				 36
#define NRF52_IRQ_I2S_IRQn				 37
#define NRF52_IRQ_FPU_IRQn				 38

#endif /* _NRF52_SOC_IRQ_H_ */
