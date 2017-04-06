/***************************************************************************//**
 * @file em_ldma.c
 * @brief Direct memory access (LDMA) module peripheral API
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
 *    claim that you wrote the original software.@n
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.@n
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

#include "em_ldma.h"

#if defined( LDMA_PRESENT ) && ( LDMA_COUNT == 1 )

#include <stddef.h>
#include "em_assert.h"
#include "em_bus.h"
#include "em_cmu.h"
#include "em_core.h"

/***************************************************************************//**
 * @addtogroup emlib
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup LDMA
 * @{
 ******************************************************************************/

#if defined( LDMA_IRQ_HANDLER_TEMPLATE )
/***************************************************************************//**
 * @brief
 *   Template for an LDMA IRQ handler.
 ******************************************************************************/
void LDMA_IRQHandler(void)
{
  uint32_t ch;
  /* Get all pending and enabled interrupts. */
  uint32_t pending = LDMA_IntGetEnabled();

  /* Loop here on an LDMA error to enable debugging. */
  while (pending & LDMA_IF_ERROR)
  {
  }

  /* Iterate over all LDMA channels. */
  for (ch = 0; ch < DMA_CHAN_COUNT; ch++)
  {
    uint32_t mask = 0x1 << ch;
    if (pending & mask)
    {
      /* Clear interrupt flag. */
      LDMA->IFC = mask;

      /* Do more stuff here, execute callbacks etc. */
    }
  }
}
#endif

/***************************************************************************//**
 * @brief
 *   De-initialize the LDMA controller.
 *
 *   LDMA interrupts are disabled and the LDMA clock is stopped.
 ******************************************************************************/
void LDMA_DeInit(void)
{
  NVIC_DisableIRQ(LDMA_IRQn);
  LDMA->IEN  = 0;
  LDMA->CHEN = 0;
  CMU_ClockEnable(cmuClock_LDMA, false);
}

/***************************************************************************//**
 * @brief
 *   Enable or disable a LDMA channel request.
 *
 * @details
 *   Use this function to enable or disable a LDMA channel request. This will
 *   prevent the LDMA from proceeding after its current transaction if disabled.
 *
 * @param[in] channel
 *   LDMA channel to enable or disable requests on.
 *
 * @param[in] enable
 *   If 'true' request will be enabled. If 'false' request will be disabled.
 ******************************************************************************/
void LDMA_EnableChannelRequest(int ch, bool enable)
{
  EFM_ASSERT(ch < DMA_CHAN_COUNT);

  BUS_RegBitWrite(&LDMA->REQDIS, ch, !enable);
}

/***************************************************************************//**
 * @brief
 *   Initialize the LDMA controller.
 *
 * @details
 *   This function will disable all the LDMA channels and enable the LDMA bus
 *   clock in the CMU. This function will also enable the LDMA IRQ in the NVIC
 *   and set the LDMA IRQ priority to a user configurable priority. The LDMA
 *   interrupt priority is configured using the @ref LDMA_Init_t structure.
 *
 * @note
 *   Since this function enables the LDMA IRQ you should always add a custom
 *   LDMA_IRQHandler to the application in order to handle any interrupts
 *   from LDMA.
 *
 * @param[in] init
 *   Pointer to initialization structure used to configure the LDMA.
 ******************************************************************************/
void LDMA_Init(const LDMA_Init_t *init)
{
  EFM_ASSERT(init != NULL);
  EFM_ASSERT(!((init->ldmaInitCtrlNumFixed << _LDMA_CTRL_NUMFIXED_SHIFT)
               & ~_LDMA_CTRL_NUMFIXED_MASK));
  EFM_ASSERT(!((init->ldmaInitCtrlSyncPrsClrEn << _LDMA_CTRL_SYNCPRSCLREN_SHIFT)
               & ~_LDMA_CTRL_SYNCPRSCLREN_MASK));
  EFM_ASSERT(!((init->ldmaInitCtrlSyncPrsSetEn << _LDMA_CTRL_SYNCPRSSETEN_SHIFT)
               & ~_LDMA_CTRL_SYNCPRSSETEN_MASK));
  EFM_ASSERT(init->ldmaInitIrqPriority < (1 << __NVIC_PRIO_BITS));

  CMU_ClockEnable(cmuClock_LDMA, true);

  LDMA->CTRL = (init->ldmaInitCtrlNumFixed << _LDMA_CTRL_NUMFIXED_SHIFT)
               | (init->ldmaInitCtrlSyncPrsClrEn << _LDMA_CTRL_SYNCPRSCLREN_SHIFT)
               | (init->ldmaInitCtrlSyncPrsSetEn << _LDMA_CTRL_SYNCPRSSETEN_SHIFT);

  LDMA->CHEN    = 0;
  LDMA->DBGHALT = 0;
  LDMA->REQDIS  = 0;

  /* Enable LDMA error interrupt. */
  LDMA->IEN = LDMA_IEN_ERROR;
  LDMA->IFC = 0xFFFFFFFF;

  NVIC_ClearPendingIRQ(LDMA_IRQn);

  /* Range is 0..7, 0 is highest priority. */
  NVIC_SetPriority(LDMA_IRQn, init->ldmaInitIrqPriority);

  NVIC_EnableIRQ(LDMA_IRQn);
}

