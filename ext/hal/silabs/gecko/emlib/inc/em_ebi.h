/***************************************************************************//**
 * @file em_ebi.h
 * @brief External Bus Iterface (EBI) peripheral API
 * @version 5.1.2
 *******************************************************************************
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

#ifndef EM_EBI_H
#define EM_EBI_H

#include "em_device.h"
#if defined(EBI_COUNT) && (EBI_COUNT > 0)

#include <stdint.h>
#include <stdbool.h>
#include "em_assert.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup EBI
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @verbatim
 *
 * ---------               ---------
 * |       |  /|       |\  | Ext.  |
 * |  EBI  | / --------- \ | Async |
 * |       | \ --------- / | Device|
 * |       |  \|       |/  |       |
 * ---------               ---------
 *         Parallel interface
 *
 * @endverbatim
 ******************************************************************************/

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

#define EBI_BANK0    (uint32_t)(1 << 1) /**< EBI address bank 0 */
#define EBI_BANK1    (uint32_t)(1 << 2) /**< EBI address bank 1 */
#define EBI_BANK2    (uint32_t)(1 << 3) /**< EBI address bank 2 */
#define EBI_BANK3    (uint32_t)(1 << 4) /**< EBI address bank 3 */

#define EBI_CS0      (uint32_t)(1 << 1) /**< EBI chip select line 0 */
#define EBI_CS1      (uint32_t)(1 << 2) /**< EBI chip select line 1 */
#define EBI_CS2      (uint32_t)(1 << 3) /**< EBI chip select line 2 */
#define EBI_CS3      (uint32_t)(1 << 4) /**< EBI chip select line 3 */

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

/** EBI Mode of operation */
typedef enum
{
  /** 8 data bits, 8 address bits */
  ebiModeD8A8      = EBI_CTRL_MODE_D8A8,
  /** 16 data bits, 16 address bits, using address latch enable */
  ebiModeD16A16ALE = EBI_CTRL_MODE_D16A16ALE,
  /** 8 data bits, 24 address bits, using address latch enable */
  ebiModeD8A24ALE  = EBI_CTRL_MODE_D8A24ALE,
#if defined(_EFM32_GIANT_FAMILY) || defined(_EFM32_WONDER_FAMILY)
  /** Mode D16 */
  ebiModeD16       = EBI_CTRL_MODE_D16,
#endif
} EBI_Mode_TypeDef;

/** EBI Polarity configuration */
typedef enum
{
  /** Active Low */
  ebiActiveLow  = 0,
  /** Active High */
  ebiActiveHigh = 1
} EBI_Polarity_TypeDef;

/** EBI Pin Line types */
typedef enum
{
  /** Address Ready line */
  ebiLineARDY,
  /** Address Latch Enable line */
  ebiLineALE,
  /** Write Enable line */
  ebiLineWE,
  /** Read Enable line */
  ebiLineRE,
  /** Chip Select line */
  ebiLineCS,
#if defined(_EFM32_GIANT_FAMILY) || defined(_EFM32_WONDER_FAMILY)
  /** BL line */
  ebiLineBL,
#endif
#if defined(_EFM32_GIANT_FAMILY) || defined(_EFM32_WONDER_FAMILY)
  /** TFT VSYNC line */
  ebiLineTFTVSync,
  /** TFT HSYNC line */
  ebiLineTFTHSync,
  /** TFT Data enable line */
  ebiLineTFTDataEn,
  /** TFT DCLK line */
  ebiLineTFTDClk,
  /** TFT Chip select line */
  ebiLineTFTCS,
#endif
} EBI_Line_TypeDef;

#if defined(_EFM32_GIANT_FAMILY) || defined(_EFM32_WONDER_FAMILY)
/** Address Pin Enable, lower limit - lower range of pins to enable */
typedef enum
{
  /** Adress lines EBI_A[0] and upwards are enabled by APEN */
  ebiALowA0 = EBI_ROUTE_ALB_A0,
  /** Adress lines EBI_A[8] and upwards are enabled by APEN */
  ebiALowA8 = EBI_ROUTE_ALB_A8,
  /** Adress lines EBI_A[16] and upwards are enabled by APEN */
  ebiALowA16 = EBI_ROUTE_ALB_A16,
  /** Adress lines EBI_A[24] and upwards are enabled by APEN */
  ebiALowA24 = EBI_ROUTE_ALB_A24,
} EBI_ALow_TypeDef;

