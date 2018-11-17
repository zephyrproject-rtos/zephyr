/***************************************************************************//**
 * @file em_emu.h
 * @brief Energy Management Unit (EMU) peripheral API
 * @version 5.6.0
 *******************************************************************************
 * # License
 * <b>Copyright 2016 Silicon Laboratories, Inc. www.silabs.com</b>
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

#ifndef EM_EMU_H
#define EM_EMU_H

#include "em_device.h"
#if defined(EMU_PRESENT)

#include <stdbool.h>
#include "em_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup EMU
 * @{
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

#if defined(_EMU_CTRL_EM23VSCALE_MASK)
#define EMU_VSCALE_PRESENT
#endif

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

#if defined(_EMU_EM4CONF_OSC_MASK)
/** EM4 duty oscillator. */
typedef enum {
  /** Select ULFRCO as duty oscillator in EM4. */
  emuEM4Osc_ULFRCO = EMU_EM4CONF_OSC_ULFRCO,
  /** Select LFXO as duty oscillator in EM4. */
  emuEM4Osc_LFXO = EMU_EM4CONF_OSC_LFXO,
  /** Select LFRCO as duty oscillator in EM4. */
  emuEM4Osc_LFRCO = EMU_EM4CONF_OSC_LFRCO
} EMU_EM4Osc_TypeDef;
#endif

#if defined(_EMU_BUCTRL_PROBE_MASK)
/** Backup Power Voltage Probe types. */
typedef enum {
  /** Disable voltage probe. */
  emuProbe_Disable = EMU_BUCTRL_PROBE_DISABLE,
  /** Connect probe to VDD_DREG. */
  emuProbe_VDDDReg = EMU_BUCTRL_PROBE_VDDDREG,
  /** Connect probe to BU_IN. */
  emuProbe_BUIN    = EMU_BUCTRL_PROBE_BUIN,
  /** Connect probe to BU_OUT. */
  emuProbe_BUOUT   = EMU_BUCTRL_PROBE_BUOUT
} EMU_Probe_TypeDef;
#endif

#if defined(_EMU_PWRCONF_PWRRES_MASK)
/** Backup Power Domain resistor selection. */
typedef enum {
  /** Main power and backup power connected with RES0 series resistance. */
  emuRes_Res0 = EMU_PWRCONF_PWRRES_RES0,
  /** Main power and backup power connected with RES1 series resistance. */
  emuRes_Res1 = EMU_PWRCONF_PWRRES_RES1,
  /** Main power and backup power connected with RES2 series resistance. */
  emuRes_Res2 = EMU_PWRCONF_PWRRES_RES2,
  /** Main power and backup power connected with RES3 series resistance. */
  emuRes_Res3 = EMU_PWRCONF_PWRRES_RES3,
} EMU_Resistor_TypeDef;
#endif

#if defined(BU_PRESENT) && defined(_SILICON_LABS_32B_SERIES_0)
/** Backup Power Domain power connection. */
typedef enum {
  /** No connection between main and backup power. */
  emuPower_None = EMU_BUINACT_PWRCON_NONE,
  /** Main power and backup power connected through diode,
      allowing current from backup to main only. */
  emuPower_BUMain = EMU_BUINACT_PWRCON_BUMAIN,
  /** Main power and backup power connected through diode,
      allowing current from main to backup only. */
  emuPower_MainBU = EMU_BUINACT_PWRCON_MAINBU,
  /** Main power and backup power connected without diode. */
  emuPower_NoDiode = EMU_BUINACT_PWRCON_NODIODE,
} EMU_Power_TypeDef;
#endif

#if defined(_EMU_BUCTRL_BUINACTPWRCON_MASK)
/** Backup Power Domain power connection configuration when not in backup mode selection. */
typedef enum {
  /** No connection. */
  emuBuBuInactPwrCon_None    = EMU_BUCTRL_BUINACTPWRCON_NONE,
  /** Allow power from backup to main. */
  emuBuBuInactPwrCon_MainBu  = EMU_BUCTRL_BUINACTPWRCON_MAINBU,
  /** Allow power from main to backup. */
  emuBuBuInactPwrCon_BuMain  = EMU_BUCTRL_BUINACTPWRCON_BUMAIN,
  /** Backup and main power are connected. */
  emuBuBuInactPwrCon_NoDiode = EMU_BUCTRL_BUINACTPWRCON_NODIODE,
} EMU_BUBuInactPwrCon_TypeDef;
#endif

#if defined(_EMU_BUCTRL_BUACTPWRCON_MASK)
/** Backup Power Domain power connection configuration in backup mode selection. */
typedef enum {
  /** No connection. */
  emuBuBuActPwrCon_None    = EMU_BUCTRL_BUACTPWRCON_NONE,
  /** Allow power from backup to main. */
  emuBuBuActPwrCon_MainBu  = EMU_BUCTRL_BUACTPWRCON_MAINBU,
  /** Allow power from main to backup. */
  emuBuBuActPwrCon_BuMain  = EMU_BUCTRL_BUACTPWRCON_BUMAIN,
  /** Backup and main power are connected. */
  emuBuBuActPwrCon_NoDiode = EMU_BUCTRL_BUACTPWRCON_NODIODE,
} EMU_BUBuActPwrCon_TypeDef;
#endif

#if defined(_EMU_BUCTRL_PWRRES_MASK)
/** Backup Power Domain resistor selection. */
typedef enum {
  /** Main power and backup power connected with RES0 series resistance. */
  emuBuPwrRes_Res0 = EMU_BUCTRL_PWRRES_RES0,
  /** Main power and backup power connected with RES1 series resistance. */
  emuBuPwrRes_Res1 = EMU_BUCTRL_PWRRES_RES1,
  /** Main power and backup power connected with RES2 series resistance. */
  emuBuPwrRes_Res2 = EMU_BUCTRL_PWRRES_RES2,
  /** Main power and backup power connected with RES3 series resistance. */
  emuBuPwrRes_Res3 = EMU_BUCTRL_PWRRES_RES3,
} EMU_BUPwrRes_TypeDef;
#endif

#if defined(_EMU_BUCTRL_VOUTRES_MASK)
/** Resistance between backup domain power supply and BU_VOUT. */
typedef enum {
  /** BU_VOUT is not connected. */
  emuBuVoutRes_Dis = EMU_BUCTRL_VOUTRES_DIS,
  /** Enable weak switch. */
  emuBuVoutRes_Weak = EMU_BUCTRL_VOUTRES_WEAK,
  /** Enable medium switch. */
  emuBuVoutRes_Med = EMU_BUCTRL_VOUTRES_MED,
  /** Enable strong switch. */
  emuBuVoutRes_Strong = EMU_BUCTRL_VOUTRES_STRONG,
} EMU_BUVoutRes_TypeDef;
#endif

/** BOD threshold setting selector, active or inactive mode. */
typedef enum {
  /** Configure BOD threshold for active mode. */
  emuBODMode_Active,
  /** Configure BOD threshold for inactive mode. */
  emuBODMode_Inactive,
} EMU_BODMode_TypeDef;

/** EM4 modes. */
typedef enum {
  /** EM4 Shutoff. */
  emuEM4Shutoff   = 0,
  /** EM4 Hibernate. */
  emuEM4Hibernate = 1,
} EMU_EM4State_TypeDef;

