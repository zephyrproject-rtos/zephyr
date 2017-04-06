/**************************************************************************//**
 * @file em_opamp.h
 * @brief Operational Amplifier (OPAMP) peripheral API
 * @version 5.1.2
 ******************************************************************************
 * @section License
 * <b>Copyright 2016 Silicon Laboratories, Inc. http://www.silabs.com</b>
 *******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Silicon Labs has no
 * obligation to support this Software. Silicon Labs is providing the
 * Software "AS IS", with no express or implied warranties of any kind,
 * including, but not limited to, any implied warranties of merchantability
 * or fitness for any particular purpose or warranties against infringement
 * of any proprietary rights of a third party.
 *
 * Silicon Labs will not be liable for any consequential, incidental, or
 * special damages, or any other relief, or for any claim by any third party,
 * arising from your use of this Software.
 *
 ******************************************************************************/

#ifndef EM_OPAMP_H
#define EM_OPAMP_H

#include "em_device.h"
#if ((defined(_SILICON_LABS_32B_SERIES_0) && defined(OPAMP_PRESENT) && (OPAMP_COUNT == 1)) \
     || (defined(_SILICON_LABS_32B_SERIES_1) && defined(VDAC_PRESENT)  && (VDAC_COUNT > 0)))

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#if defined(_SILICON_LABS_32B_SERIES_0)
#include "em_dac.h"
#elif defined (_SILICON_LABS_32B_SERIES_1)
#include "em_vdac.h"
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup OPAMP
 * @{
 ******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/** Validation of DAC OPA number for assert statements. */
#if defined(_SILICON_LABS_32B_SERIES_0)
#define DAC_OPA_VALID(opa)    ((opa) <= OPA2)
#elif defined(_SILICON_LABS_32B_SERIES_1)
#define VDAC_OPA_VALID(opa)   ((opa) <= OPA2)
#endif

/** @endcond */

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

/** OPAMP selector values. */
typedef enum
{
  OPA0 = 0,                   /**< Select OPA0. */
  OPA1 = 1,                   /**< Select OPA1. */
  OPA2 = 2                    /**< Select OPA2. */
} OPAMP_TypeDef;

/** OPAMP negative terminal input selection values. */
typedef enum
{
#if defined(_SILICON_LABS_32B_SERIES_0)
  opaNegSelDisable   = DAC_OPA0MUX_NEGSEL_DISABLE,    /**< Input disabled.               */
  opaNegSelUnityGain = DAC_OPA0MUX_NEGSEL_UG,         /**< Unity gain feedback path.     */
  opaNegSelResTap    = DAC_OPA0MUX_NEGSEL_OPATAP,     /**< Feedback resistor ladder tap. */
  opaNegSelNegPad    = DAC_OPA0MUX_NEGSEL_NEGPAD      /**< Negative pad as input.        */
#elif defined(_SILICON_LABS_32B_SERIES_1)
  opaNegSelAPORT1YCH1   = VDAC_OPA_MUX_NEGSEL_APORT1YCH1,   /**< APORT1YCH1                    */
  opaNegSelAPORT1YCH3   = VDAC_OPA_MUX_NEGSEL_APORT1YCH3,   /**< APORT1YCH3                    */
  opaNegSelAPORT1YCH5   = VDAC_OPA_MUX_NEGSEL_APORT1YCH5,   /**< APORT1YCH5                    */
  opaNegSelAPORT1YCH7   = VDAC_OPA_MUX_NEGSEL_APORT1YCH7,   /**< APORT1YCH7                    */
  opaNegSelAPORT1YCH9   = VDAC_OPA_MUX_NEGSEL_APORT1YCH9,   /**< APORT1YCH9                    */
  opaNegSelAPORT1YCH11  = VDAC_OPA_MUX_NEGSEL_APORT1YCH11,  /**< APORT1YCH11                   */
  opaNegSelAPORT1YCH13  = VDAC_OPA_MUX_NEGSEL_APORT1YCH13,  /**< APORT1YCH13                   */
  opaNegSelAPORT1YCH15  = VDAC_OPA_MUX_NEGSEL_APORT1YCH15,  /**< APORT1YCH15                   */
  opaNegSelAPORT1YCH17  = VDAC_OPA_MUX_NEGSEL_APORT1YCH17,  /**< APORT1YCH17                   */
  opaNegSelAPORT1YCH19  = VDAC_OPA_MUX_NEGSEL_APORT1YCH19,  /**< APORT1YCH19                   */
  opaNegSelAPORT1YCH21  = VDAC_OPA_MUX_NEGSEL_APORT1YCH21,  /**< APORT1YCH21                   */
  opaNegSelAPORT1YCH23  = VDAC_OPA_MUX_NEGSEL_APORT1YCH23,  /**< APORT1YCH23                   */
  opaNegSelAPORT1YCH25  = VDAC_OPA_MUX_NEGSEL_APORT1YCH25,  /**< APORT1YCH25                   */
  opaNegSelAPORT1YCH27  = VDAC_OPA_MUX_NEGSEL_APORT1YCH27,  /**< APORT1YCH27                   */
  opaNegSelAPORT1YCH29  = VDAC_OPA_MUX_NEGSEL_APORT1YCH29,  /**< APORT1YCH29                   */
  opaNegSelAPORT1YCH31  = VDAC_OPA_MUX_NEGSEL_APORT1YCH31,  /**< APORT1YCH31                   */
  opaNegSelAPORT2YCH0   = VDAC_OPA_MUX_NEGSEL_APORT2YCH0,   /**< APORT2YCH0                    */
  opaNegSelAPORT2YCH2   = VDAC_OPA_MUX_NEGSEL_APORT2YCH2,   /**< APORT2YCH2                    */
  opaNegSelAPORT2YCH4   = VDAC_OPA_MUX_NEGSEL_APORT2YCH4,   /**< APORT2YCH4                    */
  opaNegSelAPORT2YCH6   = VDAC_OPA_MUX_NEGSEL_APORT2YCH6,   /**< APORT2YCH6                    */
  opaNegSelAPORT2YCH8   = VDAC_OPA_MUX_NEGSEL_APORT2YCH8,   /**< APORT2YCH8                    */
  opaNegSelAPORT2YCH10  = VDAC_OPA_MUX_NEGSEL_APORT2YCH10,  /**< APORT2YCH10                   */
  opaNegSelAPORT2YCH12  = VDAC_OPA_MUX_NEGSEL_APORT2YCH12,  /**< APORT2YCH12                   */
  opaNegSelAPORT2YCH14  = VDAC_OPA_MUX_NEGSEL_APORT2YCH14,  /**< APORT2YCH14                   */
  opaNegSelAPORT2YCH16  = VDAC_OPA_MUX_NEGSEL_APORT2YCH16,  /**< APORT2YCH16                   */
  opaNegSelAPORT2YCH18  = VDAC_OPA_MUX_NEGSEL_APORT2YCH18,  /**< APORT2YCH18                   */
  opaNegSelAPORT2YCH20  = VDAC_OPA_MUX_NEGSEL_APORT2YCH20,  /**< APORT2YCH20                   */
  opaNegSelAPORT2YCH22  = VDAC_OPA_MUX_NEGSEL_APORT2YCH22,  /**< APORT2YCH22                   */
  opaNegSelAPORT2YCH24  = VDAC_OPA_MUX_NEGSEL_APORT2YCH24,  /**< APORT2YCH24                   */
  opaNegSelAPORT2YCH26  = VDAC_OPA_MUX_NEGSEL_APORT2YCH26,  /**< APORT2YCH26                   */
  opaNegSelAPORT2YCH28  = VDAC_OPA_MUX_NEGSEL_APORT2YCH28,  /**< APORT2YCH28                   */
  opaNegSelAPORT2YCH30  = VDAC_OPA_MUX_NEGSEL_APORT2YCH30,  /**< APORT2YCH30                   */
  opaNegSelAPORT3YCH1   = VDAC_OPA_MUX_NEGSEL_APORT3YCH1,   /**< APORT3YCH1                    */
  opaNegSelAPORT3YCH3   = VDAC_OPA_MUX_NEGSEL_APORT3YCH3,   /**< APORT3YCH3                    */
  opaNegSelAPORT3YCH5   = VDAC_OPA_MUX_NEGSEL_APORT3YCH5,   /**< APORT3YCH5                    */
  opaNegSelAPORT3YCH7   = VDAC_OPA_MUX_NEGSEL_APORT3YCH7,   /**< APORT3YCH7                    */
  opaNegSelAPORT3YCH9   = VDAC_OPA_MUX_NEGSEL_APORT3YCH9,   /**< APORT3YCH9                    */
  opaNegSelAPORT3YCH11  = VDAC_OPA_MUX_NEGSEL_APORT3YCH11,  /**< APORT3YCH11                   */
  opaNegSelAPORT3YCH13  = VDAC_OPA_MUX_NEGSEL_APORT3YCH13,  /**< APORT3YCH13                   */
  opaNegSelAPORT3YCH15  = VDAC_OPA_MUX_NEGSEL_APORT3YCH15,  /**< APORT3YCH15                   */
  opaNegSelAPORT3YCH17  = VDAC_OPA_MUX_NEGSEL_APORT3YCH17,  /**< APORT3YCH17                   */
  opaNegSelAPORT3YCH19  = VDAC_OPA_MUX_NEGSEL_APORT3YCH19,  /**< APORT3YCH19                   */
  opaNegSelAPORT3YCH21  = VDAC_OPA_MUX_NEGSEL_APORT3YCH21,  /**< APORT3YCH21                   */
  opaNegSelAPORT3YCH23  = VDAC_OPA_MUX_NEGSEL_APORT3YCH23,  /**< APORT3YCH23                   */
  opaNegSelAPORT3YCH25  = VDAC_OPA_MUX_NEGSEL_APORT3YCH25,  /**< APORT3YCH25                   */
  opaNegSelAPORT3YCH27  = VDAC_OPA_MUX_NEGSEL_APORT3YCH27,  /**< APORT3YCH27                   */
  opaNegSelAPORT3YCH29  = VDAC_OPA_MUX_NEGSEL_APORT3YCH29,  /**< APORT3YCH29                   */
  opaNegSelAPORT3YCH31  = VDAC_OPA_MUX_NEGSEL_APORT3YCH31,  /**< APORT3YCH31                   */
  opaNegSelAPORT4YCH0   = VDAC_OPA_MUX_NEGSEL_APORT4YCH0,   /**< APORT4YCH0                    */
  opaNegSelAPORT4YCH2   = VDAC_OPA_MUX_NEGSEL_APORT4YCH2,   /**< APORT4YCH2                    */
  opaNegSelAPORT4YCH4   = VDAC_OPA_MUX_NEGSEL_APORT4YCH4,   /**< APORT4YCH4                    */
  opaNegSelAPORT4YCH6   = VDAC_OPA_MUX_NEGSEL_APORT4YCH6,   /**< APORT4YCH6                    */
  opaNegSelAPORT4YCH8   = VDAC_OPA_MUX_NEGSEL_APORT4YCH8,   /**< APORT4YCH8                    */
  opaNegSelAPORT4YCH10  = VDAC_OPA_MUX_NEGSEL_APORT4YCH10,  /**< APORT4YCH10                   */
  opaNegSelAPORT4YCH12  = VDAC_OPA_MUX_NEGSEL_APORT4YCH12,  /**< APORT4YCH12                   */
  opaNegSelAPORT4YCH14  = VDAC_OPA_MUX_NEGSEL_APORT4YCH14,  /**< APORT4YCH14                   */
  opaNegSelAPORT4YCH16  = VDAC_OPA_MUX_NEGSEL_APORT4YCH16,  /**< APORT4YCH16                   */
  opaNegSelAPORT4YCH18  = VDAC_OPA_MUX_NEGSEL_APORT4YCH18,  /**< APORT4YCH18                   */
  opaNegSelAPORT4YCH20  = VDAC_OPA_MUX_NEGSEL_APORT4YCH20,  /**< APORT4YCH20                   */
  opaNegSelAPORT4YCH22  = VDAC_OPA_MUX_NEGSEL_APORT4YCH22,  /**< APORT4YCH22                   */
  opaNegSelAPORT4YCH24  = VDAC_OPA_MUX_NEGSEL_APORT4YCH24,  /**< APORT4YCH24                   */
  opaNegSelAPORT4YCH26  = VDAC_OPA_MUX_NEGSEL_APORT4YCH26,  /**< APORT4YCH26                   */
  opaNegSelAPORT4YCH28  = VDAC_OPA_MUX_NEGSEL_APORT4YCH28,  /**< APORT4YCH28                   */
  opaNegSelAPORT4YCH30  = VDAC_OPA_MUX_NEGSEL_APORT4YCH30,  /**< APORT4YCH30                   */
  opaNegSelDisable      = VDAC_OPA_MUX_NEGSEL_DISABLE,      /**< Input disabled.               */
  opaNegSelUnityGain    = VDAC_OPA_MUX_NEGSEL_UG,           /**< Unity gain feedback path.     */
  opaNegSelResTap       = VDAC_OPA_MUX_NEGSEL_OPATAP,       /**< Feedback resistor ladder tap. */
  opaNegSelNegPad       = VDAC_OPA_MUX_NEGSEL_NEGPAD        /**< Negative pad as input.        */
#endif /* defined(_SILICON_LABS_32B_SERIES_0) */
} OPAMP_NegSel_TypeDef;

/** OPAMP positive terminal input selection values. */
typedef enum
{
#if defined(_SILICON_LABS_32B_SERIES_0)
  opaPosSelDisable    = DAC_OPA0MUX_POSSEL_DISABLE,   /**< Input disabled.          */
  opaPosSelDac        = DAC_OPA0MUX_POSSEL_DAC,       /**< DAC as input (not OPA2). */
  opaPosSelPosPad     = DAC_OPA0MUX_POSSEL_POSPAD,    /**< Positive pad as input.   */
  opaPosSelOpaIn      = DAC_OPA0MUX_POSSEL_OPA0INP,   /**< Input from OPAx.         */
  opaPosSelResTapOpa0 = DAC_OPA0MUX_POSSEL_OPATAP     /**< Feedback resistor ladder tap from OPA0. */
#elif defined(_SILICON_LABS_32B_SERIES_1)
  opaPosSelAPORT1XCH0   = VDAC_OPA_MUX_POSSEL_APORT1XCH0,   /**< APORT1XCH0                    */
  opaPosSelAPORT1XCH2   = VDAC_OPA_MUX_POSSEL_APORT1XCH2,   /**< APORT1XCH2                    */
  opaPosSelAPORT1XCH4   = VDAC_OPA_MUX_POSSEL_APORT1XCH4,   /**< APORT1XCH4                    */
  opaPosSelAPORT1XCH6   = VDAC_OPA_MUX_POSSEL_APORT1XCH6,   /**< APORT1XCH6                    */
  opaPosSelAPORT1XCH8   = VDAC_OPA_MUX_POSSEL_APORT1XCH8,   /**< APORT1XCH8                    */
  opaPosSelAPORT1XCH10  = VDAC_OPA_MUX_POSSEL_APORT1XCH10,  /**< APORT1XCH10                   */
  opaPosSelAPORT1XCH12  = VDAC_OPA_MUX_POSSEL_APORT1XCH12,  /**< APORT1XCH12                   */
  opaPosSelAPORT1XCH14  = VDAC_OPA_MUX_POSSEL_APORT1XCH14,  /**< APORT1XCH14                   */
  opaPosSelAPORT1XCH16  = VDAC_OPA_MUX_POSSEL_APORT1XCH16,  /**< APORT1XCH16                   */
  opaPosSelAPORT1XCH18  = VDAC_OPA_MUX_POSSEL_APORT1XCH18,  /**< APORT1XCH18                   */
  opaPosSelAPORT1XCH20  = VDAC_OPA_MUX_POSSEL_APORT1XCH20,  /**< APORT1XCH20                   */
  opaPosSelAPORT1XCH22  = VDAC_OPA_MUX_POSSEL_APORT1XCH22,  /**< APORT1XCH22                   */
  opaPosSelAPORT1XCH24  = VDAC_OPA_MUX_POSSEL_APORT1XCH24,  /**< APORT1XCH24                   */
  opaPosSelAPORT1XCH26  = VDAC_OPA_MUX_POSSEL_APORT1XCH26,  /**< APORT1XCH26                   */
  opaPosSelAPORT1XCH28  = VDAC_OPA_MUX_POSSEL_APORT1XCH28,  /**< APORT1XCH28                   */
  opaPosSelAPORT1XCH30  = VDAC_OPA_MUX_POSSEL_APORT1XCH30,  /**< APORT1XCH30                   */
  opaPosSelAPORT2XCH1   = VDAC_OPA_MUX_POSSEL_APORT2XCH1,   /**< APORT2XCH1                    */
  opaPosSelAPORT2XCH3   = VDAC_OPA_MUX_POSSEL_APORT2XCH3,   /**< APORT2XCH3                    */
  opaPosSelAPORT2XCH5   = VDAC_OPA_MUX_POSSEL_APORT2XCH5,   /**< APORT2XCH5                    */
  opaPosSelAPORT2XCH7   = VDAC_OPA_MUX_POSSEL_APORT2XCH7,   /**< APORT2XCH7                    */
  opaPosSelAPORT2XCH9   = VDAC_OPA_MUX_POSSEL_APORT2XCH9,   /**< APORT2XCH9                    */
  opaPosSelAPORT2XCH11  = VDAC_OPA_MUX_POSSEL_APORT2XCH11,  /**< APORT2XCH11                   */
  opaPosSelAPORT2XCH13  = VDAC_OPA_MUX_POSSEL_APORT2XCH13,  /**< APORT2XCH13                   */
  opaPosSelAPORT2XCH15  = VDAC_OPA_MUX_POSSEL_APORT2XCH15,  /**< APORT2XCH15                   */
  opaPosSelAPORT2XCH17  = VDAC_OPA_MUX_POSSEL_APORT2XCH17,  /**< APORT2XCH17                   */
  opaPosSelAPORT2XCH19  = VDAC_OPA_MUX_POSSEL_APORT2XCH19,  /**< APORT2XCH19                   */
  opaPosSelAPORT2XCH21  = VDAC_OPA_MUX_POSSEL_APORT2XCH21,  /**< APORT2XCH21                   */
  opaPosSelAPORT2XCH23  = VDAC_OPA_MUX_POSSEL_APORT2XCH23,  /**< APORT2XCH23                   */
  opaPosSelAPORT2XCH25  = VDAC_OPA_MUX_POSSEL_APORT2XCH25,  /**< APORT2XCH25                   */
  opaPosSelAPORT2XCH27  = VDAC_OPA_MUX_POSSEL_APORT2XCH27,  /**< APORT2XCH27                   */
  opaPosSelAPORT2XCH29  = VDAC_OPA_MUX_POSSEL_APORT2XCH29,  /**< APORT2XCH29                   */
  opaPosSelAPORT2XCH31  = VDAC_OPA_MUX_POSSEL_APORT2XCH31,  /**< APORT2XCH31                   */
  opaPosSelAPORT3XCH0   = VDAC_OPA_MUX_POSSEL_APORT3XCH0,   /**< APORT3XCH0                    */
  opaPosSelAPORT3XCH2   = VDAC_OPA_MUX_POSSEL_APORT3XCH2,   /**< APORT3XCH2                    */
  opaPosSelAPORT3XCH4   = VDAC_OPA_MUX_POSSEL_APORT3XCH4,   /**< APORT3XCH4                    */
  opaPosSelAPORT3XCH6   = VDAC_OPA_MUX_POSSEL_APORT3XCH6,   /**< APORT3XCH6                    */
  opaPosSelAPORT3XCH8   = VDAC_OPA_MUX_POSSEL_APORT3XCH8,   /**< APORT3XCH8                    */
  opaPosSelAPORT3XCH10  = VDAC_OPA_MUX_POSSEL_APORT3XCH10,  /**< APORT3XCH10                   */
  opaPosSelAPORT3XCH12  = VDAC_OPA_MUX_POSSEL_APORT3XCH12,  /**< APORT3XCH12                   */
  opaPosSelAPORT3XCH14  = VDAC_OPA_MUX_POSSEL_APORT3XCH14,  /**< APORT3XCH14                   */
  opaPosSelAPORT3XCH16  = VDAC_OPA_MUX_POSSEL_APORT3XCH16,  /**< APORT3XCH16                   */
  opaPosSelAPORT3XCH18  = VDAC_OPA_MUX_POSSEL_APORT3XCH18,  /**< APORT3XCH18                   */
  opaPosSelAPORT3XCH20  = VDAC_OPA_MUX_POSSEL_APORT3XCH20,  /**< APORT3XCH20                   */
  opaPosSelAPORT3XCH22  = VDAC_OPA_MUX_POSSEL_APORT3XCH22,  /**< APORT3XCH22                   */
  opaPosSelAPORT3XCH24  = VDAC_OPA_MUX_POSSEL_APORT3XCH24,  /**< APORT3XCH24                   */
  opaPosSelAPORT3XCH26  = VDAC_OPA_MUX_POSSEL_APORT3XCH26,  /**< APORT3XCH26                   */
  opaPosSelAPORT3XCH28  = VDAC_OPA_MUX_POSSEL_APORT3XCH28,  /**< APORT3XCH28                   */
  opaPosSelAPORT3XCH30  = VDAC_OPA_MUX_POSSEL_APORT3XCH30,  /**< APORT3XCH30                   */
  opaPosSelAPORT4XCH1   = VDAC_OPA_MUX_POSSEL_APORT4XCH1,   /**< APORT4XCH1                    */
  opaPosSelAPORT4XCH3   = VDAC_OPA_MUX_POSSEL_APORT4XCH3,   /**< APORT4XCH3                    */
  opaPosSelAPORT4XCH5   = VDAC_OPA_MUX_POSSEL_APORT4XCH5,   /**< APORT4XCH5                    */
  opaPosSelAPORT4XCH7   = VDAC_OPA_MUX_POSSEL_APORT4XCH7,   /**< APORT4XCH7                    */
  opaPosSelAPORT4XCH9   = VDAC_OPA_MUX_POSSEL_APORT4XCH9,   /**< APORT4XCH9                    */
  opaPosSelAPORT4XCH11  = VDAC_OPA_MUX_POSSEL_APORT4XCH11,  /**< APORT4XCH11                   */
  opaPosSelAPORT4XCH13  = VDAC_OPA_MUX_POSSEL_APORT4XCH13,  /**< APORT4XCH13                   */
  opaPosSelAPORT4XCH15  = VDAC_OPA_MUX_POSSEL_APORT4XCH15,  /**< APORT4XCH15                   */
  opaPosSelAPORT4XCH17  = VDAC_OPA_MUX_POSSEL_APORT4XCH17,  /**< APORT4XCH17                   */
  opaPosSelAPORT4XCH19  = VDAC_OPA_MUX_POSSEL_APORT4XCH19,  /**< APORT4XCH19                   */
  opaPosSelAPORT4XCH21  = VDAC_OPA_MUX_POSSEL_APORT4XCH21,  /**< APORT4XCH21                   */
  opaPosSelAPORT4XCH23  = VDAC_OPA_MUX_POSSEL_APORT4XCH23,  /**< APORT4XCH23                   */
  opaPosSelAPORT4XCH25  = VDAC_OPA_MUX_POSSEL_APORT4XCH25,  /**< APORT4XCH25                   */
  opaPosSelAPORT4XCH27  = VDAC_OPA_MUX_POSSEL_APORT4XCH27,  /**< APORT4XCH27                   */
  opaPosSelAPORT4XCH29  = VDAC_OPA_MUX_POSSEL_APORT4XCH29,  /**< APORT4XCH29                   */
  opaPosSelAPORT4XCH31  = VDAC_OPA_MUX_POSSEL_APORT4XCH31,  /**< APORT4XCH31                   */
  opaPosSelDisable      = VDAC_OPA_MUX_POSSEL_DISABLE,      /**< Input disabled.               */
  opaPosSelDac          = VDAC_OPA_MUX_POSSEL_DAC,          /**< DAC as input (not OPA2).      */
  opaPosSelPosPad       = VDAC_OPA_MUX_POSSEL_POSPAD,       /**< Positive pad as input.        */
  opaPosSelOpaIn        = VDAC_OPA_MUX_POSSEL_OPANEXT,      /**< Input from OPAx.              */
  opaPosSelResTap       = VDAC_OPA_MUX_POSSEL_OPATAP        /**< Feedback resistor ladder tap. */
#endif /* defined(_SILICON_LABS_32B_SERIES_0) */
} OPAMP_PosSel_TypeDef;

/** OPAMP output terminal selection values. */
typedef enum
{
#if defined(_SILICON_LABS_32B_SERIES_0)
  opaOutModeDisable = DAC_OPA0MUX_OUTMODE_DISABLE,    /**< OPA output disabled.        */
  opaOutModeMain    = DAC_OPA0MUX_OUTMODE_MAIN,       /**< Main output to pin enabled. */
  opaOutModeAlt     = DAC_OPA0MUX_OUTMODE_ALT,        /**< Alternate output(s) enabled (not OPA2).     */
  opaOutModeAll     = DAC_OPA0MUX_OUTMODE_ALL         /**< Both main and alternate enabled (not OPA2). */
#elif defined(_SILICON_LABS_32B_SERIES_1)
  opaOutModeDisable = 0,                                                                   /**< OPA output disabled.        */
  opaOutModeMain        = VDAC_OPA_OUT_MAINOUTEN,                                          /**< Main output to pin enabled. */
  opaOutModeAlt         = VDAC_OPA_OUT_ALTOUTEN,                                           /**< Alternate output(s) enabled (not OPA2).     */
  opaOutModeAll         = VDAC_OPA_OUT_SHORT,                                              /**< Both main and alternate enabled (not OPA2). */
  opaOutModeAPORT1YCH1  = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH1),   /**< APORT output to APORT1YCH1 pin enabled.  */
  opaOutModeAPORT1YCH3  = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH3),   /**< APORT output to APORT1YCH3 pin enabled.  */
  opaOutModeAPORT1YCH5  = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH5),   /**< APORT output to APORT1YCH5 pin enabled.  */
  opaOutModeAPORT1YCH7  = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH7),   /**< APORT output to APORT1YCH7 pin enabled.  */
  opaOutModeAPORT1YCH9  = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH9),   /**< APORT output to APORT1YCH9 pin enabled.  */
  opaOutModeAPORT1YCH11 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH11),  /**< APORT output to APORT1YCH11 pin enabled. */
  opaOutModeAPORT1YCH13 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH13),  /**< APORT output to APORT1YCH13 pin enabled. */
  opaOutModeAPORT1YCH15 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH15),  /**< APORT output to APORT1YCH15 pin enabled. */
  opaOutModeAPORT1YCH17 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH17),  /**< APORT output to APORT1YCH17 pin enabled. */
  opaOutModeAPORT1YCH19 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH19),  /**< APORT output to APORT1YCH19 pin enabled. */
  opaOutModeAPORT1YCH21 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH21),  /**< APORT output to APORT1YCH21 pin enabled. */
  opaOutModeAPORT1YCH23 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH23),  /**< APORT output to APORT1YCH23 pin enabled. */
  opaOutModeAPORT1YCH25 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH25),  /**< APORT output to APORT1YCH25 pin enabled. */
  opaOutModeAPORT1YCH27 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH27),  /**< APORT output to APORT1YCH27 pin enabled. */
  opaOutModeAPORT1YCH29 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH29),  /**< APORT output to APORT1YCH29 pin enabled. */
  opaOutModeAPORT1YCH31 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT1YCH31),  /**< APORT output to APORT1YCH31 pin enabled. */
  opaOutModeAPORT2YCH0  = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH0),   /**< APORT output to APORT2YCH0 pin enabled.  */
  opaOutModeAPORT2YCH2  = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH2),   /**< APORT output to APORT2YCH2 pin enabled.  */
  opaOutModeAPORT2YCH4  = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH4),   /**< APORT output to APORT2YCH4 pin enabled.  */
  opaOutModeAPORT2YCH6  = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH6),   /**< APORT output to APORT2YCH6 pin enabled.  */
  opaOutModeAPORT2YCH8  = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH8),   /**< APORT output to APORT2YCH8 pin enabled.  */
  opaOutModeAPORT2YCH10 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH10),  /**< APORT output to APORT2YCH10 pin enabled. */
  opaOutModeAPORT2YCH12 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH12),  /**< APORT output to APORT2YCH12 pin enabled. */
  opaOutModeAPORT2YCH14 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH14),  /**< APORT output to APORT2YCH14 pin enabled. */
  opaOutModeAPORT2YCH16 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH16),  /**< APORT output to APORT2YCH16 pin enabled. */
  opaOutModeAPORT2YCH18 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH18),  /**< APORT output to APORT2YCH18 pin enabled. */
  opaOutModeAPORT2YCH20 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH20),  /**< APORT output to APORT2YCH20 pin enabled. */
  opaOutModeAPORT2YCH22 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH22),  /**< APORT output to APORT2YCH22 pin enabled. */
  opaOutModeAPORT2YCH24 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH24),  /**< APORT output to APORT2YCH24 pin enabled. */
  opaOutModeAPORT2YCH26 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH26),  /**< APORT output to APORT2YCH26 pin enabled. */
  opaOutModeAPORT2YCH28 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH28),  /**< APORT output to APORT2YCH28 pin enabled. */
  opaOutModeAPORT2YCH30 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT2YCH30),  /**< APORT output to APORT2YCH30 pin enabled. */
  opaOutModeAPORT3YCH1  = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH1),   /**< APORT output to APORT3YCH1 pin enabled.  */
  opaOutModeAPORT3YCH3  = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH3),   /**< APORT output to APORT3YCH3 pin enabled.  */
  opaOutModeAPORT3YCH5  = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH5),   /**< APORT output to APORT3YCH5 pin enabled.  */
  opaOutModeAPORT3YCH7  = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH7),   /**< APORT output to APORT3YCH7 pin enabled.  */
  opaOutModeAPORT3YCH9  = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH9),   /**< APORT output to APORT3YCH9 pin enabled.  */
  opaOutModeAPORT3YCH11 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH11),  /**< APORT output to APORT3YCH11 pin enabled. */
  opaOutModeAPORT3YCH13 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH13),  /**< APORT output to APORT3YCH13 pin enabled. */
  opaOutModeAPORT3YCH15 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH15),  /**< APORT output to APORT3YCH15 pin enabled. */
  opaOutModeAPORT3YCH17 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH17),  /**< APORT output to APORT3YCH17 pin enabled. */
  opaOutModeAPORT3YCH19 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH19),  /**< APORT output to APORT3YCH19 pin enabled. */
  opaOutModeAPORT3YCH21 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH21),  /**< APORT output to APORT3YCH21 pin enabled. */
  opaOutModeAPORT3YCH23 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH23),  /**< APORT output to APORT3YCH23 pin enabled. */
  opaOutModeAPORT3YCH25 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH25),  /**< APORT output to APORT3YCH25 pin enabled. */
  opaOutModeAPORT3YCH27 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH27),  /**< APORT output to APORT3YCH27 pin enabled. */
  opaOutModeAPORT3YCH29 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH29),  /**< APORT output to APORT3YCH29 pin enabled. */
  opaOutModeAPORT3YCH31 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT3YCH31),  /**< APORT output to APORT3YCH31 pin enabled. */
  opaOutModeAPORT4YCH0  = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH0),   /**< APORT output to APORT4YCH0 pin enabled.  */
  opaOutModeAPORT4YCH2  = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH2),   /**< APORT output to APORT4YCH2 pin enabled.  */
  opaOutModeAPORT4YCH4  = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH4),   /**< APORT output to APORT4YCH4 pin enabled.  */
  opaOutModeAPORT4YCH6  = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH6),   /**< APORT output to APORT4YCH6 pin enabled.  */
  opaOutModeAPORT4YCH8  = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH8),   /**< APORT output to APORT4YCH8 pin enabled.  */
  opaOutModeAPORT4YCH10 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH10),  /**< APORT output to APORT4YCH10 pin enabled. */
  opaOutModeAPORT4YCH12 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH12),  /**< APORT output to APORT4YCH12 pin enabled. */
  opaOutModeAPORT4YCH14 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH14),  /**< APORT output to APORT4YCH14 pin enabled. */
  opaOutModeAPORT4YCH16 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH16),  /**< APORT output to APORT4YCH16 pin enabled. */
  opaOutModeAPORT4YCH18 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH18),  /**< APORT output to APORT4YCH18 pin enabled. */
  opaOutModeAPORT4YCH20 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH20),  /**< APORT output to APORT4YCH20 pin enabled. */
  opaOutModeAPORT4YCH22 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH22),  /**< APORT output to APORT4YCH22 pin enabled. */
  opaOutModeAPORT4YCH24 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH24),  /**< APORT output to APORT4YCH24 pin enabled. */
  opaOutModeAPORT4YCH26 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH26),  /**< APORT output to APORT4YCH26 pin enabled. */
  opaOutModeAPORT4YCH28 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH28),  /**< APORT output to APORT4YCH28 pin enabled. */
  opaOutModeAPORT4YCH30 = (VDAC_OPA_OUT_APORTOUTEN | VDAC_OPA_OUT_APORTOUTSEL_APORT4YCH30),  /**< APORT output to APORT4YCH30 pin enabled. */