/** Adress Pin Enable, high limit - higher limit of pins to enable */
typedef enum
{
  /** All EBI_A pins are disabled */
  ebiAHighA0 = EBI_ROUTE_APEN_A0,
  /** All EBI_A[4:ALow] are enabled */
  ebiAHighA5 = EBI_ROUTE_APEN_A5,
  /** All EBI_A[5:ALow] are enabled */
  ebiAHighA6 = EBI_ROUTE_APEN_A6,
  /** All EBI_A[6:ALow] are enabled */
  ebiAHighA7 = EBI_ROUTE_APEN_A7,
  /** All EBI_A[7:ALow] are enabled */
  ebiAHighA8 = EBI_ROUTE_APEN_A8,
  /** All EBI_A[8:ALow] are enabled */
  ebiAHighA9 = EBI_ROUTE_APEN_A9,
  /** All EBI_A[9:ALow] are enabled */
  ebiAHighA10 = EBI_ROUTE_APEN_A10,
  /** All EBI_A[10:ALow] are enabled */
  ebiAHighA11 = EBI_ROUTE_APEN_A11,
  /** All EBI_A[11:ALow] are enabled */
  ebiAHighA12 = EBI_ROUTE_APEN_A12,
  /** All EBI_A[12:ALow] are enabled */
  ebiAHighA13 = EBI_ROUTE_APEN_A13,
  /** All EBI_A[13:ALow] are enabled */
  ebiAHighA14 = EBI_ROUTE_APEN_A14,
  /** All EBI_A[14:ALow] are enabled */
  ebiAHighA15 = EBI_ROUTE_APEN_A15,
  /** All EBI_A[15:ALow] are enabled */
  ebiAHighA16 = EBI_ROUTE_APEN_A16,
  /** All EBI_A[16:ALow] are enabled */
  ebiAHighA17 = EBI_ROUTE_APEN_A17,
  /** All EBI_A[17:ALow] are enabled */
  ebiAHighA18 = EBI_ROUTE_APEN_A18,
  /** All EBI_A[18:ALow] are enabled */
  ebiAHighA19 = EBI_ROUTE_APEN_A19,
  /** All EBI_A[19:ALow] are enabled */
  ebiAHighA20 = EBI_ROUTE_APEN_A20,
  /** All EBI_A[20:ALow] are enabled */
  ebiAHighA21 = EBI_ROUTE_APEN_A21,
  /** All EBI_A[21:ALow] are enabled */
  ebiAHighA22 = EBI_ROUTE_APEN_A22,
  /** All EBI_A[22:ALow] are enabled */
  ebiAHighA23 = EBI_ROUTE_APEN_A23,
  /** All EBI_A[23:ALow] are enabled */
  ebiAHighA24 = EBI_ROUTE_APEN_A24,
  /** All EBI_A[24:ALow] are enabled */
  ebiAHighA25 = EBI_ROUTE_APEN_A25,
  /** All EBI_A[25:ALow] are enabled */
  ebiAHighA26 = EBI_ROUTE_APEN_A26,
  /** All EBI_A[26:ALow] are enabled */
  ebiAHighA27 = EBI_ROUTE_APEN_A27,
  /** All EBI_A[27:ALow] are enabled */
  ebiAHighA28 = EBI_ROUTE_APEN_A28,
} EBI_AHigh_TypeDef;

/** EBI I/O Alternate Pin Location */
typedef enum {
  /** EBI PIN I/O Location 0 */
  ebiLocation0 = EBI_ROUTE_LOCATION_LOC0,
  /** EBI PIN I/O Location 1 */
  ebiLocation1 = EBI_ROUTE_LOCATION_LOC1,
  /** EBI PIN I/O Location 2 */
  ebiLocation2 = EBI_ROUTE_LOCATION_LOC2
} EBI_Location_TypeDef;
#endif

