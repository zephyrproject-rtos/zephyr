/***************************************************************************//**
 * @file em_lcd.h
 * @brief Liquid Crystal Display (LCD) peripheral API
 * @version 5.6.0
 *******************************************************************************
 * # License
 * <b>Copyright 2017 Silicon Laboratories, Inc. www.silabs.com</b>
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

#ifndef EM_LCD_H
#define EM_LCD_H

#include "em_device.h"

#if defined(LCD_COUNT) && (LCD_COUNT > 0)
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup LCD
 * @{
 ******************************************************************************/

/*******************************************************************************
 ********************************   DEFINES   **********************************
 ******************************************************************************/

#define LCD_DEFAULT_FRAME_RATE_DIV  4
#define LCD_DEFAULT_CONTRAST        15

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

/** MUX setting. */
typedef enum {
  /** Static (segments can be multiplexed with LCD_COM[0]). */
  lcdMuxStatic     = LCD_DISPCTRL_MUX_STATIC,
  /** Duplex / 1/2 Duty cycle (segments can be multiplexed with LCD_COM[0:1]). */
  lcdMuxDuplex     = LCD_DISPCTRL_MUX_DUPLEX,
  /** Triplex / 1/3 Duty cycle (segments can be multiplexed with LCD_COM[0:2]). */
  lcdMuxTriplex    = LCD_DISPCTRL_MUX_TRIPLEX,
  /** Quadruplex / 1/4 Duty cycle (segments can be multiplexed with LCD_COM[0:3]). */
  lcdMuxQuadruplex = LCD_DISPCTRL_MUX_QUADRUPLEX,
#if defined(LCD_DISPCTRL_MUXE_MUXE)
  /** Sextaplex / 1/6 Duty cycle (segments can be multiplexed with LCD_COM[0:5]). */
  lcdMuxSextaplex  = LCD_DISPCTRL_MUXE_MUXE | LCD_DISPCTRL_MUX_DUPLEX,
  /** Octaplex / 1/6 Duty cycle (segments can be multiplexed with LCD_COM[0:5]). */
  lcdMuxOctaplex   = LCD_DISPCTRL_MUXE_MUXE | LCD_DISPCTRL_MUX_QUADRUPLEX
#elif defined(LCD_DISPCTRL_MUX_SEXTAPLEX)
  /** Sextaplex / 1/6 Duty cycle (segments can be multiplexed with LCD_COM[0:5]). */
  lcdMuxSextaplex  = LCD_DISPCTRL_MUX_SEXTAPLEX,
  /** Octaplex / 1/6 Duty cycle (segments can be multiplexed with LCD_COM[0:5]). */
  lcdMuxOctaplex   = LCD_DISPCTRL_MUX_OCTAPLEX,
#endif
} LCD_Mux_TypeDef;

/** Wave type. */
typedef enum {
  /** Low power optimized waveform output. */
  lcdWaveLowPower = LCD_DISPCTRL_WAVE_LOWPOWER,
  /** Regular waveform output */
  lcdWaveNormal   = LCD_DISPCTRL_WAVE_NORMAL
} LCD_Wave_TypeDef;

/** Bias setting. */
typedef enum {
  /** Static (2 levels). */
  lcdBiasStatic    = LCD_DISPCTRL_BIAS_STATIC,
  /** 1/2 Bias (3 levels). */
  lcdBiasOneHalf   = LCD_DISPCTRL_BIAS_ONEHALF,
  /** 1/3 Bias (4 levels). */
  lcdBiasOneThird  = LCD_DISPCTRL_BIAS_ONETHIRD,
#if defined(LCD_DISPCTRL_BIAS_ONEFOURTH)
  /** 1/4 Bias (5 levels). */
  lcdBiasOneFourth = LCD_DISPCTRL_BIAS_ONEFOURTH,
#endif
} LCD_Bias_TypeDef;