#endif /* defined(_SILICON_LABS_32B_SERIES_0) */
} OPAMP_OutMode_TypeDef;

/** OPAMP gain values. */
typedef enum
{
#if defined(_SILICON_LABS_32B_SERIES_0)
  opaResSelDefault    = DAC_OPA0MUX_RESSEL_DEFAULT,  /**< Default value when resistor ladder is unused. */
  opaResSelR2eq0_33R1 = DAC_OPA0MUX_RESSEL_RES0,     /**< R2 = 0.33 * R1 */
  opaResSelR2eqR1     = DAC_OPA0MUX_RESSEL_RES1,     /**< R2 = R1        */
  opaResSelR1eq1_67R1 = DAC_OPA0MUX_RESSEL_RES2,     /**< R2 = 1.67 R1   */
  opaResSelR2eq2R1    = DAC_OPA0MUX_RESSEL_RES3,     /**< R2 = 2 * R1    */
  opaResSelR2eq3R1    = DAC_OPA0MUX_RESSEL_RES4,     /**< R2 = 3 * R1    */
  opaResSelR2eq4_33R1 = DAC_OPA0MUX_RESSEL_RES5,     /**< R2 = 4.33 * R1 */
  opaResSelR2eq7R1    = DAC_OPA0MUX_RESSEL_RES6,     /**< R2 = 7 * R1    */
  opaResSelR2eq15R1   = DAC_OPA0MUX_RESSEL_RES7      /**< R2 = 15 * R1   */
#elif defined(_SILICON_LABS_32B_SERIES_1)
  opaResSelDefault    = VDAC_OPA_MUX_RESSEL_DEFAULT, /**< Default value when resistor ladder is unused. */
  opaResSelR2eq0_33R1 = VDAC_OPA_MUX_RESSEL_RES0,    /**< R2 = 0.33 * R1 */
  opaResSelR2eqR1     = VDAC_OPA_MUX_RESSEL_RES1,    /**< R2 = R1        */
  opaResSelR1eq1_67R1 = VDAC_OPA_MUX_RESSEL_RES2,    /**< R2 = 1.67 R1   */
  opaResSelR2eq2_2R1  = VDAC_OPA_MUX_RESSEL_RES3,    /**< R2 = 2.2 * R1    */
  opaResSelR2eq3R1    = VDAC_OPA_MUX_RESSEL_RES4,    /**< R2 = 3 * R1    */
  opaResSelR2eq4_33R1 = VDAC_OPA_MUX_RESSEL_RES5,    /**< R2 = 4.33 * R1 */
  opaResSelR2eq7R1    = VDAC_OPA_MUX_RESSEL_RES6,    /**< R2 = 7 * R1    */
  opaResSelR2eq15R1   = VDAC_OPA_MUX_RESSEL_RES7     /**< R2 = 15 * R1   */
#endif /* defined(_SILICON_LABS_32B_SERIES_0) */
} OPAMP_ResSel_TypeDef;