/***************************************************************************//**
 * @brief
 *   Start a DMA transfer.
 *
 * @param[in] ch
 *   DMA channel.
 *
 * @param[in] transfer
 *   Initialization structure used to configure the transfer.
 *
 * @param[in] descriptor
 *   Transfer descriptor, can be an array of descriptors linked together.
 ******************************************************************************/
void LDMA_StartTransfer(int ch,
                        const LDMA_TransferCfg_t *transfer,
                        const LDMA_Descriptor_t  *descriptor)
{
  uint32_t tmp;
  CORE_DECLARE_IRQ_STATE;
  uint32_t chMask = 1 << ch;

  EFM_ASSERT(ch < DMA_CHAN_COUNT);
  EFM_ASSERT(transfer != NULL);
  EFM_ASSERT(!(transfer->ldmaReqSel & ~_LDMA_CH_REQSEL_MASK));

  EFM_ASSERT(!((transfer->ldmaCtrlSyncPrsClrOff << _LDMA_CTRL_SYNCPRSCLREN_SHIFT)
               & ~_LDMA_CTRL_SYNCPRSCLREN_MASK));
  EFM_ASSERT(!((transfer->ldmaCtrlSyncPrsClrOn << _LDMA_CTRL_SYNCPRSCLREN_SHIFT)
               & ~_LDMA_CTRL_SYNCPRSCLREN_MASK));
  EFM_ASSERT(!((transfer->ldmaCtrlSyncPrsSetOff << _LDMA_CTRL_SYNCPRSSETEN_SHIFT)
               & ~_LDMA_CTRL_SYNCPRSSETEN_MASK));
  EFM_ASSERT(!((transfer->ldmaCtrlSyncPrsSetOn << _LDMA_CTRL_SYNCPRSSETEN_SHIFT)
               & ~_LDMA_CTRL_SYNCPRSSETEN_MASK));

  EFM_ASSERT(!((transfer->ldmaCfgArbSlots << _LDMA_CH_CFG_ARBSLOTS_SHIFT)
               & ~_LDMA_CH_CFG_ARBSLOTS_MASK));
  EFM_ASSERT(!((transfer->ldmaCfgSrcIncSign << _LDMA_CH_CFG_SRCINCSIGN_SHIFT)
               & ~_LDMA_CH_CFG_SRCINCSIGN_MASK ) );
  EFM_ASSERT(!((transfer->ldmaCfgDstIncSign << _LDMA_CH_CFG_DSTINCSIGN_SHIFT)
               & ~_LDMA_CH_CFG_DSTINCSIGN_MASK));
  EFM_ASSERT(!((transfer->ldmaLoopCnt << _LDMA_CH_LOOP_LOOPCNT_SHIFT)
               & ~_LDMA_CH_LOOP_LOOPCNT_MASK));

  LDMA->CH[ch].REQSEL = transfer->ldmaReqSel;
  LDMA->CH[ch].LOOP = (transfer->ldmaLoopCnt << _LDMA_CH_LOOP_LOOPCNT_SHIFT);
  LDMA->CH[ch].CFG = (transfer->ldmaCfgArbSlots << _LDMA_CH_CFG_ARBSLOTS_SHIFT)
                     | (transfer->ldmaCfgSrcIncSign << _LDMA_CH_CFG_SRCINCSIGN_SHIFT)
                     | (transfer->ldmaCfgDstIncSign << _LDMA_CH_CFG_DSTINCSIGN_SHIFT);

  /* Set descriptor address. */
  LDMA->CH[ch].LINK = (uint32_t)descriptor & _LDMA_CH_LINK_LINKADDR_MASK;

  /* Clear pending channel interrupt. */
  LDMA->IFC = chMask;

  /* Critical region. */
  CORE_ENTER_ATOMIC();

  /* Enable channel interrupt. */
  LDMA->IEN |= chMask;

  if (transfer->ldmaReqDis)
  {
    LDMA->REQDIS |= chMask;
  }

  if (transfer->ldmaDbgHalt)
  {
    LDMA->DBGHALT |= chMask;
  }

  tmp = LDMA->CTRL;

  if (transfer->ldmaCtrlSyncPrsClrOff)
  {
    tmp &= ~_LDMA_CTRL_SYNCPRSCLREN_MASK
           | (~transfer->ldmaCtrlSyncPrsClrOff << _LDMA_CTRL_SYNCPRSCLREN_SHIFT);
  }

  if (transfer->ldmaCtrlSyncPrsClrOn)
  {
    tmp |= transfer->ldmaCtrlSyncPrsClrOn << _LDMA_CTRL_SYNCPRSCLREN_SHIFT;
  }

  if (transfer->ldmaCtrlSyncPrsSetOff)
  {
    tmp &= ~_LDMA_CTRL_SYNCPRSSETEN_MASK
           | (~transfer->ldmaCtrlSyncPrsSetOff << _LDMA_CTRL_SYNCPRSSETEN_SHIFT);
  }

  if (transfer->ldmaCtrlSyncPrsSetOn)
  {
    tmp |= transfer->ldmaCtrlSyncPrsSetOn << _LDMA_CTRL_SYNCPRSSETEN_SHIFT;
  }

  LDMA->CTRL = tmp;

  BUS_RegMaskedClear(&LDMA->CHDONE, chMask);  /* Clear the done flag.     */
  LDMA->LINKLOAD = chMask;      /* Start transfer by loading descriptor.  */

  /* Critical region end. */
  CORE_EXIT_ATOMIC();
}