#if defined(_SILICON_LABS_32B_SERIES_0)
/** VLCD Voltage Source. */
typedef enum {
  /** VLCD Powered by VDD. */
  lcdVLCDSelVDD       = LCD_DISPCTRL_VLCDSEL_VDD,
  /** VLCD Powered by external VDD / Voltage Boost. */
  lcdVLCDSelVExtBoost = LCD_DISPCTRL_VLCDSEL_VEXTBOOST
} LCD_VLCDSel_TypeDef;
#endif

/** Contrast Configuration. */
#if defined(_SILICON_LABS_32B_SERIES_0)
typedef enum {
  /** Contrast is adjusted relative to VDD (VLCD). */
  lcdConConfVLCD = LCD_DISPCTRL_CONCONF_VLCD,
  /** Contrast is adjusted relative to Ground. */
  lcdConConfGND  = LCD_DISPCTRL_CONCONF_GND
} LCD_ConConf_TypeDef;
#endif

#if defined(_SILICON_LABS_32B_SERIES_0)
/** Voltage Boost Level - Data sheets document setting for each part number. */
typedef enum {
  lcdVBoostLevel0 = LCD_DISPCTRL_VBLEV_LEVEL0, /**< Voltage boost LEVEL0. */
  lcdVBoostLevel1 = LCD_DISPCTRL_VBLEV_LEVEL1, /**< Voltage boost LEVEL1. */
  lcdVBoostLevel2 = LCD_DISPCTRL_VBLEV_LEVEL2, /**< Voltage boost LEVEL2. */
  lcdVBoostLevel3 = LCD_DISPCTRL_VBLEV_LEVEL3, /**< Voltage boost LEVEL3. */
  lcdVBoostLevel4 = LCD_DISPCTRL_VBLEV_LEVEL4, /**< Voltage boost LEVEL4. */
  lcdVBoostLevel5 = LCD_DISPCTRL_VBLEV_LEVEL5, /**< Voltage boost LEVEL5. */
  lcdVBoostLevel6 = LCD_DISPCTRL_VBLEV_LEVEL6, /**< Voltage boost LEVEL6. */
  lcdVBoostLevel7 = LCD_DISPCTRL_VBLEV_LEVEL7  /**< Voltage boost LEVEL7. */
} LCD_VBoostLevel_TypeDef;
#endif

#if defined(_SILICON_LABS_32B_SERIES_1)
/** Mode. */
typedef enum {
  lcdModeNoExtCap = LCD_DISPCTRL_MODE_NOEXTCAP, /**< No external capacitor. */
  lcdModeStepDown = LCD_DISPCTRL_MODE_STEPDOWN, /**< External cap with resistor string. */
  lcdModeCpIntOsc = LCD_DISPCTRL_MODE_CPINTOSC, /**< External cap and internal oscillator. */
} LCD_Mode_Typedef;
#endif

/** Frame Counter Clock Prescaler, FC-CLK = FrameRate (Hz) / this factor. */
typedef enum {
  /** Prescale Div 1. */
  lcdFCPrescDiv1 = LCD_BACTRL_FCPRESC_DIV1,
  /** Prescale Div 2. */
  lcdFCPrescDiv2 = LCD_BACTRL_FCPRESC_DIV2,
  /** Prescale Div 4. */
  lcdFCPrescDiv4 = LCD_BACTRL_FCPRESC_DIV4,
  /** Prescale Div 8. */
  lcdFCPrescDiv8 = LCD_BACTRL_FCPRESC_DIV8
} LCD_FCPreScale_TypeDef;

