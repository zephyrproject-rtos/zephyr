/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "fsl_wm8960.h"

/*******************************************************************************
 * Definitations
 ******************************************************************************/
#define WM8960_CHECK_RET(x, status)      \
    do                                   \
    {                                    \
        (status) = (x);                  \
        if (kStatus_Success != (status)) \
        {                                \
            return (status);             \
        }                                \
    } while (false)
/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*
 * wm8960 register cache
 * We can't read the WM8960 register space when we are
 * using 2 wire for device control, so we cache them instead.
 */
static const uint16_t wm8960_reg[WM8960_CACHEREGNUM] = {
    0x0097, 0x0097, 0x0000, 0x0000, 0x0000, 0x0008, 0x0000, 0x000a, 0x01c0, 0x0000, 0x00ff, 0x00ff, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x007b, 0x0100, 0x0032, 0x0000, 0x00c3, 0x00c3, 0x01c0, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0100, 0x0100, 0x0050, 0x0050, 0x0050, 0x0050, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0040, 0x0000, 0x0000, 0x0050, 0x0050, 0x0000, 0x0002, 0x0037, 0x004d, 0x0080, 0x0008, 0x0031, 0x0026, 0x00e9,
};

static uint16_t reg_cache[WM8960_CACHEREGNUM];

/*******************************************************************************
 * Code
 ******************************************************************************/
status_t WM8960_Init(wm8960_handle_t *handle, const wm8960_config_t *config)
{
    status_t ret = kStatus_Success;

    handle->config = config;

    /* i2c bus initialization */
    if (CODEC_I2C_Init(handle->i2cHandle, config->i2cConfig.codecI2CInstance, WM8960_I2C_BAUDRATE,
                       config->i2cConfig.codecI2CSourceClock) != (status_t)kStatus_HAL_I2cSuccess)
    {
        return kStatus_Fail;
    }
    /* load wm8960 register map */
    (void)memcpy(reg_cache, wm8960_reg, sizeof(wm8960_reg));

    /* Reset the codec */
    WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_RESET, 0x00), ret);
    /*
     * VMID=50K, Enable VREF, AINL, AINR, ADCL and ADCR
     * I2S_IN (bit 0), I2S_OUT (bit 1), DAP (bit 4), DAC (bit 5), ADC (bit 6) are powered on
     */
    WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_POWER1, 0xFE), ret);
    /*
     * Enable DACL, DACR, LOUT1, ROUT1, PLL down, SPKL, SPKR
     */
    WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_POWER2, 0x1F8), ret);
    /*
     * Enable left and right channel input PGA, left and right output mixer
     */
    WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_POWER3, 0x3C), ret);
    /* ADC and DAC uses same clock */
    WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_IFACE2, 0x40), ret);
    /* set data route */
    WM8960_CHECK_RET(WM8960_SetDataRoute(handle, config->route), ret);
    /* set data protocol */
    WM8960_CHECK_RET(WM8960_SetProtocol(handle, config->bus), ret);
    /* set master or slave */
    WM8960_SetMasterSlave(handle, config->master_slave);
    /* select left input */
    WM8960_CHECK_RET(WM8960_SetLeftInput(handle, config->leftInputSource), ret);
    /* select right input */
    WM8960_CHECK_RET(WM8960_SetRightInput(handle, config->rightInputSource), ret);
    /* speaker power */
    if (config->enableSpeaker)
    {
        WM8960_CHECK_RET(WM8960_SetModule(handle, kWM8960_ModuleSpeaker, true), ret);
    }

    WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_ADDCTL1, 0x0C0), ret);
    WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_ADDCTL4, 0x40), ret);

    WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_BYPASS1, 0x0), ret);
    WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_BYPASS2, 0x0), ret);
    /*
     * ADC volume, 0dB
     */
    WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LADC, 0x1C3), ret);
    WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_RADC, 0x1C3), ret);

    /*
     * Digital DAC volume, 0dB
     */
    WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LDAC, 0x1E0), ret);
    WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_RDAC, 0x1E0), ret);

    /*
     * Headphone volume, LOUT1 and ROUT1, 0dB
     */
    WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LOUT1, 0x16F), ret);
    WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_ROUT1, 0x16F), ret);

    /* speaker volume 6dB */
    WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LOUT2, 0x1ff), ret);
    WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_ROUT2, 0x1ff), ret);
    /* enable class D output */
    WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_CLASSD1, 0xf7), ret);

    /* Unmute DAC. */
    WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_DACCTL1, 0x0000), ret);
    WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LINVOL, 0x117), ret);
    WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_RINVOL, 0x117), ret);

    WM8960_CHECK_RET(
        WM8960_ConfigDataFormat(handle, config->format.mclk_HZ, config->format.sampleRate, config->format.bitWidth),
        ret);

    return ret;
}

