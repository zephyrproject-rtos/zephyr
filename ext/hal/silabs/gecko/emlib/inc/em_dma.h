/***************************************************************************//**
 * @file em_dma.h
 * @brief Direct memory access (DMA) API
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

#ifndef EM_DMA_H
#define EM_DMA_H

#include "em_device.h"
#if defined( DMA_PRESENT )

#include <stdio.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup DMA
 * @{
 ******************************************************************************/

/*******************************************************************************
 ********************************   ENUMS   ************************************
 ******************************************************************************/

/**
 * Amount source/destination address should be incremented for each data
 * transfer.
 */
typedef enum
{
  dmaDataInc1    = _DMA_CTRL_SRC_INC_BYTE,     /**< Increment address 1 byte. */
  dmaDataInc2    = _DMA_CTRL_SRC_INC_HALFWORD, /**< Increment address 2 bytes. */
  dmaDataInc4    = _DMA_CTRL_SRC_INC_WORD,     /**< Increment address 4 bytes. */
  dmaDataIncNone = _DMA_CTRL_SRC_INC_NONE      /**< Do not increment address. */
} DMA_DataInc_TypeDef;


/** Data sizes (in number of bytes) to be read/written by DMA transfer. */
typedef enum
{
  dmaDataSize1 = _DMA_CTRL_SRC_SIZE_BYTE,     /**< 1 byte DMA transfer size. */
  dmaDataSize2 = _DMA_CTRL_SRC_SIZE_HALFWORD, /**< 2 byte DMA transfer size. */
  dmaDataSize4 = _DMA_CTRL_SRC_SIZE_WORD      /**< 4 byte DMA transfer size. */
} DMA_DataSize_TypeDef;


/** Type of DMA transfer. */
typedef enum
{
  /** Basic DMA cycle. */
  dmaCycleCtrlBasic            = _DMA_CTRL_CYCLE_CTRL_BASIC,
  /** Auto-request DMA cycle. */
  dmaCycleCtrlAuto             = _DMA_CTRL_CYCLE_CTRL_AUTO,
  /** Ping-pong DMA cycle. */
  dmaCycleCtrlPingPong         = _DMA_CTRL_CYCLE_CTRL_PINGPONG,
  /** Memory scatter-gather DMA cycle. */
  dmaCycleCtrlMemScatterGather = _DMA_CTRL_CYCLE_CTRL_MEM_SCATTER_GATHER,
  /** Peripheral scatter-gather DMA cycle. */
  dmaCycleCtrlPerScatterGather = _DMA_CTRL_CYCLE_CTRL_PER_SCATTER_GATHER
} DMA_CycleCtrl_TypeDef;


/** Number of transfers before controller does new arbitration. */
typedef enum
{
  dmaArbitrate1    = _DMA_CTRL_R_POWER_1,    /**< Arbitrate after 1 DMA transfer. */
  dmaArbitrate2    = _DMA_CTRL_R_POWER_2,    /**< Arbitrate after 2 DMA transfers. */
  dmaArbitrate4    = _DMA_CTRL_R_POWER_4,    /**< Arbitrate after 4 DMA transfers. */
  dmaArbitrate8    = _DMA_CTRL_R_POWER_8,    /**< Arbitrate after 8 DMA transfers. */
  dmaArbitrate16   = _DMA_CTRL_R_POWER_16,   /**< Arbitrate after 16 DMA transfers. */
  dmaArbitrate32   = _DMA_CTRL_R_POWER_32,   /**< Arbitrate after 32 DMA transfers. */
  dmaArbitrate64   = _DMA_CTRL_R_POWER_64,   /**< Arbitrate after 64 DMA transfers. */
  dmaArbitrate128  = _DMA_CTRL_R_POWER_128,  /**< Arbitrate after 128 DMA transfers. */
  dmaArbitrate256  = _DMA_CTRL_R_POWER_256,  /**< Arbitrate after 256 DMA transfers. */
  dmaArbitrate512  = _DMA_CTRL_R_POWER_512,  /**< Arbitrate after 512 DMA transfers. */
  dmaArbitrate1024 = _DMA_CTRL_R_POWER_1024  /**< Arbitrate after 1024 DMA transfers. */
} DMA_ArbiterConfig_TypeDef;


/*******************************************************************************
 *******************************   STRUCTS   ***********************************
 ******************************************************************************/