/* TFT support */
#if defined(_EFM32_GIANT_FAMILY) || defined(_EFM32_WONDER_FAMILY)
/** EBI TFT Graphics Bank Select */
typedef enum
{
  /** Memory BANK0 contains frame buffer */
  ebiTFTBank0 = EBI_TFTCTRL_BANKSEL_BANK0,
  /** Memory BANK1 contains frame buffer */
  ebiTFTBank1 = EBI_TFTCTRL_BANKSEL_BANK1,
  /** Memory BANK2 contains frame buffer */
  ebiTFTBank2 = EBI_TFTCTRL_BANKSEL_BANK2,
  /** Memory BANK3 contains frame buffer */
  ebiTFTBank3 = EBI_TFTCTRL_BANKSEL_BANK3
} EBI_TFTBank_TypeDef;

/** Masking and Alpha blending source color*/
typedef enum
{
  /** Use memory as source color for masking/alpha blending */
  ebiTFTColorSrcMem    = EBI_TFTCTRL_COLOR1SRC_MEM,
  /** Use PIXEL1 register as source color for masking/alpha blending */
  ebiTFTColorSrcPixel1 = EBI_TFTCTRL_COLOR1SRC_PIXEL1,
} EBI_TFTColorSrc_TypeDef;

/** Bus Data Interleave Mode */
typedef enum
{
  /** Unlimited interleaved accesses per EBI_DCLK period. Can cause jitter */
  ebiTFTInterleaveUnlimited  = EBI_TFTCTRL_INTERLEAVE_UNLIMITED,
  /** Allow 1 interleaved access per EBI_DCLK period */
  ebiTFTInterleaveOnePerDClk = EBI_TFTCTRL_INTERLEAVE_ONEPERDCLK,
  /** Only allow accesses during porch periods */
  ebiTFTInterleavePorch      = EBI_TFTCTRL_INTERLEAVE_PORCH,
} EBI_TFTInterleave_TypeDef;

/** Control frame base pointer copy */
typedef enum
{
  /** Trigger update of frame buffer pointer on vertical sync */
  ebiTFTFrameBufTriggerVSync = EBI_TFTCTRL_FBCTRIG_VSYNC,
  /** Trigger update of frame buffer pointer on horizontal sync */
  ebiTFTFrameBufTriggerHSync = EBI_TFTCTRL_FBCTRIG_HSYNC,
} EBI_TFTFrameBufTrigger_TypeDef;

/** Control of mask and alpha blending mode */
typedef enum
{
  /** Masking and blending are disabled */
  ebiTFTMBDisabled   = EBI_TFTCTRL_MASKBLEND_DISABLED,
  /** Internal masking */
  ebiTFTMBIMask      = EBI_TFTCTRL_MASKBLEND_IMASK,
  /** Internal alpha blending */
  ebiTFTMBIAlpha     = EBI_TFTCTRL_MASKBLEND_IALPHA,
  /** Internal masking and alpha blending are enabled */
  ebiTFTMBIMaskAlpha = EBI_TFTCTRL_MASKBLEND_IMASKIALPHA,
  /** External masking */
  ebiTFTMBEMask      = EBI_TFTCTRL_MASKBLEND_EMASK,
  /** External alpha blending */
  ebiTFTMBEAlpha     = EBI_TFTCTRL_MASKBLEND_EALPHA,
  /** External masking and alpha blending */
  ebiTFTMBEMaskAlpha = EBI_TFTCTRL_MASKBLEND_EMASKEALPHA,
} EBI_TFTMaskBlend_TypeDef;

/** TFT Direct Drive mode */
typedef enum
{
  /** Disabled */
  ebiTFTDDModeDisabled = EBI_TFTCTRL_DD_DISABLED,
  /** Direct Drive from internal memory */
  ebiTFTDDModeInternal = EBI_TFTCTRL_DD_INTERNAL,
  /** Direct Drive from external memory */
  ebiTFTDDModeExternal = EBI_TFTCTRL_DD_EXTERNAL,
} EBI_TFTDDMode_TypeDef;

/** TFT Data Increment Width */
typedef enum
{
  /** Pixel increments are 1 byte at a time */
  ebiTFTWidthByte = EBI_TFTCTRL_WIDTH_BYTE,
  /** Pixel increments are 2 bytes (half word) */
  ebiTFTWidthHalfWord = EBI_TFTCTRL_WIDTH_HALFWORD,
} EBI_TFTWidth_TypeDef;

#endif

/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/

