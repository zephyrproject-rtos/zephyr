/***************************************************************************//**
 * @file em_lcd.c
 * @brief Liquid Crystal Display (LCD) Peripheral API
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

#include "em_lcd.h"
#if defined(LCD_COUNT) && (LCD_COUNT > 0)
#include "em_assert.h"
#include "em_bus.h"

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup LCD
 * @brief Liquid Crystal Display (LCD) Peripheral API
 * @details
 *  This module contains functions to control the LDC peripheral of Silicon
 *  Labs 32-bit MCUs and SoCs. The LCD driver can drive up to 8x36 segmented
 *  LCD directly. The animation feature makes it possible to have active
 *  animations without the CPU intervention.
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @brief
 *   Initialize the Liquid Crystal Display (LCD) controller.
 *
 * @details
 *   Configures the LCD controller. You must enable
 *   it afterwards, potentially configuring Frame Control and interrupts first
 *   according to requirements.
 *
 * @param[in] lcdInit
 *   A pointer to the initialization structure which configures the LCD controller.
 *
 ******************************************************************************/
void LCD_Init(const LCD_Init_TypeDef *lcdInit)
{
  uint32_t dispCtrl = LCD->DISPCTRL;

  EFM_ASSERT(lcdInit != (void *) 0);

  /* Disable the controller before reconfiguration. */
  LCD_Enable(false);

  /* Make sure the other bit fields don't get affected (i.e., voltage boost). */
  dispCtrl &= ~(0
#if defined(LCD_DISPCTRL_MUXE)
                | _LCD_DISPCTRL_MUXE_MASK
#endif
                | _LCD_DISPCTRL_MUX_MASK
                | _LCD_DISPCTRL_BIAS_MASK
                | _LCD_DISPCTRL_WAVE_MASK
#if defined(_LCD_DISPCTRL_VLCDSEL_MASK)
                | _LCD_DISPCTRL_VLCDSEL_MASK
#endif
#if defined(_LCD_DISPCTRL_CONCONF_MASK)
                | _LCD_DISPCTRL_CONCONF_MASK
#endif
#if defined(_LCD_DISPCTRL_MODE_MASK)
                | _LCD_DISPCTRL_MODE_MASK
#endif
#if defined(_LCD_DISPCTRL_CHGRDST_MASK)
                | _LCD_DISPCTRL_CHGRDST_MASK
#endif
                );

  /* Configure the controller according to the initialization structure. */
  dispCtrl |= lcdInit->mux; /* Also configures MUXE. */
  dispCtrl |= lcdInit->bias;
  dispCtrl |= lcdInit->wave;
#if defined(_SILICON_LABS_32B_SERIES_0)
  dispCtrl |= lcdInit->vlcd;
  dispCtrl |= lcdInit->contrast;
#endif
#if defined(_SILICON_LABS_32B_SERIES_1)
  dispCtrl |= lcdInit->mode;
  dispCtrl |= (lcdInit->chargeRedistribution);
#endif

  /* Update the display controller. */
  LCD->DISPCTRL = dispCtrl;

#if defined(_SILICON_LABS_32B_SERIES_1)
  LCD->FRAMERATE = lcdInit->frameRateDivider;
  LCD_ContrastSet(lcdInit->contrastLevel);
#endif

  /* Enable the controller if needed. */
  if (lcdInit->enable) {
    LCD_Enable(true);
  }
}

#if defined(_SILICON_LABS_32B_SERIES_0)
/***************************************************************************//**
 * @brief
 *   Select a source for VLCD.
 *
 * @param[in] vlcd
 *   Select a source for the VLCD voltage.
 ******************************************************************************/
void LCD_VLCDSelect(LCD_VLCDSel_TypeDef vlcd)
{
  uint32_t dispctrl = LCD->DISPCTRL;

  /* Select VEXT or VDD */
  dispctrl &= ~_LCD_DISPCTRL_VLCDSEL_MASK;
  switch (vlcd) {
    case lcdVLCDSelVExtBoost:
      dispctrl |= LCD_DISPCTRL_VLCDSEL_VEXTBOOST;
      break;
    case lcdVLCDSelVDD:
      dispctrl |= LCD_DISPCTRL_VLCDSEL_VDD;
      break;
    default:
      break;
  }

  LCD->DISPCTRL = dispctrl;
}
#endif