status_t WM8960_Deinit(wm8960_handle_t *handle)
{
    status_t ret = kStatus_Success;

    WM8960_CHECK_RET(WM8960_SetModule(handle, kWM8960_ModuleADC, false), ret);
    WM8960_CHECK_RET(WM8960_SetModule(handle, kWM8960_ModuleDAC, false), ret);
    WM8960_CHECK_RET(WM8960_SetModule(handle, kWM8960_ModuleVREF, false), ret);
    WM8960_CHECK_RET(WM8960_SetModule(handle, kWM8960_ModuleLineIn, false), ret);
    WM8960_CHECK_RET(WM8960_SetModule(handle, kWM8960_ModuleLineOut, false), ret);
    WM8960_CHECK_RET(WM8960_SetModule(handle, kWM8960_ModuleSpeaker, false), ret);
    WM8960_CHECK_RET(CODEC_I2C_Deinit(handle->i2cHandle), ret);

    return ret;
}

void WM8960_SetMasterSlave(wm8960_handle_t *handle, bool master)
{
    if (master)
    {
        (void)WM8960_ModifyReg(handle, WM8960_IFACE1, WM8960_IFACE1_MS_MASK, WM8960_IFACE1_MS(WM8960_IFACE1_MASTER));
    }
    else
    {
        (void)WM8960_ModifyReg(handle, WM8960_IFACE1, WM8960_IFACE1_MS_MASK, WM8960_IFACE1_MS(WM8960_IFACE1_SLAVE));
    }
}

status_t WM8960_SetModule(wm8960_handle_t *handle, wm8960_module_t module, bool isEnabled)
{
    status_t ret = kStatus_Success;
    switch (module)
    {
        case kWM8960_ModuleADC:
            WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_POWER1, WM8960_POWER1_ADCL_MASK,
                                              ((uint16_t)isEnabled << WM8960_POWER1_ADCL_SHIFT)),
                             ret);
            WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_POWER1, WM8960_POWER1_ADCR_MASK,
                                              ((uint16_t)isEnabled << WM8960_POWER1_ADCR_SHIFT)),
                             ret);
            break;
        case kWM8960_ModuleDAC:
            WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_POWER2, WM8960_POWER2_DACL_MASK,
                                              ((uint16_t)isEnabled << WM8960_POWER2_DACL_SHIFT)),
                             ret);
            WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_POWER2, WM8960_POWER2_DACR_MASK,
                                              ((uint16_t)isEnabled << WM8960_POWER2_DACR_SHIFT)),
                             ret);
            break;
        case kWM8960_ModuleVREF:
            WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_POWER1, WM8960_POWER1_VREF_MASK,
                                              ((uint16_t)isEnabled << WM8960_POWER1_VREF_SHIFT)),
                             ret);
            break;
        case kWM8960_ModuleLineIn:
            WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_POWER1, WM8960_POWER1_AINL_MASK,
                                              ((uint16_t)isEnabled << WM8960_POWER1_AINL_SHIFT)),
                             ret);
            WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_POWER1, WM8960_POWER1_AINR_MASK,
                                              ((uint16_t)isEnabled << WM8960_POWER1_AINR_SHIFT)),
                             ret);
            WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_POWER3, WM8960_POWER3_LMIC_MASK,
                                              ((uint16_t)isEnabled << WM8960_POWER3_LMIC_SHIFT)),
                             ret);
            WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_POWER3, WM8960_POWER3_RMIC_MASK,
                                              ((uint16_t)isEnabled << WM8960_POWER3_RMIC_SHIFT)),
                             ret);
            break;
        case kWM8960_ModuleLineOut:
            WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_POWER2, WM8960_POWER2_LOUT1_MASK,
                                              ((uint16_t)isEnabled << WM8960_POWER2_LOUT1_SHIFT)),
                             ret);
            WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_POWER2, WM8960_POWER2_ROUT1_MASK,
                                              ((uint16_t)isEnabled << WM8960_POWER2_ROUT1_SHIFT)),
                             ret);
            break;
        case kWM8960_ModuleMICB:
            WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_POWER1, WM8960_POWER1_MICB_MASK,
                                              ((uint16_t)isEnabled << WM8960_POWER1_MICB_SHIFT)),
                             ret);
            break;
        case kWM8960_ModuleSpeaker:
            WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_POWER2, WM8960_POWER2_SPKL_MASK,
                                              ((uint16_t)isEnabled << WM8960_POWER2_SPKL_SHIFT)),
                             ret);
            WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_POWER2, WM8960_POWER2_SPKR_MASK,
                                              ((uint16_t)isEnabled << WM8960_POWER2_SPKR_SHIFT)),
                             ret);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_CLASSD1, 0xF7), ret);
            break;
        case kWM8960_ModuleOMIX:
            WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_POWER3, WM8960_POWER3_LOMIX_MASK,
                                              ((uint16_t)isEnabled << WM8960_POWER3_LOMIX_SHIFT)),
                             ret);
            WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_POWER3, WM8960_POWER3_ROMIX_MASK,
                                              ((uint16_t)isEnabled << WM8960_POWER3_ROMIX_SHIFT)),
                             ret);
            break;
        default:
            ret = kStatus_InvalidArgument;
            break;
    }
    return ret;
}