/** EBI Initialization structure */
typedef struct
{
  /** EBI operation mode, data and address limits */
  EBI_Mode_TypeDef     mode;
  /** Address Ready pin polarity, active high or low */
  EBI_Polarity_TypeDef ardyPolarity;
  /** Address Latch Enable pin polarity, active high or low */
  EBI_Polarity_TypeDef alePolarity;
  /** Write Enable pin polarity, active high or low */
  EBI_Polarity_TypeDef wePolarity;
  /** Read Enable pin polarity, active high or low */
  EBI_Polarity_TypeDef rePolarity;
  /** Chip Select pin polarity, active high or low */
  EBI_Polarity_TypeDef csPolarity;
#if defined(_EFM32_GIANT_FAMILY) || defined(_EFM32_WONDER_FAMILY)
  /** Byte Lane pin polaritym, active high or low */
  EBI_Polarity_TypeDef blPolarity;
  /** Flag to enable or disable Byte Lane support */
  bool                 blEnable;
  /** Flag to enable or disable idle state insertion between transfers */
  bool                 noIdle;
#endif
  /** Flag to enable or disable Address Ready support */
  bool                 ardyEnable;
  /** Set to turn off 32 cycle timeout ability */
  bool                 ardyDisableTimeout;
  /** Mask of flags which selects address banks to configure EBI_BANK<0-3> */
  uint32_t             banks;
  /** Mask of flags which selects chip select lines to configure EBI_CS<0-3> */
  uint32_t             csLines;
  /** Number of cycles address is held after Adress Latch Enable is asserted */
  int                  addrSetupCycles;
  /** Number of cycles address is driven onto the ADDRDAT bus before ALE is asserted */
  int                  addrHoldCycles;
#if defined(_EFM32_GIANT_FAMILY) || defined(_EFM32_WONDER_FAMILY)
  /** Enable or disables half cycle duration of the ALE strobe in the last address setup cycle */
  bool                 addrHalfALE;
#endif
  /** Number of cycles for address setup before REn is asserted */
  int                  readSetupCycles;
  /** Number of cycles REn is held active */
  int                  readStrobeCycles;
  /** Number of cycles CSn is held active after REn is deasserted */
  int                  readHoldCycles;
#if defined(_EFM32_GIANT_FAMILY) || defined(_EFM32_WONDER_FAMILY)
  /** Enable or disable page mode reads */
  bool                 readPageMode;
  /** Enables or disable prefetching from sequential addresses */
  bool                 readPrefetch;
  /** Enabled or disables half cycle duration of the REn signal in the last strobe cycle  */
  bool                 readHalfRE;
#endif
  /** Number of cycles for address setup before WEn is asserted */
  int                  writeSetupCycles;
  /** Number of cycles WEn is held active */
  int                  writeStrobeCycles;
  /** Number of cycles CSn is held active after WEn is deasserted */
  int                  writeHoldCycles;
#if defined(_EFM32_GIANT_FAMILY) || defined(_EFM32_WONDER_FAMILY)
  /** Enable or disable the write buffer */
  bool                 writeBufferDisable;
  /** Enables or disables half cycle duration of the WEn signal in the last strobe cycle */
  bool                 writeHalfWE;
  /** Lower address pin limit to enable */
  EBI_ALow_TypeDef     aLow;
  /** High address pin limit to enable */
  EBI_AHigh_TypeDef    aHigh;
  /** Pin Location */
  EBI_Location_TypeDef location;
#endif
  /** Flag, if EBI should be enabled after configuration */
  bool                 enable;
} EBI_Init_TypeDef;