#if defined(_EMU_EM4CTRL_EM4IORETMODE_MASK)
typedef enum {
  /** No Retention: Pads enter reset state when entering EM4. */
  emuPinRetentionDisable = EMU_EM4CTRL_EM4IORETMODE_DISABLE,
  /** Retention through EM4: Pads enter reset state when exiting EM4. */
  emuPinRetentionEm4Exit = EMU_EM4CTRL_EM4IORETMODE_EM4EXIT,
  /** Retention through EM4 and wakeup: call @ref EMU_UnlatchPinRetention() to
      release pins from retention after EM4 wakeup. */
  emuPinRetentionLatch   = EMU_EM4CTRL_EM4IORETMODE_SWUNLATCH,
} EMU_EM4PinRetention_TypeDef;
#endif

/** Power configurations. DCDC-to-DVDD is currently the only supported mode. */
typedef enum {
  /** DCDC is connected to DVDD. */
  emuPowerConfig_DcdcToDvdd,
} EMU_PowerConfig_TypeDef;

#if defined(_EMU_DCDCCTRL_MASK)
/** DCDC operating modes. */
typedef enum {
  /** DCDC regulator bypass. */
  emuDcdcMode_Bypass = EMU_DCDCCTRL_DCDCMODE_BYPASS,
  /** DCDC low-noise mode. */
  emuDcdcMode_LowNoise = EMU_DCDCCTRL_DCDCMODE_LOWNOISE,
#if defined(_EMU_DCDCLPEM01CFG_MASK)
  /** DCDC low-power mode. */
  emuDcdcMode_LowPower = EMU_DCDCCTRL_DCDCMODE_LOWPOWER,
#endif
} EMU_DcdcMode_TypeDef;
#endif

#if defined(_EMU_DCDCCTRL_MASK)
/** DCDC conduction modes. */
typedef enum {
  /** DCDC Low-Noise Continuous Conduction Mode (CCM). EFR32 interference minimization
      features are available in this mode. */
  emuDcdcConductionMode_ContinuousLN,
  /** DCDC Low-Noise Discontinuous Conduction Mode (DCM). This mode should be used for EFM32 or
      for EFR32 when its radio is not enabled. */
  emuDcdcConductionMode_DiscontinuousLN,
} EMU_DcdcConductionMode_TypeDef;
#endif

#if defined(_EMU_PWRCTRL_MASK)
/** DCDC to DVDD mode analog peripheral power supply select. */
typedef enum {
  /** Select AVDD as analog power supply. Typically lower noise, but less energy efficient. */
  emuDcdcAnaPeripheralPower_AVDD = EMU_PWRCTRL_ANASW_AVDD,
  /** Select DCDC (DVDD) as analog power supply. Typically more energy efficient, but more noise. */
  emuDcdcAnaPeripheralPower_DCDC = EMU_PWRCTRL_ANASW_DVDD
} EMU_DcdcAnaPeripheralPower_TypeDef;
#endif

#if defined(_EMU_DCDCMISCCTRL_MASK)
/** DCDC Forced CCM and reverse current limiter control. Positive values have unit mA. */
typedef int16_t EMU_DcdcLnReverseCurrentControl_TypeDef;

/** High efficiency mode. EMU_DCDCZDETCTRL_ZDETILIMSEL is "don't care". */
#define emuDcdcLnHighEfficiency       -1

/** Default reverse current for fast transient response mode (low noise).  */
#define emuDcdcLnFastTransient         160
#endif

#if defined(_EMU_DCDCCTRL_MASK)
/** DCDC Low-noise RCO band select. */
typedef enum {
  /** Set RCO to 3MHz. */
  emuDcdcLnRcoBand_3MHz = 0,
  /** Set RCO to 4MHz. */
  emuDcdcLnRcoBand_4MHz = 1,
  /** Set RCO to 5MHz. */
  emuDcdcLnRcoBand_5MHz = 2,
  /** Set RCO to 6MHz. */
  emuDcdcLnRcoBand_6MHz = 3,
  /** Set RCO to 7MHz. */
  emuDcdcLnRcoBand_7MHz = 4,
  /** Set RCO to 8MHz. */
  emuDcdcLnRcoBand_8MHz = 5,
  /** Set RCO to 9MHz. */
  emuDcdcLnRcoBand_9MHz = 6,
  /** Set RCO to 10MHz. */
  emuDcdcLnRcoBand_10MHz = 7,
} EMU_DcdcLnRcoBand_TypeDef;

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */
/* Deprecated. */
#define EMU_DcdcLnRcoBand_3MHz          emuDcdcLnRcoBand_3MHz
#define EMU_DcdcLnRcoBand_4MHz          emuDcdcLnRcoBand_4MHz
#define EMU_DcdcLnRcoBand_5MHz          emuDcdcLnRcoBand_5MHz
#define EMU_DcdcLnRcoBand_6MHz          emuDcdcLnRcoBand_6MHz
#define EMU_DcdcLnRcoBand_7MHz          emuDcdcLnRcoBand_7MHz
#define EMU_DcdcLnRcoBand_8MHz          emuDcdcLnRcoBand_8MHz
#define EMU_DcdcLnRcoBand_9MHz          emuDcdcLnRcoBand_9MHz
#define EMU_DcdcLnRcoBand_10MHz         emuDcdcLnRcoBand_10MHz
/** @endcond */
#endif

#if defined(_EMU_DCDCCTRL_MASK)
/** DCDC Low Noise Compensator Control register. */
typedef enum {
  /** DCDC capacitor is 1uF. */
  emuDcdcLnCompCtrl_1u0F,
  /** DCDC capacitor is 4.7uF. */
  emuDcdcLnCompCtrl_4u7F,
} EMU_DcdcLnCompCtrl_TypeDef;
#endif

#if defined(EMU_STATUS_VMONRDY)
/** VMON channels. */
typedef enum {
  emuVmonChannel_AVDD,
  emuVmonChannel_ALTAVDD,
  emuVmonChannel_DVDD,
  emuVmonChannel_IOVDD0,
#if defined(_EMU_VMONIO1CTRL_EN_MASK)
  emuVmonChannel_IOVDD1,
#endif
#if defined(_EMU_VMONBUVDDCTRL_EN_MASK)
  emuVmonChannel_BUVDD,
#endif
} EMU_VmonChannel_TypeDef;
#endif /* EMU_STATUS_VMONRDY */

#if defined(_SILICON_LABS_GECKO_INTERNAL_SDID_80)
/** Bias mode configurations. */
typedef enum {
  emuBiasMode_1KHz,
  emuBiasMode_4KHz,
  emuBiasMode_Continuous
} EMU_BiasMode_TypeDef;
#endif

#if defined(EMU_VSCALE_PRESENT)
/** Supported EM0/1 Voltage Scaling Levels. */
typedef enum {
  /** High-performance voltage level. HF clock can be set to any frequency. */
  emuVScaleEM01_HighPerformance = _EMU_STATUS_VSCALE_VSCALE2,
  /** Low-power optimized voltage level. HF clock must be limited
      to @ref CMU_VSCALEEM01_LOWPOWER_VOLTAGE_CLOCK_MAX Hz at this voltage.
      EM0/1 voltage scaling is applied when core clock frequency is
      changed from @ref CMU or when calling @ref EMU_EM01Init() when HF
      clock is already below the limit. */
  emuVScaleEM01_LowPower        = _EMU_STATUS_VSCALE_VSCALE0,
} EMU_VScaleEM01_TypeDef;
#endif