/** OPAMP resistor ladder input selector values. */
typedef enum
{
#if defined(_SILICON_LABS_32B_SERIES_0)
  opaResInMuxDisable = DAC_OPA0MUX_RESINMUX_DISABLE,   /**< Resistor ladder disabled. */
  opaResInMuxOpaIn   = DAC_OPA0MUX_RESINMUX_OPA0INP,   /**< Input from OPAx.          */
  opaResInMuxNegPad  = DAC_OPA0MUX_RESINMUX_NEGPAD,    /**< Input from negative pad.  */
  opaResInMuxPosPad  = DAC_OPA0MUX_RESINMUX_POSPAD,    /**< Input from positive pad.  */
  opaResInMuxVss     = DAC_OPA0MUX_RESINMUX_VSS        /**< Input connected to Vss.   */
#elif defined(_SILICON_LABS_32B_SERIES_1)
  opaResInMuxDisable = VDAC_OPA_MUX_RESINMUX_DISABLE,  /**< Resistor ladder disabled.                  */
  opaResInMuxOpaIn   = VDAC_OPA_MUX_RESINMUX_OPANEXT,  /**< Input from OPAx.                           */
  opaResInMuxNegPad  = VDAC_OPA_MUX_RESINMUX_NEGPAD,   /**< Input from negative pad.                   */
  opaResInMuxPosPad  = VDAC_OPA_MUX_RESINMUX_POSPAD,   /**< Input from positive pad.                   */
  opaResInMuxComPad  = VDAC_OPA_MUX_RESINMUX_COMPAD,   /**< Input from negative pad of OPA0.
                                                            Direct input to support common reference.  */
  opaResInMuxCenter  = VDAC_OPA_MUX_RESINMUX_CENTER,   /**< OPA0 and OPA1 Resmux connected to form fully
                                                            differential instrumentation amplifier.    */
  opaResInMuxVss     = VDAC_OPA_MUX_RESINMUX_VSS,      /**< Input connected to Vss.                    */
#endif /* defined(_SILICON_LABS_32B_SERIES_0) */
} OPAMP_ResInMux_TypeDef;