/** Default config for EBI init structures */
#if defined(_EFM32_GIANT_FAMILY) || defined(_EFM32_WONDER_FAMILY)
#define EBI_INIT_DEFAULT                                      \
{                                                             \
  ebiModeD8A8,      /* 8 bit address, 8 bit data */           \
  ebiActiveLow,     /* ARDY polarity */                       \
  ebiActiveLow,     /* ALE polarity */                        \
  ebiActiveLow,     /* WE polarity */                         \
  ebiActiveLow,     /* RE polarity */                         \
  ebiActiveLow,     /* CS polarity */                         \
  ebiActiveLow,     /* BL polarity */                         \
  false,            /* enable BL */                           \
  false,            /* enable NOIDLE */                       \
  false,            /* enable ARDY */                         \
  false,            /* don't disable ARDY timeout */          \
  EBI_BANK0,        /* enable bank 0 */                       \
  EBI_CS0,          /* enable chip select 0 */                \
  0,                /* addr setup cycles */                   \
  1,                /* addr hold cycles */                    \
  false,            /* do not enable half cycle ALE strobe */ \
  0,                /* read setup cycles */                   \
  0,                /* read strobe cycles */                  \
  0,                /* read hold cycles */                    \
  false,            /* disable page mode */                   \
  false,            /* disable prefetch */                    \
  false,            /* do not enable half cycle REn strobe */ \
  0,                /* write setup cycles */                  \
  0,                /* write strobe cycles */                 \
  1,                /* write hold cycles */                   \
  false,            /* do not disable the write buffer */     \
  false,            /* do not enable halc cycle WEn strobe */ \
  ebiALowA0,        /* ALB - Low bound, address lines */      \
  ebiAHighA0,       /* APEN - High bound, address lines */    \
  ebiLocation0,     /* Use Location 0 */                      \
  true,             /* enable EBI */                          \
}
#else
#define EBI_INIT_DEFAULT                               \
{                                                      \
  ebiModeD8A8,        /* 8 bit address, 8 bit data */  \
  ebiActiveLow,       /* ARDY polarity */              \
  ebiActiveLow,       /* ALE polarity */               \
  ebiActiveLow,       /* WE polarity */                \
  ebiActiveLow,       /* RE polarity */                \
  ebiActiveLow,       /* CS polarity */                \
  false,              /* enable ARDY */                \
  false,              /* don't disable ARDY timeout */ \
  EBI_BANK0,          /* enable bank 0 */              \
  EBI_CS0,            /* enable chip select 0 */       \
  0,                  /* addr setup cycles */          \
  1,                  /* addr hold cycles */           \
  0,                  /* read setup cycles */          \
  0,                  /* read strobe cycles */         \
  0,                  /* read hold cycles */           \
  0,                  /* write setup cycles */         \
  0,                  /* write strobe cycles */        \
  1,                  /* write hold cycles */          \
  true,               /* enable EBI */                 \
}
#endif

#if defined(_EFM32_GIANT_FAMILY) || defined(_EFM32_WONDER_FAMILY)

/** TFT Initialization structure */
typedef struct
{
  /** External memory bank for driving display */
  EBI_TFTBank_TypeDef            bank;
  /** Width */
  EBI_TFTWidth_TypeDef           width;
  /** Color source for masking and alpha blending */
  EBI_TFTColorSrc_TypeDef        colSrc;
  /** Bus Interleave mode */
  EBI_TFTInterleave_TypeDef      interleave;
  /** Trigger for updating frame buffer pointer */
  EBI_TFTFrameBufTrigger_TypeDef fbTrigger;
  /** Drive DCLK from negative clock edge of internal clock */
  bool                           shiftDClk;
  /** Masking and alpha blending mode */
  EBI_TFTMaskBlend_TypeDef       maskBlend;
  /** TFT Direct Drive mode */
  EBI_TFTDDMode_TypeDef          driveMode;
  /** TFT Polarity for Chip Select (CS) Line */
  EBI_Polarity_TypeDef           csPolarity;
  /** TFT Polarity for Data Clock (DCLK) Line */
  EBI_Polarity_TypeDef           dclkPolarity;
  /** TFT Polarity for Data Enable (DATAEN) Line */
  EBI_Polarity_TypeDef           dataenPolarity;
  /** TFT Polarity for Horizontal Sync (HSYNC) Line */
  EBI_Polarity_TypeDef           hsyncPolarity;
  /** TFT Polarity for Vertical Sync (VSYNC) Line */
  EBI_Polarity_TypeDef           vsyncPolarity;
  /** Horizontal size in pixels */
  int                            hsize;
  /** Horizontal Front Porch Size */
  int                            hPorchFront;
  /** Horizontal Back Porch Size */
  int                            hPorchBack;
  /** Horizontal Synchronization Pulse Width */
  int                            hPulseWidth;
  /** Vertical size in pixels */
  int                            vsize;
  /** Vertical Front Porch Size */
  int                            vPorchFront;
  /** Vertical Back Porch Size */
  int                            vPorchBack;
  /** Vertical Synchronization Pulse Width */
  int                            vPulseWidth;
  /** TFT Frame Buffer address, offset to EBI bank base address */
  uint32_t                       addressOffset;
  /** TFT DCLK period in internal cycles */
  int                            dclkPeriod;
  /** Starting position of External Direct Drive relative to DCLK inactive edge */
  int                            startPosition;
  /** Number of cycles RGB data is driven before active edge of DCLK */
  int                            setupCycles;
  /** Number of cycles RGB data is held after active edge of DCLK */
  int                            holdCycles;
} EBI_TFTInit_TypeDef;