/***************************************************************************//**
 * @brief
 *   Configure Update Control.
 *
 * @param[in] ud
 *   Configures the LCD update method.
 ******************************************************************************/
void LCD_UpdateCtrl(LCD_UpdateCtrl_TypeDef ud)
{
  LCD->CTRL = (LCD->CTRL & ~_LCD_CTRL_UDCTRL_MASK) | ud;
}

/***************************************************************************//**
 * @brief
 *   Initialize the LCD Frame Counter.
 *
 * @param[in] fcInit
 *   A pointer to the Frame Counter initialization structure.
 ******************************************************************************/
void LCD_FrameCountInit(const LCD_FrameCountInit_TypeDef *fcInit)
{
  uint32_t bactrl = LCD->BACTRL;

  EFM_ASSERT(fcInit != (void *) 0);

  /* Verify that the FC Top Counter is within limits. */
  EFM_ASSERT(fcInit->top < 64);

  /* Reconfigure the frame count configuration. */
  bactrl &= ~(_LCD_BACTRL_FCTOP_MASK
              | _LCD_BACTRL_FCPRESC_MASK);
  bactrl |= (fcInit->top << _LCD_BACTRL_FCTOP_SHIFT);
  bactrl |= fcInit->prescale;

  /* Set the Blink and Animation Control Register. */
  LCD->BACTRL = bactrl;

  LCD_FrameCountEnable(fcInit->enable);
}

/***************************************************************************//**
 * @brief
 *   Configure the LCD controller Animation feature.
 *
 * @param[in] animInit
 *   A pointer to the LCD Animation initialization structure.
 ******************************************************************************/
void LCD_AnimInit(const LCD_AnimInit_TypeDef *animInit)
{
  uint32_t bactrl = LCD->BACTRL;

  EFM_ASSERT(animInit != (void *) 0);

  /* Set Animation Register Values. */
  LCD->AREGA = animInit->AReg;
  LCD->AREGB = animInit->BReg;

  /* Configure the Animation Shift and Logic. */
  bactrl &= ~(_LCD_BACTRL_AREGASC_MASK
              | _LCD_BACTRL_AREGBSC_MASK
              | _LCD_BACTRL_ALOGSEL_MASK);

  bactrl |= (animInit->AShift << _LCD_BACTRL_AREGASC_SHIFT);
  bactrl |= (animInit->BShift << _LCD_BACTRL_AREGBSC_SHIFT);
  bactrl |= animInit->animLogic;

#if defined(LCD_BACTRL_ALOC)
  bactrl &= ~(_LCD_BACTRL_ALOC_MASK);

  if (animInit->startSeg == 0) {
    bactrl |= LCD_BACTRL_ALOC_SEG0TO7;
  } else if (animInit->startSeg == 8) {
    bactrl |= LCD_BACTRL_ALOC_SEG8TO15;
  }
#endif

  /* Reconfigure. */
  LCD->BACTRL = bactrl;

  /* Enable. */
  LCD_AnimEnable(animInit->enable);
}

/***************************************************************************//**
 * @brief
 *   Enables updating this range of LCD segment lines.
 *
 * @param[in] segmentRange
 *   A range of 4 LCD segment lines to enable or disable for all enabled COM
 *   lines.
 *
 * @param[in] enable
 *   Boolean true to enable segment updates, false to disable updates.
 ******************************************************************************/
#if defined(_SILICON_LABS_32B_SERIES_0)
void LCD_SegmentRangeEnable(LCD_SegmentRange_TypeDef segmentRange, bool enable)
{
  if (enable) {
    LCD->SEGEN |= segmentRange;
  } else {
    LCD->SEGEN &= ~((uint32_t)segmentRange);
  }
}
#endif