/**
 * @brief
 *   DMA interrupt callback function pointer.
 * @details
 *   Parameters:
 *   @li channel - The DMA channel the callback function is invoked for.
 *   @li primary - Indicates if callback is invoked for completion of primary
 *     (true) or alternate (false) descriptor. This is mainly useful for
 *     ping-pong DMA cycles, in order to know which descriptor to refresh.
 *   @li user - User definable reference that may be used to pass information
 *     to be used by the callback handler. If used, the referenced data must be
 *     valid at the point when the interrupt handler invokes the callback.
 *     If callback changes  any data in the provided user structure, remember
 *     that those changes are done in interrupt context, and proper protection
 *     of data may be required.
 */
typedef void (*DMA_FuncPtr_TypeDef)(unsigned int channel, bool primary, void *user);


/**
 * @brief
 *   Callback structure that can be used to define DMA complete actions.
 * @details
 *   A reference to this structure is only stored in the primary descriptor
 *   for a channel (if callback feature is used). If callback is required
 *   for both primary and alternate descriptor completion, this must be
 *   handled by one common callback, using the provided 'primary' parameter
 *   with the callback function.
 */
typedef struct
{
  /**
   * Pointer to callback function to invoke when DMA transfer cycle done.
   * Notice that this function is invoked in interrupt context, and therefore
   * should be short and non-blocking.
   */
  DMA_FuncPtr_TypeDef cbFunc;

  /** User defined pointer to provide with callback function. */
  void                *userPtr;

  /**
   * For internal use only: Indicates if next callback applies to primary
   * or alternate descriptor completion. Mainly useful for ping-pong DMA
   * cycles. Set this value to 0 prior to configuring callback handling.
   */
  uint8_t             primary;
} DMA_CB_TypeDef;


/** Configuration structure for a channel. */
typedef struct
{
  /**
   * Select if channel priority is in the high or default priority group
   * with respect to arbitration. Within a priority group, lower numbered
   * channels have higher priority than higher numbered channels.
   */
  bool     highPri;

  /**
   * Select if interrupt shall be enabled for channel (triggering interrupt
   * handler when dma_done signal is asserted). It should normally be
   * enabled if using the callback feature for a channel, and disabled if
   * not using the callback feature.
   */
  bool     enableInt;

  /**
   * Channel control specifying the source of DMA signals. If accessing
   * peripherals, use one of the DMAREQ_nnn defines available for the
   * peripheral. Set it to 0 for memory-to-memory DMA cycles.
   */
  uint32_t select;

  /**
   * @brief
   *   User definable callback handling configuration.
   * @details
   *   Please refer to structure definition for details. The callback
   *   is invoked when the specified DMA cycle is complete (when dma_done
   *   signal asserted). The callback is invoked in interrupt context,
   *   and should be efficient and non-blocking. Set to NULL to not
   *   use the callback feature.
   * @note
   *   The referenced structure is used by the interrupt handler, and must
   *   be available until no longer used. Thus, in most cases it should
   *   not be located on the stack.
   */
  DMA_CB_TypeDef *cb;
} DMA_CfgChannel_TypeDef;


/**
 * Configuration structure for primary or alternate descriptor
 * (not used for scatter-gather DMA cycles).
 */
typedef struct
{
  /** Destination increment size for each DMA transfer */
  DMA_DataInc_TypeDef       dstInc;

  /** Source increment size for each DMA transfer */
  DMA_DataInc_TypeDef       srcInc;

  /** DMA transfer unit size. */
  DMA_DataSize_TypeDef      size;

  /**
   * Arbitration rate, ie number of DMA transfers done before rearbitration
   * takes place.
   */
  DMA_ArbiterConfig_TypeDef arbRate;

  /**
   * HPROT signal state, please refer to reference manual, DMA chapter for
   * further details. Normally set to 0 if protection is not an issue.
   * The following bits are available:
   * @li bit 0 - HPROT[1] control for source read accesses,
   *   privileged/non-privileged access
   * @li bit 3 - HPROT[1] control for destination write accesses,
   *   privileged/non-privileged access
   */
  uint8_t hprot;
} DMA_CfgDescr_TypeDef;


#if defined( _DMA_LOOP0_MASK ) && defined( _DMA_LOOP1_MASK )
/**
 * Configuration structure for loop mode
 */
typedef struct
{
  /** Enable repeated loop */
  bool      enable;
  /** Width of transfer, reload value for nMinus1 */
  uint16_t  nMinus1;
} DMA_CfgLoop_TypeDef;
#endif


#if defined( _DMA_RECT0_MASK )
/**
 * Configuration structure for rectangular copy
 */
typedef struct
{
  /** DMA channel destination stride (width of destination image, distance between lines) */
  uint16_t dstStride;
  /** DMA channel source stride (width of source image, distance between lines) */
  uint16_t srcStride;
  /** 2D copy height */
  uint16_t height;
} DMA_CfgRect_TypeDef;
#endif