/** Default configuration for EBI TFT init structure */
#define EBI_TFTINIT_DEFAULT                                                         \
{                                                                                   \
  ebiTFTBank0,                /* Select EBI Bank 0 */                               \
  ebiTFTWidthHalfWord,        /* Select 2-byte increments */                        \
  ebiTFTColorSrcMem,          /* Use memory as source for mask/blending */          \
  ebiTFTInterleaveUnlimited,  /* Unlimited interleaved accesses */                  \
  ebiTFTFrameBufTriggerVSync, /* VSYNC as frame buffer update trigger */            \
  false,                      /* Drive DCLK from negative edge of internal clock */ \
  ebiTFTMBDisabled,           /* No masking and alpha blending enabled */           \
  ebiTFTDDModeExternal,       /* Drive from external memory */                      \
  ebiActiveLow,               /* CS Active Low polarity */                          \
  ebiActiveLow,               /* DCLK Active Low polarity */                        \
  ebiActiveLow,               /* DATAEN Active Low polarity */                      \
  ebiActiveLow,               /* HSYNC Active Low polarity */                       \
  ebiActiveLow,               /* VSYNC Active Low polarity */                       \
  320,                        /* Horizontal size in pixels */                       \
  1,                          /* Horizontal Front Porch */                          \
  29,                         /* Horizontal Back Porch */                           \
  2,                          /* Horizontal Synchronization Pulse Width */          \
  240,                        /* Vertical size in pixels */                         \
  1,                          /* Vertical Front Porch */                            \
  4,                          /* Vertical Back Porch */                             \
  2,                          /* Vertical Synchronization Pulse Width */            \
  0x0000,                     /* Address offset to EBI memory base */               \
  5,                          /* DCLK Period */                                     \
  2,                          /* DCLK Start */                                      \
  1,                          /* DCLK Setup cycles */                               \
  1,                          /* DCLK Hold cycles */                                \
}

#endif
/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

void EBI_Init(const EBI_Init_TypeDef *ebiInit);
void EBI_Disable(void);
uint32_t EBI_BankAddress(uint32_t bank);
void EBI_BankEnable(uint32_t banks, bool enable);

#if defined(_EFM32_GIANT_FAMILY) || defined(_EFM32_WONDER_FAMILY)
void EBI_TFTInit(const EBI_TFTInit_TypeDef *ebiTFTInit);
void EBI_TFTSizeSet(uint32_t horizontal, uint32_t vertical);
void EBI_TFTHPorchSet(int front, int back, int pulseWidth);
void EBI_TFTVPorchSet(int front, int back, int pulseWidth);
void EBI_TFTTimingSet(int dclkPeriod, int start, int setup, int hold);
#endif

#if defined(_EFM32_GIANT_FAMILY) || defined(_EFM32_WONDER_FAMILY)
/* This functionality is only available on devices with independent timing support */
void EBI_BankReadTimingSet(uint32_t bank, int setupCycles, int strobeCycles, int holdCycles);
void EBI_BankReadTimingConfig(uint32_t bank, bool pageMode, bool prefetch, bool halfRE);

void EBI_BankWriteTimingSet(uint32_t bank, int setupCycles, int strobeCycles, int holdCycles);
void EBI_BankWriteTimingConfig(uint32_t bank, bool writeBufDisable, bool halfWE);

void EBI_BankAddressTimingSet(uint32_t bank, int setupCycles, int holdCycles);
void EBI_BankAddressTimingConfig(uint32_t bank, bool halfALE);

void EBI_BankPolaritySet(uint32_t bank, EBI_Line_TypeDef line, EBI_Polarity_TypeDef polarity);
void EBI_BankByteLaneEnable(uint32_t bank, bool enable);
void EBI_AltMapEnable(bool enable);