/***************************************************************************//**
 * @brief
 *   Stop a DMA transfer.
 *
 * @note
 *   The DMA will complete the current AHB burst transfer before stopping.
 *
 * @param[in] ch
 *   DMA channel to stop.
 ******************************************************************************/
void LDMA_StopTransfer(int ch)
{
  uint32_t chMask = 1 << ch;

  EFM_ASSERT(ch < DMA_CHAN_COUNT);

  CORE_ATOMIC_SECTION(
    LDMA->IEN &= ~chMask;
    BUS_RegMaskedClear(&LDMA->CHEN, chMask);
  )
}

/***************************************************************************//**
 * @brief
 *   Check if a DMA transfer has completed.
 *
 * @param[in] ch
 *   DMA channel to check.
 *
 * @return
 *   True if transfer has completed, false if not.
 ******************************************************************************/
bool LDMA_TransferDone(int ch)
{
  bool     retVal = false;
  uint32_t chMask = 1 << ch;

  EFM_ASSERT(ch < DMA_CHAN_COUNT);

  CORE_ATOMIC_SECTION(
    if (((LDMA->CHEN & chMask) == 0)
        && ((LDMA->CHDONE & chMask) == chMask))
    {
      retVal = true;
    }
  )
  return retVal;
}

/***************************************************************************//**
 * @brief
 *  Get number of items remaining in a transfer.
 *
 * @note
 *  This function is does not take into account that a DMA transfers with
 *  a chain of linked transfers might be ongoing. It will only check the
 *  count for the current transfer.
 *
 * @param[in] ch
 *  The channel number of the transfer to check.
 *
 * @return
 *  Number of items remaining in the transfer.
 ******************************************************************************/
uint32_t LDMA_TransferRemainingCount(int ch)
{
  uint32_t remaining, done, iflag;
  uint32_t chMask = 1 << ch;

  EFM_ASSERT(ch < DMA_CHAN_COUNT);

  CORE_ATOMIC_SECTION(
    iflag  = LDMA->IF;
    done   = LDMA->CHDONE;
    remaining = LDMA->CH[ch].CTRL;
  )

  iflag    &= chMask;
  done     &= chMask;
  remaining = (remaining & _LDMA_CH_CTRL_XFERCNT_MASK)
              >> _LDMA_CH_CTRL_XFERCNT_SHIFT;

  if (done || ((remaining == 0) && iflag))
  {
    return 0;
  }

  return remaining + 1;
}

/** @} (end addtogroup LDMA) */
/** @} (end addtogroup emlib) */
#endif /* defined( LDMA_PRESENT ) && ( LDMA_COUNT == 1 ) */