#if defined(EMU_VSCALE_PRESENT)
/** Supported EM2/3 Voltage Scaling Levels. */
typedef enum {
  /** Fast-wakeup voltage level. */
  emuVScaleEM23_FastWakeup      = _EMU_CTRL_EM23VSCALE_VSCALE2,
  /** Low-power optimized voltage level. Using this voltage level in EM2 and 3
      adds approximately 30 us to wakeup time if EM0 and 1 voltage must be scaled
      up to @ref emuVScaleEM01_HighPerformance on EM2 or 3 exit. */
  emuVScaleEM23_LowPower        = _EMU_CTRL_EM23VSCALE_VSCALE0,
} EMU_VScaleEM23_TypeDef;
#endif

#if defined(_EMU_CTRL_EM4HVSCALE_MASK)
/** Supported EM4H Voltage Scaling Levels */
typedef enum {
  /** Fast-wakeup voltage level. */
  emuVScaleEM4H_FastWakeup      = _EMU_CTRL_EM4HVSCALE_VSCALE2,
  /** Low-power optimized voltage level. Using this voltage level in EM4H
      adds approximately 30 us to wakeup time if EM0 and 1 voltage must be scaled
      up to @ref emuVScaleEM01_HighPerformance on EM4H exit. */
  emuVScaleEM4H_LowPower        = _EMU_CTRL_EM4HVSCALE_VSCALE0,
} EMU_VScaleEM4H_TypeDef;
#endif

#if defined(_EMU_EM23PERNORETAINCTRL_MASK)
/** Peripheral EM2 and 3 retention control. */
typedef enum {
#if defined(_EMU_EM23PERNORETAINCTRL_USBDIS_MASK)
  emuPeripheralRetention_USB      = _EMU_EM23PERNORETAINCTRL_USBDIS_MASK,       /* Select USB retention control.  */
#endif
#if defined(_EMU_EM23PERNORETAINCTRL_RTCDIS_MASK)
  emuPeripheralRetention_RTC      = _EMU_EM23PERNORETAINCTRL_RTCDIS_MASK,       /* Select RTC retention control.  */
#endif
#if defined(_EMU_EM23PERNORETAINCTRL_ACMP3DIS_MASK)
  emuPeripheralRetention_ACMP3    = _EMU_EM23PERNORETAINCTRL_ACMP3DIS_MASK,     /* Select ACMP3 retention control. */
#endif
#if defined(_EMU_EM23PERNORETAINCTRL_ACMP2DIS_MASK)
  emuPeripheralRetention_ACMP2    = _EMU_EM23PERNORETAINCTRL_ACMP2DIS_MASK,     /* Select ACMP2 retention control.  */
#endif
#if defined(_EMU_EM23PERNORETAINCTRL_ADC1DIS_MASK)
  emuPeripheralRetention_ADC1     = _EMU_EM23PERNORETAINCTRL_ADC1DIS_MASK,      /* Select ADC1 retention control.  */
#endif
#if defined(_EMU_EM23PERNORETAINCTRL_I2C2DIS_MASK)
  emuPeripheralRetention_I2C2     = _EMU_EM23PERNORETAINCTRL_I2C2DIS_MASK,      /* Select I2C2 retention control.  */
#endif
#if defined(_EMU_EM23PERNORETAINCTRL_LETIMER1DIS_MASK)
  emuPeripheralRetention_LETIMER1 = _EMU_EM23PERNORETAINCTRL_LETIMER1DIS_MASK,  /* Select LETIMER1 retention control.  */
#endif
#if defined(_EMU_EM23PERNORETAINCTRL_LCDDIS_MASK)
  emuPeripheralRetention_LCD      = _EMU_EM23PERNORETAINCTRL_LCDDIS_MASK,       /* Select LCD retention control.  */
#endif
#if defined(_EMU_EM23PERNORETAINCTRL_LEUART1DIS_MASK)
  emuPeripheralRetention_LEUART1  = _EMU_EM23PERNORETAINCTRL_LEUART1DIS_MASK,   /* Select LEUART1 retention control.  */
#endif
  emuPeripheralRetention_LEUART0  = _EMU_EM23PERNORETAINCTRL_LEUART0DIS_MASK,   /* Select LEUART0 retention control.  */
#if defined(_EMU_EM23PERNORETAINCTRL_CSENDIS_MASK)
  emuPeripheralRetention_CSEN     = _EMU_EM23PERNORETAINCTRL_CSENDIS_MASK,      /* Select CSEN retention control.  */
#endif
  emuPeripheralRetention_LESENSE0 = _EMU_EM23PERNORETAINCTRL_LESENSE0DIS_MASK,  /* Select LESENSE0 retention control.  */
#if defined(_EMU_EM23PERNORETAINCTRL_WDOG1DIS_MASK)
  emuPeripheralRetention_WDOG1    = _EMU_EM23PERNORETAINCTRL_WDOG1DIS_MASK,     /* Select WDOG1 retention control.  */
#endif
  emuPeripheralRetention_WDOG0    = _EMU_EM23PERNORETAINCTRL_WDOG0DIS_MASK,     /* Select WDOG0 retention control.  */
  emuPeripheralRetention_LETIMER0 = _EMU_EM23PERNORETAINCTRL_LETIMER0DIS_MASK,  /* Select LETIMER0 retention control.  */
  emuPeripheralRetention_ADC0     = _EMU_EM23PERNORETAINCTRL_ADC0DIS_MASK,      /* Select ADC0 retention control.  */
#if defined(_EMU_EM23PERNORETAINCTRL_IDAC0DIS_MASK)
  emuPeripheralRetention_IDAC0    = _EMU_EM23PERNORETAINCTRL_IDAC0DIS_MASK,     /* Select IDAC0 retention control.  */
#endif
#if defined(_EMU_EM23PERNORETAINCTRL_VDAC0DIS_MASK)
  emuPeripheralRetention_VDAC0    = _EMU_EM23PERNORETAINCTRL_VDAC0DIS_MASK,     /* Select VDAC0 retention control.  */
#endif
#if defined(_EMU_EM23PERNORETAINCTRL_I2C1DIS_MASK)
  emuPeripheralRetention_I2C1     = _EMU_EM23PERNORETAINCTRL_I2C1DIS_MASK,      /* Select I2C1 retention control.  */
#endif
  emuPeripheralRetention_I2C0     = _EMU_EM23PERNORETAINCTRL_I2C0DIS_MASK,      /* Select I2C0 retention control.  */
  emuPeripheralRetention_ACMP1    = _EMU_EM23PERNORETAINCTRL_ACMP1DIS_MASK,     /* Select ACMP1 retention control.  */
  emuPeripheralRetention_ACMP0    = _EMU_EM23PERNORETAINCTRL_ACMP0DIS_MASK,     /* Select ACMP0 retention control.  */
#if defined(_EMU_EM23PERNORETAINCTRL_PCNT1DIS_MASK)
  emuPeripheralRetention_PCNT2    = _EMU_EM23PERNORETAINCTRL_PCNT2DIS_MASK,     /* Select PCNT2 retention control.  */
  emuPeripheralRetention_PCNT1    = _EMU_EM23PERNORETAINCTRL_PCNT1DIS_MASK,     /* Select PCNT1 retention control.  */
#endif
  emuPeripheralRetention_PCNT0    = _EMU_EM23PERNORETAINCTRL_PCNT0DIS_MASK,     /* Select PCNT0 retention control.  */

  emuPeripheralRetention_D1       = _EMU_EM23PERNORETAINCTRL_LETIMER0DIS_MASK
                                    | _EMU_EM23PERNORETAINCTRL_PCNT0DIS_MASK
                                    | _EMU_EM23PERNORETAINCTRL_ADC0DIS_MASK
                                    | _EMU_EM23PERNORETAINCTRL_ACMP0DIS_MASK
                                    | _EMU_EM23PERNORETAINCTRL_LESENSE0DIS_MASK,/* Select all peripherals in domain 1. */
  emuPeripheralRetention_D2       = _EMU_EM23PERNORETAINCTRL_ACMP1DIS_MASK
#if defined(_EMU_EM23PERNORETAINCTRL_IDAC0DIS_MASK)
                                    | _EMU_EM23PERNORETAINCTRL_IDAC0DIS_MASK
#endif
#if defined(_EMU_EM23PERNORETAINCTRL_VDAC0DIS_MASK)
                                    | _EMU_EM23PERNORETAINCTRL_VDAC0DIS_MASK
#endif
#if defined(_EMU_EM23PERNORETAINCTRL_CSENDIS_MASK)
                                    | _EMU_EM23PERNORETAINCTRL_CSENDIS_MASK
#endif
                                    | _EMU_EM23PERNORETAINCTRL_LEUART0DIS_MASK
#if defined(_EMU_EM23PERNORETAINCTRL_USBDIS_MASK)
                                    | _EMU_EM23PERNORETAINCTRL_USBDIS_MASK
#endif
#if defined(_EMU_EM23PERNORETAINCTRL_RTCDIS_MASK)
                                    | _EMU_EM23PERNORETAINCTRL_RTCDIS_MASK
#endif
#if defined(_EMU_EM23PERNORETAINCTRL_ACMP3DIS_MASK)
                                    | _EMU_EM23PERNORETAINCTRL_ACMP3DIS_MASK
#endif
#if defined(_EMU_EM23PERNORETAINCTRL_ACMP2DIS_MASK)
                                    | _EMU_EM23PERNORETAINCTRL_ACMP2DIS_MASK
#endif
#if defined(_EMU_EM23PERNORETAINCTRL_ADC1DIS_MASK)
                                    | _EMU_EM23PERNORETAINCTRL_ADC1DIS_MASK
#endif
#if defined(_EMU_EM23PERNORETAINCTRL_I2C2DIS_MASK)
                                    | _EMU_EM23PERNORETAINCTRL_I2C2DIS_MASK
#endif
#if defined(_EMU_EM23PERNORETAINCTRL_LETIMER1DIS_MASK)
                                    | _EMU_EM23PERNORETAINCTRL_LETIMER1DIS_MASK
#endif
#if defined(_EMU_EM23PERNORETAINCTRL_LCDDIS_MASK)
                                    | _EMU_EM23PERNORETAINCTRL_LCDDIS_MASK
#endif
#if defined(_EMU_EM23PERNORETAINCTRL_LEUART1DIS_MASK)
                                    | _EMU_EM23PERNORETAINCTRL_LEUART1DIS_MASK
#endif
#if defined(_EMU_EM23PERNORETAINCTRL_PCNT1DIS_MASK)
                                    | _EMU_EM23PERNORETAINCTRL_PCNT1DIS_MASK
                                    | _EMU_EM23PERNORETAINCTRL_PCNT2DIS_MASK
#endif
#if defined(_EMU_EM23PERNORETAINCTRL_I2C1DIS_MASK)
                                    | _EMU_EM23PERNORETAINCTRL_I2C1DIS_MASK    /* Select all peripherals in domain 2. */
#endif
                                    | _EMU_EM23PERNORETAINCTRL_I2C0DIS_MASK,
  emuPeripheralRetention_ALL       = emuPeripheralRetention_D1
                                     | emuPeripheralRetention_D2
#if defined(_EMU_EM23PERNORETAINCTRL_WDOG1DIS_MASK)
                                     | emuPeripheralRetention_WDOG1
#endif
                                     | emuPeripheralRetention_WDOG0,            /* Select all peripherals with retention control.  */
} EMU_PeripheralRetention_TypeDef;
#endif