/***************************************************************************//**
 * @brief
 *   Enable or disable TFT Direct Drive
 *
 * @param[in] mode
 *   Drive from Internal or External memory, or Disable Direct Drive
 ******************************************************************************/
__STATIC_INLINE void EBI_TFTEnable(EBI_TFTDDMode_TypeDef mode)
{
  EBI->TFTCTRL = (EBI->TFTCTRL & ~(_EBI_TFTCTRL_DD_MASK)) | (uint32_t) mode;
}


/***************************************************************************//**
 * @brief
 *   Configure frame buffer pointer
 *
 * @param[in] address
 *   Frame pointer address, as offset by EBI base address
 ******************************************************************************/
__STATIC_INLINE void EBI_TFTFrameBaseSet(uint32_t address)
{
  EBI->TFTFRAMEBASE = (uint32_t) address;
}


/***************************************************************************//**
 * @brief Set TFT Pixel Color 0 or 1
 *
 * @param[in] pixel
 *   Which pixel instance to set
 * @param[in] color
 *   Color of pixel, 16-bit value
 ******************************************************************************/
__STATIC_INLINE void EBI_TFTPixelSet(int pixel, uint32_t color)
{
  EFM_ASSERT(pixel == 0 || pixel == 1);

  if (pixel == 0)
  {
    EBI->TFTPIXEL0 = color;
  }
  if (pixel == 1)
  {
    EBI->TFTPIXEL1 = color;
  }
}


/***************************************************************************//**
 * @brief Masking and Blending Mode Set
 *
 * @param[in] maskBlend
 *   Masking and alpha blending mode
 ******************************************************************************/
__STATIC_INLINE void EBI_TFTMaskBlendMode(EBI_TFTMaskBlend_TypeDef maskBlend)
{
  EBI->TFTCTRL = (EBI->TFTCTRL & (~_EBI_TFTCTRL_MASKBLEND_MASK))|maskBlend;
}


/***************************************************************************//**
 * @brief Set TFT Alpha Blending Factor
 *
 * @param[in] alpha
 *   8-bit value indicating blending factor
 ******************************************************************************/
__STATIC_INLINE void EBI_TFTAlphaBlendSet(uint8_t alpha)
{
  EBI->TFTALPHA = alpha;
}


/***************************************************************************//**
 * @brief Set TFT mask value
 *   Data accesses that matches this value are suppressed
 * @param[in] mask
 ******************************************************************************/
__STATIC_INLINE void EBI_TFTMaskSet(uint32_t mask)
{
  EBI->TFTMASK = mask;
}


/***************************************************************************//**
 * @brief Get current vertical position counter
 * @return
 *   Returns the current line position for the visible part of a frame
 ******************************************************************************/
__STATIC_INLINE uint32_t EBI_TFTVCount(void)
{
  return((EBI->TFTSTATUS & _EBI_TFTSTATUS_VCNT_MASK) >> _EBI_TFTSTATUS_VCNT_SHIFT);
}


/***************************************************************************//**
 * @brief Get current horizontal position counter
 * @return
 *   Returns the current horizontal pixel position within a visible line
 ******************************************************************************/
__STATIC_INLINE uint32_t EBI_TFTHCount(void)
{
  return((EBI->TFTSTATUS & _EBI_TFTSTATUS_HCNT_MASK) >> _EBI_TFTSTATUS_HCNT_SHIFT);
}


/***************************************************************************//**
 * @brief Set Frame Buffer Trigger
 *
 * @details
 *   Frame buffer pointer will be updated either on each horizontal line (hsync)
 *   or vertical update (vsync).
 *
 * @param[in] sync
 *   Trigger update of frame buffer pointer on vertical or horisontal sync.
 ******************************************************************************/
__STATIC_INLINE void EBI_TFTFBTriggerSet(EBI_TFTFrameBufTrigger_TypeDef sync)
{
  EBI->TFTCTRL = ((EBI->TFTCTRL & ~_EBI_TFTCTRL_FBCTRIG_MASK)|sync);
}


/***************************************************************************//**
 * @brief Set horizontal TFT stride value in number of bytes
 *
 * @param[in] nbytes
 *    Number of bytes to add to frame buffer pointer after each horizontal line
 *    update
 ******************************************************************************/
