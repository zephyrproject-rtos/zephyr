/*******************************************************************************
 * @file  sl_si91x_external_oscillator.h
 * @brief
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#ifndef __SL_SI91X_EXTERNAL_OSCILLATOR__
#define __SL_SI91X_EXTERNAL_OSCILLATOR__

/* <<< Use Configuration Wizard in Context Menu >>> */
/* <h>External Oscillator Configuration on UULP GPIOs */

/* <o OSC_UULP_GPIO> UULP_GPIO_NUMBER */
/*   <UULP_GPIO_1=> 1 */
/*   <UULP_GPIO_2=> 2 */
/*   <UULP_GPIO_3=> 3 */
/*   <UULP_GPIO_4=> 4 */
/* <i> Default: UULP_GPIO_3 */

#define UULP_GPIO_1   1
#define UULP_GPIO_2   2
#define UULP_GPIO_3   3
#define UULP_GPIO_4   4
#define OSC_UULP_GPIO UULP_GPIO_3

#if (OSC_UULP_GPIO == 1)
#define UULP_GPIO_OSC_MODE 3
#elif (OSC_UULP_GPIO == 2)
#define UULP_GPIO_OSC_MODE 4
#elif (OSC_UULP_GPIO == 3)
#define UULP_GPIO_OSC_MODE 5
#elif (OSC_UULP_GPIO == 4)
#define UULP_GPIO_OSC_MODE 6
#endif

/* </h> */
/* <<< end of configuration section >>> */

#endif /*__SL_SI91X_EXTERNAL_OSCILLATOR__*/