/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/

#if defined(EMU_VSCALE_PRESENT)
/** EM0 and 1 initialization structure. Voltage scaling is applied when
    the core clock frequency is changed from @ref CMU. EM0 and 1 emuVScaleEM01_HighPerformance
    is always enabled. */
typedef struct {
  bool  vScaleEM01LowPowerVoltageEnable;                 /**< EM0/1 low power voltage status. */
} EMU_EM01Init_TypeDef;
#endif

#if defined(EMU_VSCALE_PRESENT)
/** Default initialization of EM0 and 1 configuration. */
#define EMU_EM01INIT_DEFAULT                                                               \
  {                                                                                        \
    false                                                /** Do not scale down in EM0/1.*/ \
  }
#endif

/** EM2 and 3 initialization structure.  */
typedef struct {
  bool                          em23VregFullEn;         /**< Enable full VREG drive strength in EM2/3. */
#if defined(EMU_VSCALE_PRESENT)
  EMU_VScaleEM23_TypeDef        vScaleEM23Voltage;      /**< EM2/3 voltage scaling level. */
#endif
} EMU_EM23Init_TypeDef;

/** Default initialization of EM2 and 3 configuration. */
#if defined(EMU_VSCALE_PRESENT)
#define EMU_EM23INIT_DEFAULT                                                                                    \
  {                                                                                                             \
    false,                                              /* Reduced voltage regulator drive strength in EM2/3.*/ \
    emuVScaleEM23_FastWakeup,                           /* Do not scale down in EM2/3. */                       \
  }
#else
#define EMU_EM23INIT_DEFAULT                                                                                    \
  {                                                                                                             \
    false,                                              /* Reduced voltage regulator drive strength in EM2/3.*/ \
  }
#endif

#if defined(_EMU_EM4CONF_MASK) || defined(_EMU_EM4CTRL_MASK)
/** EM4 initialization structure.  */
typedef struct {
#if defined(_EMU_EM4CONF_MASK)
  /* Initialization parameters for platforms with EMU->EM4CONF register (Series 0). */
  bool                        lockConfig;       /**< Lock configuration of regulator, BOD and oscillator. */
  bool                        buBodRstDis;      /**< When set, no reset will be asserted due to Brownout when in EM4. */
  EMU_EM4Osc_TypeDef          osc;              /**< EM4 duty oscillator. */
  bool                        buRtcWakeup;      /**< Wake up on EM4 BURTC interrupt. */
  bool                        vreg;             /**< Enable EM4 voltage regulator. */
#elif defined(_EMU_EM4CTRL_MASK)
  /* Initialization parameters for platforms with EMU->EM4CTRL register (Series 1). */
  bool                        retainLfxo;       /**< Disable LFXO upon EM4 entry. */
  bool                        retainLfrco;      /**< Disable LFRCO upon EM4 entry. */
  bool                        retainUlfrco;     /**< Disable ULFRCO upon EM4 entry. */
  EMU_EM4State_TypeDef        em4State;         /**< Hibernate or shutoff EM4 state. */
  EMU_EM4PinRetention_TypeDef pinRetentionMode; /**< EM4 pin retention mode. */
#endif
#if defined(_EMU_CTRL_EM4HVSCALE_MASK)
  EMU_VScaleEM4H_TypeDef      vScaleEM4HVoltage;/**< EM4H voltage scaling level. */
#endif
} EMU_EM4Init_TypeDef;
#endif