/** Configuration structure for alternate scatter-gather descriptor. */
typedef struct
{
  /** Pointer to location to transfer data from. */
  void                      *src;

  /** Pointer to location to transfer data to. */
  void                      *dst;

  /** Destination increment size for each DMA transfer */
  DMA_DataInc_TypeDef       dstInc;

  /** Source increment size for each DMA transfer */
  DMA_DataInc_TypeDef       srcInc;

  /** DMA transfer unit size. */
  DMA_DataSize_TypeDef      size;

  /**
   * Arbitration rate, ie number of DMA transfers done before rearbitration
   * takes place.
   */
  DMA_ArbiterConfig_TypeDef arbRate;

  /** Number of DMA transfers minus 1 to do. Must be <= 1023. */
  uint16_t                  nMinus1;

  /**
   * HPROT signal state, please refer to reference manual, DMA chapter for
   * further details. Normally set to 0 if protection is not an issue.
   * The following bits are available:
   * @li bit 0 - HPROT[1] control for source read accesses,
   *   privileged/non-privileged access
   * @li bit 3 - HPROT[1] control for destination write accesses,
   *   privileged/non-privileged access
   */
  uint8_t hprot;

  /** Specify if a memory or peripheral scatter-gather DMA cycle. Notice
   *  that this parameter should be the same for all alternate
   *  descriptors.
   *  @li true - this is a peripheral scatter-gather cycle
   *  @li false - this is a memory scatter-gather cycle
   */
  bool    peripheral;
} DMA_CfgDescrSGAlt_TypeDef;


/** DMA init structure */
typedef struct
{
  /**
   * HPROT signal state when accessing the primary/alternate
   * descriptors. Normally set to 0 if protection is not an issue.
   * The following bits are available:
   * @li bit 0 - HPROT[1] control for descriptor accesses (ie when
   *   the DMA controller accesses the channel control block itself),
   *   privileged/non-privileged access
   */
  uint8_t hprot;

  /**
   * Pointer to the controlblock in memory holding descriptors (channel
   * control data structures). This memory must be properly aligned
   * at a 256 bytes. I.e. the 8 least significant bits must be zero.
   *
   * Please refer to the reference manual, DMA chapter for more details.
   *
   * It is possible to provide a smaller memory block, only covering
   * those channels actually used, if not all available channels are used.
   * Ie, if only using 4 channels (0-3), both primary and alternate
   * structures, then only 16*2*4 = 128 bytes must be provided. This
   * implementation has however no check if later exceeding such a limit
   * by configuring for instance channel 4, in which case memory overwrite
   * of some other data will occur.
   */
  DMA_DESCRIPTOR_TypeDef *controlBlock;
} DMA_Init_TypeDef;


/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

void DMA_ActivateAuto(unsigned int channel,
                      bool primary,
                      void *dst,
                      const void *src,
                      unsigned int nMinus1);
void DMA_ActivateBasic(unsigned int channel,
                       bool primary,
                       bool useBurst,
                       void *dst,
                       const void *src,
                       unsigned int nMinus1);
void DMA_ActivatePingPong(unsigned int channel,
                          bool useBurst,
                          void *primDst,
                          const void *primSrc,
                          unsigned int primNMinus1,
                          void *altDst,
                          const void *altSrc,
                          unsigned int altNMinus1);
void DMA_ActivateScatterGather(unsigned int channel,
                               bool useBurst,
                               DMA_DESCRIPTOR_TypeDef *altDescr,
                               unsigned int count);
void DMA_CfgChannel(unsigned int channel, DMA_CfgChannel_TypeDef *cfg);
void DMA_CfgDescr(unsigned int channel,
                  bool primary,
                  DMA_CfgDescr_TypeDef *cfg);
#if defined( _DMA_LOOP0_MASK ) && defined( _DMA_LOOP1_MASK )
void DMA_CfgLoop(unsigned int channel, DMA_CfgLoop_TypeDef *cfg);
#endif

#if defined( _DMA_RECT0_MASK )
void DMA_CfgRect(unsigned int channel, DMA_CfgRect_TypeDef *cfg);
#endif

#if defined( _DMA_LOOP0_MASK ) && defined( _DMA_LOOP1_MASK )
/***************************************************************************//**
 * @brief
 *   Clear Loop configuration for channel
 *
 * @param[in] channel
 *   Channel to reset loop configuration for
 ******************************************************************************/