#if defined(_SILICON_LABS_32B_SERIES_1)
typedef enum
{
  opaPrsModeDefault = VDAC_OPA_CTRL_PRSMODE_DEFAULT,  /**< Default value when PRS is not the trigger.       */
  opaPrsModePulsed  = VDAC_OPA_CTRL_PRSMODE_PULSED,   /**< PRS trigger is a pulse that starts the OPAMP
                                                           warmup sequence. The end of the warmup sequence
                                                           is controlled by timeout settings in OPAxTIMER.  */
  opaPrsModeTimed   = VDAC_OPA_CTRL_PRSMODE_TIMED,    /**< PRS trigger is a pulse long enough to provide the
                                                           OPAMP warmup sequence. The end of the warmup
                                                           sequence is controlled by the edge of the pulse. */
} OPAMP_PrsMode_TypeDef;

typedef enum
{
  opaPrsSelDefault = VDAC_OPA_CTRL_PRSSEL_DEFAULT,  /**< Default value when PRS is not the trigger. */
  opaPrsSelCh0     = VDAC_OPA_CTRL_PRSSEL_PRSCH0,   /**< PRS channel 0 triggers OPAMP.              */
  opaPrsSelCh1     = VDAC_OPA_CTRL_PRSSEL_PRSCH1,   /**< PRS channel 1 triggers OPAMP.              */
  opaPrsSelCh2     = VDAC_OPA_CTRL_PRSSEL_PRSCH2,   /**< PRS channel 2 triggers OPAMP.              */
  opaPrsSelCh3     = VDAC_OPA_CTRL_PRSSEL_PRSCH3,   /**< PRS channel 3 triggers OPAMP.              */
  opaPrsSelCh4     = VDAC_OPA_CTRL_PRSSEL_PRSCH4,   /**< PRS channel 4 triggers OPAMP.              */
  opaPrsSelCh5     = VDAC_OPA_CTRL_PRSSEL_PRSCH5,   /**< PRS channel 5 triggers OPAMP.              */
  opaPrsSelCh6     = VDAC_OPA_CTRL_PRSSEL_PRSCH6,   /**< PRS channel 6 triggers OPAMP.              */
  opaPrsSelCh7     = VDAC_OPA_CTRL_PRSSEL_PRSCH7,   /**< PRS channel 7 triggers OPAMP.              */
  opaPrsSelCh8     = VDAC_OPA_CTRL_PRSSEL_PRSCH8,   /**< PRS channel 8 triggers OPAMP.              */
  opaPrsSelCh9     = VDAC_OPA_CTRL_PRSSEL_PRSCH9,   /**< PRS channel 9 triggers OPAMP.              */
  opaPrsSelCh10    = VDAC_OPA_CTRL_PRSSEL_PRSCH10,  /**< PRS channel 10 triggers OPAMP.             */
  opaPrsSelCh11    = VDAC_OPA_CTRL_PRSSEL_PRSCH11,  /**< PRS channel 11 triggers OPAMP.             */
} OPAMP_PrsSel_TypeDef;

typedef enum
{
  opaPrsOutDefault  = VDAC_OPA_CTRL_PRSOUTMODE_DEFAULT,   /**< Default value.                    */
  opaPrsOutWarm     = VDAC_OPA_CTRL_PRSOUTMODE_WARM,      /**< Warm status available on PRS.     */
  opaPrsOutOutValid = VDAC_OPA_CTRL_PRSOUTMODE_OUTVALID,  /**< Outvalid status available on PRS. */
} OPAMP_PrsOut_TypeDef;

typedef enum
{
  opaOutScaleDefault = VDAC_OPA_CTRL_OUTSCALE_DEFAULT,  /**< Default OPAM output drive strength.    */
  opaOutScaleFull    = VDAC_OPA_CTRL_OUTSCALE_FULL,     /**< OPAMP uses full output drive strength. */
  opaOutSacleHalf    = VDAC_OPA_CTRL_OUTSCALE_HALF,     /**< OPAMP uses half output drive strength. */
} OPAMP_OutScale_Typedef;

typedef enum
{
  opaDrvStrDefault          = VDAC_OPA_CTRL_DRIVESTRENGTH_DEFAULT,        /**< Default value.                           */
  opaDrvStrLowerAccLowStr   = (0 << _VDAC_OPA_CTRL_DRIVESTRENGTH_SHIFT),  /**< Lower accuracy with low drive stregth.   */
  opaDrvStrLowAccLowStr     = (1 << _VDAC_OPA_CTRL_DRIVESTRENGTH_SHIFT),  /**< Low accuracy with low drive stregth.     */
  opaDrvStrHighAccHighStr   = (2 << _VDAC_OPA_CTRL_DRIVESTRENGTH_SHIFT),  /**< High accuracy with high drive stregth.   */
  opaDrvStrHigherAccHighStr = (3 << _VDAC_OPA_CTRL_DRIVESTRENGTH_SHIFT),  /**< Higher accuracy with high drive stregth. */
} OPAMP_DrvStr_Typedef;
#endif /* defined(_SILICON_LABS_32B_SERIES_0) */

/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/

/** OPAMP init structure. */
typedef struct
{
  OPAMP_NegSel_TypeDef   negSel;              /**< Select input source for negative terminal.    */
  OPAMP_PosSel_TypeDef   posSel;              /**< Select input source for positive terminal.    */
  OPAMP_OutMode_TypeDef  outMode;             /**< Output terminal connection.                   */
  OPAMP_ResSel_TypeDef   resSel;              /**< Select R2/R1 resistor ratio.                  */
  OPAMP_ResInMux_TypeDef resInMux;            /**< Select input source for resistor ladder.      */
  uint32_t               outPen;              /**< Alternate output enable bit mask. This value
                                                 should consist of one or more of the
                                                 @if DOXYDOC_P1_DEVICE
                                                 DAC_OPA[opa#]MUX_OUTPEN_OUT[output#] flags
                                                 (defined in \<part_name\>_dac.h) OR'ed together.
                                                 @n @n
                                                 For OPA0:
                                                 @li DAC_OPA0MUX_OUTPEN_OUT0
                                                 @li DAC_OPA0MUX_OUTPEN_OUT1
                                                 @li DAC_OPA0MUX_OUTPEN_OUT2
                                                 @li DAC_OPA0MUX_OUTPEN_OUT3
                                                 @li DAC_OPA0MUX_OUTPEN_OUT4

                                                 For OPA1:
                                                 @li DAC_OPA1MUX_OUTPEN_OUT0
                                                 @li DAC_OPA1MUX_OUTPEN_OUT1
                                                 @li DAC_OPA1MUX_OUTPEN_OUT2
                                                 @li DAC_OPA1MUX_OUTPEN_OUT3
                                                 @li DAC_OPA1MUX_OUTPEN_OUT4

                                                 For OPA2:
                                                 @li DAC_OPA2MUX_OUTPEN_OUT0
                                                 @li DAC_OPA2MUX_OUTPEN_OUT1

                                                 E.g: @n
                                                 init.outPen = DAC_OPA0MUX_OUTPEN_OUT0 |
                                                 DAC_OPA0MUX_OUTPEN_OUT2 |
                                                 DAC_OPA0MUX_OUTPEN_OUT4;

                                                 @elseif DOXYDOC_P2_DEVICE
                                                 VDAC_OPA_OUT_ALTOUTPADEN_OUT[output#] flags
                                                 (defined in \<part_name\>_vdac.h) OR'ed together.
                                                 @n @n
                                                 @li VDAC_OPA_OUT_ALTOUTPADEN_OUT0
                                                 @li VDAC_OPA_OUT_ALTOUTPADEN_OUT1
                                                 @li VDAC_OPA_OUT_ALTOUTPADEN_OUT2
                                                 @li VDAC_OPA_OUT_ALTOUTPADEN_OUT3
                                                 @li VDAC_OPA_OUT_ALTOUTPADEN_OUT4

                                                 E.g: @n
                                                 init.outPen = VDAC_OPA_OUT_ALTOUTPADEN_OUT0 |
                                                 VDAC_OPA_OUT_ALTOUTPADEN_OUT2 |
                                                 VDAC_OPA_OUT_ALTOUTPADEN_OUT4;
                                                 @endif                                          */
#if defined(_SILICON_LABS_32B_SERIES_0)
  uint32_t               bias;                /**< Set OPAMP bias current.                       */
  bool                   halfBias;            /**< Divide OPAMP bias current by 2.               */
  bool                   lpfPosPadDisable;    /**< Disable low pass filter on positive pad.      */
  bool                   lpfNegPadDisable;    /**< Disable low pass filter on negative pad.      */
  bool                   nextOut;             /**< Enable NEXTOUT signal source.                 */
  bool                   npEn;                /**< Enable positive pad.                          */
  bool                   ppEn;                /**< Enable negative pad.                          */
  bool                   shortInputs;         /**< Short OPAMP input terminals.                  */
  bool                   hcmDisable;          /**< Disable input rail-to-rail capability.        */
  bool                   defaultOffset;       /**< Use factory calibrated opamp offset value.    */
  uint32_t               offset;              /**< Opamp offset value when @ref defaultOffset is
                                                   false.                                        */
#elif defined(_SILICON_LABS_32B_SERIES_1)
  OPAMP_DrvStr_Typedef   drvStr;              /**< OPAx operation mode.                          */
  bool                   gain3xEn;            /**< Enable 3x gain resistor ladder.               */
  bool                   halfDrvStr;          /**< Half or full output drive strength.           */
  bool                   ugBwScale;           /**< Unity gain bandwidth scaled by factor of 2.5. */
  bool                   prsEn;               /**< Enable PRS as OPAMP trigger.                  */
  OPAMP_PrsMode_TypeDef  prsMode;             /**< Selects PRS trigger mode.                     */
  OPAMP_PrsSel_TypeDef   prsSel;              /**< PRS channel trigger select.                   */
  OPAMP_PrsOut_TypeDef   prsOutSel;           /**< PRS output select.                            */
  bool                   aportYMasterDisable; /**< Disable bus master request on APORT Y.        */
  bool                   aportXMasterDisable; /**< Disable bus master request on APORT X.        */
  uint32_t               settleTime;          /**< Number of clock cycles to drive the output.   */
  uint32_t               startupDly;          /**< OPAx startup delay in us.                     */
  bool                   hcmDisable;          /**< Disable input rail-to-rail capability.        */
  bool                   defaultOffsetN;      /**< Use factory calibrated opamp inverting input
                                                   offset value.                                 */
  uint32_t               offsetN;             /**< Opamp inverting input offset value when
                                                   @ref defaultOffsetInv is false.               */
  bool                   defaultOffsetP;      /**< Use factory calibrated opamp non-inverting
                                                   input offset value.                           */
  uint32_t               offsetP;             /**< Opamp non-inverting input offset value when
                                                   @ref defaultOffsetNon is false.               */
#endif /* defined(_SILICON_LABS_32B_SERIES_1) */
} OPAMP_Init_TypeDef;