#if defined(_SILICON_LABS_32B_SERIES_0)
/** Segment selection. */
typedef enum {
  /** Select segment lines 0 to 3. */
  lcdSegment0_3   = (1 << 0),
  /** Select segment lines 4 to 7. */
  lcdSegment4_7   = (1 << 1),
  /** Select segment lines 8 to 11. */
  lcdSegment8_11  = (1 << 2),
  /** Select segment lines 12 to 15. */
  lcdSegment12_15 = (1 << 3),
  /** Select segment lines 16 to 19. */
  lcdSegment16_19 = (1 << 4),
  /** Select segment lines 20 to 23. */
  lcdSegment20_23 = (1 << 5),
#if defined(_LCD_SEGD0L_MASK) && (_LCD_SEGD0L_MASK == 0x00FFFFFFUL)
  /** Select all segment lines. */
  lcdSegmentAll   = (0x003f)
#elif defined(_LCD_SEGD0H_MASK) && (_LCD_SEGD0H_MASK == 0x000000FFUL)
  /** Select segment lines 24 to 27. */
  lcdSegment24_27 = (1 << 6),
  /** Select segment lines 28 to 31. */
  lcdSegment28_31 = (1 << 7),
  /** Select segment lines 32 to 35. */
  lcdSegment32_35 = (1 << 8),
  /** Select segment lines 36 to 39. */
  lcdSegment36_39 = (1 << 9),
  /** Select all segment lines. */
  lcdSegmentAll   = (0x03ff)
#endif
} LCD_SegmentRange_TypeDef;
#endif

/** Update Data Control. */
typedef enum {
  /** Regular update, data transfer done immediately. */
  lcdUpdateCtrlRegular    = LCD_CTRL_UDCTRL_REGULAR,
  /** Data transfer done at Frame Counter event. */
  lcdUpdateCtrlFCEvent    = LCD_CTRL_UDCTRL_FCEVENT,
  /** Data transfer done at Frame Start.  */
  lcdUpdateCtrlFrameStart = LCD_CTRL_UDCTRL_FRAMESTART
} LCD_UpdateCtrl_TypeDef;

/** Animation Shift operation; none, left or right. */
typedef enum {
  /** No shift. */
  lcdAnimShiftNone  = _LCD_BACTRL_AREGASC_NOSHIFT,
  /** Shift segment bits left. */
  lcdAnimShiftLeft  = _LCD_BACTRL_AREGASC_SHIFTLEFT,
  /** Shift segment bits right. */
  lcdAnimShiftRight = _LCD_BACTRL_AREGASC_SHIFTRIGHT
} LCD_AnimShift_TypeDef;

/** Animation Logic Control, how AReg and BReg should be combined. */
typedef enum {
  /** Use bitwise logic AND to mix animation register A (AREGA) and B (AREGB). */
  lcdAnimLogicAnd = LCD_BACTRL_ALOGSEL_AND,
  /** Use bitwise logic OR to mix animation register A (AREGA) and B (AREGB).  */
  lcdAnimLogicOr  = LCD_BACTRL_ALOGSEL_OR
} LCD_AnimLogic_TypeDef;

#if defined(_LCD_DISPCTRL_CHGRDST_MASK)
/** Charge redistribution control. */
typedef enum {
  /** Disable charge redistribution. */
  lcdChargeRedistributionDisable = LCD_DISPCTRL_CHGRDST_DISABLE,
  /** Enable charge redistribution. */
  lcdChargeRedistributionEnable = LCD_DISPCTRL_CHGRDST_ONE
} LCD_ChargeRedistribution_TypeDef;
#endif

/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/

/** LCD Animation Configuration. */
typedef struct {
  /** Enable Animation at end of initialization. */
  bool                  enable;
  /** Initial Animation Register A Value. */
  uint32_t              AReg;
  /** Shift operation of Animation Register A. */
  LCD_AnimShift_TypeDef AShift;
  /** Initial Animation Register B Value. */
  uint32_t              BReg;
  /** Shift operation of Animation Register B. */
  LCD_AnimShift_TypeDef BShift;
  /** A and B Logical Operation to use for mixing and outputting resulting segments. */
  LCD_AnimLogic_TypeDef animLogic;
#if defined(LCD_BACTRL_ALOC)
  /** Number of first segment to animate. Options are 0 or 8 for Giant/Leopard. End is startSeg+7. */
  int                   startSeg;
#endif
} LCD_AnimInit_TypeDef;

/** LCD Frame Control Initialization. */
typedef struct {
  /** Enable at end. */
  bool                   enable;
  /** Frame Counter top value. */
  uint32_t               top;
  /** Frame Counter clock prescaler. */
  LCD_FCPreScale_TypeDef prescale;
} LCD_FrameCountInit_TypeDef;