__STATIC_INLINE void DMA_ResetLoop(unsigned int channel)
{
  /* Clean loop copy operation */
  switch(channel)
  {
    case 0:
      DMA->LOOP0 = _DMA_LOOP0_RESETVALUE;
      break;
    case 1:
      DMA->LOOP1 = _DMA_LOOP1_RESETVALUE;
      break;
    default:
      break;
  }
}
#endif


#if defined( _DMA_RECT0_MASK )
/***************************************************************************//**
 * @brief
 *   Clear Rect/2D DMA configuration for channel
 *
 * @param[in] channel
 *   Channel to reset loop configuration for
 ******************************************************************************/
__STATIC_INLINE void DMA_ResetRect(unsigned int channel)
{
  (void) channel;

  /* Clear rect copy operation */
  DMA->RECT0 = _DMA_RECT0_RESETVALUE;
}
#endif
void DMA_CfgDescrScatterGather(DMA_DESCRIPTOR_TypeDef *descr,
                               unsigned int indx,
                               DMA_CfgDescrSGAlt_TypeDef *cfg);
void DMA_ChannelEnable(unsigned int channel, bool enable);
bool DMA_ChannelEnabled(unsigned int channel);
void DMA_ChannelRequestEnable(unsigned int channel, bool enable);
void DMA_Init(DMA_Init_TypeDef *init);
void DMA_IRQHandler(void);
void DMA_RefreshPingPong(unsigned int channel,
                         bool primary,
                         bool useBurst,
                         void *dst,
                         const void *src,
                         unsigned int nMinus1,
                         bool last);
void DMA_Reset(void);

/***************************************************************************//**
 * @brief
 *   Clear one or more pending DMA interrupts.
 *
 * @param[in] flags
 *   Pending DMA interrupt sources to clear. Use one or more valid
 *   interrupt flags for the DMA module (DMA_IFC_nnn).
 ******************************************************************************/
__STATIC_INLINE void DMA_IntClear(uint32_t flags)
{
  DMA->IFC = flags;
}


/***************************************************************************//**
 * @brief
 *   Disable one or more DMA interrupts.
 *
 * @param[in] flags
 *   DMA interrupt sources to disable. Use one or more valid
 *   interrupt flags for the DMA module (DMA_IEN_nnn).
 ******************************************************************************/
__STATIC_INLINE void DMA_IntDisable(uint32_t flags)
{
  DMA->IEN &= ~flags;
}


/***************************************************************************//**
 * @brief
 *   Enable one or more DMA interrupts.
 *
 * @note
 *   Depending on the use, a pending interrupt may already be set prior to
 *   enabling the interrupt. Consider using DMA_IntClear() prior to enabling
 *   if such a pending interrupt should be ignored.
 *
 * @param[in] flags
 *   DMA interrupt sources to enable. Use one or more valid
 *   interrupt flags for the DMA module (DMA_IEN_nnn).
 ******************************************************************************/
__STATIC_INLINE void DMA_IntEnable(uint32_t flags)
{
  DMA->IEN |= flags;
}


/***************************************************************************//**
 * @brief
 *   Get pending DMA interrupt flags.
 *
 * @note
 *   The event bits are not cleared by the use of this function.
 *
 * @return
 *   DMA interrupt sources pending. Returns one or more valid
 *   interrupt flags for the DMA module (DMA_IF_nnn).
 ******************************************************************************/
__STATIC_INLINE uint32_t DMA_IntGet(void)
{
  return DMA->IF;
}


/***************************************************************************//**
 * @brief
 *   Get enabled and pending DMA interrupt flags.
 *   Useful for handling more interrupt sources in the same interrupt handler.
 *
 * @note
 *   Interrupt flags are not cleared by the use of this function.
 *
 * @return
 *   Pending and enabled DMA interrupt sources
 *   The return value is the bitwise AND of
 *   - the enabled interrupt sources in DMA_IEN and
 *   - the pending interrupt flags DMA_IF
 ******************************************************************************/
__STATIC_INLINE uint32_t DMA_IntGetEnabled(void)
{
  uint32_t ien;

  ien = DMA->IEN;
  return DMA->IF & ien;
}


/***************************************************************************//**
 * @brief
 *   Set one or more pending DMA interrupts
 *
 * @param[in] flags
 *   DMA interrupt sources to set to pending. Use one or more valid
 *   interrupt flags for the DMA module (DMA_IFS_nnn).
 ******************************************************************************/
__STATIC_INLINE void DMA_IntSet(uint32_t flags)
{
  DMA->IFS = flags;
}

/** @} (end addtogroup DMA) */
/** @} (end addtogroup emlib) */

#ifdef __cplusplus
}
#endif

#endif /* defined( DMA_PRESENT ) */
#endif /* EM_DMA_H */
