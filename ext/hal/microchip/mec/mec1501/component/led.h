/**
 *
 * Copyright (c) 2019 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

/** @file led.h
 *MEC1501 Breathing-Blinking LED definitions
 */
/** @defgroup MEC1501 Peripherals LED
 */

#ifndef _LED_H
#define _LED_H

#define MCHP_LED_NUM_BLOCKS	3u

#define MCHP_LED_CFG_OFS	0x00ul
#define MCHP_LED_LIMITS_OFS	0x04ul
#define MCHP_LED_DELAY_OFS	0x08ul
#define MCHP_LED_UPD_STEP_OFS	0x0Cul
#define MCHP_LED_UPD_INTRVL_OFS	0x10ul
#define MCHP_LED_OUTPUT_DLY_OFS	0x14ul

/*
 * LED Configuration Register
 */
#define MCHP_LED_CFG_CNTL_MASK		0x0003u
#define MCHP_LED_CFG_CNTL_LO		0x0000u
#define MCHP_LED_CFG_CNTL_BREATH	0x0001u
#define MCHP_LED_CFG_CNTL_BLINK		0x0002u
#define MCHP_LED_CFG_CNTL_HI		0x0003u
#define MCHP_LED_CFG_CLK_SRC_MCLK	0x0002u
#define MCHP_LED_CFG_CLK_SRC_32K	0x0000u
#define MCHP_LED_CFG_SYNC		0x0008u
#define MCHP_LED_CFG_PWM_COUNT_WIDTH_MASK	0x0030u
#define MCHP_LED_CFG_COUNT_WIDTH_8	0x0000u
#define MCHP_LED_CFG_COUNT_WIDTH_7	0x0010u
#define MCHP_LED_CFG_COUNT_WIDTH_6	0x0020u
#define MCHP_LED_CFG_EN_UPDATE		0x0040u
#define MCHP_LED_CFG_RESET		0x0080u
#define MCHP_LED_CFG_WDT_PRELOAD_MASK	0xFF00u
#define MCHP_LED_CFG_WDT_PRELOAD_POR	0x1400u
#define MCHP_LED_CFG_SYMMETRY_EN	0x10000u

/*
 * LED Limit Register
 */
#define MCHP_LED_LIM_MIN_POS	0u
#define MCHP_LED_LIM_MIN_MASK	0xFFu
#define MCHP_LED_LIM_MAX_POS	8u
#define MCHP_LED_LIM_MAX_MASK	0xFF00ul

/*
 * LED Delay Register
 */
#define MCHP_LED_DLY_LO_MASK	0x0000FFFul
#define MCHP_LED_DLY_HI_MASK	0x0FFF000ul
#define MCHP_LED_DLY_HI_POS	12u

/*
 * LED Step Size Register
 */
#define MCHP_LED_STEP_FIELD_WIDTH	4u
#define MCHP_LED_STEP_MASK0		0x0Ful
#define MCHP_LED_STEP0_POS		0u
#define MCHP_LED_STEP0_MASK		0x0000000Ful
#define MCHP_LED_STEP1_POS		4u
#define MCHP_LED_STEP1_MASK		0x000000F0ul
#define MCHP_LED_STEP2_POS		8u
#define MCHP_LED_STEP2_MASK		0x00000F00ul
#define MCHP_LED_STEP3_POS		12u
#define MCHP_LED_STEP3_MASK		0x0000F000ul
#define MCHP_LED_STEP4_POS		16u
#define MCHP_LED_STEP4_MASK		0x000F0000ul
#define MCHP_LED_STEP5_POS		20u
#define MCHP_LED_STEP5_MASK		0x00F00000ul
#define MCHP_LED_STEP6_POS		24u
#define MCHP_LED_STEP6_MASK		0x0F000000ul
#define MCHP_LED_STEP7_POS		28u
#define MCHP_LED_STEP7_MASK		0xF0000000ul

/*
 * LED Update Register
 */
#define MCHP_LED_UPDT_FIELD_WIDTH	4u
#define MCHP_LED_UPDT_MASK0		0x0Ful;
#define MCHP_LED_UPDT0_POS		0u
#define MCHP_LED_UPDT0_MASK		0x0000000Ful
#define MCHP_LED_UPDT1_POS		4u
#define MCHP_LED_UPDT1_MASK		0x000000F0ul
#define MCHP_LED_UPDT2_POS		8u
#define MCHP_LED_UPDT2_MASK		0x00000F00ul
#define MCHP_LED_UPDT3_POS		12u
#define MCHP_LED_UPDT3_MASK		0x0000F000ul
#define MCHP_LED_UPDT4_POS		16u
#define MCHP_LED_UPDT4_MASK		0x000F0000ul
#define MCHP_LED_UPDT5_POS		20u
#define MCHP_LED_UPDT5_MASK		0x00F00000ul
#define MCHP_LED_UPDT6_POS		24u
#define MCHP_LED_UPDT6_MASK		0x0F000000ul
#define MCHP_LED_UPDT7_POS		28u
#define MCHP_LED_UPDT7_MASK		0xF0000000ul

#define MCHP_BLINK_0P5_HZ_DUTY_CYCLE	0x010ul
#define MCHP_BLINK_0P5_HZ_PRESCALE	0x0FFul
#define MCHP_BLINK_1_HZ_DUTY_CYCLE	0x020ul
#define MCHP_BLINK_1_HZ_PRESCALE	0x07Ful

/* =======================================================================*/
/* ================	       LED			 ================ */
/* =======================================================================*/

#define MCHP_LED_MAX_INSTANCES	4u
#define MCHP_LED_SPACING	0x100ul
#define MCHP_LED_SPACING_PWROF2	8u

#define MCHP_LED_BASE_ADDR	0x4000B800ul

#define MCHP_LED0_ADDR		0x4000B800ul
#define MCHP_LED1_ADDR		0x4000B900ul
#define MCHP_LED2_ADDR		0x4000BA00ul
#define MCHP_LED3_ADDR		0x4000BB00ul

#define MCHP_LED_ADDR(n) \
	(MCHP_LED_BASE_ADDR + ((uint32_t)(n) << MCHP_LED_SPACING_PWROF2))

/**
  * @brief Breathing-Blinking LED (LED)
  */
typedef struct led_regs
{		/*!< (@ 0x4000B800) LED Structure   */
	__IOM uint32_t CFG;	/*! (@ 0x00000000) LED configuration */
	__IOM uint32_t LIMIT;	/*! (@ 0x00000004) LED limits */
	__IOM uint32_t DLY;	/*! (@ 0x00000008) LED delay */
	__IOM uint32_t STEP;	/*! (@ 0x0000000C) LED update step size */
	__IOM uint32_t INTRVL;	/*! (@ 0x00000010) LED update interval */
	__IOM uint32_t OUTDLY;	/*! (@ 0x00000014) LED output delay */
} LED_Type;

#endif				/* #ifndef _LED_H */
/* end led.h */
/**   @}
 */