status_t WM8960_SetDataRoute(wm8960_handle_t *handle, wm8960_route_t route)
{
    status_t ret = kStatus_Success;
    switch (route)
    {
        case kWM8960_RouteBypass:
            /* Bypass means from line-in to HP*/
            /*
             * Left LINPUT3 to left output mixer, LINPUT3 left output mixer volume = 0dB
             */
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LOUTMIX, 0x80), ret);

            /*
             * Right RINPUT3 to right output mixer, RINPUT3 right output mixer volume = 0dB
             */
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_ROUTMIX, 0x80), ret);
            break;
        case kWM8960_RoutePlayback:
            /* Data route I2S_IN-> DAC-> HP */
            /*
             * Left DAC to left output mixer, LINPUT3 left output mixer volume = 0dB
             */
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LOUTMIX, 0x100), ret);

            /*
             * Right DAC to right output mixer, RINPUT3 right output mixer volume = 0dB
             */
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_ROUTMIX, 0x100), ret);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_POWER3, 0x0C), ret);
            /* Set power for DAC */
            WM8960_CHECK_RET(WM8960_SetModule(handle, kWM8960_ModuleDAC, true), ret);
            WM8960_CHECK_RET(WM8960_SetModule(handle, kWM8960_ModuleOMIX, true), ret);
            WM8960_CHECK_RET(WM8960_SetModule(handle, kWM8960_ModuleLineOut, true), ret);
            break;
        case kWM8960_RoutePlaybackandRecord:
            /*
             * Left DAC to left output mixer, LINPUT3 left output mixer volume = 0dB
             */
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LOUTMIX, 0x100), ret);

            /*
             * Right DAC to right output mixer, RINPUT3 right output mixer volume = 0dB
             */
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_ROUTMIX, 0x100), ret);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_POWER3, 0x3C), ret);
            WM8960_CHECK_RET(WM8960_SetModule(handle, kWM8960_ModuleDAC, true), ret);
            WM8960_CHECK_RET(WM8960_SetModule(handle, kWM8960_ModuleADC, true), ret);
            WM8960_CHECK_RET(WM8960_SetModule(handle, kWM8960_ModuleLineIn, true), ret);
            WM8960_CHECK_RET(WM8960_SetModule(handle, kWM8960_ModuleOMIX, true), ret);
            WM8960_CHECK_RET(WM8960_SetModule(handle, kWM8960_ModuleLineOut, true), ret);
            break;
        case kWM8960_RouteRecord:
            /* LINE_IN->ADC->I2S_OUT */
            /*
             * Left and right input boost, LIN3BOOST and RIN3BOOST = 0dB
             */
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_POWER3, 0x30), ret);
            /* Power up ADC and AIN */
            WM8960_CHECK_RET(WM8960_SetModule(handle, kWM8960_ModuleLineIn, true), ret);
            WM8960_CHECK_RET(WM8960_SetModule(handle, kWM8960_ModuleADC, true), ret);
            break;
        default:
            ret = kStatus_InvalidArgument;
            break;
    }
    return ret;
}