/***************************************************************************//**
 * @brief
 *   Turn on or clear a segment.
 *
 * @note
 *    For the Gecko Family, the maximum configuration is (COM-lines x Segment-Lines) 4x40.
 *    For the Tiny Gecko Family, the maximum configuration is 8x20 or 4x24.
 *    For the Giant Gecko Family, the maximum configuration is 8x36 or 4x40.
 *
 * @param[in] com
 *   A COM line to change.
 *
 * @param[in] bit
 *   A bit index indicating which field to change.
 *
 * @param[in] enable
 *   True will set segment, false will clear segment.
 ******************************************************************************/
void LCD_SegmentSet(int com, int bit, bool enable)
{
#if defined(_LCD_SEGD7L_MASK)
  /* Tiny Gecko and Giant Gecko Families support up to 8 COM lines. */
  EFM_ASSERT(com < 8);
#else
  /* Gecko Family supports up to 4 COM lines. */
  EFM_ASSERT(com < 4);
#endif

#if defined(_LCD_SEGD0H_MASK)
  EFM_ASSERT(bit < 40);
#else
  /* Tiny Gecko Family supports only "low" segment registers. */
  EFM_ASSERT(bit < 32);
#endif

  /* Use a bitband access for atomic bit set/clear of the segment. */
  switch (com) {
    case 0:
      if (bit < 32) {
        BUS_RegBitWrite(&(LCD->SEGD0L), bit, enable);
      }
#if defined(_LCD_SEGD0H_MASK)
      else {
        bit -= 32;
        BUS_RegBitWrite(&(LCD->SEGD0H), bit, enable);
      }
#endif
      break;
    case 1:
      if (bit < 32) {
        BUS_RegBitWrite(&(LCD->SEGD1L), bit, enable);
      }
#if defined(_LCD_SEGD1H_MASK)
      else {
        bit -= 32;
        BUS_RegBitWrite(&(LCD->SEGD1H), bit, enable);
      }
#endif
      break;
    case 2:
      if (bit < 32) {
        BUS_RegBitWrite(&(LCD->SEGD2L), bit, enable);
      }
#if defined(_LCD_SEGD2H_MASK)
      else {
        bit -= 32;
        BUS_RegBitWrite(&(LCD->SEGD2H), bit, enable);
      }
#endif
      break;
    case 3:
      if (bit < 32) {
        BUS_RegBitWrite(&(LCD->SEGD3L), bit, enable);
      }
#if defined(_LCD_SEGD3H_MASK)
      else {
        bit -= 32;
        BUS_RegBitWrite(&(LCD->SEGD3H), bit, enable);
      }
#endif
      break;
#if defined(_LCD_SEGD4L_MASK)
    case 4:
      if (bit < 32) {
        BUS_RegBitWrite(&(LCD->SEGD4L), bit, enable);
      }
#if defined(_LCD_SEGD4H_MASK)
      else {
        bit -= 32;
        BUS_RegBitWrite(&(LCD->SEGD4H), bit, enable);
      }
#endif
      break;
#endif
#if defined(_LCD_SEGD5L_MASK)
    case 5:
      if (bit < 32) {
        BUS_RegBitWrite(&(LCD->SEGD5L), bit, enable);
      }
#if defined(_LCD_SEGD5H_MASK)
      else {
        bit -= 32;
        BUS_RegBitWrite(&(LCD->SEGD5H), bit, enable);
      }
#endif
      break;
#endif
    case 6:
#if defined(_LCD_SEGD6L_MASK)
      if (bit < 32) {
        BUS_RegBitWrite(&(LCD->SEGD6L), bit, enable);
      }
#if defined(_LCD_SEGD6H_MASK)
      else {
        bit -= 32;
        BUS_RegBitWrite(&(LCD->SEGD6H), bit, enable);
      }
#endif
      break;
#endif
#if defined(_LCD_SEGD7L_MASK)
    case 7:
      if (bit < 32) {
        BUS_RegBitWrite(&(LCD->SEGD7L), bit, enable);
      }
#if defined(_LCD_SEGD7H_MASK)
      else {
        bit -= 32;
        BUS_RegBitWrite(&(LCD->SEGD7H), bit, enable);
      }
#endif
      break;
#endif

    default:
      EFM_ASSERT(0);
      break;
  }
}