#if defined(_EMU_EM4CONF_MASK)
/** Default initialization of EM4 configuration (Series 0). */
#define EMU_EM4INIT_DEFAULT                                                                \
  {                                                                                        \
    false,                            /* Do not lock configuration after it's been set. */ \
    false,                            /* No reset will be asserted due to BOD in EM4. */   \
    emuEM4Osc_ULFRCO,                 /* Use default ULFRCO oscillator.  */                \
    true,                             /* Wake up on EM4 BURTC interrupt. */                \
    true,                             /* Enable VREG. */                                   \
  }

#elif defined(_EMU_CTRL_EM4HVSCALE_MASK)
/** Default initialization of EM4 configuration (Series 1 with VSCALE). */
#define EMU_EM4INIT_DEFAULT                                                            \
  {                                                                                    \
    false,                           /* Retain LFXO configuration upon EM4 entry. */   \
    false,                           /* Retain LFRCO configuration upon EM4 entry. */  \
    false,                           /* Retain ULFRCO configuration upon EM4 entry. */ \
    emuEM4Shutoff,                   /* Use EM4 shutoff state. */                      \
    emuPinRetentionDisable,          /* Do not retain pins in EM4. */                  \
    emuVScaleEM4H_FastWakeup,        /* Do not scale down in EM4H. */                  \
  }

#elif defined(_EMU_EM4CTRL_MASK)
/** Default initialization of EM4 configuration (Series 1 without VSCALE). */
#define EMU_EM4INIT_DEFAULT                                                            \
  {                                                                                    \
    false,                           /* Retain LFXO configuration upon EM4 entry. */   \
    false,                           /* Retain LFRCO configuration upon EM4 entry. */  \
    false,                           /* Retain ULFRCO configuration upon EM4 entry. */ \
    emuEM4Shutoff,                   /* Use EM4 shutoff state. */                      \
    emuPinRetentionDisable,          /* Do not retain pins in EM4. */                  \
  }
#endif

#if defined(BU_PRESENT) && defined(_SILICON_LABS_32B_SERIES_0)
/** Backup Power Domain Initialization structure. */
typedef struct {
  /* Backup Power Domain power configuration. */

  /** Voltage probe select, selects ADC voltage. */
  EMU_Probe_TypeDef     probe;
  /** Enable BOD calibration mode. */
  bool                  bodCal;
  /** Enable BU_STAT status pin for active BU mode. */
  bool                  statusPinEnable;

  /* Backup Power Domain connection configuration. */
  /** Power domain resistor. */
  EMU_Resistor_TypeDef  resistor;
  /** BU_VOUT strong enable. */
  bool                  voutStrong;
  /** BU_VOUT medium enable. */
  bool                  voutMed;
  /** BU_VOUT weak enable. */
  bool                  voutWeak;
  /** Power connection, when not in Backup Mode. */
  EMU_Power_TypeDef  inactivePower;
  /** Power connection, when in Backup Mode. */
  EMU_Power_TypeDef     activePower;
  /** Enable backup power domain, and release reset, enable BU_VIN pin.  */
  bool                  enable;
} EMU_BUPDInit_TypeDef;

/** Default Backup Power Domain configuration. */
#define EMU_BUPDINIT_DEFAULT                                                              \
  {                                                                                       \
    emuProbe_Disable, /* Do not enable voltage probe. */                                  \
    false,          /* Disable BOD calibration mode. */                                   \
    false,          /* Disable BU_STAT pin for backup mode indication. */                 \
                                                                                          \
    emuRes_Res0,    /* RES0 series resistance between main and backup power. */           \
    false,          /* Do not enable strong switch. */                                    \
    false,          /* Do not enable medium switch. */                                    \
    false,          /* Do not enable weak switch. */                                      \
                                                                                          \
    emuPower_None,  /* No connection between main and backup power. (inactive mode) */    \
    emuPower_None,  /* No connection between main and backup power. (active mode) */      \
    true            /* Enable BUPD enter on BOD, enable BU_VIN pin, release BU reset.  */ \
  }
#endif

#if defined(BU_PRESENT) && defined(_SILICON_LABS_32B_SERIES_1)
/** Backup Power Domain Initialization structure. */
typedef struct {
  bool disMaxComp;                            /**< Disable MAIN-BU Comparator. */
  EMU_BUBuInactPwrCon_TypeDef inactivePwrCon; /**< Power connection configuration when not in backup mode. */
  EMU_BUBuActPwrCon_TypeDef activePwrCon;     /**< Power connection configuration when in backup mode. */
  EMU_BUPwrRes_TypeDef pwrRes;                /**< Power domain resistor. */
  EMU_BUVoutRes_TypeDef voutRes;              /**< BU_VOUT resistor select. */
  bool buVinProbeEn;                          /**< Enable BU_VIN probing. */
  bool staEn;                                 /**< Enable backup mode status export. */
  bool enable;                                /**< Enable backup power domain. */
} EMU_BUInit_TypeDef;

/** Default Backup Power Domain configuration. */
#define EMU_BUINIT_DEFAULT                                                                 \
  {                                                                                        \
    false,                   /**< MAIN-BU Comparator is not disabloed */                   \
    emuBuBuInactPwrCon_None, /**< No power connection wen not in backup mode */            \
    emuBuBuActPwrCon_None,   /**< No power connection when in backup mode */               \
    emuBuPwrRes_Res0,        /**< RES0 series resistance between main and backup power. */ \
    emuBuVoutRes_Dis,        /**< Vout resistor is set to not connected */                 \
    false,                   /**< BU_VIN probe is disabled */                              \
    false,                   /**< Status export is disabled */                             \
    true                     /**< Enable backup mode */                                    \
  }
#endif