/** LCD Controller Initialization structure. */
typedef struct {
  /** Enable controller at end of initialization. */
  bool                enable;
  /** Mux configuration. */
  LCD_Mux_TypeDef     mux;
  /** Bias configuration. */
  LCD_Bias_TypeDef    bias;
  /** Wave configuration. */
  LCD_Wave_TypeDef    wave;
#if defined(_SILICON_LABS_32B_SERIES_0)
  /** VLCD Select. */
  LCD_VLCDSel_TypeDef vlcd;
  /** Contrast Configuration. */
  LCD_ConConf_TypeDef contrast;
#endif
#if defined(_SILICON_LABS_32B_SERIES_1)
  /** Mode. */
  LCD_Mode_Typedef                      mode;
  LCD_ChargeRedistribution_TypeDef      chargeRedistribution;
  uint8_t                               frameRateDivider;
  int                                   contrastLevel;
#endif
} LCD_Init_TypeDef;

/** Default configuration for LCD initialization structure, enables 160 segments.  */
#if defined(_SILICON_LABS_32B_SERIES_0)
#define LCD_INIT_DEFAULT \
  {                      \
    true,                \
    lcdMuxQuadruplex,    \
    lcdBiasOneThird,     \
    lcdWaveLowPower,     \
    lcdVLCDSelVDD,       \
    lcdConConfVLCD,      \
  }
#endif

#if defined(_SILICON_LABS_32B_SERIES_1)
#define LCD_INIT_DEFAULT           \
  {                                \
    true,                          \
    lcdMuxOctaplex,                \
    lcdBiasOneFourth,              \
    lcdWaveLowPower,               \
    lcdModeNoExtCap,               \
    lcdChargeRedistributionEnable, \
    LCD_DEFAULT_FRAME_RATE_DIV,    \
    LCD_DEFAULT_CONTRAST           \
  }
#endif

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

void LCD_Init(const LCD_Init_TypeDef *lcdInit);
#if defined(_SILICON_LABS_32B_SERIES_0)
void LCD_VLCDSelect(LCD_VLCDSel_TypeDef vlcd);
#endif
void LCD_UpdateCtrl(LCD_UpdateCtrl_TypeDef ud);
void LCD_FrameCountInit(const LCD_FrameCountInit_TypeDef *fcInit);
void LCD_AnimInit(const LCD_AnimInit_TypeDef *animInit);

#if defined(_SILICON_LABS_32B_SERIES_0)
void LCD_SegmentRangeEnable(LCD_SegmentRange_TypeDef segment, bool enable);
#endif
void LCD_SegmentSet(int com, int bit, bool enable);
void LCD_SegmentSetLow(int com, uint32_t mask, uint32_t bits);
#if defined(_LCD_SEGD0H_MASK)
void LCD_SegmentSetHigh(int com, uint32_t mask, uint32_t bits);
#endif
void LCD_ContrastSet(int level);
void LCD_BiasSet(LCD_Bias_TypeDef bias);
#if defined(_SILICON_LABS_32B_SERIES_0)
void LCD_VBoostSet(LCD_VBoostLevel_TypeDef vboost);
#endif
#if defined(LCD_CTRL_DSC)
void LCD_BiasSegmentSet(int segment, int biasLevel);
void LCD_BiasComSet(int com, int biasLevel);
#endif
#if defined(_SILICON_LABS_32B_SERIES_1)
void LCD_ModeSet(LCD_Mode_Typedef mode);
void LCD_ChargeRedistributionCyclesSet(uint8_t cycles);
#endif

/***************************************************************************//**
 * @brief
 *   Enable or disable LCD controller.
 *
 * @param[in] enable
 *   If true, enables LCD controller with current configuration. If false,
 *   disables LCD controller. Enable CMU clock for LCD for correct
 *   operation.
 ******************************************************************************/