/***************************************************************************//**
 * @brief
 *   Update 0-31 lowest segments on a given COM-line in one operation
 *   according to the bit mask.
 *
 * @param[in] com
 *   Indicates a COM line to update.
 *
 * @param[in] mask
 *   A bit mask for segments 0-31.
 *
 * @param[in] bits
 *   A bit pattern for segments 0-31.
 ******************************************************************************/
void LCD_SegmentSetLow(int com, uint32_t mask, uint32_t bits)
{
  uint32_t segData;

  /* A maximum number of com lines. */
#if defined(_LCD_SEGD7L_MASK)
  EFM_ASSERT(com < 8);
#else
  /* Gecko Family supports up to 4 COM lines. */
  EFM_ASSERT(com < 4);
#endif

  switch (com) {
    case 0:
      segData     = LCD->SEGD0L;
      segData    &= ~(mask);
      segData    |= (mask & bits);
      LCD->SEGD0L = segData;
      break;
    case 1:
      segData     = LCD->SEGD1L;
      segData    &= ~(mask);
      segData    |= (mask & bits);
      LCD->SEGD1L = segData;
      break;
    case 2:
      segData     = LCD->SEGD2L;
      segData    &= ~(mask);
      segData    |= (mask & bits);
      LCD->SEGD2L = segData;
      break;
    case 3:
      segData     = LCD->SEGD3L;
      segData    &= ~(mask);
      segData    |= (mask & bits);
      LCD->SEGD3L = segData;
      break;
#if defined(_LCD_SEGD4L_MASK)
    case 4:
      segData     = LCD->SEGD4L;
      segData    &= ~(mask);
      segData    |= (mask & bits);
      LCD->SEGD4L = segData;
      break;
#endif
#if defined(_LCD_SEGD5L_MASK)
    case 5:
      segData     = LCD->SEGD5L;
      segData    &= ~(mask);
      segData    |= (mask & bits);
      LCD->SEGD5L = segData;
      break;
#endif
#if defined(_LCD_SEGD6L_MASK)
    case 6:
      segData     = LCD->SEGD6L;
      segData    &= ~(mask);
      segData    |= (mask & bits);
      LCD->SEGD6L = segData;
      break;
#endif
#if defined(_LCD_SEGD7L_MASK)
    case 7:
      segData     = LCD->SEGD7L;
      segData    &= ~(mask);
      segData    |= (mask & bits);
      LCD->SEGD7L = segData;
      break;
#endif
    default:
      EFM_ASSERT(0);
      break;
  }
}

#if defined(_LCD_SEGD0H_MASK)
/***************************************************************************//**
 * @brief
 *   Update the high (32-39) segments on a given COM-line in one operation.
 *
 * @param[in] com
 *   Indicates a COM line to update.
 *
 * @param[in] mask
 *   A bit mask for segments 32-39.
 *
 * @param[in] bits
 *   A bit pattern for segments 32-39.
 ******************************************************************************/
void LCD_SegmentSetHigh(int com, uint32_t mask, uint32_t bits)
{
  uint32_t segData;

#if defined(_LCD_SEGD7H_MASK)
  EFM_ASSERT(com < 8);
#else
  EFM_ASSERT(com < 4);
#endif

  /* A maximum number of com lines. */
  switch (com) {
    case 0:
      segData     = LCD->SEGD0H;
      segData    &= ~(mask);
      segData    |= (mask & bits);
      LCD->SEGD0H = segData;
      break;
    case 1:
      segData     = LCD->SEGD1H;
      segData    &= ~(mask);
      segData    |= (mask & bits);
      LCD->SEGD1H = segData;
      break;
    case 2:
      segData     = LCD->SEGD2H;
      segData    &= ~(mask);
      segData    |= (mask & bits);
      LCD->SEGD2H = segData;
      break;
    case 3:
      segData     = LCD->SEGD3H;
      segData    &= ~(mask);
      segData    |= (mask & bits);
      LCD->SEGD3H = segData;
      break;
#if defined(_LCD_SEGD4H_MASK)
    case 4:
      segData     = LCD->SEGD4H;
      segData    &= ~(mask);
      segData    |= (mask & bits);
      LCD->SEGD4H = segData;
      break;
#endif
#if defined(_LCD_SEGD5H_MASK)
    case 5:
      segData     = LCD->SEGD5H;
      segData    &= ~(mask);
      segData    |= (mask & bits);
      LCD->SEGD5H = segData;
      break;
#endif
#if defined(_LCD_SEGD6H_MASK)
    case 6:
      segData     = LCD->SEGD6H;
      segData    &= ~(mask);
      segData    |= (mask & bits);
      LCD->SEGD6H = segData;
      break;
#endif
#if defined(_LCD_SEGD7H_MASK)
    case 7:
      segData     = LCD->SEGD7H;
      segData    &= ~(mask);
      segData    |= (mask & bits);
      LCD->SEGD7H = segData;
      break;
#endif
    default:
      break;
  }
}
#endif