status_t WM8960_SetLeftInput(wm8960_handle_t *handle, wm8960_input_t input)
{
    status_t ret = kStatus_Success;
    uint16_t val = 0;

    switch (input)
    {
        case kWM8960_InputClosed:
            WM8960_CHECK_RET(WM8960_ReadReg(WM8960_POWER1, &val), ret);
            val &= ~(WM8960_POWER1_AINL_MASK | WM8960_POWER1_ADCL_MASK);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_POWER1, val), ret);
            break;
        case kWM8960_InputSingleEndedMic:
            /* Only LMN1 enabled, LMICBOOST to 13db, LMIC2B enabled */
            WM8960_CHECK_RET(WM8960_ReadReg(WM8960_POWER1, &val), ret);
            val |= (WM8960_POWER1_AINL_MASK | WM8960_POWER1_ADCL_MASK | WM8960_POWER1_MICB_MASK);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_POWER1, val), ret);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LINPATH, 0x138), ret);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LINVOL, 0x117), ret);
            break;
        case kWM8960_InputDifferentialMicInput2:
            WM8960_CHECK_RET(WM8960_ReadReg(WM8960_POWER1, &val), ret);
            val |= (WM8960_POWER1_AINL_MASK | WM8960_POWER1_ADCL_MASK | WM8960_POWER1_MICB_MASK);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_POWER1, val), ret);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LINPATH, 0x178), ret);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LINVOL, 0x117), ret);
            break;
        case kWM8960_InputDifferentialMicInput3:
            WM8960_CHECK_RET(WM8960_ReadReg(WM8960_POWER1, &val), ret);
            val |= (WM8960_POWER1_AINL_MASK | WM8960_POWER1_ADCL_MASK | WM8960_POWER1_MICB_MASK);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_POWER1, val), ret);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LINPATH, 0x1B8), ret);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LINVOL, 0x117), ret);
            break;
        case kWM8960_InputLineINPUT2:
            WM8960_CHECK_RET(WM8960_ReadReg(WM8960_POWER1, &val), ret);
            val |= (WM8960_POWER1_AINL_MASK | WM8960_POWER1_ADCL_MASK);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_POWER1, val), ret);
            WM8960_CHECK_RET(WM8960_ReadReg(WM8960_INBMIX1, &val), ret);
            val |= 0xEU;
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_INBMIX1, val), ret);
            break;
        case kWM8960_InputLineINPUT3:
            WM8960_CHECK_RET(WM8960_ReadReg(WM8960_POWER1, &val), ret);
            val |= (WM8960_POWER1_AINL_MASK | WM8960_POWER1_ADCL_MASK);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_POWER1, val), ret);
            WM8960_CHECK_RET(WM8960_ReadReg(WM8960_INBMIX1, &val), ret);
            val |= 0x70U;
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_INBMIX1, val), ret);
            break;
        default:
            ret = kStatus_InvalidArgument;
            break;
    }

    return ret;
}