__STATIC_INLINE void LCD_Enable(bool enable)
{
  if (enable) {
    LCD->CTRL |= LCD_CTRL_EN;
  } else {
    LCD->CTRL &= ~LCD_CTRL_EN;
  }
}

/***************************************************************************//**
 * @brief
 *   Enables or disables LCD Animation feature.
 *
 * @param[in] enable
 *   Boolean true enables animation, false disables animation.
 ******************************************************************************/
__STATIC_INLINE void LCD_AnimEnable(bool enable)
{
  if (enable) {
    LCD->BACTRL |= LCD_BACTRL_AEN;
  } else {
    LCD->BACTRL &= ~LCD_BACTRL_AEN;
  }
}

/***************************************************************************//**
 * @brief
 *   Enables or disables LCD blink.
 *
 * @param[in] enable
 *   Boolean true enables blink, false disables blink.
 ******************************************************************************/
__STATIC_INLINE void LCD_BlinkEnable(bool enable)
{
  if (enable) {
    LCD->BACTRL |= LCD_BACTRL_BLINKEN;
  } else {
    LCD->BACTRL &= ~LCD_BACTRL_BLINKEN;
  }
}

/***************************************************************************//**
 * @brief
 *   Disables all segments, while keeping segment state.
 *
 * @param[in] enable
 *   Boolean true clears all segments, boolean false restores all segment lines.
 ******************************************************************************/
__STATIC_INLINE void LCD_BlankEnable(bool enable)
{
  if (enable) {
    LCD->BACTRL |= LCD_BACTRL_BLANK;
  } else {
    LCD->BACTRL &= ~LCD_BACTRL_BLANK;
  }
}

/***************************************************************************//**
 * @brief
 *   Enables or disables LCD Frame Control.
 *
 * @param[in] enable
 *   Boolean true enables frame counter, false disables frame counter.
 ******************************************************************************/
__STATIC_INLINE void LCD_FrameCountEnable(bool enable)
{
  if (enable) {
    LCD->BACTRL |= LCD_BACTRL_FCEN;
  } else {
    LCD->BACTRL &= ~LCD_BACTRL_FCEN;
  }
}

/***************************************************************************//**
 * @brief
 *   Returns current animation state.
 *
 * @return
 *   Animation state, in range 0-15.
 ******************************************************************************/
__STATIC_INLINE int LCD_AnimState(void)
{
  return (int)(LCD->STATUS & _LCD_STATUS_ASTATE_MASK) >> _LCD_STATUS_ASTATE_SHIFT;
}

/***************************************************************************//**
 * @brief
 *   Returns current blink state.
 *
 * @return
 *   Return value is 1 if segments are enabled, 0 if disabled.
 ******************************************************************************/
__STATIC_INLINE int LCD_BlinkState(void)
{
  return (int)(LCD->STATUS & _LCD_STATUS_BLINK_MASK) >> _LCD_STATUS_BLINK_SHIFT;
}

/***************************************************************************//**
 * @brief
 *   When set, LCD registers will not be updated until cleared.
 *
 * @param[in] enable
 *   When enable is true, update is stopped; when false, all registers are
 *   updated.
 ******************************************************************************/
__STATIC_INLINE void LCD_FreezeEnable(bool enable)
{
  if (enable) {
    LCD->FREEZE = LCD_FREEZE_REGFREEZE_FREEZE;
  } else {
    LCD->FREEZE = LCD_FREEZE_REGFREEZE_UPDATE;
  }
}

/***************************************************************************//**
 * @brief
 *   Returns SYNCBUSY bits, indicating which registers have pending updates.
 *
 * @return
 *   Bit fields for LCD registers that have pending updates.
 ******************************************************************************/
__STATIC_INLINE uint32_t LCD_SyncBusyGet(void)
{
  return LCD->SYNCBUSY;
}

/***************************************************************************//**
 * @brief
 *   Polls LCD SYNCBUSY flags, until flag has been cleared.
 *
 * @param[in] flags
 *   Bit fields for LCD registers that will be updated before we continue.
 ******************************************************************************/