#if defined(_SILICON_LABS_32B_SERIES_0)
/***************************************************************************//**
 * @brief
 *   Configure the contrast level on the LCD panel.
 *
 * @param[in] level
 *   The contrast level in range 0-31.
 ******************************************************************************/
void LCD_ContrastSet(int level)
{
  EFM_ASSERT(level < 32);

  LCD->DISPCTRL = (LCD->DISPCTRL & ~_LCD_DISPCTRL_CONLEV_MASK)
                  | (level << _LCD_DISPCTRL_CONLEV_SHIFT);
}
#endif

#if defined(_SILICON_LABS_32B_SERIES_1)
/***************************************************************************//**
 * @brief
 *   Configure the contrast level on the LCD panel.
 *
 * @param[in] level
 *   The contrast level in range 0-63.
 ******************************************************************************/
void LCD_ContrastSet(int level)
{
  EFM_ASSERT(level < 64);

  LCD->DISPCTRL = (LCD->DISPCTRL & ~_LCD_DISPCTRL_CONTRAST_MASK)
                  | (level << _LCD_DISPCTRL_CONTRAST_SHIFT);
}
#endif

/***************************************************************************//**
 * @brief
 *   Configure the bias level on the LCD panel.
 *
 * @param[in] bias
 *   The bias level.
 ******************************************************************************/
void LCD_BiasSet(LCD_Bias_TypeDef bias)
{
  LCD->DISPCTRL = (LCD->DISPCTRL & ~_LCD_DISPCTRL_BIAS_MASK) | bias;
}

#if defined(_SILICON_LABS_32B_SERIES_0)
/***************************************************************************//**
 * @brief
 *   Configure voltage booster
 *
 * The resulting voltage level is described in each part number's data sheet
 *
 * @param[in] vboost
 *   Voltage boost level
 ******************************************************************************/
void LCD_VBoostSet(LCD_VBoostLevel_TypeDef vboost)
{
  /* Reconfigure Voltage Boost */
  LCD->DISPCTRL = (LCD->DISPCTRL & ~_LCD_DISPCTRL_VBLEV_MASK) | vboost;
}
#endif

#if defined(LCD_CTRL_DSC)
/***************************************************************************//**
 * @brief
 *   Configure the bias level for a specific segment line for Direct Segment Control.
 *
 * @note
 *   When DSC is active, each configuration takes up 4 bits in the Segment
 *   Registers (SEGD0L/SEGD1H) which defines the bias level.
 *   For optimal use of this feature, the entire SEGD-registers should be set
 *   at once in an optimized routine. Therefore, this function
 *   shows how to correctly configure the bias levels and should be used
 *   with care.
 *
 * @param[in] segmentLine
 *   A segment line number.
 *
 * @param[in] biasLevel
 *   The bias configuration level, 0-4. This value must be within the constraints
 *   defined by the LCD_DISPCTRL bias settings. For more information,
 *   see the applicable Reference Manual and data sheet.
 ******************************************************************************/