status_t WM8960_SetRightInput(wm8960_handle_t *handle, wm8960_input_t input)
{
    status_t ret = kStatus_Success;
    uint16_t val = 0;

    switch (input)
    {
        case kWM8960_InputClosed:
            WM8960_CHECK_RET(WM8960_ReadReg(WM8960_POWER1, &val), ret);
            val &= ~(WM8960_POWER1_AINR_MASK | WM8960_POWER1_ADCR_MASK);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_POWER1, val), ret);
            break;
        case kWM8960_InputSingleEndedMic:
            /* Only LMN1 enabled, LMICBOOST to 13db, LMIC2B enabled */
            WM8960_CHECK_RET(WM8960_ReadReg(WM8960_POWER1, &val), ret);
            val |= (WM8960_POWER1_AINR_MASK | WM8960_POWER1_ADCR_MASK | WM8960_POWER1_MICB_MASK);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_POWER1, val), ret);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_RINPATH, 0x138), ret);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_RINVOL, 0x117), ret);
            break;
        case kWM8960_InputDifferentialMicInput2:
            WM8960_CHECK_RET(WM8960_ReadReg(WM8960_POWER1, &val), ret);
            val |= (WM8960_POWER1_AINR_MASK | WM8960_POWER1_ADCR_MASK | WM8960_POWER1_MICB_MASK);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_POWER1, val), ret);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_RINPATH, 0x178), ret);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_RINVOL, 0x117), ret);
            break;
        case kWM8960_InputDifferentialMicInput3:
            WM8960_CHECK_RET(WM8960_ReadReg(WM8960_POWER1, &val), ret);
            val |= (WM8960_POWER1_AINR_MASK | WM8960_POWER1_ADCR_MASK | WM8960_POWER1_MICB_MASK);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_POWER1, val), ret);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_RINPATH, 0x1B8), ret);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_RINVOL, 0x117), ret);
            break;
        case kWM8960_InputLineINPUT2:
            WM8960_CHECK_RET(WM8960_ReadReg(WM8960_POWER1, &val), ret);
            val |= (WM8960_POWER1_AINR_MASK | WM8960_POWER1_ADCR_MASK);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_POWER1, val), ret);
            WM8960_CHECK_RET(WM8960_ReadReg(WM8960_INBMIX2, &val), ret);
            val |= 0xEU;
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_INBMIX2, val), ret);
            break;
        case kWM8960_InputLineINPUT3:
            WM8960_CHECK_RET(WM8960_ReadReg(WM8960_POWER1, &val), ret);
            val |= (WM8960_POWER1_AINR_MASK | WM8960_POWER1_ADCR_MASK);
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_POWER1, val), ret);
            WM8960_CHECK_RET(WM8960_ReadReg(WM8960_INBMIX2, &val), ret);
            val |= 0x70U;
            WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_INBMIX2, val), ret);
            break;
        default:
            ret = kStatus_InvalidArgument;
            break;
    }

    return ret;
}

status_t WM8960_SetProtocol(wm8960_handle_t *handle, wm8960_protocol_t protocol)
{
    return WM8960_ModifyReg(handle, WM8960_IFACE1, WM8960_IFACE1_FORMAT_MASK | WM8960_IFACE1_LRP_MASK,
                            (uint16_t)protocol);
}

status_t WM8960_SetVolume(wm8960_handle_t *handle, wm8960_module_t module, uint32_t volume)
{
    uint16_t vol = 0;
    status_t ret = kStatus_Success;
    switch (module)
    {
        case kWM8960_ModuleADC:
            if (volume > 255U)
            {
                ret = kStatus_InvalidArgument;
            }
            else
            {
                vol = (uint16_t)volume;
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LADC, vol), ret);
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_RADC, vol), ret);
                /* Update volume */
                vol = (uint16_t)(0x100U | volume);
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LADC, vol), ret);
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_RADC, vol), ret);
            }
            break;
        case kWM8960_ModuleDAC:
            if (volume > 255U)
            {
                ret = kStatus_InvalidArgument;
            }
            else
            {
                vol = (uint16_t)volume;
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LDAC, vol), ret);
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_RDAC, vol), ret);
                vol = 0x100U | (uint16_t)volume;
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LDAC, vol), ret);
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_RDAC, vol), ret);
            }
            break;
        case kWM8960_ModuleHP:
            if (volume > 0x7FU)
            {
                ret = kStatus_InvalidArgument;
            }
            else
            {
                vol = (uint16_t)volume;
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LOUT1, vol), ret);
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_ROUT1, vol), ret);
                vol = 0x100U | (uint16_t)volume;
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LOUT1, vol), ret);
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_ROUT1, vol), ret);
            }
            break;
        case kWM8960_ModuleLineIn:
            if (volume > 0x3FU)
            {
                ret = kStatus_InvalidArgument;
            }
            else
            {
                vol = (uint16_t)volume;
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LINVOL, vol), ret);
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_RINVOL, vol), ret);
                vol = 0x100U | (uint16_t)volume;
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LINVOL, vol), ret);
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_RINVOL, vol), ret);
            }
            break;
        case kWM8960_ModuleSpeaker:
            if (volume > 0x7FU)
            {
                ret = kStatus_InvalidArgument;
            }
            else
            {
                vol = (uint16_t)volume;
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LOUT2, vol), ret);
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_ROUT2, vol), ret);
                vol = 0x100U | (uint16_t)volume;
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LOUT2, vol), ret);
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_ROUT2, vol), ret);
            }
            break;
        default:
            ret = kStatus_InvalidArgument;
            break;
    }
    return ret;
}

