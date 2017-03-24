/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _FSL_SIM_H_
#define _FSL_SIM_H_

#include "fsl_common.h"

/*! @addtogroup sim */
/*! @{*/


/*******************************************************************************
 * Definitions
 *******************************************************************************/

/*! @name Driver version */
/*@{*/
#define FSL_SIM_DRIVER_VERSION (MAKE_VERSION(2, 0, 0)) /*!< Driver version 2.0.0 */
/*@}*/

#if (defined(FSL_FEATURE_SIM_OPT_HAS_USB_VOLTAGE_REGULATOR) && FSL_FEATURE_SIM_OPT_HAS_USB_VOLTAGE_REGULATOR)
/*!@brief USB voltage regulator enable setting. */
enum _sim_usb_volt_reg_enable_mode
{
    kSIM_UsbVoltRegEnable = SIM_SOPT1_USBREGEN_MASK,           /*!< Enable voltage regulator. */
    kSIM_UsbVoltRegEnableInLowPower = SIM_SOPT1_USBVSTBY_MASK, /*!< Enable voltage regulator in VLPR/VLPW modes. */
    kSIM_UsbVoltRegEnableInStop = SIM_SOPT1_USBSSTBY_MASK, /*!< Enable voltage regulator in STOP/VLPS/LLS/VLLS modes. */
    kSIM_UsbVoltRegEnableInAllModes = SIM_SOPT1_USBREGEN_MASK | SIM_SOPT1_USBSSTBY_MASK |
                                      SIM_SOPT1_USBVSTBY_MASK /*!< Enable voltage regulator in all power modes. */
};
#endif /* (defined(FSL_FEATURE_SIM_OPT_HAS_USB_VOLTAGE_REGULATOR) && FSL_FEATURE_SIM_OPT_HAS_USB_VOLTAGE_REGULATOR) */

/*!@brief Unique ID. */
typedef struct _sim_uid
{
#if defined(SIM_UIDH)
    uint32_t H; /*!< UIDH.  */
#endif
    uint32_t MH; /*!< UIDMH. */
    uint32_t ML; /*!< UIDML. */
    uint32_t L;  /*!< UIDL.  */
} sim_uid_t;

/*!@brief Flash enable mode. */
enum _sim_flash_mode
{
    kSIM_FlashDisableInWait = SIM_FCFG1_FLASHDOZE_MASK, /*!< Disable flash in wait mode.   */
    kSIM_FlashDisable = SIM_FCFG1_FLASHDIS_MASK         /*!< Disable flash in normal mode. */
};

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus*/

#if (defined(FSL_FEATURE_SIM_OPT_HAS_USB_VOLTAGE_REGULATOR) && FSL_FEATURE_SIM_OPT_HAS_USB_VOLTAGE_REGULATOR)
/*!
 * @brief Sets the USB voltage regulator setting.
 *
 * This function configures whether the USB voltage regulator is enabled in
 * normal RUN mode, STOP/VLPS/LLS/VLLS modes, and VLPR/VLPW modes. The configurations
 * are passed in as mask value of \ref _sim_usb_volt_reg_enable_mode. For example, to enable
 * USB voltage regulator in RUN/VLPR/VLPW modes and disable in STOP/VLPS/LLS/VLLS mode,
 * use:
 *
 * SIM_SetUsbVoltRegulatorEnableMode(kSIM_UsbVoltRegEnable | kSIM_UsbVoltRegEnableInLowPower);
 *
 * @param mask  USB voltage regulator enable setting.
 */
void SIM_SetUsbVoltRegulatorEnableMode(uint32_t mask);
#endif /* FSL_FEATURE_SIM_OPT_HAS_USB_VOLTAGE_REGULATOR */

/*!
 * @brief Gets the unique identification register value.
 *
 * @param uid Pointer to the structure to save the UID value.
 */
void SIM_GetUniqueId(sim_uid_t *uid);

/*!
 * @brief Sets the flash enable mode.
 *
 * @param mode The mode to set; see \ref _sim_flash_mode for mode details.
 */
static inline void SIM_SetFlashMode(uint8_t mode)
{
    SIM->FCFG1 = mode;
}

#if defined(__cplusplus)
}
#endif /* __cplusplus*/

/*! @}*/

#endif /* _FSL_SIM_H_ */