__STATIC_INLINE void LCD_SyncBusyDelay(uint32_t flags)
{
  while (LCD->SYNCBUSY & flags)
    ;
}

/***************************************************************************//**
 * @brief
 *    Get pending LCD interrupt flags.
 *
 * @return
 *   Pending LCD interrupt sources. Returns a set of interrupt flags OR-ed
 *   together for multiple interrupt sources in the LCD module (LCD_IFS_nnn).
 ******************************************************************************/
__STATIC_INLINE uint32_t LCD_IntGet(void)
{
  return LCD->IF;
}

/***************************************************************************//**
 * @brief
 *   Get enabled and pending LCD interrupt flags.
 *
 * @details
 *   Useful for handling more interrupt sources in the same interrupt handler.
 *
 * @note
 *   Event bits are not cleared by the use of this function.
 *
 * @return
 *   Pending and enabled LCD interrupt sources.
 *   Return value is the bitwise AND combination of
 *   - the OR combination of enabled interrupt sources in LCD_IEN_nnn
 *   register (LCD_IEN_nnn) and
 *   - the bitwise OR combination of valid interrupt flags of LCD module
 *   (LCD_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE uint32_t LCD_IntGetEnabled(void)
{
  uint32_t ien;

  /* Stores LCD->IEN in temporary variable in order to define explicit order
   * of volatile accesses. */
  ien = LCD->IEN;

  /* Bitwise AND of pending and enabled interrupts */
  return LCD->IF & ien;
}

/***************************************************************************//**
 * @brief
 *    Set one or more pending LCD interrupts from SW.
 *
 * @param[in] flags
 *   LCD interrupt sources to set to pending. Use a set of interrupt flags
 *   OR-ed together to set multiple interrupt sources for the LCD module
 *   (LCD_IFS_nnn).
 ******************************************************************************/
__STATIC_INLINE void LCD_IntSet(uint32_t flags)
{
  LCD->IFS = flags;
}

/***************************************************************************//**
 * @brief
 *    Enable LCD interrupts.
 *
 * @param[in] flags
 *   LCD interrupt sources to enable. Use a set of interrupt flags OR-ed
 *   together to set multiple interrupt sources for LCD module
 *   (LCD_IFS_nnn).
 ******************************************************************************/
__STATIC_INLINE void LCD_IntEnable(uint32_t flags)
{
  LCD->IEN |= flags;
}

/***************************************************************************//**
 * @brief
 *    Disable LCD interrupts.
 *
 * @param[in] flags
 *   LCD interrupt sources to disable. Use a set of interrupt flags OR-ed
 *   together to disable multiple interrupt sources for LCD module
 *   (LCD_IFS_nnn).
 ******************************************************************************/
__STATIC_INLINE void LCD_IntDisable(uint32_t flags)
{
  LCD->IEN &= ~flags;
}

/***************************************************************************//**
 * @brief
 *   Clear one or more interrupt flags.
 *
 * @param[in] flags
 *   LCD interrupt sources to clear. Use a set of interrupt flags OR-ed
 *   together to clear multiple interrupt sources for LCD module
 *   (LCD_IFS_nnn).
 ******************************************************************************/
__STATIC_INLINE void LCD_IntClear(uint32_t flags)
{
  LCD->IFC = flags;
}

#if defined(LCD_CTRL_DSC)
/***************************************************************************//**
 * @brief
 *   Enable or disable LCD Direct Segment Control.
 *
 * @param[in] enable
 *   If true, enables LCD controller Direct Segment Control
 *   Segment and COM line bias levels need to be set explicitly with
 *   LCD_BiasSegmentSet() and LCD_BiasComSet() function calls respectively.
 ******************************************************************************/
__STATIC_INLINE void LCD_DSCEnable(bool enable)
{
  if (enable) {
    LCD->CTRL |= LCD_CTRL_DSC;
  } else {
    LCD->CTRL &= ~LCD_CTRL_DSC;
  }
}
#endif

/** @} (end addtogroup LCD) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* defined(LCD_COUNT) && (LCD_COUNT > 0) */

#endif /* EM_LCD_H */