uint32_t WM8960_GetVolume(wm8960_handle_t *handle, wm8960_module_t module)
{
    uint16_t vol = 0;

    switch (module)
    {
        case kWM8960_ModuleADC:
            (void)WM8960_ReadReg(WM8960_LADC, &vol);
            vol &= 0xFFU;
            break;
        case kWM8960_ModuleDAC:
            (void)WM8960_ReadReg(WM8960_LDAC, &vol);
            vol &= 0xFFU;
            break;
        case kWM8960_ModuleHP:
            (void)WM8960_ReadReg(WM8960_LOUT1, &vol);
            vol &= 0x7FU;
            break;
        case kWM8960_ModuleLineOut:
            (void)WM8960_ReadReg(WM8960_LINVOL, &vol);
            vol &= 0x3FU;
            break;
        default:
            vol = 0;
            break;
    }
    return vol;
}

status_t WM8960_SetMute(wm8960_handle_t *handle, wm8960_module_t module, bool isEnabled)
{
    status_t ret = kStatus_Success;
    switch (module)
    {
        case kWM8960_ModuleADC:
            /*
             * Digital Mute
             */
            if (isEnabled)
            {
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LADC, 0x100), ret);
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_RADC, 0x100), ret);
            }
            else
            {
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LADC, 0x1C3), ret);
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_RADC, 0x1C3), ret);
            }
            break;
        case kWM8960_ModuleDAC:
            /*
             * Digital mute
             */
            if (isEnabled)
            {
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LDAC, 0x100), ret);
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_RDAC, 0x100), ret);
            }
            else
            {
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LDAC, 0x1FF), ret);
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_RDAC, 0x1FF), ret);
            }
            break;
        case kWM8960_ModuleHP:
            /*
             * Analog mute
             */
            if (isEnabled)
            {
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LOUT1, 0x100), ret);
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_ROUT1, 0x100), ret);
            }
            else
            {
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LOUT1, 0x16F), ret);
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_ROUT1, 0x16F), ret);
            }
            break;

        case kWM8960_ModuleSpeaker:
            if (isEnabled)
            {
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LOUT2, 0x100), ret);
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_ROUT2, 0x100), ret);
            }
            else
            {
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_LOUT2, 0x16F), ret);
                WM8960_CHECK_RET(WM8960_WriteReg(handle, WM8960_ROUT2, 0x16f), ret);
            }
            break;

        case kWM8960_ModuleLineOut:
            break;
        default:
            ret = kStatus_InvalidArgument;
            break;
    }
    return ret;
}

status_t WM8960_ConfigDataFormat(wm8960_handle_t *handle, uint32_t sysclk, uint32_t sample_rate, uint32_t bits)
{
    status_t retval  = kStatus_Success;
    uint32_t divider = 0;
    uint16_t val     = 0;

    /* Compute sample rate divider, dac and adc are the same sample rate */
    divider = sysclk / sample_rate;
    if (divider == 256U)
    {
        val = 0;
    }
    else if (divider > 256U)
    {
        val = (uint16_t)(((divider / 256U) << 6U) | ((divider / 256U) << 3U));
    }
    else
    {
        return kStatus_InvalidArgument;
    }

    retval = WM8960_WriteReg(handle, WM8960_CLOCK1, val);

    /*
     * Slave mode (MS = 0), LRP = 0, 32bit WL, left justified (FORMAT[1:0]=0b01)
     */
    switch (bits)
    {
        case 16:
            retval = WM8960_ModifyReg(handle, WM8960_IFACE1, WM8960_IFACE1_WL_MASK,
                                      WM8960_IFACE1_WL(WM8960_IFACE1_WL_16BITS));
            break;
        case 20:
            retval = WM8960_ModifyReg(handle, WM8960_IFACE1, WM8960_IFACE1_WL_MASK,
                                      WM8960_IFACE1_WL(WM8960_IFACE1_WL_20BITS));
            break;
        case 24:
            retval = WM8960_ModifyReg(handle, WM8960_IFACE1, WM8960_IFACE1_WL_MASK,
                                      WM8960_IFACE1_WL(WM8960_IFACE1_WL_24BITS));
            break;
        case 32:
            retval = WM8960_ModifyReg(handle, WM8960_IFACE1, WM8960_IFACE1_WL_MASK,
                                      WM8960_IFACE1_WL(WM8960_IFACE1_WL_32BITS));
            break;
        default:
            retval = kStatus_InvalidArgument;
            break;
    }

    return retval;
}