#if defined(_EMU_DCDCCTRL_MASK)
/** DCDC initialization structure. */
typedef struct {
  EMU_PowerConfig_TypeDef powerConfig;                  /**< Device external power configuration.
                                                             @ref emuPowerConfig_DcdcToDvdd is currently the only supported mode. */
  EMU_DcdcMode_TypeDef dcdcMode;                        /**< DCDC regulator operating mode in EM0/1. */
  uint16_t mVout;                                       /**< Target output voltage (mV). */
  uint16_t em01LoadCurrent_mA;                          /**< Estimated average load current in EM0/1. (mA).
                                                             This estimate is also used for EM1 optimization;
                                                             if EM1 current is expected to be higher than EM0,
                                                             then this parameter should hold the higher EM1 current. */
  uint16_t em234LoadCurrent_uA;                         /**< Estimated average load current in EM2 (uA).
                                                             This estimate is also used for EM3 and 4 optimization;
                                                             if EM3 or 4 current is expected to be higher than EM2,
                                                             then this parameter should hold the higher EM3 or 4 current. */
  uint16_t maxCurrent_mA;                               /**< Maximum average DCDC output current (mA).
                                                             This can be set to the maximum for the power source,
                                                             for example the maximum for a battery. */
  EMU_DcdcAnaPeripheralPower_TypeDef
    anaPeripheralPower;                                 /**< Select analog peripheral power in DCDC-to-DVDD mode. */
  EMU_DcdcLnReverseCurrentControl_TypeDef
    reverseCurrentControl;                              /**< Low-noise reverse current control.
                                                             NOTE: this parameter uses special encoding:
                                                             >= 0 is forced CCM mode where the parameter is used as the
                                                                  reverse current threshold in mA.
                                                             -1   is encoded as emuDcdcLnHighEfficiencyMode (EFM32 only). */
  EMU_DcdcLnCompCtrl_TypeDef dcdcLnCompCtrl;            /**< DCDC Low-noise mode compensator control. */
} EMU_DCDCInit_TypeDef;

/** Default DCDC initialization. */
#if defined(_EFM_DEVICE)
#if defined(_SILICON_LABS_GECKO_INTERNAL_SDID_80)
#define EMU_DCDCINIT_DEFAULT                                                         \
  {                                                                                  \
    emuPowerConfig_DcdcToDvdd,   /* DCDC to DVDD. */                                 \
    emuDcdcMode_LowNoise,        /* Low-noise mode in EM0. */                        \
    1800,                        /* Nominal output voltage for DVDD mode, 1.8V.  */  \
    5,                           /* Nominal EM0/1 load current of less than 5mA. */  \
    10,                          /* Nominal EM2/3/4 load current less than 10uA.  */ \
    200,                         /* Maximum average current of 200mA
                                    (assume strong battery or other power source). */      \
    emuDcdcAnaPeripheralPower_DCDC,/* Select DCDC as analog power supply (lower power). */ \
    emuDcdcLnHighEfficiency,     /* Use high-efficiency mode. */                           \
    emuDcdcLnCompCtrl_1u0F,      /* 1uF DCDC capacitor. */                                 \
  }
#else
#define EMU_DCDCINIT_DEFAULT                                                         \
  {                                                                                  \
    emuPowerConfig_DcdcToDvdd,   /* DCDC to DVDD. */                                 \
    emuDcdcMode_LowPower,        /* Low-power mode in EM0. */                        \
    1800,                        /* Nominal output voltage for DVDD mode, 1.8V.  */  \
    5,                           /* Nominal EM0/1 load current of less than 5mA. */  \
    10,                          /* Nominal EM2/3/4 load current less than 10uA.  */ \
    200,                         /* Maximum average current of 200mA
                                    (assume strong battery or other power source). */ \
    emuDcdcAnaPeripheralPower_AVDD,/* Select AVDD as analog power supply). */         \
    emuDcdcLnHighEfficiency,     /* Use high-efficiency mode. */                      \
    emuDcdcLnCompCtrl_4u7F,      /* 4.7uF DCDC capacitor. */                          \
  }
#endif

#else /* EFR32 device. */
#if defined(_SILICON_LABS_GECKO_INTERNAL_SDID_80)
#define EMU_DCDCINIT_DEFAULT                                                         \
  {                                                                                  \
    emuPowerConfig_DcdcToDvdd,   /* DCDC to DVDD. */                                 \
    emuDcdcMode_LowNoise,        /* Low-noise mode in EM0. */                        \
    1800,                        /* Nominal output voltage for DVDD mode, 1.8V.  */  \
    15,                          /* Nominal EM0/1 load current of less than 15mA. */ \
    10,                          /* Nominal EM2/3/4 load current less than 10uA.  */ \
    200,                         /* Maximum average current of 200mA
                                    (assume strong battery or other power source). */      \
    emuDcdcAnaPeripheralPower_DCDC,/* Select DCDC as analog power supply (lower power). */ \
    160,                         /* Maximum reverse current of 160mA. */                   \
    emuDcdcLnCompCtrl_1u0F,      /* 1uF DCDC capacitor. */                                 \
  }
#else
#define EMU_DCDCINIT_DEFAULT                                                         \
  {                                                                                  \
    emuPowerConfig_DcdcToDvdd,   /* DCDC to DVDD. */                                 \
    emuDcdcMode_LowNoise,        /* Low-noise mode in EM0. */                        \
    1800,                        /* Nominal output voltage for DVDD mode, 1.8V.  */  \
    15,                          /* Nominal EM0/1 load current of less than 15mA. */ \
    10,                          /* Nominal EM2/3/4 load current less than 10uA.  */ \
    200,                         /* Maximum average current of 200mA
                                    (assume strong battery or other power source). */      \
    emuDcdcAnaPeripheralPower_DCDC,/* Select DCDC as analog power supply (lower power). */ \
    160,                         /* Maximum reverse current of 160mA. */                   \
    emuDcdcLnCompCtrl_4u7F,      /* 4.7uF DCDC capacitor. */                               \
  }
#endif
#endif
#endif

#if defined(EMU_STATUS_VMONRDY)
/** VMON initialization structure. */
typedef struct {
  EMU_VmonChannel_TypeDef channel;      /**< VMON channel to configure. */
  int threshold;                        /**< Trigger threshold (mV). Supported range is 1620 mV to 3400 mV. */
  bool riseWakeup;                      /**< Wake up from EM4H on rising edge. */
  bool fallWakeup;                      /**< Wake up from EM4H on falling edge. */
  bool enable;                          /**< Enable VMON channel. */
  bool retDisable;                      /**< Disable IO0 retention when voltage drops below threshold (IOVDD only). */
} EMU_VmonInit_TypeDef;

/** Default VMON initialization structure. */
#define EMU_VMONINIT_DEFAULT                                                         \
  {                                                                                  \
    emuVmonChannel_AVDD,                /* AVDD VMON channel. */                     \
    3200,                               /* 3.2 V threshold. */                       \
    false,                              /* Do not wake from EM4H on rising edge. */  \
    false,                              /* Do not wake from EM4H on falling edge. */ \
    true,                               /* Enable VMON channel. */                   \
    false                               /* Do not disable IO0 retention  */          \
  }

/** VMON Hysteresis initialization structure. */
typedef struct {
  EMU_VmonChannel_TypeDef channel;      /**< VMON channel to configure. */
  int riseThreshold;                    /**< Rising threshold (mV). */
  int fallThreshold;                    /**< Falling threshold (mV). */
  bool riseWakeup;                      /**< Wake up from EM4H on rising edge. */
  bool fallWakeup;                      /**< Wake up from EM4H on falling edge. */
  bool enable;                          /**< Enable VMON channel. */
} EMU_VmonHystInit_TypeDef;

/** Default VMON Hysteresis initialization structure. */
#define EMU_VMONHYSTINIT_DEFAULT                                                     \
  {                                                                                  \
    emuVmonChannel_AVDD,                /* AVDD VMON channel. */                     \
    3200,                               /* 3.2 V rise threshold. */                  \
    3200,                               /* 3.2 V fall threshold. */                  \
    false,                              /* Do not wake from EM4H on rising edge. */  \
    false,                              /* Do not wake from EM4H on falling edge. */ \
    true                                /* Enable VMON channel. */                   \
  }
