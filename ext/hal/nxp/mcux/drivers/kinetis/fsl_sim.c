/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_sim.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.sim"
#endif

/*******************************************************************************
 * Codes
 ******************************************************************************/
#if (defined(FSL_FEATURE_SIM_OPT_HAS_USB_VOLTAGE_REGULATOR) && FSL_FEATURE_SIM_OPT_HAS_USB_VOLTAGE_REGULATOR)
/*!
 * brief Sets the USB voltage regulator setting.
 *
 * This function configures whether the USB voltage regulator is enabled in
 * normal RUN mode, STOP/VLPS/LLS/VLLS modes, and VLPR/VLPW modes. The configurations
 * are passed in as mask value of \ref _sim_usb_volt_reg_enable_mode. For example, to enable
 * USB voltage regulator in RUN/VLPR/VLPW modes and disable in STOP/VLPS/LLS/VLLS mode,
 * use:
 *
 * SIM_SetUsbVoltRegulatorEnableMode(kSIM_UsbVoltRegEnable | kSIM_UsbVoltRegEnableInLowPower);
 *
 * param mask  USB voltage regulator enable setting.
 */
void SIM_SetUsbVoltRegulatorEnableMode(uint32_t mask)
{
    SIM->SOPT1CFG |= (SIM_SOPT1CFG_URWE_MASK | SIM_SOPT1CFG_UVSWE_MASK | SIM_SOPT1CFG_USSWE_MASK);

    SIM->SOPT1 = (SIM->SOPT1 & ~kSIM_UsbVoltRegEnableInAllModes) | mask;
}
#endif /* FSL_FEATURE_SIM_OPT_HAS_USB_VOLTAGE_REGULATOR */

/*!
 * brief Gets the unique identification register value.
 *
 * param uid Pointer to the structure to save the UID value.
 */
void SIM_GetUniqueId(sim_uid_t *uid)
{
#if defined(SIM_UIDH)
    uid->H = SIM->UIDH;
#endif
#if (defined(FSL_FEATURE_SIM_HAS_UIDM) && FSL_FEATURE_SIM_HAS_UIDM)
    uid->M = SIM->UIDM;
#else
    uid->MH = SIM->UIDMH;
    uid->ML = SIM->UIDML;
#endif /* FSL_FEATURE_SIM_HAS_UIDM */
    uid->L = SIM->UIDL;
}

#if (defined(FSL_FEATURE_SIM_HAS_RF_MAC_ADDR) && FSL_FEATURE_SIM_HAS_RF_MAC_ADDR)
/*!
 * brief Gets the RF address register value.
 *
 * param info Pointer to the structure to save the RF address value.
 */
void SIM_GetRfAddr(sim_rf_addr_t *info)
{
    info->rfAddrL = SIM->RFADDRL;
    info->rfAddrH = SIM->RFADDRH;
}
#endif /* FSL_FEATURE_SIM_HAS_RF_MAC_ADDR */