status_t WM8960_SetJackDetect(wm8960_handle_t *handle, bool isEnabled)
{
    status_t retval = 0;
    uint16_t val    = 0;

    if (WM8960_ReadReg(WM8960_ADDCTL2, &val) != kStatus_Success)
    {
        return kStatus_Fail;
    }

    if (isEnabled)
    {
        val |= 0x40U;
    }
    else
    {
        val &= 0xCFU;
    }

    retval = WM8960_WriteReg(handle, WM8960_ADDCTL2, val);

    return retval;
}

status_t WM8960_WriteReg(wm8960_handle_t *handle, uint8_t reg, uint16_t val)
{
    uint8_t cmd;
    uint8_t buff = (uint8_t)val & 0xFFU;

    /* The register address */
    cmd = (reg << 1U) | (uint8_t)((val >> 8U) & 0x0001U);

    reg_cache[reg] = val;

    return CODEC_I2C_Send(handle->i2cHandle, handle->config->slaveAddress, cmd, 1U, &buff, 1U);
}

status_t WM8960_ReadReg(uint8_t reg, uint16_t *val)
{
    if (reg >= WM8960_CACHEREGNUM)
    {
        return kStatus_InvalidArgument;
    }

    *val = reg_cache[reg];

    return kStatus_Success;
}

status_t WM8960_ModifyReg(wm8960_handle_t *handle, uint8_t reg, uint16_t mask, uint16_t val)
{
    status_t retval  = 0;
    uint16_t reg_val = 0;
    retval           = WM8960_ReadReg(reg, &reg_val);
    if (retval != kStatus_Success)
    {
        return kStatus_Fail;
    }
    reg_val &= (uint16_t)~mask;
    reg_val |= val;
    retval = WM8960_WriteReg(handle, reg, reg_val);
    if (retval != kStatus_Success)
    {
        return kStatus_Fail;
    }
    return kStatus_Success;
}

status_t WM8960_SetPlay(wm8960_handle_t *handle, uint32_t playSource)
{
    status_t ret = kStatus_Success;

    if (((uint32_t)kWM8960_PlaySourcePGA & playSource) != 0U)
    {
        WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_BYPASS1, 0x80U, 0x80U), ret);
        WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_BYPASS2, 0x80U, 0x80U), ret);
        WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_LOUTMIX, 0x180U, 0U), ret);
        WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_ROUTMIX, 0x180U, 0U), ret);
    }

    if ((playSource & (uint32_t)kWM8960_PlaySourceDAC) != 0U)
    {
        WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_BYPASS1, 0x80U, 0x00U), ret);
        WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_BYPASS2, 0x80U, 0x00U), ret);
        WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_LOUTMIX, 0x180U, 0x100U), ret);
        WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_ROUTMIX, 0x180U, 0x100U), ret);
    }

    if ((playSource & (uint32_t)kWM8960_PlaySourceInput) != 0U)
    {
        WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_BYPASS1, 0x80U, 0x0U), ret);
        WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_BYPASS2, 0x80U, 0x0U), ret);
        WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_LOUTMIX, 0x180U, 0x80U), ret);
        WM8960_CHECK_RET(WM8960_ModifyReg(handle, WM8960_ROUTMIX, 0x180U, 0x80U), ret);
    }

    return ret;
}