void LCD_BiasSegmentSet(int segmentLine, int biasLevel)
{
  int               biasRegister;
  int               bitShift;
  volatile uint32_t *segmentRegister;

#if !defined(_LCD_SEGD0H_MASK)
  EFM_ASSERT(segmentLine < 20);

  /* A bias configuration for 8 segment lines per SEGDnL register. */
  biasRegister = segmentLine / 8;
  bitShift     = (segmentLine % 8) * 4;

  switch (biasRegister) {
    case 0:
      segmentRegister = &LCD->SEGD0L;
      break;
    case 1:
      segmentRegister = &LCD->SEGD1L;
      break;
    case 2:
      segmentRegister = &LCD->SEGD2L;
      break;
    case 3:
      segmentRegister = &LCD->SEGD3L;
      break;
    default:
      segmentRegister = (uint32_t *)0x00000000;
      EFM_ASSERT(0);
      break;
  }
#else
  EFM_ASSERT(segmentLine < 40);

  /* A bias configuration for 10 segment lines per SEGDn L+H registers. */
  biasRegister = segmentLine / 10;
  bitShift     = (segmentLine % 10) * 4;

  switch (biasRegister) {
    case 0:
      if (bitShift < 32) {
        segmentRegister = &LCD->SEGD0L;
      } else {
        segmentRegister = &LCD->SEGD0H;
        bitShift       -= 32;
      }
      break;
    case 1:
      if (bitShift < 32) {
        segmentRegister = &LCD->SEGD1L;
      } else {
        segmentRegister = &LCD->SEGD1H;
        bitShift       -= 32;
      }
      break;
    case 2:
      if (bitShift < 32) {
        segmentRegister = &LCD->SEGD2L;
      } else {
        segmentRegister = &LCD->SEGD1H;
        bitShift       -= 32;
      }
      break;
    case 3:
      if (bitShift < 32) {
        segmentRegister = &LCD->SEGD3L;
      } else {
        segmentRegister = &LCD->SEGD3H;
        bitShift       -= 32;
      }
      break;
    default:
      segmentRegister = (uint32_t *)0x00000000;
      EFM_ASSERT(0);
      break;
  }
#endif

  /* Configure a new bias setting. */
  *segmentRegister = (*segmentRegister & ~(0xF << bitShift)) | (biasLevel << bitShift);
}
#endif

#if defined(LCD_CTRL_DSC)
/***************************************************************************//**
 * @brief
 *   Configure the bias level for a specific segment line.
 *
 * @note
 *   When DSC is active, each configuration takes up 4 bits in the Segment
 *   Registers (SEGD4L/SEGD4H) which defines bias level.
 *   For optimal use of this feature, the entire SEGD-registers should be set
 *   at once in a optimized routine. Therefore, this function
 *   shows how to correctly configure the bias levels and should be used
 *   with care.
 *
 * @param[in] comLine
 *   A COM line number, 0-7.
 *
 * @param[in] biasLevel
 *   The bias configuration level, 0-4. This value must be within the constraints
 *   defined by the LCD_DISPCTRL bias settings.
 *   For more information, see the appropriate Reference Manual and data sheet.
 ******************************************************************************/
void LCD_BiasComSet(int comLine, int biasLevel)
{
  int bitShift;
  EFM_ASSERT(comLine < 8);

  bitShift    = comLine * 4;
  LCD->SEGD4L = (LCD->SEGD4L & ~(0xF << bitShift)) | (biasLevel << bitShift);
}
#endif

#if defined(_SILICON_LABS_32B_SERIES_1)
/***************************************************************************//**
 * @brief
 *   Configure the mode for the LCD panel.
 *
 * @param[in] mode
 *   A mode.
 ******************************************************************************/
void LCD_ModeSet(LCD_Mode_Typedef mode)
{
  LCD->DISPCTRL = (LCD->DISPCTRL & ~_LCD_DISPCTRL_MODE_MASK) | mode;
}

/***************************************************************************//**
 * @brief
 *   Configure the charge redistribution cycles for the LCD panel.
 *
 * @param[in] chgrDst
 *   Charge redistribution cycles, range 0-4.
 ******************************************************************************/
void LCD_ChargeRedistributionCyclesSet(uint8_t cycles)
{
  EFM_ASSERT(cycles <= 4);

  LCD->DISPCTRL = (LCD->DISPCTRL & ~_LCD_DISPCTRL_CHGRDST_MASK)
                  | (cycles << _LCD_DISPCTRL_CHGRDST_SHIFT);
}
#endif

/** @} (end addtogroup LCD) */
/** @} (end addtogroup emlib) */

#endif /* defined(LCD_COUNT) && (LCD_COUNT > 0) */