#endif /* EMU_STATUS_VMONRDY */

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

#if defined(EMU_VSCALE_PRESENT)
void EMU_EM01Init(const EMU_EM01Init_TypeDef *em01Init);
#endif
void EMU_EM23Init(const EMU_EM23Init_TypeDef *em23Init);
#if defined(_EMU_EM4CONF_MASK) || defined(_EMU_EM4CTRL_MASK)
void EMU_EM4Init(const EMU_EM4Init_TypeDef *em4Init);
#endif
void EMU_EnterEM2(bool restore);
void EMU_EnterEM3(bool restore);
void EMU_Save(void);
void EMU_Restore(void);
void EMU_EnterEM4(void);
#if defined(_EMU_EM4CTRL_MASK)
void EMU_EnterEM4H(void);
void EMU_EnterEM4S(void);
#endif
void EMU_MemPwrDown(uint32_t blocks);
void EMU_RamPowerDown(uint32_t start, uint32_t end);
void EMU_RamPowerUp(void);
#if defined(_EMU_EM23PERNORETAINCTRL_MASK)
void EMU_PeripheralRetention(EMU_PeripheralRetention_TypeDef periMask, bool enable);
#endif
void EMU_UpdateOscConfig(void);
#if defined(EMU_VSCALE_PRESENT)
void EMU_VScaleEM01ByClock(uint32_t clockFrequency, bool wait);
void EMU_VScaleEM01(EMU_VScaleEM01_TypeDef voltage, bool wait);
#endif
#if defined(BU_PRESENT) && defined(_SILICON_LABS_32B_SERIES_0)
void EMU_BUPDInit(const EMU_BUPDInit_TypeDef *bupdInit);
void EMU_BUThresholdSet(EMU_BODMode_TypeDef mode, uint32_t value);
void EMU_BUThresRangeSet(EMU_BODMode_TypeDef mode, uint32_t value);
#endif
#if defined(BU_PRESENT) && defined(_SILICON_LABS_32B_SERIES_1)
void EMU_BUInit(const EMU_BUInit_TypeDef *buInit);
#endif
#if defined(_EMU_BUCTRL_DISMAXCOMP_MASK)
void EMU_BUDisMaxCompSet(bool disableMainBuComparator);
#endif
#if defined(_EMU_BUCTRL_BUINACTPWRCON_MASK)
void EMU_BUBuInactPwrConSet(EMU_BUBuInactPwrCon_TypeDef inactPwrCon);
#endif
#if defined(_EMU_BUCTRL_BUACTPWRCON_MASK)
void EMU_BUBuActPwrConSet(EMU_BUBuActPwrCon_TypeDef actPwrCon);
#endif
#if defined(_EMU_BUCTRL_PWRRES_MASK)
void EMU_BUPwrResSet(EMU_BUPwrRes_TypeDef pwrRes);
#endif
#if defined(_EMU_BUCTRL_VOUTRES_MASK)
void EMU_BUVoutResSet(EMU_BUVoutRes_TypeDef resistorSel);
#endif
#if defined(_EMU_BUCTRL_BUVINPROBEEN_MASK)
void EMU_BUBuVinProbeEnSet(bool enable);
#endif
#if defined(_EMU_BUCTRL_STATEN_MASK)
void EMU_BUStatEnSet(bool enable);
#endif
#if defined(_EMU_BUCTRL_EN_MASK)
void EMU_BUEnableSet(bool enable);
#endif
#if defined(_EMU_DCDCCTRL_MASK)
bool EMU_DCDCInit(const EMU_DCDCInit_TypeDef *dcdcInit);
void EMU_DCDCModeSet(EMU_DcdcMode_TypeDef dcdcMode);
void EMU_DCDCConductionModeSet(EMU_DcdcConductionMode_TypeDef conductionMode, bool rcoDefaultSet);
bool EMU_DCDCOutputVoltageSet(uint32_t mV, bool setLpVoltage, bool setLnVoltage);
void EMU_DCDCOptimizeSlice(uint32_t em0LoadCurrentmA);
void EMU_DCDCLnRcoBandSet(EMU_DcdcLnRcoBand_TypeDef band);
bool EMU_DCDCPowerOff(void);
#endif
#if defined(EMU_STATUS_VMONRDY)
void EMU_VmonInit(const EMU_VmonInit_TypeDef *vmonInit);
void EMU_VmonHystInit(const EMU_VmonHystInit_TypeDef *vmonInit);
void EMU_VmonEnable(EMU_VmonChannel_TypeDef channel, bool enable);
bool EMU_VmonChannelStatusGet(EMU_VmonChannel_TypeDef channel);
#endif

/***************************************************************************//**
 * @brief
 *   Enter energy mode 1 (EM1).
 ******************************************************************************/
__STATIC_INLINE void EMU_EnterEM1(void)
{
  /* Enter sleep mode. */
  SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
  __WFI();
}

#if defined(EMU_VSCALE_PRESENT)
/***************************************************************************//**
 * @brief
 *   Wait for voltage scaling to complete.
 ******************************************************************************/
__STATIC_INLINE void EMU_VScaleWait(void)
{
  while (BUS_RegBitRead(&EMU->STATUS, _EMU_STATUS_VSCALEBUSY_SHIFT) != 0U) {
  }
}
#endif

#if defined(EMU_VSCALE_PRESENT)
/***************************************************************************//**
 * @brief
 *   Get current voltage scaling level.
 *
 * @return
 *    Current voltage scaling level.
 ******************************************************************************/
__STATIC_INLINE EMU_VScaleEM01_TypeDef EMU_VScaleGet(void)
{
  EMU_VScaleWait();
  return (EMU_VScaleEM01_TypeDef)((uint32_t)
                                  ((EMU->STATUS & _EMU_STATUS_VSCALE_MASK)
                                   >> _EMU_STATUS_VSCALE_SHIFT));
}
#endif

#if defined(_EMU_STATUS_VMONRDY_MASK)
/***************************************************************************//**
 * @brief
 *   Get the status of the voltage monitor (VMON).
 *
 * @return
 *   Status of the VMON. True if all the enabled channels are ready, false if
 *   one or more of the enabled channels are not ready.
 ******************************************************************************/
__STATIC_INLINE bool EMU_VmonStatusGet(void)
{
  return BUS_RegBitRead(&EMU->STATUS, _EMU_STATUS_VMONRDY_SHIFT) != 0U;
}
#endif /* _EMU_STATUS_VMONRDY_MASK */

#if defined(_EMU_IF_MASK)
/***************************************************************************//**
 * @brief
 *   Clear one or more pending EMU interrupts.
 *
 * @param[in] flags
 *   Pending EMU interrupt sources to clear. Use one or more valid
 *   interrupt flags for the EMU module (EMU_IFC_nnn).
 ******************************************************************************/
__STATIC_INLINE void EMU_IntClear(uint32_t flags)
{
#if defined(EMU_HAS_SET_CLEAR)
  EMU->IF_CLR = flags;
#else
  EMU->IFC = flags;
#endif
}

/***************************************************************************//**
 * @brief
 *   Disable one or more EMU interrupts.
 *
 * @param[in] flags
 *   EMU interrupt sources to disable. Use one or more valid
 *   interrupt flags for the EMU module (EMU_IEN_nnn).
 ******************************************************************************/