__STATIC_INLINE void EBI_TFTHStrideSet(uint32_t nbytes)
{
  EFM_ASSERT(nbytes < 0x1000);

  EBI->TFTSTRIDE = (EBI->TFTSTRIDE & ~(_EBI_TFTSTRIDE_HSTRIDE_MASK))|
    (nbytes<<_EBI_TFTSTRIDE_HSTRIDE_SHIFT);
}


/***************************************************************************//**
 * @brief
 *   Clear one or more pending EBI interrupts.
 * @param[in] flags
 *   Pending EBI interrupt source to clear. Use a logical OR combination
 *   of valid interrupt flags for the EBI module (EBI_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE void EBI_IntClear(uint32_t flags)
{
  EBI->IFC = flags;
}


/***************************************************************************//**
 * @brief
 *   Set one or more pending EBI interrupts.
 *
 * @param[in] flags
 *   EBI interrupt sources to set to pending. Use a logical OR combination of
 *   valid interrupt flags for the EBI module (EBI_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE void EBI_IntSet(uint32_t flags)
{
  EBI->IFS = flags;
}


/***************************************************************************//**
 * @brief
 *   Disable one or more EBI interrupts.
 *
 * @param[in] flags
 *   EBI interrupt sources to disable. Use logical OR combination of valid
 *   interrupt flags for the EBI module (EBI_IF_nnn)
 ******************************************************************************/
__STATIC_INLINE void EBI_IntDisable(uint32_t flags)
{
  EBI->IEN &= ~(flags);
}


/***************************************************************************//**
 * @brief
 *   Enable one or more EBI interrupts.
 *
 * @param[in] flags
 *   EBI interrupt sources to enable. Use logical OR combination of valid
 *   interrupt flags for the EBI module (EBI_IF_nnn)
 ******************************************************************************/
__STATIC_INLINE void EBI_IntEnable(uint32_t flags)
{
  EBI->IEN |= flags;
}


/***************************************************************************//**
 * @brief
 *   Get pending EBI interrupt flags.
 *
 * @note
 *   The event bits are not cleared by the use of this function
 *
 * @return
 *   EBI interrupt sources pending, a logical combination of valid EBI
 *   interrupt flags, EBI_IF_nnn
 ******************************************************************************/
__STATIC_INLINE uint32_t EBI_IntGet(void)
{
  return EBI->IF;
}


/***************************************************************************//**
 * @brief
 *   Get enabled and pending EBI interrupt flags.
 *   Useful for handling more interrupt sources in the same interrupt handler.
 *
 * @note
 *   Interrupt flags are not cleared by the use of this function.
 *
 * @return
 *   Pending and enabled EBI interrupt sources
 *   The return value is the bitwise AND of
 *   - the enabled interrupt sources in EBI_IEN and
 *   - the pending interrupt flags EBI_IF
 ******************************************************************************/
__STATIC_INLINE uint32_t EBI_IntGetEnabled(void)
{
  uint32_t ien;

  ien = EBI->IEN;
  return EBI->IF & ien;
}


/***************************************************************************//**
 * @brief
 *   Start ECC generator on NAND flash transfers.
 ******************************************************************************/
__STATIC_INLINE void EBI_StartNandEccGen(void)
{
  EBI->CMD = EBI_CMD_ECCSTART | EBI_CMD_ECCCLEAR;
}


/***************************************************************************//**
 * @brief
 *   Stop NAND flash ECC generator and return generated ECC.
 *
 * @return
 *   The generated ECC.
 ******************************************************************************/
__STATIC_INLINE uint32_t EBI_StopNandEccGen( void )
{
  EBI->CMD = EBI_CMD_ECCSTOP;
  return EBI->ECCPARITY;
}
#endif

void EBI_ChipSelectEnable(uint32_t banks, bool enable);
void EBI_ReadTimingSet(int setupCycles, int strobeCycles, int holdCycles);
void EBI_WriteTimingSet(int setupCycles, int strobeCycles, int holdCycles);
void EBI_AddressTimingSet(int setupCycles, int holdCycles);
void EBI_PolaritySet(EBI_Line_TypeDef line, EBI_Polarity_TypeDef polarity);

/** @} (end addtogroup EBI) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* defined(EBI_COUNT) && (EBI_COUNT > 0) */

#endif /* EM_EBI_H */