#if defined(_SILICON_LABS_32B_SERIES_0)
/** Configuration of OPA0/1 in unity gain voltage follower mode. */
#define OPA_INIT_UNITY_GAIN                                                     \
{                                                                               \
  opaNegSelUnityGain,             /* Unity gain.                             */ \
  opaPosSelPosPad,                /* Pos input from pad.                     */ \
  opaOutModeMain,                 /* Main output enabled.                    */ \
  opaResSelDefault,               /* Resistor ladder is not used.            */ \
  opaResInMuxDisable,             /* Resistor ladder disabled.               */ \
  0,                              /* No alternate outputs enabled.           */ \
  _DAC_BIASPROG_BIASPROG_DEFAULT, /* Default bias setting.                   */ \
  _DAC_BIASPROG_HALFBIAS_DEFAULT, /* Default half-bias setting.              */ \
  false,                          /* No low pass filter on pos pad.          */ \
  false,                          /* No low pass filter on neg pad.          */ \
  false,                          /* No nextout output enabled.              */ \
  false,                          /* Neg pad disabled.                       */ \
  true,                           /* Pos pad enabled, used as signal input.  */ \
  false,                          /* No shorting of inputs.                  */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use factory calibrated opamp offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA2 in unity gain voltage follower mode. */
#define OPA_INIT_UNITY_GAIN_OPA2                                                \
{                                                                               \
  opaNegSelUnityGain,             /* Unity gain.                             */ \
  opaPosSelPosPad,                /* Pos input from pad.                     */ \
  opaOutModeMain,                 /* Main output enabled.                    */ \
  opaResSelDefault,               /* Resistor ladder is not used.            */ \
  opaResInMuxDisable,             /* Resistor ladder disabled.               */ \
  DAC_OPA0MUX_OUTPEN_OUT0,        /* Alternate output 0 enabled.             */ \
  _DAC_BIASPROG_BIASPROG_DEFAULT, /* Default bias setting.                   */ \
  _DAC_BIASPROG_HALFBIAS_DEFAULT, /* Default half-bias setting.              */ \
  false,                          /* No low pass filter on pos pad.          */ \
  false,                          /* No low pass filter on neg pad.          */ \
  false,                          /* No nextout output enabled.              */ \
  false,                          /* Neg pad disabled.                       */ \
  true,                           /* Pos pad enabled, used as signal input.  */ \
  false,                          /* No shorting of inputs.                  */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use factory calibrated opamp offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA0/1 in non-inverting amplifier mode.           */
#define OPA_INIT_NON_INVERTING                                                  \
{                                                                               \
  opaNegSelResTap,                /* Neg input from resistor ladder tap.     */ \
  opaPosSelPosPad,                /* Pos input from pad.                     */ \
  opaOutModeMain,                 /* Main output enabled.                    */ \
  opaResSelR2eq0_33R1,            /* R2 = 1/3 R1                             */ \
  opaResInMuxNegPad,              /* Resistor ladder input from neg pad.     */ \
  0,                              /* No alternate outputs enabled.           */ \
  _DAC_BIASPROG_BIASPROG_DEFAULT, /* Default bias setting.                   */ \
  _DAC_BIASPROG_HALFBIAS_DEFAULT, /* Default half-bias setting.              */ \
  false,                          /* No low pass filter on pos pad.          */ \
  false,                          /* No low pass filter on neg pad.          */ \
  false,                          /* No nextout output enabled.              */ \
  true,                           /* Neg pad enabled, used as signal ground. */ \
  true,                           /* Pos pad enabled, used as signal input.  */ \
  false,                          /* No shorting of inputs.                  */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use factory calibrated opamp offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA2 in non-inverting amplifier mode. */
#define OPA_INIT_NON_INVERTING_OPA2                                             \
{                                                                               \
  opaNegSelResTap,                /* Neg input from resistor ladder tap.     */ \
  opaPosSelPosPad,                /* Pos input from pad.                     */ \
  opaOutModeMain,                 /* Main output enabled.                    */ \
  opaResSelR2eq0_33R1,            /* R2 = 1/3 R1                             */ \
  opaResInMuxNegPad,              /* Resistor ladder input from neg pad.     */ \
  DAC_OPA0MUX_OUTPEN_OUT0,        /* Alternate output 0 enabled.             */ \
  _DAC_BIASPROG_BIASPROG_DEFAULT, /* Default bias setting.                   */ \
  _DAC_BIASPROG_HALFBIAS_DEFAULT, /* Default half-bias setting.              */ \
  false,                          /* No low pass filter on pos pad.          */ \
  false,                          /* No low pass filter on neg pad.          */ \
  false,                          /* No nextout output enabled.              */ \
  true,                           /* Neg pad enabled, used as signal ground. */ \
  true,                           /* Pos pad enabled, used as signal input.  */ \
  false,                          /* No shorting of inputs.                  */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use factory calibrated opamp offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA0/1 in inverting amplifier mode. */
#define OPA_INIT_INVERTING                                                      \
{                                                                               \
  opaNegSelResTap,                /* Neg input from resistor ladder tap.     */ \
  opaPosSelPosPad,                /* Pos input from pad.                     */ \
  opaOutModeMain,                 /* Main output enabled.                    */ \
  opaResSelR2eqR1,                /* R2 = R1                                 */ \
  opaResInMuxNegPad,              /* Resistor ladder input from neg pad.     */ \
  0,                              /* No alternate outputs enabled.           */ \
  _DAC_BIASPROG_BIASPROG_DEFAULT, /* Default bias setting.                   */ \
  _DAC_BIASPROG_HALFBIAS_DEFAULT, /* Default half-bias setting.              */ \
  false,                          /* No low pass filter on pos pad.          */ \
  false,                          /* No low pass filter on neg pad.          */ \
  false,                          /* No nextout output enabled.              */ \
  true,                           /* Neg pad enabled, used as signal input.  */ \
  true,                           /* Pos pad enabled, used as signal ground. */ \
  false,                          /* No shorting of inputs.                  */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use factory calibrated opamp offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA2 in inverting amplifier mode. */
#define OPA_INIT_INVERTING_OPA2                                                 \
{                                                                               \
  opaNegSelResTap,                /* Neg input from resistor ladder tap.     */ \
  opaPosSelPosPad,                /* Pos input from pad.                     */ \
  opaOutModeMain,                 /* Main output enabled.                    */ \
  opaResSelR2eqR1,                /* R2 = R1                                 */ \
  opaResInMuxNegPad,              /* Resistor ladder input from neg pad.     */ \
  DAC_OPA0MUX_OUTPEN_OUT0,        /* Alternate output 0 enabled.             */ \
  _DAC_BIASPROG_BIASPROG_DEFAULT, /* Default bias setting.                   */ \
  _DAC_BIASPROG_HALFBIAS_DEFAULT, /* Default half-bias setting.              */ \
  false,                          /* No low pass filter on pos pad.          */ \
  false,                          /* No low pass filter on neg pad.          */ \
  false,                          /* No nextout output enabled.              */ \
  true,                           /* Neg pad enabled, used as signal input.  */ \
  true,                           /* Pos pad enabled, used as signal ground. */ \
  false,                          /* No shorting of inputs.                  */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use factory calibrated opamp offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA0 in cascaded non-inverting amplifier mode. */
#define OPA_INIT_CASCADED_NON_INVERTING_OPA0                                    \
{                                                                               \
  opaNegSelResTap,                /* Neg input from resistor ladder tap.     */ \
  opaPosSelPosPad,                /* Pos input from pad.                     */ \
  opaOutModeAll,                  /* Both main and alternate outputs.        */ \
  opaResSelR2eq0_33R1,            /* R2 = 1/3 R1                             */ \
  opaResInMuxNegPad,              /* Resistor ladder input from neg pad.     */ \
  0,                              /* No alternate outputs enabled.           */ \
  _DAC_BIASPROG_BIASPROG_DEFAULT, /* Default bias setting.                   */ \
  _DAC_BIASPROG_HALFBIAS_DEFAULT, /* Default half-bias setting.              */ \
  false,                          /* No low pass filter on pos pad.          */ \
  false,                          /* No low pass filter on neg pad.          */ \
  true,                           /* Pass output to next stage (OPA1).       */ \
  true,                           /* Neg pad enabled, used as signal ground. */ \
  true,                           /* Pos pad enabled, used as signal input.  */ \
  false,                          /* No shorting of inputs.                  */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use factory calibrated opamp offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA1 in cascaded non-inverting amplifier mode. */
#define OPA_INIT_CASCADED_NON_INVERTING_OPA1                                    \
{                                                                               \
  opaNegSelResTap,                /* Neg input from resistor ladder tap.     */ \
  opaPosSelOpaIn,                 /* Pos input from OPA0 output.             */ \
  opaOutModeAll,                  /* Both main and alternate outputs.        */ \
  opaResSelR2eq0_33R1,            /* R2 = 1/3 R1                             */ \
  opaResInMuxNegPad,              /* Resistor ladder input from neg pad.     */ \
  0,                              /* No alternate outputs enabled.           */ \
  _DAC_BIASPROG_BIASPROG_DEFAULT, /* Default bias setting.                   */ \
  _DAC_BIASPROG_HALFBIAS_DEFAULT, /* Default half-bias setting.              */ \
  false,                          /* No low pass filter on pos pad.          */ \
  false,                          /* No low pass filter on neg pad.          */ \
  true,                           /* Pass output to next stage (OPA2).       */ \
  true,                           /* Neg pad enabled, used as signal ground. */ \
  false,                          /* Pos pad disabled.                       */ \
  false,                          /* No shorting of inputs.                  */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use factory calibrated opamp offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA2 in cascaded non-inverting amplifier mode. */
#define OPA_INIT_CASCADED_NON_INVERTING_OPA2                                    \
{                                                                               \
  opaNegSelResTap,                /* Neg input from resistor ladder tap.     */ \
  opaPosSelOpaIn,                 /* Pos input from OPA1 output.             */ \
  opaOutModeMain,                 /* Main output enabled.                    */ \
  opaResSelR2eq0_33R1,            /* R2 = 1/3 R1                             */ \
  opaResInMuxNegPad,              /* Resistor ladder input from neg pad.     */ \
  DAC_OPA0MUX_OUTPEN_OUT0,        /* Alternate output 0 enabled.             */ \
  _DAC_BIASPROG_BIASPROG_DEFAULT, /* Default bias setting.                   */ \
  _DAC_BIASPROG_HALFBIAS_DEFAULT, /* Default half-bias setting.              */ \
  false,                          /* No low pass filter on pos pad.          */ \
  false,                          /* No low pass filter on neg pad.          */ \
  false,                          /* No nextout output enabled.              */ \
  true,                           /* Neg pad enabled, used as signal ground. */ \
  false,                          /* Pos pad disabled.                       */ \
  false,                          /* No shorting of inputs.                  */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use factory calibrated opamp offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA0 in cascaded inverting amplifier mode. */
#define OPA_INIT_CASCADED_INVERTING_OPA0                                        \
{                                                                               \
  opaNegSelResTap,                /* Neg input from resistor ladder tap.     */ \
  opaPosSelPosPad,                /* Pos input from pad.                     */ \
  opaOutModeAll,                  /* Both main and alternate outputs.        */ \
  opaResSelR2eqR1,                /* R2 = R1                                 */ \
  opaResInMuxNegPad,              /* Resistor ladder input from neg pad.     */ \
  0,                              /* No alternate outputs enabled.           */ \
  _DAC_BIASPROG_BIASPROG_DEFAULT, /* Default bias setting.                   */ \
  _DAC_BIASPROG_HALFBIAS_DEFAULT, /* Default half-bias setting.              */ \
  false,                          /* No low pass filter on pos pad.          */ \
  false,                          /* No low pass filter on neg pad.          */ \
  true,                           /* Pass output to next stage (OPA1).       */ \
  true,                           /* Neg pad enabled, used as signal input.  */ \
  true,                           /* Pos pad enabled, used as signal ground. */ \
  false,                          /* No shorting of inputs.                  */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use factory calibrated opamp offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA1 in cascaded inverting amplifier mode. */
#define OPA_INIT_CASCADED_INVERTING_OPA1                                        \
{                                                                               \
  opaNegSelResTap,                /* Neg input from resistor ladder tap.     */ \
  opaPosSelPosPad,                /* Pos input from pad.                     */ \
  opaOutModeAll,                  /* Both main and alternate outputs.        */ \
  opaResSelR2eqR1,                /* R2 = R1                                 */ \
  opaResInMuxOpaIn,               /* Resistor ladder input from OPA0.        */ \
  0,                              /* No alternate outputs enabled.           */ \
  _DAC_BIASPROG_BIASPROG_DEFAULT, /* Default bias setting.                   */ \
  _DAC_BIASPROG_HALFBIAS_DEFAULT, /* Default half-bias setting.              */ \
  false,                          /* No low pass filter on pos pad.          */ \
  false,                          /* No low pass filter on neg pad.          */ \
  true,                           /* Pass output to next stage (OPA2).       */ \
  false,                          /* Neg pad disabled.                       */ \
  true,                           /* Pos pad enabled, used as signal ground. */ \
  false,                          /* No shorting of inputs.                  */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use factory calibrated opamp offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA2 in cascaded inverting amplifier mode. */
#define OPA_INIT_CASCADED_INVERTING_OPA2                                        \
{                                                                               \
  opaNegSelResTap,                /* Neg input from resistor ladder tap.     */ \
  opaPosSelPosPad,                /* Pos input from pad.                     */ \
  opaOutModeMain,                 /* Main output enabled.                    */ \
  opaResSelR2eqR1,                /* R2 = R1                                 */ \
  opaResInMuxOpaIn,               /* Resistor ladder input from OPA1.        */ \
  DAC_OPA0MUX_OUTPEN_OUT0,        /* Alternate output 0 enabled.             */ \
  _DAC_BIASPROG_BIASPROG_DEFAULT, /* Default bias setting.                   */ \
  _DAC_BIASPROG_HALFBIAS_DEFAULT, /* Default half-bias setting.              */ \
  false,                          /* No low pass filter on pos pad.          */ \
  false,                          /* No low pass filter on neg pad.          */ \
  false,                          /* No nextout output enabled.              */ \
  false,                          /* Neg pad disabled.                       */ \
  true,                           /* Pos pad enabled, used as signal ground. */ \
  false,                          /* No shorting of inputs.                  */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use factory calibrated opamp offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA0 in two-opamp differential driver mode. */
#define OPA_INIT_DIFF_DRIVER_OPA0                                               \
{                                                                               \
  opaNegSelUnityGain,             /* Unity gain.                             */ \
  opaPosSelPosPad,                /* Pos input from pad.                     */ \
  opaOutModeAll,                  /* Both main and alternate outputs.        */ \
  opaResSelDefault,               /* Resistor ladder is not used.            */ \
  opaResInMuxDisable,             /* Resistor ladder disabled.               */ \
  0,                              /* No alternate outputs enabled.           */ \
  _DAC_BIASPROG_BIASPROG_DEFAULT, /* Default bias setting.                   */ \
  _DAC_BIASPROG_HALFBIAS_DEFAULT, /* Default half-bias setting.              */ \
  false,                          /* No low pass filter on pos pad.          */ \
  false,                          /* No low pass filter on neg pad.          */ \
  true,                           /* Pass output to next stage (OPA1).       */ \
  false,                          /* Neg pad disabled.                       */ \
  true,                           /* Pos pad enabled, used as signal input.  */ \
  false,                          /* No shorting of inputs.                  */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use factory calibrated opamp offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA1 in two-opamp differential driver mode. */
#define OPA_INIT_DIFF_DRIVER_OPA1                                               \
{                                                                               \
  opaNegSelResTap,                /* Neg input from resistor ladder tap.     */ \
  opaPosSelPosPad,                /* Pos input from pad.                     */ \
  opaOutModeMain,                 /* Main output enabled.                    */ \
  opaResSelR2eqR1,                /* R2 = R1                                 */ \
  opaResInMuxOpaIn,               /* Resistor ladder input from OPA0.        */ \
  0,                              /* No alternate outputs enabled.           */ \
  _DAC_BIASPROG_BIASPROG_DEFAULT, /* Default bias setting.                   */ \
  _DAC_BIASPROG_HALFBIAS_DEFAULT, /* Default half-bias setting.              */ \
  false,                          /* No low pass filter on pos pad.          */ \
  false,                          /* No low pass filter on neg pad.          */ \
  false,                          /* No nextout output enabled.              */ \
  false,                          /* Neg pad disabled.                       */ \
  true,                           /* Pos pad enabled, used as signal ground. */ \
  false,                          /* No shorting of inputs.                  */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use factory calibrated opamp offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA0 in three-opamp differential receiver mode. */
#define OPA_INIT_DIFF_RECEIVER_OPA0                                             \
{                                                                               \
  opaNegSelUnityGain,             /* Unity gain.                             */ \
  opaPosSelPosPad,                /* Pos input from pad.                     */ \
  opaOutModeAll,                  /* Both main and alternate outputs.        */ \
  opaResSelR2eqR1,                /* R2 = R1                                 */ \
  opaResInMuxNegPad,              /* Resistor ladder input from neg pad.     */ \
  0,                              /* No alternate outputs enabled.           */ \
  _DAC_BIASPROG_BIASPROG_DEFAULT, /* Default bias setting.                   */ \
  _DAC_BIASPROG_HALFBIAS_DEFAULT, /* Default half-bias setting.              */ \
  false,                          /* No low pass filter on pos pad.          */ \
  false,                          /* No low pass filter on neg pad.          */ \
  true,                           /* Pass output to next stage (OPA2).       */ \
  true,                           /* Neg pad enabled, used as signal ground. */ \
  true,                           /* Pos pad enabled, used as signal input.  */ \
  false,                          /* No shorting of inputs.                  */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use factory calibrated opamp offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA1 in three-opamp differential receiver mode. */
#define OPA_INIT_DIFF_RECEIVER_OPA1                                             \
{                                                                               \
  opaNegSelUnityGain,             /* Unity gain.                             */ \
  opaPosSelPosPad,                /* Pos input from pad.                     */ \
  opaOutModeAll,                  /* Both main and alternate outputs.        */ \
  opaResSelDefault,               /* Resistor ladder is not used.            */ \
  opaResInMuxDisable,             /* Disable resistor ladder.                */ \
  0,                              /* No alternate outputs enabled.           */ \
  _DAC_BIASPROG_BIASPROG_DEFAULT, /* Default bias setting.                   */ \
  _DAC_BIASPROG_HALFBIAS_DEFAULT, /* Default half-bias setting.              */ \
  false,                          /* No low pass filter on pos pad.          */ \
  false,                          /* No low pass filter on neg pad.          */ \
  true,                           /* Pass output to next stage (OPA2).       */ \
  false,                          /* Neg pad disabled.                       */ \
  true,                           /* Pos pad enabled, used as signal input.  */ \
  false,                          /* No shorting of inputs.                  */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use factory calibrated opamp offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA2 in three-opamp differential receiver mode. */
#define OPA_INIT_DIFF_RECEIVER_OPA2                                             \
{                                                                               \
  opaNegSelResTap,                /* Input from resistor ladder tap.         */ \
  opaPosSelResTapOpa0,            /* Input from OPA0 resistor ladder tap.    */ \
  opaOutModeMain,                 /* Main output enabled.                    */ \
  opaResSelR2eqR1,                /* R2 = R1                                 */ \
  opaResInMuxOpaIn,               /* Resistor ladder input from OPA1.        */ \
  DAC_OPA0MUX_OUTPEN_OUT0,        /* Enable alternate output 0.              */ \
  _DAC_BIASPROG_BIASPROG_DEFAULT, /* Default bias setting.                   */ \
  _DAC_BIASPROG_HALFBIAS_DEFAULT, /* Default half-bias setting.              */ \
  false,                          /* No low pass filter on pos pad.          */ \
  false,                          /* No low pass filter on neg pad.          */ \
  false,                          /* No nextout output enabled.              */ \
  false,                          /* Neg pad disabled.                       */ \
  false,                          /* Pos pad disabled.                       */ \
  false,                          /* No shorting of inputs.                  */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use factory calibrated opamp offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

#elif defined(_SILICON_LABS_32B_SERIES_1)
/** Configuration of OPA in unity gain voltage follower mode. */
#define OPA_INIT_UNITY_GAIN                                                     \
{                                                                               \
  opaNegSelUnityGain,             /* Unity gain.                             */ \
  opaPosSelPosPad,                /* Pos input from pad.                     */ \
  opaOutModeMain,                 /* Main output enabled.                    */ \
  opaResSelDefault,               /* Resistor ladder is not used.            */ \
  opaResInMuxDisable,             /* Resistor ladder disabled.               */ \
  0,                              /* No alternate outputs enabled.           */ \
  opaDrvStrDefault,               /* Default opamp operation mode.           */ \
  false,                          /* Disable 3x gain setting.                */ \
  false,                          /* Use full output drive strength.         */ \
  false,                          /* Disable unity-gain bandwidth scaling.   */ \
  false,                          /* Opamp triggered by OPAxEN.              */ \
  opaPrsModeDefault,              /* PRS is not used to trigger opamp.       */ \
  opaPrsSelDefault,               /* PRS is not used to trigger opamp.       */ \
  opaPrsOutDefault,               /* Default PRS output setting.             */ \
  false,                          /* Bus mastering enabled on APORTX.        */ \
  false,                          /* Bus mastering enabled on APORTY.        */ \
  3,                              /* 3us settle time with default DrvStr.    */ \
  0,                              /* No startup delay.                       */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use calibrated inverting offset.        */ \
  0,                              /* Opamp offset value (not used).          */ \
  true,                           /* Use calibrated non-inverting offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA in non-inverting amplifier mode.           */
#define OPA_INIT_NON_INVERTING                                                  \
{                                                                               \
  opaNegSelResTap,                /* Neg input from resistor ladder tap.     */ \
  opaPosSelPosPad,                /* Pos input from pad.                     */ \
  opaOutModeMain,                 /* Main output enabled.                    */ \
  opaResSelR2eq0_33R1,            /* R2 = 1/3 R1                             */ \
  opaResInMuxNegPad,              /* Resistor ladder input from neg pad.     */ \
  0,                              /* No alternate outputs enabled.           */ \
  opaDrvStrDefault,               /* Default opamp operation mode.           */ \
  false,                          /* Disable 3x gain setting.                */ \
  false,                          /* Use full output drive strength.         */ \
  false,                          /* Disable unity-gain bandwidth scaling.   */ \
  false,                          /* Opamp triggered by OPAxEN.              */ \
  opaPrsModeDefault,              /* PRS is not used to trigger opamp.       */ \
  opaPrsSelDefault,               /* PRS is not used to trigger opamp.       */ \
  opaPrsOutDefault,               /* Default PRS output setting.             */ \
  false,                          /* Bus mastering enabled on APORTX.        */ \
  false,                          /* Bus mastering enabled on APORTY.        */ \
  3,                              /* 3us settle time with default DrvStr.    */ \
  0,                              /* No startup delay.                       */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use calibrated inverting offset.        */ \
  0,                              /* Opamp offset value (not used).          */ \
  true,                           /* Use calibrated non-inverting offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA in inverting amplifier mode. */
#define OPA_INIT_INVERTING                                                      \
{                                                                               \
  opaNegSelResTap,                /* Neg input from resistor ladder tap.     */ \
  opaPosSelPosPad,                /* Pos input from pad.                     */ \
  opaOutModeMain,                 /* Main output enabled.                    */ \
  opaResSelR2eqR1,                /* R2 = R1                                 */ \
  opaResInMuxNegPad,              /* Resistor ladder input from neg pad.     */ \
  0,                              /* No alternate outputs enabled.           */ \
  opaDrvStrDefault,               /* Default opamp operation mode.           */ \
  false,                          /* Disable 3x gain setting.                */ \
  false,                          /* Use full output drive strength.         */ \
  false,                          /* Disable unity-gain bandwidth scaling.   */ \
  false,                          /* Opamp triggered by OPAxEN.              */ \
  opaPrsModeDefault,              /* PRS is not used to trigger opamp.       */ \
  opaPrsSelDefault,               /* PRS is not used to trigger opamp.       */ \
  opaPrsOutDefault,               /* Default PRS output setting.             */ \
  false,                          /* Bus mastering enabled on APORTX.        */ \
  false,                          /* Bus mastering enabled on APORTY.        */ \
  3,                              /* 3us settle time with default DrvStr.    */ \
  0,                              /* No startup delay.                       */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use calibrated inverting offset.        */ \
  0,                              /* Opamp offset value (not used).          */ \
  true,                           /* Use calibrated non-inverting offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA0 in cascaded non-inverting amplifier mode. */
#define OPA_INIT_CASCADED_NON_INVERTING_OPA0                                    \
{                                                                               \
  opaNegSelResTap,                /* Neg input from resistor ladder tap.     */ \
  opaPosSelPosPad,                /* Pos input from pad.                     */ \
  opaOutModeMain,                 /* Main output enabled.                    */ \
  opaResSelR2eq0_33R1,            /* R2 = 1/3 R1                             */ \
  opaResInMuxNegPad,              /* Resistor ladder input from neg pad.     */ \
  0,                              /* No alternate outputs enabled.           */ \
  opaDrvStrDefault,               /* Default opamp operation mode.           */ \
  false,                          /* Disable 3x gain setting.                */ \
  false,                          /* Use full output drive strength.         */ \
  false,                          /* Disable unity-gain bandwidth scaling.   */ \
  false,                          /* Opamp triggered by OPAxEN.              */ \
  opaPrsModeDefault,              /* PRS is not used to trigger opamp.       */ \
  opaPrsSelDefault,               /* PRS is not used to trigger opamp.       */ \
  opaPrsOutDefault,               /* Default PRS output setting.             */ \
  false,                          /* Bus mastering enabled on APORTX.        */ \
  false,                          /* Bus mastering enabled on APORTY.        */ \
  3,                              /* 3us settle time with default DrvStr.    */ \
  0,                              /* No startup delay.                       */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use calibrated inverting offset.        */ \
  0,                              /* Opamp offset value (not used).          */ \
  true,                           /* Use calibrated non-inverting offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA1 in cascaded non-inverting amplifier mode. */
#define OPA_INIT_CASCADED_NON_INVERTING_OPA1                                    \
{                                                                               \
  opaNegSelResTap,                /* Neg input from resistor ladder tap.     */ \
  opaPosSelOpaIn,                 /* Pos input from OPA0 output.             */ \
  opaOutModeMain,                 /* Main output enabled.                    */ \
  opaResSelR2eq0_33R1,            /* R2 = 1/3 R1                             */ \
  opaResInMuxNegPad,              /* Resistor ladder input from neg pad.     */ \
  0,                              /* No alternate outputs enabled.           */ \
  opaDrvStrDefault,               /* Default opamp operation mode.           */ \
  false,                          /* Disable 3x gain setting.                */ \
  false,                          /* Use full output drive strength.         */ \
  false,                          /* Disable unity-gain bandwidth scaling.   */ \
  false,                          /* Opamp triggered by OPAxEN.              */ \
  opaPrsModeDefault,              /* PRS is not used to trigger opamp.       */ \
  opaPrsSelDefault,               /* PRS is not used to trigger opamp.       */ \
  opaPrsOutDefault,               /* Default PRS output setting.             */ \
  false,                          /* Bus mastering enabled on APORTX.        */ \
  false,                          /* Bus mastering enabled on APORTY.        */ \
  3,                              /* 3us settle time with default DrvStr.    */ \
  0,                              /* No startup delay.                       */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use calibrated inverting offset.        */ \
  0,                              /* Opamp offset value (not used).          */ \
  true,                           /* Use calibrated non-inverting offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA2 in cascaded non-inverting amplifier mode. */
#define OPA_INIT_CASCADED_NON_INVERTING_OPA2                                    \
{                                                                               \
  opaNegSelResTap,                /* Neg input from resistor ladder tap.     */ \
  opaPosSelOpaIn,                 /* Pos input from OPA1 output.             */ \
  opaOutModeMain,                 /* Main output enabled.                    */ \
  opaResSelR2eq0_33R1,            /* R2 = 1/3 R1                             */ \
  opaResInMuxNegPad,              /* Resistor ladder input from neg pad.     */ \
  0,                              /* No alternate outputs enabled.           */ \
  opaDrvStrDefault,               /* Default opamp operation mode.           */ \
  false,                          /* Disable 3x gain setting.                */ \
  false,                          /* Use full output drive strength.         */ \
  false,                          /* Disable unity-gain bandwidth scaling.   */ \
  false,                          /* Opamp triggered by OPAxEN.              */ \
  opaPrsModeDefault,              /* PRS is not used to trigger opamp.       */ \
  opaPrsSelDefault,               /* PRS is not used to trigger opamp.       */ \
  opaPrsOutDefault,               /* Default PRS output setting.             */ \
  false,                          /* Bus mastering enabled on APORTX.        */ \
  false,                          /* Bus mastering enabled on APORTY.        */ \
  3,                              /* 3us settle time with default DrvStr.    */ \
  0,                              /* No startup delay.                       */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use calibrated inverting offset.        */ \
  0,                              /* Opamp offset value (not used).          */ \
  true,                           /* Use calibrated non-inverting offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA0 in cascaded inverting amplifier mode. */
#define OPA_INIT_CASCADED_INVERTING_OPA0                                        \
{                                                                               \
  opaNegSelResTap,                /* Neg input from resistor ladder tap.     */ \
  opaPosSelPosPad,                /* Pos input from pad.                     */ \
  opaOutModeMain,                 /* Main output enabled.                    */ \
  opaResSelR2eqR1,                /* R2 = R1                                 */ \
  opaResInMuxNegPad,              /* Resistor ladder input from neg pad.     */ \
  0,                              /* No alternate outputs enabled.           */ \
  opaDrvStrDefault,               /* Default opamp operation mode.           */ \
  false,                          /* Disable 3x gain setting.                */ \
  false,                          /* Use full output drive strength.         */ \
  false,                          /* Disable unity-gain bandwidth scaling.   */ \
  false,                          /* Opamp triggered by OPAxEN.              */ \
  opaPrsModeDefault,              /* PRS is not used to trigger opamp.       */ \
  opaPrsSelDefault,               /* PRS is not used to trigger opamp.       */ \
  opaPrsOutDefault,               /* Default PRS output setting.             */ \
  false,                          /* Bus mastering enabled on APORTX.        */ \
  false,                          /* Bus mastering enabled on APORTY.        */ \
  3,                              /* 3us settle time with default DrvStr.    */ \
  0,                              /* No startup delay.                       */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use calibrated inverting offset.        */ \
  0,                              /* Opamp offset value (not used).          */ \
  true,                           /* Use calibrated non-inverting offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA1 in cascaded inverting amplifier mode. */
#define OPA_INIT_CASCADED_INVERTING_OPA1                                        \
{                                                                               \
  opaNegSelResTap,                /* Neg input from resistor ladder tap.     */ \
  opaPosSelPosPad,                /* Pos input from pad.                     */ \
  opaOutModeMain,                 /* Main output enabled.                    */ \
  opaResSelR2eqR1,                /* R2 = R1                                 */ \
  opaResInMuxOpaIn,               /* Resistor ladder input from OPA0.        */ \
  0,                              /* No alternate outputs enabled.           */ \
  opaDrvStrDefault,               /* Default opamp operation mode.           */ \
  false,                          /* Disable 3x gain setting.                */ \
  false,                          /* Use full output drive strength.         */ \
  false,                          /* Disable unity-gain bandwidth scaling.   */ \
  false,                          /* Opamp triggered by OPAxEN.              */ \
  opaPrsModeDefault,              /* PRS is not used to trigger opamp.       */ \
  opaPrsSelDefault,               /* PRS is not used to trigger opamp.       */ \
  opaPrsOutDefault,               /* Default PRS output setting.             */ \
  false,                          /* Bus mastering enabled on APORTX.        */ \
  false,                          /* Bus mastering enabled on APORTY.        */ \
  3,                              /* 3us settle time with default DrvStr.    */ \
  0,                              /* No startup delay.                       */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use calibrated inverting offset.        */ \
  0,                              /* Opamp offset value (not used).          */ \
  true,                           /* Use calibrated non-inverting offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA2 in cascaded inverting amplifier mode. */
#define OPA_INIT_CASCADED_INVERTING_OPA2                                        \
{                                                                               \
  opaNegSelResTap,                /* Neg input from resistor ladder tap.     */ \
  opaPosSelPosPad,                /* Pos input from pad.                     */ \
  opaOutModeMain,                 /* Main output enabled.                    */ \
  opaResSelR2eqR1,                /* R2 = R1                                 */ \
  opaResInMuxOpaIn,               /* Resistor ladder input from OPA1.        */ \
  0,                              /* No alternate outputs enabled.           */ \
  opaDrvStrDefault,               /* Default opamp operation mode.           */ \
  false,                          /* Disable 3x gain setting.                */ \
  false,                          /* Use full output drive strength.         */ \
  false,                          /* Disable unity-gain bandwidth scaling.   */ \
  false,                          /* Opamp triggered by OPAxEN.              */ \
  opaPrsModeDefault,              /* PRS is not used to trigger opamp.       */ \
  opaPrsSelDefault,               /* PRS is not used to trigger opamp.       */ \
  opaPrsOutDefault,               /* Default PRS output setting.             */ \
  false,                          /* Bus mastering enabled on APORTX.        */ \
  false,                          /* Bus mastering enabled on APORTY.        */ \
  3,                              /* 3us settle time with default DrvStr.    */ \
  0,                              /* No startup delay.                       */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use calibrated inverting offset.        */ \
  0,                              /* Opamp offset value (not used).          */ \
  true,                           /* Use calibrated non-inverting offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA0 in two-opamp differential driver mode. */
#define OPA_INIT_DIFF_DRIVER_OPA0                                               \
{                                                                               \
  opaNegSelUnityGain,             /* Unity gain.                             */ \
  opaPosSelPosPad,                /* Pos input from pad.                     */ \
  opaOutModeMain,                 /* Main output enabled.                    */ \
  opaResSelDefault,               /* Resistor ladder is not used.            */ \
  opaResInMuxDisable,             /* Resistor ladder disabled.               */ \
  0,                              /* No alternate outputs enabled.           */ \
  opaDrvStrDefault,               /* Default opamp operation mode.           */ \
  false,                          /* Disable 3x gain setting.                */ \
  false,                          /* Use full output drive strength.         */ \
  false,                          /* Disable unity-gain bandwidth scaling.   */ \
  false,                          /* Opamp triggered by OPAxEN.              */ \
  opaPrsModeDefault,              /* PRS is not used to trigger opamp.       */ \
  opaPrsSelDefault,               /* PRS is not used to trigger opamp.       */ \
  opaPrsOutDefault,               /* Default PRS output setting.             */ \
  false,                          /* Bus mastering enabled on APORTX.        */ \
  false,                          /* Bus mastering enabled on APORTY.        */ \
  3,                              /* 3us settle time with default DrvStr.    */ \
  0,                              /* No startup delay.                       */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use calibrated inverting offset.        */ \
  0,                              /* Opamp offset value (not used).          */ \
  true,                           /* Use calibrated non-inverting offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA1 in two-opamp differential driver mode. */
#define OPA_INIT_DIFF_DRIVER_OPA1                                               \
{                                                                               \
  opaNegSelResTap,                /* Neg input from resistor ladder tap.     */ \
  opaPosSelPosPad,                /* Pos input from pad.                     */ \
  opaOutModeMain,                 /* Main output enabled.                    */ \
  opaResSelR2eqR1,                /* R2 = R1                                 */ \
  opaResInMuxOpaIn,               /* Resistor ladder input from OPA0.        */ \
  0,                              /* No alternate outputs enabled.           */ \
  opaDrvStrDefault,               /* Default opamp operation mode.           */ \
  false,                          /* Disable 3x gain setting.                */ \
  false,                          /* Use full output drive strength.         */ \
  false,                          /* Disable unity-gain bandwidth scaling.   */ \
  false,                          /* Opamp triggered by OPAxEN.              */ \
  opaPrsModeDefault,              /* PRS is not used to trigger opamp.       */ \
  opaPrsSelDefault,               /* PRS is not used to trigger opamp.       */ \
  opaPrsOutDefault,               /* Default PRS output setting.             */ \
  false,                          /* Bus mastering enabled on APORTX.        */ \
  false,                          /* Bus mastering enabled on APORTY.        */ \
  3,                              /* 3us settle time with default DrvStr.    */ \
  0,                              /* No startup delay.                       */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use calibrated inverting offset.        */ \
  0,                              /* Opamp offset value (not used).          */ \
  true,                           /* Use calibrated non-inverting offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA0 in three-opamp differential receiver mode. */
#define OPA_INIT_DIFF_RECEIVER_OPA0                                             \
{                                                                               \
  opaNegSelUnityGain,             /* Unity gain.                             */ \
  opaPosSelPosPad,                /* Pos input from pad.                     */ \
  opaOutModeMain,                 /* Main output enabled.                    */ \
  opaResSelR2eqR1,                /* R2 = R1                                 */ \
  opaResInMuxNegPad,              /* Resistor ladder input from neg pad.     */ \
  0,                              /* No alternate outputs enabled.           */ \
  opaDrvStrDefault,               /* Default opamp operation mode.           */ \
  false,                          /* Disable 3x gain setting.                */ \
  false,                          /* Use full output drive strength.         */ \
  false,                          /* Disable unity-gain bandwidth scaling.   */ \
  false,                          /* Opamp triggered by OPAxEN.              */ \
  opaPrsModeDefault,              /* PRS is not used to trigger opamp.       */ \
  opaPrsSelDefault,               /* PRS is not used to trigger opamp.       */ \
  opaPrsOutDefault,               /* Default PRS output setting.             */ \
  false,                          /* Bus mastering enabled on APORTX.        */ \
  false,                          /* Bus mastering enabled on APORTY.        */ \
  3,                              /* 3us settle time with default DrvStr.    */ \
  0,                              /* No startup delay.                       */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use calibrated inverting offset.        */ \
  0,                              /* Opamp offset value (not used).          */ \
  true,                           /* Use calibrated non-inverting offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA1 in three-opamp differential receiver mode. */
#define OPA_INIT_DIFF_RECEIVER_OPA1                                             \
{                                                                               \
  opaNegSelUnityGain,             /* Unity gain.                             */ \
  opaPosSelPosPad,                /* Pos input from pad.                     */ \
  opaOutModeMain,                 /* Main output enabled.                    */ \
  opaResSelDefault,               /* Resistor ladder is not used.            */ \
  opaResInMuxDisable,             /* Disable resistor ladder.                */ \
  0,                              /* No alternate outputs enabled.           */ \
  opaDrvStrDefault,               /* Default opamp operation mode.           */ \
  false,                          /* Disable 3x gain setting.                */ \
  false,                          /* Use full output drive strength.         */ \
  false,                          /* Disable unity-gain bandwidth scaling.   */ \
  false,                          /* Opamp triggered by OPAxEN.              */ \
  opaPrsModeDefault,              /* PRS is not used to trigger opamp.       */ \
  opaPrsSelDefault,               /* PRS is not used to trigger opamp.       */ \
  opaPrsOutDefault,               /* Default PRS output setting.             */ \
  false,                          /* Bus mastering enabled on APORTX.        */ \
  false,                          /* Bus mastering enabled on APORTY.        */ \
  3,                              /* 3us settle time with default DrvStr.    */ \
  0,                              /* No startup delay.                       */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use calibrated inverting offset.        */ \
  0,                              /* Opamp offset value (not used).          */ \
  true,                           /* Use calibrated non-inverting offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA2 in three-opamp differential receiver mode. */
#define OPA_INIT_DIFF_RECEIVER_OPA2                                             \
{                                                                               \
  opaNegSelResTap,                /* Input from resistor ladder tap.         */ \
  opaPosSelResTap,                /* Input from OPA0 resistor ladder tap.    */ \
  opaOutModeMain,                 /* Main output enabled.                    */ \
  opaResSelR2eqR1,                /* R2 = R1                                 */ \
  opaResInMuxOpaIn,               /* Resistor ladder input from OPA1.        */ \
  0,                              /* No alternate outputs enabled.           */ \
  opaDrvStrDefault,               /* Default opamp operation mode.           */ \
  false,                          /* Disable 3x gain setting.                */ \
  false,                          /* Use full output drive strength.         */ \
  false,                          /* Disable unity-gain bandwidth scaling.   */ \
  false,                          /* Opamp triggered by OPAxEN.              */ \
  opaPrsModeDefault,              /* PRS is not used to trigger opamp.       */ \
  opaPrsSelDefault,               /* PRS is not used to trigger opamp.       */ \
  opaPrsOutDefault,               /* Default PRS output setting.             */ \
  false,                          /* Bus mastering enabled on APORTX.        */ \
  false,                          /* Bus mastering enabled on APORTY.        */ \
  3,                              /* 3us settle time with default DrvStr.    */ \
  0,                              /* No startup delay.                       */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use calibrated inverting offset.        */ \
  0,                              /* Opamp offset value (not used).          */ \
  true,                           /* Use calibrated non-inverting offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA0 in two-opamp instrumentation amplifier mode. */
#define OPA_INIT_INSTR_AMP_OPA0                                                 \
{                                                                               \
  opaNegSelResTap,                /* Input from resistor ladder tap.         */ \
  opaPosSelPosPad,                /* Pos input from pad.                     */ \
  opaOutModeMain,                 /* Main output enabled.                    */ \
  opaResSelR2eqR1,                /* R2 = R1                                 */ \
  opaResInMuxCenter,              /* OPA0/OPA1 resistor ladders connected.   */ \
  0,                              /* No alternate outputs enabled.           */ \
  opaDrvStrDefault,               /* Default opamp operation mode.           */ \
  false,                          /* Disable 3x gain setting.                */ \
  false,                          /* Use full output drive strength.         */ \
  false,                          /* Disable unity-gain bandwidth scaling.   */ \
  false,                          /* Opamp triggered by OPAxEN.              */ \
  opaPrsModeDefault,              /* PRS is not used to trigger opamp.       */ \
  opaPrsSelDefault,               /* PRS is not used to trigger opamp.       */ \
  opaPrsOutDefault,               /* Default PRS output setting.             */ \
  false,                          /* Bus mastering enabled on APORTX.        */ \
  false,                          /* Bus mastering enabled on APORTY.        */ \
  3,                              /* 3us settle time with default DrvStr.    */ \
  0,                              /* No startup delay.                       */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use calibrated inverting offset.        */ \
  0,                              /* Opamp offset value (not used).          */ \
  true,                           /* Use calibrated non-inverting offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

/** Configuration of OPA1 in two-opamp instrumentation amplifier mode. */
#define OPA_INIT_INSTR_AMP_OPA1                                                 \
{                                                                               \
  opaNegSelNegPad,                /* Neg input from pad.                     */ \
  opaPosSelResTap,                /* Input from resistor ladder tap.         */ \
  opaOutModeMain,                 /* Main output enabled.                    */ \
  opaResSelR2eqR1,                /* R2 = R1                                 */ \
  opaResInMuxCenter,              /* OPA0/OPA1 resistor ladders connected.   */ \
  0,                              /* No alternate outputs enabled.           */ \
  opaDrvStrDefault,               /* Default opamp operation mode.           */ \
  false,                          /* Disable 3x gain setting.                */ \
  false,                          /* Use full output drive strength.         */ \
  false,                          /* Disable unity-gain bandwidth scaling.   */ \
  false,                          /* Opamp triggered by OPAxEN.              */ \
  opaPrsModeDefault,              /* PRS is not used to trigger opamp.       */ \
  opaPrsSelDefault,               /* PRS is not used to trigger opamp.       */ \
  opaPrsOutDefault,               /* Default PRS output setting.             */ \
  false,                          /* Bus mastering enabled on APORTX.        */ \
  false,                          /* Bus mastering enabled on APORTY.        */ \
  3,                              /* 3us settle time with default DrvStr.    */ \
  0,                              /* No startup delay.                       */ \
  false,                          /* Rail-to-rail input enabled.             */ \
  true,                           /* Use calibrated inverting offset.        */ \
  0,                              /* Opamp offset value (not used).          */ \
  true,                           /* Use calibrated non-inverting offset.    */ \
  0                               /* Opamp offset value (not used).          */ \
}

#endif /* defined(_SILICON_LABS_32B_SERIES_0) */

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

#if defined(_SILICON_LABS_32B_SERIES_0)
void      OPAMP_Disable(DAC_TypeDef *dac, OPAMP_TypeDef opa);
void      OPAMP_Enable(DAC_TypeDef *dac, OPAMP_TypeDef opa, const OPAMP_Init_TypeDef *init);
#elif defined(_SILICON_LABS_32B_SERIES_1)
void      OPAMP_Disable(VDAC_TypeDef *dac, OPAMP_TypeDef opa);
void      OPAMP_Enable(VDAC_TypeDef *dac, OPAMP_TypeDef opa, const OPAMP_Init_TypeDef *init);
#endif /* defined(_SILICON_LABS_32B_SERIES_0) */

/** @} (end addtogroup OPAMP) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* (defined(OPAMP_PRESENT) && (OPAMP_COUNT == 1))
           || defined(VDAC_PRESENT) && (VDAC_COUNT > 0) */
#endif /* EM_OPAMP_H */