__STATIC_INLINE void EMU_IntDisable(uint32_t flags)
{
#if defined(EMU_HAS_SET_CLEAR)
  EMU->IEN_CLR = flags;
#else
  EMU->IEN &= ~flags;
#endif
}

/***************************************************************************//**
 * @brief
 *   Enable one or more EMU interrupts.
 *
 * @note
 *   Depending on the use, a pending interrupt may already be set prior to
 *   enabling the interrupt. To ignore a pending interrupt, consider using
 *   @ref EMU_IntClear() prior to enabling the interrupt.
 *
 * @param[in] flags
 *   EMU interrupt sources to enable. Use one or more valid
 *   interrupt flags for the EMU module (EMU_IEN_nnn).
 ******************************************************************************/
__STATIC_INLINE void EMU_IntEnable(uint32_t flags)
{
#if defined(EMU_HAS_SET_CLEAR)
  EMU->IEN_SET = flags;
#else
  EMU->IEN |= flags;
#endif
}

/***************************************************************************//**
 * @brief
 *   Get pending EMU interrupt flags.
 *
 * @note
 *   Event bits are not cleared by the use of this function.
 *
 * @return
 *   EMU interrupt sources pending. Returns one or more valid
 *   interrupt flags for the EMU module (EMU_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE uint32_t EMU_IntGet(void)
{
  return EMU->IF;
}

/***************************************************************************//**
 * @brief
 *   Get enabled and pending EMU interrupt flags.
 *   Useful for handling more interrupt sources in the same interrupt handler.
 *
 * @note
 *   Interrupt flags are not cleared by the use of this function.
 *
 * @return
 *   Pending and enabled EMU interrupt sources
 *   Return value is the bitwise AND of
 *   - the enabled interrupt sources in EMU_IEN and
 *   - the pending interrupt flags EMU_IF.
 ******************************************************************************/
__STATIC_INLINE uint32_t EMU_IntGetEnabled(void)
{
  uint32_t ien;

  ien = EMU->IEN;
  return EMU->IF & ien;
}

/***************************************************************************//**
 * @brief
 *   Set one or more pending EMU interrupts.
 *
 * @param[in] flags
 *   EMU interrupt sources to set to pending. Use one or more valid
 *   interrupt flags for the EMU module (EMU_IFS_nnn).
 ******************************************************************************/
__STATIC_INLINE void EMU_IntSet(uint32_t flags)
{
#if defined(EMU_HAS_SET_CLEAR)
  EMU->IF_SET = flags;
#else
  EMU->IFS = flags;
#endif
}
#endif /* _EMU_IF_MASK */

#if defined(_EMU_EM4CONF_LOCKCONF_MASK)
/***************************************************************************//**
 * @brief
 *   Enable or disable EM4 lock configuration.
 * @param[in] enable
 *   If true, locks down EM4 configuration.
 ******************************************************************************/
__STATIC_INLINE void EMU_EM4Lock(bool enable)
{
  BUS_RegBitWrite(&(EMU->EM4CONF), _EMU_EM4CONF_LOCKCONF_SHIFT, enable);
}
#endif

#if defined(_EMU_STATUS_BURDY_MASK)
/***************************************************************************//**
 * @brief
 *   Halts until backup power functionality is ready.
 ******************************************************************************/
__STATIC_INLINE void EMU_BUReady(void)
{
  while (!(EMU->STATUS & EMU_STATUS_BURDY))
    ;
}
#endif

#if defined(_EMU_ROUTE_BUVINPEN_MASK)
/***************************************************************************//**
 * @brief
 *   Disable BU_VIN support.
 * @param[in] enable
 *   If true, enables BU_VIN input pin support, if false disables it.
 ******************************************************************************/
__STATIC_INLINE void EMU_BUPinEnable(bool enable)
{
  BUS_RegBitWrite(&(EMU->ROUTE), _EMU_ROUTE_BUVINPEN_SHIFT, enable);
}
#endif

/***************************************************************************//**
 * @brief
 *   Lock EMU registers in order to protect them against unintended
 *   modification.
 *
 * @note
 *   If locking EMU registers, they must be unlocked prior to using any
 *   EMU API functions modifying EMU registers, excluding interrupt control
 *   and regulator control if the architecture has a EMU_PWRCTRL register.
 *   An exception to this is the energy mode entering API (EMU_EnterEMn()),
 *   which can be used when the EMU registers are locked.
 ******************************************************************************/
__STATIC_INLINE void EMU_Lock(void)
{
  EMU->LOCK = 0x0;
}

/***************************************************************************//**
 * @brief
 *   Unlock the EMU so that writing to locked registers again is possible.
 ******************************************************************************/
__STATIC_INLINE void EMU_Unlock(void)
{
  EMU->LOCK = EMU_LOCK_LOCKKEY_UNLOCK;
}

#if defined(_EMU_PWRLOCK_MASK)
/***************************************************************************//**
 * @brief
 *   Lock the EMU regulator control registers in order to protect against
 *   unintended modification.
 ******************************************************************************/
__STATIC_INLINE void EMU_PowerLock(void)
{
  EMU->PWRLOCK = EMU_PWRLOCK_LOCKKEY_LOCK;
}

/***************************************************************************//**
 * @brief
 *   Unlock the EMU power control registers so that writing to
 *   locked registers again is possible.
 ******************************************************************************/
__STATIC_INLINE void EMU_PowerUnlock(void)
{
  EMU->PWRLOCK = EMU_PWRLOCK_LOCKKEY_UNLOCK;
}
#endif

#if defined(_EMU_CTRL_EM2BLOCK_MASK)
/***************************************************************************//**
 * @brief
 *   Block entering EM2 or higher number energy modes.
 ******************************************************************************/
__STATIC_INLINE void EMU_EM2Block(void)
{
  BUS_RegBitWrite(&EMU->CTRL, _EMU_CTRL_EM2BLOCK_SHIFT, 1U);
}

/***************************************************************************//**
 * @brief
 *   Unblock entering EM2 or higher number energy modes.
 ******************************************************************************/
__STATIC_INLINE void EMU_EM2UnBlock(void)
{
  BUS_RegBitWrite(&EMU->CTRL, _EMU_CTRL_EM2BLOCK_SHIFT, 0U);
}
#endif

#if defined(_EMU_EM4CTRL_EM4IORETMODE_MASK)
/***************************************************************************//**
 * @brief
 *   When EM4 pin retention is set to emuPinRetentionLatch, then pins are retained
 *   through EM4 entry and wakeup. The pin state is released by calling this function.
 *   The feature allows peripherals or GPIO to be re-initialized after EM4 exit (reset),
 *   and when initialization is done, this function can release pins and return control
 *   to the peripherals or GPIO.
 ******************************************************************************/
__STATIC_INLINE void EMU_UnlatchPinRetention(void)
{
  EMU->CMD = EMU_CMD_EM4UNLATCH;
}
#endif

#if defined(_SILICON_LABS_GECKO_INTERNAL_SDID_80)
void EMU_SetBiasMode(EMU_BiasMode_TypeDef mode);
#endif

/** @} (end addtogroup EMU) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* defined( EMU_PRESENT ) */
#endif /* EM_EMU_H */
