/*

Copyright (c) 2010 - 2018, Nordic Semiconductor ASA All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

3. Neither the name of Nordic Semiconductor ASA nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef __NRF9160_BITS_H
#define __NRF9160_BITS_H

/*lint ++flb "Enter library region" */

/* Peripheral: CLOCK */
/* Description: Clock management 0 */

/* Register: CLOCK_TASKS_HFCLKSTART */
/* Description: Start HFCLK source */

/* Bit 0 : Start HFCLK source */
#define CLOCK_TASKS_HFCLKSTART_TASKS_HFCLKSTART_Pos (0UL) /*!< Position of TASKS_HFCLKSTART field. */
#define CLOCK_TASKS_HFCLKSTART_TASKS_HFCLKSTART_Msk (0x1UL << CLOCK_TASKS_HFCLKSTART_TASKS_HFCLKSTART_Pos) /*!< Bit mask of TASKS_HFCLKSTART field. */
#define CLOCK_TASKS_HFCLKSTART_TASKS_HFCLKSTART_Trigger (1UL) /*!< Trigger task */

/* Register: CLOCK_TASKS_HFCLKSTOP */
/* Description: Stop HFCLK source */

/* Bit 0 : Stop HFCLK source */
#define CLOCK_TASKS_HFCLKSTOP_TASKS_HFCLKSTOP_Pos (0UL) /*!< Position of TASKS_HFCLKSTOP field. */
#define CLOCK_TASKS_HFCLKSTOP_TASKS_HFCLKSTOP_Msk (0x1UL << CLOCK_TASKS_HFCLKSTOP_TASKS_HFCLKSTOP_Pos) /*!< Bit mask of TASKS_HFCLKSTOP field. */
#define CLOCK_TASKS_HFCLKSTOP_TASKS_HFCLKSTOP_Trigger (1UL) /*!< Trigger task */

/* Register: CLOCK_TASKS_LFCLKSTART */
/* Description: Start LFCLK source */

/* Bit 0 : Start LFCLK source */
#define CLOCK_TASKS_LFCLKSTART_TASKS_LFCLKSTART_Pos (0UL) /*!< Position of TASKS_LFCLKSTART field. */
#define CLOCK_TASKS_LFCLKSTART_TASKS_LFCLKSTART_Msk (0x1UL << CLOCK_TASKS_LFCLKSTART_TASKS_LFCLKSTART_Pos) /*!< Bit mask of TASKS_LFCLKSTART field. */
#define CLOCK_TASKS_LFCLKSTART_TASKS_LFCLKSTART_Trigger (1UL) /*!< Trigger task */

/* Register: CLOCK_TASKS_LFCLKSTOP */
/* Description: Stop LFCLK source */

/* Bit 0 : Stop LFCLK source */
#define CLOCK_TASKS_LFCLKSTOP_TASKS_LFCLKSTOP_Pos (0UL) /*!< Position of TASKS_LFCLKSTOP field. */
#define CLOCK_TASKS_LFCLKSTOP_TASKS_LFCLKSTOP_Msk (0x1UL << CLOCK_TASKS_LFCLKSTOP_TASKS_LFCLKSTOP_Pos) /*!< Bit mask of TASKS_LFCLKSTOP field. */
#define CLOCK_TASKS_LFCLKSTOP_TASKS_LFCLKSTOP_Trigger (1UL) /*!< Trigger task */

/* Register: CLOCK_SUBSCRIBE_HFCLKSTART */
/* Description: Subscribe configuration for task HFCLKSTART */

/* Bit 31 :   */
#define CLOCK_SUBSCRIBE_HFCLKSTART_EN_Pos (31UL) /*!< Position of EN field. */
#define CLOCK_SUBSCRIBE_HFCLKSTART_EN_Msk (0x1UL << CLOCK_SUBSCRIBE_HFCLKSTART_EN_Pos) /*!< Bit mask of EN field. */
#define CLOCK_SUBSCRIBE_HFCLKSTART_EN_Disabled (0UL) /*!< Disable subscription */
#define CLOCK_SUBSCRIBE_HFCLKSTART_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task HFCLKSTART will subscribe to */
#define CLOCK_SUBSCRIBE_HFCLKSTART_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define CLOCK_SUBSCRIBE_HFCLKSTART_CHIDX_Msk (0xFUL << CLOCK_SUBSCRIBE_HFCLKSTART_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: CLOCK_SUBSCRIBE_HFCLKSTOP */
/* Description: Subscribe configuration for task HFCLKSTOP */

/* Bit 31 :   */
#define CLOCK_SUBSCRIBE_HFCLKSTOP_EN_Pos (31UL) /*!< Position of EN field. */
#define CLOCK_SUBSCRIBE_HFCLKSTOP_EN_Msk (0x1UL << CLOCK_SUBSCRIBE_HFCLKSTOP_EN_Pos) /*!< Bit mask of EN field. */
#define CLOCK_SUBSCRIBE_HFCLKSTOP_EN_Disabled (0UL) /*!< Disable subscription */
#define CLOCK_SUBSCRIBE_HFCLKSTOP_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task HFCLKSTOP will subscribe to */
#define CLOCK_SUBSCRIBE_HFCLKSTOP_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define CLOCK_SUBSCRIBE_HFCLKSTOP_CHIDX_Msk (0xFUL << CLOCK_SUBSCRIBE_HFCLKSTOP_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: CLOCK_SUBSCRIBE_LFCLKSTART */
/* Description: Subscribe configuration for task LFCLKSTART */

/* Bit 31 :   */
#define CLOCK_SUBSCRIBE_LFCLKSTART_EN_Pos (31UL) /*!< Position of EN field. */
#define CLOCK_SUBSCRIBE_LFCLKSTART_EN_Msk (0x1UL << CLOCK_SUBSCRIBE_LFCLKSTART_EN_Pos) /*!< Bit mask of EN field. */
#define CLOCK_SUBSCRIBE_LFCLKSTART_EN_Disabled (0UL) /*!< Disable subscription */
#define CLOCK_SUBSCRIBE_LFCLKSTART_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task LFCLKSTART will subscribe to */
#define CLOCK_SUBSCRIBE_LFCLKSTART_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define CLOCK_SUBSCRIBE_LFCLKSTART_CHIDX_Msk (0xFUL << CLOCK_SUBSCRIBE_LFCLKSTART_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: CLOCK_SUBSCRIBE_LFCLKSTOP */
/* Description: Subscribe configuration for task LFCLKSTOP */

/* Bit 31 :   */
#define CLOCK_SUBSCRIBE_LFCLKSTOP_EN_Pos (31UL) /*!< Position of EN field. */
#define CLOCK_SUBSCRIBE_LFCLKSTOP_EN_Msk (0x1UL << CLOCK_SUBSCRIBE_LFCLKSTOP_EN_Pos) /*!< Bit mask of EN field. */
#define CLOCK_SUBSCRIBE_LFCLKSTOP_EN_Disabled (0UL) /*!< Disable subscription */
#define CLOCK_SUBSCRIBE_LFCLKSTOP_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task LFCLKSTOP will subscribe to */
#define CLOCK_SUBSCRIBE_LFCLKSTOP_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define CLOCK_SUBSCRIBE_LFCLKSTOP_CHIDX_Msk (0xFUL << CLOCK_SUBSCRIBE_LFCLKSTOP_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: CLOCK_EVENTS_HFCLKSTARTED */
/* Description: HFCLK oscillator started */

/* Bit 0 : HFCLK oscillator started */
#define CLOCK_EVENTS_HFCLKSTARTED_EVENTS_HFCLKSTARTED_Pos (0UL) /*!< Position of EVENTS_HFCLKSTARTED field. */
#define CLOCK_EVENTS_HFCLKSTARTED_EVENTS_HFCLKSTARTED_Msk (0x1UL << CLOCK_EVENTS_HFCLKSTARTED_EVENTS_HFCLKSTARTED_Pos) /*!< Bit mask of EVENTS_HFCLKSTARTED field. */
#define CLOCK_EVENTS_HFCLKSTARTED_EVENTS_HFCLKSTARTED_NotGenerated (0UL) /*!< Event not generated */
#define CLOCK_EVENTS_HFCLKSTARTED_EVENTS_HFCLKSTARTED_Generated (1UL) /*!< Event generated */

/* Register: CLOCK_EVENTS_LFCLKSTARTED */
/* Description: LFCLK started */

/* Bit 0 : LFCLK started */
#define CLOCK_EVENTS_LFCLKSTARTED_EVENTS_LFCLKSTARTED_Pos (0UL) /*!< Position of EVENTS_LFCLKSTARTED field. */
#define CLOCK_EVENTS_LFCLKSTARTED_EVENTS_LFCLKSTARTED_Msk (0x1UL << CLOCK_EVENTS_LFCLKSTARTED_EVENTS_LFCLKSTARTED_Pos) /*!< Bit mask of EVENTS_LFCLKSTARTED field. */
#define CLOCK_EVENTS_LFCLKSTARTED_EVENTS_LFCLKSTARTED_NotGenerated (0UL) /*!< Event not generated */
#define CLOCK_EVENTS_LFCLKSTARTED_EVENTS_LFCLKSTARTED_Generated (1UL) /*!< Event generated */

/* Register: CLOCK_PUBLISH_HFCLKSTARTED */
/* Description: Publish configuration for event HFCLKSTARTED */

/* Bit 31 :   */
#define CLOCK_PUBLISH_HFCLKSTARTED_EN_Pos (31UL) /*!< Position of EN field. */
#define CLOCK_PUBLISH_HFCLKSTARTED_EN_Msk (0x1UL << CLOCK_PUBLISH_HFCLKSTARTED_EN_Pos) /*!< Bit mask of EN field. */
#define CLOCK_PUBLISH_HFCLKSTARTED_EN_Disabled (0UL) /*!< Disable publishing */
#define CLOCK_PUBLISH_HFCLKSTARTED_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event HFCLKSTARTED will publish to. */
#define CLOCK_PUBLISH_HFCLKSTARTED_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define CLOCK_PUBLISH_HFCLKSTARTED_CHIDX_Msk (0xFUL << CLOCK_PUBLISH_HFCLKSTARTED_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: CLOCK_PUBLISH_LFCLKSTARTED */
/* Description: Publish configuration for event LFCLKSTARTED */

/* Bit 31 :   */
#define CLOCK_PUBLISH_LFCLKSTARTED_EN_Pos (31UL) /*!< Position of EN field. */
#define CLOCK_PUBLISH_LFCLKSTARTED_EN_Msk (0x1UL << CLOCK_PUBLISH_LFCLKSTARTED_EN_Pos) /*!< Bit mask of EN field. */
#define CLOCK_PUBLISH_LFCLKSTARTED_EN_Disabled (0UL) /*!< Disable publishing */
#define CLOCK_PUBLISH_LFCLKSTARTED_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event LFCLKSTARTED will publish to. */
#define CLOCK_PUBLISH_LFCLKSTARTED_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define CLOCK_PUBLISH_LFCLKSTARTED_CHIDX_Msk (0xFUL << CLOCK_PUBLISH_LFCLKSTARTED_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: CLOCK_INTEN */
/* Description: Enable or disable interrupt */

/* Bit 1 : Enable or disable interrupt for event LFCLKSTARTED */
#define CLOCK_INTEN_LFCLKSTARTED_Pos (1UL) /*!< Position of LFCLKSTARTED field. */
#define CLOCK_INTEN_LFCLKSTARTED_Msk (0x1UL << CLOCK_INTEN_LFCLKSTARTED_Pos) /*!< Bit mask of LFCLKSTARTED field. */
#define CLOCK_INTEN_LFCLKSTARTED_Disabled (0UL) /*!< Disable */
#define CLOCK_INTEN_LFCLKSTARTED_Enabled (1UL) /*!< Enable */

/* Bit 0 : Enable or disable interrupt for event HFCLKSTARTED */
#define CLOCK_INTEN_HFCLKSTARTED_Pos (0UL) /*!< Position of HFCLKSTARTED field. */
#define CLOCK_INTEN_HFCLKSTARTED_Msk (0x1UL << CLOCK_INTEN_HFCLKSTARTED_Pos) /*!< Bit mask of HFCLKSTARTED field. */
#define CLOCK_INTEN_HFCLKSTARTED_Disabled (0UL) /*!< Disable */
#define CLOCK_INTEN_HFCLKSTARTED_Enabled (1UL) /*!< Enable */

/* Register: CLOCK_INTENSET */
/* Description: Enable interrupt */

/* Bit 1 : Write '1' to enable interrupt for event LFCLKSTARTED */
#define CLOCK_INTENSET_LFCLKSTARTED_Pos (1UL) /*!< Position of LFCLKSTARTED field. */
#define CLOCK_INTENSET_LFCLKSTARTED_Msk (0x1UL << CLOCK_INTENSET_LFCLKSTARTED_Pos) /*!< Bit mask of LFCLKSTARTED field. */
#define CLOCK_INTENSET_LFCLKSTARTED_Disabled (0UL) /*!< Read: Disabled */
#define CLOCK_INTENSET_LFCLKSTARTED_Enabled (1UL) /*!< Read: Enabled */
#define CLOCK_INTENSET_LFCLKSTARTED_Set (1UL) /*!< Enable */

/* Bit 0 : Write '1' to enable interrupt for event HFCLKSTARTED */
#define CLOCK_INTENSET_HFCLKSTARTED_Pos (0UL) /*!< Position of HFCLKSTARTED field. */
#define CLOCK_INTENSET_HFCLKSTARTED_Msk (0x1UL << CLOCK_INTENSET_HFCLKSTARTED_Pos) /*!< Bit mask of HFCLKSTARTED field. */
#define CLOCK_INTENSET_HFCLKSTARTED_Disabled (0UL) /*!< Read: Disabled */
#define CLOCK_INTENSET_HFCLKSTARTED_Enabled (1UL) /*!< Read: Enabled */
#define CLOCK_INTENSET_HFCLKSTARTED_Set (1UL) /*!< Enable */

/* Register: CLOCK_INTENCLR */
/* Description: Disable interrupt */

/* Bit 1 : Write '1' to disable interrupt for event LFCLKSTARTED */
#define CLOCK_INTENCLR_LFCLKSTARTED_Pos (1UL) /*!< Position of LFCLKSTARTED field. */
#define CLOCK_INTENCLR_LFCLKSTARTED_Msk (0x1UL << CLOCK_INTENCLR_LFCLKSTARTED_Pos) /*!< Bit mask of LFCLKSTARTED field. */
#define CLOCK_INTENCLR_LFCLKSTARTED_Disabled (0UL) /*!< Read: Disabled */
#define CLOCK_INTENCLR_LFCLKSTARTED_Enabled (1UL) /*!< Read: Enabled */
#define CLOCK_INTENCLR_LFCLKSTARTED_Clear (1UL) /*!< Disable */

/* Bit 0 : Write '1' to disable interrupt for event HFCLKSTARTED */
#define CLOCK_INTENCLR_HFCLKSTARTED_Pos (0UL) /*!< Position of HFCLKSTARTED field. */
#define CLOCK_INTENCLR_HFCLKSTARTED_Msk (0x1UL << CLOCK_INTENCLR_HFCLKSTARTED_Pos) /*!< Bit mask of HFCLKSTARTED field. */
#define CLOCK_INTENCLR_HFCLKSTARTED_Disabled (0UL) /*!< Read: Disabled */
#define CLOCK_INTENCLR_HFCLKSTARTED_Enabled (1UL) /*!< Read: Enabled */
#define CLOCK_INTENCLR_HFCLKSTARTED_Clear (1UL) /*!< Disable */

/* Register: CLOCK_INTPEND */
/* Description: Pending interrupts */

/* Bit 1 : Read pending status of interrupt for event LFCLKSTARTED */
#define CLOCK_INTPEND_LFCLKSTARTED_Pos (1UL) /*!< Position of LFCLKSTARTED field. */
#define CLOCK_INTPEND_LFCLKSTARTED_Msk (0x1UL << CLOCK_INTPEND_LFCLKSTARTED_Pos) /*!< Bit mask of LFCLKSTARTED field. */
#define CLOCK_INTPEND_LFCLKSTARTED_NotPending (0UL) /*!< Read: Not pending */
#define CLOCK_INTPEND_LFCLKSTARTED_Pending (1UL) /*!< Read: Pending */

/* Bit 0 : Read pending status of interrupt for event HFCLKSTARTED */
#define CLOCK_INTPEND_HFCLKSTARTED_Pos (0UL) /*!< Position of HFCLKSTARTED field. */
#define CLOCK_INTPEND_HFCLKSTARTED_Msk (0x1UL << CLOCK_INTPEND_HFCLKSTARTED_Pos) /*!< Bit mask of HFCLKSTARTED field. */
#define CLOCK_INTPEND_HFCLKSTARTED_NotPending (0UL) /*!< Read: Not pending */
#define CLOCK_INTPEND_HFCLKSTARTED_Pending (1UL) /*!< Read: Pending */

/* Register: CLOCK_HFCLKRUN */
/* Description: Status indicating that HFCLKSTART task has been triggered */

/* Bit 0 : HFCLKSTART task triggered or not */
#define CLOCK_HFCLKRUN_STATUS_Pos (0UL) /*!< Position of STATUS field. */
#define CLOCK_HFCLKRUN_STATUS_Msk (0x1UL << CLOCK_HFCLKRUN_STATUS_Pos) /*!< Bit mask of STATUS field. */
#define CLOCK_HFCLKRUN_STATUS_NotTriggered (0UL) /*!< Task not triggered */
#define CLOCK_HFCLKRUN_STATUS_Triggered (1UL) /*!< Task triggered */

/* Register: CLOCK_HFCLKSTAT */
/* Description: The register shows if HFXO has been requested by triggering HFCLKSTART task and if it has been started (STATE) */

/* Bit 16 : HFCLK state */
#define CLOCK_HFCLKSTAT_STATE_Pos (16UL) /*!< Position of STATE field. */
#define CLOCK_HFCLKSTAT_STATE_Msk (0x1UL << CLOCK_HFCLKSTAT_STATE_Pos) /*!< Bit mask of STATE field. */
#define CLOCK_HFCLKSTAT_STATE_NotRunning (0UL) /*!< HFXO has not been started or HFCLKSTOP task has been triggered */
#define CLOCK_HFCLKSTAT_STATE_Running (1UL) /*!< HFXO has been started (HFCLKSTARTED event has been generated) */

/* Bit 0 : Active clock source */
#define CLOCK_HFCLKSTAT_SRC_Pos (0UL) /*!< Position of SRC field. */
#define CLOCK_HFCLKSTAT_SRC_Msk (0x1UL << CLOCK_HFCLKSTAT_SRC_Pos) /*!< Bit mask of SRC field. */
#define CLOCK_HFCLKSTAT_SRC_HFXO (1UL) /*!< HFXO - 64 MHz clock derived from external 32 MHz crystal oscillator */

/* Register: CLOCK_LFCLKRUN */
/* Description: Status indicating that LFCLKSTART task has been triggered */

/* Bit 0 : LFCLKSTART task triggered or not */
#define CLOCK_LFCLKRUN_STATUS_Pos (0UL) /*!< Position of STATUS field. */
#define CLOCK_LFCLKRUN_STATUS_Msk (0x1UL << CLOCK_LFCLKRUN_STATUS_Pos) /*!< Bit mask of STATUS field. */
#define CLOCK_LFCLKRUN_STATUS_NotTriggered (0UL) /*!< Task not triggered */
#define CLOCK_LFCLKRUN_STATUS_Triggered (1UL) /*!< Task triggered */

/* Register: CLOCK_LFCLKSTAT */
/* Description: The register shows which LFCLK source has been requested (SRC) when triggering LFCLKSTART task and if the source has been started (STATE) */

/* Bit 16 : LFCLK state */
#define CLOCK_LFCLKSTAT_STATE_Pos (16UL) /*!< Position of STATE field. */
#define CLOCK_LFCLKSTAT_STATE_Msk (0x1UL << CLOCK_LFCLKSTAT_STATE_Pos) /*!< Bit mask of STATE field. */
#define CLOCK_LFCLKSTAT_STATE_NotRunning (0UL) /*!< Requested LFCLK source has not been started or LFCLKSTOP task has been triggered */
#define CLOCK_LFCLKSTAT_STATE_Running (1UL) /*!< Requested LFCLK source has been started (LFCLKSTARTED event has been generated) */

/* Bits 1..0 : Active clock source */
#define CLOCK_LFCLKSTAT_SRC_Pos (0UL) /*!< Position of SRC field. */
#define CLOCK_LFCLKSTAT_SRC_Msk (0x3UL << CLOCK_LFCLKSTAT_SRC_Pos) /*!< Bit mask of SRC field. */
#define CLOCK_LFCLKSTAT_SRC_RFU (0UL) /*!< Reserved for future use */
#define CLOCK_LFCLKSTAT_SRC_LFRC (1UL) /*!< 32.768 kHz RC oscillator */
#define CLOCK_LFCLKSTAT_SRC_LFXO (2UL) /*!< 32.768 kHz crystal oscillator */

/* Register: CLOCK_LFCLKSRCCOPY */
/* Description: Copy of LFCLKSRC register, set after LFCLKSTART task has been triggered */

/* Bits 1..0 : Clock source */
#define CLOCK_LFCLKSRCCOPY_SRC_Pos (0UL) /*!< Position of SRC field. */
#define CLOCK_LFCLKSRCCOPY_SRC_Msk (0x3UL << CLOCK_LFCLKSRCCOPY_SRC_Pos) /*!< Bit mask of SRC field. */
#define CLOCK_LFCLKSRCCOPY_SRC_RFU (0UL) /*!< Reserved for future use */
#define CLOCK_LFCLKSRCCOPY_SRC_LFRC (1UL) /*!< 32.768 kHz RC oscillator */
#define CLOCK_LFCLKSRCCOPY_SRC_LFXO (2UL) /*!< 32.768 kHz crystal oscillator */

/* Register: CLOCK_LFCLKSRC */
/* Description: Clock source for the LFCLK. LFCLKSTART task starts starts a clock source selected with this register. */

/* Bits 1..0 : Clock source */
#define CLOCK_LFCLKSRC_SRC_Pos (0UL) /*!< Position of SRC field. */
#define CLOCK_LFCLKSRC_SRC_Msk (0x3UL << CLOCK_LFCLKSRC_SRC_Pos) /*!< Bit mask of SRC field. */
#define CLOCK_LFCLKSRC_SRC_RFU (0UL) /*!< Reserved for future use (equals selecting LFRC) */
#define CLOCK_LFCLKSRC_SRC_LFRC (1UL) /*!< 32.768 kHz RC oscillator */
#define CLOCK_LFCLKSRC_SRC_LFXO (2UL) /*!< 32.768 kHz crystal oscillator */


/* Peripheral: CRYPTOCELL */
/* Description: ARM TrustZone CryptoCell register interface */

/* Register: CRYPTOCELL_ENABLE */
/* Description: Enable CRYPTOCELL subsystem */

/* Bit 0 : Enable or disable the CRYPTOCELL subsystem */
#define CRYPTOCELL_ENABLE_ENABLE_Pos (0UL) /*!< Position of ENABLE field. */
#define CRYPTOCELL_ENABLE_ENABLE_Msk (0x1UL << CRYPTOCELL_ENABLE_ENABLE_Pos) /*!< Bit mask of ENABLE field. */
#define CRYPTOCELL_ENABLE_ENABLE_Disabled (0UL) /*!< CRYPTOCELL subsystem disabled */
#define CRYPTOCELL_ENABLE_ENABLE_Enabled (1UL) /*!< CRYPTOCELL subsystem enabled */


/* Peripheral: CTRLAPPERI */
/* Description: Control access port */

/* Register: CTRLAPPERI_MAILBOX_RXDATA */
/* Description: Data sent from the debugger to the CPU */

/* Bits 31..0 : Data received from debugger */
#define CTRLAPPERI_MAILBOX_RXDATA_RXDATA_Pos (0UL) /*!< Position of RXDATA field. */
#define CTRLAPPERI_MAILBOX_RXDATA_RXDATA_Msk (0xFFFFFFFFUL << CTRLAPPERI_MAILBOX_RXDATA_RXDATA_Pos) /*!< Bit mask of RXDATA field. */

/* Register: CTRLAPPERI_MAILBOX_RXSTATUS */
/* Description: Status to indicate if data sent from the debugger to the CPU has been read */

/* Bit 0 : Status of data in register RXDATA */
#define CTRLAPPERI_MAILBOX_RXSTATUS_RXSTATUS_Pos (0UL) /*!< Position of RXSTATUS field. */
#define CTRLAPPERI_MAILBOX_RXSTATUS_RXSTATUS_Msk (0x1UL << CTRLAPPERI_MAILBOX_RXSTATUS_RXSTATUS_Pos) /*!< Bit mask of RXSTATUS field. */
#define CTRLAPPERI_MAILBOX_RXSTATUS_RXSTATUS_NoDataPending (0UL) /*!< No data pending in register RXDATA */
#define CTRLAPPERI_MAILBOX_RXSTATUS_RXSTATUS_DataPending (1UL) /*!< Data pending in register RXDATA */

/* Register: CTRLAPPERI_MAILBOX_TXDATA */
/* Description: Data sent from the CPU to the debugger */

/* Bits 31..0 : Data sent to debugger */
#define CTRLAPPERI_MAILBOX_TXDATA_TXDATA_Pos (0UL) /*!< Position of TXDATA field. */
#define CTRLAPPERI_MAILBOX_TXDATA_TXDATA_Msk (0xFFFFFFFFUL << CTRLAPPERI_MAILBOX_TXDATA_TXDATA_Pos) /*!< Bit mask of TXDATA field. */

/* Register: CTRLAPPERI_MAILBOX_TXSTATUS */
/* Description: Status to indicate if data sent from the CPU to the debugger status has been read */

/* Bit 0 : Status of data in register TXDATA */
#define CTRLAPPERI_MAILBOX_TXSTATUS_TXSTATUS_Pos (0UL) /*!< Position of TXSTATUS field. */
#define CTRLAPPERI_MAILBOX_TXSTATUS_TXSTATUS_Msk (0x1UL << CTRLAPPERI_MAILBOX_TXSTATUS_TXSTATUS_Pos) /*!< Bit mask of TXSTATUS field. */
#define CTRLAPPERI_MAILBOX_TXSTATUS_TXSTATUS_NoDataPending (0UL) /*!< No data pending in register TXDATA */
#define CTRLAPPERI_MAILBOX_TXSTATUS_TXSTATUS_DataPending (1UL) /*!< Data pending in register TXDATA */

/* Register: CTRLAPPERI_ERASEPROTECT_LOCK */
/* Description: Lock ERASEALL mechanism */

/* Bit 0 : Enable or disable the ERASEALL mechanism */
#define CTRLAPPERI_ERASEPROTECT_LOCK_ERASEPROTECTLOCK_Pos (0UL) /*!< Position of ERASEPROTECTLOCK field. */
#define CTRLAPPERI_ERASEPROTECT_LOCK_ERASEPROTECTLOCK_Msk (0x1UL << CTRLAPPERI_ERASEPROTECT_LOCK_ERASEPROTECTLOCK_Pos) /*!< Bit mask of ERASEPROTECTLOCK field. */
#define CTRLAPPERI_ERASEPROTECT_LOCK_ERASEPROTECTLOCK_Unlocked (0UL) /*!< ERASEALL can be issued */
#define CTRLAPPERI_ERASEPROTECT_LOCK_ERASEPROTECTLOCK_Locked (1UL) /*!< ERASEALL is locked */

/* Register: CTRLAPPERI_ERASEPROTECT_DISABLE */
/* Description: Unlock ERASEPROTECT and perform ERASEALL */

/* Bits 31..0 : Initiate secure erase even though ERASEPROTECT is enabled if KEY fields match */
#define CTRLAPPERI_ERASEPROTECT_DISABLE_KEY_Pos (0UL) /*!< Position of KEY field. */
#define CTRLAPPERI_ERASEPROTECT_DISABLE_KEY_Msk (0xFFFFFFFFUL << CTRLAPPERI_ERASEPROTECT_DISABLE_KEY_Pos) /*!< Bit mask of KEY field. */


/* Peripheral: DPPIC */
/* Description: Distributed Programmable Peripheral Interconnect Controller 0 */

/* Register: DPPIC_TASKS_CHG_EN */
/* Description: Description cluster: Enable channel group n */

/* Bit 0 : Enable channel group n */
#define DPPIC_TASKS_CHG_EN_EN_Pos (0UL) /*!< Position of EN field. */
#define DPPIC_TASKS_CHG_EN_EN_Msk (0x1UL << DPPIC_TASKS_CHG_EN_EN_Pos) /*!< Bit mask of EN field. */
#define DPPIC_TASKS_CHG_EN_EN_Trigger (1UL) /*!< Trigger task */

/* Register: DPPIC_TASKS_CHG_DIS */
/* Description: Description cluster: Disable channel group n */

/* Bit 0 : Disable channel group n */
#define DPPIC_TASKS_CHG_DIS_DIS_Pos (0UL) /*!< Position of DIS field. */
#define DPPIC_TASKS_CHG_DIS_DIS_Msk (0x1UL << DPPIC_TASKS_CHG_DIS_DIS_Pos) /*!< Bit mask of DIS field. */
#define DPPIC_TASKS_CHG_DIS_DIS_Trigger (1UL) /*!< Trigger task */

/* Register: DPPIC_SUBSCRIBE_CHG_EN */
/* Description: Description cluster: Subscribe configuration for task CHG[n].EN */

/* Bit 31 :   */
#define DPPIC_SUBSCRIBE_CHG_EN_EN_Pos (31UL) /*!< Position of EN field. */
#define DPPIC_SUBSCRIBE_CHG_EN_EN_Msk (0x1UL << DPPIC_SUBSCRIBE_CHG_EN_EN_Pos) /*!< Bit mask of EN field. */
#define DPPIC_SUBSCRIBE_CHG_EN_EN_Disabled (0UL) /*!< Disable subscription */
#define DPPIC_SUBSCRIBE_CHG_EN_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task CHG[n].EN will subscribe to */
#define DPPIC_SUBSCRIBE_CHG_EN_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define DPPIC_SUBSCRIBE_CHG_EN_CHIDX_Msk (0xFUL << DPPIC_SUBSCRIBE_CHG_EN_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: DPPIC_SUBSCRIBE_CHG_DIS */
/* Description: Description cluster: Subscribe configuration for task CHG[n].DIS */

/* Bit 31 :   */
#define DPPIC_SUBSCRIBE_CHG_DIS_EN_Pos (31UL) /*!< Position of EN field. */
#define DPPIC_SUBSCRIBE_CHG_DIS_EN_Msk (0x1UL << DPPIC_SUBSCRIBE_CHG_DIS_EN_Pos) /*!< Bit mask of EN field. */
#define DPPIC_SUBSCRIBE_CHG_DIS_EN_Disabled (0UL) /*!< Disable subscription */
#define DPPIC_SUBSCRIBE_CHG_DIS_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task CHG[n].DIS will subscribe to */
#define DPPIC_SUBSCRIBE_CHG_DIS_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define DPPIC_SUBSCRIBE_CHG_DIS_CHIDX_Msk (0xFUL << DPPIC_SUBSCRIBE_CHG_DIS_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: DPPIC_CHEN */
/* Description: Channel enable register */

/* Bit 15 : Enable or disable channel 15 */
#define DPPIC_CHEN_CH15_Pos (15UL) /*!< Position of CH15 field. */
#define DPPIC_CHEN_CH15_Msk (0x1UL << DPPIC_CHEN_CH15_Pos) /*!< Bit mask of CH15 field. */
#define DPPIC_CHEN_CH15_Disabled (0UL) /*!< Disable channel */
#define DPPIC_CHEN_CH15_Enabled (1UL) /*!< Enable channel */

/* Bit 14 : Enable or disable channel 14 */
#define DPPIC_CHEN_CH14_Pos (14UL) /*!< Position of CH14 field. */
#define DPPIC_CHEN_CH14_Msk (0x1UL << DPPIC_CHEN_CH14_Pos) /*!< Bit mask of CH14 field. */
#define DPPIC_CHEN_CH14_Disabled (0UL) /*!< Disable channel */
#define DPPIC_CHEN_CH14_Enabled (1UL) /*!< Enable channel */

/* Bit 13 : Enable or disable channel 13 */
#define DPPIC_CHEN_CH13_Pos (13UL) /*!< Position of CH13 field. */
#define DPPIC_CHEN_CH13_Msk (0x1UL << DPPIC_CHEN_CH13_Pos) /*!< Bit mask of CH13 field. */
#define DPPIC_CHEN_CH13_Disabled (0UL) /*!< Disable channel */
#define DPPIC_CHEN_CH13_Enabled (1UL) /*!< Enable channel */

/* Bit 12 : Enable or disable channel 12 */
#define DPPIC_CHEN_CH12_Pos (12UL) /*!< Position of CH12 field. */
#define DPPIC_CHEN_CH12_Msk (0x1UL << DPPIC_CHEN_CH12_Pos) /*!< Bit mask of CH12 field. */
#define DPPIC_CHEN_CH12_Disabled (0UL) /*!< Disable channel */
#define DPPIC_CHEN_CH12_Enabled (1UL) /*!< Enable channel */

/* Bit 11 : Enable or disable channel 11 */
#define DPPIC_CHEN_CH11_Pos (11UL) /*!< Position of CH11 field. */
#define DPPIC_CHEN_CH11_Msk (0x1UL << DPPIC_CHEN_CH11_Pos) /*!< Bit mask of CH11 field. */
#define DPPIC_CHEN_CH11_Disabled (0UL) /*!< Disable channel */
#define DPPIC_CHEN_CH11_Enabled (1UL) /*!< Enable channel */

/* Bit 10 : Enable or disable channel 10 */
#define DPPIC_CHEN_CH10_Pos (10UL) /*!< Position of CH10 field. */
#define DPPIC_CHEN_CH10_Msk (0x1UL << DPPIC_CHEN_CH10_Pos) /*!< Bit mask of CH10 field. */
#define DPPIC_CHEN_CH10_Disabled (0UL) /*!< Disable channel */
#define DPPIC_CHEN_CH10_Enabled (1UL) /*!< Enable channel */

/* Bit 9 : Enable or disable channel 9 */
#define DPPIC_CHEN_CH9_Pos (9UL) /*!< Position of CH9 field. */
#define DPPIC_CHEN_CH9_Msk (0x1UL << DPPIC_CHEN_CH9_Pos) /*!< Bit mask of CH9 field. */
#define DPPIC_CHEN_CH9_Disabled (0UL) /*!< Disable channel */
#define DPPIC_CHEN_CH9_Enabled (1UL) /*!< Enable channel */

/* Bit 8 : Enable or disable channel 8 */
#define DPPIC_CHEN_CH8_Pos (8UL) /*!< Position of CH8 field. */
#define DPPIC_CHEN_CH8_Msk (0x1UL << DPPIC_CHEN_CH8_Pos) /*!< Bit mask of CH8 field. */
#define DPPIC_CHEN_CH8_Disabled (0UL) /*!< Disable channel */
#define DPPIC_CHEN_CH8_Enabled (1UL) /*!< Enable channel */

/* Bit 7 : Enable or disable channel 7 */
#define DPPIC_CHEN_CH7_Pos (7UL) /*!< Position of CH7 field. */
#define DPPIC_CHEN_CH7_Msk (0x1UL << DPPIC_CHEN_CH7_Pos) /*!< Bit mask of CH7 field. */
#define DPPIC_CHEN_CH7_Disabled (0UL) /*!< Disable channel */
#define DPPIC_CHEN_CH7_Enabled (1UL) /*!< Enable channel */

/* Bit 6 : Enable or disable channel 6 */
#define DPPIC_CHEN_CH6_Pos (6UL) /*!< Position of CH6 field. */
#define DPPIC_CHEN_CH6_Msk (0x1UL << DPPIC_CHEN_CH6_Pos) /*!< Bit mask of CH6 field. */
#define DPPIC_CHEN_CH6_Disabled (0UL) /*!< Disable channel */
#define DPPIC_CHEN_CH6_Enabled (1UL) /*!< Enable channel */

/* Bit 5 : Enable or disable channel 5 */
#define DPPIC_CHEN_CH5_Pos (5UL) /*!< Position of CH5 field. */
#define DPPIC_CHEN_CH5_Msk (0x1UL << DPPIC_CHEN_CH5_Pos) /*!< Bit mask of CH5 field. */
#define DPPIC_CHEN_CH5_Disabled (0UL) /*!< Disable channel */
#define DPPIC_CHEN_CH5_Enabled (1UL) /*!< Enable channel */

/* Bit 4 : Enable or disable channel 4 */
#define DPPIC_CHEN_CH4_Pos (4UL) /*!< Position of CH4 field. */
#define DPPIC_CHEN_CH4_Msk (0x1UL << DPPIC_CHEN_CH4_Pos) /*!< Bit mask of CH4 field. */
#define DPPIC_CHEN_CH4_Disabled (0UL) /*!< Disable channel */
#define DPPIC_CHEN_CH4_Enabled (1UL) /*!< Enable channel */

/* Bit 3 : Enable or disable channel 3 */
#define DPPIC_CHEN_CH3_Pos (3UL) /*!< Position of CH3 field. */
#define DPPIC_CHEN_CH3_Msk (0x1UL << DPPIC_CHEN_CH3_Pos) /*!< Bit mask of CH3 field. */
#define DPPIC_CHEN_CH3_Disabled (0UL) /*!< Disable channel */
#define DPPIC_CHEN_CH3_Enabled (1UL) /*!< Enable channel */

/* Bit 2 : Enable or disable channel 2 */
#define DPPIC_CHEN_CH2_Pos (2UL) /*!< Position of CH2 field. */
#define DPPIC_CHEN_CH2_Msk (0x1UL << DPPIC_CHEN_CH2_Pos) /*!< Bit mask of CH2 field. */
#define DPPIC_CHEN_CH2_Disabled (0UL) /*!< Disable channel */
#define DPPIC_CHEN_CH2_Enabled (1UL) /*!< Enable channel */

/* Bit 1 : Enable or disable channel 1 */
#define DPPIC_CHEN_CH1_Pos (1UL) /*!< Position of CH1 field. */
#define DPPIC_CHEN_CH1_Msk (0x1UL << DPPIC_CHEN_CH1_Pos) /*!< Bit mask of CH1 field. */
#define DPPIC_CHEN_CH1_Disabled (0UL) /*!< Disable channel */
#define DPPIC_CHEN_CH1_Enabled (1UL) /*!< Enable channel */

/* Bit 0 : Enable or disable channel 0 */
#define DPPIC_CHEN_CH0_Pos (0UL) /*!< Position of CH0 field. */
#define DPPIC_CHEN_CH0_Msk (0x1UL << DPPIC_CHEN_CH0_Pos) /*!< Bit mask of CH0 field. */
#define DPPIC_CHEN_CH0_Disabled (0UL) /*!< Disable channel */
#define DPPIC_CHEN_CH0_Enabled (1UL) /*!< Enable channel */

/* Register: DPPIC_CHENSET */
/* Description: Channel enable set register */

/* Bit 15 : Channel 15 enable set register.  Writing '0' has no effect */
#define DPPIC_CHENSET_CH15_Pos (15UL) /*!< Position of CH15 field. */
#define DPPIC_CHENSET_CH15_Msk (0x1UL << DPPIC_CHENSET_CH15_Pos) /*!< Bit mask of CH15 field. */
#define DPPIC_CHENSET_CH15_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENSET_CH15_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENSET_CH15_Set (1UL) /*!< Write: Enable channel */

/* Bit 14 : Channel 14 enable set register.  Writing '0' has no effect */
#define DPPIC_CHENSET_CH14_Pos (14UL) /*!< Position of CH14 field. */
#define DPPIC_CHENSET_CH14_Msk (0x1UL << DPPIC_CHENSET_CH14_Pos) /*!< Bit mask of CH14 field. */
#define DPPIC_CHENSET_CH14_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENSET_CH14_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENSET_CH14_Set (1UL) /*!< Write: Enable channel */

/* Bit 13 : Channel 13 enable set register.  Writing '0' has no effect */
#define DPPIC_CHENSET_CH13_Pos (13UL) /*!< Position of CH13 field. */
#define DPPIC_CHENSET_CH13_Msk (0x1UL << DPPIC_CHENSET_CH13_Pos) /*!< Bit mask of CH13 field. */
#define DPPIC_CHENSET_CH13_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENSET_CH13_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENSET_CH13_Set (1UL) /*!< Write: Enable channel */

/* Bit 12 : Channel 12 enable set register.  Writing '0' has no effect */
#define DPPIC_CHENSET_CH12_Pos (12UL) /*!< Position of CH12 field. */
#define DPPIC_CHENSET_CH12_Msk (0x1UL << DPPIC_CHENSET_CH12_Pos) /*!< Bit mask of CH12 field. */
#define DPPIC_CHENSET_CH12_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENSET_CH12_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENSET_CH12_Set (1UL) /*!< Write: Enable channel */

/* Bit 11 : Channel 11 enable set register.  Writing '0' has no effect */
#define DPPIC_CHENSET_CH11_Pos (11UL) /*!< Position of CH11 field. */
#define DPPIC_CHENSET_CH11_Msk (0x1UL << DPPIC_CHENSET_CH11_Pos) /*!< Bit mask of CH11 field. */
#define DPPIC_CHENSET_CH11_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENSET_CH11_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENSET_CH11_Set (1UL) /*!< Write: Enable channel */

/* Bit 10 : Channel 10 enable set register.  Writing '0' has no effect */
#define DPPIC_CHENSET_CH10_Pos (10UL) /*!< Position of CH10 field. */
#define DPPIC_CHENSET_CH10_Msk (0x1UL << DPPIC_CHENSET_CH10_Pos) /*!< Bit mask of CH10 field. */
#define DPPIC_CHENSET_CH10_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENSET_CH10_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENSET_CH10_Set (1UL) /*!< Write: Enable channel */

/* Bit 9 : Channel 9 enable set register.  Writing '0' has no effect */
#define DPPIC_CHENSET_CH9_Pos (9UL) /*!< Position of CH9 field. */
#define DPPIC_CHENSET_CH9_Msk (0x1UL << DPPIC_CHENSET_CH9_Pos) /*!< Bit mask of CH9 field. */
#define DPPIC_CHENSET_CH9_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENSET_CH9_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENSET_CH9_Set (1UL) /*!< Write: Enable channel */

/* Bit 8 : Channel 8 enable set register.  Writing '0' has no effect */
#define DPPIC_CHENSET_CH8_Pos (8UL) /*!< Position of CH8 field. */
#define DPPIC_CHENSET_CH8_Msk (0x1UL << DPPIC_CHENSET_CH8_Pos) /*!< Bit mask of CH8 field. */
#define DPPIC_CHENSET_CH8_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENSET_CH8_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENSET_CH8_Set (1UL) /*!< Write: Enable channel */

/* Bit 7 : Channel 7 enable set register.  Writing '0' has no effect */
#define DPPIC_CHENSET_CH7_Pos (7UL) /*!< Position of CH7 field. */
#define DPPIC_CHENSET_CH7_Msk (0x1UL << DPPIC_CHENSET_CH7_Pos) /*!< Bit mask of CH7 field. */
#define DPPIC_CHENSET_CH7_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENSET_CH7_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENSET_CH7_Set (1UL) /*!< Write: Enable channel */

/* Bit 6 : Channel 6 enable set register.  Writing '0' has no effect */
#define DPPIC_CHENSET_CH6_Pos (6UL) /*!< Position of CH6 field. */
#define DPPIC_CHENSET_CH6_Msk (0x1UL << DPPIC_CHENSET_CH6_Pos) /*!< Bit mask of CH6 field. */
#define DPPIC_CHENSET_CH6_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENSET_CH6_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENSET_CH6_Set (1UL) /*!< Write: Enable channel */

/* Bit 5 : Channel 5 enable set register.  Writing '0' has no effect */
#define DPPIC_CHENSET_CH5_Pos (5UL) /*!< Position of CH5 field. */
#define DPPIC_CHENSET_CH5_Msk (0x1UL << DPPIC_CHENSET_CH5_Pos) /*!< Bit mask of CH5 field. */
#define DPPIC_CHENSET_CH5_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENSET_CH5_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENSET_CH5_Set (1UL) /*!< Write: Enable channel */

/* Bit 4 : Channel 4 enable set register.  Writing '0' has no effect */
#define DPPIC_CHENSET_CH4_Pos (4UL) /*!< Position of CH4 field. */
#define DPPIC_CHENSET_CH4_Msk (0x1UL << DPPIC_CHENSET_CH4_Pos) /*!< Bit mask of CH4 field. */
#define DPPIC_CHENSET_CH4_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENSET_CH4_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENSET_CH4_Set (1UL) /*!< Write: Enable channel */

/* Bit 3 : Channel 3 enable set register.  Writing '0' has no effect */
#define DPPIC_CHENSET_CH3_Pos (3UL) /*!< Position of CH3 field. */
#define DPPIC_CHENSET_CH3_Msk (0x1UL << DPPIC_CHENSET_CH3_Pos) /*!< Bit mask of CH3 field. */
#define DPPIC_CHENSET_CH3_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENSET_CH3_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENSET_CH3_Set (1UL) /*!< Write: Enable channel */

/* Bit 2 : Channel 2 enable set register.  Writing '0' has no effect */
#define DPPIC_CHENSET_CH2_Pos (2UL) /*!< Position of CH2 field. */
#define DPPIC_CHENSET_CH2_Msk (0x1UL << DPPIC_CHENSET_CH2_Pos) /*!< Bit mask of CH2 field. */
#define DPPIC_CHENSET_CH2_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENSET_CH2_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENSET_CH2_Set (1UL) /*!< Write: Enable channel */

/* Bit 1 : Channel 1 enable set register.  Writing '0' has no effect */
#define DPPIC_CHENSET_CH1_Pos (1UL) /*!< Position of CH1 field. */
#define DPPIC_CHENSET_CH1_Msk (0x1UL << DPPIC_CHENSET_CH1_Pos) /*!< Bit mask of CH1 field. */
#define DPPIC_CHENSET_CH1_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENSET_CH1_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENSET_CH1_Set (1UL) /*!< Write: Enable channel */

/* Bit 0 : Channel 0 enable set register.  Writing '0' has no effect */
#define DPPIC_CHENSET_CH0_Pos (0UL) /*!< Position of CH0 field. */
#define DPPIC_CHENSET_CH0_Msk (0x1UL << DPPIC_CHENSET_CH0_Pos) /*!< Bit mask of CH0 field. */
#define DPPIC_CHENSET_CH0_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENSET_CH0_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENSET_CH0_Set (1UL) /*!< Write: Enable channel */

/* Register: DPPIC_CHENCLR */
/* Description: Channel enable clear register */

/* Bit 15 : Channel 15 enable clear register.  Writing '0' has no effect */
#define DPPIC_CHENCLR_CH15_Pos (15UL) /*!< Position of CH15 field. */
#define DPPIC_CHENCLR_CH15_Msk (0x1UL << DPPIC_CHENCLR_CH15_Pos) /*!< Bit mask of CH15 field. */
#define DPPIC_CHENCLR_CH15_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENCLR_CH15_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENCLR_CH15_Clear (1UL) /*!< Write: disable channel */

/* Bit 14 : Channel 14 enable clear register.  Writing '0' has no effect */
#define DPPIC_CHENCLR_CH14_Pos (14UL) /*!< Position of CH14 field. */
#define DPPIC_CHENCLR_CH14_Msk (0x1UL << DPPIC_CHENCLR_CH14_Pos) /*!< Bit mask of CH14 field. */
#define DPPIC_CHENCLR_CH14_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENCLR_CH14_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENCLR_CH14_Clear (1UL) /*!< Write: disable channel */

/* Bit 13 : Channel 13 enable clear register.  Writing '0' has no effect */
#define DPPIC_CHENCLR_CH13_Pos (13UL) /*!< Position of CH13 field. */
#define DPPIC_CHENCLR_CH13_Msk (0x1UL << DPPIC_CHENCLR_CH13_Pos) /*!< Bit mask of CH13 field. */
#define DPPIC_CHENCLR_CH13_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENCLR_CH13_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENCLR_CH13_Clear (1UL) /*!< Write: disable channel */

/* Bit 12 : Channel 12 enable clear register.  Writing '0' has no effect */
#define DPPIC_CHENCLR_CH12_Pos (12UL) /*!< Position of CH12 field. */
#define DPPIC_CHENCLR_CH12_Msk (0x1UL << DPPIC_CHENCLR_CH12_Pos) /*!< Bit mask of CH12 field. */
#define DPPIC_CHENCLR_CH12_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENCLR_CH12_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENCLR_CH12_Clear (1UL) /*!< Write: disable channel */

/* Bit 11 : Channel 11 enable clear register.  Writing '0' has no effect */
#define DPPIC_CHENCLR_CH11_Pos (11UL) /*!< Position of CH11 field. */
#define DPPIC_CHENCLR_CH11_Msk (0x1UL << DPPIC_CHENCLR_CH11_Pos) /*!< Bit mask of CH11 field. */
#define DPPIC_CHENCLR_CH11_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENCLR_CH11_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENCLR_CH11_Clear (1UL) /*!< Write: disable channel */

/* Bit 10 : Channel 10 enable clear register.  Writing '0' has no effect */
#define DPPIC_CHENCLR_CH10_Pos (10UL) /*!< Position of CH10 field. */
#define DPPIC_CHENCLR_CH10_Msk (0x1UL << DPPIC_CHENCLR_CH10_Pos) /*!< Bit mask of CH10 field. */
#define DPPIC_CHENCLR_CH10_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENCLR_CH10_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENCLR_CH10_Clear (1UL) /*!< Write: disable channel */

/* Bit 9 : Channel 9 enable clear register.  Writing '0' has no effect */
#define DPPIC_CHENCLR_CH9_Pos (9UL) /*!< Position of CH9 field. */
#define DPPIC_CHENCLR_CH9_Msk (0x1UL << DPPIC_CHENCLR_CH9_Pos) /*!< Bit mask of CH9 field. */
#define DPPIC_CHENCLR_CH9_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENCLR_CH9_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENCLR_CH9_Clear (1UL) /*!< Write: disable channel */

/* Bit 8 : Channel 8 enable clear register.  Writing '0' has no effect */
#define DPPIC_CHENCLR_CH8_Pos (8UL) /*!< Position of CH8 field. */
#define DPPIC_CHENCLR_CH8_Msk (0x1UL << DPPIC_CHENCLR_CH8_Pos) /*!< Bit mask of CH8 field. */
#define DPPIC_CHENCLR_CH8_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENCLR_CH8_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENCLR_CH8_Clear (1UL) /*!< Write: disable channel */

/* Bit 7 : Channel 7 enable clear register.  Writing '0' has no effect */
#define DPPIC_CHENCLR_CH7_Pos (7UL) /*!< Position of CH7 field. */
#define DPPIC_CHENCLR_CH7_Msk (0x1UL << DPPIC_CHENCLR_CH7_Pos) /*!< Bit mask of CH7 field. */
#define DPPIC_CHENCLR_CH7_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENCLR_CH7_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENCLR_CH7_Clear (1UL) /*!< Write: disable channel */

/* Bit 6 : Channel 6 enable clear register.  Writing '0' has no effect */
#define DPPIC_CHENCLR_CH6_Pos (6UL) /*!< Position of CH6 field. */
#define DPPIC_CHENCLR_CH6_Msk (0x1UL << DPPIC_CHENCLR_CH6_Pos) /*!< Bit mask of CH6 field. */
#define DPPIC_CHENCLR_CH6_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENCLR_CH6_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENCLR_CH6_Clear (1UL) /*!< Write: disable channel */

/* Bit 5 : Channel 5 enable clear register.  Writing '0' has no effect */
#define DPPIC_CHENCLR_CH5_Pos (5UL) /*!< Position of CH5 field. */
#define DPPIC_CHENCLR_CH5_Msk (0x1UL << DPPIC_CHENCLR_CH5_Pos) /*!< Bit mask of CH5 field. */
#define DPPIC_CHENCLR_CH5_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENCLR_CH5_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENCLR_CH5_Clear (1UL) /*!< Write: disable channel */

/* Bit 4 : Channel 4 enable clear register.  Writing '0' has no effect */
#define DPPIC_CHENCLR_CH4_Pos (4UL) /*!< Position of CH4 field. */
#define DPPIC_CHENCLR_CH4_Msk (0x1UL << DPPIC_CHENCLR_CH4_Pos) /*!< Bit mask of CH4 field. */
#define DPPIC_CHENCLR_CH4_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENCLR_CH4_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENCLR_CH4_Clear (1UL) /*!< Write: disable channel */

/* Bit 3 : Channel 3 enable clear register.  Writing '0' has no effect */
#define DPPIC_CHENCLR_CH3_Pos (3UL) /*!< Position of CH3 field. */
#define DPPIC_CHENCLR_CH3_Msk (0x1UL << DPPIC_CHENCLR_CH3_Pos) /*!< Bit mask of CH3 field. */
#define DPPIC_CHENCLR_CH3_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENCLR_CH3_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENCLR_CH3_Clear (1UL) /*!< Write: disable channel */

/* Bit 2 : Channel 2 enable clear register.  Writing '0' has no effect */
#define DPPIC_CHENCLR_CH2_Pos (2UL) /*!< Position of CH2 field. */
#define DPPIC_CHENCLR_CH2_Msk (0x1UL << DPPIC_CHENCLR_CH2_Pos) /*!< Bit mask of CH2 field. */
#define DPPIC_CHENCLR_CH2_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENCLR_CH2_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENCLR_CH2_Clear (1UL) /*!< Write: disable channel */

/* Bit 1 : Channel 1 enable clear register.  Writing '0' has no effect */
#define DPPIC_CHENCLR_CH1_Pos (1UL) /*!< Position of CH1 field. */
#define DPPIC_CHENCLR_CH1_Msk (0x1UL << DPPIC_CHENCLR_CH1_Pos) /*!< Bit mask of CH1 field. */
#define DPPIC_CHENCLR_CH1_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENCLR_CH1_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENCLR_CH1_Clear (1UL) /*!< Write: disable channel */

/* Bit 0 : Channel 0 enable clear register.  Writing '0' has no effect */
#define DPPIC_CHENCLR_CH0_Pos (0UL) /*!< Position of CH0 field. */
#define DPPIC_CHENCLR_CH0_Msk (0x1UL << DPPIC_CHENCLR_CH0_Pos) /*!< Bit mask of CH0 field. */
#define DPPIC_CHENCLR_CH0_Disabled (0UL) /*!< Read: channel disabled */
#define DPPIC_CHENCLR_CH0_Enabled (1UL) /*!< Read: channel enabled */
#define DPPIC_CHENCLR_CH0_Clear (1UL) /*!< Write: disable channel */

/* Register: DPPIC_CHG */
/* Description: Description collection: Channel group n Note: Writes to this register is ignored if either SUBSCRIBE_CHG[n].EN/DIS are enabled. */

/* Bit 15 : Include or exclude channel 15 */
#define DPPIC_CHG_CH15_Pos (15UL) /*!< Position of CH15 field. */
#define DPPIC_CHG_CH15_Msk (0x1UL << DPPIC_CHG_CH15_Pos) /*!< Bit mask of CH15 field. */
#define DPPIC_CHG_CH15_Excluded (0UL) /*!< Exclude */
#define DPPIC_CHG_CH15_Included (1UL) /*!< Include */

/* Bit 14 : Include or exclude channel 14 */
#define DPPIC_CHG_CH14_Pos (14UL) /*!< Position of CH14 field. */
#define DPPIC_CHG_CH14_Msk (0x1UL << DPPIC_CHG_CH14_Pos) /*!< Bit mask of CH14 field. */
#define DPPIC_CHG_CH14_Excluded (0UL) /*!< Exclude */
#define DPPIC_CHG_CH14_Included (1UL) /*!< Include */

/* Bit 13 : Include or exclude channel 13 */
#define DPPIC_CHG_CH13_Pos (13UL) /*!< Position of CH13 field. */
#define DPPIC_CHG_CH13_Msk (0x1UL << DPPIC_CHG_CH13_Pos) /*!< Bit mask of CH13 field. */
#define DPPIC_CHG_CH13_Excluded (0UL) /*!< Exclude */
#define DPPIC_CHG_CH13_Included (1UL) /*!< Include */

/* Bit 12 : Include or exclude channel 12 */
#define DPPIC_CHG_CH12_Pos (12UL) /*!< Position of CH12 field. */
#define DPPIC_CHG_CH12_Msk (0x1UL << DPPIC_CHG_CH12_Pos) /*!< Bit mask of CH12 field. */
#define DPPIC_CHG_CH12_Excluded (0UL) /*!< Exclude */
#define DPPIC_CHG_CH12_Included (1UL) /*!< Include */

/* Bit 11 : Include or exclude channel 11 */
#define DPPIC_CHG_CH11_Pos (11UL) /*!< Position of CH11 field. */
#define DPPIC_CHG_CH11_Msk (0x1UL << DPPIC_CHG_CH11_Pos) /*!< Bit mask of CH11 field. */
#define DPPIC_CHG_CH11_Excluded (0UL) /*!< Exclude */
#define DPPIC_CHG_CH11_Included (1UL) /*!< Include */

/* Bit 10 : Include or exclude channel 10 */
#define DPPIC_CHG_CH10_Pos (10UL) /*!< Position of CH10 field. */
#define DPPIC_CHG_CH10_Msk (0x1UL << DPPIC_CHG_CH10_Pos) /*!< Bit mask of CH10 field. */
#define DPPIC_CHG_CH10_Excluded (0UL) /*!< Exclude */
#define DPPIC_CHG_CH10_Included (1UL) /*!< Include */

/* Bit 9 : Include or exclude channel 9 */
#define DPPIC_CHG_CH9_Pos (9UL) /*!< Position of CH9 field. */
#define DPPIC_CHG_CH9_Msk (0x1UL << DPPIC_CHG_CH9_Pos) /*!< Bit mask of CH9 field. */
#define DPPIC_CHG_CH9_Excluded (0UL) /*!< Exclude */
#define DPPIC_CHG_CH9_Included (1UL) /*!< Include */

/* Bit 8 : Include or exclude channel 8 */
#define DPPIC_CHG_CH8_Pos (8UL) /*!< Position of CH8 field. */
#define DPPIC_CHG_CH8_Msk (0x1UL << DPPIC_CHG_CH8_Pos) /*!< Bit mask of CH8 field. */
#define DPPIC_CHG_CH8_Excluded (0UL) /*!< Exclude */
#define DPPIC_CHG_CH8_Included (1UL) /*!< Include */

/* Bit 7 : Include or exclude channel 7 */
#define DPPIC_CHG_CH7_Pos (7UL) /*!< Position of CH7 field. */
#define DPPIC_CHG_CH7_Msk (0x1UL << DPPIC_CHG_CH7_Pos) /*!< Bit mask of CH7 field. */
#define DPPIC_CHG_CH7_Excluded (0UL) /*!< Exclude */
#define DPPIC_CHG_CH7_Included (1UL) /*!< Include */

/* Bit 6 : Include or exclude channel 6 */
#define DPPIC_CHG_CH6_Pos (6UL) /*!< Position of CH6 field. */
#define DPPIC_CHG_CH6_Msk (0x1UL << DPPIC_CHG_CH6_Pos) /*!< Bit mask of CH6 field. */
#define DPPIC_CHG_CH6_Excluded (0UL) /*!< Exclude */
#define DPPIC_CHG_CH6_Included (1UL) /*!< Include */

/* Bit 5 : Include or exclude channel 5 */
#define DPPIC_CHG_CH5_Pos (5UL) /*!< Position of CH5 field. */
#define DPPIC_CHG_CH5_Msk (0x1UL << DPPIC_CHG_CH5_Pos) /*!< Bit mask of CH5 field. */
#define DPPIC_CHG_CH5_Excluded (0UL) /*!< Exclude */
#define DPPIC_CHG_CH5_Included (1UL) /*!< Include */

/* Bit 4 : Include or exclude channel 4 */
#define DPPIC_CHG_CH4_Pos (4UL) /*!< Position of CH4 field. */
#define DPPIC_CHG_CH4_Msk (0x1UL << DPPIC_CHG_CH4_Pos) /*!< Bit mask of CH4 field. */
#define DPPIC_CHG_CH4_Excluded (0UL) /*!< Exclude */
#define DPPIC_CHG_CH4_Included (1UL) /*!< Include */

/* Bit 3 : Include or exclude channel 3 */
#define DPPIC_CHG_CH3_Pos (3UL) /*!< Position of CH3 field. */
#define DPPIC_CHG_CH3_Msk (0x1UL << DPPIC_CHG_CH3_Pos) /*!< Bit mask of CH3 field. */
#define DPPIC_CHG_CH3_Excluded (0UL) /*!< Exclude */
#define DPPIC_CHG_CH3_Included (1UL) /*!< Include */

/* Bit 2 : Include or exclude channel 2 */
#define DPPIC_CHG_CH2_Pos (2UL) /*!< Position of CH2 field. */
#define DPPIC_CHG_CH2_Msk (0x1UL << DPPIC_CHG_CH2_Pos) /*!< Bit mask of CH2 field. */
#define DPPIC_CHG_CH2_Excluded (0UL) /*!< Exclude */
#define DPPIC_CHG_CH2_Included (1UL) /*!< Include */

/* Bit 1 : Include or exclude channel 1 */
#define DPPIC_CHG_CH1_Pos (1UL) /*!< Position of CH1 field. */
#define DPPIC_CHG_CH1_Msk (0x1UL << DPPIC_CHG_CH1_Pos) /*!< Bit mask of CH1 field. */
#define DPPIC_CHG_CH1_Excluded (0UL) /*!< Exclude */
#define DPPIC_CHG_CH1_Included (1UL) /*!< Include */

/* Bit 0 : Include or exclude channel 0 */
#define DPPIC_CHG_CH0_Pos (0UL) /*!< Position of CH0 field. */
#define DPPIC_CHG_CH0_Msk (0x1UL << DPPIC_CHG_CH0_Pos) /*!< Bit mask of CH0 field. */
#define DPPIC_CHG_CH0_Excluded (0UL) /*!< Exclude */
#define DPPIC_CHG_CH0_Included (1UL) /*!< Include */


/* Peripheral: EGU */
/* Description: Event Generator Unit 0 */

/* Register: EGU_TASKS_TRIGGER */
/* Description: Description collection: Trigger n for triggering the corresponding TRIGGERED[n] event */

/* Bit 0 : Trigger n for triggering the corresponding TRIGGERED[n] event */
#define EGU_TASKS_TRIGGER_TASKS_TRIGGER_Pos (0UL) /*!< Position of TASKS_TRIGGER field. */
#define EGU_TASKS_TRIGGER_TASKS_TRIGGER_Msk (0x1UL << EGU_TASKS_TRIGGER_TASKS_TRIGGER_Pos) /*!< Bit mask of TASKS_TRIGGER field. */
#define EGU_TASKS_TRIGGER_TASKS_TRIGGER_Trigger (1UL) /*!< Trigger task */

/* Register: EGU_SUBSCRIBE_TRIGGER */
/* Description: Description collection: Subscribe configuration for task TRIGGER[n] */

/* Bit 31 :   */
#define EGU_SUBSCRIBE_TRIGGER_EN_Pos (31UL) /*!< Position of EN field. */
#define EGU_SUBSCRIBE_TRIGGER_EN_Msk (0x1UL << EGU_SUBSCRIBE_TRIGGER_EN_Pos) /*!< Bit mask of EN field. */
#define EGU_SUBSCRIBE_TRIGGER_EN_Disabled (0UL) /*!< Disable subscription */
#define EGU_SUBSCRIBE_TRIGGER_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task TRIGGER[n] will subscribe to */
#define EGU_SUBSCRIBE_TRIGGER_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define EGU_SUBSCRIBE_TRIGGER_CHIDX_Msk (0xFUL << EGU_SUBSCRIBE_TRIGGER_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: EGU_EVENTS_TRIGGERED */
/* Description: Description collection: Event number n generated by triggering the corresponding TRIGGER[n] task */

/* Bit 0 : Event number n generated by triggering the corresponding TRIGGER[n] task */
#define EGU_EVENTS_TRIGGERED_EVENTS_TRIGGERED_Pos (0UL) /*!< Position of EVENTS_TRIGGERED field. */
#define EGU_EVENTS_TRIGGERED_EVENTS_TRIGGERED_Msk (0x1UL << EGU_EVENTS_TRIGGERED_EVENTS_TRIGGERED_Pos) /*!< Bit mask of EVENTS_TRIGGERED field. */
#define EGU_EVENTS_TRIGGERED_EVENTS_TRIGGERED_NotGenerated (0UL) /*!< Event not generated */
#define EGU_EVENTS_TRIGGERED_EVENTS_TRIGGERED_Generated (1UL) /*!< Event generated */

/* Register: EGU_PUBLISH_TRIGGERED */
/* Description: Description collection: Publish configuration for event TRIGGERED[n] */

/* Bit 31 :   */
#define EGU_PUBLISH_TRIGGERED_EN_Pos (31UL) /*!< Position of EN field. */
#define EGU_PUBLISH_TRIGGERED_EN_Msk (0x1UL << EGU_PUBLISH_TRIGGERED_EN_Pos) /*!< Bit mask of EN field. */
#define EGU_PUBLISH_TRIGGERED_EN_Disabled (0UL) /*!< Disable publishing */
#define EGU_PUBLISH_TRIGGERED_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event TRIGGERED[n] will publish to. */
#define EGU_PUBLISH_TRIGGERED_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define EGU_PUBLISH_TRIGGERED_CHIDX_Msk (0xFUL << EGU_PUBLISH_TRIGGERED_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: EGU_INTEN */
/* Description: Enable or disable interrupt */

/* Bit 15 : Enable or disable interrupt for event TRIGGERED[15] */
#define EGU_INTEN_TRIGGERED15_Pos (15UL) /*!< Position of TRIGGERED15 field. */
#define EGU_INTEN_TRIGGERED15_Msk (0x1UL << EGU_INTEN_TRIGGERED15_Pos) /*!< Bit mask of TRIGGERED15 field. */
#define EGU_INTEN_TRIGGERED15_Disabled (0UL) /*!< Disable */
#define EGU_INTEN_TRIGGERED15_Enabled (1UL) /*!< Enable */

/* Bit 14 : Enable or disable interrupt for event TRIGGERED[14] */
#define EGU_INTEN_TRIGGERED14_Pos (14UL) /*!< Position of TRIGGERED14 field. */
#define EGU_INTEN_TRIGGERED14_Msk (0x1UL << EGU_INTEN_TRIGGERED14_Pos) /*!< Bit mask of TRIGGERED14 field. */
#define EGU_INTEN_TRIGGERED14_Disabled (0UL) /*!< Disable */
#define EGU_INTEN_TRIGGERED14_Enabled (1UL) /*!< Enable */

/* Bit 13 : Enable or disable interrupt for event TRIGGERED[13] */
#define EGU_INTEN_TRIGGERED13_Pos (13UL) /*!< Position of TRIGGERED13 field. */
#define EGU_INTEN_TRIGGERED13_Msk (0x1UL << EGU_INTEN_TRIGGERED13_Pos) /*!< Bit mask of TRIGGERED13 field. */
#define EGU_INTEN_TRIGGERED13_Disabled (0UL) /*!< Disable */
#define EGU_INTEN_TRIGGERED13_Enabled (1UL) /*!< Enable */

/* Bit 12 : Enable or disable interrupt for event TRIGGERED[12] */
#define EGU_INTEN_TRIGGERED12_Pos (12UL) /*!< Position of TRIGGERED12 field. */
#define EGU_INTEN_TRIGGERED12_Msk (0x1UL << EGU_INTEN_TRIGGERED12_Pos) /*!< Bit mask of TRIGGERED12 field. */
#define EGU_INTEN_TRIGGERED12_Disabled (0UL) /*!< Disable */
#define EGU_INTEN_TRIGGERED12_Enabled (1UL) /*!< Enable */

/* Bit 11 : Enable or disable interrupt for event TRIGGERED[11] */
#define EGU_INTEN_TRIGGERED11_Pos (11UL) /*!< Position of TRIGGERED11 field. */
#define EGU_INTEN_TRIGGERED11_Msk (0x1UL << EGU_INTEN_TRIGGERED11_Pos) /*!< Bit mask of TRIGGERED11 field. */
#define EGU_INTEN_TRIGGERED11_Disabled (0UL) /*!< Disable */
#define EGU_INTEN_TRIGGERED11_Enabled (1UL) /*!< Enable */

/* Bit 10 : Enable or disable interrupt for event TRIGGERED[10] */
#define EGU_INTEN_TRIGGERED10_Pos (10UL) /*!< Position of TRIGGERED10 field. */
#define EGU_INTEN_TRIGGERED10_Msk (0x1UL << EGU_INTEN_TRIGGERED10_Pos) /*!< Bit mask of TRIGGERED10 field. */
#define EGU_INTEN_TRIGGERED10_Disabled (0UL) /*!< Disable */
#define EGU_INTEN_TRIGGERED10_Enabled (1UL) /*!< Enable */

/* Bit 9 : Enable or disable interrupt for event TRIGGERED[9] */
#define EGU_INTEN_TRIGGERED9_Pos (9UL) /*!< Position of TRIGGERED9 field. */
#define EGU_INTEN_TRIGGERED9_Msk (0x1UL << EGU_INTEN_TRIGGERED9_Pos) /*!< Bit mask of TRIGGERED9 field. */
#define EGU_INTEN_TRIGGERED9_Disabled (0UL) /*!< Disable */
#define EGU_INTEN_TRIGGERED9_Enabled (1UL) /*!< Enable */

/* Bit 8 : Enable or disable interrupt for event TRIGGERED[8] */
#define EGU_INTEN_TRIGGERED8_Pos (8UL) /*!< Position of TRIGGERED8 field. */
#define EGU_INTEN_TRIGGERED8_Msk (0x1UL << EGU_INTEN_TRIGGERED8_Pos) /*!< Bit mask of TRIGGERED8 field. */
#define EGU_INTEN_TRIGGERED8_Disabled (0UL) /*!< Disable */
#define EGU_INTEN_TRIGGERED8_Enabled (1UL) /*!< Enable */

/* Bit 7 : Enable or disable interrupt for event TRIGGERED[7] */
#define EGU_INTEN_TRIGGERED7_Pos (7UL) /*!< Position of TRIGGERED7 field. */
#define EGU_INTEN_TRIGGERED7_Msk (0x1UL << EGU_INTEN_TRIGGERED7_Pos) /*!< Bit mask of TRIGGERED7 field. */
#define EGU_INTEN_TRIGGERED7_Disabled (0UL) /*!< Disable */
#define EGU_INTEN_TRIGGERED7_Enabled (1UL) /*!< Enable */

/* Bit 6 : Enable or disable interrupt for event TRIGGERED[6] */
#define EGU_INTEN_TRIGGERED6_Pos (6UL) /*!< Position of TRIGGERED6 field. */
#define EGU_INTEN_TRIGGERED6_Msk (0x1UL << EGU_INTEN_TRIGGERED6_Pos) /*!< Bit mask of TRIGGERED6 field. */
#define EGU_INTEN_TRIGGERED6_Disabled (0UL) /*!< Disable */
#define EGU_INTEN_TRIGGERED6_Enabled (1UL) /*!< Enable */

/* Bit 5 : Enable or disable interrupt for event TRIGGERED[5] */
#define EGU_INTEN_TRIGGERED5_Pos (5UL) /*!< Position of TRIGGERED5 field. */
#define EGU_INTEN_TRIGGERED5_Msk (0x1UL << EGU_INTEN_TRIGGERED5_Pos) /*!< Bit mask of TRIGGERED5 field. */
#define EGU_INTEN_TRIGGERED5_Disabled (0UL) /*!< Disable */
#define EGU_INTEN_TRIGGERED5_Enabled (1UL) /*!< Enable */

/* Bit 4 : Enable or disable interrupt for event TRIGGERED[4] */
#define EGU_INTEN_TRIGGERED4_Pos (4UL) /*!< Position of TRIGGERED4 field. */
#define EGU_INTEN_TRIGGERED4_Msk (0x1UL << EGU_INTEN_TRIGGERED4_Pos) /*!< Bit mask of TRIGGERED4 field. */
#define EGU_INTEN_TRIGGERED4_Disabled (0UL) /*!< Disable */
#define EGU_INTEN_TRIGGERED4_Enabled (1UL) /*!< Enable */

/* Bit 3 : Enable or disable interrupt for event TRIGGERED[3] */
#define EGU_INTEN_TRIGGERED3_Pos (3UL) /*!< Position of TRIGGERED3 field. */
#define EGU_INTEN_TRIGGERED3_Msk (0x1UL << EGU_INTEN_TRIGGERED3_Pos) /*!< Bit mask of TRIGGERED3 field. */
#define EGU_INTEN_TRIGGERED3_Disabled (0UL) /*!< Disable */
#define EGU_INTEN_TRIGGERED3_Enabled (1UL) /*!< Enable */

/* Bit 2 : Enable or disable interrupt for event TRIGGERED[2] */
#define EGU_INTEN_TRIGGERED2_Pos (2UL) /*!< Position of TRIGGERED2 field. */
#define EGU_INTEN_TRIGGERED2_Msk (0x1UL << EGU_INTEN_TRIGGERED2_Pos) /*!< Bit mask of TRIGGERED2 field. */
#define EGU_INTEN_TRIGGERED2_Disabled (0UL) /*!< Disable */
#define EGU_INTEN_TRIGGERED2_Enabled (1UL) /*!< Enable */

/* Bit 1 : Enable or disable interrupt for event TRIGGERED[1] */
#define EGU_INTEN_TRIGGERED1_Pos (1UL) /*!< Position of TRIGGERED1 field. */
#define EGU_INTEN_TRIGGERED1_Msk (0x1UL << EGU_INTEN_TRIGGERED1_Pos) /*!< Bit mask of TRIGGERED1 field. */
#define EGU_INTEN_TRIGGERED1_Disabled (0UL) /*!< Disable */
#define EGU_INTEN_TRIGGERED1_Enabled (1UL) /*!< Enable */

/* Bit 0 : Enable or disable interrupt for event TRIGGERED[0] */
#define EGU_INTEN_TRIGGERED0_Pos (0UL) /*!< Position of TRIGGERED0 field. */
#define EGU_INTEN_TRIGGERED0_Msk (0x1UL << EGU_INTEN_TRIGGERED0_Pos) /*!< Bit mask of TRIGGERED0 field. */
#define EGU_INTEN_TRIGGERED0_Disabled (0UL) /*!< Disable */
#define EGU_INTEN_TRIGGERED0_Enabled (1UL) /*!< Enable */

/* Register: EGU_INTENSET */
/* Description: Enable interrupt */

/* Bit 15 : Write '1' to enable interrupt for event TRIGGERED[15] */
#define EGU_INTENSET_TRIGGERED15_Pos (15UL) /*!< Position of TRIGGERED15 field. */
#define EGU_INTENSET_TRIGGERED15_Msk (0x1UL << EGU_INTENSET_TRIGGERED15_Pos) /*!< Bit mask of TRIGGERED15 field. */
#define EGU_INTENSET_TRIGGERED15_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENSET_TRIGGERED15_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENSET_TRIGGERED15_Set (1UL) /*!< Enable */

/* Bit 14 : Write '1' to enable interrupt for event TRIGGERED[14] */
#define EGU_INTENSET_TRIGGERED14_Pos (14UL) /*!< Position of TRIGGERED14 field. */
#define EGU_INTENSET_TRIGGERED14_Msk (0x1UL << EGU_INTENSET_TRIGGERED14_Pos) /*!< Bit mask of TRIGGERED14 field. */
#define EGU_INTENSET_TRIGGERED14_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENSET_TRIGGERED14_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENSET_TRIGGERED14_Set (1UL) /*!< Enable */

/* Bit 13 : Write '1' to enable interrupt for event TRIGGERED[13] */
#define EGU_INTENSET_TRIGGERED13_Pos (13UL) /*!< Position of TRIGGERED13 field. */
#define EGU_INTENSET_TRIGGERED13_Msk (0x1UL << EGU_INTENSET_TRIGGERED13_Pos) /*!< Bit mask of TRIGGERED13 field. */
#define EGU_INTENSET_TRIGGERED13_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENSET_TRIGGERED13_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENSET_TRIGGERED13_Set (1UL) /*!< Enable */

/* Bit 12 : Write '1' to enable interrupt for event TRIGGERED[12] */
#define EGU_INTENSET_TRIGGERED12_Pos (12UL) /*!< Position of TRIGGERED12 field. */
#define EGU_INTENSET_TRIGGERED12_Msk (0x1UL << EGU_INTENSET_TRIGGERED12_Pos) /*!< Bit mask of TRIGGERED12 field. */
#define EGU_INTENSET_TRIGGERED12_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENSET_TRIGGERED12_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENSET_TRIGGERED12_Set (1UL) /*!< Enable */

/* Bit 11 : Write '1' to enable interrupt for event TRIGGERED[11] */
#define EGU_INTENSET_TRIGGERED11_Pos (11UL) /*!< Position of TRIGGERED11 field. */
#define EGU_INTENSET_TRIGGERED11_Msk (0x1UL << EGU_INTENSET_TRIGGERED11_Pos) /*!< Bit mask of TRIGGERED11 field. */
#define EGU_INTENSET_TRIGGERED11_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENSET_TRIGGERED11_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENSET_TRIGGERED11_Set (1UL) /*!< Enable */

/* Bit 10 : Write '1' to enable interrupt for event TRIGGERED[10] */
#define EGU_INTENSET_TRIGGERED10_Pos (10UL) /*!< Position of TRIGGERED10 field. */
#define EGU_INTENSET_TRIGGERED10_Msk (0x1UL << EGU_INTENSET_TRIGGERED10_Pos) /*!< Bit mask of TRIGGERED10 field. */
#define EGU_INTENSET_TRIGGERED10_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENSET_TRIGGERED10_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENSET_TRIGGERED10_Set (1UL) /*!< Enable */

/* Bit 9 : Write '1' to enable interrupt for event TRIGGERED[9] */
#define EGU_INTENSET_TRIGGERED9_Pos (9UL) /*!< Position of TRIGGERED9 field. */
#define EGU_INTENSET_TRIGGERED9_Msk (0x1UL << EGU_INTENSET_TRIGGERED9_Pos) /*!< Bit mask of TRIGGERED9 field. */
#define EGU_INTENSET_TRIGGERED9_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENSET_TRIGGERED9_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENSET_TRIGGERED9_Set (1UL) /*!< Enable */

/* Bit 8 : Write '1' to enable interrupt for event TRIGGERED[8] */
#define EGU_INTENSET_TRIGGERED8_Pos (8UL) /*!< Position of TRIGGERED8 field. */
#define EGU_INTENSET_TRIGGERED8_Msk (0x1UL << EGU_INTENSET_TRIGGERED8_Pos) /*!< Bit mask of TRIGGERED8 field. */
#define EGU_INTENSET_TRIGGERED8_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENSET_TRIGGERED8_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENSET_TRIGGERED8_Set (1UL) /*!< Enable */

/* Bit 7 : Write '1' to enable interrupt for event TRIGGERED[7] */
#define EGU_INTENSET_TRIGGERED7_Pos (7UL) /*!< Position of TRIGGERED7 field. */
#define EGU_INTENSET_TRIGGERED7_Msk (0x1UL << EGU_INTENSET_TRIGGERED7_Pos) /*!< Bit mask of TRIGGERED7 field. */
#define EGU_INTENSET_TRIGGERED7_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENSET_TRIGGERED7_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENSET_TRIGGERED7_Set (1UL) /*!< Enable */

/* Bit 6 : Write '1' to enable interrupt for event TRIGGERED[6] */
#define EGU_INTENSET_TRIGGERED6_Pos (6UL) /*!< Position of TRIGGERED6 field. */
#define EGU_INTENSET_TRIGGERED6_Msk (0x1UL << EGU_INTENSET_TRIGGERED6_Pos) /*!< Bit mask of TRIGGERED6 field. */
#define EGU_INTENSET_TRIGGERED6_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENSET_TRIGGERED6_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENSET_TRIGGERED6_Set (1UL) /*!< Enable */

/* Bit 5 : Write '1' to enable interrupt for event TRIGGERED[5] */
#define EGU_INTENSET_TRIGGERED5_Pos (5UL) /*!< Position of TRIGGERED5 field. */
#define EGU_INTENSET_TRIGGERED5_Msk (0x1UL << EGU_INTENSET_TRIGGERED5_Pos) /*!< Bit mask of TRIGGERED5 field. */
#define EGU_INTENSET_TRIGGERED5_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENSET_TRIGGERED5_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENSET_TRIGGERED5_Set (1UL) /*!< Enable */

/* Bit 4 : Write '1' to enable interrupt for event TRIGGERED[4] */
#define EGU_INTENSET_TRIGGERED4_Pos (4UL) /*!< Position of TRIGGERED4 field. */
#define EGU_INTENSET_TRIGGERED4_Msk (0x1UL << EGU_INTENSET_TRIGGERED4_Pos) /*!< Bit mask of TRIGGERED4 field. */
#define EGU_INTENSET_TRIGGERED4_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENSET_TRIGGERED4_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENSET_TRIGGERED4_Set (1UL) /*!< Enable */

/* Bit 3 : Write '1' to enable interrupt for event TRIGGERED[3] */
#define EGU_INTENSET_TRIGGERED3_Pos (3UL) /*!< Position of TRIGGERED3 field. */
#define EGU_INTENSET_TRIGGERED3_Msk (0x1UL << EGU_INTENSET_TRIGGERED3_Pos) /*!< Bit mask of TRIGGERED3 field. */
#define EGU_INTENSET_TRIGGERED3_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENSET_TRIGGERED3_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENSET_TRIGGERED3_Set (1UL) /*!< Enable */

/* Bit 2 : Write '1' to enable interrupt for event TRIGGERED[2] */
#define EGU_INTENSET_TRIGGERED2_Pos (2UL) /*!< Position of TRIGGERED2 field. */
#define EGU_INTENSET_TRIGGERED2_Msk (0x1UL << EGU_INTENSET_TRIGGERED2_Pos) /*!< Bit mask of TRIGGERED2 field. */
#define EGU_INTENSET_TRIGGERED2_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENSET_TRIGGERED2_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENSET_TRIGGERED2_Set (1UL) /*!< Enable */

/* Bit 1 : Write '1' to enable interrupt for event TRIGGERED[1] */
#define EGU_INTENSET_TRIGGERED1_Pos (1UL) /*!< Position of TRIGGERED1 field. */
#define EGU_INTENSET_TRIGGERED1_Msk (0x1UL << EGU_INTENSET_TRIGGERED1_Pos) /*!< Bit mask of TRIGGERED1 field. */
#define EGU_INTENSET_TRIGGERED1_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENSET_TRIGGERED1_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENSET_TRIGGERED1_Set (1UL) /*!< Enable */

/* Bit 0 : Write '1' to enable interrupt for event TRIGGERED[0] */
#define EGU_INTENSET_TRIGGERED0_Pos (0UL) /*!< Position of TRIGGERED0 field. */
#define EGU_INTENSET_TRIGGERED0_Msk (0x1UL << EGU_INTENSET_TRIGGERED0_Pos) /*!< Bit mask of TRIGGERED0 field. */
#define EGU_INTENSET_TRIGGERED0_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENSET_TRIGGERED0_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENSET_TRIGGERED0_Set (1UL) /*!< Enable */

/* Register: EGU_INTENCLR */
/* Description: Disable interrupt */

/* Bit 15 : Write '1' to disable interrupt for event TRIGGERED[15] */
#define EGU_INTENCLR_TRIGGERED15_Pos (15UL) /*!< Position of TRIGGERED15 field. */
#define EGU_INTENCLR_TRIGGERED15_Msk (0x1UL << EGU_INTENCLR_TRIGGERED15_Pos) /*!< Bit mask of TRIGGERED15 field. */
#define EGU_INTENCLR_TRIGGERED15_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENCLR_TRIGGERED15_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENCLR_TRIGGERED15_Clear (1UL) /*!< Disable */

/* Bit 14 : Write '1' to disable interrupt for event TRIGGERED[14] */
#define EGU_INTENCLR_TRIGGERED14_Pos (14UL) /*!< Position of TRIGGERED14 field. */
#define EGU_INTENCLR_TRIGGERED14_Msk (0x1UL << EGU_INTENCLR_TRIGGERED14_Pos) /*!< Bit mask of TRIGGERED14 field. */
#define EGU_INTENCLR_TRIGGERED14_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENCLR_TRIGGERED14_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENCLR_TRIGGERED14_Clear (1UL) /*!< Disable */

/* Bit 13 : Write '1' to disable interrupt for event TRIGGERED[13] */
#define EGU_INTENCLR_TRIGGERED13_Pos (13UL) /*!< Position of TRIGGERED13 field. */
#define EGU_INTENCLR_TRIGGERED13_Msk (0x1UL << EGU_INTENCLR_TRIGGERED13_Pos) /*!< Bit mask of TRIGGERED13 field. */
#define EGU_INTENCLR_TRIGGERED13_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENCLR_TRIGGERED13_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENCLR_TRIGGERED13_Clear (1UL) /*!< Disable */

/* Bit 12 : Write '1' to disable interrupt for event TRIGGERED[12] */
#define EGU_INTENCLR_TRIGGERED12_Pos (12UL) /*!< Position of TRIGGERED12 field. */
#define EGU_INTENCLR_TRIGGERED12_Msk (0x1UL << EGU_INTENCLR_TRIGGERED12_Pos) /*!< Bit mask of TRIGGERED12 field. */
#define EGU_INTENCLR_TRIGGERED12_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENCLR_TRIGGERED12_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENCLR_TRIGGERED12_Clear (1UL) /*!< Disable */

/* Bit 11 : Write '1' to disable interrupt for event TRIGGERED[11] */
#define EGU_INTENCLR_TRIGGERED11_Pos (11UL) /*!< Position of TRIGGERED11 field. */
#define EGU_INTENCLR_TRIGGERED11_Msk (0x1UL << EGU_INTENCLR_TRIGGERED11_Pos) /*!< Bit mask of TRIGGERED11 field. */
#define EGU_INTENCLR_TRIGGERED11_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENCLR_TRIGGERED11_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENCLR_TRIGGERED11_Clear (1UL) /*!< Disable */

/* Bit 10 : Write '1' to disable interrupt for event TRIGGERED[10] */
#define EGU_INTENCLR_TRIGGERED10_Pos (10UL) /*!< Position of TRIGGERED10 field. */
#define EGU_INTENCLR_TRIGGERED10_Msk (0x1UL << EGU_INTENCLR_TRIGGERED10_Pos) /*!< Bit mask of TRIGGERED10 field. */
#define EGU_INTENCLR_TRIGGERED10_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENCLR_TRIGGERED10_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENCLR_TRIGGERED10_Clear (1UL) /*!< Disable */

/* Bit 9 : Write '1' to disable interrupt for event TRIGGERED[9] */
#define EGU_INTENCLR_TRIGGERED9_Pos (9UL) /*!< Position of TRIGGERED9 field. */
#define EGU_INTENCLR_TRIGGERED9_Msk (0x1UL << EGU_INTENCLR_TRIGGERED9_Pos) /*!< Bit mask of TRIGGERED9 field. */
#define EGU_INTENCLR_TRIGGERED9_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENCLR_TRIGGERED9_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENCLR_TRIGGERED9_Clear (1UL) /*!< Disable */

/* Bit 8 : Write '1' to disable interrupt for event TRIGGERED[8] */
#define EGU_INTENCLR_TRIGGERED8_Pos (8UL) /*!< Position of TRIGGERED8 field. */
#define EGU_INTENCLR_TRIGGERED8_Msk (0x1UL << EGU_INTENCLR_TRIGGERED8_Pos) /*!< Bit mask of TRIGGERED8 field. */
#define EGU_INTENCLR_TRIGGERED8_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENCLR_TRIGGERED8_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENCLR_TRIGGERED8_Clear (1UL) /*!< Disable */

/* Bit 7 : Write '1' to disable interrupt for event TRIGGERED[7] */
#define EGU_INTENCLR_TRIGGERED7_Pos (7UL) /*!< Position of TRIGGERED7 field. */
#define EGU_INTENCLR_TRIGGERED7_Msk (0x1UL << EGU_INTENCLR_TRIGGERED7_Pos) /*!< Bit mask of TRIGGERED7 field. */
#define EGU_INTENCLR_TRIGGERED7_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENCLR_TRIGGERED7_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENCLR_TRIGGERED7_Clear (1UL) /*!< Disable */

/* Bit 6 : Write '1' to disable interrupt for event TRIGGERED[6] */
#define EGU_INTENCLR_TRIGGERED6_Pos (6UL) /*!< Position of TRIGGERED6 field. */
#define EGU_INTENCLR_TRIGGERED6_Msk (0x1UL << EGU_INTENCLR_TRIGGERED6_Pos) /*!< Bit mask of TRIGGERED6 field. */
#define EGU_INTENCLR_TRIGGERED6_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENCLR_TRIGGERED6_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENCLR_TRIGGERED6_Clear (1UL) /*!< Disable */

/* Bit 5 : Write '1' to disable interrupt for event TRIGGERED[5] */
#define EGU_INTENCLR_TRIGGERED5_Pos (5UL) /*!< Position of TRIGGERED5 field. */
#define EGU_INTENCLR_TRIGGERED5_Msk (0x1UL << EGU_INTENCLR_TRIGGERED5_Pos) /*!< Bit mask of TRIGGERED5 field. */
#define EGU_INTENCLR_TRIGGERED5_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENCLR_TRIGGERED5_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENCLR_TRIGGERED5_Clear (1UL) /*!< Disable */

/* Bit 4 : Write '1' to disable interrupt for event TRIGGERED[4] */
#define EGU_INTENCLR_TRIGGERED4_Pos (4UL) /*!< Position of TRIGGERED4 field. */
#define EGU_INTENCLR_TRIGGERED4_Msk (0x1UL << EGU_INTENCLR_TRIGGERED4_Pos) /*!< Bit mask of TRIGGERED4 field. */
#define EGU_INTENCLR_TRIGGERED4_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENCLR_TRIGGERED4_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENCLR_TRIGGERED4_Clear (1UL) /*!< Disable */

/* Bit 3 : Write '1' to disable interrupt for event TRIGGERED[3] */
#define EGU_INTENCLR_TRIGGERED3_Pos (3UL) /*!< Position of TRIGGERED3 field. */
#define EGU_INTENCLR_TRIGGERED3_Msk (0x1UL << EGU_INTENCLR_TRIGGERED3_Pos) /*!< Bit mask of TRIGGERED3 field. */
#define EGU_INTENCLR_TRIGGERED3_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENCLR_TRIGGERED3_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENCLR_TRIGGERED3_Clear (1UL) /*!< Disable */

/* Bit 2 : Write '1' to disable interrupt for event TRIGGERED[2] */
#define EGU_INTENCLR_TRIGGERED2_Pos (2UL) /*!< Position of TRIGGERED2 field. */
#define EGU_INTENCLR_TRIGGERED2_Msk (0x1UL << EGU_INTENCLR_TRIGGERED2_Pos) /*!< Bit mask of TRIGGERED2 field. */
#define EGU_INTENCLR_TRIGGERED2_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENCLR_TRIGGERED2_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENCLR_TRIGGERED2_Clear (1UL) /*!< Disable */

/* Bit 1 : Write '1' to disable interrupt for event TRIGGERED[1] */
#define EGU_INTENCLR_TRIGGERED1_Pos (1UL) /*!< Position of TRIGGERED1 field. */
#define EGU_INTENCLR_TRIGGERED1_Msk (0x1UL << EGU_INTENCLR_TRIGGERED1_Pos) /*!< Bit mask of TRIGGERED1 field. */
#define EGU_INTENCLR_TRIGGERED1_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENCLR_TRIGGERED1_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENCLR_TRIGGERED1_Clear (1UL) /*!< Disable */

/* Bit 0 : Write '1' to disable interrupt for event TRIGGERED[0] */
#define EGU_INTENCLR_TRIGGERED0_Pos (0UL) /*!< Position of TRIGGERED0 field. */
#define EGU_INTENCLR_TRIGGERED0_Msk (0x1UL << EGU_INTENCLR_TRIGGERED0_Pos) /*!< Bit mask of TRIGGERED0 field. */
#define EGU_INTENCLR_TRIGGERED0_Disabled (0UL) /*!< Read: Disabled */
#define EGU_INTENCLR_TRIGGERED0_Enabled (1UL) /*!< Read: Enabled */
#define EGU_INTENCLR_TRIGGERED0_Clear (1UL) /*!< Disable */


/* Peripheral: FICR */
/* Description: Factory Information Configuration Registers */

/* Register: FICR_INFO_DEVICEID */
/* Description: Description collection: Device identifier */

/* Bits 31..0 : 64 bit unique device identifier */
#define FICR_INFO_DEVICEID_DEVICEID_Pos (0UL) /*!< Position of DEVICEID field. */
#define FICR_INFO_DEVICEID_DEVICEID_Msk (0xFFFFFFFFUL << FICR_INFO_DEVICEID_DEVICEID_Pos) /*!< Bit mask of DEVICEID field. */

/* Register: FICR_INFO_PART */
/* Description: Part code */

/* Bits 31..0 : Part code */
#define FICR_INFO_PART_PART_Pos (0UL) /*!< Position of PART field. */
#define FICR_INFO_PART_PART_Msk (0xFFFFFFFFUL << FICR_INFO_PART_PART_Pos) /*!< Bit mask of PART field. */
#define FICR_INFO_PART_PART_N9160 (0x9160UL) /*!< nRF9160 */

/* Register: FICR_INFO_VARIANT */
/* Description: Part Variant, Hardware version and Production configuration */

/* Bits 31..0 : Part Variant, Hardware version and Production configuration, encoded as ASCII */
#define FICR_INFO_VARIANT_VARIANT_Pos (0UL) /*!< Position of VARIANT field. */
#define FICR_INFO_VARIANT_VARIANT_Msk (0xFFFFFFFFUL << FICR_INFO_VARIANT_VARIANT_Pos) /*!< Bit mask of VARIANT field. */
#define FICR_INFO_VARIANT_VARIANT_AAA0 (0x41414130UL) /*!< AAA0 */
#define FICR_INFO_VARIANT_VARIANT_AAAA (0x41414141UL) /*!< AAAA */

/* Register: FICR_INFO_PACKAGE */
/* Description: Package option */

/* Bits 31..0 : Package option */
#define FICR_INFO_PACKAGE_PACKAGE_Pos (0UL) /*!< Position of PACKAGE field. */
#define FICR_INFO_PACKAGE_PACKAGE_Msk (0xFFFFFFFFUL << FICR_INFO_PACKAGE_PACKAGE_Pos) /*!< Bit mask of PACKAGE field. */
#define FICR_INFO_PACKAGE_PACKAGE_CC (0x2000UL) /*!< CCxx - 236 ball wlCSP */

/* Register: FICR_INFO_RAM */
/* Description: RAM variant */

/* Bits 31..0 : RAM variant */
#define FICR_INFO_RAM_RAM_Pos (0UL) /*!< Position of RAM field. */
#define FICR_INFO_RAM_RAM_Msk (0xFFFFFFFFUL << FICR_INFO_RAM_RAM_Pos) /*!< Bit mask of RAM field. */
#define FICR_INFO_RAM_RAM_K256 (0x100UL) /*!< 256  kByte RAM */
#define FICR_INFO_RAM_RAM_Unspecified (0xFFFFFFFFUL) /*!< Unspecified */

/* Register: FICR_INFO_FLASH */
/* Description: Flash variant */

/* Bits 31..0 : Flash variant */
#define FICR_INFO_FLASH_FLASH_Pos (0UL) /*!< Position of FLASH field. */
#define FICR_INFO_FLASH_FLASH_Msk (0xFFFFFFFFUL << FICR_INFO_FLASH_FLASH_Pos) /*!< Bit mask of FLASH field. */
#define FICR_INFO_FLASH_FLASH_K1024 (0x400UL) /*!< 1 MByte FLASH */

/* Register: FICR_INFO_CODEPAGESIZE */
/* Description: Code memory page size */

/* Bits 31..0 : Code memory page size */
#define FICR_INFO_CODEPAGESIZE_CODEPAGESIZE_Pos (0UL) /*!< Position of CODEPAGESIZE field. */
#define FICR_INFO_CODEPAGESIZE_CODEPAGESIZE_Msk (0xFFFFFFFFUL << FICR_INFO_CODEPAGESIZE_CODEPAGESIZE_Pos) /*!< Bit mask of CODEPAGESIZE field. */

/* Register: FICR_INFO_CODESIZE */
/* Description: Code memory size */

/* Bits 31..0 : Code memory size in number of pages */
#define FICR_INFO_CODESIZE_CODESIZE_Pos (0UL) /*!< Position of CODESIZE field. */
#define FICR_INFO_CODESIZE_CODESIZE_Msk (0xFFFFFFFFUL << FICR_INFO_CODESIZE_CODESIZE_Pos) /*!< Bit mask of CODESIZE field. */

/* Register: FICR_INFO_DEVICETYPE */
/* Description: Device type */

/* Bits 31..0 : Device type */
#define FICR_INFO_DEVICETYPE_DEVICETYPE_Pos (0UL) /*!< Position of DEVICETYPE field. */
#define FICR_INFO_DEVICETYPE_DEVICETYPE_Msk (0xFFFFFFFFUL << FICR_INFO_DEVICETYPE_DEVICETYPE_Pos) /*!< Bit mask of DEVICETYPE field. */
#define FICR_INFO_DEVICETYPE_DEVICETYPE_Die (0x0000000UL) /*!< Device is an physical DIE */
#define FICR_INFO_DEVICETYPE_DEVICETYPE_FPGA (0xFFFFFFFFUL) /*!< Device is an FPGA */

/* Register: FICR_TRIMCNF_ADDR */
/* Description: Description cluster: Address */

/* Bits 31..0 : Address */
#define FICR_TRIMCNF_ADDR_Address_Pos (0UL) /*!< Position of Address field. */
#define FICR_TRIMCNF_ADDR_Address_Msk (0xFFFFFFFFUL << FICR_TRIMCNF_ADDR_Address_Pos) /*!< Bit mask of Address field. */

/* Register: FICR_TRIMCNF_DATA */
/* Description: Description cluster: Data */

/* Bits 31..0 : Data */
#define FICR_TRIMCNF_DATA_Data_Pos (0UL) /*!< Position of Data field. */
#define FICR_TRIMCNF_DATA_Data_Msk (0xFFFFFFFFUL << FICR_TRIMCNF_DATA_Data_Pos) /*!< Bit mask of Data field. */

/* Register: FICR_TRNG90B_BYTES */
/* Description: Amount of bytes for the required entropy bits */

/* Bits 31..0 : Amount of bytes for the required entropy bits */
#define FICR_TRNG90B_BYTES_BYTES_Pos (0UL) /*!< Position of BYTES field. */
#define FICR_TRNG90B_BYTES_BYTES_Msk (0xFFFFFFFFUL << FICR_TRNG90B_BYTES_BYTES_Pos) /*!< Bit mask of BYTES field. */

/* Register: FICR_TRNG90B_RCCUTOFF */
/* Description: Repetition counter cutoff */

/* Bits 31..0 : Repetition counter cutoff */
#define FICR_TRNG90B_RCCUTOFF_RCCUTOFF_Pos (0UL) /*!< Position of RCCUTOFF field. */
#define FICR_TRNG90B_RCCUTOFF_RCCUTOFF_Msk (0xFFFFFFFFUL << FICR_TRNG90B_RCCUTOFF_RCCUTOFF_Pos) /*!< Bit mask of RCCUTOFF field. */

/* Register: FICR_TRNG90B_APCUTOFF */
/* Description: Adaptive proportion cutoff */

/* Bits 31..0 : Adaptive proportion cutoff */
#define FICR_TRNG90B_APCUTOFF_APCUTOFF_Pos (0UL) /*!< Position of APCUTOFF field. */
#define FICR_TRNG90B_APCUTOFF_APCUTOFF_Msk (0xFFFFFFFFUL << FICR_TRNG90B_APCUTOFF_APCUTOFF_Pos) /*!< Bit mask of APCUTOFF field. */

/* Register: FICR_TRNG90B_STARTUP */
/* Description: Amount of bytes for the startup tests */

/* Bits 31..0 : Amount of bytes for the startup tests */
#define FICR_TRNG90B_STARTUP_STARTUP_Pos (0UL) /*!< Position of STARTUP field. */
#define FICR_TRNG90B_STARTUP_STARTUP_Msk (0xFFFFFFFFUL << FICR_TRNG90B_STARTUP_STARTUP_Pos) /*!< Bit mask of STARTUP field. */

/* Register: FICR_TRNG90B_ROSC1 */
/* Description: Sample count for ring oscillator 1 */

/* Bits 31..0 : Sample count for ring oscillator 1 */
#define FICR_TRNG90B_ROSC1_ROSC1_Pos (0UL) /*!< Position of ROSC1 field. */
#define FICR_TRNG90B_ROSC1_ROSC1_Msk (0xFFFFFFFFUL << FICR_TRNG90B_ROSC1_ROSC1_Pos) /*!< Bit mask of ROSC1 field. */

/* Register: FICR_TRNG90B_ROSC2 */
/* Description: Sample count for ring oscillator 2 */

/* Bits 31..0 : Sample count for ring oscillator 2 */
#define FICR_TRNG90B_ROSC2_ROSC2_Pos (0UL) /*!< Position of ROSC2 field. */
#define FICR_TRNG90B_ROSC2_ROSC2_Msk (0xFFFFFFFFUL << FICR_TRNG90B_ROSC2_ROSC2_Pos) /*!< Bit mask of ROSC2 field. */

/* Register: FICR_TRNG90B_ROSC3 */
/* Description: Sample count for ring oscillator 3 */

/* Bits 31..0 : Sample count for ring oscillator 3 */
#define FICR_TRNG90B_ROSC3_ROSC3_Pos (0UL) /*!< Position of ROSC3 field. */
#define FICR_TRNG90B_ROSC3_ROSC3_Msk (0xFFFFFFFFUL << FICR_TRNG90B_ROSC3_ROSC3_Pos) /*!< Bit mask of ROSC3 field. */

/* Register: FICR_TRNG90B_ROSC4 */
/* Description: Sample count for ring oscillator 4 */

/* Bits 31..0 : Sample count for ring oscillator 4 */
#define FICR_TRNG90B_ROSC4_ROSC4_Pos (0UL) /*!< Position of ROSC4 field. */
#define FICR_TRNG90B_ROSC4_ROSC4_Msk (0xFFFFFFFFUL << FICR_TRNG90B_ROSC4_ROSC4_Pos) /*!< Bit mask of ROSC4 field. */


/* Peripheral: GPIOTE */
/* Description: GPIO Tasks and Events 0 */

/* Register: GPIOTE_TASKS_OUT */
/* Description: Description collection: Task for writing to pin specified in CONFIG[n].PSEL. Action on pin is configured in CONFIG[n].POLARITY. */

/* Bit 0 : Task for writing to pin specified in CONFIG[n].PSEL. Action on pin is configured in CONFIG[n].POLARITY. */
#define GPIOTE_TASKS_OUT_TASKS_OUT_Pos (0UL) /*!< Position of TASKS_OUT field. */
#define GPIOTE_TASKS_OUT_TASKS_OUT_Msk (0x1UL << GPIOTE_TASKS_OUT_TASKS_OUT_Pos) /*!< Bit mask of TASKS_OUT field. */
#define GPIOTE_TASKS_OUT_TASKS_OUT_Trigger (1UL) /*!< Trigger task */

/* Register: GPIOTE_TASKS_SET */
/* Description: Description collection: Task for writing to pin specified in CONFIG[n].PSEL. Action on pin is to set it high. */

/* Bit 0 : Task for writing to pin specified in CONFIG[n].PSEL. Action on pin is to set it high. */
#define GPIOTE_TASKS_SET_TASKS_SET_Pos (0UL) /*!< Position of TASKS_SET field. */
#define GPIOTE_TASKS_SET_TASKS_SET_Msk (0x1UL << GPIOTE_TASKS_SET_TASKS_SET_Pos) /*!< Bit mask of TASKS_SET field. */
#define GPIOTE_TASKS_SET_TASKS_SET_Trigger (1UL) /*!< Trigger task */

/* Register: GPIOTE_TASKS_CLR */
/* Description: Description collection: Task for writing to pin specified in CONFIG[n].PSEL. Action on pin is to set it low. */

/* Bit 0 : Task for writing to pin specified in CONFIG[n].PSEL. Action on pin is to set it low. */
#define GPIOTE_TASKS_CLR_TASKS_CLR_Pos (0UL) /*!< Position of TASKS_CLR field. */
#define GPIOTE_TASKS_CLR_TASKS_CLR_Msk (0x1UL << GPIOTE_TASKS_CLR_TASKS_CLR_Pos) /*!< Bit mask of TASKS_CLR field. */
#define GPIOTE_TASKS_CLR_TASKS_CLR_Trigger (1UL) /*!< Trigger task */

/* Register: GPIOTE_SUBSCRIBE_OUT */
/* Description: Description collection: Subscribe configuration for task OUT[n] */

/* Bit 31 :   */
#define GPIOTE_SUBSCRIBE_OUT_EN_Pos (31UL) /*!< Position of EN field. */
#define GPIOTE_SUBSCRIBE_OUT_EN_Msk (0x1UL << GPIOTE_SUBSCRIBE_OUT_EN_Pos) /*!< Bit mask of EN field. */
#define GPIOTE_SUBSCRIBE_OUT_EN_Disabled (0UL) /*!< Disable subscription */
#define GPIOTE_SUBSCRIBE_OUT_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task OUT[n] will subscribe to */
#define GPIOTE_SUBSCRIBE_OUT_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define GPIOTE_SUBSCRIBE_OUT_CHIDX_Msk (0xFUL << GPIOTE_SUBSCRIBE_OUT_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: GPIOTE_SUBSCRIBE_SET */
/* Description: Description collection: Subscribe configuration for task SET[n] */

/* Bit 31 :   */
#define GPIOTE_SUBSCRIBE_SET_EN_Pos (31UL) /*!< Position of EN field. */
#define GPIOTE_SUBSCRIBE_SET_EN_Msk (0x1UL << GPIOTE_SUBSCRIBE_SET_EN_Pos) /*!< Bit mask of EN field. */
#define GPIOTE_SUBSCRIBE_SET_EN_Disabled (0UL) /*!< Disable subscription */
#define GPIOTE_SUBSCRIBE_SET_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task SET[n] will subscribe to */
#define GPIOTE_SUBSCRIBE_SET_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define GPIOTE_SUBSCRIBE_SET_CHIDX_Msk (0xFUL << GPIOTE_SUBSCRIBE_SET_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: GPIOTE_SUBSCRIBE_CLR */
/* Description: Description collection: Subscribe configuration for task CLR[n] */

/* Bit 31 :   */
#define GPIOTE_SUBSCRIBE_CLR_EN_Pos (31UL) /*!< Position of EN field. */
#define GPIOTE_SUBSCRIBE_CLR_EN_Msk (0x1UL << GPIOTE_SUBSCRIBE_CLR_EN_Pos) /*!< Bit mask of EN field. */
#define GPIOTE_SUBSCRIBE_CLR_EN_Disabled (0UL) /*!< Disable subscription */
#define GPIOTE_SUBSCRIBE_CLR_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task CLR[n] will subscribe to */
#define GPIOTE_SUBSCRIBE_CLR_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define GPIOTE_SUBSCRIBE_CLR_CHIDX_Msk (0xFUL << GPIOTE_SUBSCRIBE_CLR_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: GPIOTE_EVENTS_IN */
/* Description: Description collection: Event generated from pin specified in CONFIG[n].PSEL */

/* Bit 0 : Event generated from pin specified in CONFIG[n].PSEL */
#define GPIOTE_EVENTS_IN_EVENTS_IN_Pos (0UL) /*!< Position of EVENTS_IN field. */
#define GPIOTE_EVENTS_IN_EVENTS_IN_Msk (0x1UL << GPIOTE_EVENTS_IN_EVENTS_IN_Pos) /*!< Bit mask of EVENTS_IN field. */
#define GPIOTE_EVENTS_IN_EVENTS_IN_NotGenerated (0UL) /*!< Event not generated */
#define GPIOTE_EVENTS_IN_EVENTS_IN_Generated (1UL) /*!< Event generated */

/* Register: GPIOTE_EVENTS_PORT */
/* Description: Event generated from multiple input GPIO pins with SENSE mechanism enabled */

/* Bit 0 : Event generated from multiple input GPIO pins with SENSE mechanism enabled */
#define GPIOTE_EVENTS_PORT_EVENTS_PORT_Pos (0UL) /*!< Position of EVENTS_PORT field. */
#define GPIOTE_EVENTS_PORT_EVENTS_PORT_Msk (0x1UL << GPIOTE_EVENTS_PORT_EVENTS_PORT_Pos) /*!< Bit mask of EVENTS_PORT field. */
#define GPIOTE_EVENTS_PORT_EVENTS_PORT_NotGenerated (0UL) /*!< Event not generated */
#define GPIOTE_EVENTS_PORT_EVENTS_PORT_Generated (1UL) /*!< Event generated */

/* Register: GPIOTE_PUBLISH_IN */
/* Description: Description collection: Publish configuration for event IN[n] */

/* Bit 31 :   */
#define GPIOTE_PUBLISH_IN_EN_Pos (31UL) /*!< Position of EN field. */
#define GPIOTE_PUBLISH_IN_EN_Msk (0x1UL << GPIOTE_PUBLISH_IN_EN_Pos) /*!< Bit mask of EN field. */
#define GPIOTE_PUBLISH_IN_EN_Disabled (0UL) /*!< Disable publishing */
#define GPIOTE_PUBLISH_IN_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event IN[n] will publish to. */
#define GPIOTE_PUBLISH_IN_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define GPIOTE_PUBLISH_IN_CHIDX_Msk (0xFUL << GPIOTE_PUBLISH_IN_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: GPIOTE_PUBLISH_PORT */
/* Description: Publish configuration for event PORT */

/* Bit 31 :   */
#define GPIOTE_PUBLISH_PORT_EN_Pos (31UL) /*!< Position of EN field. */
#define GPIOTE_PUBLISH_PORT_EN_Msk (0x1UL << GPIOTE_PUBLISH_PORT_EN_Pos) /*!< Bit mask of EN field. */
#define GPIOTE_PUBLISH_PORT_EN_Disabled (0UL) /*!< Disable publishing */
#define GPIOTE_PUBLISH_PORT_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event PORT will publish to. */
#define GPIOTE_PUBLISH_PORT_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define GPIOTE_PUBLISH_PORT_CHIDX_Msk (0xFUL << GPIOTE_PUBLISH_PORT_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: GPIOTE_INTENSET */
/* Description: Enable interrupt */

/* Bit 31 : Write '1' to enable interrupt for event PORT */
#define GPIOTE_INTENSET_PORT_Pos (31UL) /*!< Position of PORT field. */
#define GPIOTE_INTENSET_PORT_Msk (0x1UL << GPIOTE_INTENSET_PORT_Pos) /*!< Bit mask of PORT field. */
#define GPIOTE_INTENSET_PORT_Disabled (0UL) /*!< Read: Disabled */
#define GPIOTE_INTENSET_PORT_Enabled (1UL) /*!< Read: Enabled */
#define GPIOTE_INTENSET_PORT_Set (1UL) /*!< Enable */

/* Bit 7 : Write '1' to enable interrupt for event IN[7] */
#define GPIOTE_INTENSET_IN7_Pos (7UL) /*!< Position of IN7 field. */
#define GPIOTE_INTENSET_IN7_Msk (0x1UL << GPIOTE_INTENSET_IN7_Pos) /*!< Bit mask of IN7 field. */
#define GPIOTE_INTENSET_IN7_Disabled (0UL) /*!< Read: Disabled */
#define GPIOTE_INTENSET_IN7_Enabled (1UL) /*!< Read: Enabled */
#define GPIOTE_INTENSET_IN7_Set (1UL) /*!< Enable */

/* Bit 6 : Write '1' to enable interrupt for event IN[6] */
#define GPIOTE_INTENSET_IN6_Pos (6UL) /*!< Position of IN6 field. */
#define GPIOTE_INTENSET_IN6_Msk (0x1UL << GPIOTE_INTENSET_IN6_Pos) /*!< Bit mask of IN6 field. */
#define GPIOTE_INTENSET_IN6_Disabled (0UL) /*!< Read: Disabled */
#define GPIOTE_INTENSET_IN6_Enabled (1UL) /*!< Read: Enabled */
#define GPIOTE_INTENSET_IN6_Set (1UL) /*!< Enable */

/* Bit 5 : Write '1' to enable interrupt for event IN[5] */
#define GPIOTE_INTENSET_IN5_Pos (5UL) /*!< Position of IN5 field. */
#define GPIOTE_INTENSET_IN5_Msk (0x1UL << GPIOTE_INTENSET_IN5_Pos) /*!< Bit mask of IN5 field. */
#define GPIOTE_INTENSET_IN5_Disabled (0UL) /*!< Read: Disabled */
#define GPIOTE_INTENSET_IN5_Enabled (1UL) /*!< Read: Enabled */
#define GPIOTE_INTENSET_IN5_Set (1UL) /*!< Enable */

/* Bit 4 : Write '1' to enable interrupt for event IN[4] */
#define GPIOTE_INTENSET_IN4_Pos (4UL) /*!< Position of IN4 field. */
#define GPIOTE_INTENSET_IN4_Msk (0x1UL << GPIOTE_INTENSET_IN4_Pos) /*!< Bit mask of IN4 field. */
#define GPIOTE_INTENSET_IN4_Disabled (0UL) /*!< Read: Disabled */
#define GPIOTE_INTENSET_IN4_Enabled (1UL) /*!< Read: Enabled */
#define GPIOTE_INTENSET_IN4_Set (1UL) /*!< Enable */

/* Bit 3 : Write '1' to enable interrupt for event IN[3] */
#define GPIOTE_INTENSET_IN3_Pos (3UL) /*!< Position of IN3 field. */
#define GPIOTE_INTENSET_IN3_Msk (0x1UL << GPIOTE_INTENSET_IN3_Pos) /*!< Bit mask of IN3 field. */
#define GPIOTE_INTENSET_IN3_Disabled (0UL) /*!< Read: Disabled */
#define GPIOTE_INTENSET_IN3_Enabled (1UL) /*!< Read: Enabled */
#define GPIOTE_INTENSET_IN3_Set (1UL) /*!< Enable */

/* Bit 2 : Write '1' to enable interrupt for event IN[2] */
#define GPIOTE_INTENSET_IN2_Pos (2UL) /*!< Position of IN2 field. */
#define GPIOTE_INTENSET_IN2_Msk (0x1UL << GPIOTE_INTENSET_IN2_Pos) /*!< Bit mask of IN2 field. */
#define GPIOTE_INTENSET_IN2_Disabled (0UL) /*!< Read: Disabled */
#define GPIOTE_INTENSET_IN2_Enabled (1UL) /*!< Read: Enabled */
#define GPIOTE_INTENSET_IN2_Set (1UL) /*!< Enable */

/* Bit 1 : Write '1' to enable interrupt for event IN[1] */
#define GPIOTE_INTENSET_IN1_Pos (1UL) /*!< Position of IN1 field. */
#define GPIOTE_INTENSET_IN1_Msk (0x1UL << GPIOTE_INTENSET_IN1_Pos) /*!< Bit mask of IN1 field. */
#define GPIOTE_INTENSET_IN1_Disabled (0UL) /*!< Read: Disabled */
#define GPIOTE_INTENSET_IN1_Enabled (1UL) /*!< Read: Enabled */
#define GPIOTE_INTENSET_IN1_Set (1UL) /*!< Enable */

/* Bit 0 : Write '1' to enable interrupt for event IN[0] */
#define GPIOTE_INTENSET_IN0_Pos (0UL) /*!< Position of IN0 field. */
#define GPIOTE_INTENSET_IN0_Msk (0x1UL << GPIOTE_INTENSET_IN0_Pos) /*!< Bit mask of IN0 field. */
#define GPIOTE_INTENSET_IN0_Disabled (0UL) /*!< Read: Disabled */
#define GPIOTE_INTENSET_IN0_Enabled (1UL) /*!< Read: Enabled */
#define GPIOTE_INTENSET_IN0_Set (1UL) /*!< Enable */

/* Register: GPIOTE_INTENCLR */
/* Description: Disable interrupt */

/* Bit 31 : Write '1' to disable interrupt for event PORT */
#define GPIOTE_INTENCLR_PORT_Pos (31UL) /*!< Position of PORT field. */
#define GPIOTE_INTENCLR_PORT_Msk (0x1UL << GPIOTE_INTENCLR_PORT_Pos) /*!< Bit mask of PORT field. */
#define GPIOTE_INTENCLR_PORT_Disabled (0UL) /*!< Read: Disabled */
#define GPIOTE_INTENCLR_PORT_Enabled (1UL) /*!< Read: Enabled */
#define GPIOTE_INTENCLR_PORT_Clear (1UL) /*!< Disable */

/* Bit 7 : Write '1' to disable interrupt for event IN[7] */
#define GPIOTE_INTENCLR_IN7_Pos (7UL) /*!< Position of IN7 field. */
#define GPIOTE_INTENCLR_IN7_Msk (0x1UL << GPIOTE_INTENCLR_IN7_Pos) /*!< Bit mask of IN7 field. */
#define GPIOTE_INTENCLR_IN7_Disabled (0UL) /*!< Read: Disabled */
#define GPIOTE_INTENCLR_IN7_Enabled (1UL) /*!< Read: Enabled */
#define GPIOTE_INTENCLR_IN7_Clear (1UL) /*!< Disable */

/* Bit 6 : Write '1' to disable interrupt for event IN[6] */
#define GPIOTE_INTENCLR_IN6_Pos (6UL) /*!< Position of IN6 field. */
#define GPIOTE_INTENCLR_IN6_Msk (0x1UL << GPIOTE_INTENCLR_IN6_Pos) /*!< Bit mask of IN6 field. */
#define GPIOTE_INTENCLR_IN6_Disabled (0UL) /*!< Read: Disabled */
#define GPIOTE_INTENCLR_IN6_Enabled (1UL) /*!< Read: Enabled */
#define GPIOTE_INTENCLR_IN6_Clear (1UL) /*!< Disable */

/* Bit 5 : Write '1' to disable interrupt for event IN[5] */
#define GPIOTE_INTENCLR_IN5_Pos (5UL) /*!< Position of IN5 field. */
#define GPIOTE_INTENCLR_IN5_Msk (0x1UL << GPIOTE_INTENCLR_IN5_Pos) /*!< Bit mask of IN5 field. */
#define GPIOTE_INTENCLR_IN5_Disabled (0UL) /*!< Read: Disabled */
#define GPIOTE_INTENCLR_IN5_Enabled (1UL) /*!< Read: Enabled */
#define GPIOTE_INTENCLR_IN5_Clear (1UL) /*!< Disable */

/* Bit 4 : Write '1' to disable interrupt for event IN[4] */
#define GPIOTE_INTENCLR_IN4_Pos (4UL) /*!< Position of IN4 field. */
#define GPIOTE_INTENCLR_IN4_Msk (0x1UL << GPIOTE_INTENCLR_IN4_Pos) /*!< Bit mask of IN4 field. */
#define GPIOTE_INTENCLR_IN4_Disabled (0UL) /*!< Read: Disabled */
#define GPIOTE_INTENCLR_IN4_Enabled (1UL) /*!< Read: Enabled */
#define GPIOTE_INTENCLR_IN4_Clear (1UL) /*!< Disable */

/* Bit 3 : Write '1' to disable interrupt for event IN[3] */
#define GPIOTE_INTENCLR_IN3_Pos (3UL) /*!< Position of IN3 field. */
#define GPIOTE_INTENCLR_IN3_Msk (0x1UL << GPIOTE_INTENCLR_IN3_Pos) /*!< Bit mask of IN3 field. */
#define GPIOTE_INTENCLR_IN3_Disabled (0UL) /*!< Read: Disabled */
#define GPIOTE_INTENCLR_IN3_Enabled (1UL) /*!< Read: Enabled */
#define GPIOTE_INTENCLR_IN3_Clear (1UL) /*!< Disable */

/* Bit 2 : Write '1' to disable interrupt for event IN[2] */
#define GPIOTE_INTENCLR_IN2_Pos (2UL) /*!< Position of IN2 field. */
#define GPIOTE_INTENCLR_IN2_Msk (0x1UL << GPIOTE_INTENCLR_IN2_Pos) /*!< Bit mask of IN2 field. */
#define GPIOTE_INTENCLR_IN2_Disabled (0UL) /*!< Read: Disabled */
#define GPIOTE_INTENCLR_IN2_Enabled (1UL) /*!< Read: Enabled */
#define GPIOTE_INTENCLR_IN2_Clear (1UL) /*!< Disable */

/* Bit 1 : Write '1' to disable interrupt for event IN[1] */
#define GPIOTE_INTENCLR_IN1_Pos (1UL) /*!< Position of IN1 field. */
#define GPIOTE_INTENCLR_IN1_Msk (0x1UL << GPIOTE_INTENCLR_IN1_Pos) /*!< Bit mask of IN1 field. */
#define GPIOTE_INTENCLR_IN1_Disabled (0UL) /*!< Read: Disabled */
#define GPIOTE_INTENCLR_IN1_Enabled (1UL) /*!< Read: Enabled */
#define GPIOTE_INTENCLR_IN1_Clear (1UL) /*!< Disable */

/* Bit 0 : Write '1' to disable interrupt for event IN[0] */
#define GPIOTE_INTENCLR_IN0_Pos (0UL) /*!< Position of IN0 field. */
#define GPIOTE_INTENCLR_IN0_Msk (0x1UL << GPIOTE_INTENCLR_IN0_Pos) /*!< Bit mask of IN0 field. */
#define GPIOTE_INTENCLR_IN0_Disabled (0UL) /*!< Read: Disabled */
#define GPIOTE_INTENCLR_IN0_Enabled (1UL) /*!< Read: Enabled */
#define GPIOTE_INTENCLR_IN0_Clear (1UL) /*!< Disable */

/* Register: GPIOTE_CONFIG */
/* Description: Description collection: Configuration for OUT[n], SET[n] and CLR[n] tasks and IN[n] event */

/* Bit 20 : When in task mode: Initial value of the output when the GPIOTE channel is configured. When in event mode: No effect. */
#define GPIOTE_CONFIG_OUTINIT_Pos (20UL) /*!< Position of OUTINIT field. */
#define GPIOTE_CONFIG_OUTINIT_Msk (0x1UL << GPIOTE_CONFIG_OUTINIT_Pos) /*!< Bit mask of OUTINIT field. */
#define GPIOTE_CONFIG_OUTINIT_Low (0UL) /*!< Task mode: Initial value of pin before task triggering is low */
#define GPIOTE_CONFIG_OUTINIT_High (1UL) /*!< Task mode: Initial value of pin before task triggering is high */

/* Bits 17..16 : When In task mode: Operation to be performed on output when OUT[n] task is triggered. When In event mode: Operation on input that shall trigger IN[n] event. */
#define GPIOTE_CONFIG_POLARITY_Pos (16UL) /*!< Position of POLARITY field. */
#define GPIOTE_CONFIG_POLARITY_Msk (0x3UL << GPIOTE_CONFIG_POLARITY_Pos) /*!< Bit mask of POLARITY field. */
#define GPIOTE_CONFIG_POLARITY_None (0UL) /*!< Task mode: No effect on pin from OUT[n] task. Event mode: no IN[n] event generated on pin activity. */
#define GPIOTE_CONFIG_POLARITY_LoToHi (1UL) /*!< Task mode: Set pin from OUT[n] task. Event mode: Generate IN[n] event when rising edge on pin. */
#define GPIOTE_CONFIG_POLARITY_HiToLo (2UL) /*!< Task mode: Clear pin from OUT[n] task. Event mode: Generate IN[n] event when falling edge on pin. */
#define GPIOTE_CONFIG_POLARITY_Toggle (3UL) /*!< Task mode: Toggle pin from OUT[n]. Event mode: Generate IN[n] when any change on pin. */

/* Bits 12..8 : GPIO number associated with SET[n], CLR[n] and OUT[n] tasks and IN[n] event */
#define GPIOTE_CONFIG_PSEL_Pos (8UL) /*!< Position of PSEL field. */
#define GPIOTE_CONFIG_PSEL_Msk (0x1FUL << GPIOTE_CONFIG_PSEL_Pos) /*!< Bit mask of PSEL field. */

/* Bits 1..0 : Mode */
#define GPIOTE_CONFIG_MODE_Pos (0UL) /*!< Position of MODE field. */
#define GPIOTE_CONFIG_MODE_Msk (0x3UL << GPIOTE_CONFIG_MODE_Pos) /*!< Bit mask of MODE field. */
#define GPIOTE_CONFIG_MODE_Disabled (0UL) /*!< Disabled. Pin specified by PSEL will not be acquired by the GPIOTE module. */
#define GPIOTE_CONFIG_MODE_Event (1UL) /*!< Event mode */
#define GPIOTE_CONFIG_MODE_Task (3UL) /*!< Task mode */


/* Peripheral: I2S */
/* Description: Inter-IC Sound 0 */

/* Register: I2S_TASKS_START */
/* Description: Starts continuous I2S transfer. Also starts MCK generator when this is enabled. */

/* Bit 0 : Starts continuous I2S transfer. Also starts MCK generator when this is enabled. */
#define I2S_TASKS_START_TASKS_START_Pos (0UL) /*!< Position of TASKS_START field. */
#define I2S_TASKS_START_TASKS_START_Msk (0x1UL << I2S_TASKS_START_TASKS_START_Pos) /*!< Bit mask of TASKS_START field. */
#define I2S_TASKS_START_TASKS_START_Trigger (1UL) /*!< Trigger task */

/* Register: I2S_TASKS_STOP */
/* Description: Stops I2S transfer. Also stops MCK generator. Triggering this task will cause the STOPPED event to be generated. */

/* Bit 0 : Stops I2S transfer. Also stops MCK generator. Triggering this task will cause the STOPPED event to be generated. */
#define I2S_TASKS_STOP_TASKS_STOP_Pos (0UL) /*!< Position of TASKS_STOP field. */
#define I2S_TASKS_STOP_TASKS_STOP_Msk (0x1UL << I2S_TASKS_STOP_TASKS_STOP_Pos) /*!< Bit mask of TASKS_STOP field. */
#define I2S_TASKS_STOP_TASKS_STOP_Trigger (1UL) /*!< Trigger task */

/* Register: I2S_SUBSCRIBE_START */
/* Description: Subscribe configuration for task START */

/* Bit 31 :   */
#define I2S_SUBSCRIBE_START_EN_Pos (31UL) /*!< Position of EN field. */
#define I2S_SUBSCRIBE_START_EN_Msk (0x1UL << I2S_SUBSCRIBE_START_EN_Pos) /*!< Bit mask of EN field. */
#define I2S_SUBSCRIBE_START_EN_Disabled (0UL) /*!< Disable subscription */
#define I2S_SUBSCRIBE_START_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task START will subscribe to */
#define I2S_SUBSCRIBE_START_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define I2S_SUBSCRIBE_START_CHIDX_Msk (0xFUL << I2S_SUBSCRIBE_START_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: I2S_SUBSCRIBE_STOP */
/* Description: Subscribe configuration for task STOP */

/* Bit 31 :   */
#define I2S_SUBSCRIBE_STOP_EN_Pos (31UL) /*!< Position of EN field. */
#define I2S_SUBSCRIBE_STOP_EN_Msk (0x1UL << I2S_SUBSCRIBE_STOP_EN_Pos) /*!< Bit mask of EN field. */
#define I2S_SUBSCRIBE_STOP_EN_Disabled (0UL) /*!< Disable subscription */
#define I2S_SUBSCRIBE_STOP_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task STOP will subscribe to */
#define I2S_SUBSCRIBE_STOP_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define I2S_SUBSCRIBE_STOP_CHIDX_Msk (0xFUL << I2S_SUBSCRIBE_STOP_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: I2S_EVENTS_RXPTRUPD */
/* Description: The RXD.PTR register has been copied to internal double-buffers.
      When the I2S module is started and RX is enabled, this event will be generated for every RXTXD.MAXCNT words that are received on the SDIN pin. */

/* Bit 0 : The RXD.PTR register has been copied to internal double-buffers.
      When the I2S module is started and RX is enabled, this event will be generated for every RXTXD.MAXCNT words that are received on the SDIN pin. */
#define I2S_EVENTS_RXPTRUPD_EVENTS_RXPTRUPD_Pos (0UL) /*!< Position of EVENTS_RXPTRUPD field. */
#define I2S_EVENTS_RXPTRUPD_EVENTS_RXPTRUPD_Msk (0x1UL << I2S_EVENTS_RXPTRUPD_EVENTS_RXPTRUPD_Pos) /*!< Bit mask of EVENTS_RXPTRUPD field. */
#define I2S_EVENTS_RXPTRUPD_EVENTS_RXPTRUPD_NotGenerated (0UL) /*!< Event not generated */
#define I2S_EVENTS_RXPTRUPD_EVENTS_RXPTRUPD_Generated (1UL) /*!< Event generated */

/* Register: I2S_EVENTS_STOPPED */
/* Description: I2S transfer stopped. */

/* Bit 0 : I2S transfer stopped. */
#define I2S_EVENTS_STOPPED_EVENTS_STOPPED_Pos (0UL) /*!< Position of EVENTS_STOPPED field. */
#define I2S_EVENTS_STOPPED_EVENTS_STOPPED_Msk (0x1UL << I2S_EVENTS_STOPPED_EVENTS_STOPPED_Pos) /*!< Bit mask of EVENTS_STOPPED field. */
#define I2S_EVENTS_STOPPED_EVENTS_STOPPED_NotGenerated (0UL) /*!< Event not generated */
#define I2S_EVENTS_STOPPED_EVENTS_STOPPED_Generated (1UL) /*!< Event generated */

/* Register: I2S_EVENTS_TXPTRUPD */
/* Description: The TDX.PTR register has been copied to internal double-buffers.
      When the I2S module is started and TX is enabled, this event will be generated for every RXTXD.MAXCNT words that are sent on the SDOUT pin. */

/* Bit 0 : The TDX.PTR register has been copied to internal double-buffers.
      When the I2S module is started and TX is enabled, this event will be generated for every RXTXD.MAXCNT words that are sent on the SDOUT pin. */
#define I2S_EVENTS_TXPTRUPD_EVENTS_TXPTRUPD_Pos (0UL) /*!< Position of EVENTS_TXPTRUPD field. */
#define I2S_EVENTS_TXPTRUPD_EVENTS_TXPTRUPD_Msk (0x1UL << I2S_EVENTS_TXPTRUPD_EVENTS_TXPTRUPD_Pos) /*!< Bit mask of EVENTS_TXPTRUPD field. */
#define I2S_EVENTS_TXPTRUPD_EVENTS_TXPTRUPD_NotGenerated (0UL) /*!< Event not generated */
#define I2S_EVENTS_TXPTRUPD_EVENTS_TXPTRUPD_Generated (1UL) /*!< Event generated */

/* Register: I2S_PUBLISH_RXPTRUPD */
/* Description: Publish configuration for event RXPTRUPD */

/* Bit 31 :   */
#define I2S_PUBLISH_RXPTRUPD_EN_Pos (31UL) /*!< Position of EN field. */
#define I2S_PUBLISH_RXPTRUPD_EN_Msk (0x1UL << I2S_PUBLISH_RXPTRUPD_EN_Pos) /*!< Bit mask of EN field. */
#define I2S_PUBLISH_RXPTRUPD_EN_Disabled (0UL) /*!< Disable publishing */
#define I2S_PUBLISH_RXPTRUPD_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event RXPTRUPD will publish to. */
#define I2S_PUBLISH_RXPTRUPD_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define I2S_PUBLISH_RXPTRUPD_CHIDX_Msk (0xFUL << I2S_PUBLISH_RXPTRUPD_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: I2S_PUBLISH_STOPPED */
/* Description: Publish configuration for event STOPPED */

/* Bit 31 :   */
#define I2S_PUBLISH_STOPPED_EN_Pos (31UL) /*!< Position of EN field. */
#define I2S_PUBLISH_STOPPED_EN_Msk (0x1UL << I2S_PUBLISH_STOPPED_EN_Pos) /*!< Bit mask of EN field. */
#define I2S_PUBLISH_STOPPED_EN_Disabled (0UL) /*!< Disable publishing */
#define I2S_PUBLISH_STOPPED_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event STOPPED will publish to. */
#define I2S_PUBLISH_STOPPED_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define I2S_PUBLISH_STOPPED_CHIDX_Msk (0xFUL << I2S_PUBLISH_STOPPED_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: I2S_PUBLISH_TXPTRUPD */
/* Description: Publish configuration for event TXPTRUPD */

/* Bit 31 :   */
#define I2S_PUBLISH_TXPTRUPD_EN_Pos (31UL) /*!< Position of EN field. */
#define I2S_PUBLISH_TXPTRUPD_EN_Msk (0x1UL << I2S_PUBLISH_TXPTRUPD_EN_Pos) /*!< Bit mask of EN field. */
#define I2S_PUBLISH_TXPTRUPD_EN_Disabled (0UL) /*!< Disable publishing */
#define I2S_PUBLISH_TXPTRUPD_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event TXPTRUPD will publish to. */
#define I2S_PUBLISH_TXPTRUPD_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define I2S_PUBLISH_TXPTRUPD_CHIDX_Msk (0xFUL << I2S_PUBLISH_TXPTRUPD_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: I2S_INTEN */
/* Description: Enable or disable interrupt */

/* Bit 5 : Enable or disable interrupt for event TXPTRUPD */
#define I2S_INTEN_TXPTRUPD_Pos (5UL) /*!< Position of TXPTRUPD field. */
#define I2S_INTEN_TXPTRUPD_Msk (0x1UL << I2S_INTEN_TXPTRUPD_Pos) /*!< Bit mask of TXPTRUPD field. */
#define I2S_INTEN_TXPTRUPD_Disabled (0UL) /*!< Disable */
#define I2S_INTEN_TXPTRUPD_Enabled (1UL) /*!< Enable */

/* Bit 2 : Enable or disable interrupt for event STOPPED */
#define I2S_INTEN_STOPPED_Pos (2UL) /*!< Position of STOPPED field. */
#define I2S_INTEN_STOPPED_Msk (0x1UL << I2S_INTEN_STOPPED_Pos) /*!< Bit mask of STOPPED field. */
#define I2S_INTEN_STOPPED_Disabled (0UL) /*!< Disable */
#define I2S_INTEN_STOPPED_Enabled (1UL) /*!< Enable */

/* Bit 1 : Enable or disable interrupt for event RXPTRUPD */
#define I2S_INTEN_RXPTRUPD_Pos (1UL) /*!< Position of RXPTRUPD field. */
#define I2S_INTEN_RXPTRUPD_Msk (0x1UL << I2S_INTEN_RXPTRUPD_Pos) /*!< Bit mask of RXPTRUPD field. */
#define I2S_INTEN_RXPTRUPD_Disabled (0UL) /*!< Disable */
#define I2S_INTEN_RXPTRUPD_Enabled (1UL) /*!< Enable */

/* Register: I2S_INTENSET */
/* Description: Enable interrupt */

/* Bit 5 : Write '1' to enable interrupt for event TXPTRUPD */
#define I2S_INTENSET_TXPTRUPD_Pos (5UL) /*!< Position of TXPTRUPD field. */
#define I2S_INTENSET_TXPTRUPD_Msk (0x1UL << I2S_INTENSET_TXPTRUPD_Pos) /*!< Bit mask of TXPTRUPD field. */
#define I2S_INTENSET_TXPTRUPD_Disabled (0UL) /*!< Read: Disabled */
#define I2S_INTENSET_TXPTRUPD_Enabled (1UL) /*!< Read: Enabled */
#define I2S_INTENSET_TXPTRUPD_Set (1UL) /*!< Enable */

/* Bit 2 : Write '1' to enable interrupt for event STOPPED */
#define I2S_INTENSET_STOPPED_Pos (2UL) /*!< Position of STOPPED field. */
#define I2S_INTENSET_STOPPED_Msk (0x1UL << I2S_INTENSET_STOPPED_Pos) /*!< Bit mask of STOPPED field. */
#define I2S_INTENSET_STOPPED_Disabled (0UL) /*!< Read: Disabled */
#define I2S_INTENSET_STOPPED_Enabled (1UL) /*!< Read: Enabled */
#define I2S_INTENSET_STOPPED_Set (1UL) /*!< Enable */

/* Bit 1 : Write '1' to enable interrupt for event RXPTRUPD */
#define I2S_INTENSET_RXPTRUPD_Pos (1UL) /*!< Position of RXPTRUPD field. */
#define I2S_INTENSET_RXPTRUPD_Msk (0x1UL << I2S_INTENSET_RXPTRUPD_Pos) /*!< Bit mask of RXPTRUPD field. */
#define I2S_INTENSET_RXPTRUPD_Disabled (0UL) /*!< Read: Disabled */
#define I2S_INTENSET_RXPTRUPD_Enabled (1UL) /*!< Read: Enabled */
#define I2S_INTENSET_RXPTRUPD_Set (1UL) /*!< Enable */

/* Register: I2S_INTENCLR */
/* Description: Disable interrupt */

/* Bit 5 : Write '1' to disable interrupt for event TXPTRUPD */
#define I2S_INTENCLR_TXPTRUPD_Pos (5UL) /*!< Position of TXPTRUPD field. */
#define I2S_INTENCLR_TXPTRUPD_Msk (0x1UL << I2S_INTENCLR_TXPTRUPD_Pos) /*!< Bit mask of TXPTRUPD field. */
#define I2S_INTENCLR_TXPTRUPD_Disabled (0UL) /*!< Read: Disabled */
#define I2S_INTENCLR_TXPTRUPD_Enabled (1UL) /*!< Read: Enabled */
#define I2S_INTENCLR_TXPTRUPD_Clear (1UL) /*!< Disable */

/* Bit 2 : Write '1' to disable interrupt for event STOPPED */
#define I2S_INTENCLR_STOPPED_Pos (2UL) /*!< Position of STOPPED field. */
#define I2S_INTENCLR_STOPPED_Msk (0x1UL << I2S_INTENCLR_STOPPED_Pos) /*!< Bit mask of STOPPED field. */
#define I2S_INTENCLR_STOPPED_Disabled (0UL) /*!< Read: Disabled */
#define I2S_INTENCLR_STOPPED_Enabled (1UL) /*!< Read: Enabled */
#define I2S_INTENCLR_STOPPED_Clear (1UL) /*!< Disable */

/* Bit 1 : Write '1' to disable interrupt for event RXPTRUPD */
#define I2S_INTENCLR_RXPTRUPD_Pos (1UL) /*!< Position of RXPTRUPD field. */
#define I2S_INTENCLR_RXPTRUPD_Msk (0x1UL << I2S_INTENCLR_RXPTRUPD_Pos) /*!< Bit mask of RXPTRUPD field. */
#define I2S_INTENCLR_RXPTRUPD_Disabled (0UL) /*!< Read: Disabled */
#define I2S_INTENCLR_RXPTRUPD_Enabled (1UL) /*!< Read: Enabled */
#define I2S_INTENCLR_RXPTRUPD_Clear (1UL) /*!< Disable */

/* Register: I2S_ENABLE */
/* Description: Enable I2S module. */

/* Bit 0 : Enable I2S module. */
#define I2S_ENABLE_ENABLE_Pos (0UL) /*!< Position of ENABLE field. */
#define I2S_ENABLE_ENABLE_Msk (0x1UL << I2S_ENABLE_ENABLE_Pos) /*!< Bit mask of ENABLE field. */
#define I2S_ENABLE_ENABLE_Disabled (0UL) /*!< Disable */
#define I2S_ENABLE_ENABLE_Enabled (1UL) /*!< Enable */

/* Register: I2S_CONFIG_MODE */
/* Description: I2S mode. */

/* Bit 0 : I2S mode. */
#define I2S_CONFIG_MODE_MODE_Pos (0UL) /*!< Position of MODE field. */
#define I2S_CONFIG_MODE_MODE_Msk (0x1UL << I2S_CONFIG_MODE_MODE_Pos) /*!< Bit mask of MODE field. */
#define I2S_CONFIG_MODE_MODE_Master (0UL) /*!< Master mode. SCK and LRCK generated from internal master clcok (MCK) and output on pins defined by PSEL.xxx. */
#define I2S_CONFIG_MODE_MODE_Slave (1UL) /*!< Slave mode. SCK and LRCK generated by external master and received on pins defined by PSEL.xxx */

/* Register: I2S_CONFIG_RXEN */
/* Description: Reception (RX) enable. */

/* Bit 0 : Reception (RX) enable. */
#define I2S_CONFIG_RXEN_RXEN_Pos (0UL) /*!< Position of RXEN field. */
#define I2S_CONFIG_RXEN_RXEN_Msk (0x1UL << I2S_CONFIG_RXEN_RXEN_Pos) /*!< Bit mask of RXEN field. */
#define I2S_CONFIG_RXEN_RXEN_Disabled (0UL) /*!< Reception disabled and now data will be written to the RXD.PTR address. */
#define I2S_CONFIG_RXEN_RXEN_Enabled (1UL) /*!< Reception enabled. */

/* Register: I2S_CONFIG_TXEN */
/* Description: Transmission (TX) enable. */

/* Bit 0 : Transmission (TX) enable. */
#define I2S_CONFIG_TXEN_TXEN_Pos (0UL) /*!< Position of TXEN field. */
#define I2S_CONFIG_TXEN_TXEN_Msk (0x1UL << I2S_CONFIG_TXEN_TXEN_Pos) /*!< Bit mask of TXEN field. */
#define I2S_CONFIG_TXEN_TXEN_Disabled (0UL) /*!< Transmission disabled and now data will be read from the RXD.TXD address. */
#define I2S_CONFIG_TXEN_TXEN_Enabled (1UL) /*!< Transmission enabled. */

/* Register: I2S_CONFIG_MCKEN */
/* Description: Master clock generator enable. */

/* Bit 0 : Master clock generator enable. */
#define I2S_CONFIG_MCKEN_MCKEN_Pos (0UL) /*!< Position of MCKEN field. */
#define I2S_CONFIG_MCKEN_MCKEN_Msk (0x1UL << I2S_CONFIG_MCKEN_MCKEN_Pos) /*!< Bit mask of MCKEN field. */
#define I2S_CONFIG_MCKEN_MCKEN_Disabled (0UL) /*!< Master clock generator disabled and PSEL.MCK not connected(available as GPIO). */
#define I2S_CONFIG_MCKEN_MCKEN_Enabled (1UL) /*!< Master clock generator running and MCK output on PSEL.MCK. */

/* Register: I2S_CONFIG_MCKFREQ */
/* Description: Master clock generator frequency. */

/* Bits 31..0 : Master clock generator frequency. */
#define I2S_CONFIG_MCKFREQ_MCKFREQ_Pos (0UL) /*!< Position of MCKFREQ field. */
#define I2S_CONFIG_MCKFREQ_MCKFREQ_Msk (0xFFFFFFFFUL << I2S_CONFIG_MCKFREQ_MCKFREQ_Pos) /*!< Bit mask of MCKFREQ field. */
#define I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV125 (0x020C0000UL) /*!< 32 MHz / 125 = 0.256 MHz */
#define I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV63 (0x04100000UL) /*!< 32 MHz / 63 = 0.5079365 MHz */
#define I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV42 (0x06000000UL) /*!< 32 MHz / 42 = 0.7619048 MHz */
#define I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV32 (0x08000000UL) /*!< 32 MHz / 32 = 1.0 MHz */
#define I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV31 (0x08400000UL) /*!< 32 MHz / 31 = 1.0322581 MHz */
#define I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV30 (0x08800000UL) /*!< 32 MHz / 30 = 1.0666667 MHz */
#define I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV23 (0x0B000000UL) /*!< 32 MHz / 23 = 1.3913043 MHz */
#define I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV21 (0x0C000000UL) /*!< 32 MHz / 21 = 1.5238095 */
#define I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV16 (0x10000000UL) /*!< 32 MHz / 16 = 2.0 MHz */
#define I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV15 (0x11000000UL) /*!< 32 MHz / 15 = 2.1333333 MHz */
#define I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV11 (0x16000000UL) /*!< 32 MHz / 11 = 2.9090909 MHz */
#define I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV10 (0x18000000UL) /*!< 32 MHz / 10 = 3.2 MHz */
#define I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV8 (0x20000000UL) /*!< 32 MHz / 8 = 4.0 MHz */

/* Register: I2S_CONFIG_RATIO */
/* Description: MCK / LRCK ratio. */

/* Bits 3..0 : MCK / LRCK ratio. */
#define I2S_CONFIG_RATIO_RATIO_Pos (0UL) /*!< Position of RATIO field. */
#define I2S_CONFIG_RATIO_RATIO_Msk (0xFUL << I2S_CONFIG_RATIO_RATIO_Pos) /*!< Bit mask of RATIO field. */
#define I2S_CONFIG_RATIO_RATIO_32X (0UL) /*!< LRCK = MCK / 32 */
#define I2S_CONFIG_RATIO_RATIO_48X (1UL) /*!< LRCK = MCK / 48 */
#define I2S_CONFIG_RATIO_RATIO_64X (2UL) /*!< LRCK = MCK / 64 */
#define I2S_CONFIG_RATIO_RATIO_96X (3UL) /*!< LRCK = MCK / 96 */
#define I2S_CONFIG_RATIO_RATIO_128X (4UL) /*!< LRCK = MCK / 128 */
#define I2S_CONFIG_RATIO_RATIO_192X (5UL) /*!< LRCK = MCK / 192 */
#define I2S_CONFIG_RATIO_RATIO_256X (6UL) /*!< LRCK = MCK / 256 */
#define I2S_CONFIG_RATIO_RATIO_384X (7UL) /*!< LRCK = MCK / 384 */
#define I2S_CONFIG_RATIO_RATIO_512X (8UL) /*!< LRCK = MCK / 512 */

/* Register: I2S_CONFIG_SWIDTH */
/* Description: Sample width. */

/* Bits 1..0 : Sample width. */
#define I2S_CONFIG_SWIDTH_SWIDTH_Pos (0UL) /*!< Position of SWIDTH field. */
#define I2S_CONFIG_SWIDTH_SWIDTH_Msk (0x3UL << I2S_CONFIG_SWIDTH_SWIDTH_Pos) /*!< Bit mask of SWIDTH field. */
#define I2S_CONFIG_SWIDTH_SWIDTH_8Bit (0UL) /*!< 8 bit. */
#define I2S_CONFIG_SWIDTH_SWIDTH_16Bit (1UL) /*!< 16 bit. */
#define I2S_CONFIG_SWIDTH_SWIDTH_24Bit (2UL) /*!< 24 bit. */

/* Register: I2S_CONFIG_ALIGN */
/* Description: Alignment of sample within a frame. */

/* Bit 0 : Alignment of sample within a frame. */
#define I2S_CONFIG_ALIGN_ALIGN_Pos (0UL) /*!< Position of ALIGN field. */
#define I2S_CONFIG_ALIGN_ALIGN_Msk (0x1UL << I2S_CONFIG_ALIGN_ALIGN_Pos) /*!< Bit mask of ALIGN field. */
#define I2S_CONFIG_ALIGN_ALIGN_Left (0UL) /*!< Left-aligned. */
#define I2S_CONFIG_ALIGN_ALIGN_Right (1UL) /*!< Right-aligned. */

/* Register: I2S_CONFIG_FORMAT */
/* Description: Frame format. */

/* Bit 0 : Frame format. */
#define I2S_CONFIG_FORMAT_FORMAT_Pos (0UL) /*!< Position of FORMAT field. */
#define I2S_CONFIG_FORMAT_FORMAT_Msk (0x1UL << I2S_CONFIG_FORMAT_FORMAT_Pos) /*!< Bit mask of FORMAT field. */
#define I2S_CONFIG_FORMAT_FORMAT_I2S (0UL) /*!< Original I2S format. */
#define I2S_CONFIG_FORMAT_FORMAT_Aligned (1UL) /*!< Alternate (left- or right-aligned) format. */

/* Register: I2S_CONFIG_CHANNELS */
/* Description: Enable channels. */

/* Bits 1..0 : Enable channels. */
#define I2S_CONFIG_CHANNELS_CHANNELS_Pos (0UL) /*!< Position of CHANNELS field. */
#define I2S_CONFIG_CHANNELS_CHANNELS_Msk (0x3UL << I2S_CONFIG_CHANNELS_CHANNELS_Pos) /*!< Bit mask of CHANNELS field. */
#define I2S_CONFIG_CHANNELS_CHANNELS_Stereo (0UL) /*!< Stereo. */
#define I2S_CONFIG_CHANNELS_CHANNELS_Left (1UL) /*!< Left only. */
#define I2S_CONFIG_CHANNELS_CHANNELS_Right (2UL) /*!< Right only. */

/* Register: I2S_RXD_PTR */
/* Description: Receive buffer RAM start address. */

/* Bits 31..0 : Receive buffer Data RAM start address. When receiving, words containing samples will be written to this address. This address is a word aligned Data RAM address. */
#define I2S_RXD_PTR_PTR_Pos (0UL) /*!< Position of PTR field. */
#define I2S_RXD_PTR_PTR_Msk (0xFFFFFFFFUL << I2S_RXD_PTR_PTR_Pos) /*!< Bit mask of PTR field. */

/* Register: I2S_TXD_PTR */
/* Description: Transmit buffer RAM start address. */

/* Bits 31..0 : Transmit buffer Data RAM start address. When transmitting, words containing samples will be fetched from this address. This address is a word aligned Data RAM address. */
#define I2S_TXD_PTR_PTR_Pos (0UL) /*!< Position of PTR field. */
#define I2S_TXD_PTR_PTR_Msk (0xFFFFFFFFUL << I2S_TXD_PTR_PTR_Pos) /*!< Bit mask of PTR field. */

/* Register: I2S_RXTXD_MAXCNT */
/* Description: Size of RXD and TXD buffers. */

/* Bits 13..0 : Size of RXD and TXD buffers in number of 32 bit words. */
#define I2S_RXTXD_MAXCNT_MAXCNT_Pos (0UL) /*!< Position of MAXCNT field. */
#define I2S_RXTXD_MAXCNT_MAXCNT_Msk (0x3FFFUL << I2S_RXTXD_MAXCNT_MAXCNT_Pos) /*!< Bit mask of MAXCNT field. */

/* Register: I2S_PSEL_MCK */
/* Description: Pin select for MCK signal. */

/* Bit 31 : Connection */
#define I2S_PSEL_MCK_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define I2S_PSEL_MCK_CONNECT_Msk (0x1UL << I2S_PSEL_MCK_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define I2S_PSEL_MCK_CONNECT_Connected (0UL) /*!< Connect */
#define I2S_PSEL_MCK_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define I2S_PSEL_MCK_PIN_Pos (0UL) /*!< Position of PIN field. */
#define I2S_PSEL_MCK_PIN_Msk (0x1FUL << I2S_PSEL_MCK_PIN_Pos) /*!< Bit mask of PIN field. */

/* Register: I2S_PSEL_SCK */
/* Description: Pin select for SCK signal. */

/* Bit 31 : Connection */
#define I2S_PSEL_SCK_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define I2S_PSEL_SCK_CONNECT_Msk (0x1UL << I2S_PSEL_SCK_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define I2S_PSEL_SCK_CONNECT_Connected (0UL) /*!< Connect */
#define I2S_PSEL_SCK_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define I2S_PSEL_SCK_PIN_Pos (0UL) /*!< Position of PIN field. */
#define I2S_PSEL_SCK_PIN_Msk (0x1FUL << I2S_PSEL_SCK_PIN_Pos) /*!< Bit mask of PIN field. */

/* Register: I2S_PSEL_LRCK */
/* Description: Pin select for LRCK signal. */

/* Bit 31 : Connection */
#define I2S_PSEL_LRCK_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define I2S_PSEL_LRCK_CONNECT_Msk (0x1UL << I2S_PSEL_LRCK_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define I2S_PSEL_LRCK_CONNECT_Connected (0UL) /*!< Connect */
#define I2S_PSEL_LRCK_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define I2S_PSEL_LRCK_PIN_Pos (0UL) /*!< Position of PIN field. */
#define I2S_PSEL_LRCK_PIN_Msk (0x1FUL << I2S_PSEL_LRCK_PIN_Pos) /*!< Bit mask of PIN field. */

/* Register: I2S_PSEL_SDIN */
/* Description: Pin select for SDIN signal. */

/* Bit 31 : Connection */
#define I2S_PSEL_SDIN_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define I2S_PSEL_SDIN_CONNECT_Msk (0x1UL << I2S_PSEL_SDIN_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define I2S_PSEL_SDIN_CONNECT_Connected (0UL) /*!< Connect */
#define I2S_PSEL_SDIN_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define I2S_PSEL_SDIN_PIN_Pos (0UL) /*!< Position of PIN field. */
#define I2S_PSEL_SDIN_PIN_Msk (0x1FUL << I2S_PSEL_SDIN_PIN_Pos) /*!< Bit mask of PIN field. */

/* Register: I2S_PSEL_SDOUT */
/* Description: Pin select for SDOUT signal. */

/* Bit 31 : Connection */
#define I2S_PSEL_SDOUT_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define I2S_PSEL_SDOUT_CONNECT_Msk (0x1UL << I2S_PSEL_SDOUT_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define I2S_PSEL_SDOUT_CONNECT_Connected (0UL) /*!< Connect */
#define I2S_PSEL_SDOUT_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define I2S_PSEL_SDOUT_PIN_Pos (0UL) /*!< Position of PIN field. */
#define I2S_PSEL_SDOUT_PIN_Msk (0x1FUL << I2S_PSEL_SDOUT_PIN_Pos) /*!< Bit mask of PIN field. */


/* Peripheral: IPC */
/* Description: Inter Processor Communication 0 */

/* Register: IPC_TASKS_SEND */
/* Description: Description collection: Trigger events on channel enabled in SEND_CNF[n]. */

/* Bit 0 : Trigger events on channel enabled in SEND_CNF[n]. */
#define IPC_TASKS_SEND_TASKS_SEND_Pos (0UL) /*!< Position of TASKS_SEND field. */
#define IPC_TASKS_SEND_TASKS_SEND_Msk (0x1UL << IPC_TASKS_SEND_TASKS_SEND_Pos) /*!< Bit mask of TASKS_SEND field. */
#define IPC_TASKS_SEND_TASKS_SEND_Trigger (1UL) /*!< Trigger task */

/* Register: IPC_SUBSCRIBE_SEND */
/* Description: Description collection: Subscribe configuration for task SEND[n] */

/* Bit 31 :   */
#define IPC_SUBSCRIBE_SEND_EN_Pos (31UL) /*!< Position of EN field. */
#define IPC_SUBSCRIBE_SEND_EN_Msk (0x1UL << IPC_SUBSCRIBE_SEND_EN_Pos) /*!< Bit mask of EN field. */
#define IPC_SUBSCRIBE_SEND_EN_Disabled (0UL) /*!< Disable subscription */
#define IPC_SUBSCRIBE_SEND_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task SEND[n] will subscribe to */
#define IPC_SUBSCRIBE_SEND_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define IPC_SUBSCRIBE_SEND_CHIDX_Msk (0xFUL << IPC_SUBSCRIBE_SEND_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: IPC_EVENTS_RECEIVE */
/* Description: Description collection: Event received on one or more of the enabled channels in RECEIVE_CNF[n]. */

/* Bit 0 : Event received on one or more of the enabled channels in RECEIVE_CNF[n]. */
#define IPC_EVENTS_RECEIVE_EVENTS_RECEIVE_Pos (0UL) /*!< Position of EVENTS_RECEIVE field. */
#define IPC_EVENTS_RECEIVE_EVENTS_RECEIVE_Msk (0x1UL << IPC_EVENTS_RECEIVE_EVENTS_RECEIVE_Pos) /*!< Bit mask of EVENTS_RECEIVE field. */
#define IPC_EVENTS_RECEIVE_EVENTS_RECEIVE_NotGenerated (0UL) /*!< Event not generated */
#define IPC_EVENTS_RECEIVE_EVENTS_RECEIVE_Generated (1UL) /*!< Event generated */

/* Register: IPC_PUBLISH_RECEIVE */
/* Description: Description collection: Publish configuration for event RECEIVE[n] */

/* Bit 31 :   */
#define IPC_PUBLISH_RECEIVE_EN_Pos (31UL) /*!< Position of EN field. */
#define IPC_PUBLISH_RECEIVE_EN_Msk (0x1UL << IPC_PUBLISH_RECEIVE_EN_Pos) /*!< Bit mask of EN field. */
#define IPC_PUBLISH_RECEIVE_EN_Disabled (0UL) /*!< Disable publishing */
#define IPC_PUBLISH_RECEIVE_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event RECEIVE[n] will publish to. */
#define IPC_PUBLISH_RECEIVE_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define IPC_PUBLISH_RECEIVE_CHIDX_Msk (0xFUL << IPC_PUBLISH_RECEIVE_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: IPC_INTEN */
/* Description: Enable or disable interrupt */

/* Bit 7 : Enable or disable interrupt for event RECEIVE[7] */
#define IPC_INTEN_RECEIVE7_Pos (7UL) /*!< Position of RECEIVE7 field. */
#define IPC_INTEN_RECEIVE7_Msk (0x1UL << IPC_INTEN_RECEIVE7_Pos) /*!< Bit mask of RECEIVE7 field. */
#define IPC_INTEN_RECEIVE7_Disabled (0UL) /*!< Disable */
#define IPC_INTEN_RECEIVE7_Enabled (1UL) /*!< Enable */

/* Bit 6 : Enable or disable interrupt for event RECEIVE[6] */
#define IPC_INTEN_RECEIVE6_Pos (6UL) /*!< Position of RECEIVE6 field. */
#define IPC_INTEN_RECEIVE6_Msk (0x1UL << IPC_INTEN_RECEIVE6_Pos) /*!< Bit mask of RECEIVE6 field. */
#define IPC_INTEN_RECEIVE6_Disabled (0UL) /*!< Disable */
#define IPC_INTEN_RECEIVE6_Enabled (1UL) /*!< Enable */

/* Bit 5 : Enable or disable interrupt for event RECEIVE[5] */
#define IPC_INTEN_RECEIVE5_Pos (5UL) /*!< Position of RECEIVE5 field. */
#define IPC_INTEN_RECEIVE5_Msk (0x1UL << IPC_INTEN_RECEIVE5_Pos) /*!< Bit mask of RECEIVE5 field. */
#define IPC_INTEN_RECEIVE5_Disabled (0UL) /*!< Disable */
#define IPC_INTEN_RECEIVE5_Enabled (1UL) /*!< Enable */

/* Bit 4 : Enable or disable interrupt for event RECEIVE[4] */
#define IPC_INTEN_RECEIVE4_Pos (4UL) /*!< Position of RECEIVE4 field. */
#define IPC_INTEN_RECEIVE4_Msk (0x1UL << IPC_INTEN_RECEIVE4_Pos) /*!< Bit mask of RECEIVE4 field. */
#define IPC_INTEN_RECEIVE4_Disabled (0UL) /*!< Disable */
#define IPC_INTEN_RECEIVE4_Enabled (1UL) /*!< Enable */

/* Bit 3 : Enable or disable interrupt for event RECEIVE[3] */
#define IPC_INTEN_RECEIVE3_Pos (3UL) /*!< Position of RECEIVE3 field. */
#define IPC_INTEN_RECEIVE3_Msk (0x1UL << IPC_INTEN_RECEIVE3_Pos) /*!< Bit mask of RECEIVE3 field. */
#define IPC_INTEN_RECEIVE3_Disabled (0UL) /*!< Disable */
#define IPC_INTEN_RECEIVE3_Enabled (1UL) /*!< Enable */

/* Bit 2 : Enable or disable interrupt for event RECEIVE[2] */
#define IPC_INTEN_RECEIVE2_Pos (2UL) /*!< Position of RECEIVE2 field. */
#define IPC_INTEN_RECEIVE2_Msk (0x1UL << IPC_INTEN_RECEIVE2_Pos) /*!< Bit mask of RECEIVE2 field. */
#define IPC_INTEN_RECEIVE2_Disabled (0UL) /*!< Disable */
#define IPC_INTEN_RECEIVE2_Enabled (1UL) /*!< Enable */

/* Bit 1 : Enable or disable interrupt for event RECEIVE[1] */
#define IPC_INTEN_RECEIVE1_Pos (1UL) /*!< Position of RECEIVE1 field. */
#define IPC_INTEN_RECEIVE1_Msk (0x1UL << IPC_INTEN_RECEIVE1_Pos) /*!< Bit mask of RECEIVE1 field. */
#define IPC_INTEN_RECEIVE1_Disabled (0UL) /*!< Disable */
#define IPC_INTEN_RECEIVE1_Enabled (1UL) /*!< Enable */

/* Bit 0 : Enable or disable interrupt for event RECEIVE[0] */
#define IPC_INTEN_RECEIVE0_Pos (0UL) /*!< Position of RECEIVE0 field. */
#define IPC_INTEN_RECEIVE0_Msk (0x1UL << IPC_INTEN_RECEIVE0_Pos) /*!< Bit mask of RECEIVE0 field. */
#define IPC_INTEN_RECEIVE0_Disabled (0UL) /*!< Disable */
#define IPC_INTEN_RECEIVE0_Enabled (1UL) /*!< Enable */

/* Register: IPC_INTENSET */
/* Description: Enable interrupt */

/* Bit 7 : Write '1' to enable interrupt for event RECEIVE[7] */
#define IPC_INTENSET_RECEIVE7_Pos (7UL) /*!< Position of RECEIVE7 field. */
#define IPC_INTENSET_RECEIVE7_Msk (0x1UL << IPC_INTENSET_RECEIVE7_Pos) /*!< Bit mask of RECEIVE7 field. */
#define IPC_INTENSET_RECEIVE7_Disabled (0UL) /*!< Read: Disabled */
#define IPC_INTENSET_RECEIVE7_Enabled (1UL) /*!< Read: Enabled */
#define IPC_INTENSET_RECEIVE7_Set (1UL) /*!< Enable */

/* Bit 6 : Write '1' to enable interrupt for event RECEIVE[6] */
#define IPC_INTENSET_RECEIVE6_Pos (6UL) /*!< Position of RECEIVE6 field. */
#define IPC_INTENSET_RECEIVE6_Msk (0x1UL << IPC_INTENSET_RECEIVE6_Pos) /*!< Bit mask of RECEIVE6 field. */
#define IPC_INTENSET_RECEIVE6_Disabled (0UL) /*!< Read: Disabled */
#define IPC_INTENSET_RECEIVE6_Enabled (1UL) /*!< Read: Enabled */
#define IPC_INTENSET_RECEIVE6_Set (1UL) /*!< Enable */

/* Bit 5 : Write '1' to enable interrupt for event RECEIVE[5] */
#define IPC_INTENSET_RECEIVE5_Pos (5UL) /*!< Position of RECEIVE5 field. */
#define IPC_INTENSET_RECEIVE5_Msk (0x1UL << IPC_INTENSET_RECEIVE5_Pos) /*!< Bit mask of RECEIVE5 field. */
#define IPC_INTENSET_RECEIVE5_Disabled (0UL) /*!< Read: Disabled */
#define IPC_INTENSET_RECEIVE5_Enabled (1UL) /*!< Read: Enabled */
#define IPC_INTENSET_RECEIVE5_Set (1UL) /*!< Enable */

/* Bit 4 : Write '1' to enable interrupt for event RECEIVE[4] */
#define IPC_INTENSET_RECEIVE4_Pos (4UL) /*!< Position of RECEIVE4 field. */
#define IPC_INTENSET_RECEIVE4_Msk (0x1UL << IPC_INTENSET_RECEIVE4_Pos) /*!< Bit mask of RECEIVE4 field. */
#define IPC_INTENSET_RECEIVE4_Disabled (0UL) /*!< Read: Disabled */
#define IPC_INTENSET_RECEIVE4_Enabled (1UL) /*!< Read: Enabled */
#define IPC_INTENSET_RECEIVE4_Set (1UL) /*!< Enable */

/* Bit 3 : Write '1' to enable interrupt for event RECEIVE[3] */
#define IPC_INTENSET_RECEIVE3_Pos (3UL) /*!< Position of RECEIVE3 field. */
#define IPC_INTENSET_RECEIVE3_Msk (0x1UL << IPC_INTENSET_RECEIVE3_Pos) /*!< Bit mask of RECEIVE3 field. */
#define IPC_INTENSET_RECEIVE3_Disabled (0UL) /*!< Read: Disabled */
#define IPC_INTENSET_RECEIVE3_Enabled (1UL) /*!< Read: Enabled */
#define IPC_INTENSET_RECEIVE3_Set (1UL) /*!< Enable */

/* Bit 2 : Write '1' to enable interrupt for event RECEIVE[2] */
#define IPC_INTENSET_RECEIVE2_Pos (2UL) /*!< Position of RECEIVE2 field. */
#define IPC_INTENSET_RECEIVE2_Msk (0x1UL << IPC_INTENSET_RECEIVE2_Pos) /*!< Bit mask of RECEIVE2 field. */
#define IPC_INTENSET_RECEIVE2_Disabled (0UL) /*!< Read: Disabled */
#define IPC_INTENSET_RECEIVE2_Enabled (1UL) /*!< Read: Enabled */
#define IPC_INTENSET_RECEIVE2_Set (1UL) /*!< Enable */

/* Bit 1 : Write '1' to enable interrupt for event RECEIVE[1] */
#define IPC_INTENSET_RECEIVE1_Pos (1UL) /*!< Position of RECEIVE1 field. */
#define IPC_INTENSET_RECEIVE1_Msk (0x1UL << IPC_INTENSET_RECEIVE1_Pos) /*!< Bit mask of RECEIVE1 field. */
#define IPC_INTENSET_RECEIVE1_Disabled (0UL) /*!< Read: Disabled */
#define IPC_INTENSET_RECEIVE1_Enabled (1UL) /*!< Read: Enabled */
#define IPC_INTENSET_RECEIVE1_Set (1UL) /*!< Enable */

/* Bit 0 : Write '1' to enable interrupt for event RECEIVE[0] */
#define IPC_INTENSET_RECEIVE0_Pos (0UL) /*!< Position of RECEIVE0 field. */
#define IPC_INTENSET_RECEIVE0_Msk (0x1UL << IPC_INTENSET_RECEIVE0_Pos) /*!< Bit mask of RECEIVE0 field. */
#define IPC_INTENSET_RECEIVE0_Disabled (0UL) /*!< Read: Disabled */
#define IPC_INTENSET_RECEIVE0_Enabled (1UL) /*!< Read: Enabled */
#define IPC_INTENSET_RECEIVE0_Set (1UL) /*!< Enable */

/* Register: IPC_INTENCLR */
/* Description: Disable interrupt */

/* Bit 7 : Write '1' to disable interrupt for event RECEIVE[7] */
#define IPC_INTENCLR_RECEIVE7_Pos (7UL) /*!< Position of RECEIVE7 field. */
#define IPC_INTENCLR_RECEIVE7_Msk (0x1UL << IPC_INTENCLR_RECEIVE7_Pos) /*!< Bit mask of RECEIVE7 field. */
#define IPC_INTENCLR_RECEIVE7_Disabled (0UL) /*!< Read: Disabled */
#define IPC_INTENCLR_RECEIVE7_Enabled (1UL) /*!< Read: Enabled */
#define IPC_INTENCLR_RECEIVE7_Clear (1UL) /*!< Disable */

/* Bit 6 : Write '1' to disable interrupt for event RECEIVE[6] */
#define IPC_INTENCLR_RECEIVE6_Pos (6UL) /*!< Position of RECEIVE6 field. */
#define IPC_INTENCLR_RECEIVE6_Msk (0x1UL << IPC_INTENCLR_RECEIVE6_Pos) /*!< Bit mask of RECEIVE6 field. */
#define IPC_INTENCLR_RECEIVE6_Disabled (0UL) /*!< Read: Disabled */
#define IPC_INTENCLR_RECEIVE6_Enabled (1UL) /*!< Read: Enabled */
#define IPC_INTENCLR_RECEIVE6_Clear (1UL) /*!< Disable */

/* Bit 5 : Write '1' to disable interrupt for event RECEIVE[5] */
#define IPC_INTENCLR_RECEIVE5_Pos (5UL) /*!< Position of RECEIVE5 field. */
#define IPC_INTENCLR_RECEIVE5_Msk (0x1UL << IPC_INTENCLR_RECEIVE5_Pos) /*!< Bit mask of RECEIVE5 field. */
#define IPC_INTENCLR_RECEIVE5_Disabled (0UL) /*!< Read: Disabled */
#define IPC_INTENCLR_RECEIVE5_Enabled (1UL) /*!< Read: Enabled */
#define IPC_INTENCLR_RECEIVE5_Clear (1UL) /*!< Disable */

/* Bit 4 : Write '1' to disable interrupt for event RECEIVE[4] */
#define IPC_INTENCLR_RECEIVE4_Pos (4UL) /*!< Position of RECEIVE4 field. */
#define IPC_INTENCLR_RECEIVE4_Msk (0x1UL << IPC_INTENCLR_RECEIVE4_Pos) /*!< Bit mask of RECEIVE4 field. */
#define IPC_INTENCLR_RECEIVE4_Disabled (0UL) /*!< Read: Disabled */
#define IPC_INTENCLR_RECEIVE4_Enabled (1UL) /*!< Read: Enabled */
#define IPC_INTENCLR_RECEIVE4_Clear (1UL) /*!< Disable */

/* Bit 3 : Write '1' to disable interrupt for event RECEIVE[3] */
#define IPC_INTENCLR_RECEIVE3_Pos (3UL) /*!< Position of RECEIVE3 field. */
#define IPC_INTENCLR_RECEIVE3_Msk (0x1UL << IPC_INTENCLR_RECEIVE3_Pos) /*!< Bit mask of RECEIVE3 field. */
#define IPC_INTENCLR_RECEIVE3_Disabled (0UL) /*!< Read: Disabled */
#define IPC_INTENCLR_RECEIVE3_Enabled (1UL) /*!< Read: Enabled */
#define IPC_INTENCLR_RECEIVE3_Clear (1UL) /*!< Disable */

/* Bit 2 : Write '1' to disable interrupt for event RECEIVE[2] */
#define IPC_INTENCLR_RECEIVE2_Pos (2UL) /*!< Position of RECEIVE2 field. */
#define IPC_INTENCLR_RECEIVE2_Msk (0x1UL << IPC_INTENCLR_RECEIVE2_Pos) /*!< Bit mask of RECEIVE2 field. */
#define IPC_INTENCLR_RECEIVE2_Disabled (0UL) /*!< Read: Disabled */
#define IPC_INTENCLR_RECEIVE2_Enabled (1UL) /*!< Read: Enabled */
#define IPC_INTENCLR_RECEIVE2_Clear (1UL) /*!< Disable */

/* Bit 1 : Write '1' to disable interrupt for event RECEIVE[1] */
#define IPC_INTENCLR_RECEIVE1_Pos (1UL) /*!< Position of RECEIVE1 field. */
#define IPC_INTENCLR_RECEIVE1_Msk (0x1UL << IPC_INTENCLR_RECEIVE1_Pos) /*!< Bit mask of RECEIVE1 field. */
#define IPC_INTENCLR_RECEIVE1_Disabled (0UL) /*!< Read: Disabled */
#define IPC_INTENCLR_RECEIVE1_Enabled (1UL) /*!< Read: Enabled */
#define IPC_INTENCLR_RECEIVE1_Clear (1UL) /*!< Disable */

/* Bit 0 : Write '1' to disable interrupt for event RECEIVE[0] */
#define IPC_INTENCLR_RECEIVE0_Pos (0UL) /*!< Position of RECEIVE0 field. */
#define IPC_INTENCLR_RECEIVE0_Msk (0x1UL << IPC_INTENCLR_RECEIVE0_Pos) /*!< Bit mask of RECEIVE0 field. */
#define IPC_INTENCLR_RECEIVE0_Disabled (0UL) /*!< Read: Disabled */
#define IPC_INTENCLR_RECEIVE0_Enabled (1UL) /*!< Read: Enabled */
#define IPC_INTENCLR_RECEIVE0_Clear (1UL) /*!< Disable */

/* Register: IPC_INTPEND */
/* Description: Pending interrupts */

/* Bit 7 : Read pending status of interrupt for event RECEIVE[7] */
#define IPC_INTPEND_RECEIVE7_Pos (7UL) /*!< Position of RECEIVE7 field. */
#define IPC_INTPEND_RECEIVE7_Msk (0x1UL << IPC_INTPEND_RECEIVE7_Pos) /*!< Bit mask of RECEIVE7 field. */
#define IPC_INTPEND_RECEIVE7_NotPending (0UL) /*!< Read: Not pending */
#define IPC_INTPEND_RECEIVE7_Pending (1UL) /*!< Read: Pending */

/* Bit 6 : Read pending status of interrupt for event RECEIVE[6] */
#define IPC_INTPEND_RECEIVE6_Pos (6UL) /*!< Position of RECEIVE6 field. */
#define IPC_INTPEND_RECEIVE6_Msk (0x1UL << IPC_INTPEND_RECEIVE6_Pos) /*!< Bit mask of RECEIVE6 field. */
#define IPC_INTPEND_RECEIVE6_NotPending (0UL) /*!< Read: Not pending */
#define IPC_INTPEND_RECEIVE6_Pending (1UL) /*!< Read: Pending */

/* Bit 5 : Read pending status of interrupt for event RECEIVE[5] */
#define IPC_INTPEND_RECEIVE5_Pos (5UL) /*!< Position of RECEIVE5 field. */
#define IPC_INTPEND_RECEIVE5_Msk (0x1UL << IPC_INTPEND_RECEIVE5_Pos) /*!< Bit mask of RECEIVE5 field. */
#define IPC_INTPEND_RECEIVE5_NotPending (0UL) /*!< Read: Not pending */
#define IPC_INTPEND_RECEIVE5_Pending (1UL) /*!< Read: Pending */

/* Bit 4 : Read pending status of interrupt for event RECEIVE[4] */
#define IPC_INTPEND_RECEIVE4_Pos (4UL) /*!< Position of RECEIVE4 field. */
#define IPC_INTPEND_RECEIVE4_Msk (0x1UL << IPC_INTPEND_RECEIVE4_Pos) /*!< Bit mask of RECEIVE4 field. */
#define IPC_INTPEND_RECEIVE4_NotPending (0UL) /*!< Read: Not pending */
#define IPC_INTPEND_RECEIVE4_Pending (1UL) /*!< Read: Pending */

/* Bit 3 : Read pending status of interrupt for event RECEIVE[3] */
#define IPC_INTPEND_RECEIVE3_Pos (3UL) /*!< Position of RECEIVE3 field. */
#define IPC_INTPEND_RECEIVE3_Msk (0x1UL << IPC_INTPEND_RECEIVE3_Pos) /*!< Bit mask of RECEIVE3 field. */
#define IPC_INTPEND_RECEIVE3_NotPending (0UL) /*!< Read: Not pending */
#define IPC_INTPEND_RECEIVE3_Pending (1UL) /*!< Read: Pending */

/* Bit 2 : Read pending status of interrupt for event RECEIVE[2] */
#define IPC_INTPEND_RECEIVE2_Pos (2UL) /*!< Position of RECEIVE2 field. */
#define IPC_INTPEND_RECEIVE2_Msk (0x1UL << IPC_INTPEND_RECEIVE2_Pos) /*!< Bit mask of RECEIVE2 field. */
#define IPC_INTPEND_RECEIVE2_NotPending (0UL) /*!< Read: Not pending */
#define IPC_INTPEND_RECEIVE2_Pending (1UL) /*!< Read: Pending */

/* Bit 1 : Read pending status of interrupt for event RECEIVE[1] */
#define IPC_INTPEND_RECEIVE1_Pos (1UL) /*!< Position of RECEIVE1 field. */
#define IPC_INTPEND_RECEIVE1_Msk (0x1UL << IPC_INTPEND_RECEIVE1_Pos) /*!< Bit mask of RECEIVE1 field. */
#define IPC_INTPEND_RECEIVE1_NotPending (0UL) /*!< Read: Not pending */
#define IPC_INTPEND_RECEIVE1_Pending (1UL) /*!< Read: Pending */

/* Bit 0 : Read pending status of interrupt for event RECEIVE[0] */
#define IPC_INTPEND_RECEIVE0_Pos (0UL) /*!< Position of RECEIVE0 field. */
#define IPC_INTPEND_RECEIVE0_Msk (0x1UL << IPC_INTPEND_RECEIVE0_Pos) /*!< Bit mask of RECEIVE0 field. */
#define IPC_INTPEND_RECEIVE0_NotPending (0UL) /*!< Read: Not pending */
#define IPC_INTPEND_RECEIVE0_Pending (1UL) /*!< Read: Pending */

/* Register: IPC_SEND_CNF */
/* Description: Description collection: Send event configuration for TASKS_SEND[n]. */

/* Bit 7 : Enable broadcasting on channel 7. */
#define IPC_SEND_CNF_CHEN7_Pos (7UL) /*!< Position of CHEN7 field. */
#define IPC_SEND_CNF_CHEN7_Msk (0x1UL << IPC_SEND_CNF_CHEN7_Pos) /*!< Bit mask of CHEN7 field. */
#define IPC_SEND_CNF_CHEN7_Disable (0UL) /*!< Disable broadcast. */
#define IPC_SEND_CNF_CHEN7_Enable (1UL) /*!< Enable broadcast. */

/* Bit 6 : Enable broadcasting on channel 6. */
#define IPC_SEND_CNF_CHEN6_Pos (6UL) /*!< Position of CHEN6 field. */
#define IPC_SEND_CNF_CHEN6_Msk (0x1UL << IPC_SEND_CNF_CHEN6_Pos) /*!< Bit mask of CHEN6 field. */
#define IPC_SEND_CNF_CHEN6_Disable (0UL) /*!< Disable broadcast. */
#define IPC_SEND_CNF_CHEN6_Enable (1UL) /*!< Enable broadcast. */

/* Bit 5 : Enable broadcasting on channel 5. */
#define IPC_SEND_CNF_CHEN5_Pos (5UL) /*!< Position of CHEN5 field. */
#define IPC_SEND_CNF_CHEN5_Msk (0x1UL << IPC_SEND_CNF_CHEN5_Pos) /*!< Bit mask of CHEN5 field. */
#define IPC_SEND_CNF_CHEN5_Disable (0UL) /*!< Disable broadcast. */
#define IPC_SEND_CNF_CHEN5_Enable (1UL) /*!< Enable broadcast. */

/* Bit 4 : Enable broadcasting on channel 4. */
#define IPC_SEND_CNF_CHEN4_Pos (4UL) /*!< Position of CHEN4 field. */
#define IPC_SEND_CNF_CHEN4_Msk (0x1UL << IPC_SEND_CNF_CHEN4_Pos) /*!< Bit mask of CHEN4 field. */
#define IPC_SEND_CNF_CHEN4_Disable (0UL) /*!< Disable broadcast. */
#define IPC_SEND_CNF_CHEN4_Enable (1UL) /*!< Enable broadcast. */

/* Bit 3 : Enable broadcasting on channel 3. */
#define IPC_SEND_CNF_CHEN3_Pos (3UL) /*!< Position of CHEN3 field. */
#define IPC_SEND_CNF_CHEN3_Msk (0x1UL << IPC_SEND_CNF_CHEN3_Pos) /*!< Bit mask of CHEN3 field. */
#define IPC_SEND_CNF_CHEN3_Disable (0UL) /*!< Disable broadcast. */
#define IPC_SEND_CNF_CHEN3_Enable (1UL) /*!< Enable broadcast. */

/* Bit 2 : Enable broadcasting on channel 2. */
#define IPC_SEND_CNF_CHEN2_Pos (2UL) /*!< Position of CHEN2 field. */
#define IPC_SEND_CNF_CHEN2_Msk (0x1UL << IPC_SEND_CNF_CHEN2_Pos) /*!< Bit mask of CHEN2 field. */
#define IPC_SEND_CNF_CHEN2_Disable (0UL) /*!< Disable broadcast. */
#define IPC_SEND_CNF_CHEN2_Enable (1UL) /*!< Enable broadcast. */

/* Bit 1 : Enable broadcasting on channel 1. */
#define IPC_SEND_CNF_CHEN1_Pos (1UL) /*!< Position of CHEN1 field. */
#define IPC_SEND_CNF_CHEN1_Msk (0x1UL << IPC_SEND_CNF_CHEN1_Pos) /*!< Bit mask of CHEN1 field. */
#define IPC_SEND_CNF_CHEN1_Disable (0UL) /*!< Disable broadcast. */
#define IPC_SEND_CNF_CHEN1_Enable (1UL) /*!< Enable broadcast. */

/* Bit 0 : Enable broadcasting on channel 0. */
#define IPC_SEND_CNF_CHEN0_Pos (0UL) /*!< Position of CHEN0 field. */
#define IPC_SEND_CNF_CHEN0_Msk (0x1UL << IPC_SEND_CNF_CHEN0_Pos) /*!< Bit mask of CHEN0 field. */
#define IPC_SEND_CNF_CHEN0_Disable (0UL) /*!< Disable broadcast. */
#define IPC_SEND_CNF_CHEN0_Enable (1UL) /*!< Enable broadcast. */

/* Register: IPC_RECEIVE_CNF */
/* Description: Description collection: Receive event configuration for EVENTS_RECEIVE[n]. */

/* Bit 7 : Enable subscription to channel 7. */
#define IPC_RECEIVE_CNF_CHEN7_Pos (7UL) /*!< Position of CHEN7 field. */
#define IPC_RECEIVE_CNF_CHEN7_Msk (0x1UL << IPC_RECEIVE_CNF_CHEN7_Pos) /*!< Bit mask of CHEN7 field. */
#define IPC_RECEIVE_CNF_CHEN7_Disable (0UL) /*!< Disable events. */
#define IPC_RECEIVE_CNF_CHEN7_Enable (1UL) /*!< Enable events. */

/* Bit 6 : Enable subscription to channel 6. */
#define IPC_RECEIVE_CNF_CHEN6_Pos (6UL) /*!< Position of CHEN6 field. */
#define IPC_RECEIVE_CNF_CHEN6_Msk (0x1UL << IPC_RECEIVE_CNF_CHEN6_Pos) /*!< Bit mask of CHEN6 field. */
#define IPC_RECEIVE_CNF_CHEN6_Disable (0UL) /*!< Disable events. */
#define IPC_RECEIVE_CNF_CHEN6_Enable (1UL) /*!< Enable events. */

/* Bit 5 : Enable subscription to channel 5. */
#define IPC_RECEIVE_CNF_CHEN5_Pos (5UL) /*!< Position of CHEN5 field. */
#define IPC_RECEIVE_CNF_CHEN5_Msk (0x1UL << IPC_RECEIVE_CNF_CHEN5_Pos) /*!< Bit mask of CHEN5 field. */
#define IPC_RECEIVE_CNF_CHEN5_Disable (0UL) /*!< Disable events. */
#define IPC_RECEIVE_CNF_CHEN5_Enable (1UL) /*!< Enable events. */

/* Bit 4 : Enable subscription to channel 4. */
#define IPC_RECEIVE_CNF_CHEN4_Pos (4UL) /*!< Position of CHEN4 field. */
#define IPC_RECEIVE_CNF_CHEN4_Msk (0x1UL << IPC_RECEIVE_CNF_CHEN4_Pos) /*!< Bit mask of CHEN4 field. */
#define IPC_RECEIVE_CNF_CHEN4_Disable (0UL) /*!< Disable events. */
#define IPC_RECEIVE_CNF_CHEN4_Enable (1UL) /*!< Enable events. */

/* Bit 3 : Enable subscription to channel 3. */
#define IPC_RECEIVE_CNF_CHEN3_Pos (3UL) /*!< Position of CHEN3 field. */
#define IPC_RECEIVE_CNF_CHEN3_Msk (0x1UL << IPC_RECEIVE_CNF_CHEN3_Pos) /*!< Bit mask of CHEN3 field. */
#define IPC_RECEIVE_CNF_CHEN3_Disable (0UL) /*!< Disable events. */
#define IPC_RECEIVE_CNF_CHEN3_Enable (1UL) /*!< Enable events. */

/* Bit 2 : Enable subscription to channel 2. */
#define IPC_RECEIVE_CNF_CHEN2_Pos (2UL) /*!< Position of CHEN2 field. */
#define IPC_RECEIVE_CNF_CHEN2_Msk (0x1UL << IPC_RECEIVE_CNF_CHEN2_Pos) /*!< Bit mask of CHEN2 field. */
#define IPC_RECEIVE_CNF_CHEN2_Disable (0UL) /*!< Disable events. */
#define IPC_RECEIVE_CNF_CHEN2_Enable (1UL) /*!< Enable events. */

/* Bit 1 : Enable subscription to channel 1. */
#define IPC_RECEIVE_CNF_CHEN1_Pos (1UL) /*!< Position of CHEN1 field. */
#define IPC_RECEIVE_CNF_CHEN1_Msk (0x1UL << IPC_RECEIVE_CNF_CHEN1_Pos) /*!< Bit mask of CHEN1 field. */
#define IPC_RECEIVE_CNF_CHEN1_Disable (0UL) /*!< Disable events. */
#define IPC_RECEIVE_CNF_CHEN1_Enable (1UL) /*!< Enable events. */

/* Bit 0 : Enable subscription to channel 0. */
#define IPC_RECEIVE_CNF_CHEN0_Pos (0UL) /*!< Position of CHEN0 field. */
#define IPC_RECEIVE_CNF_CHEN0_Msk (0x1UL << IPC_RECEIVE_CNF_CHEN0_Pos) /*!< Bit mask of CHEN0 field. */
#define IPC_RECEIVE_CNF_CHEN0_Disable (0UL) /*!< Disable events. */
#define IPC_RECEIVE_CNF_CHEN0_Enable (1UL) /*!< Enable events. */

/* Register: IPC_GPMEM */
/* Description: Description collection: General purpose memory. */

/* Bits 31..0 : General purpose memory */
#define IPC_GPMEM_GPMEM_Pos (0UL) /*!< Position of GPMEM field. */
#define IPC_GPMEM_GPMEM_Msk (0xFFFFFFFFUL << IPC_GPMEM_GPMEM_Pos) /*!< Bit mask of GPMEM field. */


/* Peripheral: KMU */
/* Description: Key management unit 0 */

/* Register: KMU_TASKS_PUSH_KEYSLOT */
/* Description: Push a key slot over secure APB */

/* Bit 0 : Push a key slot over secure APB */
#define KMU_TASKS_PUSH_KEYSLOT_TASKS_PUSH_KEYSLOT_Pos (0UL) /*!< Position of TASKS_PUSH_KEYSLOT field. */
#define KMU_TASKS_PUSH_KEYSLOT_TASKS_PUSH_KEYSLOT_Msk (0x1UL << KMU_TASKS_PUSH_KEYSLOT_TASKS_PUSH_KEYSLOT_Pos) /*!< Bit mask of TASKS_PUSH_KEYSLOT field. */
#define KMU_TASKS_PUSH_KEYSLOT_TASKS_PUSH_KEYSLOT_Trigger (1UL) /*!< Trigger task */

/* Register: KMU_EVENTS_KEYSLOT_PUSHED */
/* Description: Key successfully pushed over secure APB */

/* Bit 0 : Key successfully pushed over secure APB */
#define KMU_EVENTS_KEYSLOT_PUSHED_EVENTS_KEYSLOT_PUSHED_Pos (0UL) /*!< Position of EVENTS_KEYSLOT_PUSHED field. */
#define KMU_EVENTS_KEYSLOT_PUSHED_EVENTS_KEYSLOT_PUSHED_Msk (0x1UL << KMU_EVENTS_KEYSLOT_PUSHED_EVENTS_KEYSLOT_PUSHED_Pos) /*!< Bit mask of EVENTS_KEYSLOT_PUSHED field. */
#define KMU_EVENTS_KEYSLOT_PUSHED_EVENTS_KEYSLOT_PUSHED_NotGenerated (0UL) /*!< Event not generated */
#define KMU_EVENTS_KEYSLOT_PUSHED_EVENTS_KEYSLOT_PUSHED_Generated (1UL) /*!< Event generated */

/* Register: KMU_EVENTS_KEYSLOT_REVOKED */
/* Description: Key has been revoked and cannot be tasked for selection */

/* Bit 0 : Key has been revoked and cannot be tasked for selection */
#define KMU_EVENTS_KEYSLOT_REVOKED_EVENTS_KEYSLOT_REVOKED_Pos (0UL) /*!< Position of EVENTS_KEYSLOT_REVOKED field. */
#define KMU_EVENTS_KEYSLOT_REVOKED_EVENTS_KEYSLOT_REVOKED_Msk (0x1UL << KMU_EVENTS_KEYSLOT_REVOKED_EVENTS_KEYSLOT_REVOKED_Pos) /*!< Bit mask of EVENTS_KEYSLOT_REVOKED field. */
#define KMU_EVENTS_KEYSLOT_REVOKED_EVENTS_KEYSLOT_REVOKED_NotGenerated (0UL) /*!< Event not generated */
#define KMU_EVENTS_KEYSLOT_REVOKED_EVENTS_KEYSLOT_REVOKED_Generated (1UL) /*!< Event generated */

/* Register: KMU_EVENTS_KEYSLOT_ERROR */
/* Description: No key slot selected, no destination address defined, or error during push operation */

/* Bit 0 : No key slot selected, no destination address defined, or error during push operation */
#define KMU_EVENTS_KEYSLOT_ERROR_EVENTS_KEYSLOT_ERROR_Pos (0UL) /*!< Position of EVENTS_KEYSLOT_ERROR field. */
#define KMU_EVENTS_KEYSLOT_ERROR_EVENTS_KEYSLOT_ERROR_Msk (0x1UL << KMU_EVENTS_KEYSLOT_ERROR_EVENTS_KEYSLOT_ERROR_Pos) /*!< Bit mask of EVENTS_KEYSLOT_ERROR field. */
#define KMU_EVENTS_KEYSLOT_ERROR_EVENTS_KEYSLOT_ERROR_NotGenerated (0UL) /*!< Event not generated */
#define KMU_EVENTS_KEYSLOT_ERROR_EVENTS_KEYSLOT_ERROR_Generated (1UL) /*!< Event generated */

/* Register: KMU_INTEN */
/* Description: Enable or disable interrupt */

/* Bit 2 : Enable or disable interrupt for event KEYSLOT_ERROR */
#define KMU_INTEN_KEYSLOT_ERROR_Pos (2UL) /*!< Position of KEYSLOT_ERROR field. */
#define KMU_INTEN_KEYSLOT_ERROR_Msk (0x1UL << KMU_INTEN_KEYSLOT_ERROR_Pos) /*!< Bit mask of KEYSLOT_ERROR field. */
#define KMU_INTEN_KEYSLOT_ERROR_Disabled (0UL) /*!< Disable */
#define KMU_INTEN_KEYSLOT_ERROR_Enabled (1UL) /*!< Enable */

/* Bit 1 : Enable or disable interrupt for event KEYSLOT_REVOKED */
#define KMU_INTEN_KEYSLOT_REVOKED_Pos (1UL) /*!< Position of KEYSLOT_REVOKED field. */
#define KMU_INTEN_KEYSLOT_REVOKED_Msk (0x1UL << KMU_INTEN_KEYSLOT_REVOKED_Pos) /*!< Bit mask of KEYSLOT_REVOKED field. */
#define KMU_INTEN_KEYSLOT_REVOKED_Disabled (0UL) /*!< Disable */
#define KMU_INTEN_KEYSLOT_REVOKED_Enabled (1UL) /*!< Enable */

/* Bit 0 : Enable or disable interrupt for event KEYSLOT_PUSHED */
#define KMU_INTEN_KEYSLOT_PUSHED_Pos (0UL) /*!< Position of KEYSLOT_PUSHED field. */
#define KMU_INTEN_KEYSLOT_PUSHED_Msk (0x1UL << KMU_INTEN_KEYSLOT_PUSHED_Pos) /*!< Bit mask of KEYSLOT_PUSHED field. */
#define KMU_INTEN_KEYSLOT_PUSHED_Disabled (0UL) /*!< Disable */
#define KMU_INTEN_KEYSLOT_PUSHED_Enabled (1UL) /*!< Enable */

/* Register: KMU_INTENSET */
/* Description: Enable interrupt */

/* Bit 2 : Write '1' to enable interrupt for event KEYSLOT_ERROR */
#define KMU_INTENSET_KEYSLOT_ERROR_Pos (2UL) /*!< Position of KEYSLOT_ERROR field. */
#define KMU_INTENSET_KEYSLOT_ERROR_Msk (0x1UL << KMU_INTENSET_KEYSLOT_ERROR_Pos) /*!< Bit mask of KEYSLOT_ERROR field. */
#define KMU_INTENSET_KEYSLOT_ERROR_Disabled (0UL) /*!< Read: Disabled */
#define KMU_INTENSET_KEYSLOT_ERROR_Enabled (1UL) /*!< Read: Enabled */
#define KMU_INTENSET_KEYSLOT_ERROR_Set (1UL) /*!< Enable */

/* Bit 1 : Write '1' to enable interrupt for event KEYSLOT_REVOKED */
#define KMU_INTENSET_KEYSLOT_REVOKED_Pos (1UL) /*!< Position of KEYSLOT_REVOKED field. */
#define KMU_INTENSET_KEYSLOT_REVOKED_Msk (0x1UL << KMU_INTENSET_KEYSLOT_REVOKED_Pos) /*!< Bit mask of KEYSLOT_REVOKED field. */
#define KMU_INTENSET_KEYSLOT_REVOKED_Disabled (0UL) /*!< Read: Disabled */
#define KMU_INTENSET_KEYSLOT_REVOKED_Enabled (1UL) /*!< Read: Enabled */
#define KMU_INTENSET_KEYSLOT_REVOKED_Set (1UL) /*!< Enable */

/* Bit 0 : Write '1' to enable interrupt for event KEYSLOT_PUSHED */
#define KMU_INTENSET_KEYSLOT_PUSHED_Pos (0UL) /*!< Position of KEYSLOT_PUSHED field. */
#define KMU_INTENSET_KEYSLOT_PUSHED_Msk (0x1UL << KMU_INTENSET_KEYSLOT_PUSHED_Pos) /*!< Bit mask of KEYSLOT_PUSHED field. */
#define KMU_INTENSET_KEYSLOT_PUSHED_Disabled (0UL) /*!< Read: Disabled */
#define KMU_INTENSET_KEYSLOT_PUSHED_Enabled (1UL) /*!< Read: Enabled */
#define KMU_INTENSET_KEYSLOT_PUSHED_Set (1UL) /*!< Enable */

/* Register: KMU_INTENCLR */
/* Description: Disable interrupt */

/* Bit 2 : Write '1' to disable interrupt for event KEYSLOT_ERROR */
#define KMU_INTENCLR_KEYSLOT_ERROR_Pos (2UL) /*!< Position of KEYSLOT_ERROR field. */
#define KMU_INTENCLR_KEYSLOT_ERROR_Msk (0x1UL << KMU_INTENCLR_KEYSLOT_ERROR_Pos) /*!< Bit mask of KEYSLOT_ERROR field. */
#define KMU_INTENCLR_KEYSLOT_ERROR_Disabled (0UL) /*!< Read: Disabled */
#define KMU_INTENCLR_KEYSLOT_ERROR_Enabled (1UL) /*!< Read: Enabled */
#define KMU_INTENCLR_KEYSLOT_ERROR_Clear (1UL) /*!< Disable */

/* Bit 1 : Write '1' to disable interrupt for event KEYSLOT_REVOKED */
#define KMU_INTENCLR_KEYSLOT_REVOKED_Pos (1UL) /*!< Position of KEYSLOT_REVOKED field. */
#define KMU_INTENCLR_KEYSLOT_REVOKED_Msk (0x1UL << KMU_INTENCLR_KEYSLOT_REVOKED_Pos) /*!< Bit mask of KEYSLOT_REVOKED field. */
#define KMU_INTENCLR_KEYSLOT_REVOKED_Disabled (0UL) /*!< Read: Disabled */
#define KMU_INTENCLR_KEYSLOT_REVOKED_Enabled (1UL) /*!< Read: Enabled */
#define KMU_INTENCLR_KEYSLOT_REVOKED_Clear (1UL) /*!< Disable */

/* Bit 0 : Write '1' to disable interrupt for event KEYSLOT_PUSHED */
#define KMU_INTENCLR_KEYSLOT_PUSHED_Pos (0UL) /*!< Position of KEYSLOT_PUSHED field. */
#define KMU_INTENCLR_KEYSLOT_PUSHED_Msk (0x1UL << KMU_INTENCLR_KEYSLOT_PUSHED_Pos) /*!< Bit mask of KEYSLOT_PUSHED field. */
#define KMU_INTENCLR_KEYSLOT_PUSHED_Disabled (0UL) /*!< Read: Disabled */
#define KMU_INTENCLR_KEYSLOT_PUSHED_Enabled (1UL) /*!< Read: Enabled */
#define KMU_INTENCLR_KEYSLOT_PUSHED_Clear (1UL) /*!< Disable */

/* Register: KMU_INTPEND */
/* Description: Pending interrupts */

/* Bit 2 : Read pending status of interrupt for event KEYSLOT_ERROR */
#define KMU_INTPEND_KEYSLOT_ERROR_Pos (2UL) /*!< Position of KEYSLOT_ERROR field. */
#define KMU_INTPEND_KEYSLOT_ERROR_Msk (0x1UL << KMU_INTPEND_KEYSLOT_ERROR_Pos) /*!< Bit mask of KEYSLOT_ERROR field. */
#define KMU_INTPEND_KEYSLOT_ERROR_NotPending (0UL) /*!< Read: Not pending */
#define KMU_INTPEND_KEYSLOT_ERROR_Pending (1UL) /*!< Read: Pending */

/* Bit 1 : Read pending status of interrupt for event KEYSLOT_REVOKED */
#define KMU_INTPEND_KEYSLOT_REVOKED_Pos (1UL) /*!< Position of KEYSLOT_REVOKED field. */
#define KMU_INTPEND_KEYSLOT_REVOKED_Msk (0x1UL << KMU_INTPEND_KEYSLOT_REVOKED_Pos) /*!< Bit mask of KEYSLOT_REVOKED field. */
#define KMU_INTPEND_KEYSLOT_REVOKED_NotPending (0UL) /*!< Read: Not pending */
#define KMU_INTPEND_KEYSLOT_REVOKED_Pending (1UL) /*!< Read: Pending */

/* Bit 0 : Read pending status of interrupt for event KEYSLOT_PUSHED */
#define KMU_INTPEND_KEYSLOT_PUSHED_Pos (0UL) /*!< Position of KEYSLOT_PUSHED field. */
#define KMU_INTPEND_KEYSLOT_PUSHED_Msk (0x1UL << KMU_INTPEND_KEYSLOT_PUSHED_Pos) /*!< Bit mask of KEYSLOT_PUSHED field. */
#define KMU_INTPEND_KEYSLOT_PUSHED_NotPending (0UL) /*!< Read: Not pending */
#define KMU_INTPEND_KEYSLOT_PUSHED_Pending (1UL) /*!< Read: Pending */

/* Register: KMU_STATUS */
/* Description: Status bits for KMU operation */

/* Bit 1 : Violation status */
#define KMU_STATUS_BLOCKED_Pos (1UL) /*!< Position of BLOCKED field. */
#define KMU_STATUS_BLOCKED_Msk (0x1UL << KMU_STATUS_BLOCKED_Pos) /*!< Bit mask of BLOCKED field. */
#define KMU_STATUS_BLOCKED_Disabled (0UL) /*!< No access violation detected */
#define KMU_STATUS_BLOCKED_Enabled (1UL) /*!< Access violation detected and blocked */

/* Bit 0 : Key slot ID successfully selected by the KMU */
#define KMU_STATUS_SELECTED_Pos (0UL) /*!< Position of SELECTED field. */
#define KMU_STATUS_SELECTED_Msk (0x1UL << KMU_STATUS_SELECTED_Pos) /*!< Bit mask of SELECTED field. */
#define KMU_STATUS_SELECTED_Disabled (0UL) /*!< No key slot ID selected by KMU */
#define KMU_STATUS_SELECTED_Enabled (1UL) /*!< Key slot ID successfully selected by KMU */

/* Register: KMU_SELECTKEYSLOT */
/* Description: Select key slot ID to be read over AHB or pushed over secure APB when TASKS_PUSH_KEYSLOT is started */

/* Bits 7..0 : Select key slot ID to be read over AHB, or pushed over secure APB, when TASKS_PUSH_KEYSLOT is started NOTE: ID=0 is not a valid key ID. The 0 ID should be used when the KMU is idle or not in use NOTE: Note that index N in UICR-&gt;KEYSLOT.KEY[N] and UICR-&gt;KEYSLOT.CONFIG[N] corresponds to KMU keyslot ID=N+1 */
#define KMU_SELECTKEYSLOT_ID_Pos (0UL) /*!< Position of ID field. */
#define KMU_SELECTKEYSLOT_ID_Msk (0xFFUL << KMU_SELECTKEYSLOT_ID_Pos) /*!< Bit mask of ID field. */


/* Peripheral: NVMC */
/* Description: Non-volatile memory controller 0 */

/* Register: NVMC_READY */
/* Description: Ready flag */

/* Bit 0 : NVMC is ready or busy */
#define NVMC_READY_READY_Pos (0UL) /*!< Position of READY field. */
#define NVMC_READY_READY_Msk (0x1UL << NVMC_READY_READY_Pos) /*!< Bit mask of READY field. */
#define NVMC_READY_READY_Busy (0UL) /*!< NVMC is busy (on-going write or erase operation) */
#define NVMC_READY_READY_Ready (1UL) /*!< NVMC is ready */

/* Register: NVMC_READYNEXT */
/* Description: Ready flag */

/* Bit 0 : NVMC can accept a new write operation */
#define NVMC_READYNEXT_READYNEXT_Pos (0UL) /*!< Position of READYNEXT field. */
#define NVMC_READYNEXT_READYNEXT_Msk (0x1UL << NVMC_READYNEXT_READYNEXT_Pos) /*!< Bit mask of READYNEXT field. */
#define NVMC_READYNEXT_READYNEXT_Busy (0UL) /*!< NVMC cannot accept any write operation */
#define NVMC_READYNEXT_READYNEXT_Ready (1UL) /*!< NVMC is ready */

/* Register: NVMC_CONFIG */
/* Description: Configuration register */

/* Bits 2..0 : Program memory access mode. It is strongly recommended to only activate erase and write modes when they are actively used. Enabling write or erase will invalidate the cache and keep it invalidated. */
#define NVMC_CONFIG_WEN_Pos (0UL) /*!< Position of WEN field. */
#define NVMC_CONFIG_WEN_Msk (0x7UL << NVMC_CONFIG_WEN_Pos) /*!< Bit mask of WEN field. */
#define NVMC_CONFIG_WEN_Ren (0UL) /*!< Read only access */
#define NVMC_CONFIG_WEN_Wen (1UL) /*!< Write enabled */
#define NVMC_CONFIG_WEN_Een (2UL) /*!< Erase enabled */
#define NVMC_CONFIG_WEN_PEen (4UL) /*!< Partial erase enabled */

/* Register: NVMC_ERASEALL */
/* Description: Register for erasing all non-volatile user memory */

/* Bit 0 : Erase all non-volatile memory including UICR registers. Note that erasing must be enabled by setting CONFIG.WEN = Een before the non-volatile memory can be erased. */
#define NVMC_ERASEALL_ERASEALL_Pos (0UL) /*!< Position of ERASEALL field. */
#define NVMC_ERASEALL_ERASEALL_Msk (0x1UL << NVMC_ERASEALL_ERASEALL_Pos) /*!< Bit mask of ERASEALL field. */
#define NVMC_ERASEALL_ERASEALL_NoOperation (0UL) /*!< No operation */
#define NVMC_ERASEALL_ERASEALL_Erase (1UL) /*!< Start chip erase */

/* Register: NVMC_ERASEPAGEPARTIALCFG */
/* Description: Register for partial erase configuration */

/* Bits 6..0 : Duration of the partial erase in milliseconds */
#define NVMC_ERASEPAGEPARTIALCFG_DURATION_Pos (0UL) /*!< Position of DURATION field. */
#define NVMC_ERASEPAGEPARTIALCFG_DURATION_Msk (0x7FUL << NVMC_ERASEPAGEPARTIALCFG_DURATION_Pos) /*!< Bit mask of DURATION field. */

/* Register: NVMC_ICACHECNF */
/* Description: I-code cache configuration register */

/* Bit 8 : Cache profiling enable */
#define NVMC_ICACHECNF_CACHEPROFEN_Pos (8UL) /*!< Position of CACHEPROFEN field. */
#define NVMC_ICACHECNF_CACHEPROFEN_Msk (0x1UL << NVMC_ICACHECNF_CACHEPROFEN_Pos) /*!< Bit mask of CACHEPROFEN field. */
#define NVMC_ICACHECNF_CACHEPROFEN_Disabled (0UL) /*!< Disable cache profiling */
#define NVMC_ICACHECNF_CACHEPROFEN_Enabled (1UL) /*!< Enable cache profiling */

/* Bit 0 : Cache enable */
#define NVMC_ICACHECNF_CACHEEN_Pos (0UL) /*!< Position of CACHEEN field. */
#define NVMC_ICACHECNF_CACHEEN_Msk (0x1UL << NVMC_ICACHECNF_CACHEEN_Pos) /*!< Bit mask of CACHEEN field. */
#define NVMC_ICACHECNF_CACHEEN_Disabled (0UL) /*!< Disable cache. Invalidates all cache entries. */
#define NVMC_ICACHECNF_CACHEEN_Enabled (1UL) /*!< Enable cache */

/* Register: NVMC_IHIT */
/* Description: I-code cache hit counter */

/* Bits 31..0 : Number of cache hits */
#define NVMC_IHIT_HITS_Pos (0UL) /*!< Position of HITS field. */
#define NVMC_IHIT_HITS_Msk (0xFFFFFFFFUL << NVMC_IHIT_HITS_Pos) /*!< Bit mask of HITS field. */

/* Register: NVMC_IMISS */
/* Description: I-code cache miss counter */

/* Bits 31..0 : Number of cache misses */
#define NVMC_IMISS_MISSES_Pos (0UL) /*!< Position of MISSES field. */
#define NVMC_IMISS_MISSES_Msk (0xFFFFFFFFUL << NVMC_IMISS_MISSES_Pos) /*!< Bit mask of MISSES field. */

/* Register: NVMC_CONFIGNS */
/* Description: Unspecified */

/* Bits 1..0 : Program memory access mode. It is strongly recommended to only activate erase and write modes when they are actively used. Enabling write or erase will invalidate the cache and keep it invalidated. */
#define NVMC_CONFIGNS_WEN_Pos (0UL) /*!< Position of WEN field. */
#define NVMC_CONFIGNS_WEN_Msk (0x3UL << NVMC_CONFIGNS_WEN_Pos) /*!< Bit mask of WEN field. */
#define NVMC_CONFIGNS_WEN_Ren (0UL) /*!< Read only access */
#define NVMC_CONFIGNS_WEN_Wen (1UL) /*!< Write enabled */
#define NVMC_CONFIGNS_WEN_Een (2UL) /*!< Erase enabled */

/* Register: NVMC_WRITEUICRNS */
/* Description: Non-secure APPROTECT enable register */

/* Bits 31..4 : Key to write in order to validate the write operation */
#define NVMC_WRITEUICRNS_KEY_Pos (4UL) /*!< Position of KEY field. */
#define NVMC_WRITEUICRNS_KEY_Msk (0xFFFFFFFUL << NVMC_WRITEUICRNS_KEY_Pos) /*!< Bit mask of KEY field. */
#define NVMC_WRITEUICRNS_KEY_Keyvalid (0xAFBE5A7UL) /*!< Key value */

/* Bit 0 : Allow non-secure code to set APPROTECT */
#define NVMC_WRITEUICRNS_SET_Pos (0UL) /*!< Position of SET field. */
#define NVMC_WRITEUICRNS_SET_Msk (0x1UL << NVMC_WRITEUICRNS_SET_Pos) /*!< Bit mask of SET field. */
#define NVMC_WRITEUICRNS_SET_Set (1UL) /*!< Set value */

/* Register: NVMC_FORCEONNVM */
/* Description: Force on all NVM supplies. Also see the internal section in the NVMC chapter. */

/* Bit 0 : Force on all NVM supplies. Also see the internal section in the NVMC chapter. */
#define NVMC_FORCEONNVM_FORCEONNVM_Pos (0UL) /*!< Position of FORCEONNVM field. */
#define NVMC_FORCEONNVM_FORCEONNVM_Msk (0x1UL << NVMC_FORCEONNVM_FORCEONNVM_Pos) /*!< Bit mask of FORCEONNVM field. */
#define NVMC_FORCEONNVM_FORCEONNVM_DoNotForceOn (0UL) /*!< Do not force on NVM supply */
#define NVMC_FORCEONNVM_FORCEONNVM_ForceOn (1UL) /*!< Force on NVM supply */

/* Register: NVMC_FORCEOFFNVM */
/* Description: Force off NVM supply. Also see the internal section in the NVMC chapter. */

/* Bits 31..8 : KEY */
#define NVMC_FORCEOFFNVM_KEY_Pos (8UL) /*!< Position of KEY field. */
#define NVMC_FORCEOFFNVM_KEY_Msk (0xFFFFFFUL << NVMC_FORCEOFFNVM_KEY_Pos) /*!< Bit mask of KEY field. */
#define NVMC_FORCEOFFNVM_KEY_EnableWrite (0xACCE55UL) /*!< Must be written in order to write to bits 0-7. Any other value will ignore writes to this register. Read as zero. */

/* Bit 1 : Force off NVM supply 1. Also see the internal section in the NVMC chapter. */
#define NVMC_FORCEOFFNVM_FORCEOFFNVM1_Pos (1UL) /*!< Position of FORCEOFFNVM1 field. */
#define NVMC_FORCEOFFNVM_FORCEOFFNVM1_Msk (0x1UL << NVMC_FORCEOFFNVM_FORCEOFFNVM1_Pos) /*!< Bit mask of FORCEOFFNVM1 field. */
#define NVMC_FORCEOFFNVM_FORCEOFFNVM1_DoNotForceOff (0UL) /*!< Do not force off supply */
#define NVMC_FORCEOFFNVM_FORCEOFFNVM1_ForceOff (1UL) /*!< Force off supply */

/* Bit 0 : Force off NVM supply 0. Also see the internal section in the NVMC chapter. */
#define NVMC_FORCEOFFNVM_FORCEOFFNVM0_Pos (0UL) /*!< Position of FORCEOFFNVM0 field. */
#define NVMC_FORCEOFFNVM_FORCEOFFNVM0_Msk (0x1UL << NVMC_FORCEOFFNVM_FORCEOFFNVM0_Pos) /*!< Bit mask of FORCEOFFNVM0 field. */
#define NVMC_FORCEOFFNVM_FORCEOFFNVM0_DoNotForceOff (0UL) /*!< Do not force off supply */
#define NVMC_FORCEOFFNVM_FORCEOFFNVM0_ForceOff (1UL) /*!< Force off supply */


/* Peripheral: GPIO */
/* Description: GPIO Port 0 */

/* Register: GPIO_OUT */
/* Description: Write GPIO port */

/* Bit 31 : Pin 31 */
#define GPIO_OUT_PIN31_Pos (31UL) /*!< Position of PIN31 field. */
#define GPIO_OUT_PIN31_Msk (0x1UL << GPIO_OUT_PIN31_Pos) /*!< Bit mask of PIN31 field. */
#define GPIO_OUT_PIN31_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN31_High (1UL) /*!< Pin driver is high */

/* Bit 30 : Pin 30 */
#define GPIO_OUT_PIN30_Pos (30UL) /*!< Position of PIN30 field. */
#define GPIO_OUT_PIN30_Msk (0x1UL << GPIO_OUT_PIN30_Pos) /*!< Bit mask of PIN30 field. */
#define GPIO_OUT_PIN30_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN30_High (1UL) /*!< Pin driver is high */

/* Bit 29 : Pin 29 */
#define GPIO_OUT_PIN29_Pos (29UL) /*!< Position of PIN29 field. */
#define GPIO_OUT_PIN29_Msk (0x1UL << GPIO_OUT_PIN29_Pos) /*!< Bit mask of PIN29 field. */
#define GPIO_OUT_PIN29_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN29_High (1UL) /*!< Pin driver is high */

/* Bit 28 : Pin 28 */
#define GPIO_OUT_PIN28_Pos (28UL) /*!< Position of PIN28 field. */
#define GPIO_OUT_PIN28_Msk (0x1UL << GPIO_OUT_PIN28_Pos) /*!< Bit mask of PIN28 field. */
#define GPIO_OUT_PIN28_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN28_High (1UL) /*!< Pin driver is high */

/* Bit 27 : Pin 27 */
#define GPIO_OUT_PIN27_Pos (27UL) /*!< Position of PIN27 field. */
#define GPIO_OUT_PIN27_Msk (0x1UL << GPIO_OUT_PIN27_Pos) /*!< Bit mask of PIN27 field. */
#define GPIO_OUT_PIN27_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN27_High (1UL) /*!< Pin driver is high */

/* Bit 26 : Pin 26 */
#define GPIO_OUT_PIN26_Pos (26UL) /*!< Position of PIN26 field. */
#define GPIO_OUT_PIN26_Msk (0x1UL << GPIO_OUT_PIN26_Pos) /*!< Bit mask of PIN26 field. */
#define GPIO_OUT_PIN26_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN26_High (1UL) /*!< Pin driver is high */

/* Bit 25 : Pin 25 */
#define GPIO_OUT_PIN25_Pos (25UL) /*!< Position of PIN25 field. */
#define GPIO_OUT_PIN25_Msk (0x1UL << GPIO_OUT_PIN25_Pos) /*!< Bit mask of PIN25 field. */
#define GPIO_OUT_PIN25_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN25_High (1UL) /*!< Pin driver is high */

/* Bit 24 : Pin 24 */
#define GPIO_OUT_PIN24_Pos (24UL) /*!< Position of PIN24 field. */
#define GPIO_OUT_PIN24_Msk (0x1UL << GPIO_OUT_PIN24_Pos) /*!< Bit mask of PIN24 field. */
#define GPIO_OUT_PIN24_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN24_High (1UL) /*!< Pin driver is high */

/* Bit 23 : Pin 23 */
#define GPIO_OUT_PIN23_Pos (23UL) /*!< Position of PIN23 field. */
#define GPIO_OUT_PIN23_Msk (0x1UL << GPIO_OUT_PIN23_Pos) /*!< Bit mask of PIN23 field. */
#define GPIO_OUT_PIN23_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN23_High (1UL) /*!< Pin driver is high */

/* Bit 22 : Pin 22 */
#define GPIO_OUT_PIN22_Pos (22UL) /*!< Position of PIN22 field. */
#define GPIO_OUT_PIN22_Msk (0x1UL << GPIO_OUT_PIN22_Pos) /*!< Bit mask of PIN22 field. */
#define GPIO_OUT_PIN22_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN22_High (1UL) /*!< Pin driver is high */

/* Bit 21 : Pin 21 */
#define GPIO_OUT_PIN21_Pos (21UL) /*!< Position of PIN21 field. */
#define GPIO_OUT_PIN21_Msk (0x1UL << GPIO_OUT_PIN21_Pos) /*!< Bit mask of PIN21 field. */
#define GPIO_OUT_PIN21_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN21_High (1UL) /*!< Pin driver is high */

/* Bit 20 : Pin 20 */
#define GPIO_OUT_PIN20_Pos (20UL) /*!< Position of PIN20 field. */
#define GPIO_OUT_PIN20_Msk (0x1UL << GPIO_OUT_PIN20_Pos) /*!< Bit mask of PIN20 field. */
#define GPIO_OUT_PIN20_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN20_High (1UL) /*!< Pin driver is high */

/* Bit 19 : Pin 19 */
#define GPIO_OUT_PIN19_Pos (19UL) /*!< Position of PIN19 field. */
#define GPIO_OUT_PIN19_Msk (0x1UL << GPIO_OUT_PIN19_Pos) /*!< Bit mask of PIN19 field. */
#define GPIO_OUT_PIN19_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN19_High (1UL) /*!< Pin driver is high */

/* Bit 18 : Pin 18 */
#define GPIO_OUT_PIN18_Pos (18UL) /*!< Position of PIN18 field. */
#define GPIO_OUT_PIN18_Msk (0x1UL << GPIO_OUT_PIN18_Pos) /*!< Bit mask of PIN18 field. */
#define GPIO_OUT_PIN18_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN18_High (1UL) /*!< Pin driver is high */

/* Bit 17 : Pin 17 */
#define GPIO_OUT_PIN17_Pos (17UL) /*!< Position of PIN17 field. */
#define GPIO_OUT_PIN17_Msk (0x1UL << GPIO_OUT_PIN17_Pos) /*!< Bit mask of PIN17 field. */
#define GPIO_OUT_PIN17_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN17_High (1UL) /*!< Pin driver is high */

/* Bit 16 : Pin 16 */
#define GPIO_OUT_PIN16_Pos (16UL) /*!< Position of PIN16 field. */
#define GPIO_OUT_PIN16_Msk (0x1UL << GPIO_OUT_PIN16_Pos) /*!< Bit mask of PIN16 field. */
#define GPIO_OUT_PIN16_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN16_High (1UL) /*!< Pin driver is high */

/* Bit 15 : Pin 15 */
#define GPIO_OUT_PIN15_Pos (15UL) /*!< Position of PIN15 field. */
#define GPIO_OUT_PIN15_Msk (0x1UL << GPIO_OUT_PIN15_Pos) /*!< Bit mask of PIN15 field. */
#define GPIO_OUT_PIN15_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN15_High (1UL) /*!< Pin driver is high */

/* Bit 14 : Pin 14 */
#define GPIO_OUT_PIN14_Pos (14UL) /*!< Position of PIN14 field. */
#define GPIO_OUT_PIN14_Msk (0x1UL << GPIO_OUT_PIN14_Pos) /*!< Bit mask of PIN14 field. */
#define GPIO_OUT_PIN14_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN14_High (1UL) /*!< Pin driver is high */

/* Bit 13 : Pin 13 */
#define GPIO_OUT_PIN13_Pos (13UL) /*!< Position of PIN13 field. */
#define GPIO_OUT_PIN13_Msk (0x1UL << GPIO_OUT_PIN13_Pos) /*!< Bit mask of PIN13 field. */
#define GPIO_OUT_PIN13_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN13_High (1UL) /*!< Pin driver is high */

/* Bit 12 : Pin 12 */
#define GPIO_OUT_PIN12_Pos (12UL) /*!< Position of PIN12 field. */
#define GPIO_OUT_PIN12_Msk (0x1UL << GPIO_OUT_PIN12_Pos) /*!< Bit mask of PIN12 field. */
#define GPIO_OUT_PIN12_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN12_High (1UL) /*!< Pin driver is high */

/* Bit 11 : Pin 11 */
#define GPIO_OUT_PIN11_Pos (11UL) /*!< Position of PIN11 field. */
#define GPIO_OUT_PIN11_Msk (0x1UL << GPIO_OUT_PIN11_Pos) /*!< Bit mask of PIN11 field. */
#define GPIO_OUT_PIN11_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN11_High (1UL) /*!< Pin driver is high */

/* Bit 10 : Pin 10 */
#define GPIO_OUT_PIN10_Pos (10UL) /*!< Position of PIN10 field. */
#define GPIO_OUT_PIN10_Msk (0x1UL << GPIO_OUT_PIN10_Pos) /*!< Bit mask of PIN10 field. */
#define GPIO_OUT_PIN10_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN10_High (1UL) /*!< Pin driver is high */

/* Bit 9 : Pin 9 */
#define GPIO_OUT_PIN9_Pos (9UL) /*!< Position of PIN9 field. */
#define GPIO_OUT_PIN9_Msk (0x1UL << GPIO_OUT_PIN9_Pos) /*!< Bit mask of PIN9 field. */
#define GPIO_OUT_PIN9_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN9_High (1UL) /*!< Pin driver is high */

/* Bit 8 : Pin 8 */
#define GPIO_OUT_PIN8_Pos (8UL) /*!< Position of PIN8 field. */
#define GPIO_OUT_PIN8_Msk (0x1UL << GPIO_OUT_PIN8_Pos) /*!< Bit mask of PIN8 field. */
#define GPIO_OUT_PIN8_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN8_High (1UL) /*!< Pin driver is high */

/* Bit 7 : Pin 7 */
#define GPIO_OUT_PIN7_Pos (7UL) /*!< Position of PIN7 field. */
#define GPIO_OUT_PIN7_Msk (0x1UL << GPIO_OUT_PIN7_Pos) /*!< Bit mask of PIN7 field. */
#define GPIO_OUT_PIN7_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN7_High (1UL) /*!< Pin driver is high */

/* Bit 6 : Pin 6 */
#define GPIO_OUT_PIN6_Pos (6UL) /*!< Position of PIN6 field. */
#define GPIO_OUT_PIN6_Msk (0x1UL << GPIO_OUT_PIN6_Pos) /*!< Bit mask of PIN6 field. */
#define GPIO_OUT_PIN6_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN6_High (1UL) /*!< Pin driver is high */

/* Bit 5 : Pin 5 */
#define GPIO_OUT_PIN5_Pos (5UL) /*!< Position of PIN5 field. */
#define GPIO_OUT_PIN5_Msk (0x1UL << GPIO_OUT_PIN5_Pos) /*!< Bit mask of PIN5 field. */
#define GPIO_OUT_PIN5_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN5_High (1UL) /*!< Pin driver is high */

/* Bit 4 : Pin 4 */
#define GPIO_OUT_PIN4_Pos (4UL) /*!< Position of PIN4 field. */
#define GPIO_OUT_PIN4_Msk (0x1UL << GPIO_OUT_PIN4_Pos) /*!< Bit mask of PIN4 field. */
#define GPIO_OUT_PIN4_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN4_High (1UL) /*!< Pin driver is high */

/* Bit 3 : Pin 3 */
#define GPIO_OUT_PIN3_Pos (3UL) /*!< Position of PIN3 field. */
#define GPIO_OUT_PIN3_Msk (0x1UL << GPIO_OUT_PIN3_Pos) /*!< Bit mask of PIN3 field. */
#define GPIO_OUT_PIN3_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN3_High (1UL) /*!< Pin driver is high */

/* Bit 2 : Pin 2 */
#define GPIO_OUT_PIN2_Pos (2UL) /*!< Position of PIN2 field. */
#define GPIO_OUT_PIN2_Msk (0x1UL << GPIO_OUT_PIN2_Pos) /*!< Bit mask of PIN2 field. */
#define GPIO_OUT_PIN2_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN2_High (1UL) /*!< Pin driver is high */

/* Bit 1 : Pin 1 */
#define GPIO_OUT_PIN1_Pos (1UL) /*!< Position of PIN1 field. */
#define GPIO_OUT_PIN1_Msk (0x1UL << GPIO_OUT_PIN1_Pos) /*!< Bit mask of PIN1 field. */
#define GPIO_OUT_PIN1_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN1_High (1UL) /*!< Pin driver is high */

/* Bit 0 : Pin 0 */
#define GPIO_OUT_PIN0_Pos (0UL) /*!< Position of PIN0 field. */
#define GPIO_OUT_PIN0_Msk (0x1UL << GPIO_OUT_PIN0_Pos) /*!< Bit mask of PIN0 field. */
#define GPIO_OUT_PIN0_Low (0UL) /*!< Pin driver is low */
#define GPIO_OUT_PIN0_High (1UL) /*!< Pin driver is high */

/* Register: GPIO_OUTSET */
/* Description: Set individual bits in GPIO port */

/* Bit 31 : Pin 31 */
#define GPIO_OUTSET_PIN31_Pos (31UL) /*!< Position of PIN31 field. */
#define GPIO_OUTSET_PIN31_Msk (0x1UL << GPIO_OUTSET_PIN31_Pos) /*!< Bit mask of PIN31 field. */
#define GPIO_OUTSET_PIN31_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN31_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN31_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 30 : Pin 30 */
#define GPIO_OUTSET_PIN30_Pos (30UL) /*!< Position of PIN30 field. */
#define GPIO_OUTSET_PIN30_Msk (0x1UL << GPIO_OUTSET_PIN30_Pos) /*!< Bit mask of PIN30 field. */
#define GPIO_OUTSET_PIN30_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN30_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN30_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 29 : Pin 29 */
#define GPIO_OUTSET_PIN29_Pos (29UL) /*!< Position of PIN29 field. */
#define GPIO_OUTSET_PIN29_Msk (0x1UL << GPIO_OUTSET_PIN29_Pos) /*!< Bit mask of PIN29 field. */
#define GPIO_OUTSET_PIN29_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN29_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN29_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 28 : Pin 28 */
#define GPIO_OUTSET_PIN28_Pos (28UL) /*!< Position of PIN28 field. */
#define GPIO_OUTSET_PIN28_Msk (0x1UL << GPIO_OUTSET_PIN28_Pos) /*!< Bit mask of PIN28 field. */
#define GPIO_OUTSET_PIN28_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN28_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN28_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 27 : Pin 27 */
#define GPIO_OUTSET_PIN27_Pos (27UL) /*!< Position of PIN27 field. */
#define GPIO_OUTSET_PIN27_Msk (0x1UL << GPIO_OUTSET_PIN27_Pos) /*!< Bit mask of PIN27 field. */
#define GPIO_OUTSET_PIN27_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN27_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN27_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 26 : Pin 26 */
#define GPIO_OUTSET_PIN26_Pos (26UL) /*!< Position of PIN26 field. */
#define GPIO_OUTSET_PIN26_Msk (0x1UL << GPIO_OUTSET_PIN26_Pos) /*!< Bit mask of PIN26 field. */
#define GPIO_OUTSET_PIN26_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN26_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN26_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 25 : Pin 25 */
#define GPIO_OUTSET_PIN25_Pos (25UL) /*!< Position of PIN25 field. */
#define GPIO_OUTSET_PIN25_Msk (0x1UL << GPIO_OUTSET_PIN25_Pos) /*!< Bit mask of PIN25 field. */
#define GPIO_OUTSET_PIN25_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN25_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN25_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 24 : Pin 24 */
#define GPIO_OUTSET_PIN24_Pos (24UL) /*!< Position of PIN24 field. */
#define GPIO_OUTSET_PIN24_Msk (0x1UL << GPIO_OUTSET_PIN24_Pos) /*!< Bit mask of PIN24 field. */
#define GPIO_OUTSET_PIN24_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN24_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN24_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 23 : Pin 23 */
#define GPIO_OUTSET_PIN23_Pos (23UL) /*!< Position of PIN23 field. */
#define GPIO_OUTSET_PIN23_Msk (0x1UL << GPIO_OUTSET_PIN23_Pos) /*!< Bit mask of PIN23 field. */
#define GPIO_OUTSET_PIN23_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN23_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN23_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 22 : Pin 22 */
#define GPIO_OUTSET_PIN22_Pos (22UL) /*!< Position of PIN22 field. */
#define GPIO_OUTSET_PIN22_Msk (0x1UL << GPIO_OUTSET_PIN22_Pos) /*!< Bit mask of PIN22 field. */
#define GPIO_OUTSET_PIN22_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN22_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN22_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 21 : Pin 21 */
#define GPIO_OUTSET_PIN21_Pos (21UL) /*!< Position of PIN21 field. */
#define GPIO_OUTSET_PIN21_Msk (0x1UL << GPIO_OUTSET_PIN21_Pos) /*!< Bit mask of PIN21 field. */
#define GPIO_OUTSET_PIN21_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN21_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN21_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 20 : Pin 20 */
#define GPIO_OUTSET_PIN20_Pos (20UL) /*!< Position of PIN20 field. */
#define GPIO_OUTSET_PIN20_Msk (0x1UL << GPIO_OUTSET_PIN20_Pos) /*!< Bit mask of PIN20 field. */
#define GPIO_OUTSET_PIN20_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN20_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN20_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 19 : Pin 19 */
#define GPIO_OUTSET_PIN19_Pos (19UL) /*!< Position of PIN19 field. */
#define GPIO_OUTSET_PIN19_Msk (0x1UL << GPIO_OUTSET_PIN19_Pos) /*!< Bit mask of PIN19 field. */
#define GPIO_OUTSET_PIN19_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN19_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN19_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 18 : Pin 18 */
#define GPIO_OUTSET_PIN18_Pos (18UL) /*!< Position of PIN18 field. */
#define GPIO_OUTSET_PIN18_Msk (0x1UL << GPIO_OUTSET_PIN18_Pos) /*!< Bit mask of PIN18 field. */
#define GPIO_OUTSET_PIN18_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN18_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN18_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 17 : Pin 17 */
#define GPIO_OUTSET_PIN17_Pos (17UL) /*!< Position of PIN17 field. */
#define GPIO_OUTSET_PIN17_Msk (0x1UL << GPIO_OUTSET_PIN17_Pos) /*!< Bit mask of PIN17 field. */
#define GPIO_OUTSET_PIN17_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN17_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN17_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 16 : Pin 16 */
#define GPIO_OUTSET_PIN16_Pos (16UL) /*!< Position of PIN16 field. */
#define GPIO_OUTSET_PIN16_Msk (0x1UL << GPIO_OUTSET_PIN16_Pos) /*!< Bit mask of PIN16 field. */
#define GPIO_OUTSET_PIN16_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN16_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN16_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 15 : Pin 15 */
#define GPIO_OUTSET_PIN15_Pos (15UL) /*!< Position of PIN15 field. */
#define GPIO_OUTSET_PIN15_Msk (0x1UL << GPIO_OUTSET_PIN15_Pos) /*!< Bit mask of PIN15 field. */
#define GPIO_OUTSET_PIN15_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN15_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN15_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 14 : Pin 14 */
#define GPIO_OUTSET_PIN14_Pos (14UL) /*!< Position of PIN14 field. */
#define GPIO_OUTSET_PIN14_Msk (0x1UL << GPIO_OUTSET_PIN14_Pos) /*!< Bit mask of PIN14 field. */
#define GPIO_OUTSET_PIN14_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN14_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN14_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 13 : Pin 13 */
#define GPIO_OUTSET_PIN13_Pos (13UL) /*!< Position of PIN13 field. */
#define GPIO_OUTSET_PIN13_Msk (0x1UL << GPIO_OUTSET_PIN13_Pos) /*!< Bit mask of PIN13 field. */
#define GPIO_OUTSET_PIN13_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN13_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN13_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 12 : Pin 12 */
#define GPIO_OUTSET_PIN12_Pos (12UL) /*!< Position of PIN12 field. */
#define GPIO_OUTSET_PIN12_Msk (0x1UL << GPIO_OUTSET_PIN12_Pos) /*!< Bit mask of PIN12 field. */
#define GPIO_OUTSET_PIN12_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN12_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN12_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 11 : Pin 11 */
#define GPIO_OUTSET_PIN11_Pos (11UL) /*!< Position of PIN11 field. */
#define GPIO_OUTSET_PIN11_Msk (0x1UL << GPIO_OUTSET_PIN11_Pos) /*!< Bit mask of PIN11 field. */
#define GPIO_OUTSET_PIN11_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN11_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN11_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 10 : Pin 10 */
#define GPIO_OUTSET_PIN10_Pos (10UL) /*!< Position of PIN10 field. */
#define GPIO_OUTSET_PIN10_Msk (0x1UL << GPIO_OUTSET_PIN10_Pos) /*!< Bit mask of PIN10 field. */
#define GPIO_OUTSET_PIN10_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN10_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN10_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 9 : Pin 9 */
#define GPIO_OUTSET_PIN9_Pos (9UL) /*!< Position of PIN9 field. */
#define GPIO_OUTSET_PIN9_Msk (0x1UL << GPIO_OUTSET_PIN9_Pos) /*!< Bit mask of PIN9 field. */
#define GPIO_OUTSET_PIN9_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN9_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN9_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 8 : Pin 8 */
#define GPIO_OUTSET_PIN8_Pos (8UL) /*!< Position of PIN8 field. */
#define GPIO_OUTSET_PIN8_Msk (0x1UL << GPIO_OUTSET_PIN8_Pos) /*!< Bit mask of PIN8 field. */
#define GPIO_OUTSET_PIN8_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN8_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN8_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 7 : Pin 7 */
#define GPIO_OUTSET_PIN7_Pos (7UL) /*!< Position of PIN7 field. */
#define GPIO_OUTSET_PIN7_Msk (0x1UL << GPIO_OUTSET_PIN7_Pos) /*!< Bit mask of PIN7 field. */
#define GPIO_OUTSET_PIN7_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN7_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN7_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 6 : Pin 6 */
#define GPIO_OUTSET_PIN6_Pos (6UL) /*!< Position of PIN6 field. */
#define GPIO_OUTSET_PIN6_Msk (0x1UL << GPIO_OUTSET_PIN6_Pos) /*!< Bit mask of PIN6 field. */
#define GPIO_OUTSET_PIN6_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN6_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN6_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 5 : Pin 5 */
#define GPIO_OUTSET_PIN5_Pos (5UL) /*!< Position of PIN5 field. */
#define GPIO_OUTSET_PIN5_Msk (0x1UL << GPIO_OUTSET_PIN5_Pos) /*!< Bit mask of PIN5 field. */
#define GPIO_OUTSET_PIN5_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN5_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN5_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 4 : Pin 4 */
#define GPIO_OUTSET_PIN4_Pos (4UL) /*!< Position of PIN4 field. */
#define GPIO_OUTSET_PIN4_Msk (0x1UL << GPIO_OUTSET_PIN4_Pos) /*!< Bit mask of PIN4 field. */
#define GPIO_OUTSET_PIN4_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN4_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN4_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 3 : Pin 3 */
#define GPIO_OUTSET_PIN3_Pos (3UL) /*!< Position of PIN3 field. */
#define GPIO_OUTSET_PIN3_Msk (0x1UL << GPIO_OUTSET_PIN3_Pos) /*!< Bit mask of PIN3 field. */
#define GPIO_OUTSET_PIN3_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN3_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN3_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 2 : Pin 2 */
#define GPIO_OUTSET_PIN2_Pos (2UL) /*!< Position of PIN2 field. */
#define GPIO_OUTSET_PIN2_Msk (0x1UL << GPIO_OUTSET_PIN2_Pos) /*!< Bit mask of PIN2 field. */
#define GPIO_OUTSET_PIN2_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN2_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN2_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 1 : Pin 1 */
#define GPIO_OUTSET_PIN1_Pos (1UL) /*!< Position of PIN1 field. */
#define GPIO_OUTSET_PIN1_Msk (0x1UL << GPIO_OUTSET_PIN1_Pos) /*!< Bit mask of PIN1 field. */
#define GPIO_OUTSET_PIN1_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN1_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN1_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Bit 0 : Pin 0 */
#define GPIO_OUTSET_PIN0_Pos (0UL) /*!< Position of PIN0 field. */
#define GPIO_OUTSET_PIN0_Msk (0x1UL << GPIO_OUTSET_PIN0_Pos) /*!< Bit mask of PIN0 field. */
#define GPIO_OUTSET_PIN0_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTSET_PIN0_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTSET_PIN0_Set (1UL) /*!< Write: writing a '1' sets the pin high; writing a '0' has no effect */

/* Register: GPIO_OUTCLR */
/* Description: Clear individual bits in GPIO port */

/* Bit 31 : Pin 31 */
#define GPIO_OUTCLR_PIN31_Pos (31UL) /*!< Position of PIN31 field. */
#define GPIO_OUTCLR_PIN31_Msk (0x1UL << GPIO_OUTCLR_PIN31_Pos) /*!< Bit mask of PIN31 field. */
#define GPIO_OUTCLR_PIN31_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN31_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN31_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 30 : Pin 30 */
#define GPIO_OUTCLR_PIN30_Pos (30UL) /*!< Position of PIN30 field. */
#define GPIO_OUTCLR_PIN30_Msk (0x1UL << GPIO_OUTCLR_PIN30_Pos) /*!< Bit mask of PIN30 field. */
#define GPIO_OUTCLR_PIN30_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN30_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN30_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 29 : Pin 29 */
#define GPIO_OUTCLR_PIN29_Pos (29UL) /*!< Position of PIN29 field. */
#define GPIO_OUTCLR_PIN29_Msk (0x1UL << GPIO_OUTCLR_PIN29_Pos) /*!< Bit mask of PIN29 field. */
#define GPIO_OUTCLR_PIN29_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN29_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN29_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 28 : Pin 28 */
#define GPIO_OUTCLR_PIN28_Pos (28UL) /*!< Position of PIN28 field. */
#define GPIO_OUTCLR_PIN28_Msk (0x1UL << GPIO_OUTCLR_PIN28_Pos) /*!< Bit mask of PIN28 field. */
#define GPIO_OUTCLR_PIN28_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN28_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN28_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 27 : Pin 27 */
#define GPIO_OUTCLR_PIN27_Pos (27UL) /*!< Position of PIN27 field. */
#define GPIO_OUTCLR_PIN27_Msk (0x1UL << GPIO_OUTCLR_PIN27_Pos) /*!< Bit mask of PIN27 field. */
#define GPIO_OUTCLR_PIN27_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN27_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN27_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 26 : Pin 26 */
#define GPIO_OUTCLR_PIN26_Pos (26UL) /*!< Position of PIN26 field. */
#define GPIO_OUTCLR_PIN26_Msk (0x1UL << GPIO_OUTCLR_PIN26_Pos) /*!< Bit mask of PIN26 field. */
#define GPIO_OUTCLR_PIN26_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN26_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN26_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 25 : Pin 25 */
#define GPIO_OUTCLR_PIN25_Pos (25UL) /*!< Position of PIN25 field. */
#define GPIO_OUTCLR_PIN25_Msk (0x1UL << GPIO_OUTCLR_PIN25_Pos) /*!< Bit mask of PIN25 field. */
#define GPIO_OUTCLR_PIN25_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN25_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN25_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 24 : Pin 24 */
#define GPIO_OUTCLR_PIN24_Pos (24UL) /*!< Position of PIN24 field. */
#define GPIO_OUTCLR_PIN24_Msk (0x1UL << GPIO_OUTCLR_PIN24_Pos) /*!< Bit mask of PIN24 field. */
#define GPIO_OUTCLR_PIN24_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN24_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN24_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 23 : Pin 23 */
#define GPIO_OUTCLR_PIN23_Pos (23UL) /*!< Position of PIN23 field. */
#define GPIO_OUTCLR_PIN23_Msk (0x1UL << GPIO_OUTCLR_PIN23_Pos) /*!< Bit mask of PIN23 field. */
#define GPIO_OUTCLR_PIN23_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN23_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN23_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 22 : Pin 22 */
#define GPIO_OUTCLR_PIN22_Pos (22UL) /*!< Position of PIN22 field. */
#define GPIO_OUTCLR_PIN22_Msk (0x1UL << GPIO_OUTCLR_PIN22_Pos) /*!< Bit mask of PIN22 field. */
#define GPIO_OUTCLR_PIN22_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN22_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN22_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 21 : Pin 21 */
#define GPIO_OUTCLR_PIN21_Pos (21UL) /*!< Position of PIN21 field. */
#define GPIO_OUTCLR_PIN21_Msk (0x1UL << GPIO_OUTCLR_PIN21_Pos) /*!< Bit mask of PIN21 field. */
#define GPIO_OUTCLR_PIN21_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN21_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN21_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 20 : Pin 20 */
#define GPIO_OUTCLR_PIN20_Pos (20UL) /*!< Position of PIN20 field. */
#define GPIO_OUTCLR_PIN20_Msk (0x1UL << GPIO_OUTCLR_PIN20_Pos) /*!< Bit mask of PIN20 field. */
#define GPIO_OUTCLR_PIN20_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN20_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN20_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 19 : Pin 19 */
#define GPIO_OUTCLR_PIN19_Pos (19UL) /*!< Position of PIN19 field. */
#define GPIO_OUTCLR_PIN19_Msk (0x1UL << GPIO_OUTCLR_PIN19_Pos) /*!< Bit mask of PIN19 field. */
#define GPIO_OUTCLR_PIN19_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN19_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN19_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 18 : Pin 18 */
#define GPIO_OUTCLR_PIN18_Pos (18UL) /*!< Position of PIN18 field. */
#define GPIO_OUTCLR_PIN18_Msk (0x1UL << GPIO_OUTCLR_PIN18_Pos) /*!< Bit mask of PIN18 field. */
#define GPIO_OUTCLR_PIN18_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN18_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN18_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 17 : Pin 17 */
#define GPIO_OUTCLR_PIN17_Pos (17UL) /*!< Position of PIN17 field. */
#define GPIO_OUTCLR_PIN17_Msk (0x1UL << GPIO_OUTCLR_PIN17_Pos) /*!< Bit mask of PIN17 field. */
#define GPIO_OUTCLR_PIN17_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN17_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN17_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 16 : Pin 16 */
#define GPIO_OUTCLR_PIN16_Pos (16UL) /*!< Position of PIN16 field. */
#define GPIO_OUTCLR_PIN16_Msk (0x1UL << GPIO_OUTCLR_PIN16_Pos) /*!< Bit mask of PIN16 field. */
#define GPIO_OUTCLR_PIN16_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN16_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN16_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 15 : Pin 15 */
#define GPIO_OUTCLR_PIN15_Pos (15UL) /*!< Position of PIN15 field. */
#define GPIO_OUTCLR_PIN15_Msk (0x1UL << GPIO_OUTCLR_PIN15_Pos) /*!< Bit mask of PIN15 field. */
#define GPIO_OUTCLR_PIN15_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN15_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN15_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 14 : Pin 14 */
#define GPIO_OUTCLR_PIN14_Pos (14UL) /*!< Position of PIN14 field. */
#define GPIO_OUTCLR_PIN14_Msk (0x1UL << GPIO_OUTCLR_PIN14_Pos) /*!< Bit mask of PIN14 field. */
#define GPIO_OUTCLR_PIN14_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN14_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN14_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 13 : Pin 13 */
#define GPIO_OUTCLR_PIN13_Pos (13UL) /*!< Position of PIN13 field. */
#define GPIO_OUTCLR_PIN13_Msk (0x1UL << GPIO_OUTCLR_PIN13_Pos) /*!< Bit mask of PIN13 field. */
#define GPIO_OUTCLR_PIN13_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN13_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN13_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 12 : Pin 12 */
#define GPIO_OUTCLR_PIN12_Pos (12UL) /*!< Position of PIN12 field. */
#define GPIO_OUTCLR_PIN12_Msk (0x1UL << GPIO_OUTCLR_PIN12_Pos) /*!< Bit mask of PIN12 field. */
#define GPIO_OUTCLR_PIN12_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN12_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN12_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 11 : Pin 11 */
#define GPIO_OUTCLR_PIN11_Pos (11UL) /*!< Position of PIN11 field. */
#define GPIO_OUTCLR_PIN11_Msk (0x1UL << GPIO_OUTCLR_PIN11_Pos) /*!< Bit mask of PIN11 field. */
#define GPIO_OUTCLR_PIN11_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN11_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN11_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 10 : Pin 10 */
#define GPIO_OUTCLR_PIN10_Pos (10UL) /*!< Position of PIN10 field. */
#define GPIO_OUTCLR_PIN10_Msk (0x1UL << GPIO_OUTCLR_PIN10_Pos) /*!< Bit mask of PIN10 field. */
#define GPIO_OUTCLR_PIN10_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN10_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN10_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 9 : Pin 9 */
#define GPIO_OUTCLR_PIN9_Pos (9UL) /*!< Position of PIN9 field. */
#define GPIO_OUTCLR_PIN9_Msk (0x1UL << GPIO_OUTCLR_PIN9_Pos) /*!< Bit mask of PIN9 field. */
#define GPIO_OUTCLR_PIN9_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN9_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN9_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 8 : Pin 8 */
#define GPIO_OUTCLR_PIN8_Pos (8UL) /*!< Position of PIN8 field. */
#define GPIO_OUTCLR_PIN8_Msk (0x1UL << GPIO_OUTCLR_PIN8_Pos) /*!< Bit mask of PIN8 field. */
#define GPIO_OUTCLR_PIN8_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN8_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN8_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 7 : Pin 7 */
#define GPIO_OUTCLR_PIN7_Pos (7UL) /*!< Position of PIN7 field. */
#define GPIO_OUTCLR_PIN7_Msk (0x1UL << GPIO_OUTCLR_PIN7_Pos) /*!< Bit mask of PIN7 field. */
#define GPIO_OUTCLR_PIN7_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN7_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN7_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 6 : Pin 6 */
#define GPIO_OUTCLR_PIN6_Pos (6UL) /*!< Position of PIN6 field. */
#define GPIO_OUTCLR_PIN6_Msk (0x1UL << GPIO_OUTCLR_PIN6_Pos) /*!< Bit mask of PIN6 field. */
#define GPIO_OUTCLR_PIN6_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN6_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN6_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 5 : Pin 5 */
#define GPIO_OUTCLR_PIN5_Pos (5UL) /*!< Position of PIN5 field. */
#define GPIO_OUTCLR_PIN5_Msk (0x1UL << GPIO_OUTCLR_PIN5_Pos) /*!< Bit mask of PIN5 field. */
#define GPIO_OUTCLR_PIN5_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN5_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN5_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 4 : Pin 4 */
#define GPIO_OUTCLR_PIN4_Pos (4UL) /*!< Position of PIN4 field. */
#define GPIO_OUTCLR_PIN4_Msk (0x1UL << GPIO_OUTCLR_PIN4_Pos) /*!< Bit mask of PIN4 field. */
#define GPIO_OUTCLR_PIN4_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN4_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN4_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 3 : Pin 3 */
#define GPIO_OUTCLR_PIN3_Pos (3UL) /*!< Position of PIN3 field. */
#define GPIO_OUTCLR_PIN3_Msk (0x1UL << GPIO_OUTCLR_PIN3_Pos) /*!< Bit mask of PIN3 field. */
#define GPIO_OUTCLR_PIN3_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN3_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN3_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 2 : Pin 2 */
#define GPIO_OUTCLR_PIN2_Pos (2UL) /*!< Position of PIN2 field. */
#define GPIO_OUTCLR_PIN2_Msk (0x1UL << GPIO_OUTCLR_PIN2_Pos) /*!< Bit mask of PIN2 field. */
#define GPIO_OUTCLR_PIN2_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN2_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN2_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 1 : Pin 1 */
#define GPIO_OUTCLR_PIN1_Pos (1UL) /*!< Position of PIN1 field. */
#define GPIO_OUTCLR_PIN1_Msk (0x1UL << GPIO_OUTCLR_PIN1_Pos) /*!< Bit mask of PIN1 field. */
#define GPIO_OUTCLR_PIN1_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN1_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN1_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Bit 0 : Pin 0 */
#define GPIO_OUTCLR_PIN0_Pos (0UL) /*!< Position of PIN0 field. */
#define GPIO_OUTCLR_PIN0_Msk (0x1UL << GPIO_OUTCLR_PIN0_Pos) /*!< Bit mask of PIN0 field. */
#define GPIO_OUTCLR_PIN0_Low (0UL) /*!< Read: pin driver is low */
#define GPIO_OUTCLR_PIN0_High (1UL) /*!< Read: pin driver is high */
#define GPIO_OUTCLR_PIN0_Clear (1UL) /*!< Write: writing a '1' sets the pin low; writing a '0' has no effect */

/* Register: GPIO_IN */
/* Description: Read GPIO port */

/* Bit 31 : Pin 31 */
#define GPIO_IN_PIN31_Pos (31UL) /*!< Position of PIN31 field. */
#define GPIO_IN_PIN31_Msk (0x1UL << GPIO_IN_PIN31_Pos) /*!< Bit mask of PIN31 field. */
#define GPIO_IN_PIN31_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN31_High (1UL) /*!< Pin input is high */

/* Bit 30 : Pin 30 */
#define GPIO_IN_PIN30_Pos (30UL) /*!< Position of PIN30 field. */
#define GPIO_IN_PIN30_Msk (0x1UL << GPIO_IN_PIN30_Pos) /*!< Bit mask of PIN30 field. */
#define GPIO_IN_PIN30_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN30_High (1UL) /*!< Pin input is high */

/* Bit 29 : Pin 29 */
#define GPIO_IN_PIN29_Pos (29UL) /*!< Position of PIN29 field. */
#define GPIO_IN_PIN29_Msk (0x1UL << GPIO_IN_PIN29_Pos) /*!< Bit mask of PIN29 field. */
#define GPIO_IN_PIN29_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN29_High (1UL) /*!< Pin input is high */

/* Bit 28 : Pin 28 */
#define GPIO_IN_PIN28_Pos (28UL) /*!< Position of PIN28 field. */
#define GPIO_IN_PIN28_Msk (0x1UL << GPIO_IN_PIN28_Pos) /*!< Bit mask of PIN28 field. */
#define GPIO_IN_PIN28_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN28_High (1UL) /*!< Pin input is high */

/* Bit 27 : Pin 27 */
#define GPIO_IN_PIN27_Pos (27UL) /*!< Position of PIN27 field. */
#define GPIO_IN_PIN27_Msk (0x1UL << GPIO_IN_PIN27_Pos) /*!< Bit mask of PIN27 field. */
#define GPIO_IN_PIN27_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN27_High (1UL) /*!< Pin input is high */

/* Bit 26 : Pin 26 */
#define GPIO_IN_PIN26_Pos (26UL) /*!< Position of PIN26 field. */
#define GPIO_IN_PIN26_Msk (0x1UL << GPIO_IN_PIN26_Pos) /*!< Bit mask of PIN26 field. */
#define GPIO_IN_PIN26_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN26_High (1UL) /*!< Pin input is high */

/* Bit 25 : Pin 25 */
#define GPIO_IN_PIN25_Pos (25UL) /*!< Position of PIN25 field. */
#define GPIO_IN_PIN25_Msk (0x1UL << GPIO_IN_PIN25_Pos) /*!< Bit mask of PIN25 field. */
#define GPIO_IN_PIN25_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN25_High (1UL) /*!< Pin input is high */

/* Bit 24 : Pin 24 */
#define GPIO_IN_PIN24_Pos (24UL) /*!< Position of PIN24 field. */
#define GPIO_IN_PIN24_Msk (0x1UL << GPIO_IN_PIN24_Pos) /*!< Bit mask of PIN24 field. */
#define GPIO_IN_PIN24_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN24_High (1UL) /*!< Pin input is high */

/* Bit 23 : Pin 23 */
#define GPIO_IN_PIN23_Pos (23UL) /*!< Position of PIN23 field. */
#define GPIO_IN_PIN23_Msk (0x1UL << GPIO_IN_PIN23_Pos) /*!< Bit mask of PIN23 field. */
#define GPIO_IN_PIN23_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN23_High (1UL) /*!< Pin input is high */

/* Bit 22 : Pin 22 */
#define GPIO_IN_PIN22_Pos (22UL) /*!< Position of PIN22 field. */
#define GPIO_IN_PIN22_Msk (0x1UL << GPIO_IN_PIN22_Pos) /*!< Bit mask of PIN22 field. */
#define GPIO_IN_PIN22_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN22_High (1UL) /*!< Pin input is high */

/* Bit 21 : Pin 21 */
#define GPIO_IN_PIN21_Pos (21UL) /*!< Position of PIN21 field. */
#define GPIO_IN_PIN21_Msk (0x1UL << GPIO_IN_PIN21_Pos) /*!< Bit mask of PIN21 field. */
#define GPIO_IN_PIN21_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN21_High (1UL) /*!< Pin input is high */

/* Bit 20 : Pin 20 */
#define GPIO_IN_PIN20_Pos (20UL) /*!< Position of PIN20 field. */
#define GPIO_IN_PIN20_Msk (0x1UL << GPIO_IN_PIN20_Pos) /*!< Bit mask of PIN20 field. */
#define GPIO_IN_PIN20_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN20_High (1UL) /*!< Pin input is high */

/* Bit 19 : Pin 19 */
#define GPIO_IN_PIN19_Pos (19UL) /*!< Position of PIN19 field. */
#define GPIO_IN_PIN19_Msk (0x1UL << GPIO_IN_PIN19_Pos) /*!< Bit mask of PIN19 field. */
#define GPIO_IN_PIN19_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN19_High (1UL) /*!< Pin input is high */

/* Bit 18 : Pin 18 */
#define GPIO_IN_PIN18_Pos (18UL) /*!< Position of PIN18 field. */
#define GPIO_IN_PIN18_Msk (0x1UL << GPIO_IN_PIN18_Pos) /*!< Bit mask of PIN18 field. */
#define GPIO_IN_PIN18_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN18_High (1UL) /*!< Pin input is high */

/* Bit 17 : Pin 17 */
#define GPIO_IN_PIN17_Pos (17UL) /*!< Position of PIN17 field. */
#define GPIO_IN_PIN17_Msk (0x1UL << GPIO_IN_PIN17_Pos) /*!< Bit mask of PIN17 field. */
#define GPIO_IN_PIN17_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN17_High (1UL) /*!< Pin input is high */

/* Bit 16 : Pin 16 */
#define GPIO_IN_PIN16_Pos (16UL) /*!< Position of PIN16 field. */
#define GPIO_IN_PIN16_Msk (0x1UL << GPIO_IN_PIN16_Pos) /*!< Bit mask of PIN16 field. */
#define GPIO_IN_PIN16_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN16_High (1UL) /*!< Pin input is high */

/* Bit 15 : Pin 15 */
#define GPIO_IN_PIN15_Pos (15UL) /*!< Position of PIN15 field. */
#define GPIO_IN_PIN15_Msk (0x1UL << GPIO_IN_PIN15_Pos) /*!< Bit mask of PIN15 field. */
#define GPIO_IN_PIN15_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN15_High (1UL) /*!< Pin input is high */

/* Bit 14 : Pin 14 */
#define GPIO_IN_PIN14_Pos (14UL) /*!< Position of PIN14 field. */
#define GPIO_IN_PIN14_Msk (0x1UL << GPIO_IN_PIN14_Pos) /*!< Bit mask of PIN14 field. */
#define GPIO_IN_PIN14_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN14_High (1UL) /*!< Pin input is high */

/* Bit 13 : Pin 13 */
#define GPIO_IN_PIN13_Pos (13UL) /*!< Position of PIN13 field. */
#define GPIO_IN_PIN13_Msk (0x1UL << GPIO_IN_PIN13_Pos) /*!< Bit mask of PIN13 field. */
#define GPIO_IN_PIN13_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN13_High (1UL) /*!< Pin input is high */

/* Bit 12 : Pin 12 */
#define GPIO_IN_PIN12_Pos (12UL) /*!< Position of PIN12 field. */
#define GPIO_IN_PIN12_Msk (0x1UL << GPIO_IN_PIN12_Pos) /*!< Bit mask of PIN12 field. */
#define GPIO_IN_PIN12_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN12_High (1UL) /*!< Pin input is high */

/* Bit 11 : Pin 11 */
#define GPIO_IN_PIN11_Pos (11UL) /*!< Position of PIN11 field. */
#define GPIO_IN_PIN11_Msk (0x1UL << GPIO_IN_PIN11_Pos) /*!< Bit mask of PIN11 field. */
#define GPIO_IN_PIN11_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN11_High (1UL) /*!< Pin input is high */

/* Bit 10 : Pin 10 */
#define GPIO_IN_PIN10_Pos (10UL) /*!< Position of PIN10 field. */
#define GPIO_IN_PIN10_Msk (0x1UL << GPIO_IN_PIN10_Pos) /*!< Bit mask of PIN10 field. */
#define GPIO_IN_PIN10_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN10_High (1UL) /*!< Pin input is high */

/* Bit 9 : Pin 9 */
#define GPIO_IN_PIN9_Pos (9UL) /*!< Position of PIN9 field. */
#define GPIO_IN_PIN9_Msk (0x1UL << GPIO_IN_PIN9_Pos) /*!< Bit mask of PIN9 field. */
#define GPIO_IN_PIN9_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN9_High (1UL) /*!< Pin input is high */

/* Bit 8 : Pin 8 */
#define GPIO_IN_PIN8_Pos (8UL) /*!< Position of PIN8 field. */
#define GPIO_IN_PIN8_Msk (0x1UL << GPIO_IN_PIN8_Pos) /*!< Bit mask of PIN8 field. */
#define GPIO_IN_PIN8_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN8_High (1UL) /*!< Pin input is high */

/* Bit 7 : Pin 7 */
#define GPIO_IN_PIN7_Pos (7UL) /*!< Position of PIN7 field. */
#define GPIO_IN_PIN7_Msk (0x1UL << GPIO_IN_PIN7_Pos) /*!< Bit mask of PIN7 field. */
#define GPIO_IN_PIN7_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN7_High (1UL) /*!< Pin input is high */

/* Bit 6 : Pin 6 */
#define GPIO_IN_PIN6_Pos (6UL) /*!< Position of PIN6 field. */
#define GPIO_IN_PIN6_Msk (0x1UL << GPIO_IN_PIN6_Pos) /*!< Bit mask of PIN6 field. */
#define GPIO_IN_PIN6_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN6_High (1UL) /*!< Pin input is high */

/* Bit 5 : Pin 5 */
#define GPIO_IN_PIN5_Pos (5UL) /*!< Position of PIN5 field. */
#define GPIO_IN_PIN5_Msk (0x1UL << GPIO_IN_PIN5_Pos) /*!< Bit mask of PIN5 field. */
#define GPIO_IN_PIN5_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN5_High (1UL) /*!< Pin input is high */

/* Bit 4 : Pin 4 */
#define GPIO_IN_PIN4_Pos (4UL) /*!< Position of PIN4 field. */
#define GPIO_IN_PIN4_Msk (0x1UL << GPIO_IN_PIN4_Pos) /*!< Bit mask of PIN4 field. */
#define GPIO_IN_PIN4_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN4_High (1UL) /*!< Pin input is high */

/* Bit 3 : Pin 3 */
#define GPIO_IN_PIN3_Pos (3UL) /*!< Position of PIN3 field. */
#define GPIO_IN_PIN3_Msk (0x1UL << GPIO_IN_PIN3_Pos) /*!< Bit mask of PIN3 field. */
#define GPIO_IN_PIN3_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN3_High (1UL) /*!< Pin input is high */

/* Bit 2 : Pin 2 */
#define GPIO_IN_PIN2_Pos (2UL) /*!< Position of PIN2 field. */
#define GPIO_IN_PIN2_Msk (0x1UL << GPIO_IN_PIN2_Pos) /*!< Bit mask of PIN2 field. */
#define GPIO_IN_PIN2_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN2_High (1UL) /*!< Pin input is high */

/* Bit 1 : Pin 1 */
#define GPIO_IN_PIN1_Pos (1UL) /*!< Position of PIN1 field. */
#define GPIO_IN_PIN1_Msk (0x1UL << GPIO_IN_PIN1_Pos) /*!< Bit mask of PIN1 field. */
#define GPIO_IN_PIN1_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN1_High (1UL) /*!< Pin input is high */

/* Bit 0 : Pin 0 */
#define GPIO_IN_PIN0_Pos (0UL) /*!< Position of PIN0 field. */
#define GPIO_IN_PIN0_Msk (0x1UL << GPIO_IN_PIN0_Pos) /*!< Bit mask of PIN0 field. */
#define GPIO_IN_PIN0_Low (0UL) /*!< Pin input is low */
#define GPIO_IN_PIN0_High (1UL) /*!< Pin input is high */

/* Register: GPIO_DIR */
/* Description: Direction of GPIO pins */

/* Bit 31 : Pin 31 */
#define GPIO_DIR_PIN31_Pos (31UL) /*!< Position of PIN31 field. */
#define GPIO_DIR_PIN31_Msk (0x1UL << GPIO_DIR_PIN31_Pos) /*!< Bit mask of PIN31 field. */
#define GPIO_DIR_PIN31_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN31_Output (1UL) /*!< Pin set as output */

/* Bit 30 : Pin 30 */
#define GPIO_DIR_PIN30_Pos (30UL) /*!< Position of PIN30 field. */
#define GPIO_DIR_PIN30_Msk (0x1UL << GPIO_DIR_PIN30_Pos) /*!< Bit mask of PIN30 field. */
#define GPIO_DIR_PIN30_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN30_Output (1UL) /*!< Pin set as output */

/* Bit 29 : Pin 29 */
#define GPIO_DIR_PIN29_Pos (29UL) /*!< Position of PIN29 field. */
#define GPIO_DIR_PIN29_Msk (0x1UL << GPIO_DIR_PIN29_Pos) /*!< Bit mask of PIN29 field. */
#define GPIO_DIR_PIN29_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN29_Output (1UL) /*!< Pin set as output */

/* Bit 28 : Pin 28 */
#define GPIO_DIR_PIN28_Pos (28UL) /*!< Position of PIN28 field. */
#define GPIO_DIR_PIN28_Msk (0x1UL << GPIO_DIR_PIN28_Pos) /*!< Bit mask of PIN28 field. */
#define GPIO_DIR_PIN28_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN28_Output (1UL) /*!< Pin set as output */

/* Bit 27 : Pin 27 */
#define GPIO_DIR_PIN27_Pos (27UL) /*!< Position of PIN27 field. */
#define GPIO_DIR_PIN27_Msk (0x1UL << GPIO_DIR_PIN27_Pos) /*!< Bit mask of PIN27 field. */
#define GPIO_DIR_PIN27_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN27_Output (1UL) /*!< Pin set as output */

/* Bit 26 : Pin 26 */
#define GPIO_DIR_PIN26_Pos (26UL) /*!< Position of PIN26 field. */
#define GPIO_DIR_PIN26_Msk (0x1UL << GPIO_DIR_PIN26_Pos) /*!< Bit mask of PIN26 field. */
#define GPIO_DIR_PIN26_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN26_Output (1UL) /*!< Pin set as output */

/* Bit 25 : Pin 25 */
#define GPIO_DIR_PIN25_Pos (25UL) /*!< Position of PIN25 field. */
#define GPIO_DIR_PIN25_Msk (0x1UL << GPIO_DIR_PIN25_Pos) /*!< Bit mask of PIN25 field. */
#define GPIO_DIR_PIN25_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN25_Output (1UL) /*!< Pin set as output */

/* Bit 24 : Pin 24 */
#define GPIO_DIR_PIN24_Pos (24UL) /*!< Position of PIN24 field. */
#define GPIO_DIR_PIN24_Msk (0x1UL << GPIO_DIR_PIN24_Pos) /*!< Bit mask of PIN24 field. */
#define GPIO_DIR_PIN24_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN24_Output (1UL) /*!< Pin set as output */

/* Bit 23 : Pin 23 */
#define GPIO_DIR_PIN23_Pos (23UL) /*!< Position of PIN23 field. */
#define GPIO_DIR_PIN23_Msk (0x1UL << GPIO_DIR_PIN23_Pos) /*!< Bit mask of PIN23 field. */
#define GPIO_DIR_PIN23_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN23_Output (1UL) /*!< Pin set as output */

/* Bit 22 : Pin 22 */
#define GPIO_DIR_PIN22_Pos (22UL) /*!< Position of PIN22 field. */
#define GPIO_DIR_PIN22_Msk (0x1UL << GPIO_DIR_PIN22_Pos) /*!< Bit mask of PIN22 field. */
#define GPIO_DIR_PIN22_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN22_Output (1UL) /*!< Pin set as output */

/* Bit 21 : Pin 21 */
#define GPIO_DIR_PIN21_Pos (21UL) /*!< Position of PIN21 field. */
#define GPIO_DIR_PIN21_Msk (0x1UL << GPIO_DIR_PIN21_Pos) /*!< Bit mask of PIN21 field. */
#define GPIO_DIR_PIN21_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN21_Output (1UL) /*!< Pin set as output */

/* Bit 20 : Pin 20 */
#define GPIO_DIR_PIN20_Pos (20UL) /*!< Position of PIN20 field. */
#define GPIO_DIR_PIN20_Msk (0x1UL << GPIO_DIR_PIN20_Pos) /*!< Bit mask of PIN20 field. */
#define GPIO_DIR_PIN20_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN20_Output (1UL) /*!< Pin set as output */

/* Bit 19 : Pin 19 */
#define GPIO_DIR_PIN19_Pos (19UL) /*!< Position of PIN19 field. */
#define GPIO_DIR_PIN19_Msk (0x1UL << GPIO_DIR_PIN19_Pos) /*!< Bit mask of PIN19 field. */
#define GPIO_DIR_PIN19_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN19_Output (1UL) /*!< Pin set as output */

/* Bit 18 : Pin 18 */
#define GPIO_DIR_PIN18_Pos (18UL) /*!< Position of PIN18 field. */
#define GPIO_DIR_PIN18_Msk (0x1UL << GPIO_DIR_PIN18_Pos) /*!< Bit mask of PIN18 field. */
#define GPIO_DIR_PIN18_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN18_Output (1UL) /*!< Pin set as output */

/* Bit 17 : Pin 17 */
#define GPIO_DIR_PIN17_Pos (17UL) /*!< Position of PIN17 field. */
#define GPIO_DIR_PIN17_Msk (0x1UL << GPIO_DIR_PIN17_Pos) /*!< Bit mask of PIN17 field. */
#define GPIO_DIR_PIN17_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN17_Output (1UL) /*!< Pin set as output */

/* Bit 16 : Pin 16 */
#define GPIO_DIR_PIN16_Pos (16UL) /*!< Position of PIN16 field. */
#define GPIO_DIR_PIN16_Msk (0x1UL << GPIO_DIR_PIN16_Pos) /*!< Bit mask of PIN16 field. */
#define GPIO_DIR_PIN16_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN16_Output (1UL) /*!< Pin set as output */

/* Bit 15 : Pin 15 */
#define GPIO_DIR_PIN15_Pos (15UL) /*!< Position of PIN15 field. */
#define GPIO_DIR_PIN15_Msk (0x1UL << GPIO_DIR_PIN15_Pos) /*!< Bit mask of PIN15 field. */
#define GPIO_DIR_PIN15_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN15_Output (1UL) /*!< Pin set as output */

/* Bit 14 : Pin 14 */
#define GPIO_DIR_PIN14_Pos (14UL) /*!< Position of PIN14 field. */
#define GPIO_DIR_PIN14_Msk (0x1UL << GPIO_DIR_PIN14_Pos) /*!< Bit mask of PIN14 field. */
#define GPIO_DIR_PIN14_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN14_Output (1UL) /*!< Pin set as output */

/* Bit 13 : Pin 13 */
#define GPIO_DIR_PIN13_Pos (13UL) /*!< Position of PIN13 field. */
#define GPIO_DIR_PIN13_Msk (0x1UL << GPIO_DIR_PIN13_Pos) /*!< Bit mask of PIN13 field. */
#define GPIO_DIR_PIN13_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN13_Output (1UL) /*!< Pin set as output */

/* Bit 12 : Pin 12 */
#define GPIO_DIR_PIN12_Pos (12UL) /*!< Position of PIN12 field. */
#define GPIO_DIR_PIN12_Msk (0x1UL << GPIO_DIR_PIN12_Pos) /*!< Bit mask of PIN12 field. */
#define GPIO_DIR_PIN12_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN12_Output (1UL) /*!< Pin set as output */

/* Bit 11 : Pin 11 */
#define GPIO_DIR_PIN11_Pos (11UL) /*!< Position of PIN11 field. */
#define GPIO_DIR_PIN11_Msk (0x1UL << GPIO_DIR_PIN11_Pos) /*!< Bit mask of PIN11 field. */
#define GPIO_DIR_PIN11_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN11_Output (1UL) /*!< Pin set as output */

/* Bit 10 : Pin 10 */
#define GPIO_DIR_PIN10_Pos (10UL) /*!< Position of PIN10 field. */
#define GPIO_DIR_PIN10_Msk (0x1UL << GPIO_DIR_PIN10_Pos) /*!< Bit mask of PIN10 field. */
#define GPIO_DIR_PIN10_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN10_Output (1UL) /*!< Pin set as output */

/* Bit 9 : Pin 9 */
#define GPIO_DIR_PIN9_Pos (9UL) /*!< Position of PIN9 field. */
#define GPIO_DIR_PIN9_Msk (0x1UL << GPIO_DIR_PIN9_Pos) /*!< Bit mask of PIN9 field. */
#define GPIO_DIR_PIN9_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN9_Output (1UL) /*!< Pin set as output */

/* Bit 8 : Pin 8 */
#define GPIO_DIR_PIN8_Pos (8UL) /*!< Position of PIN8 field. */
#define GPIO_DIR_PIN8_Msk (0x1UL << GPIO_DIR_PIN8_Pos) /*!< Bit mask of PIN8 field. */
#define GPIO_DIR_PIN8_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN8_Output (1UL) /*!< Pin set as output */

/* Bit 7 : Pin 7 */
#define GPIO_DIR_PIN7_Pos (7UL) /*!< Position of PIN7 field. */
#define GPIO_DIR_PIN7_Msk (0x1UL << GPIO_DIR_PIN7_Pos) /*!< Bit mask of PIN7 field. */
#define GPIO_DIR_PIN7_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN7_Output (1UL) /*!< Pin set as output */

/* Bit 6 : Pin 6 */
#define GPIO_DIR_PIN6_Pos (6UL) /*!< Position of PIN6 field. */
#define GPIO_DIR_PIN6_Msk (0x1UL << GPIO_DIR_PIN6_Pos) /*!< Bit mask of PIN6 field. */
#define GPIO_DIR_PIN6_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN6_Output (1UL) /*!< Pin set as output */

/* Bit 5 : Pin 5 */
#define GPIO_DIR_PIN5_Pos (5UL) /*!< Position of PIN5 field. */
#define GPIO_DIR_PIN5_Msk (0x1UL << GPIO_DIR_PIN5_Pos) /*!< Bit mask of PIN5 field. */
#define GPIO_DIR_PIN5_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN5_Output (1UL) /*!< Pin set as output */

/* Bit 4 : Pin 4 */
#define GPIO_DIR_PIN4_Pos (4UL) /*!< Position of PIN4 field. */
#define GPIO_DIR_PIN4_Msk (0x1UL << GPIO_DIR_PIN4_Pos) /*!< Bit mask of PIN4 field. */
#define GPIO_DIR_PIN4_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN4_Output (1UL) /*!< Pin set as output */

/* Bit 3 : Pin 3 */
#define GPIO_DIR_PIN3_Pos (3UL) /*!< Position of PIN3 field. */
#define GPIO_DIR_PIN3_Msk (0x1UL << GPIO_DIR_PIN3_Pos) /*!< Bit mask of PIN3 field. */
#define GPIO_DIR_PIN3_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN3_Output (1UL) /*!< Pin set as output */

/* Bit 2 : Pin 2 */
#define GPIO_DIR_PIN2_Pos (2UL) /*!< Position of PIN2 field. */
#define GPIO_DIR_PIN2_Msk (0x1UL << GPIO_DIR_PIN2_Pos) /*!< Bit mask of PIN2 field. */
#define GPIO_DIR_PIN2_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN2_Output (1UL) /*!< Pin set as output */

/* Bit 1 : Pin 1 */
#define GPIO_DIR_PIN1_Pos (1UL) /*!< Position of PIN1 field. */
#define GPIO_DIR_PIN1_Msk (0x1UL << GPIO_DIR_PIN1_Pos) /*!< Bit mask of PIN1 field. */
#define GPIO_DIR_PIN1_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN1_Output (1UL) /*!< Pin set as output */

/* Bit 0 : Pin 0 */
#define GPIO_DIR_PIN0_Pos (0UL) /*!< Position of PIN0 field. */
#define GPIO_DIR_PIN0_Msk (0x1UL << GPIO_DIR_PIN0_Pos) /*!< Bit mask of PIN0 field. */
#define GPIO_DIR_PIN0_Input (0UL) /*!< Pin set as input */
#define GPIO_DIR_PIN0_Output (1UL) /*!< Pin set as output */

/* Register: GPIO_DIRSET */
/* Description: DIR set register */

/* Bit 31 : Set as output pin 31 */
#define GPIO_DIRSET_PIN31_Pos (31UL) /*!< Position of PIN31 field. */
#define GPIO_DIRSET_PIN31_Msk (0x1UL << GPIO_DIRSET_PIN31_Pos) /*!< Bit mask of PIN31 field. */
#define GPIO_DIRSET_PIN31_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN31_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN31_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 30 : Set as output pin 30 */
#define GPIO_DIRSET_PIN30_Pos (30UL) /*!< Position of PIN30 field. */
#define GPIO_DIRSET_PIN30_Msk (0x1UL << GPIO_DIRSET_PIN30_Pos) /*!< Bit mask of PIN30 field. */
#define GPIO_DIRSET_PIN30_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN30_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN30_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 29 : Set as output pin 29 */
#define GPIO_DIRSET_PIN29_Pos (29UL) /*!< Position of PIN29 field. */
#define GPIO_DIRSET_PIN29_Msk (0x1UL << GPIO_DIRSET_PIN29_Pos) /*!< Bit mask of PIN29 field. */
#define GPIO_DIRSET_PIN29_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN29_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN29_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 28 : Set as output pin 28 */
#define GPIO_DIRSET_PIN28_Pos (28UL) /*!< Position of PIN28 field. */
#define GPIO_DIRSET_PIN28_Msk (0x1UL << GPIO_DIRSET_PIN28_Pos) /*!< Bit mask of PIN28 field. */
#define GPIO_DIRSET_PIN28_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN28_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN28_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 27 : Set as output pin 27 */
#define GPIO_DIRSET_PIN27_Pos (27UL) /*!< Position of PIN27 field. */
#define GPIO_DIRSET_PIN27_Msk (0x1UL << GPIO_DIRSET_PIN27_Pos) /*!< Bit mask of PIN27 field. */
#define GPIO_DIRSET_PIN27_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN27_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN27_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 26 : Set as output pin 26 */
#define GPIO_DIRSET_PIN26_Pos (26UL) /*!< Position of PIN26 field. */
#define GPIO_DIRSET_PIN26_Msk (0x1UL << GPIO_DIRSET_PIN26_Pos) /*!< Bit mask of PIN26 field. */
#define GPIO_DIRSET_PIN26_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN26_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN26_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 25 : Set as output pin 25 */
#define GPIO_DIRSET_PIN25_Pos (25UL) /*!< Position of PIN25 field. */
#define GPIO_DIRSET_PIN25_Msk (0x1UL << GPIO_DIRSET_PIN25_Pos) /*!< Bit mask of PIN25 field. */
#define GPIO_DIRSET_PIN25_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN25_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN25_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 24 : Set as output pin 24 */
#define GPIO_DIRSET_PIN24_Pos (24UL) /*!< Position of PIN24 field. */
#define GPIO_DIRSET_PIN24_Msk (0x1UL << GPIO_DIRSET_PIN24_Pos) /*!< Bit mask of PIN24 field. */
#define GPIO_DIRSET_PIN24_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN24_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN24_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 23 : Set as output pin 23 */
#define GPIO_DIRSET_PIN23_Pos (23UL) /*!< Position of PIN23 field. */
#define GPIO_DIRSET_PIN23_Msk (0x1UL << GPIO_DIRSET_PIN23_Pos) /*!< Bit mask of PIN23 field. */
#define GPIO_DIRSET_PIN23_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN23_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN23_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 22 : Set as output pin 22 */
#define GPIO_DIRSET_PIN22_Pos (22UL) /*!< Position of PIN22 field. */
#define GPIO_DIRSET_PIN22_Msk (0x1UL << GPIO_DIRSET_PIN22_Pos) /*!< Bit mask of PIN22 field. */
#define GPIO_DIRSET_PIN22_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN22_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN22_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 21 : Set as output pin 21 */
#define GPIO_DIRSET_PIN21_Pos (21UL) /*!< Position of PIN21 field. */
#define GPIO_DIRSET_PIN21_Msk (0x1UL << GPIO_DIRSET_PIN21_Pos) /*!< Bit mask of PIN21 field. */
#define GPIO_DIRSET_PIN21_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN21_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN21_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 20 : Set as output pin 20 */
#define GPIO_DIRSET_PIN20_Pos (20UL) /*!< Position of PIN20 field. */
#define GPIO_DIRSET_PIN20_Msk (0x1UL << GPIO_DIRSET_PIN20_Pos) /*!< Bit mask of PIN20 field. */
#define GPIO_DIRSET_PIN20_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN20_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN20_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 19 : Set as output pin 19 */
#define GPIO_DIRSET_PIN19_Pos (19UL) /*!< Position of PIN19 field. */
#define GPIO_DIRSET_PIN19_Msk (0x1UL << GPIO_DIRSET_PIN19_Pos) /*!< Bit mask of PIN19 field. */
#define GPIO_DIRSET_PIN19_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN19_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN19_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 18 : Set as output pin 18 */
#define GPIO_DIRSET_PIN18_Pos (18UL) /*!< Position of PIN18 field. */
#define GPIO_DIRSET_PIN18_Msk (0x1UL << GPIO_DIRSET_PIN18_Pos) /*!< Bit mask of PIN18 field. */
#define GPIO_DIRSET_PIN18_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN18_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN18_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 17 : Set as output pin 17 */
#define GPIO_DIRSET_PIN17_Pos (17UL) /*!< Position of PIN17 field. */
#define GPIO_DIRSET_PIN17_Msk (0x1UL << GPIO_DIRSET_PIN17_Pos) /*!< Bit mask of PIN17 field. */
#define GPIO_DIRSET_PIN17_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN17_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN17_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 16 : Set as output pin 16 */
#define GPIO_DIRSET_PIN16_Pos (16UL) /*!< Position of PIN16 field. */
#define GPIO_DIRSET_PIN16_Msk (0x1UL << GPIO_DIRSET_PIN16_Pos) /*!< Bit mask of PIN16 field. */
#define GPIO_DIRSET_PIN16_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN16_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN16_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 15 : Set as output pin 15 */
#define GPIO_DIRSET_PIN15_Pos (15UL) /*!< Position of PIN15 field. */
#define GPIO_DIRSET_PIN15_Msk (0x1UL << GPIO_DIRSET_PIN15_Pos) /*!< Bit mask of PIN15 field. */
#define GPIO_DIRSET_PIN15_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN15_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN15_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 14 : Set as output pin 14 */
#define GPIO_DIRSET_PIN14_Pos (14UL) /*!< Position of PIN14 field. */
#define GPIO_DIRSET_PIN14_Msk (0x1UL << GPIO_DIRSET_PIN14_Pos) /*!< Bit mask of PIN14 field. */
#define GPIO_DIRSET_PIN14_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN14_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN14_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 13 : Set as output pin 13 */
#define GPIO_DIRSET_PIN13_Pos (13UL) /*!< Position of PIN13 field. */
#define GPIO_DIRSET_PIN13_Msk (0x1UL << GPIO_DIRSET_PIN13_Pos) /*!< Bit mask of PIN13 field. */
#define GPIO_DIRSET_PIN13_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN13_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN13_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 12 : Set as output pin 12 */
#define GPIO_DIRSET_PIN12_Pos (12UL) /*!< Position of PIN12 field. */
#define GPIO_DIRSET_PIN12_Msk (0x1UL << GPIO_DIRSET_PIN12_Pos) /*!< Bit mask of PIN12 field. */
#define GPIO_DIRSET_PIN12_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN12_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN12_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 11 : Set as output pin 11 */
#define GPIO_DIRSET_PIN11_Pos (11UL) /*!< Position of PIN11 field. */
#define GPIO_DIRSET_PIN11_Msk (0x1UL << GPIO_DIRSET_PIN11_Pos) /*!< Bit mask of PIN11 field. */
#define GPIO_DIRSET_PIN11_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN11_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN11_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 10 : Set as output pin 10 */
#define GPIO_DIRSET_PIN10_Pos (10UL) /*!< Position of PIN10 field. */
#define GPIO_DIRSET_PIN10_Msk (0x1UL << GPIO_DIRSET_PIN10_Pos) /*!< Bit mask of PIN10 field. */
#define GPIO_DIRSET_PIN10_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN10_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN10_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 9 : Set as output pin 9 */
#define GPIO_DIRSET_PIN9_Pos (9UL) /*!< Position of PIN9 field. */
#define GPIO_DIRSET_PIN9_Msk (0x1UL << GPIO_DIRSET_PIN9_Pos) /*!< Bit mask of PIN9 field. */
#define GPIO_DIRSET_PIN9_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN9_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN9_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 8 : Set as output pin 8 */
#define GPIO_DIRSET_PIN8_Pos (8UL) /*!< Position of PIN8 field. */
#define GPIO_DIRSET_PIN8_Msk (0x1UL << GPIO_DIRSET_PIN8_Pos) /*!< Bit mask of PIN8 field. */
#define GPIO_DIRSET_PIN8_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN8_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN8_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 7 : Set as output pin 7 */
#define GPIO_DIRSET_PIN7_Pos (7UL) /*!< Position of PIN7 field. */
#define GPIO_DIRSET_PIN7_Msk (0x1UL << GPIO_DIRSET_PIN7_Pos) /*!< Bit mask of PIN7 field. */
#define GPIO_DIRSET_PIN7_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN7_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN7_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 6 : Set as output pin 6 */
#define GPIO_DIRSET_PIN6_Pos (6UL) /*!< Position of PIN6 field. */
#define GPIO_DIRSET_PIN6_Msk (0x1UL << GPIO_DIRSET_PIN6_Pos) /*!< Bit mask of PIN6 field. */
#define GPIO_DIRSET_PIN6_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN6_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN6_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 5 : Set as output pin 5 */
#define GPIO_DIRSET_PIN5_Pos (5UL) /*!< Position of PIN5 field. */
#define GPIO_DIRSET_PIN5_Msk (0x1UL << GPIO_DIRSET_PIN5_Pos) /*!< Bit mask of PIN5 field. */
#define GPIO_DIRSET_PIN5_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN5_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN5_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 4 : Set as output pin 4 */
#define GPIO_DIRSET_PIN4_Pos (4UL) /*!< Position of PIN4 field. */
#define GPIO_DIRSET_PIN4_Msk (0x1UL << GPIO_DIRSET_PIN4_Pos) /*!< Bit mask of PIN4 field. */
#define GPIO_DIRSET_PIN4_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN4_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN4_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 3 : Set as output pin 3 */
#define GPIO_DIRSET_PIN3_Pos (3UL) /*!< Position of PIN3 field. */
#define GPIO_DIRSET_PIN3_Msk (0x1UL << GPIO_DIRSET_PIN3_Pos) /*!< Bit mask of PIN3 field. */
#define GPIO_DIRSET_PIN3_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN3_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN3_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 2 : Set as output pin 2 */
#define GPIO_DIRSET_PIN2_Pos (2UL) /*!< Position of PIN2 field. */
#define GPIO_DIRSET_PIN2_Msk (0x1UL << GPIO_DIRSET_PIN2_Pos) /*!< Bit mask of PIN2 field. */
#define GPIO_DIRSET_PIN2_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN2_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN2_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 1 : Set as output pin 1 */
#define GPIO_DIRSET_PIN1_Pos (1UL) /*!< Position of PIN1 field. */
#define GPIO_DIRSET_PIN1_Msk (0x1UL << GPIO_DIRSET_PIN1_Pos) /*!< Bit mask of PIN1 field. */
#define GPIO_DIRSET_PIN1_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN1_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN1_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Bit 0 : Set as output pin 0 */
#define GPIO_DIRSET_PIN0_Pos (0UL) /*!< Position of PIN0 field. */
#define GPIO_DIRSET_PIN0_Msk (0x1UL << GPIO_DIRSET_PIN0_Pos) /*!< Bit mask of PIN0 field. */
#define GPIO_DIRSET_PIN0_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRSET_PIN0_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRSET_PIN0_Set (1UL) /*!< Write: writing a '1' sets pin to output; writing a '0' has no effect */

/* Register: GPIO_DIRCLR */
/* Description: DIR clear register */

/* Bit 31 : Set as input pin 31 */
#define GPIO_DIRCLR_PIN31_Pos (31UL) /*!< Position of PIN31 field. */
#define GPIO_DIRCLR_PIN31_Msk (0x1UL << GPIO_DIRCLR_PIN31_Pos) /*!< Bit mask of PIN31 field. */
#define GPIO_DIRCLR_PIN31_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN31_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN31_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 30 : Set as input pin 30 */
#define GPIO_DIRCLR_PIN30_Pos (30UL) /*!< Position of PIN30 field. */
#define GPIO_DIRCLR_PIN30_Msk (0x1UL << GPIO_DIRCLR_PIN30_Pos) /*!< Bit mask of PIN30 field. */
#define GPIO_DIRCLR_PIN30_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN30_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN30_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 29 : Set as input pin 29 */
#define GPIO_DIRCLR_PIN29_Pos (29UL) /*!< Position of PIN29 field. */
#define GPIO_DIRCLR_PIN29_Msk (0x1UL << GPIO_DIRCLR_PIN29_Pos) /*!< Bit mask of PIN29 field. */
#define GPIO_DIRCLR_PIN29_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN29_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN29_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 28 : Set as input pin 28 */
#define GPIO_DIRCLR_PIN28_Pos (28UL) /*!< Position of PIN28 field. */
#define GPIO_DIRCLR_PIN28_Msk (0x1UL << GPIO_DIRCLR_PIN28_Pos) /*!< Bit mask of PIN28 field. */
#define GPIO_DIRCLR_PIN28_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN28_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN28_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 27 : Set as input pin 27 */
#define GPIO_DIRCLR_PIN27_Pos (27UL) /*!< Position of PIN27 field. */
#define GPIO_DIRCLR_PIN27_Msk (0x1UL << GPIO_DIRCLR_PIN27_Pos) /*!< Bit mask of PIN27 field. */
#define GPIO_DIRCLR_PIN27_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN27_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN27_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 26 : Set as input pin 26 */
#define GPIO_DIRCLR_PIN26_Pos (26UL) /*!< Position of PIN26 field. */
#define GPIO_DIRCLR_PIN26_Msk (0x1UL << GPIO_DIRCLR_PIN26_Pos) /*!< Bit mask of PIN26 field. */
#define GPIO_DIRCLR_PIN26_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN26_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN26_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 25 : Set as input pin 25 */
#define GPIO_DIRCLR_PIN25_Pos (25UL) /*!< Position of PIN25 field. */
#define GPIO_DIRCLR_PIN25_Msk (0x1UL << GPIO_DIRCLR_PIN25_Pos) /*!< Bit mask of PIN25 field. */
#define GPIO_DIRCLR_PIN25_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN25_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN25_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 24 : Set as input pin 24 */
#define GPIO_DIRCLR_PIN24_Pos (24UL) /*!< Position of PIN24 field. */
#define GPIO_DIRCLR_PIN24_Msk (0x1UL << GPIO_DIRCLR_PIN24_Pos) /*!< Bit mask of PIN24 field. */
#define GPIO_DIRCLR_PIN24_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN24_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN24_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 23 : Set as input pin 23 */
#define GPIO_DIRCLR_PIN23_Pos (23UL) /*!< Position of PIN23 field. */
#define GPIO_DIRCLR_PIN23_Msk (0x1UL << GPIO_DIRCLR_PIN23_Pos) /*!< Bit mask of PIN23 field. */
#define GPIO_DIRCLR_PIN23_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN23_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN23_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 22 : Set as input pin 22 */
#define GPIO_DIRCLR_PIN22_Pos (22UL) /*!< Position of PIN22 field. */
#define GPIO_DIRCLR_PIN22_Msk (0x1UL << GPIO_DIRCLR_PIN22_Pos) /*!< Bit mask of PIN22 field. */
#define GPIO_DIRCLR_PIN22_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN22_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN22_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 21 : Set as input pin 21 */
#define GPIO_DIRCLR_PIN21_Pos (21UL) /*!< Position of PIN21 field. */
#define GPIO_DIRCLR_PIN21_Msk (0x1UL << GPIO_DIRCLR_PIN21_Pos) /*!< Bit mask of PIN21 field. */
#define GPIO_DIRCLR_PIN21_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN21_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN21_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 20 : Set as input pin 20 */
#define GPIO_DIRCLR_PIN20_Pos (20UL) /*!< Position of PIN20 field. */
#define GPIO_DIRCLR_PIN20_Msk (0x1UL << GPIO_DIRCLR_PIN20_Pos) /*!< Bit mask of PIN20 field. */
#define GPIO_DIRCLR_PIN20_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN20_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN20_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 19 : Set as input pin 19 */
#define GPIO_DIRCLR_PIN19_Pos (19UL) /*!< Position of PIN19 field. */
#define GPIO_DIRCLR_PIN19_Msk (0x1UL << GPIO_DIRCLR_PIN19_Pos) /*!< Bit mask of PIN19 field. */
#define GPIO_DIRCLR_PIN19_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN19_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN19_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 18 : Set as input pin 18 */
#define GPIO_DIRCLR_PIN18_Pos (18UL) /*!< Position of PIN18 field. */
#define GPIO_DIRCLR_PIN18_Msk (0x1UL << GPIO_DIRCLR_PIN18_Pos) /*!< Bit mask of PIN18 field. */
#define GPIO_DIRCLR_PIN18_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN18_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN18_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 17 : Set as input pin 17 */
#define GPIO_DIRCLR_PIN17_Pos (17UL) /*!< Position of PIN17 field. */
#define GPIO_DIRCLR_PIN17_Msk (0x1UL << GPIO_DIRCLR_PIN17_Pos) /*!< Bit mask of PIN17 field. */
#define GPIO_DIRCLR_PIN17_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN17_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN17_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 16 : Set as input pin 16 */
#define GPIO_DIRCLR_PIN16_Pos (16UL) /*!< Position of PIN16 field. */
#define GPIO_DIRCLR_PIN16_Msk (0x1UL << GPIO_DIRCLR_PIN16_Pos) /*!< Bit mask of PIN16 field. */
#define GPIO_DIRCLR_PIN16_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN16_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN16_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 15 : Set as input pin 15 */
#define GPIO_DIRCLR_PIN15_Pos (15UL) /*!< Position of PIN15 field. */
#define GPIO_DIRCLR_PIN15_Msk (0x1UL << GPIO_DIRCLR_PIN15_Pos) /*!< Bit mask of PIN15 field. */
#define GPIO_DIRCLR_PIN15_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN15_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN15_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 14 : Set as input pin 14 */
#define GPIO_DIRCLR_PIN14_Pos (14UL) /*!< Position of PIN14 field. */
#define GPIO_DIRCLR_PIN14_Msk (0x1UL << GPIO_DIRCLR_PIN14_Pos) /*!< Bit mask of PIN14 field. */
#define GPIO_DIRCLR_PIN14_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN14_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN14_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 13 : Set as input pin 13 */
#define GPIO_DIRCLR_PIN13_Pos (13UL) /*!< Position of PIN13 field. */
#define GPIO_DIRCLR_PIN13_Msk (0x1UL << GPIO_DIRCLR_PIN13_Pos) /*!< Bit mask of PIN13 field. */
#define GPIO_DIRCLR_PIN13_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN13_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN13_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 12 : Set as input pin 12 */
#define GPIO_DIRCLR_PIN12_Pos (12UL) /*!< Position of PIN12 field. */
#define GPIO_DIRCLR_PIN12_Msk (0x1UL << GPIO_DIRCLR_PIN12_Pos) /*!< Bit mask of PIN12 field. */
#define GPIO_DIRCLR_PIN12_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN12_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN12_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 11 : Set as input pin 11 */
#define GPIO_DIRCLR_PIN11_Pos (11UL) /*!< Position of PIN11 field. */
#define GPIO_DIRCLR_PIN11_Msk (0x1UL << GPIO_DIRCLR_PIN11_Pos) /*!< Bit mask of PIN11 field. */
#define GPIO_DIRCLR_PIN11_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN11_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN11_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 10 : Set as input pin 10 */
#define GPIO_DIRCLR_PIN10_Pos (10UL) /*!< Position of PIN10 field. */
#define GPIO_DIRCLR_PIN10_Msk (0x1UL << GPIO_DIRCLR_PIN10_Pos) /*!< Bit mask of PIN10 field. */
#define GPIO_DIRCLR_PIN10_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN10_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN10_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 9 : Set as input pin 9 */
#define GPIO_DIRCLR_PIN9_Pos (9UL) /*!< Position of PIN9 field. */
#define GPIO_DIRCLR_PIN9_Msk (0x1UL << GPIO_DIRCLR_PIN9_Pos) /*!< Bit mask of PIN9 field. */
#define GPIO_DIRCLR_PIN9_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN9_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN9_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 8 : Set as input pin 8 */
#define GPIO_DIRCLR_PIN8_Pos (8UL) /*!< Position of PIN8 field. */
#define GPIO_DIRCLR_PIN8_Msk (0x1UL << GPIO_DIRCLR_PIN8_Pos) /*!< Bit mask of PIN8 field. */
#define GPIO_DIRCLR_PIN8_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN8_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN8_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 7 : Set as input pin 7 */
#define GPIO_DIRCLR_PIN7_Pos (7UL) /*!< Position of PIN7 field. */
#define GPIO_DIRCLR_PIN7_Msk (0x1UL << GPIO_DIRCLR_PIN7_Pos) /*!< Bit mask of PIN7 field. */
#define GPIO_DIRCLR_PIN7_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN7_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN7_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 6 : Set as input pin 6 */
#define GPIO_DIRCLR_PIN6_Pos (6UL) /*!< Position of PIN6 field. */
#define GPIO_DIRCLR_PIN6_Msk (0x1UL << GPIO_DIRCLR_PIN6_Pos) /*!< Bit mask of PIN6 field. */
#define GPIO_DIRCLR_PIN6_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN6_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN6_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 5 : Set as input pin 5 */
#define GPIO_DIRCLR_PIN5_Pos (5UL) /*!< Position of PIN5 field. */
#define GPIO_DIRCLR_PIN5_Msk (0x1UL << GPIO_DIRCLR_PIN5_Pos) /*!< Bit mask of PIN5 field. */
#define GPIO_DIRCLR_PIN5_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN5_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN5_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 4 : Set as input pin 4 */
#define GPIO_DIRCLR_PIN4_Pos (4UL) /*!< Position of PIN4 field. */
#define GPIO_DIRCLR_PIN4_Msk (0x1UL << GPIO_DIRCLR_PIN4_Pos) /*!< Bit mask of PIN4 field. */
#define GPIO_DIRCLR_PIN4_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN4_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN4_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 3 : Set as input pin 3 */
#define GPIO_DIRCLR_PIN3_Pos (3UL) /*!< Position of PIN3 field. */
#define GPIO_DIRCLR_PIN3_Msk (0x1UL << GPIO_DIRCLR_PIN3_Pos) /*!< Bit mask of PIN3 field. */
#define GPIO_DIRCLR_PIN3_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN3_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN3_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 2 : Set as input pin 2 */
#define GPIO_DIRCLR_PIN2_Pos (2UL) /*!< Position of PIN2 field. */
#define GPIO_DIRCLR_PIN2_Msk (0x1UL << GPIO_DIRCLR_PIN2_Pos) /*!< Bit mask of PIN2 field. */
#define GPIO_DIRCLR_PIN2_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN2_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN2_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 1 : Set as input pin 1 */
#define GPIO_DIRCLR_PIN1_Pos (1UL) /*!< Position of PIN1 field. */
#define GPIO_DIRCLR_PIN1_Msk (0x1UL << GPIO_DIRCLR_PIN1_Pos) /*!< Bit mask of PIN1 field. */
#define GPIO_DIRCLR_PIN1_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN1_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN1_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Bit 0 : Set as input pin 0 */
#define GPIO_DIRCLR_PIN0_Pos (0UL) /*!< Position of PIN0 field. */
#define GPIO_DIRCLR_PIN0_Msk (0x1UL << GPIO_DIRCLR_PIN0_Pos) /*!< Bit mask of PIN0 field. */
#define GPIO_DIRCLR_PIN0_Input (0UL) /*!< Read: pin set as input */
#define GPIO_DIRCLR_PIN0_Output (1UL) /*!< Read: pin set as output */
#define GPIO_DIRCLR_PIN0_Clear (1UL) /*!< Write: writing a '1' sets pin to input; writing a '0' has no effect */

/* Register: GPIO_LATCH */
/* Description: Latch register indicating what GPIO pins that have met the criteria set in the PIN_CNF[n].SENSE registers */

/* Bit 31 : Status on whether PIN31 has met criteria set in PIN_CNF31.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN31_Pos (31UL) /*!< Position of PIN31 field. */
#define GPIO_LATCH_PIN31_Msk (0x1UL << GPIO_LATCH_PIN31_Pos) /*!< Bit mask of PIN31 field. */
#define GPIO_LATCH_PIN31_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN31_Latched (1UL) /*!< Criteria has been met */

/* Bit 30 : Status on whether PIN30 has met criteria set in PIN_CNF30.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN30_Pos (30UL) /*!< Position of PIN30 field. */
#define GPIO_LATCH_PIN30_Msk (0x1UL << GPIO_LATCH_PIN30_Pos) /*!< Bit mask of PIN30 field. */
#define GPIO_LATCH_PIN30_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN30_Latched (1UL) /*!< Criteria has been met */

/* Bit 29 : Status on whether PIN29 has met criteria set in PIN_CNF29.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN29_Pos (29UL) /*!< Position of PIN29 field. */
#define GPIO_LATCH_PIN29_Msk (0x1UL << GPIO_LATCH_PIN29_Pos) /*!< Bit mask of PIN29 field. */
#define GPIO_LATCH_PIN29_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN29_Latched (1UL) /*!< Criteria has been met */

/* Bit 28 : Status on whether PIN28 has met criteria set in PIN_CNF28.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN28_Pos (28UL) /*!< Position of PIN28 field. */
#define GPIO_LATCH_PIN28_Msk (0x1UL << GPIO_LATCH_PIN28_Pos) /*!< Bit mask of PIN28 field. */
#define GPIO_LATCH_PIN28_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN28_Latched (1UL) /*!< Criteria has been met */

/* Bit 27 : Status on whether PIN27 has met criteria set in PIN_CNF27.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN27_Pos (27UL) /*!< Position of PIN27 field. */
#define GPIO_LATCH_PIN27_Msk (0x1UL << GPIO_LATCH_PIN27_Pos) /*!< Bit mask of PIN27 field. */
#define GPIO_LATCH_PIN27_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN27_Latched (1UL) /*!< Criteria has been met */

/* Bit 26 : Status on whether PIN26 has met criteria set in PIN_CNF26.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN26_Pos (26UL) /*!< Position of PIN26 field. */
#define GPIO_LATCH_PIN26_Msk (0x1UL << GPIO_LATCH_PIN26_Pos) /*!< Bit mask of PIN26 field. */
#define GPIO_LATCH_PIN26_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN26_Latched (1UL) /*!< Criteria has been met */

/* Bit 25 : Status on whether PIN25 has met criteria set in PIN_CNF25.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN25_Pos (25UL) /*!< Position of PIN25 field. */
#define GPIO_LATCH_PIN25_Msk (0x1UL << GPIO_LATCH_PIN25_Pos) /*!< Bit mask of PIN25 field. */
#define GPIO_LATCH_PIN25_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN25_Latched (1UL) /*!< Criteria has been met */

/* Bit 24 : Status on whether PIN24 has met criteria set in PIN_CNF24.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN24_Pos (24UL) /*!< Position of PIN24 field. */
#define GPIO_LATCH_PIN24_Msk (0x1UL << GPIO_LATCH_PIN24_Pos) /*!< Bit mask of PIN24 field. */
#define GPIO_LATCH_PIN24_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN24_Latched (1UL) /*!< Criteria has been met */

/* Bit 23 : Status on whether PIN23 has met criteria set in PIN_CNF23.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN23_Pos (23UL) /*!< Position of PIN23 field. */
#define GPIO_LATCH_PIN23_Msk (0x1UL << GPIO_LATCH_PIN23_Pos) /*!< Bit mask of PIN23 field. */
#define GPIO_LATCH_PIN23_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN23_Latched (1UL) /*!< Criteria has been met */

/* Bit 22 : Status on whether PIN22 has met criteria set in PIN_CNF22.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN22_Pos (22UL) /*!< Position of PIN22 field. */
#define GPIO_LATCH_PIN22_Msk (0x1UL << GPIO_LATCH_PIN22_Pos) /*!< Bit mask of PIN22 field. */
#define GPIO_LATCH_PIN22_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN22_Latched (1UL) /*!< Criteria has been met */

/* Bit 21 : Status on whether PIN21 has met criteria set in PIN_CNF21.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN21_Pos (21UL) /*!< Position of PIN21 field. */
#define GPIO_LATCH_PIN21_Msk (0x1UL << GPIO_LATCH_PIN21_Pos) /*!< Bit mask of PIN21 field. */
#define GPIO_LATCH_PIN21_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN21_Latched (1UL) /*!< Criteria has been met */

/* Bit 20 : Status on whether PIN20 has met criteria set in PIN_CNF20.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN20_Pos (20UL) /*!< Position of PIN20 field. */
#define GPIO_LATCH_PIN20_Msk (0x1UL << GPIO_LATCH_PIN20_Pos) /*!< Bit mask of PIN20 field. */
#define GPIO_LATCH_PIN20_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN20_Latched (1UL) /*!< Criteria has been met */

/* Bit 19 : Status on whether PIN19 has met criteria set in PIN_CNF19.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN19_Pos (19UL) /*!< Position of PIN19 field. */
#define GPIO_LATCH_PIN19_Msk (0x1UL << GPIO_LATCH_PIN19_Pos) /*!< Bit mask of PIN19 field. */
#define GPIO_LATCH_PIN19_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN19_Latched (1UL) /*!< Criteria has been met */

/* Bit 18 : Status on whether PIN18 has met criteria set in PIN_CNF18.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN18_Pos (18UL) /*!< Position of PIN18 field. */
#define GPIO_LATCH_PIN18_Msk (0x1UL << GPIO_LATCH_PIN18_Pos) /*!< Bit mask of PIN18 field. */
#define GPIO_LATCH_PIN18_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN18_Latched (1UL) /*!< Criteria has been met */

/* Bit 17 : Status on whether PIN17 has met criteria set in PIN_CNF17.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN17_Pos (17UL) /*!< Position of PIN17 field. */
#define GPIO_LATCH_PIN17_Msk (0x1UL << GPIO_LATCH_PIN17_Pos) /*!< Bit mask of PIN17 field. */
#define GPIO_LATCH_PIN17_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN17_Latched (1UL) /*!< Criteria has been met */

/* Bit 16 : Status on whether PIN16 has met criteria set in PIN_CNF16.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN16_Pos (16UL) /*!< Position of PIN16 field. */
#define GPIO_LATCH_PIN16_Msk (0x1UL << GPIO_LATCH_PIN16_Pos) /*!< Bit mask of PIN16 field. */
#define GPIO_LATCH_PIN16_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN16_Latched (1UL) /*!< Criteria has been met */

/* Bit 15 : Status on whether PIN15 has met criteria set in PIN_CNF15.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN15_Pos (15UL) /*!< Position of PIN15 field. */
#define GPIO_LATCH_PIN15_Msk (0x1UL << GPIO_LATCH_PIN15_Pos) /*!< Bit mask of PIN15 field. */
#define GPIO_LATCH_PIN15_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN15_Latched (1UL) /*!< Criteria has been met */

/* Bit 14 : Status on whether PIN14 has met criteria set in PIN_CNF14.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN14_Pos (14UL) /*!< Position of PIN14 field. */
#define GPIO_LATCH_PIN14_Msk (0x1UL << GPIO_LATCH_PIN14_Pos) /*!< Bit mask of PIN14 field. */
#define GPIO_LATCH_PIN14_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN14_Latched (1UL) /*!< Criteria has been met */

/* Bit 13 : Status on whether PIN13 has met criteria set in PIN_CNF13.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN13_Pos (13UL) /*!< Position of PIN13 field. */
#define GPIO_LATCH_PIN13_Msk (0x1UL << GPIO_LATCH_PIN13_Pos) /*!< Bit mask of PIN13 field. */
#define GPIO_LATCH_PIN13_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN13_Latched (1UL) /*!< Criteria has been met */

/* Bit 12 : Status on whether PIN12 has met criteria set in PIN_CNF12.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN12_Pos (12UL) /*!< Position of PIN12 field. */
#define GPIO_LATCH_PIN12_Msk (0x1UL << GPIO_LATCH_PIN12_Pos) /*!< Bit mask of PIN12 field. */
#define GPIO_LATCH_PIN12_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN12_Latched (1UL) /*!< Criteria has been met */

/* Bit 11 : Status on whether PIN11 has met criteria set in PIN_CNF11.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN11_Pos (11UL) /*!< Position of PIN11 field. */
#define GPIO_LATCH_PIN11_Msk (0x1UL << GPIO_LATCH_PIN11_Pos) /*!< Bit mask of PIN11 field. */
#define GPIO_LATCH_PIN11_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN11_Latched (1UL) /*!< Criteria has been met */

/* Bit 10 : Status on whether PIN10 has met criteria set in PIN_CNF10.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN10_Pos (10UL) /*!< Position of PIN10 field. */
#define GPIO_LATCH_PIN10_Msk (0x1UL << GPIO_LATCH_PIN10_Pos) /*!< Bit mask of PIN10 field. */
#define GPIO_LATCH_PIN10_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN10_Latched (1UL) /*!< Criteria has been met */

/* Bit 9 : Status on whether PIN9 has met criteria set in PIN_CNF9.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN9_Pos (9UL) /*!< Position of PIN9 field. */
#define GPIO_LATCH_PIN9_Msk (0x1UL << GPIO_LATCH_PIN9_Pos) /*!< Bit mask of PIN9 field. */
#define GPIO_LATCH_PIN9_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN9_Latched (1UL) /*!< Criteria has been met */

/* Bit 8 : Status on whether PIN8 has met criteria set in PIN_CNF8.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN8_Pos (8UL) /*!< Position of PIN8 field. */
#define GPIO_LATCH_PIN8_Msk (0x1UL << GPIO_LATCH_PIN8_Pos) /*!< Bit mask of PIN8 field. */
#define GPIO_LATCH_PIN8_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN8_Latched (1UL) /*!< Criteria has been met */

/* Bit 7 : Status on whether PIN7 has met criteria set in PIN_CNF7.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN7_Pos (7UL) /*!< Position of PIN7 field. */
#define GPIO_LATCH_PIN7_Msk (0x1UL << GPIO_LATCH_PIN7_Pos) /*!< Bit mask of PIN7 field. */
#define GPIO_LATCH_PIN7_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN7_Latched (1UL) /*!< Criteria has been met */

/* Bit 6 : Status on whether PIN6 has met criteria set in PIN_CNF6.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN6_Pos (6UL) /*!< Position of PIN6 field. */
#define GPIO_LATCH_PIN6_Msk (0x1UL << GPIO_LATCH_PIN6_Pos) /*!< Bit mask of PIN6 field. */
#define GPIO_LATCH_PIN6_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN6_Latched (1UL) /*!< Criteria has been met */

/* Bit 5 : Status on whether PIN5 has met criteria set in PIN_CNF5.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN5_Pos (5UL) /*!< Position of PIN5 field. */
#define GPIO_LATCH_PIN5_Msk (0x1UL << GPIO_LATCH_PIN5_Pos) /*!< Bit mask of PIN5 field. */
#define GPIO_LATCH_PIN5_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN5_Latched (1UL) /*!< Criteria has been met */

/* Bit 4 : Status on whether PIN4 has met criteria set in PIN_CNF4.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN4_Pos (4UL) /*!< Position of PIN4 field. */
#define GPIO_LATCH_PIN4_Msk (0x1UL << GPIO_LATCH_PIN4_Pos) /*!< Bit mask of PIN4 field. */
#define GPIO_LATCH_PIN4_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN4_Latched (1UL) /*!< Criteria has been met */

/* Bit 3 : Status on whether PIN3 has met criteria set in PIN_CNF3.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN3_Pos (3UL) /*!< Position of PIN3 field. */
#define GPIO_LATCH_PIN3_Msk (0x1UL << GPIO_LATCH_PIN3_Pos) /*!< Bit mask of PIN3 field. */
#define GPIO_LATCH_PIN3_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN3_Latched (1UL) /*!< Criteria has been met */

/* Bit 2 : Status on whether PIN2 has met criteria set in PIN_CNF2.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN2_Pos (2UL) /*!< Position of PIN2 field. */
#define GPIO_LATCH_PIN2_Msk (0x1UL << GPIO_LATCH_PIN2_Pos) /*!< Bit mask of PIN2 field. */
#define GPIO_LATCH_PIN2_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN2_Latched (1UL) /*!< Criteria has been met */

/* Bit 1 : Status on whether PIN1 has met criteria set in PIN_CNF1.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN1_Pos (1UL) /*!< Position of PIN1 field. */
#define GPIO_LATCH_PIN1_Msk (0x1UL << GPIO_LATCH_PIN1_Pos) /*!< Bit mask of PIN1 field. */
#define GPIO_LATCH_PIN1_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN1_Latched (1UL) /*!< Criteria has been met */

/* Bit 0 : Status on whether PIN0 has met criteria set in PIN_CNF0.SENSE register. Write '1' to clear. */
#define GPIO_LATCH_PIN0_Pos (0UL) /*!< Position of PIN0 field. */
#define GPIO_LATCH_PIN0_Msk (0x1UL << GPIO_LATCH_PIN0_Pos) /*!< Bit mask of PIN0 field. */
#define GPIO_LATCH_PIN0_NotLatched (0UL) /*!< Criteria has not been met */
#define GPIO_LATCH_PIN0_Latched (1UL) /*!< Criteria has been met */

/* Register: GPIO_DETECTMODE */
/* Description: Select between default DETECT signal behaviour and LDETECT mode (For non-secure pin only) */

/* Bit 0 : Select between default DETECT signal behaviour and LDETECT mode */
#define GPIO_DETECTMODE_DETECTMODE_Pos (0UL) /*!< Position of DETECTMODE field. */
#define GPIO_DETECTMODE_DETECTMODE_Msk (0x1UL << GPIO_DETECTMODE_DETECTMODE_Pos) /*!< Bit mask of DETECTMODE field. */
#define GPIO_DETECTMODE_DETECTMODE_Default (0UL) /*!< DETECT directly connected to PIN DETECT signals */
#define GPIO_DETECTMODE_DETECTMODE_LDETECT (1UL) /*!< Use the latched LDETECT behaviour */

/* Register: GPIO_DETECTMODE_SEC */
/* Description: Select between default DETECT signal behaviour and LDETECT mode (For secure pin only) */

/* Bit 0 : Select between default DETECT signal behaviour and LDETECT mode */
#define GPIO_DETECTMODE_SEC_DETECTMODE_Pos (0UL) /*!< Position of DETECTMODE field. */
#define GPIO_DETECTMODE_SEC_DETECTMODE_Msk (0x1UL << GPIO_DETECTMODE_SEC_DETECTMODE_Pos) /*!< Bit mask of DETECTMODE field. */
#define GPIO_DETECTMODE_SEC_DETECTMODE_Default (0UL) /*!< DETECT directly connected to PIN DETECT signals */
#define GPIO_DETECTMODE_SEC_DETECTMODE_LDETECT (1UL) /*!< Use the latched LDETECT behaviour */

/* Register: GPIO_PIN_CNF */
/* Description: Description collection: Configuration of GPIO pins */

/* Bits 17..16 : Pin sensing mechanism */
#define GPIO_PIN_CNF_SENSE_Pos (16UL) /*!< Position of SENSE field. */
#define GPIO_PIN_CNF_SENSE_Msk (0x3UL << GPIO_PIN_CNF_SENSE_Pos) /*!< Bit mask of SENSE field. */
#define GPIO_PIN_CNF_SENSE_Disabled (0UL) /*!< Disabled */
#define GPIO_PIN_CNF_SENSE_High (2UL) /*!< Sense for high level */
#define GPIO_PIN_CNF_SENSE_Low (3UL) /*!< Sense for low level */

/* Bits 10..8 : Drive configuration */
#define GPIO_PIN_CNF_DRIVE_Pos (8UL) /*!< Position of DRIVE field. */
#define GPIO_PIN_CNF_DRIVE_Msk (0x7UL << GPIO_PIN_CNF_DRIVE_Pos) /*!< Bit mask of DRIVE field. */
#define GPIO_PIN_CNF_DRIVE_S0S1 (0UL) /*!< Standard '0', standard '1' */
#define GPIO_PIN_CNF_DRIVE_H0S1 (1UL) /*!< High drive '0', standard '1' */
#define GPIO_PIN_CNF_DRIVE_S0H1 (2UL) /*!< Standard '0', high drive '1' */
#define GPIO_PIN_CNF_DRIVE_H0H1 (3UL) /*!< High drive '0', high 'drive '1'' */
#define GPIO_PIN_CNF_DRIVE_D0S1 (4UL) /*!< Disconnect '0' standard '1' (normally used for wired-or connections) */
#define GPIO_PIN_CNF_DRIVE_D0H1 (5UL) /*!< Disconnect '0', high drive '1' (normally used for wired-or connections) */
#define GPIO_PIN_CNF_DRIVE_S0D1 (6UL) /*!< Standard '0'. disconnect '1' (normally used for wired-and connections) */
#define GPIO_PIN_CNF_DRIVE_H0D1 (7UL) /*!< High drive '0', disconnect '1' (normally used for wired-and connections) */

/* Bits 3..2 : Pull configuration */
#define GPIO_PIN_CNF_PULL_Pos (2UL) /*!< Position of PULL field. */
#define GPIO_PIN_CNF_PULL_Msk (0x3UL << GPIO_PIN_CNF_PULL_Pos) /*!< Bit mask of PULL field. */
#define GPIO_PIN_CNF_PULL_Disabled (0UL) /*!< No pull */
#define GPIO_PIN_CNF_PULL_Pulldown (1UL) /*!< Pull down on pin */
#define GPIO_PIN_CNF_PULL_Pullup (3UL) /*!< Pull up on pin */

/* Bit 1 : Connect or disconnect input buffer */
#define GPIO_PIN_CNF_INPUT_Pos (1UL) /*!< Position of INPUT field. */
#define GPIO_PIN_CNF_INPUT_Msk (0x1UL << GPIO_PIN_CNF_INPUT_Pos) /*!< Bit mask of INPUT field. */
#define GPIO_PIN_CNF_INPUT_Connect (0UL) /*!< Connect input buffer */
#define GPIO_PIN_CNF_INPUT_Disconnect (1UL) /*!< Disconnect input buffer */

/* Bit 0 : Pin direction. Same physical register as DIR register */
#define GPIO_PIN_CNF_DIR_Pos (0UL) /*!< Position of DIR field. */
#define GPIO_PIN_CNF_DIR_Msk (0x1UL << GPIO_PIN_CNF_DIR_Pos) /*!< Bit mask of DIR field. */
#define GPIO_PIN_CNF_DIR_Input (0UL) /*!< Configure pin as an input pin */
#define GPIO_PIN_CNF_DIR_Output (1UL) /*!< Configure pin as an output pin */


/* Peripheral: PDM */
/* Description: Pulse Density Modulation (Digital Microphone) Interface 0 */

/* Register: PDM_TASKS_START */
/* Description: Starts continuous PDM transfer */

/* Bit 0 : Starts continuous PDM transfer */
#define PDM_TASKS_START_TASKS_START_Pos (0UL) /*!< Position of TASKS_START field. */
#define PDM_TASKS_START_TASKS_START_Msk (0x1UL << PDM_TASKS_START_TASKS_START_Pos) /*!< Bit mask of TASKS_START field. */
#define PDM_TASKS_START_TASKS_START_Trigger (1UL) /*!< Trigger task */

/* Register: PDM_TASKS_STOP */
/* Description: Stops PDM transfer */

/* Bit 0 : Stops PDM transfer */
#define PDM_TASKS_STOP_TASKS_STOP_Pos (0UL) /*!< Position of TASKS_STOP field. */
#define PDM_TASKS_STOP_TASKS_STOP_Msk (0x1UL << PDM_TASKS_STOP_TASKS_STOP_Pos) /*!< Bit mask of TASKS_STOP field. */
#define PDM_TASKS_STOP_TASKS_STOP_Trigger (1UL) /*!< Trigger task */

/* Register: PDM_SUBSCRIBE_START */
/* Description: Subscribe configuration for task START */

/* Bit 31 :   */
#define PDM_SUBSCRIBE_START_EN_Pos (31UL) /*!< Position of EN field. */
#define PDM_SUBSCRIBE_START_EN_Msk (0x1UL << PDM_SUBSCRIBE_START_EN_Pos) /*!< Bit mask of EN field. */
#define PDM_SUBSCRIBE_START_EN_Disabled (0UL) /*!< Disable subscription */
#define PDM_SUBSCRIBE_START_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task START will subscribe to */
#define PDM_SUBSCRIBE_START_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define PDM_SUBSCRIBE_START_CHIDX_Msk (0xFUL << PDM_SUBSCRIBE_START_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: PDM_SUBSCRIBE_STOP */
/* Description: Subscribe configuration for task STOP */

/* Bit 31 :   */
#define PDM_SUBSCRIBE_STOP_EN_Pos (31UL) /*!< Position of EN field. */
#define PDM_SUBSCRIBE_STOP_EN_Msk (0x1UL << PDM_SUBSCRIBE_STOP_EN_Pos) /*!< Bit mask of EN field. */
#define PDM_SUBSCRIBE_STOP_EN_Disabled (0UL) /*!< Disable subscription */
#define PDM_SUBSCRIBE_STOP_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task STOP will subscribe to */
#define PDM_SUBSCRIBE_STOP_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define PDM_SUBSCRIBE_STOP_CHIDX_Msk (0xFUL << PDM_SUBSCRIBE_STOP_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: PDM_EVENTS_STARTED */
/* Description: PDM transfer has started */

/* Bit 0 : PDM transfer has started */
#define PDM_EVENTS_STARTED_EVENTS_STARTED_Pos (0UL) /*!< Position of EVENTS_STARTED field. */
#define PDM_EVENTS_STARTED_EVENTS_STARTED_Msk (0x1UL << PDM_EVENTS_STARTED_EVENTS_STARTED_Pos) /*!< Bit mask of EVENTS_STARTED field. */
#define PDM_EVENTS_STARTED_EVENTS_STARTED_NotGenerated (0UL) /*!< Event not generated */
#define PDM_EVENTS_STARTED_EVENTS_STARTED_Generated (1UL) /*!< Event generated */

/* Register: PDM_EVENTS_STOPPED */
/* Description: PDM transfer has finished */

/* Bit 0 : PDM transfer has finished */
#define PDM_EVENTS_STOPPED_EVENTS_STOPPED_Pos (0UL) /*!< Position of EVENTS_STOPPED field. */
#define PDM_EVENTS_STOPPED_EVENTS_STOPPED_Msk (0x1UL << PDM_EVENTS_STOPPED_EVENTS_STOPPED_Pos) /*!< Bit mask of EVENTS_STOPPED field. */
#define PDM_EVENTS_STOPPED_EVENTS_STOPPED_NotGenerated (0UL) /*!< Event not generated */
#define PDM_EVENTS_STOPPED_EVENTS_STOPPED_Generated (1UL) /*!< Event generated */

/* Register: PDM_EVENTS_END */
/* Description: The PDM has written the last sample specified by SAMPLE.MAXCNT (or the last sample after a STOP task has been received) to Data RAM */

/* Bit 0 : The PDM has written the last sample specified by SAMPLE.MAXCNT (or the last sample after a STOP task has been received) to Data RAM */
#define PDM_EVENTS_END_EVENTS_END_Pos (0UL) /*!< Position of EVENTS_END field. */
#define PDM_EVENTS_END_EVENTS_END_Msk (0x1UL << PDM_EVENTS_END_EVENTS_END_Pos) /*!< Bit mask of EVENTS_END field. */
#define PDM_EVENTS_END_EVENTS_END_NotGenerated (0UL) /*!< Event not generated */
#define PDM_EVENTS_END_EVENTS_END_Generated (1UL) /*!< Event generated */

/* Register: PDM_PUBLISH_STARTED */
/* Description: Publish configuration for event STARTED */

/* Bit 31 :   */
#define PDM_PUBLISH_STARTED_EN_Pos (31UL) /*!< Position of EN field. */
#define PDM_PUBLISH_STARTED_EN_Msk (0x1UL << PDM_PUBLISH_STARTED_EN_Pos) /*!< Bit mask of EN field. */
#define PDM_PUBLISH_STARTED_EN_Disabled (0UL) /*!< Disable publishing */
#define PDM_PUBLISH_STARTED_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event STARTED will publish to. */
#define PDM_PUBLISH_STARTED_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define PDM_PUBLISH_STARTED_CHIDX_Msk (0xFUL << PDM_PUBLISH_STARTED_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: PDM_PUBLISH_STOPPED */
/* Description: Publish configuration for event STOPPED */

/* Bit 31 :   */
#define PDM_PUBLISH_STOPPED_EN_Pos (31UL) /*!< Position of EN field. */
#define PDM_PUBLISH_STOPPED_EN_Msk (0x1UL << PDM_PUBLISH_STOPPED_EN_Pos) /*!< Bit mask of EN field. */
#define PDM_PUBLISH_STOPPED_EN_Disabled (0UL) /*!< Disable publishing */
#define PDM_PUBLISH_STOPPED_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event STOPPED will publish to. */
#define PDM_PUBLISH_STOPPED_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define PDM_PUBLISH_STOPPED_CHIDX_Msk (0xFUL << PDM_PUBLISH_STOPPED_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: PDM_PUBLISH_END */
/* Description: Publish configuration for event END */

/* Bit 31 :   */
#define PDM_PUBLISH_END_EN_Pos (31UL) /*!< Position of EN field. */
#define PDM_PUBLISH_END_EN_Msk (0x1UL << PDM_PUBLISH_END_EN_Pos) /*!< Bit mask of EN field. */
#define PDM_PUBLISH_END_EN_Disabled (0UL) /*!< Disable publishing */
#define PDM_PUBLISH_END_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event END will publish to. */
#define PDM_PUBLISH_END_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define PDM_PUBLISH_END_CHIDX_Msk (0xFUL << PDM_PUBLISH_END_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: PDM_INTEN */
/* Description: Enable or disable interrupt */

/* Bit 2 : Enable or disable interrupt for event END */
#define PDM_INTEN_END_Pos (2UL) /*!< Position of END field. */
#define PDM_INTEN_END_Msk (0x1UL << PDM_INTEN_END_Pos) /*!< Bit mask of END field. */
#define PDM_INTEN_END_Disabled (0UL) /*!< Disable */
#define PDM_INTEN_END_Enabled (1UL) /*!< Enable */

/* Bit 1 : Enable or disable interrupt for event STOPPED */
#define PDM_INTEN_STOPPED_Pos (1UL) /*!< Position of STOPPED field. */
#define PDM_INTEN_STOPPED_Msk (0x1UL << PDM_INTEN_STOPPED_Pos) /*!< Bit mask of STOPPED field. */
#define PDM_INTEN_STOPPED_Disabled (0UL) /*!< Disable */
#define PDM_INTEN_STOPPED_Enabled (1UL) /*!< Enable */

/* Bit 0 : Enable or disable interrupt for event STARTED */
#define PDM_INTEN_STARTED_Pos (0UL) /*!< Position of STARTED field. */
#define PDM_INTEN_STARTED_Msk (0x1UL << PDM_INTEN_STARTED_Pos) /*!< Bit mask of STARTED field. */
#define PDM_INTEN_STARTED_Disabled (0UL) /*!< Disable */
#define PDM_INTEN_STARTED_Enabled (1UL) /*!< Enable */

/* Register: PDM_INTENSET */
/* Description: Enable interrupt */

/* Bit 2 : Write '1' to enable interrupt for event END */
#define PDM_INTENSET_END_Pos (2UL) /*!< Position of END field. */
#define PDM_INTENSET_END_Msk (0x1UL << PDM_INTENSET_END_Pos) /*!< Bit mask of END field. */
#define PDM_INTENSET_END_Disabled (0UL) /*!< Read: Disabled */
#define PDM_INTENSET_END_Enabled (1UL) /*!< Read: Enabled */
#define PDM_INTENSET_END_Set (1UL) /*!< Enable */

/* Bit 1 : Write '1' to enable interrupt for event STOPPED */
#define PDM_INTENSET_STOPPED_Pos (1UL) /*!< Position of STOPPED field. */
#define PDM_INTENSET_STOPPED_Msk (0x1UL << PDM_INTENSET_STOPPED_Pos) /*!< Bit mask of STOPPED field. */
#define PDM_INTENSET_STOPPED_Disabled (0UL) /*!< Read: Disabled */
#define PDM_INTENSET_STOPPED_Enabled (1UL) /*!< Read: Enabled */
#define PDM_INTENSET_STOPPED_Set (1UL) /*!< Enable */

/* Bit 0 : Write '1' to enable interrupt for event STARTED */
#define PDM_INTENSET_STARTED_Pos (0UL) /*!< Position of STARTED field. */
#define PDM_INTENSET_STARTED_Msk (0x1UL << PDM_INTENSET_STARTED_Pos) /*!< Bit mask of STARTED field. */
#define PDM_INTENSET_STARTED_Disabled (0UL) /*!< Read: Disabled */
#define PDM_INTENSET_STARTED_Enabled (1UL) /*!< Read: Enabled */
#define PDM_INTENSET_STARTED_Set (1UL) /*!< Enable */

/* Register: PDM_INTENCLR */
/* Description: Disable interrupt */

/* Bit 2 : Write '1' to disable interrupt for event END */
#define PDM_INTENCLR_END_Pos (2UL) /*!< Position of END field. */
#define PDM_INTENCLR_END_Msk (0x1UL << PDM_INTENCLR_END_Pos) /*!< Bit mask of END field. */
#define PDM_INTENCLR_END_Disabled (0UL) /*!< Read: Disabled */
#define PDM_INTENCLR_END_Enabled (1UL) /*!< Read: Enabled */
#define PDM_INTENCLR_END_Clear (1UL) /*!< Disable */

/* Bit 1 : Write '1' to disable interrupt for event STOPPED */
#define PDM_INTENCLR_STOPPED_Pos (1UL) /*!< Position of STOPPED field. */
#define PDM_INTENCLR_STOPPED_Msk (0x1UL << PDM_INTENCLR_STOPPED_Pos) /*!< Bit mask of STOPPED field. */
#define PDM_INTENCLR_STOPPED_Disabled (0UL) /*!< Read: Disabled */
#define PDM_INTENCLR_STOPPED_Enabled (1UL) /*!< Read: Enabled */
#define PDM_INTENCLR_STOPPED_Clear (1UL) /*!< Disable */

/* Bit 0 : Write '1' to disable interrupt for event STARTED */
#define PDM_INTENCLR_STARTED_Pos (0UL) /*!< Position of STARTED field. */
#define PDM_INTENCLR_STARTED_Msk (0x1UL << PDM_INTENCLR_STARTED_Pos) /*!< Bit mask of STARTED field. */
#define PDM_INTENCLR_STARTED_Disabled (0UL) /*!< Read: Disabled */
#define PDM_INTENCLR_STARTED_Enabled (1UL) /*!< Read: Enabled */
#define PDM_INTENCLR_STARTED_Clear (1UL) /*!< Disable */

/* Register: PDM_ENABLE */
/* Description: PDM module enable register */

/* Bit 0 : Enable or disable PDM module */
#define PDM_ENABLE_ENABLE_Pos (0UL) /*!< Position of ENABLE field. */
#define PDM_ENABLE_ENABLE_Msk (0x1UL << PDM_ENABLE_ENABLE_Pos) /*!< Bit mask of ENABLE field. */
#define PDM_ENABLE_ENABLE_Disabled (0UL) /*!< Disable */
#define PDM_ENABLE_ENABLE_Enabled (1UL) /*!< Enable */

/* Register: PDM_PDMCLKCTRL */
/* Description: PDM clock generator control */

/* Bits 31..0 : PDM_CLK frequency */
#define PDM_PDMCLKCTRL_FREQ_Pos (0UL) /*!< Position of FREQ field. */
#define PDM_PDMCLKCTRL_FREQ_Msk (0xFFFFFFFFUL << PDM_PDMCLKCTRL_FREQ_Pos) /*!< Bit mask of FREQ field. */
#define PDM_PDMCLKCTRL_FREQ_1000K (0x08000000UL) /*!< PDM_CLK = 32 MHz / 32 = 1.000 MHz */
#define PDM_PDMCLKCTRL_FREQ_Default (0x08400000UL) /*!< PDM_CLK = 32 MHz / 31 = 1.032 MHz. Nominal clock for RATIO=Ratio64. */
#define PDM_PDMCLKCTRL_FREQ_1067K (0x08800000UL) /*!< PDM_CLK = 32 MHz / 30 = 1.067 MHz */
#define PDM_PDMCLKCTRL_FREQ_1231K (0x09800000UL) /*!< PDM_CLK = 32 MHz / 26 = 1.231 MHz */
#define PDM_PDMCLKCTRL_FREQ_1280K (0x0A000000UL) /*!< PDM_CLK = 32 MHz / 25 = 1.280 MHz. Nominal clock for RATIO=Ratio80. */
#define PDM_PDMCLKCTRL_FREQ_1333K (0x0A800000UL) /*!< PDM_CLK = 32 MHz / 24 = 1.333 MHz */

/* Register: PDM_MODE */
/* Description: Defines the routing of the connected PDM microphones' signals */

/* Bit 1 : Defines on which PDM_CLK edge Left (or mono) is sampled */
#define PDM_MODE_EDGE_Pos (1UL) /*!< Position of EDGE field. */
#define PDM_MODE_EDGE_Msk (0x1UL << PDM_MODE_EDGE_Pos) /*!< Bit mask of EDGE field. */
#define PDM_MODE_EDGE_LeftFalling (0UL) /*!< Left (or mono) is sampled on falling edge of PDM_CLK */
#define PDM_MODE_EDGE_LeftRising (1UL) /*!< Left (or mono) is sampled on rising edge of PDM_CLK */

/* Bit 0 : Mono or stereo operation */
#define PDM_MODE_OPERATION_Pos (0UL) /*!< Position of OPERATION field. */
#define PDM_MODE_OPERATION_Msk (0x1UL << PDM_MODE_OPERATION_Pos) /*!< Bit mask of OPERATION field. */
#define PDM_MODE_OPERATION_Stereo (0UL) /*!< Sample and store one pair (Left + Right) of 16bit samples per RAM word R=[31:16]; L=[15:0] */
#define PDM_MODE_OPERATION_Mono (1UL) /*!< Sample and store two successive Left samples (16 bit each) per RAM word L1=[31:16]; L0=[15:0] */

/* Register: PDM_GAINL */
/* Description: Left output gain adjustment */

/* Bits 6..0 : Left output gain adjustment, in 0.5 dB steps, around the default module gain (see electrical parameters) 0x00    -20 dB gain adjust 0x01  -19.5 dB gain adjust (...) 0x27   -0.5 dB gain adjust 0x28      0 dB gain adjust 0x29   +0.5 dB gain adjust (...) 0x4F  +19.5 dB gain adjust 0x50    +20 dB gain adjust */
#define PDM_GAINL_GAINL_Pos (0UL) /*!< Position of GAINL field. */
#define PDM_GAINL_GAINL_Msk (0x7FUL << PDM_GAINL_GAINL_Pos) /*!< Bit mask of GAINL field. */
#define PDM_GAINL_GAINL_MinGain (0x00UL) /*!< -20dB gain adjustment (minimum) */
#define PDM_GAINL_GAINL_DefaultGain (0x28UL) /*!< 0dB gain adjustment */
#define PDM_GAINL_GAINL_MaxGain (0x50UL) /*!< +20dB gain adjustment (maximum) */

/* Register: PDM_GAINR */
/* Description: Right output gain adjustment */

/* Bits 6..0 : Right output gain adjustment, in 0.5 dB steps, around the default module gain (see electrical parameters) */
#define PDM_GAINR_GAINR_Pos (0UL) /*!< Position of GAINR field. */
#define PDM_GAINR_GAINR_Msk (0x7FUL << PDM_GAINR_GAINR_Pos) /*!< Bit mask of GAINR field. */
#define PDM_GAINR_GAINR_MinGain (0x00UL) /*!< -20dB gain adjustment (minimum) */
#define PDM_GAINR_GAINR_DefaultGain (0x28UL) /*!< 0dB gain adjustment */
#define PDM_GAINR_GAINR_MaxGain (0x50UL) /*!< +20dB gain adjustment (maximum) */

/* Register: PDM_RATIO */
/* Description: Selects the ratio between PDM_CLK and output sample rate. Change PDMCLKCTRL accordingly. */

/* Bit 0 : Selects the ratio between PDM_CLK and output sample rate */
#define PDM_RATIO_RATIO_Pos (0UL) /*!< Position of RATIO field. */
#define PDM_RATIO_RATIO_Msk (0x1UL << PDM_RATIO_RATIO_Pos) /*!< Bit mask of RATIO field. */
#define PDM_RATIO_RATIO_Ratio64 (0UL) /*!< Ratio of 64 */
#define PDM_RATIO_RATIO_Ratio80 (1UL) /*!< Ratio of 80 */

/* Register: PDM_PSEL_CLK */
/* Description: Pin number configuration for PDM CLK signal */

/* Bit 31 : Connection */
#define PDM_PSEL_CLK_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define PDM_PSEL_CLK_CONNECT_Msk (0x1UL << PDM_PSEL_CLK_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define PDM_PSEL_CLK_CONNECT_Connected (0UL) /*!< Connect */
#define PDM_PSEL_CLK_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define PDM_PSEL_CLK_PIN_Pos (0UL) /*!< Position of PIN field. */
#define PDM_PSEL_CLK_PIN_Msk (0x1FUL << PDM_PSEL_CLK_PIN_Pos) /*!< Bit mask of PIN field. */

/* Register: PDM_PSEL_DIN */
/* Description: Pin number configuration for PDM DIN signal */

/* Bit 31 : Connection */
#define PDM_PSEL_DIN_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define PDM_PSEL_DIN_CONNECT_Msk (0x1UL << PDM_PSEL_DIN_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define PDM_PSEL_DIN_CONNECT_Connected (0UL) /*!< Connect */
#define PDM_PSEL_DIN_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define PDM_PSEL_DIN_PIN_Pos (0UL) /*!< Position of PIN field. */
#define PDM_PSEL_DIN_PIN_Msk (0x1FUL << PDM_PSEL_DIN_PIN_Pos) /*!< Bit mask of PIN field. */

/* Register: PDM_SAMPLE_PTR */
/* Description: RAM address pointer to write samples to with EasyDMA */

/* Bits 31..0 : Address to write PDM samples to over DMA */
#define PDM_SAMPLE_PTR_SAMPLEPTR_Pos (0UL) /*!< Position of SAMPLEPTR field. */
#define PDM_SAMPLE_PTR_SAMPLEPTR_Msk (0xFFFFFFFFUL << PDM_SAMPLE_PTR_SAMPLEPTR_Pos) /*!< Bit mask of SAMPLEPTR field. */

/* Register: PDM_SAMPLE_MAXCNT */
/* Description: Number of samples to allocate memory for in EasyDMA mode */

/* Bits 14..0 : Length of DMA RAM allocation in number of samples */
#define PDM_SAMPLE_MAXCNT_BUFFSIZE_Pos (0UL) /*!< Position of BUFFSIZE field. */
#define PDM_SAMPLE_MAXCNT_BUFFSIZE_Msk (0x7FFFUL << PDM_SAMPLE_MAXCNT_BUFFSIZE_Pos) /*!< Bit mask of BUFFSIZE field. */


/* Peripheral: POWER */
/* Description: Power control 0 */

/* Register: POWER_TASKS_CONSTLAT */
/* Description: Enable constant latency mode. */

/* Bit 0 : Enable constant latency mode. */
#define POWER_TASKS_CONSTLAT_TASKS_CONSTLAT_Pos (0UL) /*!< Position of TASKS_CONSTLAT field. */
#define POWER_TASKS_CONSTLAT_TASKS_CONSTLAT_Msk (0x1UL << POWER_TASKS_CONSTLAT_TASKS_CONSTLAT_Pos) /*!< Bit mask of TASKS_CONSTLAT field. */
#define POWER_TASKS_CONSTLAT_TASKS_CONSTLAT_Trigger (1UL) /*!< Trigger task */

/* Register: POWER_TASKS_LOWPWR */
/* Description: Enable low power mode (variable latency) */

/* Bit 0 : Enable low power mode (variable latency) */
#define POWER_TASKS_LOWPWR_TASKS_LOWPWR_Pos (0UL) /*!< Position of TASKS_LOWPWR field. */
#define POWER_TASKS_LOWPWR_TASKS_LOWPWR_Msk (0x1UL << POWER_TASKS_LOWPWR_TASKS_LOWPWR_Pos) /*!< Bit mask of TASKS_LOWPWR field. */
#define POWER_TASKS_LOWPWR_TASKS_LOWPWR_Trigger (1UL) /*!< Trigger task */

/* Register: POWER_SUBSCRIBE_CONSTLAT */
/* Description: Subscribe configuration for task CONSTLAT */

/* Bit 31 :   */
#define POWER_SUBSCRIBE_CONSTLAT_EN_Pos (31UL) /*!< Position of EN field. */
#define POWER_SUBSCRIBE_CONSTLAT_EN_Msk (0x1UL << POWER_SUBSCRIBE_CONSTLAT_EN_Pos) /*!< Bit mask of EN field. */
#define POWER_SUBSCRIBE_CONSTLAT_EN_Disabled (0UL) /*!< Disable subscription */
#define POWER_SUBSCRIBE_CONSTLAT_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task CONSTLAT will subscribe to */
#define POWER_SUBSCRIBE_CONSTLAT_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define POWER_SUBSCRIBE_CONSTLAT_CHIDX_Msk (0xFUL << POWER_SUBSCRIBE_CONSTLAT_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: POWER_SUBSCRIBE_LOWPWR */
/* Description: Subscribe configuration for task LOWPWR */

/* Bit 31 :   */
#define POWER_SUBSCRIBE_LOWPWR_EN_Pos (31UL) /*!< Position of EN field. */
#define POWER_SUBSCRIBE_LOWPWR_EN_Msk (0x1UL << POWER_SUBSCRIBE_LOWPWR_EN_Pos) /*!< Bit mask of EN field. */
#define POWER_SUBSCRIBE_LOWPWR_EN_Disabled (0UL) /*!< Disable subscription */
#define POWER_SUBSCRIBE_LOWPWR_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task LOWPWR will subscribe to */
#define POWER_SUBSCRIBE_LOWPWR_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define POWER_SUBSCRIBE_LOWPWR_CHIDX_Msk (0xFUL << POWER_SUBSCRIBE_LOWPWR_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: POWER_EVENTS_POFWARN */
/* Description: Power failure warning */

/* Bit 0 : Power failure warning */
#define POWER_EVENTS_POFWARN_EVENTS_POFWARN_Pos (0UL) /*!< Position of EVENTS_POFWARN field. */
#define POWER_EVENTS_POFWARN_EVENTS_POFWARN_Msk (0x1UL << POWER_EVENTS_POFWARN_EVENTS_POFWARN_Pos) /*!< Bit mask of EVENTS_POFWARN field. */
#define POWER_EVENTS_POFWARN_EVENTS_POFWARN_NotGenerated (0UL) /*!< Event not generated */
#define POWER_EVENTS_POFWARN_EVENTS_POFWARN_Generated (1UL) /*!< Event generated */

/* Register: POWER_EVENTS_SLEEPENTER */
/* Description: CPU entered WFI/WFE sleep */

/* Bit 0 : CPU entered WFI/WFE sleep */
#define POWER_EVENTS_SLEEPENTER_EVENTS_SLEEPENTER_Pos (0UL) /*!< Position of EVENTS_SLEEPENTER field. */
#define POWER_EVENTS_SLEEPENTER_EVENTS_SLEEPENTER_Msk (0x1UL << POWER_EVENTS_SLEEPENTER_EVENTS_SLEEPENTER_Pos) /*!< Bit mask of EVENTS_SLEEPENTER field. */
#define POWER_EVENTS_SLEEPENTER_EVENTS_SLEEPENTER_NotGenerated (0UL) /*!< Event not generated */
#define POWER_EVENTS_SLEEPENTER_EVENTS_SLEEPENTER_Generated (1UL) /*!< Event generated */

/* Register: POWER_EVENTS_SLEEPEXIT */
/* Description: CPU exited WFI/WFE sleep */

/* Bit 0 : CPU exited WFI/WFE sleep */
#define POWER_EVENTS_SLEEPEXIT_EVENTS_SLEEPEXIT_Pos (0UL) /*!< Position of EVENTS_SLEEPEXIT field. */
#define POWER_EVENTS_SLEEPEXIT_EVENTS_SLEEPEXIT_Msk (0x1UL << POWER_EVENTS_SLEEPEXIT_EVENTS_SLEEPEXIT_Pos) /*!< Bit mask of EVENTS_SLEEPEXIT field. */
#define POWER_EVENTS_SLEEPEXIT_EVENTS_SLEEPEXIT_NotGenerated (0UL) /*!< Event not generated */
#define POWER_EVENTS_SLEEPEXIT_EVENTS_SLEEPEXIT_Generated (1UL) /*!< Event generated */

/* Register: POWER_PUBLISH_POFWARN */
/* Description: Publish configuration for event POFWARN */

/* Bit 31 :   */
#define POWER_PUBLISH_POFWARN_EN_Pos (31UL) /*!< Position of EN field. */
#define POWER_PUBLISH_POFWARN_EN_Msk (0x1UL << POWER_PUBLISH_POFWARN_EN_Pos) /*!< Bit mask of EN field. */
#define POWER_PUBLISH_POFWARN_EN_Disabled (0UL) /*!< Disable publishing */
#define POWER_PUBLISH_POFWARN_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event POFWARN will publish to. */
#define POWER_PUBLISH_POFWARN_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define POWER_PUBLISH_POFWARN_CHIDX_Msk (0xFUL << POWER_PUBLISH_POFWARN_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: POWER_PUBLISH_SLEEPENTER */
/* Description: Publish configuration for event SLEEPENTER */

/* Bit 31 :   */
#define POWER_PUBLISH_SLEEPENTER_EN_Pos (31UL) /*!< Position of EN field. */
#define POWER_PUBLISH_SLEEPENTER_EN_Msk (0x1UL << POWER_PUBLISH_SLEEPENTER_EN_Pos) /*!< Bit mask of EN field. */
#define POWER_PUBLISH_SLEEPENTER_EN_Disabled (0UL) /*!< Disable publishing */
#define POWER_PUBLISH_SLEEPENTER_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event SLEEPENTER will publish to. */
#define POWER_PUBLISH_SLEEPENTER_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define POWER_PUBLISH_SLEEPENTER_CHIDX_Msk (0xFUL << POWER_PUBLISH_SLEEPENTER_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: POWER_PUBLISH_SLEEPEXIT */
/* Description: Publish configuration for event SLEEPEXIT */

/* Bit 31 :   */
#define POWER_PUBLISH_SLEEPEXIT_EN_Pos (31UL) /*!< Position of EN field. */
#define POWER_PUBLISH_SLEEPEXIT_EN_Msk (0x1UL << POWER_PUBLISH_SLEEPEXIT_EN_Pos) /*!< Bit mask of EN field. */
#define POWER_PUBLISH_SLEEPEXIT_EN_Disabled (0UL) /*!< Disable publishing */
#define POWER_PUBLISH_SLEEPEXIT_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event SLEEPEXIT will publish to. */
#define POWER_PUBLISH_SLEEPEXIT_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define POWER_PUBLISH_SLEEPEXIT_CHIDX_Msk (0xFUL << POWER_PUBLISH_SLEEPEXIT_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: POWER_INTEN */
/* Description: Enable or disable interrupt */

/* Bit 6 : Enable or disable interrupt for event SLEEPEXIT */
#define POWER_INTEN_SLEEPEXIT_Pos (6UL) /*!< Position of SLEEPEXIT field. */
#define POWER_INTEN_SLEEPEXIT_Msk (0x1UL << POWER_INTEN_SLEEPEXIT_Pos) /*!< Bit mask of SLEEPEXIT field. */
#define POWER_INTEN_SLEEPEXIT_Disabled (0UL) /*!< Disable */
#define POWER_INTEN_SLEEPEXIT_Enabled (1UL) /*!< Enable */

/* Bit 5 : Enable or disable interrupt for event SLEEPENTER */
#define POWER_INTEN_SLEEPENTER_Pos (5UL) /*!< Position of SLEEPENTER field. */
#define POWER_INTEN_SLEEPENTER_Msk (0x1UL << POWER_INTEN_SLEEPENTER_Pos) /*!< Bit mask of SLEEPENTER field. */
#define POWER_INTEN_SLEEPENTER_Disabled (0UL) /*!< Disable */
#define POWER_INTEN_SLEEPENTER_Enabled (1UL) /*!< Enable */

/* Bit 2 : Enable or disable interrupt for event POFWARN */
#define POWER_INTEN_POFWARN_Pos (2UL) /*!< Position of POFWARN field. */
#define POWER_INTEN_POFWARN_Msk (0x1UL << POWER_INTEN_POFWARN_Pos) /*!< Bit mask of POFWARN field. */
#define POWER_INTEN_POFWARN_Disabled (0UL) /*!< Disable */
#define POWER_INTEN_POFWARN_Enabled (1UL) /*!< Enable */

/* Register: POWER_INTENSET */
/* Description: Enable interrupt */

/* Bit 6 : Write '1' to enable interrupt for event SLEEPEXIT */
#define POWER_INTENSET_SLEEPEXIT_Pos (6UL) /*!< Position of SLEEPEXIT field. */
#define POWER_INTENSET_SLEEPEXIT_Msk (0x1UL << POWER_INTENSET_SLEEPEXIT_Pos) /*!< Bit mask of SLEEPEXIT field. */
#define POWER_INTENSET_SLEEPEXIT_Disabled (0UL) /*!< Read: Disabled */
#define POWER_INTENSET_SLEEPEXIT_Enabled (1UL) /*!< Read: Enabled */
#define POWER_INTENSET_SLEEPEXIT_Set (1UL) /*!< Enable */

/* Bit 5 : Write '1' to enable interrupt for event SLEEPENTER */
#define POWER_INTENSET_SLEEPENTER_Pos (5UL) /*!< Position of SLEEPENTER field. */
#define POWER_INTENSET_SLEEPENTER_Msk (0x1UL << POWER_INTENSET_SLEEPENTER_Pos) /*!< Bit mask of SLEEPENTER field. */
#define POWER_INTENSET_SLEEPENTER_Disabled (0UL) /*!< Read: Disabled */
#define POWER_INTENSET_SLEEPENTER_Enabled (1UL) /*!< Read: Enabled */
#define POWER_INTENSET_SLEEPENTER_Set (1UL) /*!< Enable */

/* Bit 2 : Write '1' to enable interrupt for event POFWARN */
#define POWER_INTENSET_POFWARN_Pos (2UL) /*!< Position of POFWARN field. */
#define POWER_INTENSET_POFWARN_Msk (0x1UL << POWER_INTENSET_POFWARN_Pos) /*!< Bit mask of POFWARN field. */
#define POWER_INTENSET_POFWARN_Disabled (0UL) /*!< Read: Disabled */
#define POWER_INTENSET_POFWARN_Enabled (1UL) /*!< Read: Enabled */
#define POWER_INTENSET_POFWARN_Set (1UL) /*!< Enable */

/* Register: POWER_INTENCLR */
/* Description: Disable interrupt */

/* Bit 6 : Write '1' to disable interrupt for event SLEEPEXIT */
#define POWER_INTENCLR_SLEEPEXIT_Pos (6UL) /*!< Position of SLEEPEXIT field. */
#define POWER_INTENCLR_SLEEPEXIT_Msk (0x1UL << POWER_INTENCLR_SLEEPEXIT_Pos) /*!< Bit mask of SLEEPEXIT field. */
#define POWER_INTENCLR_SLEEPEXIT_Disabled (0UL) /*!< Read: Disabled */
#define POWER_INTENCLR_SLEEPEXIT_Enabled (1UL) /*!< Read: Enabled */
#define POWER_INTENCLR_SLEEPEXIT_Clear (1UL) /*!< Disable */

/* Bit 5 : Write '1' to disable interrupt for event SLEEPENTER */
#define POWER_INTENCLR_SLEEPENTER_Pos (5UL) /*!< Position of SLEEPENTER field. */
#define POWER_INTENCLR_SLEEPENTER_Msk (0x1UL << POWER_INTENCLR_SLEEPENTER_Pos) /*!< Bit mask of SLEEPENTER field. */
#define POWER_INTENCLR_SLEEPENTER_Disabled (0UL) /*!< Read: Disabled */
#define POWER_INTENCLR_SLEEPENTER_Enabled (1UL) /*!< Read: Enabled */
#define POWER_INTENCLR_SLEEPENTER_Clear (1UL) /*!< Disable */

/* Bit 2 : Write '1' to disable interrupt for event POFWARN */
#define POWER_INTENCLR_POFWARN_Pos (2UL) /*!< Position of POFWARN field. */
#define POWER_INTENCLR_POFWARN_Msk (0x1UL << POWER_INTENCLR_POFWARN_Pos) /*!< Bit mask of POFWARN field. */
#define POWER_INTENCLR_POFWARN_Disabled (0UL) /*!< Read: Disabled */
#define POWER_INTENCLR_POFWARN_Enabled (1UL) /*!< Read: Enabled */
#define POWER_INTENCLR_POFWARN_Clear (1UL) /*!< Disable */

/* Register: POWER_RESETREAS */
/* Description: Reset reason */

/* Bit 18 : Reset triggered through CTRL-AP */
#define POWER_RESETREAS_CTRLAP_Pos (18UL) /*!< Position of CTRLAP field. */
#define POWER_RESETREAS_CTRLAP_Msk (0x1UL << POWER_RESETREAS_CTRLAP_Pos) /*!< Bit mask of CTRLAP field. */
#define POWER_RESETREAS_CTRLAP_NotDetected (0UL) /*!< Not detected */
#define POWER_RESETREAS_CTRLAP_Detected (1UL) /*!< Detected */

/* Bit 17 : Reset from CPU lock-up detected */
#define POWER_RESETREAS_LOCKUP_Pos (17UL) /*!< Position of LOCKUP field. */
#define POWER_RESETREAS_LOCKUP_Msk (0x1UL << POWER_RESETREAS_LOCKUP_Pos) /*!< Bit mask of LOCKUP field. */
#define POWER_RESETREAS_LOCKUP_NotDetected (0UL) /*!< Not detected */
#define POWER_RESETREAS_LOCKUP_Detected (1UL) /*!< Detected */

/* Bit 16 : Reset from AIRCR.SYSRESETREQ detected */
#define POWER_RESETREAS_SREQ_Pos (16UL) /*!< Position of SREQ field. */
#define POWER_RESETREAS_SREQ_Msk (0x1UL << POWER_RESETREAS_SREQ_Pos) /*!< Bit mask of SREQ field. */
#define POWER_RESETREAS_SREQ_NotDetected (0UL) /*!< Not detected */
#define POWER_RESETREAS_SREQ_Detected (1UL) /*!< Detected */

/* Bit 4 : Reset due to wakeup from System OFF mode, when wakeup is triggered by entering debug interface mode */
#define POWER_RESETREAS_DIF_Pos (4UL) /*!< Position of DIF field. */
#define POWER_RESETREAS_DIF_Msk (0x1UL << POWER_RESETREAS_DIF_Pos) /*!< Bit mask of DIF field. */
#define POWER_RESETREAS_DIF_NotDetected (0UL) /*!< Not detected */
#define POWER_RESETREAS_DIF_Detected (1UL) /*!< Detected */

/* Bit 2 : Reset due to wakeup from System OFF mode, when wakeup is triggered by DETECT signal from GPIO */
#define POWER_RESETREAS_OFF_Pos (2UL) /*!< Position of OFF field. */
#define POWER_RESETREAS_OFF_Msk (0x1UL << POWER_RESETREAS_OFF_Pos) /*!< Bit mask of OFF field. */
#define POWER_RESETREAS_OFF_NotDetected (0UL) /*!< Not detected */
#define POWER_RESETREAS_OFF_Detected (1UL) /*!< Detected */

/* Bit 1 : Reset from global watchdog detected */
#define POWER_RESETREAS_DOG_Pos (1UL) /*!< Position of DOG field. */
#define POWER_RESETREAS_DOG_Msk (0x1UL << POWER_RESETREAS_DOG_Pos) /*!< Bit mask of DOG field. */
#define POWER_RESETREAS_DOG_NotDetected (0UL) /*!< Not detected */
#define POWER_RESETREAS_DOG_Detected (1UL) /*!< Detected */

/* Bit 0 : Reset from pin reset detected */
#define POWER_RESETREAS_RESETPIN_Pos (0UL) /*!< Position of RESETPIN field. */
#define POWER_RESETREAS_RESETPIN_Msk (0x1UL << POWER_RESETREAS_RESETPIN_Pos) /*!< Bit mask of RESETPIN field. */
#define POWER_RESETREAS_RESETPIN_NotDetected (0UL) /*!< Not detected */
#define POWER_RESETREAS_RESETPIN_Detected (1UL) /*!< Detected */

/* Register: POWER_POWERSTATUS */
/* Description: Modem domain power status */

/* Bit 0 : LTE modem domain status */
#define POWER_POWERSTATUS_LTEMODEM_Pos (0UL) /*!< Position of LTEMODEM field. */
#define POWER_POWERSTATUS_LTEMODEM_Msk (0x1UL << POWER_POWERSTATUS_LTEMODEM_Pos) /*!< Bit mask of LTEMODEM field. */
#define POWER_POWERSTATUS_LTEMODEM_OFF (0UL) /*!< LTE modem domain is powered off */
#define POWER_POWERSTATUS_LTEMODEM_ON (1UL) /*!< LTE modem domain is powered on */

/* Register: POWER_GPREGRET */
/* Description: Description collection: General purpose retention register */

/* Bits 7..0 : General purpose retention register */
#define POWER_GPREGRET_GPREGRET_Pos (0UL) /*!< Position of GPREGRET field. */
#define POWER_GPREGRET_GPREGRET_Msk (0xFFUL << POWER_GPREGRET_GPREGRET_Pos) /*!< Bit mask of GPREGRET field. */


/* Peripheral: PWM */
/* Description: Pulse width modulation unit 0 */

/* Register: PWM_TASKS_STOP */
/* Description: Stops PWM pulse generation on all channels at the end of current PWM period, and stops sequence playback */

/* Bit 0 : Stops PWM pulse generation on all channels at the end of current PWM period, and stops sequence playback */
#define PWM_TASKS_STOP_TASKS_STOP_Pos (0UL) /*!< Position of TASKS_STOP field. */
#define PWM_TASKS_STOP_TASKS_STOP_Msk (0x1UL << PWM_TASKS_STOP_TASKS_STOP_Pos) /*!< Bit mask of TASKS_STOP field. */
#define PWM_TASKS_STOP_TASKS_STOP_Trigger (1UL) /*!< Trigger task */

/* Register: PWM_TASKS_SEQSTART */
/* Description: Description collection: Loads the first PWM value on all enabled channels from sequence n, and starts playing that sequence at the rate defined in SEQ[n]REFRESH and/or DECODER.MODE. Causes PWM generation to start if not running. */

/* Bit 0 : Loads the first PWM value on all enabled channels from sequence n, and starts playing that sequence at the rate defined in SEQ[n]REFRESH and/or DECODER.MODE. Causes PWM generation to start if not running. */
#define PWM_TASKS_SEQSTART_TASKS_SEQSTART_Pos (0UL) /*!< Position of TASKS_SEQSTART field. */
#define PWM_TASKS_SEQSTART_TASKS_SEQSTART_Msk (0x1UL << PWM_TASKS_SEQSTART_TASKS_SEQSTART_Pos) /*!< Bit mask of TASKS_SEQSTART field. */
#define PWM_TASKS_SEQSTART_TASKS_SEQSTART_Trigger (1UL) /*!< Trigger task */

/* Register: PWM_TASKS_NEXTSTEP */
/* Description: Steps by one value in the current sequence on all enabled channels if DECODER.MODE=NextStep. Does not cause PWM generation to start if not running. */

/* Bit 0 : Steps by one value in the current sequence on all enabled channels if DECODER.MODE=NextStep. Does not cause PWM generation to start if not running. */
#define PWM_TASKS_NEXTSTEP_TASKS_NEXTSTEP_Pos (0UL) /*!< Position of TASKS_NEXTSTEP field. */
#define PWM_TASKS_NEXTSTEP_TASKS_NEXTSTEP_Msk (0x1UL << PWM_TASKS_NEXTSTEP_TASKS_NEXTSTEP_Pos) /*!< Bit mask of TASKS_NEXTSTEP field. */
#define PWM_TASKS_NEXTSTEP_TASKS_NEXTSTEP_Trigger (1UL) /*!< Trigger task */

/* Register: PWM_SUBSCRIBE_STOP */
/* Description: Subscribe configuration for task STOP */

/* Bit 31 :   */
#define PWM_SUBSCRIBE_STOP_EN_Pos (31UL) /*!< Position of EN field. */
#define PWM_SUBSCRIBE_STOP_EN_Msk (0x1UL << PWM_SUBSCRIBE_STOP_EN_Pos) /*!< Bit mask of EN field. */
#define PWM_SUBSCRIBE_STOP_EN_Disabled (0UL) /*!< Disable subscription */
#define PWM_SUBSCRIBE_STOP_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task STOP will subscribe to */
#define PWM_SUBSCRIBE_STOP_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define PWM_SUBSCRIBE_STOP_CHIDX_Msk (0xFUL << PWM_SUBSCRIBE_STOP_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: PWM_SUBSCRIBE_SEQSTART */
/* Description: Description collection: Subscribe configuration for task SEQSTART[n] */

/* Bit 31 :   */
#define PWM_SUBSCRIBE_SEQSTART_EN_Pos (31UL) /*!< Position of EN field. */
#define PWM_SUBSCRIBE_SEQSTART_EN_Msk (0x1UL << PWM_SUBSCRIBE_SEQSTART_EN_Pos) /*!< Bit mask of EN field. */
#define PWM_SUBSCRIBE_SEQSTART_EN_Disabled (0UL) /*!< Disable subscription */
#define PWM_SUBSCRIBE_SEQSTART_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task SEQSTART[n] will subscribe to */
#define PWM_SUBSCRIBE_SEQSTART_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define PWM_SUBSCRIBE_SEQSTART_CHIDX_Msk (0xFUL << PWM_SUBSCRIBE_SEQSTART_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: PWM_SUBSCRIBE_NEXTSTEP */
/* Description: Subscribe configuration for task NEXTSTEP */

/* Bit 31 :   */
#define PWM_SUBSCRIBE_NEXTSTEP_EN_Pos (31UL) /*!< Position of EN field. */
#define PWM_SUBSCRIBE_NEXTSTEP_EN_Msk (0x1UL << PWM_SUBSCRIBE_NEXTSTEP_EN_Pos) /*!< Bit mask of EN field. */
#define PWM_SUBSCRIBE_NEXTSTEP_EN_Disabled (0UL) /*!< Disable subscription */
#define PWM_SUBSCRIBE_NEXTSTEP_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task NEXTSTEP will subscribe to */
#define PWM_SUBSCRIBE_NEXTSTEP_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define PWM_SUBSCRIBE_NEXTSTEP_CHIDX_Msk (0xFUL << PWM_SUBSCRIBE_NEXTSTEP_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: PWM_EVENTS_STOPPED */
/* Description: Response to STOP task, emitted when PWM pulses are no longer generated */

/* Bit 0 : Response to STOP task, emitted when PWM pulses are no longer generated */
#define PWM_EVENTS_STOPPED_EVENTS_STOPPED_Pos (0UL) /*!< Position of EVENTS_STOPPED field. */
#define PWM_EVENTS_STOPPED_EVENTS_STOPPED_Msk (0x1UL << PWM_EVENTS_STOPPED_EVENTS_STOPPED_Pos) /*!< Bit mask of EVENTS_STOPPED field. */
#define PWM_EVENTS_STOPPED_EVENTS_STOPPED_NotGenerated (0UL) /*!< Event not generated */
#define PWM_EVENTS_STOPPED_EVENTS_STOPPED_Generated (1UL) /*!< Event generated */

/* Register: PWM_EVENTS_SEQSTARTED */
/* Description: Description collection: First PWM period started on sequence n */

/* Bit 0 : First PWM period started on sequence n */
#define PWM_EVENTS_SEQSTARTED_EVENTS_SEQSTARTED_Pos (0UL) /*!< Position of EVENTS_SEQSTARTED field. */
#define PWM_EVENTS_SEQSTARTED_EVENTS_SEQSTARTED_Msk (0x1UL << PWM_EVENTS_SEQSTARTED_EVENTS_SEQSTARTED_Pos) /*!< Bit mask of EVENTS_SEQSTARTED field. */
#define PWM_EVENTS_SEQSTARTED_EVENTS_SEQSTARTED_NotGenerated (0UL) /*!< Event not generated */
#define PWM_EVENTS_SEQSTARTED_EVENTS_SEQSTARTED_Generated (1UL) /*!< Event generated */

/* Register: PWM_EVENTS_SEQEND */
/* Description: Description collection: Emitted at end of every sequence n, when last value from RAM has been applied to wave counter */

/* Bit 0 : Emitted at end of every sequence n, when last value from RAM has been applied to wave counter */
#define PWM_EVENTS_SEQEND_EVENTS_SEQEND_Pos (0UL) /*!< Position of EVENTS_SEQEND field. */
#define PWM_EVENTS_SEQEND_EVENTS_SEQEND_Msk (0x1UL << PWM_EVENTS_SEQEND_EVENTS_SEQEND_Pos) /*!< Bit mask of EVENTS_SEQEND field. */
#define PWM_EVENTS_SEQEND_EVENTS_SEQEND_NotGenerated (0UL) /*!< Event not generated */
#define PWM_EVENTS_SEQEND_EVENTS_SEQEND_Generated (1UL) /*!< Event generated */

/* Register: PWM_EVENTS_PWMPERIODEND */
/* Description: Emitted at the end of each PWM period */

/* Bit 0 : Emitted at the end of each PWM period */
#define PWM_EVENTS_PWMPERIODEND_EVENTS_PWMPERIODEND_Pos (0UL) /*!< Position of EVENTS_PWMPERIODEND field. */
#define PWM_EVENTS_PWMPERIODEND_EVENTS_PWMPERIODEND_Msk (0x1UL << PWM_EVENTS_PWMPERIODEND_EVENTS_PWMPERIODEND_Pos) /*!< Bit mask of EVENTS_PWMPERIODEND field. */
#define PWM_EVENTS_PWMPERIODEND_EVENTS_PWMPERIODEND_NotGenerated (0UL) /*!< Event not generated */
#define PWM_EVENTS_PWMPERIODEND_EVENTS_PWMPERIODEND_Generated (1UL) /*!< Event generated */

/* Register: PWM_EVENTS_LOOPSDONE */
/* Description: Concatenated sequences have been played the amount of times defined in LOOP.CNT */

/* Bit 0 : Concatenated sequences have been played the amount of times defined in LOOP.CNT */
#define PWM_EVENTS_LOOPSDONE_EVENTS_LOOPSDONE_Pos (0UL) /*!< Position of EVENTS_LOOPSDONE field. */
#define PWM_EVENTS_LOOPSDONE_EVENTS_LOOPSDONE_Msk (0x1UL << PWM_EVENTS_LOOPSDONE_EVENTS_LOOPSDONE_Pos) /*!< Bit mask of EVENTS_LOOPSDONE field. */
#define PWM_EVENTS_LOOPSDONE_EVENTS_LOOPSDONE_NotGenerated (0UL) /*!< Event not generated */
#define PWM_EVENTS_LOOPSDONE_EVENTS_LOOPSDONE_Generated (1UL) /*!< Event generated */

/* Register: PWM_PUBLISH_STOPPED */
/* Description: Publish configuration for event STOPPED */

/* Bit 31 :   */
#define PWM_PUBLISH_STOPPED_EN_Pos (31UL) /*!< Position of EN field. */
#define PWM_PUBLISH_STOPPED_EN_Msk (0x1UL << PWM_PUBLISH_STOPPED_EN_Pos) /*!< Bit mask of EN field. */
#define PWM_PUBLISH_STOPPED_EN_Disabled (0UL) /*!< Disable publishing */
#define PWM_PUBLISH_STOPPED_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event STOPPED will publish to. */
#define PWM_PUBLISH_STOPPED_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define PWM_PUBLISH_STOPPED_CHIDX_Msk (0xFUL << PWM_PUBLISH_STOPPED_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: PWM_PUBLISH_SEQSTARTED */
/* Description: Description collection: Publish configuration for event SEQSTARTED[n] */

/* Bit 31 :   */
#define PWM_PUBLISH_SEQSTARTED_EN_Pos (31UL) /*!< Position of EN field. */
#define PWM_PUBLISH_SEQSTARTED_EN_Msk (0x1UL << PWM_PUBLISH_SEQSTARTED_EN_Pos) /*!< Bit mask of EN field. */
#define PWM_PUBLISH_SEQSTARTED_EN_Disabled (0UL) /*!< Disable publishing */
#define PWM_PUBLISH_SEQSTARTED_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event SEQSTARTED[n] will publish to. */
#define PWM_PUBLISH_SEQSTARTED_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define PWM_PUBLISH_SEQSTARTED_CHIDX_Msk (0xFUL << PWM_PUBLISH_SEQSTARTED_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: PWM_PUBLISH_SEQEND */
/* Description: Description collection: Publish configuration for event SEQEND[n] */

/* Bit 31 :   */
#define PWM_PUBLISH_SEQEND_EN_Pos (31UL) /*!< Position of EN field. */
#define PWM_PUBLISH_SEQEND_EN_Msk (0x1UL << PWM_PUBLISH_SEQEND_EN_Pos) /*!< Bit mask of EN field. */
#define PWM_PUBLISH_SEQEND_EN_Disabled (0UL) /*!< Disable publishing */
#define PWM_PUBLISH_SEQEND_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event SEQEND[n] will publish to. */
#define PWM_PUBLISH_SEQEND_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define PWM_PUBLISH_SEQEND_CHIDX_Msk (0xFUL << PWM_PUBLISH_SEQEND_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: PWM_PUBLISH_PWMPERIODEND */
/* Description: Publish configuration for event PWMPERIODEND */

/* Bit 31 :   */
#define PWM_PUBLISH_PWMPERIODEND_EN_Pos (31UL) /*!< Position of EN field. */
#define PWM_PUBLISH_PWMPERIODEND_EN_Msk (0x1UL << PWM_PUBLISH_PWMPERIODEND_EN_Pos) /*!< Bit mask of EN field. */
#define PWM_PUBLISH_PWMPERIODEND_EN_Disabled (0UL) /*!< Disable publishing */
#define PWM_PUBLISH_PWMPERIODEND_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event PWMPERIODEND will publish to. */
#define PWM_PUBLISH_PWMPERIODEND_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define PWM_PUBLISH_PWMPERIODEND_CHIDX_Msk (0xFUL << PWM_PUBLISH_PWMPERIODEND_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: PWM_PUBLISH_LOOPSDONE */
/* Description: Publish configuration for event LOOPSDONE */

/* Bit 31 :   */
#define PWM_PUBLISH_LOOPSDONE_EN_Pos (31UL) /*!< Position of EN field. */
#define PWM_PUBLISH_LOOPSDONE_EN_Msk (0x1UL << PWM_PUBLISH_LOOPSDONE_EN_Pos) /*!< Bit mask of EN field. */
#define PWM_PUBLISH_LOOPSDONE_EN_Disabled (0UL) /*!< Disable publishing */
#define PWM_PUBLISH_LOOPSDONE_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event LOOPSDONE will publish to. */
#define PWM_PUBLISH_LOOPSDONE_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define PWM_PUBLISH_LOOPSDONE_CHIDX_Msk (0xFUL << PWM_PUBLISH_LOOPSDONE_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: PWM_SHORTS */
/* Description: Shortcuts between local events and tasks */

/* Bit 4 : Shortcut between event LOOPSDONE and task STOP */
#define PWM_SHORTS_LOOPSDONE_STOP_Pos (4UL) /*!< Position of LOOPSDONE_STOP field. */
#define PWM_SHORTS_LOOPSDONE_STOP_Msk (0x1UL << PWM_SHORTS_LOOPSDONE_STOP_Pos) /*!< Bit mask of LOOPSDONE_STOP field. */
#define PWM_SHORTS_LOOPSDONE_STOP_Disabled (0UL) /*!< Disable shortcut */
#define PWM_SHORTS_LOOPSDONE_STOP_Enabled (1UL) /*!< Enable shortcut */

/* Bit 3 : Shortcut between event LOOPSDONE and task SEQSTART[1] */
#define PWM_SHORTS_LOOPSDONE_SEQSTART1_Pos (3UL) /*!< Position of LOOPSDONE_SEQSTART1 field. */
#define PWM_SHORTS_LOOPSDONE_SEQSTART1_Msk (0x1UL << PWM_SHORTS_LOOPSDONE_SEQSTART1_Pos) /*!< Bit mask of LOOPSDONE_SEQSTART1 field. */
#define PWM_SHORTS_LOOPSDONE_SEQSTART1_Disabled (0UL) /*!< Disable shortcut */
#define PWM_SHORTS_LOOPSDONE_SEQSTART1_Enabled (1UL) /*!< Enable shortcut */

/* Bit 2 : Shortcut between event LOOPSDONE and task SEQSTART[0] */
#define PWM_SHORTS_LOOPSDONE_SEQSTART0_Pos (2UL) /*!< Position of LOOPSDONE_SEQSTART0 field. */
#define PWM_SHORTS_LOOPSDONE_SEQSTART0_Msk (0x1UL << PWM_SHORTS_LOOPSDONE_SEQSTART0_Pos) /*!< Bit mask of LOOPSDONE_SEQSTART0 field. */
#define PWM_SHORTS_LOOPSDONE_SEQSTART0_Disabled (0UL) /*!< Disable shortcut */
#define PWM_SHORTS_LOOPSDONE_SEQSTART0_Enabled (1UL) /*!< Enable shortcut */

/* Bit 1 : Shortcut between event SEQEND[1] and task STOP */
#define PWM_SHORTS_SEQEND1_STOP_Pos (1UL) /*!< Position of SEQEND1_STOP field. */
#define PWM_SHORTS_SEQEND1_STOP_Msk (0x1UL << PWM_SHORTS_SEQEND1_STOP_Pos) /*!< Bit mask of SEQEND1_STOP field. */
#define PWM_SHORTS_SEQEND1_STOP_Disabled (0UL) /*!< Disable shortcut */
#define PWM_SHORTS_SEQEND1_STOP_Enabled (1UL) /*!< Enable shortcut */

/* Bit 0 : Shortcut between event SEQEND[0] and task STOP */
#define PWM_SHORTS_SEQEND0_STOP_Pos (0UL) /*!< Position of SEQEND0_STOP field. */
#define PWM_SHORTS_SEQEND0_STOP_Msk (0x1UL << PWM_SHORTS_SEQEND0_STOP_Pos) /*!< Bit mask of SEQEND0_STOP field. */
#define PWM_SHORTS_SEQEND0_STOP_Disabled (0UL) /*!< Disable shortcut */
#define PWM_SHORTS_SEQEND0_STOP_Enabled (1UL) /*!< Enable shortcut */

/* Register: PWM_INTEN */
/* Description: Enable or disable interrupt */

/* Bit 7 : Enable or disable interrupt for event LOOPSDONE */
#define PWM_INTEN_LOOPSDONE_Pos (7UL) /*!< Position of LOOPSDONE field. */
#define PWM_INTEN_LOOPSDONE_Msk (0x1UL << PWM_INTEN_LOOPSDONE_Pos) /*!< Bit mask of LOOPSDONE field. */
#define PWM_INTEN_LOOPSDONE_Disabled (0UL) /*!< Disable */
#define PWM_INTEN_LOOPSDONE_Enabled (1UL) /*!< Enable */

/* Bit 6 : Enable or disable interrupt for event PWMPERIODEND */
#define PWM_INTEN_PWMPERIODEND_Pos (6UL) /*!< Position of PWMPERIODEND field. */
#define PWM_INTEN_PWMPERIODEND_Msk (0x1UL << PWM_INTEN_PWMPERIODEND_Pos) /*!< Bit mask of PWMPERIODEND field. */
#define PWM_INTEN_PWMPERIODEND_Disabled (0UL) /*!< Disable */
#define PWM_INTEN_PWMPERIODEND_Enabled (1UL) /*!< Enable */

/* Bit 5 : Enable or disable interrupt for event SEQEND[1] */
#define PWM_INTEN_SEQEND1_Pos (5UL) /*!< Position of SEQEND1 field. */
#define PWM_INTEN_SEQEND1_Msk (0x1UL << PWM_INTEN_SEQEND1_Pos) /*!< Bit mask of SEQEND1 field. */
#define PWM_INTEN_SEQEND1_Disabled (0UL) /*!< Disable */
#define PWM_INTEN_SEQEND1_Enabled (1UL) /*!< Enable */

/* Bit 4 : Enable or disable interrupt for event SEQEND[0] */
#define PWM_INTEN_SEQEND0_Pos (4UL) /*!< Position of SEQEND0 field. */
#define PWM_INTEN_SEQEND0_Msk (0x1UL << PWM_INTEN_SEQEND0_Pos) /*!< Bit mask of SEQEND0 field. */
#define PWM_INTEN_SEQEND0_Disabled (0UL) /*!< Disable */
#define PWM_INTEN_SEQEND0_Enabled (1UL) /*!< Enable */

/* Bit 3 : Enable or disable interrupt for event SEQSTARTED[1] */
#define PWM_INTEN_SEQSTARTED1_Pos (3UL) /*!< Position of SEQSTARTED1 field. */
#define PWM_INTEN_SEQSTARTED1_Msk (0x1UL << PWM_INTEN_SEQSTARTED1_Pos) /*!< Bit mask of SEQSTARTED1 field. */
#define PWM_INTEN_SEQSTARTED1_Disabled (0UL) /*!< Disable */
#define PWM_INTEN_SEQSTARTED1_Enabled (1UL) /*!< Enable */

/* Bit 2 : Enable or disable interrupt for event SEQSTARTED[0] */
#define PWM_INTEN_SEQSTARTED0_Pos (2UL) /*!< Position of SEQSTARTED0 field. */
#define PWM_INTEN_SEQSTARTED0_Msk (0x1UL << PWM_INTEN_SEQSTARTED0_Pos) /*!< Bit mask of SEQSTARTED0 field. */
#define PWM_INTEN_SEQSTARTED0_Disabled (0UL) /*!< Disable */
#define PWM_INTEN_SEQSTARTED0_Enabled (1UL) /*!< Enable */

/* Bit 1 : Enable or disable interrupt for event STOPPED */
#define PWM_INTEN_STOPPED_Pos (1UL) /*!< Position of STOPPED field. */
#define PWM_INTEN_STOPPED_Msk (0x1UL << PWM_INTEN_STOPPED_Pos) /*!< Bit mask of STOPPED field. */
#define PWM_INTEN_STOPPED_Disabled (0UL) /*!< Disable */
#define PWM_INTEN_STOPPED_Enabled (1UL) /*!< Enable */

/* Register: PWM_INTENSET */
/* Description: Enable interrupt */

/* Bit 7 : Write '1' to enable interrupt for event LOOPSDONE */
#define PWM_INTENSET_LOOPSDONE_Pos (7UL) /*!< Position of LOOPSDONE field. */
#define PWM_INTENSET_LOOPSDONE_Msk (0x1UL << PWM_INTENSET_LOOPSDONE_Pos) /*!< Bit mask of LOOPSDONE field. */
#define PWM_INTENSET_LOOPSDONE_Disabled (0UL) /*!< Read: Disabled */
#define PWM_INTENSET_LOOPSDONE_Enabled (1UL) /*!< Read: Enabled */
#define PWM_INTENSET_LOOPSDONE_Set (1UL) /*!< Enable */

/* Bit 6 : Write '1' to enable interrupt for event PWMPERIODEND */
#define PWM_INTENSET_PWMPERIODEND_Pos (6UL) /*!< Position of PWMPERIODEND field. */
#define PWM_INTENSET_PWMPERIODEND_Msk (0x1UL << PWM_INTENSET_PWMPERIODEND_Pos) /*!< Bit mask of PWMPERIODEND field. */
#define PWM_INTENSET_PWMPERIODEND_Disabled (0UL) /*!< Read: Disabled */
#define PWM_INTENSET_PWMPERIODEND_Enabled (1UL) /*!< Read: Enabled */
#define PWM_INTENSET_PWMPERIODEND_Set (1UL) /*!< Enable */

/* Bit 5 : Write '1' to enable interrupt for event SEQEND[1] */
#define PWM_INTENSET_SEQEND1_Pos (5UL) /*!< Position of SEQEND1 field. */
#define PWM_INTENSET_SEQEND1_Msk (0x1UL << PWM_INTENSET_SEQEND1_Pos) /*!< Bit mask of SEQEND1 field. */
#define PWM_INTENSET_SEQEND1_Disabled (0UL) /*!< Read: Disabled */
#define PWM_INTENSET_SEQEND1_Enabled (1UL) /*!< Read: Enabled */
#define PWM_INTENSET_SEQEND1_Set (1UL) /*!< Enable */

/* Bit 4 : Write '1' to enable interrupt for event SEQEND[0] */
#define PWM_INTENSET_SEQEND0_Pos (4UL) /*!< Position of SEQEND0 field. */
#define PWM_INTENSET_SEQEND0_Msk (0x1UL << PWM_INTENSET_SEQEND0_Pos) /*!< Bit mask of SEQEND0 field. */
#define PWM_INTENSET_SEQEND0_Disabled (0UL) /*!< Read: Disabled */
#define PWM_INTENSET_SEQEND0_Enabled (1UL) /*!< Read: Enabled */
#define PWM_INTENSET_SEQEND0_Set (1UL) /*!< Enable */

/* Bit 3 : Write '1' to enable interrupt for event SEQSTARTED[1] */
#define PWM_INTENSET_SEQSTARTED1_Pos (3UL) /*!< Position of SEQSTARTED1 field. */
#define PWM_INTENSET_SEQSTARTED1_Msk (0x1UL << PWM_INTENSET_SEQSTARTED1_Pos) /*!< Bit mask of SEQSTARTED1 field. */
#define PWM_INTENSET_SEQSTARTED1_Disabled (0UL) /*!< Read: Disabled */
#define PWM_INTENSET_SEQSTARTED1_Enabled (1UL) /*!< Read: Enabled */
#define PWM_INTENSET_SEQSTARTED1_Set (1UL) /*!< Enable */

/* Bit 2 : Write '1' to enable interrupt for event SEQSTARTED[0] */
#define PWM_INTENSET_SEQSTARTED0_Pos (2UL) /*!< Position of SEQSTARTED0 field. */
#define PWM_INTENSET_SEQSTARTED0_Msk (0x1UL << PWM_INTENSET_SEQSTARTED0_Pos) /*!< Bit mask of SEQSTARTED0 field. */
#define PWM_INTENSET_SEQSTARTED0_Disabled (0UL) /*!< Read: Disabled */
#define PWM_INTENSET_SEQSTARTED0_Enabled (1UL) /*!< Read: Enabled */
#define PWM_INTENSET_SEQSTARTED0_Set (1UL) /*!< Enable */

/* Bit 1 : Write '1' to enable interrupt for event STOPPED */
#define PWM_INTENSET_STOPPED_Pos (1UL) /*!< Position of STOPPED field. */
#define PWM_INTENSET_STOPPED_Msk (0x1UL << PWM_INTENSET_STOPPED_Pos) /*!< Bit mask of STOPPED field. */
#define PWM_INTENSET_STOPPED_Disabled (0UL) /*!< Read: Disabled */
#define PWM_INTENSET_STOPPED_Enabled (1UL) /*!< Read: Enabled */
#define PWM_INTENSET_STOPPED_Set (1UL) /*!< Enable */

/* Register: PWM_INTENCLR */
/* Description: Disable interrupt */

/* Bit 7 : Write '1' to disable interrupt for event LOOPSDONE */
#define PWM_INTENCLR_LOOPSDONE_Pos (7UL) /*!< Position of LOOPSDONE field. */
#define PWM_INTENCLR_LOOPSDONE_Msk (0x1UL << PWM_INTENCLR_LOOPSDONE_Pos) /*!< Bit mask of LOOPSDONE field. */
#define PWM_INTENCLR_LOOPSDONE_Disabled (0UL) /*!< Read: Disabled */
#define PWM_INTENCLR_LOOPSDONE_Enabled (1UL) /*!< Read: Enabled */
#define PWM_INTENCLR_LOOPSDONE_Clear (1UL) /*!< Disable */

/* Bit 6 : Write '1' to disable interrupt for event PWMPERIODEND */
#define PWM_INTENCLR_PWMPERIODEND_Pos (6UL) /*!< Position of PWMPERIODEND field. */
#define PWM_INTENCLR_PWMPERIODEND_Msk (0x1UL << PWM_INTENCLR_PWMPERIODEND_Pos) /*!< Bit mask of PWMPERIODEND field. */
#define PWM_INTENCLR_PWMPERIODEND_Disabled (0UL) /*!< Read: Disabled */
#define PWM_INTENCLR_PWMPERIODEND_Enabled (1UL) /*!< Read: Enabled */
#define PWM_INTENCLR_PWMPERIODEND_Clear (1UL) /*!< Disable */

/* Bit 5 : Write '1' to disable interrupt for event SEQEND[1] */
#define PWM_INTENCLR_SEQEND1_Pos (5UL) /*!< Position of SEQEND1 field. */
#define PWM_INTENCLR_SEQEND1_Msk (0x1UL << PWM_INTENCLR_SEQEND1_Pos) /*!< Bit mask of SEQEND1 field. */
#define PWM_INTENCLR_SEQEND1_Disabled (0UL) /*!< Read: Disabled */
#define PWM_INTENCLR_SEQEND1_Enabled (1UL) /*!< Read: Enabled */
#define PWM_INTENCLR_SEQEND1_Clear (1UL) /*!< Disable */

/* Bit 4 : Write '1' to disable interrupt for event SEQEND[0] */
#define PWM_INTENCLR_SEQEND0_Pos (4UL) /*!< Position of SEQEND0 field. */
#define PWM_INTENCLR_SEQEND0_Msk (0x1UL << PWM_INTENCLR_SEQEND0_Pos) /*!< Bit mask of SEQEND0 field. */
#define PWM_INTENCLR_SEQEND0_Disabled (0UL) /*!< Read: Disabled */
#define PWM_INTENCLR_SEQEND0_Enabled (1UL) /*!< Read: Enabled */
#define PWM_INTENCLR_SEQEND0_Clear (1UL) /*!< Disable */

/* Bit 3 : Write '1' to disable interrupt for event SEQSTARTED[1] */
#define PWM_INTENCLR_SEQSTARTED1_Pos (3UL) /*!< Position of SEQSTARTED1 field. */
#define PWM_INTENCLR_SEQSTARTED1_Msk (0x1UL << PWM_INTENCLR_SEQSTARTED1_Pos) /*!< Bit mask of SEQSTARTED1 field. */
#define PWM_INTENCLR_SEQSTARTED1_Disabled (0UL) /*!< Read: Disabled */
#define PWM_INTENCLR_SEQSTARTED1_Enabled (1UL) /*!< Read: Enabled */
#define PWM_INTENCLR_SEQSTARTED1_Clear (1UL) /*!< Disable */

/* Bit 2 : Write '1' to disable interrupt for event SEQSTARTED[0] */
#define PWM_INTENCLR_SEQSTARTED0_Pos (2UL) /*!< Position of SEQSTARTED0 field. */
#define PWM_INTENCLR_SEQSTARTED0_Msk (0x1UL << PWM_INTENCLR_SEQSTARTED0_Pos) /*!< Bit mask of SEQSTARTED0 field. */
#define PWM_INTENCLR_SEQSTARTED0_Disabled (0UL) /*!< Read: Disabled */
#define PWM_INTENCLR_SEQSTARTED0_Enabled (1UL) /*!< Read: Enabled */
#define PWM_INTENCLR_SEQSTARTED0_Clear (1UL) /*!< Disable */

/* Bit 1 : Write '1' to disable interrupt for event STOPPED */
#define PWM_INTENCLR_STOPPED_Pos (1UL) /*!< Position of STOPPED field. */
#define PWM_INTENCLR_STOPPED_Msk (0x1UL << PWM_INTENCLR_STOPPED_Pos) /*!< Bit mask of STOPPED field. */
#define PWM_INTENCLR_STOPPED_Disabled (0UL) /*!< Read: Disabled */
#define PWM_INTENCLR_STOPPED_Enabled (1UL) /*!< Read: Enabled */
#define PWM_INTENCLR_STOPPED_Clear (1UL) /*!< Disable */

/* Register: PWM_ENABLE */
/* Description: PWM module enable register */

/* Bit 0 : Enable or disable PWM module */
#define PWM_ENABLE_ENABLE_Pos (0UL) /*!< Position of ENABLE field. */
#define PWM_ENABLE_ENABLE_Msk (0x1UL << PWM_ENABLE_ENABLE_Pos) /*!< Bit mask of ENABLE field. */
#define PWM_ENABLE_ENABLE_Disabled (0UL) /*!< Disabled */
#define PWM_ENABLE_ENABLE_Enabled (1UL) /*!< Enable */

/* Register: PWM_MODE */
/* Description: Selects operating mode of the wave counter */

/* Bit 0 : Selects up mode or up-and-down mode for the counter */
#define PWM_MODE_UPDOWN_Pos (0UL) /*!< Position of UPDOWN field. */
#define PWM_MODE_UPDOWN_Msk (0x1UL << PWM_MODE_UPDOWN_Pos) /*!< Bit mask of UPDOWN field. */
#define PWM_MODE_UPDOWN_Up (0UL) /*!< Up counter, edge-aligned PWM duty cycle */
#define PWM_MODE_UPDOWN_UpAndDown (1UL) /*!< Up and down counter, center-aligned PWM duty cycle */

/* Register: PWM_COUNTERTOP */
/* Description: Value up to which the pulse generator counter counts */

/* Bits 14..0 : Value up to which the pulse generator counter counts. This register is ignored when DECODER.MODE=WaveForm and only values from RAM are used. */
#define PWM_COUNTERTOP_COUNTERTOP_Pos (0UL) /*!< Position of COUNTERTOP field. */
#define PWM_COUNTERTOP_COUNTERTOP_Msk (0x7FFFUL << PWM_COUNTERTOP_COUNTERTOP_Pos) /*!< Bit mask of COUNTERTOP field. */

/* Register: PWM_PRESCALER */
/* Description: Configuration for PWM_CLK */

/* Bits 2..0 : Prescaler of PWM_CLK */
#define PWM_PRESCALER_PRESCALER_Pos (0UL) /*!< Position of PRESCALER field. */
#define PWM_PRESCALER_PRESCALER_Msk (0x7UL << PWM_PRESCALER_PRESCALER_Pos) /*!< Bit mask of PRESCALER field. */
#define PWM_PRESCALER_PRESCALER_DIV_1 (0UL) /*!< Divide by 1 (16 MHz) */
#define PWM_PRESCALER_PRESCALER_DIV_2 (1UL) /*!< Divide by 2 (8 MHz) */
#define PWM_PRESCALER_PRESCALER_DIV_4 (2UL) /*!< Divide by 4 (4 MHz) */
#define PWM_PRESCALER_PRESCALER_DIV_8 (3UL) /*!< Divide by 8 (2 MHz) */
#define PWM_PRESCALER_PRESCALER_DIV_16 (4UL) /*!< Divide by 16 (1 MHz) */
#define PWM_PRESCALER_PRESCALER_DIV_32 (5UL) /*!< Divide by 32 (500 kHz) */
#define PWM_PRESCALER_PRESCALER_DIV_64 (6UL) /*!< Divide by 64 (250 kHz) */
#define PWM_PRESCALER_PRESCALER_DIV_128 (7UL) /*!< Divide by 128 (125 kHz) */

/* Register: PWM_DECODER */
/* Description: Configuration of the decoder */

/* Bit 8 : Selects source for advancing the active sequence */
#define PWM_DECODER_MODE_Pos (8UL) /*!< Position of MODE field. */
#define PWM_DECODER_MODE_Msk (0x1UL << PWM_DECODER_MODE_Pos) /*!< Bit mask of MODE field. */
#define PWM_DECODER_MODE_RefreshCount (0UL) /*!< SEQ[n].REFRESH is used to determine loading internal compare registers */
#define PWM_DECODER_MODE_NextStep (1UL) /*!< NEXTSTEP task causes a new value to be loaded to internal compare registers */

/* Bits 1..0 : How a sequence is read from RAM and spread to the compare register */
#define PWM_DECODER_LOAD_Pos (0UL) /*!< Position of LOAD field. */
#define PWM_DECODER_LOAD_Msk (0x3UL << PWM_DECODER_LOAD_Pos) /*!< Bit mask of LOAD field. */
#define PWM_DECODER_LOAD_Common (0UL) /*!< 1st half word (16-bit) used in all PWM channels 0..3 */
#define PWM_DECODER_LOAD_Grouped (1UL) /*!< 1st half word (16-bit) used in channel 0..1; 2nd word in channel 2..3 */
#define PWM_DECODER_LOAD_Individual (2UL) /*!< 1st half word (16-bit) in ch.0; 2nd in ch.1; ...; 4th in ch.3 */
#define PWM_DECODER_LOAD_WaveForm (3UL) /*!< 1st half word (16-bit) in ch.0; 2nd in ch.1; ...; 4th in COUNTERTOP */

/* Register: PWM_LOOP */
/* Description: Number of playbacks of a loop */

/* Bits 15..0 : Number of playbacks of pattern cycles */
#define PWM_LOOP_CNT_Pos (0UL) /*!< Position of CNT field. */
#define PWM_LOOP_CNT_Msk (0xFFFFUL << PWM_LOOP_CNT_Pos) /*!< Bit mask of CNT field. */
#define PWM_LOOP_CNT_Disabled (0UL) /*!< Looping disabled (stop at the end of the sequence) */

/* Register: PWM_SEQ_PTR */
/* Description: Description cluster: Beginning address in RAM of this sequence */

/* Bits 31..0 : Beginning address in RAM of this sequence */
#define PWM_SEQ_PTR_PTR_Pos (0UL) /*!< Position of PTR field. */
#define PWM_SEQ_PTR_PTR_Msk (0xFFFFFFFFUL << PWM_SEQ_PTR_PTR_Pos) /*!< Bit mask of PTR field. */

/* Register: PWM_SEQ_CNT */
/* Description: Description cluster: Number of values (duty cycles) in this sequence */

/* Bits 14..0 : Number of values (duty cycles) in this sequence */
#define PWM_SEQ_CNT_CNT_Pos (0UL) /*!< Position of CNT field. */
#define PWM_SEQ_CNT_CNT_Msk (0x7FFFUL << PWM_SEQ_CNT_CNT_Pos) /*!< Bit mask of CNT field. */
#define PWM_SEQ_CNT_CNT_Disabled (0UL) /*!< Sequence is disabled, and shall not be started as it is empty */

/* Register: PWM_SEQ_REFRESH */
/* Description: Description cluster: Number of additional PWM periods between samples loaded into compare register */

/* Bits 23..0 : Number of additional PWM periods between samples loaded into compare register (load every REFRESH.CNT+1 PWM periods) */
#define PWM_SEQ_REFRESH_CNT_Pos (0UL) /*!< Position of CNT field. */
#define PWM_SEQ_REFRESH_CNT_Msk (0xFFFFFFUL << PWM_SEQ_REFRESH_CNT_Pos) /*!< Bit mask of CNT field. */
#define PWM_SEQ_REFRESH_CNT_Continuous (0UL) /*!< Update every PWM period */

/* Register: PWM_SEQ_ENDDELAY */
/* Description: Description cluster: Time added after the sequence */

/* Bits 23..0 : Time added after the sequence in PWM periods */
#define PWM_SEQ_ENDDELAY_CNT_Pos (0UL) /*!< Position of CNT field. */
#define PWM_SEQ_ENDDELAY_CNT_Msk (0xFFFFFFUL << PWM_SEQ_ENDDELAY_CNT_Pos) /*!< Bit mask of CNT field. */

/* Register: PWM_PSEL_OUT */
/* Description: Description collection: Output pin select for PWM channel n */

/* Bit 31 : Connection */
#define PWM_PSEL_OUT_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define PWM_PSEL_OUT_CONNECT_Msk (0x1UL << PWM_PSEL_OUT_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define PWM_PSEL_OUT_CONNECT_Connected (0UL) /*!< Connect */
#define PWM_PSEL_OUT_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define PWM_PSEL_OUT_PIN_Pos (0UL) /*!< Position of PIN field. */
#define PWM_PSEL_OUT_PIN_Msk (0x1FUL << PWM_PSEL_OUT_PIN_Pos) /*!< Bit mask of PIN field. */


/* Peripheral: REGULATORS */
/* Description: Voltage regulators control 0 */

/* Register: REGULATORS_SYSTEMOFF */
/* Description: System OFF register */

/* Bit 0 : Enable System OFF mode */
#define REGULATORS_SYSTEMOFF_SYSTEMOFF_Pos (0UL) /*!< Position of SYSTEMOFF field. */
#define REGULATORS_SYSTEMOFF_SYSTEMOFF_Msk (0x1UL << REGULATORS_SYSTEMOFF_SYSTEMOFF_Pos) /*!< Bit mask of SYSTEMOFF field. */
#define REGULATORS_SYSTEMOFF_SYSTEMOFF_Enable (1UL) /*!< Enable System OFF mode */

/* Register: REGULATORS_POFCON */
/* Description: Power-fail comparator configuration */

/* Bits 4..1 : Power-fail comparator threshold setting */
#define REGULATORS_POFCON_THRESHOLD_Pos (1UL) /*!< Position of THRESHOLD field. */
#define REGULATORS_POFCON_THRESHOLD_Msk (0xFUL << REGULATORS_POFCON_THRESHOLD_Pos) /*!< Bit mask of THRESHOLD field. */
#define REGULATORS_POFCON_THRESHOLD_V19 (6UL) /*!< Set threshold to 1.9 V */
#define REGULATORS_POFCON_THRESHOLD_V20 (7UL) /*!< Set threshold to 2.0 V */
#define REGULATORS_POFCON_THRESHOLD_V21 (8UL) /*!< Set threshold to 2.1 V */
#define REGULATORS_POFCON_THRESHOLD_V22 (9UL) /*!< Set threshold to 2.2 V */
#define REGULATORS_POFCON_THRESHOLD_V23 (10UL) /*!< Set threshold to 2.3 V */
#define REGULATORS_POFCON_THRESHOLD_V24 (11UL) /*!< Set threshold to 2.4 V */
#define REGULATORS_POFCON_THRESHOLD_V25 (12UL) /*!< Set threshold to 2.5 V */
#define REGULATORS_POFCON_THRESHOLD_V26 (13UL) /*!< Set threshold to 2.6 V */
#define REGULATORS_POFCON_THRESHOLD_V27 (14UL) /*!< Set threshold to 2.7 V */
#define REGULATORS_POFCON_THRESHOLD_V28 (15UL) /*!< Set threshold to 2.8 V */

/* Bit 0 : Enable or disable power-fail comparator */
#define REGULATORS_POFCON_POF_Pos (0UL) /*!< Position of POF field. */
#define REGULATORS_POFCON_POF_Msk (0x1UL << REGULATORS_POFCON_POF_Pos) /*!< Bit mask of POF field. */
#define REGULATORS_POFCON_POF_Disabled (0UL) /*!< Disable */
#define REGULATORS_POFCON_POF_Enabled (1UL) /*!< Enable */

/* Register: REGULATORS_DCDCEN */
/* Description: Enable DC/DC mode of the main voltage regulator */

/* Bit 0 : Enable DC/DC converter */
#define REGULATORS_DCDCEN_DCDCEN_Pos (0UL) /*!< Position of DCDCEN field. */
#define REGULATORS_DCDCEN_DCDCEN_Msk (0x1UL << REGULATORS_DCDCEN_DCDCEN_Pos) /*!< Bit mask of DCDCEN field. */
#define REGULATORS_DCDCEN_DCDCEN_Disabled (0UL) /*!< DC/DC mode is disabled */
#define REGULATORS_DCDCEN_DCDCEN_Enabled (1UL) /*!< DC/DC mode is enabled */


/* Peripheral: RTC */
/* Description: Real-time counter 0 */

/* Register: RTC_TASKS_START */
/* Description: Start RTC counter */

/* Bit 0 : Start RTC counter */
#define RTC_TASKS_START_TASKS_START_Pos (0UL) /*!< Position of TASKS_START field. */
#define RTC_TASKS_START_TASKS_START_Msk (0x1UL << RTC_TASKS_START_TASKS_START_Pos) /*!< Bit mask of TASKS_START field. */
#define RTC_TASKS_START_TASKS_START_Trigger (1UL) /*!< Trigger task */

/* Register: RTC_TASKS_STOP */
/* Description: Stop RTC counter */

/* Bit 0 : Stop RTC counter */
#define RTC_TASKS_STOP_TASKS_STOP_Pos (0UL) /*!< Position of TASKS_STOP field. */
#define RTC_TASKS_STOP_TASKS_STOP_Msk (0x1UL << RTC_TASKS_STOP_TASKS_STOP_Pos) /*!< Bit mask of TASKS_STOP field. */
#define RTC_TASKS_STOP_TASKS_STOP_Trigger (1UL) /*!< Trigger task */

/* Register: RTC_TASKS_CLEAR */
/* Description: Clear RTC counter */

/* Bit 0 : Clear RTC counter */
#define RTC_TASKS_CLEAR_TASKS_CLEAR_Pos (0UL) /*!< Position of TASKS_CLEAR field. */
#define RTC_TASKS_CLEAR_TASKS_CLEAR_Msk (0x1UL << RTC_TASKS_CLEAR_TASKS_CLEAR_Pos) /*!< Bit mask of TASKS_CLEAR field. */
#define RTC_TASKS_CLEAR_TASKS_CLEAR_Trigger (1UL) /*!< Trigger task */

/* Register: RTC_TASKS_TRIGOVRFLW */
/* Description: Set counter to 0xFFFFF0 */

/* Bit 0 : Set counter to 0xFFFFF0 */
#define RTC_TASKS_TRIGOVRFLW_TASKS_TRIGOVRFLW_Pos (0UL) /*!< Position of TASKS_TRIGOVRFLW field. */
#define RTC_TASKS_TRIGOVRFLW_TASKS_TRIGOVRFLW_Msk (0x1UL << RTC_TASKS_TRIGOVRFLW_TASKS_TRIGOVRFLW_Pos) /*!< Bit mask of TASKS_TRIGOVRFLW field. */
#define RTC_TASKS_TRIGOVRFLW_TASKS_TRIGOVRFLW_Trigger (1UL) /*!< Trigger task */

/* Register: RTC_SUBSCRIBE_START */
/* Description: Subscribe configuration for task START */

/* Bit 31 :   */
#define RTC_SUBSCRIBE_START_EN_Pos (31UL) /*!< Position of EN field. */
#define RTC_SUBSCRIBE_START_EN_Msk (0x1UL << RTC_SUBSCRIBE_START_EN_Pos) /*!< Bit mask of EN field. */
#define RTC_SUBSCRIBE_START_EN_Disabled (0UL) /*!< Disable subscription */
#define RTC_SUBSCRIBE_START_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task START will subscribe to */
#define RTC_SUBSCRIBE_START_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define RTC_SUBSCRIBE_START_CHIDX_Msk (0xFUL << RTC_SUBSCRIBE_START_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: RTC_SUBSCRIBE_STOP */
/* Description: Subscribe configuration for task STOP */

/* Bit 31 :   */
#define RTC_SUBSCRIBE_STOP_EN_Pos (31UL) /*!< Position of EN field. */
#define RTC_SUBSCRIBE_STOP_EN_Msk (0x1UL << RTC_SUBSCRIBE_STOP_EN_Pos) /*!< Bit mask of EN field. */
#define RTC_SUBSCRIBE_STOP_EN_Disabled (0UL) /*!< Disable subscription */
#define RTC_SUBSCRIBE_STOP_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task STOP will subscribe to */
#define RTC_SUBSCRIBE_STOP_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define RTC_SUBSCRIBE_STOP_CHIDX_Msk (0xFUL << RTC_SUBSCRIBE_STOP_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: RTC_SUBSCRIBE_CLEAR */
/* Description: Subscribe configuration for task CLEAR */

/* Bit 31 :   */
#define RTC_SUBSCRIBE_CLEAR_EN_Pos (31UL) /*!< Position of EN field. */
#define RTC_SUBSCRIBE_CLEAR_EN_Msk (0x1UL << RTC_SUBSCRIBE_CLEAR_EN_Pos) /*!< Bit mask of EN field. */
#define RTC_SUBSCRIBE_CLEAR_EN_Disabled (0UL) /*!< Disable subscription */
#define RTC_SUBSCRIBE_CLEAR_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task CLEAR will subscribe to */
#define RTC_SUBSCRIBE_CLEAR_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define RTC_SUBSCRIBE_CLEAR_CHIDX_Msk (0xFUL << RTC_SUBSCRIBE_CLEAR_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: RTC_SUBSCRIBE_TRIGOVRFLW */
/* Description: Subscribe configuration for task TRIGOVRFLW */

/* Bit 31 :   */
#define RTC_SUBSCRIBE_TRIGOVRFLW_EN_Pos (31UL) /*!< Position of EN field. */
#define RTC_SUBSCRIBE_TRIGOVRFLW_EN_Msk (0x1UL << RTC_SUBSCRIBE_TRIGOVRFLW_EN_Pos) /*!< Bit mask of EN field. */
#define RTC_SUBSCRIBE_TRIGOVRFLW_EN_Disabled (0UL) /*!< Disable subscription */
#define RTC_SUBSCRIBE_TRIGOVRFLW_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task TRIGOVRFLW will subscribe to */
#define RTC_SUBSCRIBE_TRIGOVRFLW_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define RTC_SUBSCRIBE_TRIGOVRFLW_CHIDX_Msk (0xFUL << RTC_SUBSCRIBE_TRIGOVRFLW_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: RTC_EVENTS_TICK */
/* Description: Event on counter increment */

/* Bit 0 : Event on counter increment */
#define RTC_EVENTS_TICK_EVENTS_TICK_Pos (0UL) /*!< Position of EVENTS_TICK field. */
#define RTC_EVENTS_TICK_EVENTS_TICK_Msk (0x1UL << RTC_EVENTS_TICK_EVENTS_TICK_Pos) /*!< Bit mask of EVENTS_TICK field. */
#define RTC_EVENTS_TICK_EVENTS_TICK_NotGenerated (0UL) /*!< Event not generated */
#define RTC_EVENTS_TICK_EVENTS_TICK_Generated (1UL) /*!< Event generated */

/* Register: RTC_EVENTS_OVRFLW */
/* Description: Event on counter overflow */

/* Bit 0 : Event on counter overflow */
#define RTC_EVENTS_OVRFLW_EVENTS_OVRFLW_Pos (0UL) /*!< Position of EVENTS_OVRFLW field. */
#define RTC_EVENTS_OVRFLW_EVENTS_OVRFLW_Msk (0x1UL << RTC_EVENTS_OVRFLW_EVENTS_OVRFLW_Pos) /*!< Bit mask of EVENTS_OVRFLW field. */
#define RTC_EVENTS_OVRFLW_EVENTS_OVRFLW_NotGenerated (0UL) /*!< Event not generated */
#define RTC_EVENTS_OVRFLW_EVENTS_OVRFLW_Generated (1UL) /*!< Event generated */

/* Register: RTC_EVENTS_COMPARE */
/* Description: Description collection: Compare event on CC[n] match */

/* Bit 0 : Compare event on CC[n] match */
#define RTC_EVENTS_COMPARE_EVENTS_COMPARE_Pos (0UL) /*!< Position of EVENTS_COMPARE field. */
#define RTC_EVENTS_COMPARE_EVENTS_COMPARE_Msk (0x1UL << RTC_EVENTS_COMPARE_EVENTS_COMPARE_Pos) /*!< Bit mask of EVENTS_COMPARE field. */
#define RTC_EVENTS_COMPARE_EVENTS_COMPARE_NotGenerated (0UL) /*!< Event not generated */
#define RTC_EVENTS_COMPARE_EVENTS_COMPARE_Generated (1UL) /*!< Event generated */

/* Register: RTC_PUBLISH_TICK */
/* Description: Publish configuration for event TICK */

/* Bit 31 :   */
#define RTC_PUBLISH_TICK_EN_Pos (31UL) /*!< Position of EN field. */
#define RTC_PUBLISH_TICK_EN_Msk (0x1UL << RTC_PUBLISH_TICK_EN_Pos) /*!< Bit mask of EN field. */
#define RTC_PUBLISH_TICK_EN_Disabled (0UL) /*!< Disable publishing */
#define RTC_PUBLISH_TICK_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event TICK will publish to. */
#define RTC_PUBLISH_TICK_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define RTC_PUBLISH_TICK_CHIDX_Msk (0xFUL << RTC_PUBLISH_TICK_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: RTC_PUBLISH_OVRFLW */
/* Description: Publish configuration for event OVRFLW */

/* Bit 31 :   */
#define RTC_PUBLISH_OVRFLW_EN_Pos (31UL) /*!< Position of EN field. */
#define RTC_PUBLISH_OVRFLW_EN_Msk (0x1UL << RTC_PUBLISH_OVRFLW_EN_Pos) /*!< Bit mask of EN field. */
#define RTC_PUBLISH_OVRFLW_EN_Disabled (0UL) /*!< Disable publishing */
#define RTC_PUBLISH_OVRFLW_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event OVRFLW will publish to. */
#define RTC_PUBLISH_OVRFLW_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define RTC_PUBLISH_OVRFLW_CHIDX_Msk (0xFUL << RTC_PUBLISH_OVRFLW_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: RTC_PUBLISH_COMPARE */
/* Description: Description collection: Publish configuration for event COMPARE[n] */

/* Bit 31 :   */
#define RTC_PUBLISH_COMPARE_EN_Pos (31UL) /*!< Position of EN field. */
#define RTC_PUBLISH_COMPARE_EN_Msk (0x1UL << RTC_PUBLISH_COMPARE_EN_Pos) /*!< Bit mask of EN field. */
#define RTC_PUBLISH_COMPARE_EN_Disabled (0UL) /*!< Disable publishing */
#define RTC_PUBLISH_COMPARE_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event COMPARE[n] will publish to. */
#define RTC_PUBLISH_COMPARE_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define RTC_PUBLISH_COMPARE_CHIDX_Msk (0xFUL << RTC_PUBLISH_COMPARE_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: RTC_INTENSET */
/* Description: Enable interrupt */

/* Bit 19 : Write '1' to enable interrupt for event COMPARE[3] */
#define RTC_INTENSET_COMPARE3_Pos (19UL) /*!< Position of COMPARE3 field. */
#define RTC_INTENSET_COMPARE3_Msk (0x1UL << RTC_INTENSET_COMPARE3_Pos) /*!< Bit mask of COMPARE3 field. */
#define RTC_INTENSET_COMPARE3_Disabled (0UL) /*!< Read: Disabled */
#define RTC_INTENSET_COMPARE3_Enabled (1UL) /*!< Read: Enabled */
#define RTC_INTENSET_COMPARE3_Set (1UL) /*!< Enable */

/* Bit 18 : Write '1' to enable interrupt for event COMPARE[2] */
#define RTC_INTENSET_COMPARE2_Pos (18UL) /*!< Position of COMPARE2 field. */
#define RTC_INTENSET_COMPARE2_Msk (0x1UL << RTC_INTENSET_COMPARE2_Pos) /*!< Bit mask of COMPARE2 field. */
#define RTC_INTENSET_COMPARE2_Disabled (0UL) /*!< Read: Disabled */
#define RTC_INTENSET_COMPARE2_Enabled (1UL) /*!< Read: Enabled */
#define RTC_INTENSET_COMPARE2_Set (1UL) /*!< Enable */

/* Bit 17 : Write '1' to enable interrupt for event COMPARE[1] */
#define RTC_INTENSET_COMPARE1_Pos (17UL) /*!< Position of COMPARE1 field. */
#define RTC_INTENSET_COMPARE1_Msk (0x1UL << RTC_INTENSET_COMPARE1_Pos) /*!< Bit mask of COMPARE1 field. */
#define RTC_INTENSET_COMPARE1_Disabled (0UL) /*!< Read: Disabled */
#define RTC_INTENSET_COMPARE1_Enabled (1UL) /*!< Read: Enabled */
#define RTC_INTENSET_COMPARE1_Set (1UL) /*!< Enable */

/* Bit 16 : Write '1' to enable interrupt for event COMPARE[0] */
#define RTC_INTENSET_COMPARE0_Pos (16UL) /*!< Position of COMPARE0 field. */
#define RTC_INTENSET_COMPARE0_Msk (0x1UL << RTC_INTENSET_COMPARE0_Pos) /*!< Bit mask of COMPARE0 field. */
#define RTC_INTENSET_COMPARE0_Disabled (0UL) /*!< Read: Disabled */
#define RTC_INTENSET_COMPARE0_Enabled (1UL) /*!< Read: Enabled */
#define RTC_INTENSET_COMPARE0_Set (1UL) /*!< Enable */

/* Bit 1 : Write '1' to enable interrupt for event OVRFLW */
#define RTC_INTENSET_OVRFLW_Pos (1UL) /*!< Position of OVRFLW field. */
#define RTC_INTENSET_OVRFLW_Msk (0x1UL << RTC_INTENSET_OVRFLW_Pos) /*!< Bit mask of OVRFLW field. */
#define RTC_INTENSET_OVRFLW_Disabled (0UL) /*!< Read: Disabled */
#define RTC_INTENSET_OVRFLW_Enabled (1UL) /*!< Read: Enabled */
#define RTC_INTENSET_OVRFLW_Set (1UL) /*!< Enable */

/* Bit 0 : Write '1' to enable interrupt for event TICK */
#define RTC_INTENSET_TICK_Pos (0UL) /*!< Position of TICK field. */
#define RTC_INTENSET_TICK_Msk (0x1UL << RTC_INTENSET_TICK_Pos) /*!< Bit mask of TICK field. */
#define RTC_INTENSET_TICK_Disabled (0UL) /*!< Read: Disabled */
#define RTC_INTENSET_TICK_Enabled (1UL) /*!< Read: Enabled */
#define RTC_INTENSET_TICK_Set (1UL) /*!< Enable */

/* Register: RTC_INTENCLR */
/* Description: Disable interrupt */

/* Bit 19 : Write '1' to disable interrupt for event COMPARE[3] */
#define RTC_INTENCLR_COMPARE3_Pos (19UL) /*!< Position of COMPARE3 field. */
#define RTC_INTENCLR_COMPARE3_Msk (0x1UL << RTC_INTENCLR_COMPARE3_Pos) /*!< Bit mask of COMPARE3 field. */
#define RTC_INTENCLR_COMPARE3_Disabled (0UL) /*!< Read: Disabled */
#define RTC_INTENCLR_COMPARE3_Enabled (1UL) /*!< Read: Enabled */
#define RTC_INTENCLR_COMPARE3_Clear (1UL) /*!< Disable */

/* Bit 18 : Write '1' to disable interrupt for event COMPARE[2] */
#define RTC_INTENCLR_COMPARE2_Pos (18UL) /*!< Position of COMPARE2 field. */
#define RTC_INTENCLR_COMPARE2_Msk (0x1UL << RTC_INTENCLR_COMPARE2_Pos) /*!< Bit mask of COMPARE2 field. */
#define RTC_INTENCLR_COMPARE2_Disabled (0UL) /*!< Read: Disabled */
#define RTC_INTENCLR_COMPARE2_Enabled (1UL) /*!< Read: Enabled */
#define RTC_INTENCLR_COMPARE2_Clear (1UL) /*!< Disable */

/* Bit 17 : Write '1' to disable interrupt for event COMPARE[1] */
#define RTC_INTENCLR_COMPARE1_Pos (17UL) /*!< Position of COMPARE1 field. */
#define RTC_INTENCLR_COMPARE1_Msk (0x1UL << RTC_INTENCLR_COMPARE1_Pos) /*!< Bit mask of COMPARE1 field. */
#define RTC_INTENCLR_COMPARE1_Disabled (0UL) /*!< Read: Disabled */
#define RTC_INTENCLR_COMPARE1_Enabled (1UL) /*!< Read: Enabled */
#define RTC_INTENCLR_COMPARE1_Clear (1UL) /*!< Disable */

/* Bit 16 : Write '1' to disable interrupt for event COMPARE[0] */
#define RTC_INTENCLR_COMPARE0_Pos (16UL) /*!< Position of COMPARE0 field. */
#define RTC_INTENCLR_COMPARE0_Msk (0x1UL << RTC_INTENCLR_COMPARE0_Pos) /*!< Bit mask of COMPARE0 field. */
#define RTC_INTENCLR_COMPARE0_Disabled (0UL) /*!< Read: Disabled */
#define RTC_INTENCLR_COMPARE0_Enabled (1UL) /*!< Read: Enabled */
#define RTC_INTENCLR_COMPARE0_Clear (1UL) /*!< Disable */

/* Bit 1 : Write '1' to disable interrupt for event OVRFLW */
#define RTC_INTENCLR_OVRFLW_Pos (1UL) /*!< Position of OVRFLW field. */
#define RTC_INTENCLR_OVRFLW_Msk (0x1UL << RTC_INTENCLR_OVRFLW_Pos) /*!< Bit mask of OVRFLW field. */
#define RTC_INTENCLR_OVRFLW_Disabled (0UL) /*!< Read: Disabled */
#define RTC_INTENCLR_OVRFLW_Enabled (1UL) /*!< Read: Enabled */
#define RTC_INTENCLR_OVRFLW_Clear (1UL) /*!< Disable */

/* Bit 0 : Write '1' to disable interrupt for event TICK */
#define RTC_INTENCLR_TICK_Pos (0UL) /*!< Position of TICK field. */
#define RTC_INTENCLR_TICK_Msk (0x1UL << RTC_INTENCLR_TICK_Pos) /*!< Bit mask of TICK field. */
#define RTC_INTENCLR_TICK_Disabled (0UL) /*!< Read: Disabled */
#define RTC_INTENCLR_TICK_Enabled (1UL) /*!< Read: Enabled */
#define RTC_INTENCLR_TICK_Clear (1UL) /*!< Disable */

/* Register: RTC_EVTEN */
/* Description: Enable or disable event routing */

/* Bit 19 : Enable or disable event routing for event COMPARE[3] */
#define RTC_EVTEN_COMPARE3_Pos (19UL) /*!< Position of COMPARE3 field. */
#define RTC_EVTEN_COMPARE3_Msk (0x1UL << RTC_EVTEN_COMPARE3_Pos) /*!< Bit mask of COMPARE3 field. */
#define RTC_EVTEN_COMPARE3_Disabled (0UL) /*!< Disable */
#define RTC_EVTEN_COMPARE3_Enabled (1UL) /*!< Disable */

/* Bit 18 : Enable or disable event routing for event COMPARE[2] */
#define RTC_EVTEN_COMPARE2_Pos (18UL) /*!< Position of COMPARE2 field. */
#define RTC_EVTEN_COMPARE2_Msk (0x1UL << RTC_EVTEN_COMPARE2_Pos) /*!< Bit mask of COMPARE2 field. */
#define RTC_EVTEN_COMPARE2_Disabled (0UL) /*!< Disable */
#define RTC_EVTEN_COMPARE2_Enabled (1UL) /*!< Disable */

/* Bit 17 : Enable or disable event routing for event COMPARE[1] */
#define RTC_EVTEN_COMPARE1_Pos (17UL) /*!< Position of COMPARE1 field. */
#define RTC_EVTEN_COMPARE1_Msk (0x1UL << RTC_EVTEN_COMPARE1_Pos) /*!< Bit mask of COMPARE1 field. */
#define RTC_EVTEN_COMPARE1_Disabled (0UL) /*!< Disable */
#define RTC_EVTEN_COMPARE1_Enabled (1UL) /*!< Disable */

/* Bit 16 : Enable or disable event routing for event COMPARE[0] */
#define RTC_EVTEN_COMPARE0_Pos (16UL) /*!< Position of COMPARE0 field. */
#define RTC_EVTEN_COMPARE0_Msk (0x1UL << RTC_EVTEN_COMPARE0_Pos) /*!< Bit mask of COMPARE0 field. */
#define RTC_EVTEN_COMPARE0_Disabled (0UL) /*!< Disable */
#define RTC_EVTEN_COMPARE0_Enabled (1UL) /*!< Disable */

/* Bit 1 : Enable or disable event routing for event OVRFLW */
#define RTC_EVTEN_OVRFLW_Pos (1UL) /*!< Position of OVRFLW field. */
#define RTC_EVTEN_OVRFLW_Msk (0x1UL << RTC_EVTEN_OVRFLW_Pos) /*!< Bit mask of OVRFLW field. */
#define RTC_EVTEN_OVRFLW_Disabled (0UL) /*!< Disable */
#define RTC_EVTEN_OVRFLW_Enabled (1UL) /*!< Disable */

/* Bit 0 : Enable or disable event routing for event TICK */
#define RTC_EVTEN_TICK_Pos (0UL) /*!< Position of TICK field. */
#define RTC_EVTEN_TICK_Msk (0x1UL << RTC_EVTEN_TICK_Pos) /*!< Bit mask of TICK field. */
#define RTC_EVTEN_TICK_Disabled (0UL) /*!< Disable */
#define RTC_EVTEN_TICK_Enabled (1UL) /*!< Disable */

/* Register: RTC_EVTENSET */
/* Description: Enable event routing */

/* Bit 19 : Write '1' to enable event routing for event COMPARE[3] */
#define RTC_EVTENSET_COMPARE3_Pos (19UL) /*!< Position of COMPARE3 field. */
#define RTC_EVTENSET_COMPARE3_Msk (0x1UL << RTC_EVTENSET_COMPARE3_Pos) /*!< Bit mask of COMPARE3 field. */
#define RTC_EVTENSET_COMPARE3_Disabled (0UL) /*!< Read: Disabled */
#define RTC_EVTENSET_COMPARE3_Enabled (1UL) /*!< Read: Enabled */
#define RTC_EVTENSET_COMPARE3_Set (1UL) /*!< Enable */

/* Bit 18 : Write '1' to enable event routing for event COMPARE[2] */
#define RTC_EVTENSET_COMPARE2_Pos (18UL) /*!< Position of COMPARE2 field. */
#define RTC_EVTENSET_COMPARE2_Msk (0x1UL << RTC_EVTENSET_COMPARE2_Pos) /*!< Bit mask of COMPARE2 field. */
#define RTC_EVTENSET_COMPARE2_Disabled (0UL) /*!< Read: Disabled */
#define RTC_EVTENSET_COMPARE2_Enabled (1UL) /*!< Read: Enabled */
#define RTC_EVTENSET_COMPARE2_Set (1UL) /*!< Enable */

/* Bit 17 : Write '1' to enable event routing for event COMPARE[1] */
#define RTC_EVTENSET_COMPARE1_Pos (17UL) /*!< Position of COMPARE1 field. */
#define RTC_EVTENSET_COMPARE1_Msk (0x1UL << RTC_EVTENSET_COMPARE1_Pos) /*!< Bit mask of COMPARE1 field. */
#define RTC_EVTENSET_COMPARE1_Disabled (0UL) /*!< Read: Disabled */
#define RTC_EVTENSET_COMPARE1_Enabled (1UL) /*!< Read: Enabled */
#define RTC_EVTENSET_COMPARE1_Set (1UL) /*!< Enable */

/* Bit 16 : Write '1' to enable event routing for event COMPARE[0] */
#define RTC_EVTENSET_COMPARE0_Pos (16UL) /*!< Position of COMPARE0 field. */
#define RTC_EVTENSET_COMPARE0_Msk (0x1UL << RTC_EVTENSET_COMPARE0_Pos) /*!< Bit mask of COMPARE0 field. */
#define RTC_EVTENSET_COMPARE0_Disabled (0UL) /*!< Read: Disabled */
#define RTC_EVTENSET_COMPARE0_Enabled (1UL) /*!< Read: Enabled */
#define RTC_EVTENSET_COMPARE0_Set (1UL) /*!< Enable */

/* Bit 1 : Write '1' to enable event routing for event OVRFLW */
#define RTC_EVTENSET_OVRFLW_Pos (1UL) /*!< Position of OVRFLW field. */
#define RTC_EVTENSET_OVRFLW_Msk (0x1UL << RTC_EVTENSET_OVRFLW_Pos) /*!< Bit mask of OVRFLW field. */
#define RTC_EVTENSET_OVRFLW_Disabled (0UL) /*!< Read: Disabled */
#define RTC_EVTENSET_OVRFLW_Enabled (1UL) /*!< Read: Enabled */
#define RTC_EVTENSET_OVRFLW_Set (1UL) /*!< Enable */

/* Bit 0 : Write '1' to enable event routing for event TICK */
#define RTC_EVTENSET_TICK_Pos (0UL) /*!< Position of TICK field. */
#define RTC_EVTENSET_TICK_Msk (0x1UL << RTC_EVTENSET_TICK_Pos) /*!< Bit mask of TICK field. */
#define RTC_EVTENSET_TICK_Disabled (0UL) /*!< Read: Disabled */
#define RTC_EVTENSET_TICK_Enabled (1UL) /*!< Read: Enabled */
#define RTC_EVTENSET_TICK_Set (1UL) /*!< Enable */

/* Register: RTC_EVTENCLR */
/* Description: Disable event routing */

/* Bit 19 : Write '1' to disable event routing for event COMPARE[3] */
#define RTC_EVTENCLR_COMPARE3_Pos (19UL) /*!< Position of COMPARE3 field. */
#define RTC_EVTENCLR_COMPARE3_Msk (0x1UL << RTC_EVTENCLR_COMPARE3_Pos) /*!< Bit mask of COMPARE3 field. */
#define RTC_EVTENCLR_COMPARE3_Disabled (0UL) /*!< Read: Disabled */
#define RTC_EVTENCLR_COMPARE3_Enabled (1UL) /*!< Read: Enabled */
#define RTC_EVTENCLR_COMPARE3_Clear (1UL) /*!< Disable */

/* Bit 18 : Write '1' to disable event routing for event COMPARE[2] */
#define RTC_EVTENCLR_COMPARE2_Pos (18UL) /*!< Position of COMPARE2 field. */
#define RTC_EVTENCLR_COMPARE2_Msk (0x1UL << RTC_EVTENCLR_COMPARE2_Pos) /*!< Bit mask of COMPARE2 field. */
#define RTC_EVTENCLR_COMPARE2_Disabled (0UL) /*!< Read: Disabled */
#define RTC_EVTENCLR_COMPARE2_Enabled (1UL) /*!< Read: Enabled */
#define RTC_EVTENCLR_COMPARE2_Clear (1UL) /*!< Disable */

/* Bit 17 : Write '1' to disable event routing for event COMPARE[1] */
#define RTC_EVTENCLR_COMPARE1_Pos (17UL) /*!< Position of COMPARE1 field. */
#define RTC_EVTENCLR_COMPARE1_Msk (0x1UL << RTC_EVTENCLR_COMPARE1_Pos) /*!< Bit mask of COMPARE1 field. */
#define RTC_EVTENCLR_COMPARE1_Disabled (0UL) /*!< Read: Disabled */
#define RTC_EVTENCLR_COMPARE1_Enabled (1UL) /*!< Read: Enabled */
#define RTC_EVTENCLR_COMPARE1_Clear (1UL) /*!< Disable */

/* Bit 16 : Write '1' to disable event routing for event COMPARE[0] */
#define RTC_EVTENCLR_COMPARE0_Pos (16UL) /*!< Position of COMPARE0 field. */
#define RTC_EVTENCLR_COMPARE0_Msk (0x1UL << RTC_EVTENCLR_COMPARE0_Pos) /*!< Bit mask of COMPARE0 field. */
#define RTC_EVTENCLR_COMPARE0_Disabled (0UL) /*!< Read: Disabled */
#define RTC_EVTENCLR_COMPARE0_Enabled (1UL) /*!< Read: Enabled */
#define RTC_EVTENCLR_COMPARE0_Clear (1UL) /*!< Disable */

/* Bit 1 : Write '1' to disable event routing for event OVRFLW */
#define RTC_EVTENCLR_OVRFLW_Pos (1UL) /*!< Position of OVRFLW field. */
#define RTC_EVTENCLR_OVRFLW_Msk (0x1UL << RTC_EVTENCLR_OVRFLW_Pos) /*!< Bit mask of OVRFLW field. */
#define RTC_EVTENCLR_OVRFLW_Disabled (0UL) /*!< Read: Disabled */
#define RTC_EVTENCLR_OVRFLW_Enabled (1UL) /*!< Read: Enabled */
#define RTC_EVTENCLR_OVRFLW_Clear (1UL) /*!< Disable */

/* Bit 0 : Write '1' to disable event routing for event TICK */
#define RTC_EVTENCLR_TICK_Pos (0UL) /*!< Position of TICK field. */
#define RTC_EVTENCLR_TICK_Msk (0x1UL << RTC_EVTENCLR_TICK_Pos) /*!< Bit mask of TICK field. */
#define RTC_EVTENCLR_TICK_Disabled (0UL) /*!< Read: Disabled */
#define RTC_EVTENCLR_TICK_Enabled (1UL) /*!< Read: Enabled */
#define RTC_EVTENCLR_TICK_Clear (1UL) /*!< Disable */

/* Register: RTC_COUNTER */
/* Description: Current counter value */

/* Bits 23..0 : Counter value */
#define RTC_COUNTER_COUNTER_Pos (0UL) /*!< Position of COUNTER field. */
#define RTC_COUNTER_COUNTER_Msk (0xFFFFFFUL << RTC_COUNTER_COUNTER_Pos) /*!< Bit mask of COUNTER field. */

/* Register: RTC_PRESCALER */
/* Description: 12-bit prescaler for counter frequency (32768/(PRESCALER+1)). Must be written when RTC is stopped. */

/* Bits 11..0 : Prescaler value */
#define RTC_PRESCALER_PRESCALER_Pos (0UL) /*!< Position of PRESCALER field. */
#define RTC_PRESCALER_PRESCALER_Msk (0xFFFUL << RTC_PRESCALER_PRESCALER_Pos) /*!< Bit mask of PRESCALER field. */

/* Register: RTC_CC */
/* Description: Description collection: Compare register n */

/* Bits 23..0 : Compare value */
#define RTC_CC_COMPARE_Pos (0UL) /*!< Position of COMPARE field. */
#define RTC_CC_COMPARE_Msk (0xFFFFFFUL << RTC_CC_COMPARE_Pos) /*!< Bit mask of COMPARE field. */


/* Peripheral: SAADC */
/* Description: Analog to Digital Converter 0 */

/* Register: SAADC_TASKS_START */
/* Description: Start the ADC and prepare the result buffer in RAM */

/* Bit 0 : Start the ADC and prepare the result buffer in RAM */
#define SAADC_TASKS_START_TASKS_START_Pos (0UL) /*!< Position of TASKS_START field. */
#define SAADC_TASKS_START_TASKS_START_Msk (0x1UL << SAADC_TASKS_START_TASKS_START_Pos) /*!< Bit mask of TASKS_START field. */
#define SAADC_TASKS_START_TASKS_START_Trigger (1UL) /*!< Trigger task */

/* Register: SAADC_TASKS_SAMPLE */
/* Description: Take one ADC sample, if scan is enabled all channels are sampled */

/* Bit 0 : Take one ADC sample, if scan is enabled all channels are sampled */
#define SAADC_TASKS_SAMPLE_TASKS_SAMPLE_Pos (0UL) /*!< Position of TASKS_SAMPLE field. */
#define SAADC_TASKS_SAMPLE_TASKS_SAMPLE_Msk (0x1UL << SAADC_TASKS_SAMPLE_TASKS_SAMPLE_Pos) /*!< Bit mask of TASKS_SAMPLE field. */
#define SAADC_TASKS_SAMPLE_TASKS_SAMPLE_Trigger (1UL) /*!< Trigger task */

/* Register: SAADC_TASKS_STOP */
/* Description: Stop the ADC and terminate any on-going conversion */

/* Bit 0 : Stop the ADC and terminate any on-going conversion */
#define SAADC_TASKS_STOP_TASKS_STOP_Pos (0UL) /*!< Position of TASKS_STOP field. */
#define SAADC_TASKS_STOP_TASKS_STOP_Msk (0x1UL << SAADC_TASKS_STOP_TASKS_STOP_Pos) /*!< Bit mask of TASKS_STOP field. */
#define SAADC_TASKS_STOP_TASKS_STOP_Trigger (1UL) /*!< Trigger task */

/* Register: SAADC_TASKS_CALIBRATEOFFSET */
/* Description: Starts offset auto-calibration */

/* Bit 0 : Starts offset auto-calibration */
#define SAADC_TASKS_CALIBRATEOFFSET_TASKS_CALIBRATEOFFSET_Pos (0UL) /*!< Position of TASKS_CALIBRATEOFFSET field. */
#define SAADC_TASKS_CALIBRATEOFFSET_TASKS_CALIBRATEOFFSET_Msk (0x1UL << SAADC_TASKS_CALIBRATEOFFSET_TASKS_CALIBRATEOFFSET_Pos) /*!< Bit mask of TASKS_CALIBRATEOFFSET field. */
#define SAADC_TASKS_CALIBRATEOFFSET_TASKS_CALIBRATEOFFSET_Trigger (1UL) /*!< Trigger task */

/* Register: SAADC_SUBSCRIBE_START */
/* Description: Subscribe configuration for task START */

/* Bit 31 :   */
#define SAADC_SUBSCRIBE_START_EN_Pos (31UL) /*!< Position of EN field. */
#define SAADC_SUBSCRIBE_START_EN_Msk (0x1UL << SAADC_SUBSCRIBE_START_EN_Pos) /*!< Bit mask of EN field. */
#define SAADC_SUBSCRIBE_START_EN_Disabled (0UL) /*!< Disable subscription */
#define SAADC_SUBSCRIBE_START_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task START will subscribe to */
#define SAADC_SUBSCRIBE_START_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SAADC_SUBSCRIBE_START_CHIDX_Msk (0xFUL << SAADC_SUBSCRIBE_START_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SAADC_SUBSCRIBE_SAMPLE */
/* Description: Subscribe configuration for task SAMPLE */

/* Bit 31 :   */
#define SAADC_SUBSCRIBE_SAMPLE_EN_Pos (31UL) /*!< Position of EN field. */
#define SAADC_SUBSCRIBE_SAMPLE_EN_Msk (0x1UL << SAADC_SUBSCRIBE_SAMPLE_EN_Pos) /*!< Bit mask of EN field. */
#define SAADC_SUBSCRIBE_SAMPLE_EN_Disabled (0UL) /*!< Disable subscription */
#define SAADC_SUBSCRIBE_SAMPLE_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task SAMPLE will subscribe to */
#define SAADC_SUBSCRIBE_SAMPLE_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SAADC_SUBSCRIBE_SAMPLE_CHIDX_Msk (0xFUL << SAADC_SUBSCRIBE_SAMPLE_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SAADC_SUBSCRIBE_STOP */
/* Description: Subscribe configuration for task STOP */

/* Bit 31 :   */
#define SAADC_SUBSCRIBE_STOP_EN_Pos (31UL) /*!< Position of EN field. */
#define SAADC_SUBSCRIBE_STOP_EN_Msk (0x1UL << SAADC_SUBSCRIBE_STOP_EN_Pos) /*!< Bit mask of EN field. */
#define SAADC_SUBSCRIBE_STOP_EN_Disabled (0UL) /*!< Disable subscription */
#define SAADC_SUBSCRIBE_STOP_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task STOP will subscribe to */
#define SAADC_SUBSCRIBE_STOP_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SAADC_SUBSCRIBE_STOP_CHIDX_Msk (0xFUL << SAADC_SUBSCRIBE_STOP_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SAADC_SUBSCRIBE_CALIBRATEOFFSET */
/* Description: Subscribe configuration for task CALIBRATEOFFSET */

/* Bit 31 :   */
#define SAADC_SUBSCRIBE_CALIBRATEOFFSET_EN_Pos (31UL) /*!< Position of EN field. */
#define SAADC_SUBSCRIBE_CALIBRATEOFFSET_EN_Msk (0x1UL << SAADC_SUBSCRIBE_CALIBRATEOFFSET_EN_Pos) /*!< Bit mask of EN field. */
#define SAADC_SUBSCRIBE_CALIBRATEOFFSET_EN_Disabled (0UL) /*!< Disable subscription */
#define SAADC_SUBSCRIBE_CALIBRATEOFFSET_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task CALIBRATEOFFSET will subscribe to */
#define SAADC_SUBSCRIBE_CALIBRATEOFFSET_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SAADC_SUBSCRIBE_CALIBRATEOFFSET_CHIDX_Msk (0xFUL << SAADC_SUBSCRIBE_CALIBRATEOFFSET_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SAADC_EVENTS_STARTED */
/* Description: The ADC has started */

/* Bit 0 : The ADC has started */
#define SAADC_EVENTS_STARTED_EVENTS_STARTED_Pos (0UL) /*!< Position of EVENTS_STARTED field. */
#define SAADC_EVENTS_STARTED_EVENTS_STARTED_Msk (0x1UL << SAADC_EVENTS_STARTED_EVENTS_STARTED_Pos) /*!< Bit mask of EVENTS_STARTED field. */
#define SAADC_EVENTS_STARTED_EVENTS_STARTED_NotGenerated (0UL) /*!< Event not generated */
#define SAADC_EVENTS_STARTED_EVENTS_STARTED_Generated (1UL) /*!< Event generated */

/* Register: SAADC_EVENTS_END */
/* Description: The ADC has filled up the Result buffer */

/* Bit 0 : The ADC has filled up the Result buffer */
#define SAADC_EVENTS_END_EVENTS_END_Pos (0UL) /*!< Position of EVENTS_END field. */
#define SAADC_EVENTS_END_EVENTS_END_Msk (0x1UL << SAADC_EVENTS_END_EVENTS_END_Pos) /*!< Bit mask of EVENTS_END field. */
#define SAADC_EVENTS_END_EVENTS_END_NotGenerated (0UL) /*!< Event not generated */
#define SAADC_EVENTS_END_EVENTS_END_Generated (1UL) /*!< Event generated */

/* Register: SAADC_EVENTS_DONE */
/* Description: A conversion task has been completed. Depending on the mode, multiple conversions might be needed for a result to be transferred to RAM. */

/* Bit 0 : A conversion task has been completed. Depending on the mode, multiple conversions might be needed for a result to be transferred to RAM. */
#define SAADC_EVENTS_DONE_EVENTS_DONE_Pos (0UL) /*!< Position of EVENTS_DONE field. */
#define SAADC_EVENTS_DONE_EVENTS_DONE_Msk (0x1UL << SAADC_EVENTS_DONE_EVENTS_DONE_Pos) /*!< Bit mask of EVENTS_DONE field. */
#define SAADC_EVENTS_DONE_EVENTS_DONE_NotGenerated (0UL) /*!< Event not generated */
#define SAADC_EVENTS_DONE_EVENTS_DONE_Generated (1UL) /*!< Event generated */

/* Register: SAADC_EVENTS_RESULTDONE */
/* Description: A result is ready to get transferred to RAM. */

/* Bit 0 : A result is ready to get transferred to RAM. */
#define SAADC_EVENTS_RESULTDONE_EVENTS_RESULTDONE_Pos (0UL) /*!< Position of EVENTS_RESULTDONE field. */
#define SAADC_EVENTS_RESULTDONE_EVENTS_RESULTDONE_Msk (0x1UL << SAADC_EVENTS_RESULTDONE_EVENTS_RESULTDONE_Pos) /*!< Bit mask of EVENTS_RESULTDONE field. */
#define SAADC_EVENTS_RESULTDONE_EVENTS_RESULTDONE_NotGenerated (0UL) /*!< Event not generated */
#define SAADC_EVENTS_RESULTDONE_EVENTS_RESULTDONE_Generated (1UL) /*!< Event generated */

/* Register: SAADC_EVENTS_CALIBRATEDONE */
/* Description: Calibration is complete */

/* Bit 0 : Calibration is complete */
#define SAADC_EVENTS_CALIBRATEDONE_EVENTS_CALIBRATEDONE_Pos (0UL) /*!< Position of EVENTS_CALIBRATEDONE field. */
#define SAADC_EVENTS_CALIBRATEDONE_EVENTS_CALIBRATEDONE_Msk (0x1UL << SAADC_EVENTS_CALIBRATEDONE_EVENTS_CALIBRATEDONE_Pos) /*!< Bit mask of EVENTS_CALIBRATEDONE field. */
#define SAADC_EVENTS_CALIBRATEDONE_EVENTS_CALIBRATEDONE_NotGenerated (0UL) /*!< Event not generated */
#define SAADC_EVENTS_CALIBRATEDONE_EVENTS_CALIBRATEDONE_Generated (1UL) /*!< Event generated */

/* Register: SAADC_EVENTS_STOPPED */
/* Description: The ADC has stopped */

/* Bit 0 : The ADC has stopped */
#define SAADC_EVENTS_STOPPED_EVENTS_STOPPED_Pos (0UL) /*!< Position of EVENTS_STOPPED field. */
#define SAADC_EVENTS_STOPPED_EVENTS_STOPPED_Msk (0x1UL << SAADC_EVENTS_STOPPED_EVENTS_STOPPED_Pos) /*!< Bit mask of EVENTS_STOPPED field. */
#define SAADC_EVENTS_STOPPED_EVENTS_STOPPED_NotGenerated (0UL) /*!< Event not generated */
#define SAADC_EVENTS_STOPPED_EVENTS_STOPPED_Generated (1UL) /*!< Event generated */

/* Register: SAADC_EVENTS_CH_LIMITH */
/* Description: Description cluster: Last results is equal or above CH[n].LIMIT.HIGH */

/* Bit 0 : Last results is equal or above CH[n].LIMIT.HIGH */
#define SAADC_EVENTS_CH_LIMITH_LIMITH_Pos (0UL) /*!< Position of LIMITH field. */
#define SAADC_EVENTS_CH_LIMITH_LIMITH_Msk (0x1UL << SAADC_EVENTS_CH_LIMITH_LIMITH_Pos) /*!< Bit mask of LIMITH field. */
#define SAADC_EVENTS_CH_LIMITH_LIMITH_NotGenerated (0UL) /*!< Event not generated */
#define SAADC_EVENTS_CH_LIMITH_LIMITH_Generated (1UL) /*!< Event generated */

/* Register: SAADC_EVENTS_CH_LIMITL */
/* Description: Description cluster: Last results is equal or below CH[n].LIMIT.LOW */

/* Bit 0 : Last results is equal or below CH[n].LIMIT.LOW */
#define SAADC_EVENTS_CH_LIMITL_LIMITL_Pos (0UL) /*!< Position of LIMITL field. */
#define SAADC_EVENTS_CH_LIMITL_LIMITL_Msk (0x1UL << SAADC_EVENTS_CH_LIMITL_LIMITL_Pos) /*!< Bit mask of LIMITL field. */
#define SAADC_EVENTS_CH_LIMITL_LIMITL_NotGenerated (0UL) /*!< Event not generated */
#define SAADC_EVENTS_CH_LIMITL_LIMITL_Generated (1UL) /*!< Event generated */

/* Register: SAADC_PUBLISH_STARTED */
/* Description: Publish configuration for event STARTED */

/* Bit 31 :   */
#define SAADC_PUBLISH_STARTED_EN_Pos (31UL) /*!< Position of EN field. */
#define SAADC_PUBLISH_STARTED_EN_Msk (0x1UL << SAADC_PUBLISH_STARTED_EN_Pos) /*!< Bit mask of EN field. */
#define SAADC_PUBLISH_STARTED_EN_Disabled (0UL) /*!< Disable publishing */
#define SAADC_PUBLISH_STARTED_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event STARTED will publish to. */
#define SAADC_PUBLISH_STARTED_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SAADC_PUBLISH_STARTED_CHIDX_Msk (0xFUL << SAADC_PUBLISH_STARTED_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SAADC_PUBLISH_END */
/* Description: Publish configuration for event END */

/* Bit 31 :   */
#define SAADC_PUBLISH_END_EN_Pos (31UL) /*!< Position of EN field. */
#define SAADC_PUBLISH_END_EN_Msk (0x1UL << SAADC_PUBLISH_END_EN_Pos) /*!< Bit mask of EN field. */
#define SAADC_PUBLISH_END_EN_Disabled (0UL) /*!< Disable publishing */
#define SAADC_PUBLISH_END_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event END will publish to. */
#define SAADC_PUBLISH_END_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SAADC_PUBLISH_END_CHIDX_Msk (0xFUL << SAADC_PUBLISH_END_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SAADC_PUBLISH_DONE */
/* Description: Publish configuration for event DONE */

/* Bit 31 :   */
#define SAADC_PUBLISH_DONE_EN_Pos (31UL) /*!< Position of EN field. */
#define SAADC_PUBLISH_DONE_EN_Msk (0x1UL << SAADC_PUBLISH_DONE_EN_Pos) /*!< Bit mask of EN field. */
#define SAADC_PUBLISH_DONE_EN_Disabled (0UL) /*!< Disable publishing */
#define SAADC_PUBLISH_DONE_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event DONE will publish to. */
#define SAADC_PUBLISH_DONE_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SAADC_PUBLISH_DONE_CHIDX_Msk (0xFUL << SAADC_PUBLISH_DONE_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SAADC_PUBLISH_RESULTDONE */
/* Description: Publish configuration for event RESULTDONE */

/* Bit 31 :   */
#define SAADC_PUBLISH_RESULTDONE_EN_Pos (31UL) /*!< Position of EN field. */
#define SAADC_PUBLISH_RESULTDONE_EN_Msk (0x1UL << SAADC_PUBLISH_RESULTDONE_EN_Pos) /*!< Bit mask of EN field. */
#define SAADC_PUBLISH_RESULTDONE_EN_Disabled (0UL) /*!< Disable publishing */
#define SAADC_PUBLISH_RESULTDONE_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event RESULTDONE will publish to. */
#define SAADC_PUBLISH_RESULTDONE_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SAADC_PUBLISH_RESULTDONE_CHIDX_Msk (0xFUL << SAADC_PUBLISH_RESULTDONE_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SAADC_PUBLISH_CALIBRATEDONE */
/* Description: Publish configuration for event CALIBRATEDONE */

/* Bit 31 :   */
#define SAADC_PUBLISH_CALIBRATEDONE_EN_Pos (31UL) /*!< Position of EN field. */
#define SAADC_PUBLISH_CALIBRATEDONE_EN_Msk (0x1UL << SAADC_PUBLISH_CALIBRATEDONE_EN_Pos) /*!< Bit mask of EN field. */
#define SAADC_PUBLISH_CALIBRATEDONE_EN_Disabled (0UL) /*!< Disable publishing */
#define SAADC_PUBLISH_CALIBRATEDONE_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event CALIBRATEDONE will publish to. */
#define SAADC_PUBLISH_CALIBRATEDONE_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SAADC_PUBLISH_CALIBRATEDONE_CHIDX_Msk (0xFUL << SAADC_PUBLISH_CALIBRATEDONE_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SAADC_PUBLISH_STOPPED */
/* Description: Publish configuration for event STOPPED */

/* Bit 31 :   */
#define SAADC_PUBLISH_STOPPED_EN_Pos (31UL) /*!< Position of EN field. */
#define SAADC_PUBLISH_STOPPED_EN_Msk (0x1UL << SAADC_PUBLISH_STOPPED_EN_Pos) /*!< Bit mask of EN field. */
#define SAADC_PUBLISH_STOPPED_EN_Disabled (0UL) /*!< Disable publishing */
#define SAADC_PUBLISH_STOPPED_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event STOPPED will publish to. */
#define SAADC_PUBLISH_STOPPED_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SAADC_PUBLISH_STOPPED_CHIDX_Msk (0xFUL << SAADC_PUBLISH_STOPPED_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SAADC_PUBLISH_CH_LIMITH */
/* Description: Description cluster: Publish configuration for event CH[n].LIMITH */

/* Bit 31 :   */
#define SAADC_PUBLISH_CH_LIMITH_EN_Pos (31UL) /*!< Position of EN field. */
#define SAADC_PUBLISH_CH_LIMITH_EN_Msk (0x1UL << SAADC_PUBLISH_CH_LIMITH_EN_Pos) /*!< Bit mask of EN field. */
#define SAADC_PUBLISH_CH_LIMITH_EN_Disabled (0UL) /*!< Disable publishing */
#define SAADC_PUBLISH_CH_LIMITH_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event CH[n].LIMITH will publish to. */
#define SAADC_PUBLISH_CH_LIMITH_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SAADC_PUBLISH_CH_LIMITH_CHIDX_Msk (0xFUL << SAADC_PUBLISH_CH_LIMITH_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SAADC_PUBLISH_CH_LIMITL */
/* Description: Description cluster: Publish configuration for event CH[n].LIMITL */

/* Bit 31 :   */
#define SAADC_PUBLISH_CH_LIMITL_EN_Pos (31UL) /*!< Position of EN field. */
#define SAADC_PUBLISH_CH_LIMITL_EN_Msk (0x1UL << SAADC_PUBLISH_CH_LIMITL_EN_Pos) /*!< Bit mask of EN field. */
#define SAADC_PUBLISH_CH_LIMITL_EN_Disabled (0UL) /*!< Disable publishing */
#define SAADC_PUBLISH_CH_LIMITL_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event CH[n].LIMITL will publish to. */
#define SAADC_PUBLISH_CH_LIMITL_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SAADC_PUBLISH_CH_LIMITL_CHIDX_Msk (0xFUL << SAADC_PUBLISH_CH_LIMITL_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SAADC_INTEN */
/* Description: Enable or disable interrupt */

/* Bit 21 : Enable or disable interrupt for event CH7LIMITL */
#define SAADC_INTEN_CH7LIMITL_Pos (21UL) /*!< Position of CH7LIMITL field. */
#define SAADC_INTEN_CH7LIMITL_Msk (0x1UL << SAADC_INTEN_CH7LIMITL_Pos) /*!< Bit mask of CH7LIMITL field. */
#define SAADC_INTEN_CH7LIMITL_Disabled (0UL) /*!< Disable */
#define SAADC_INTEN_CH7LIMITL_Enabled (1UL) /*!< Enable */

/* Bit 20 : Enable or disable interrupt for event CH7LIMITH */
#define SAADC_INTEN_CH7LIMITH_Pos (20UL) /*!< Position of CH7LIMITH field. */
#define SAADC_INTEN_CH7LIMITH_Msk (0x1UL << SAADC_INTEN_CH7LIMITH_Pos) /*!< Bit mask of CH7LIMITH field. */
#define SAADC_INTEN_CH7LIMITH_Disabled (0UL) /*!< Disable */
#define SAADC_INTEN_CH7LIMITH_Enabled (1UL) /*!< Enable */

/* Bit 19 : Enable or disable interrupt for event CH6LIMITL */
#define SAADC_INTEN_CH6LIMITL_Pos (19UL) /*!< Position of CH6LIMITL field. */
#define SAADC_INTEN_CH6LIMITL_Msk (0x1UL << SAADC_INTEN_CH6LIMITL_Pos) /*!< Bit mask of CH6LIMITL field. */
#define SAADC_INTEN_CH6LIMITL_Disabled (0UL) /*!< Disable */
#define SAADC_INTEN_CH6LIMITL_Enabled (1UL) /*!< Enable */

/* Bit 18 : Enable or disable interrupt for event CH6LIMITH */
#define SAADC_INTEN_CH6LIMITH_Pos (18UL) /*!< Position of CH6LIMITH field. */
#define SAADC_INTEN_CH6LIMITH_Msk (0x1UL << SAADC_INTEN_CH6LIMITH_Pos) /*!< Bit mask of CH6LIMITH field. */
#define SAADC_INTEN_CH6LIMITH_Disabled (0UL) /*!< Disable */
#define SAADC_INTEN_CH6LIMITH_Enabled (1UL) /*!< Enable */

/* Bit 17 : Enable or disable interrupt for event CH5LIMITL */
#define SAADC_INTEN_CH5LIMITL_Pos (17UL) /*!< Position of CH5LIMITL field. */
#define SAADC_INTEN_CH5LIMITL_Msk (0x1UL << SAADC_INTEN_CH5LIMITL_Pos) /*!< Bit mask of CH5LIMITL field. */
#define SAADC_INTEN_CH5LIMITL_Disabled (0UL) /*!< Disable */
#define SAADC_INTEN_CH5LIMITL_Enabled (1UL) /*!< Enable */

/* Bit 16 : Enable or disable interrupt for event CH5LIMITH */
#define SAADC_INTEN_CH5LIMITH_Pos (16UL) /*!< Position of CH5LIMITH field. */
#define SAADC_INTEN_CH5LIMITH_Msk (0x1UL << SAADC_INTEN_CH5LIMITH_Pos) /*!< Bit mask of CH5LIMITH field. */
#define SAADC_INTEN_CH5LIMITH_Disabled (0UL) /*!< Disable */
#define SAADC_INTEN_CH5LIMITH_Enabled (1UL) /*!< Enable */

/* Bit 15 : Enable or disable interrupt for event CH4LIMITL */
#define SAADC_INTEN_CH4LIMITL_Pos (15UL) /*!< Position of CH4LIMITL field. */
#define SAADC_INTEN_CH4LIMITL_Msk (0x1UL << SAADC_INTEN_CH4LIMITL_Pos) /*!< Bit mask of CH4LIMITL field. */
#define SAADC_INTEN_CH4LIMITL_Disabled (0UL) /*!< Disable */
#define SAADC_INTEN_CH4LIMITL_Enabled (1UL) /*!< Enable */

/* Bit 14 : Enable or disable interrupt for event CH4LIMITH */
#define SAADC_INTEN_CH4LIMITH_Pos (14UL) /*!< Position of CH4LIMITH field. */
#define SAADC_INTEN_CH4LIMITH_Msk (0x1UL << SAADC_INTEN_CH4LIMITH_Pos) /*!< Bit mask of CH4LIMITH field. */
#define SAADC_INTEN_CH4LIMITH_Disabled (0UL) /*!< Disable */
#define SAADC_INTEN_CH4LIMITH_Enabled (1UL) /*!< Enable */

/* Bit 13 : Enable or disable interrupt for event CH3LIMITL */
#define SAADC_INTEN_CH3LIMITL_Pos (13UL) /*!< Position of CH3LIMITL field. */
#define SAADC_INTEN_CH3LIMITL_Msk (0x1UL << SAADC_INTEN_CH3LIMITL_Pos) /*!< Bit mask of CH3LIMITL field. */
#define SAADC_INTEN_CH3LIMITL_Disabled (0UL) /*!< Disable */
#define SAADC_INTEN_CH3LIMITL_Enabled (1UL) /*!< Enable */

/* Bit 12 : Enable or disable interrupt for event CH3LIMITH */
#define SAADC_INTEN_CH3LIMITH_Pos (12UL) /*!< Position of CH3LIMITH field. */
#define SAADC_INTEN_CH3LIMITH_Msk (0x1UL << SAADC_INTEN_CH3LIMITH_Pos) /*!< Bit mask of CH3LIMITH field. */
#define SAADC_INTEN_CH3LIMITH_Disabled (0UL) /*!< Disable */
#define SAADC_INTEN_CH3LIMITH_Enabled (1UL) /*!< Enable */

/* Bit 11 : Enable or disable interrupt for event CH2LIMITL */
#define SAADC_INTEN_CH2LIMITL_Pos (11UL) /*!< Position of CH2LIMITL field. */
#define SAADC_INTEN_CH2LIMITL_Msk (0x1UL << SAADC_INTEN_CH2LIMITL_Pos) /*!< Bit mask of CH2LIMITL field. */
#define SAADC_INTEN_CH2LIMITL_Disabled (0UL) /*!< Disable */
#define SAADC_INTEN_CH2LIMITL_Enabled (1UL) /*!< Enable */

/* Bit 10 : Enable or disable interrupt for event CH2LIMITH */
#define SAADC_INTEN_CH2LIMITH_Pos (10UL) /*!< Position of CH2LIMITH field. */
#define SAADC_INTEN_CH2LIMITH_Msk (0x1UL << SAADC_INTEN_CH2LIMITH_Pos) /*!< Bit mask of CH2LIMITH field. */
#define SAADC_INTEN_CH2LIMITH_Disabled (0UL) /*!< Disable */
#define SAADC_INTEN_CH2LIMITH_Enabled (1UL) /*!< Enable */

/* Bit 9 : Enable or disable interrupt for event CH1LIMITL */
#define SAADC_INTEN_CH1LIMITL_Pos (9UL) /*!< Position of CH1LIMITL field. */
#define SAADC_INTEN_CH1LIMITL_Msk (0x1UL << SAADC_INTEN_CH1LIMITL_Pos) /*!< Bit mask of CH1LIMITL field. */
#define SAADC_INTEN_CH1LIMITL_Disabled (0UL) /*!< Disable */
#define SAADC_INTEN_CH1LIMITL_Enabled (1UL) /*!< Enable */

/* Bit 8 : Enable or disable interrupt for event CH1LIMITH */
#define SAADC_INTEN_CH1LIMITH_Pos (8UL) /*!< Position of CH1LIMITH field. */
#define SAADC_INTEN_CH1LIMITH_Msk (0x1UL << SAADC_INTEN_CH1LIMITH_Pos) /*!< Bit mask of CH1LIMITH field. */
#define SAADC_INTEN_CH1LIMITH_Disabled (0UL) /*!< Disable */
#define SAADC_INTEN_CH1LIMITH_Enabled (1UL) /*!< Enable */

/* Bit 7 : Enable or disable interrupt for event CH0LIMITL */
#define SAADC_INTEN_CH0LIMITL_Pos (7UL) /*!< Position of CH0LIMITL field. */
#define SAADC_INTEN_CH0LIMITL_Msk (0x1UL << SAADC_INTEN_CH0LIMITL_Pos) /*!< Bit mask of CH0LIMITL field. */
#define SAADC_INTEN_CH0LIMITL_Disabled (0UL) /*!< Disable */
#define SAADC_INTEN_CH0LIMITL_Enabled (1UL) /*!< Enable */

/* Bit 6 : Enable or disable interrupt for event CH0LIMITH */
#define SAADC_INTEN_CH0LIMITH_Pos (6UL) /*!< Position of CH0LIMITH field. */
#define SAADC_INTEN_CH0LIMITH_Msk (0x1UL << SAADC_INTEN_CH0LIMITH_Pos) /*!< Bit mask of CH0LIMITH field. */
#define SAADC_INTEN_CH0LIMITH_Disabled (0UL) /*!< Disable */
#define SAADC_INTEN_CH0LIMITH_Enabled (1UL) /*!< Enable */

/* Bit 5 : Enable or disable interrupt for event STOPPED */
#define SAADC_INTEN_STOPPED_Pos (5UL) /*!< Position of STOPPED field. */
#define SAADC_INTEN_STOPPED_Msk (0x1UL << SAADC_INTEN_STOPPED_Pos) /*!< Bit mask of STOPPED field. */
#define SAADC_INTEN_STOPPED_Disabled (0UL) /*!< Disable */
#define SAADC_INTEN_STOPPED_Enabled (1UL) /*!< Enable */

/* Bit 4 : Enable or disable interrupt for event CALIBRATEDONE */
#define SAADC_INTEN_CALIBRATEDONE_Pos (4UL) /*!< Position of CALIBRATEDONE field. */
#define SAADC_INTEN_CALIBRATEDONE_Msk (0x1UL << SAADC_INTEN_CALIBRATEDONE_Pos) /*!< Bit mask of CALIBRATEDONE field. */
#define SAADC_INTEN_CALIBRATEDONE_Disabled (0UL) /*!< Disable */
#define SAADC_INTEN_CALIBRATEDONE_Enabled (1UL) /*!< Enable */

/* Bit 3 : Enable or disable interrupt for event RESULTDONE */
#define SAADC_INTEN_RESULTDONE_Pos (3UL) /*!< Position of RESULTDONE field. */
#define SAADC_INTEN_RESULTDONE_Msk (0x1UL << SAADC_INTEN_RESULTDONE_Pos) /*!< Bit mask of RESULTDONE field. */
#define SAADC_INTEN_RESULTDONE_Disabled (0UL) /*!< Disable */
#define SAADC_INTEN_RESULTDONE_Enabled (1UL) /*!< Enable */

/* Bit 2 : Enable or disable interrupt for event DONE */
#define SAADC_INTEN_DONE_Pos (2UL) /*!< Position of DONE field. */
#define SAADC_INTEN_DONE_Msk (0x1UL << SAADC_INTEN_DONE_Pos) /*!< Bit mask of DONE field. */
#define SAADC_INTEN_DONE_Disabled (0UL) /*!< Disable */
#define SAADC_INTEN_DONE_Enabled (1UL) /*!< Enable */

/* Bit 1 : Enable or disable interrupt for event END */
#define SAADC_INTEN_END_Pos (1UL) /*!< Position of END field. */
#define SAADC_INTEN_END_Msk (0x1UL << SAADC_INTEN_END_Pos) /*!< Bit mask of END field. */
#define SAADC_INTEN_END_Disabled (0UL) /*!< Disable */
#define SAADC_INTEN_END_Enabled (1UL) /*!< Enable */

/* Bit 0 : Enable or disable interrupt for event STARTED */
#define SAADC_INTEN_STARTED_Pos (0UL) /*!< Position of STARTED field. */
#define SAADC_INTEN_STARTED_Msk (0x1UL << SAADC_INTEN_STARTED_Pos) /*!< Bit mask of STARTED field. */
#define SAADC_INTEN_STARTED_Disabled (0UL) /*!< Disable */
#define SAADC_INTEN_STARTED_Enabled (1UL) /*!< Enable */

/* Register: SAADC_INTENSET */
/* Description: Enable interrupt */

/* Bit 21 : Write '1' to enable interrupt for event CH7LIMITL */
#define SAADC_INTENSET_CH7LIMITL_Pos (21UL) /*!< Position of CH7LIMITL field. */
#define SAADC_INTENSET_CH7LIMITL_Msk (0x1UL << SAADC_INTENSET_CH7LIMITL_Pos) /*!< Bit mask of CH7LIMITL field. */
#define SAADC_INTENSET_CH7LIMITL_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENSET_CH7LIMITL_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENSET_CH7LIMITL_Set (1UL) /*!< Enable */

/* Bit 20 : Write '1' to enable interrupt for event CH7LIMITH */
#define SAADC_INTENSET_CH7LIMITH_Pos (20UL) /*!< Position of CH7LIMITH field. */
#define SAADC_INTENSET_CH7LIMITH_Msk (0x1UL << SAADC_INTENSET_CH7LIMITH_Pos) /*!< Bit mask of CH7LIMITH field. */
#define SAADC_INTENSET_CH7LIMITH_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENSET_CH7LIMITH_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENSET_CH7LIMITH_Set (1UL) /*!< Enable */

/* Bit 19 : Write '1' to enable interrupt for event CH6LIMITL */
#define SAADC_INTENSET_CH6LIMITL_Pos (19UL) /*!< Position of CH6LIMITL field. */
#define SAADC_INTENSET_CH6LIMITL_Msk (0x1UL << SAADC_INTENSET_CH6LIMITL_Pos) /*!< Bit mask of CH6LIMITL field. */
#define SAADC_INTENSET_CH6LIMITL_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENSET_CH6LIMITL_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENSET_CH6LIMITL_Set (1UL) /*!< Enable */

/* Bit 18 : Write '1' to enable interrupt for event CH6LIMITH */
#define SAADC_INTENSET_CH6LIMITH_Pos (18UL) /*!< Position of CH6LIMITH field. */
#define SAADC_INTENSET_CH6LIMITH_Msk (0x1UL << SAADC_INTENSET_CH6LIMITH_Pos) /*!< Bit mask of CH6LIMITH field. */
#define SAADC_INTENSET_CH6LIMITH_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENSET_CH6LIMITH_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENSET_CH6LIMITH_Set (1UL) /*!< Enable */

/* Bit 17 : Write '1' to enable interrupt for event CH5LIMITL */
#define SAADC_INTENSET_CH5LIMITL_Pos (17UL) /*!< Position of CH5LIMITL field. */
#define SAADC_INTENSET_CH5LIMITL_Msk (0x1UL << SAADC_INTENSET_CH5LIMITL_Pos) /*!< Bit mask of CH5LIMITL field. */
#define SAADC_INTENSET_CH5LIMITL_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENSET_CH5LIMITL_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENSET_CH5LIMITL_Set (1UL) /*!< Enable */

/* Bit 16 : Write '1' to enable interrupt for event CH5LIMITH */
#define SAADC_INTENSET_CH5LIMITH_Pos (16UL) /*!< Position of CH5LIMITH field. */
#define SAADC_INTENSET_CH5LIMITH_Msk (0x1UL << SAADC_INTENSET_CH5LIMITH_Pos) /*!< Bit mask of CH5LIMITH field. */
#define SAADC_INTENSET_CH5LIMITH_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENSET_CH5LIMITH_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENSET_CH5LIMITH_Set (1UL) /*!< Enable */

/* Bit 15 : Write '1' to enable interrupt for event CH4LIMITL */
#define SAADC_INTENSET_CH4LIMITL_Pos (15UL) /*!< Position of CH4LIMITL field. */
#define SAADC_INTENSET_CH4LIMITL_Msk (0x1UL << SAADC_INTENSET_CH4LIMITL_Pos) /*!< Bit mask of CH4LIMITL field. */
#define SAADC_INTENSET_CH4LIMITL_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENSET_CH4LIMITL_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENSET_CH4LIMITL_Set (1UL) /*!< Enable */

/* Bit 14 : Write '1' to enable interrupt for event CH4LIMITH */
#define SAADC_INTENSET_CH4LIMITH_Pos (14UL) /*!< Position of CH4LIMITH field. */
#define SAADC_INTENSET_CH4LIMITH_Msk (0x1UL << SAADC_INTENSET_CH4LIMITH_Pos) /*!< Bit mask of CH4LIMITH field. */
#define SAADC_INTENSET_CH4LIMITH_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENSET_CH4LIMITH_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENSET_CH4LIMITH_Set (1UL) /*!< Enable */

/* Bit 13 : Write '1' to enable interrupt for event CH3LIMITL */
#define SAADC_INTENSET_CH3LIMITL_Pos (13UL) /*!< Position of CH3LIMITL field. */
#define SAADC_INTENSET_CH3LIMITL_Msk (0x1UL << SAADC_INTENSET_CH3LIMITL_Pos) /*!< Bit mask of CH3LIMITL field. */
#define SAADC_INTENSET_CH3LIMITL_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENSET_CH3LIMITL_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENSET_CH3LIMITL_Set (1UL) /*!< Enable */

/* Bit 12 : Write '1' to enable interrupt for event CH3LIMITH */
#define SAADC_INTENSET_CH3LIMITH_Pos (12UL) /*!< Position of CH3LIMITH field. */
#define SAADC_INTENSET_CH3LIMITH_Msk (0x1UL << SAADC_INTENSET_CH3LIMITH_Pos) /*!< Bit mask of CH3LIMITH field. */
#define SAADC_INTENSET_CH3LIMITH_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENSET_CH3LIMITH_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENSET_CH3LIMITH_Set (1UL) /*!< Enable */

/* Bit 11 : Write '1' to enable interrupt for event CH2LIMITL */
#define SAADC_INTENSET_CH2LIMITL_Pos (11UL) /*!< Position of CH2LIMITL field. */
#define SAADC_INTENSET_CH2LIMITL_Msk (0x1UL << SAADC_INTENSET_CH2LIMITL_Pos) /*!< Bit mask of CH2LIMITL field. */
#define SAADC_INTENSET_CH2LIMITL_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENSET_CH2LIMITL_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENSET_CH2LIMITL_Set (1UL) /*!< Enable */

/* Bit 10 : Write '1' to enable interrupt for event CH2LIMITH */
#define SAADC_INTENSET_CH2LIMITH_Pos (10UL) /*!< Position of CH2LIMITH field. */
#define SAADC_INTENSET_CH2LIMITH_Msk (0x1UL << SAADC_INTENSET_CH2LIMITH_Pos) /*!< Bit mask of CH2LIMITH field. */
#define SAADC_INTENSET_CH2LIMITH_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENSET_CH2LIMITH_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENSET_CH2LIMITH_Set (1UL) /*!< Enable */

/* Bit 9 : Write '1' to enable interrupt for event CH1LIMITL */
#define SAADC_INTENSET_CH1LIMITL_Pos (9UL) /*!< Position of CH1LIMITL field. */
#define SAADC_INTENSET_CH1LIMITL_Msk (0x1UL << SAADC_INTENSET_CH1LIMITL_Pos) /*!< Bit mask of CH1LIMITL field. */
#define SAADC_INTENSET_CH1LIMITL_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENSET_CH1LIMITL_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENSET_CH1LIMITL_Set (1UL) /*!< Enable */

/* Bit 8 : Write '1' to enable interrupt for event CH1LIMITH */
#define SAADC_INTENSET_CH1LIMITH_Pos (8UL) /*!< Position of CH1LIMITH field. */
#define SAADC_INTENSET_CH1LIMITH_Msk (0x1UL << SAADC_INTENSET_CH1LIMITH_Pos) /*!< Bit mask of CH1LIMITH field. */
#define SAADC_INTENSET_CH1LIMITH_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENSET_CH1LIMITH_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENSET_CH1LIMITH_Set (1UL) /*!< Enable */

/* Bit 7 : Write '1' to enable interrupt for event CH0LIMITL */
#define SAADC_INTENSET_CH0LIMITL_Pos (7UL) /*!< Position of CH0LIMITL field. */
#define SAADC_INTENSET_CH0LIMITL_Msk (0x1UL << SAADC_INTENSET_CH0LIMITL_Pos) /*!< Bit mask of CH0LIMITL field. */
#define SAADC_INTENSET_CH0LIMITL_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENSET_CH0LIMITL_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENSET_CH0LIMITL_Set (1UL) /*!< Enable */

/* Bit 6 : Write '1' to enable interrupt for event CH0LIMITH */
#define SAADC_INTENSET_CH0LIMITH_Pos (6UL) /*!< Position of CH0LIMITH field. */
#define SAADC_INTENSET_CH0LIMITH_Msk (0x1UL << SAADC_INTENSET_CH0LIMITH_Pos) /*!< Bit mask of CH0LIMITH field. */
#define SAADC_INTENSET_CH0LIMITH_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENSET_CH0LIMITH_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENSET_CH0LIMITH_Set (1UL) /*!< Enable */

/* Bit 5 : Write '1' to enable interrupt for event STOPPED */
#define SAADC_INTENSET_STOPPED_Pos (5UL) /*!< Position of STOPPED field. */
#define SAADC_INTENSET_STOPPED_Msk (0x1UL << SAADC_INTENSET_STOPPED_Pos) /*!< Bit mask of STOPPED field. */
#define SAADC_INTENSET_STOPPED_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENSET_STOPPED_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENSET_STOPPED_Set (1UL) /*!< Enable */

/* Bit 4 : Write '1' to enable interrupt for event CALIBRATEDONE */
#define SAADC_INTENSET_CALIBRATEDONE_Pos (4UL) /*!< Position of CALIBRATEDONE field. */
#define SAADC_INTENSET_CALIBRATEDONE_Msk (0x1UL << SAADC_INTENSET_CALIBRATEDONE_Pos) /*!< Bit mask of CALIBRATEDONE field. */
#define SAADC_INTENSET_CALIBRATEDONE_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENSET_CALIBRATEDONE_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENSET_CALIBRATEDONE_Set (1UL) /*!< Enable */

/* Bit 3 : Write '1' to enable interrupt for event RESULTDONE */
#define SAADC_INTENSET_RESULTDONE_Pos (3UL) /*!< Position of RESULTDONE field. */
#define SAADC_INTENSET_RESULTDONE_Msk (0x1UL << SAADC_INTENSET_RESULTDONE_Pos) /*!< Bit mask of RESULTDONE field. */
#define SAADC_INTENSET_RESULTDONE_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENSET_RESULTDONE_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENSET_RESULTDONE_Set (1UL) /*!< Enable */

/* Bit 2 : Write '1' to enable interrupt for event DONE */
#define SAADC_INTENSET_DONE_Pos (2UL) /*!< Position of DONE field. */
#define SAADC_INTENSET_DONE_Msk (0x1UL << SAADC_INTENSET_DONE_Pos) /*!< Bit mask of DONE field. */
#define SAADC_INTENSET_DONE_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENSET_DONE_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENSET_DONE_Set (1UL) /*!< Enable */

/* Bit 1 : Write '1' to enable interrupt for event END */
#define SAADC_INTENSET_END_Pos (1UL) /*!< Position of END field. */
#define SAADC_INTENSET_END_Msk (0x1UL << SAADC_INTENSET_END_Pos) /*!< Bit mask of END field. */
#define SAADC_INTENSET_END_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENSET_END_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENSET_END_Set (1UL) /*!< Enable */

/* Bit 0 : Write '1' to enable interrupt for event STARTED */
#define SAADC_INTENSET_STARTED_Pos (0UL) /*!< Position of STARTED field. */
#define SAADC_INTENSET_STARTED_Msk (0x1UL << SAADC_INTENSET_STARTED_Pos) /*!< Bit mask of STARTED field. */
#define SAADC_INTENSET_STARTED_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENSET_STARTED_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENSET_STARTED_Set (1UL) /*!< Enable */

/* Register: SAADC_INTENCLR */
/* Description: Disable interrupt */

/* Bit 21 : Write '1' to disable interrupt for event CH7LIMITL */
#define SAADC_INTENCLR_CH7LIMITL_Pos (21UL) /*!< Position of CH7LIMITL field. */
#define SAADC_INTENCLR_CH7LIMITL_Msk (0x1UL << SAADC_INTENCLR_CH7LIMITL_Pos) /*!< Bit mask of CH7LIMITL field. */
#define SAADC_INTENCLR_CH7LIMITL_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENCLR_CH7LIMITL_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENCLR_CH7LIMITL_Clear (1UL) /*!< Disable */

/* Bit 20 : Write '1' to disable interrupt for event CH7LIMITH */
#define SAADC_INTENCLR_CH7LIMITH_Pos (20UL) /*!< Position of CH7LIMITH field. */
#define SAADC_INTENCLR_CH7LIMITH_Msk (0x1UL << SAADC_INTENCLR_CH7LIMITH_Pos) /*!< Bit mask of CH7LIMITH field. */
#define SAADC_INTENCLR_CH7LIMITH_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENCLR_CH7LIMITH_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENCLR_CH7LIMITH_Clear (1UL) /*!< Disable */

/* Bit 19 : Write '1' to disable interrupt for event CH6LIMITL */
#define SAADC_INTENCLR_CH6LIMITL_Pos (19UL) /*!< Position of CH6LIMITL field. */
#define SAADC_INTENCLR_CH6LIMITL_Msk (0x1UL << SAADC_INTENCLR_CH6LIMITL_Pos) /*!< Bit mask of CH6LIMITL field. */
#define SAADC_INTENCLR_CH6LIMITL_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENCLR_CH6LIMITL_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENCLR_CH6LIMITL_Clear (1UL) /*!< Disable */

/* Bit 18 : Write '1' to disable interrupt for event CH6LIMITH */
#define SAADC_INTENCLR_CH6LIMITH_Pos (18UL) /*!< Position of CH6LIMITH field. */
#define SAADC_INTENCLR_CH6LIMITH_Msk (0x1UL << SAADC_INTENCLR_CH6LIMITH_Pos) /*!< Bit mask of CH6LIMITH field. */
#define SAADC_INTENCLR_CH6LIMITH_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENCLR_CH6LIMITH_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENCLR_CH6LIMITH_Clear (1UL) /*!< Disable */

/* Bit 17 : Write '1' to disable interrupt for event CH5LIMITL */
#define SAADC_INTENCLR_CH5LIMITL_Pos (17UL) /*!< Position of CH5LIMITL field. */
#define SAADC_INTENCLR_CH5LIMITL_Msk (0x1UL << SAADC_INTENCLR_CH5LIMITL_Pos) /*!< Bit mask of CH5LIMITL field. */
#define SAADC_INTENCLR_CH5LIMITL_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENCLR_CH5LIMITL_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENCLR_CH5LIMITL_Clear (1UL) /*!< Disable */

/* Bit 16 : Write '1' to disable interrupt for event CH5LIMITH */
#define SAADC_INTENCLR_CH5LIMITH_Pos (16UL) /*!< Position of CH5LIMITH field. */
#define SAADC_INTENCLR_CH5LIMITH_Msk (0x1UL << SAADC_INTENCLR_CH5LIMITH_Pos) /*!< Bit mask of CH5LIMITH field. */
#define SAADC_INTENCLR_CH5LIMITH_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENCLR_CH5LIMITH_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENCLR_CH5LIMITH_Clear (1UL) /*!< Disable */

/* Bit 15 : Write '1' to disable interrupt for event CH4LIMITL */
#define SAADC_INTENCLR_CH4LIMITL_Pos (15UL) /*!< Position of CH4LIMITL field. */
#define SAADC_INTENCLR_CH4LIMITL_Msk (0x1UL << SAADC_INTENCLR_CH4LIMITL_Pos) /*!< Bit mask of CH4LIMITL field. */
#define SAADC_INTENCLR_CH4LIMITL_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENCLR_CH4LIMITL_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENCLR_CH4LIMITL_Clear (1UL) /*!< Disable */

/* Bit 14 : Write '1' to disable interrupt for event CH4LIMITH */
#define SAADC_INTENCLR_CH4LIMITH_Pos (14UL) /*!< Position of CH4LIMITH field. */
#define SAADC_INTENCLR_CH4LIMITH_Msk (0x1UL << SAADC_INTENCLR_CH4LIMITH_Pos) /*!< Bit mask of CH4LIMITH field. */
#define SAADC_INTENCLR_CH4LIMITH_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENCLR_CH4LIMITH_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENCLR_CH4LIMITH_Clear (1UL) /*!< Disable */

/* Bit 13 : Write '1' to disable interrupt for event CH3LIMITL */
#define SAADC_INTENCLR_CH3LIMITL_Pos (13UL) /*!< Position of CH3LIMITL field. */
#define SAADC_INTENCLR_CH3LIMITL_Msk (0x1UL << SAADC_INTENCLR_CH3LIMITL_Pos) /*!< Bit mask of CH3LIMITL field. */
#define SAADC_INTENCLR_CH3LIMITL_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENCLR_CH3LIMITL_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENCLR_CH3LIMITL_Clear (1UL) /*!< Disable */

/* Bit 12 : Write '1' to disable interrupt for event CH3LIMITH */
#define SAADC_INTENCLR_CH3LIMITH_Pos (12UL) /*!< Position of CH3LIMITH field. */
#define SAADC_INTENCLR_CH3LIMITH_Msk (0x1UL << SAADC_INTENCLR_CH3LIMITH_Pos) /*!< Bit mask of CH3LIMITH field. */
#define SAADC_INTENCLR_CH3LIMITH_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENCLR_CH3LIMITH_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENCLR_CH3LIMITH_Clear (1UL) /*!< Disable */

/* Bit 11 : Write '1' to disable interrupt for event CH2LIMITL */
#define SAADC_INTENCLR_CH2LIMITL_Pos (11UL) /*!< Position of CH2LIMITL field. */
#define SAADC_INTENCLR_CH2LIMITL_Msk (0x1UL << SAADC_INTENCLR_CH2LIMITL_Pos) /*!< Bit mask of CH2LIMITL field. */
#define SAADC_INTENCLR_CH2LIMITL_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENCLR_CH2LIMITL_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENCLR_CH2LIMITL_Clear (1UL) /*!< Disable */

/* Bit 10 : Write '1' to disable interrupt for event CH2LIMITH */
#define SAADC_INTENCLR_CH2LIMITH_Pos (10UL) /*!< Position of CH2LIMITH field. */
#define SAADC_INTENCLR_CH2LIMITH_Msk (0x1UL << SAADC_INTENCLR_CH2LIMITH_Pos) /*!< Bit mask of CH2LIMITH field. */
#define SAADC_INTENCLR_CH2LIMITH_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENCLR_CH2LIMITH_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENCLR_CH2LIMITH_Clear (1UL) /*!< Disable */

/* Bit 9 : Write '1' to disable interrupt for event CH1LIMITL */
#define SAADC_INTENCLR_CH1LIMITL_Pos (9UL) /*!< Position of CH1LIMITL field. */
#define SAADC_INTENCLR_CH1LIMITL_Msk (0x1UL << SAADC_INTENCLR_CH1LIMITL_Pos) /*!< Bit mask of CH1LIMITL field. */
#define SAADC_INTENCLR_CH1LIMITL_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENCLR_CH1LIMITL_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENCLR_CH1LIMITL_Clear (1UL) /*!< Disable */

/* Bit 8 : Write '1' to disable interrupt for event CH1LIMITH */
#define SAADC_INTENCLR_CH1LIMITH_Pos (8UL) /*!< Position of CH1LIMITH field. */
#define SAADC_INTENCLR_CH1LIMITH_Msk (0x1UL << SAADC_INTENCLR_CH1LIMITH_Pos) /*!< Bit mask of CH1LIMITH field. */
#define SAADC_INTENCLR_CH1LIMITH_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENCLR_CH1LIMITH_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENCLR_CH1LIMITH_Clear (1UL) /*!< Disable */

/* Bit 7 : Write '1' to disable interrupt for event CH0LIMITL */
#define SAADC_INTENCLR_CH0LIMITL_Pos (7UL) /*!< Position of CH0LIMITL field. */
#define SAADC_INTENCLR_CH0LIMITL_Msk (0x1UL << SAADC_INTENCLR_CH0LIMITL_Pos) /*!< Bit mask of CH0LIMITL field. */
#define SAADC_INTENCLR_CH0LIMITL_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENCLR_CH0LIMITL_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENCLR_CH0LIMITL_Clear (1UL) /*!< Disable */

/* Bit 6 : Write '1' to disable interrupt for event CH0LIMITH */
#define SAADC_INTENCLR_CH0LIMITH_Pos (6UL) /*!< Position of CH0LIMITH field. */
#define SAADC_INTENCLR_CH0LIMITH_Msk (0x1UL << SAADC_INTENCLR_CH0LIMITH_Pos) /*!< Bit mask of CH0LIMITH field. */
#define SAADC_INTENCLR_CH0LIMITH_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENCLR_CH0LIMITH_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENCLR_CH0LIMITH_Clear (1UL) /*!< Disable */

/* Bit 5 : Write '1' to disable interrupt for event STOPPED */
#define SAADC_INTENCLR_STOPPED_Pos (5UL) /*!< Position of STOPPED field. */
#define SAADC_INTENCLR_STOPPED_Msk (0x1UL << SAADC_INTENCLR_STOPPED_Pos) /*!< Bit mask of STOPPED field. */
#define SAADC_INTENCLR_STOPPED_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENCLR_STOPPED_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENCLR_STOPPED_Clear (1UL) /*!< Disable */

/* Bit 4 : Write '1' to disable interrupt for event CALIBRATEDONE */
#define SAADC_INTENCLR_CALIBRATEDONE_Pos (4UL) /*!< Position of CALIBRATEDONE field. */
#define SAADC_INTENCLR_CALIBRATEDONE_Msk (0x1UL << SAADC_INTENCLR_CALIBRATEDONE_Pos) /*!< Bit mask of CALIBRATEDONE field. */
#define SAADC_INTENCLR_CALIBRATEDONE_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENCLR_CALIBRATEDONE_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENCLR_CALIBRATEDONE_Clear (1UL) /*!< Disable */

/* Bit 3 : Write '1' to disable interrupt for event RESULTDONE */
#define SAADC_INTENCLR_RESULTDONE_Pos (3UL) /*!< Position of RESULTDONE field. */
#define SAADC_INTENCLR_RESULTDONE_Msk (0x1UL << SAADC_INTENCLR_RESULTDONE_Pos) /*!< Bit mask of RESULTDONE field. */
#define SAADC_INTENCLR_RESULTDONE_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENCLR_RESULTDONE_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENCLR_RESULTDONE_Clear (1UL) /*!< Disable */

/* Bit 2 : Write '1' to disable interrupt for event DONE */
#define SAADC_INTENCLR_DONE_Pos (2UL) /*!< Position of DONE field. */
#define SAADC_INTENCLR_DONE_Msk (0x1UL << SAADC_INTENCLR_DONE_Pos) /*!< Bit mask of DONE field. */
#define SAADC_INTENCLR_DONE_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENCLR_DONE_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENCLR_DONE_Clear (1UL) /*!< Disable */

/* Bit 1 : Write '1' to disable interrupt for event END */
#define SAADC_INTENCLR_END_Pos (1UL) /*!< Position of END field. */
#define SAADC_INTENCLR_END_Msk (0x1UL << SAADC_INTENCLR_END_Pos) /*!< Bit mask of END field. */
#define SAADC_INTENCLR_END_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENCLR_END_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENCLR_END_Clear (1UL) /*!< Disable */

/* Bit 0 : Write '1' to disable interrupt for event STARTED */
#define SAADC_INTENCLR_STARTED_Pos (0UL) /*!< Position of STARTED field. */
#define SAADC_INTENCLR_STARTED_Msk (0x1UL << SAADC_INTENCLR_STARTED_Pos) /*!< Bit mask of STARTED field. */
#define SAADC_INTENCLR_STARTED_Disabled (0UL) /*!< Read: Disabled */
#define SAADC_INTENCLR_STARTED_Enabled (1UL) /*!< Read: Enabled */
#define SAADC_INTENCLR_STARTED_Clear (1UL) /*!< Disable */

/* Register: SAADC_STATUS */
/* Description: Status */

/* Bit 0 : Status */
#define SAADC_STATUS_STATUS_Pos (0UL) /*!< Position of STATUS field. */
#define SAADC_STATUS_STATUS_Msk (0x1UL << SAADC_STATUS_STATUS_Pos) /*!< Bit mask of STATUS field. */
#define SAADC_STATUS_STATUS_Ready (0UL) /*!< ADC is ready. No on-going conversion. */
#define SAADC_STATUS_STATUS_Busy (1UL) /*!< ADC is busy. Conversion in progress. */

/* Register: SAADC_ENABLE */
/* Description: Enable or disable ADC */

/* Bit 0 : Enable or disable ADC */
#define SAADC_ENABLE_ENABLE_Pos (0UL) /*!< Position of ENABLE field. */
#define SAADC_ENABLE_ENABLE_Msk (0x1UL << SAADC_ENABLE_ENABLE_Pos) /*!< Bit mask of ENABLE field. */
#define SAADC_ENABLE_ENABLE_Disabled (0UL) /*!< Disable ADC */
#define SAADC_ENABLE_ENABLE_Enabled (1UL) /*!< Enable ADC */

/* Register: SAADC_CH_PSELP */
/* Description: Description cluster: Input positive pin selection for CH[n] */

/* Bits 4..0 : Analog positive input channel */
#define SAADC_CH_PSELP_PSELP_Pos (0UL) /*!< Position of PSELP field. */
#define SAADC_CH_PSELP_PSELP_Msk (0x1FUL << SAADC_CH_PSELP_PSELP_Pos) /*!< Bit mask of PSELP field. */
#define SAADC_CH_PSELP_PSELP_NC (0UL) /*!< Not connected */
#define SAADC_CH_PSELP_PSELP_AnalogInput0 (1UL) /*!< AIN0 */
#define SAADC_CH_PSELP_PSELP_AnalogInput1 (2UL) /*!< AIN1 */
#define SAADC_CH_PSELP_PSELP_AnalogInput2 (3UL) /*!< AIN2 */
#define SAADC_CH_PSELP_PSELP_AnalogInput3 (4UL) /*!< AIN3 */
#define SAADC_CH_PSELP_PSELP_AnalogInput4 (5UL) /*!< AIN4 */
#define SAADC_CH_PSELP_PSELP_AnalogInput5 (6UL) /*!< AIN5 */
#define SAADC_CH_PSELP_PSELP_AnalogInput6 (7UL) /*!< AIN6 */
#define SAADC_CH_PSELP_PSELP_AnalogInput7 (8UL) /*!< AIN7 */
#define SAADC_CH_PSELP_PSELP_VDD (9UL) /*!< VDD */

/* Register: SAADC_CH_PSELN */
/* Description: Description cluster: Input negative pin selection for CH[n] */

/* Bits 4..0 : Analog negative input, enables differential channel */
#define SAADC_CH_PSELN_PSELN_Pos (0UL) /*!< Position of PSELN field. */
#define SAADC_CH_PSELN_PSELN_Msk (0x1FUL << SAADC_CH_PSELN_PSELN_Pos) /*!< Bit mask of PSELN field. */
#define SAADC_CH_PSELN_PSELN_NC (0UL) /*!< Not connected */
#define SAADC_CH_PSELN_PSELN_AnalogInput0 (1UL) /*!< AIN0 */
#define SAADC_CH_PSELN_PSELN_AnalogInput1 (2UL) /*!< AIN1 */
#define SAADC_CH_PSELN_PSELN_AnalogInput2 (3UL) /*!< AIN2 */
#define SAADC_CH_PSELN_PSELN_AnalogInput3 (4UL) /*!< AIN3 */
#define SAADC_CH_PSELN_PSELN_AnalogInput4 (5UL) /*!< AIN4 */
#define SAADC_CH_PSELN_PSELN_AnalogInput5 (6UL) /*!< AIN5 */
#define SAADC_CH_PSELN_PSELN_AnalogInput6 (7UL) /*!< AIN6 */
#define SAADC_CH_PSELN_PSELN_AnalogInput7 (8UL) /*!< AIN7 */
#define SAADC_CH_PSELN_PSELN_VDD (9UL) /*!< VDD */

/* Register: SAADC_CH_CONFIG */
/* Description: Description cluster: Input configuration for CH[n] */

/* Bit 24 : Enable burst mode */
#define SAADC_CH_CONFIG_BURST_Pos (24UL) /*!< Position of BURST field. */
#define SAADC_CH_CONFIG_BURST_Msk (0x1UL << SAADC_CH_CONFIG_BURST_Pos) /*!< Bit mask of BURST field. */
#define SAADC_CH_CONFIG_BURST_Disabled (0UL) /*!< Burst mode is disabled (normal operation) */
#define SAADC_CH_CONFIG_BURST_Enabled (1UL) /*!< Burst mode is enabled. SAADC takes 2^OVERSAMPLE number of samples as fast as it can, and sends the average to Data RAM. */

/* Bit 20 : Enable differential mode */
#define SAADC_CH_CONFIG_MODE_Pos (20UL) /*!< Position of MODE field. */
#define SAADC_CH_CONFIG_MODE_Msk (0x1UL << SAADC_CH_CONFIG_MODE_Pos) /*!< Bit mask of MODE field. */
#define SAADC_CH_CONFIG_MODE_SE (0UL) /*!< Single ended, PSELN will be ignored, negative input to ADC shorted to GND */
#define SAADC_CH_CONFIG_MODE_Diff (1UL) /*!< Differential */

/* Bits 18..16 : Acquisition time, the time the ADC uses to sample the input voltage */
#define SAADC_CH_CONFIG_TACQ_Pos (16UL) /*!< Position of TACQ field. */
#define SAADC_CH_CONFIG_TACQ_Msk (0x7UL << SAADC_CH_CONFIG_TACQ_Pos) /*!< Bit mask of TACQ field. */
#define SAADC_CH_CONFIG_TACQ_3us (0UL) /*!< 3 us */
#define SAADC_CH_CONFIG_TACQ_5us (1UL) /*!< 5 us */
#define SAADC_CH_CONFIG_TACQ_10us (2UL) /*!< 10 us */
#define SAADC_CH_CONFIG_TACQ_15us (3UL) /*!< 15 us */
#define SAADC_CH_CONFIG_TACQ_20us (4UL) /*!< 20 us */
#define SAADC_CH_CONFIG_TACQ_40us (5UL) /*!< 40 us */

/* Bit 12 : Reference control */
#define SAADC_CH_CONFIG_REFSEL_Pos (12UL) /*!< Position of REFSEL field. */
#define SAADC_CH_CONFIG_REFSEL_Msk (0x1UL << SAADC_CH_CONFIG_REFSEL_Pos) /*!< Bit mask of REFSEL field. */
#define SAADC_CH_CONFIG_REFSEL_Internal (0UL) /*!< Internal reference (0.6 V) */
#define SAADC_CH_CONFIG_REFSEL_VDD1_4 (1UL) /*!< VDD/4 as reference */

/* Bits 10..8 : Gain control */
#define SAADC_CH_CONFIG_GAIN_Pos (8UL) /*!< Position of GAIN field. */
#define SAADC_CH_CONFIG_GAIN_Msk (0x7UL << SAADC_CH_CONFIG_GAIN_Pos) /*!< Bit mask of GAIN field. */
#define SAADC_CH_CONFIG_GAIN_Gain1_6 (0UL) /*!< 1/6 */
#define SAADC_CH_CONFIG_GAIN_Gain1_5 (1UL) /*!< 1/5 */
#define SAADC_CH_CONFIG_GAIN_Gain1_4 (2UL) /*!< 1/4 */
#define SAADC_CH_CONFIG_GAIN_Gain1_3 (3UL) /*!< 1/3 */
#define SAADC_CH_CONFIG_GAIN_Gain1_2 (4UL) /*!< 1/2 */
#define SAADC_CH_CONFIG_GAIN_Gain1 (5UL) /*!< 1 */
#define SAADC_CH_CONFIG_GAIN_Gain2 (6UL) /*!< 2 */
#define SAADC_CH_CONFIG_GAIN_Gain4 (7UL) /*!< 4 */

/* Bits 5..4 : Negative channel resistor control */
#define SAADC_CH_CONFIG_RESN_Pos (4UL) /*!< Position of RESN field. */
#define SAADC_CH_CONFIG_RESN_Msk (0x3UL << SAADC_CH_CONFIG_RESN_Pos) /*!< Bit mask of RESN field. */
#define SAADC_CH_CONFIG_RESN_Bypass (0UL) /*!< Bypass resistor ladder */
#define SAADC_CH_CONFIG_RESN_Pulldown (1UL) /*!< Pull-down to GND */
#define SAADC_CH_CONFIG_RESN_Pullup (2UL) /*!< Pull-up to VDD */
#define SAADC_CH_CONFIG_RESN_VDD1_2 (3UL) /*!< Set input at VDD/2 */

/* Bits 1..0 : Positive channel resistor control */
#define SAADC_CH_CONFIG_RESP_Pos (0UL) /*!< Position of RESP field. */
#define SAADC_CH_CONFIG_RESP_Msk (0x3UL << SAADC_CH_CONFIG_RESP_Pos) /*!< Bit mask of RESP field. */
#define SAADC_CH_CONFIG_RESP_Bypass (0UL) /*!< Bypass resistor ladder */
#define SAADC_CH_CONFIG_RESP_Pulldown (1UL) /*!< Pull-down to GND */
#define SAADC_CH_CONFIG_RESP_Pullup (2UL) /*!< Pull-up to VDD */
#define SAADC_CH_CONFIG_RESP_VDD1_2 (3UL) /*!< Set input at VDD/2 */

/* Register: SAADC_CH_LIMIT */
/* Description: Description cluster: High/low limits for event monitoring a channel */

/* Bits 31..16 : High level limit */
#define SAADC_CH_LIMIT_HIGH_Pos (16UL) /*!< Position of HIGH field. */
#define SAADC_CH_LIMIT_HIGH_Msk (0xFFFFUL << SAADC_CH_LIMIT_HIGH_Pos) /*!< Bit mask of HIGH field. */

/* Bits 15..0 : Low level limit */
#define SAADC_CH_LIMIT_LOW_Pos (0UL) /*!< Position of LOW field. */
#define SAADC_CH_LIMIT_LOW_Msk (0xFFFFUL << SAADC_CH_LIMIT_LOW_Pos) /*!< Bit mask of LOW field. */

/* Register: SAADC_RESOLUTION */
/* Description: Resolution configuration */

/* Bits 2..0 : Set the resolution */
#define SAADC_RESOLUTION_VAL_Pos (0UL) /*!< Position of VAL field. */
#define SAADC_RESOLUTION_VAL_Msk (0x7UL << SAADC_RESOLUTION_VAL_Pos) /*!< Bit mask of VAL field. */
#define SAADC_RESOLUTION_VAL_8bit (0UL) /*!< 8 bit */
#define SAADC_RESOLUTION_VAL_10bit (1UL) /*!< 10 bit */
#define SAADC_RESOLUTION_VAL_12bit (2UL) /*!< 12 bit */
#define SAADC_RESOLUTION_VAL_14bit (3UL) /*!< 14 bit */

/* Register: SAADC_OVERSAMPLE */
/* Description: Oversampling configuration. OVERSAMPLE should not be combined with SCAN. The RESOLUTION is applied before averaging, thus for high OVERSAMPLE a higher RESOLUTION should be used. */

/* Bits 3..0 : Oversample control */
#define SAADC_OVERSAMPLE_OVERSAMPLE_Pos (0UL) /*!< Position of OVERSAMPLE field. */
#define SAADC_OVERSAMPLE_OVERSAMPLE_Msk (0xFUL << SAADC_OVERSAMPLE_OVERSAMPLE_Pos) /*!< Bit mask of OVERSAMPLE field. */
#define SAADC_OVERSAMPLE_OVERSAMPLE_Bypass (0UL) /*!< Bypass oversampling */
#define SAADC_OVERSAMPLE_OVERSAMPLE_Over2x (1UL) /*!< Oversample 2x */
#define SAADC_OVERSAMPLE_OVERSAMPLE_Over4x (2UL) /*!< Oversample 4x */
#define SAADC_OVERSAMPLE_OVERSAMPLE_Over8x (3UL) /*!< Oversample 8x */
#define SAADC_OVERSAMPLE_OVERSAMPLE_Over16x (4UL) /*!< Oversample 16x */
#define SAADC_OVERSAMPLE_OVERSAMPLE_Over32x (5UL) /*!< Oversample 32x */
#define SAADC_OVERSAMPLE_OVERSAMPLE_Over64x (6UL) /*!< Oversample 64x */
#define SAADC_OVERSAMPLE_OVERSAMPLE_Over128x (7UL) /*!< Oversample 128x */
#define SAADC_OVERSAMPLE_OVERSAMPLE_Over256x (8UL) /*!< Oversample 256x */

/* Register: SAADC_SAMPLERATE */
/* Description: Controls normal or continuous sample rate */

/* Bit 12 : Select mode for sample rate control */
#define SAADC_SAMPLERATE_MODE_Pos (12UL) /*!< Position of MODE field. */
#define SAADC_SAMPLERATE_MODE_Msk (0x1UL << SAADC_SAMPLERATE_MODE_Pos) /*!< Bit mask of MODE field. */
#define SAADC_SAMPLERATE_MODE_Task (0UL) /*!< Rate is controlled from SAMPLE task */
#define SAADC_SAMPLERATE_MODE_Timers (1UL) /*!< Rate is controlled from local timer (use CC to control the rate) */

/* Bits 10..0 : Capture and compare value. Sample rate is 16 MHz/CC */
#define SAADC_SAMPLERATE_CC_Pos (0UL) /*!< Position of CC field. */
#define SAADC_SAMPLERATE_CC_Msk (0x7FFUL << SAADC_SAMPLERATE_CC_Pos) /*!< Bit mask of CC field. */

/* Register: SAADC_RESULT_PTR */
/* Description: Data pointer */

/* Bits 31..0 : Data pointer */
#define SAADC_RESULT_PTR_PTR_Pos (0UL) /*!< Position of PTR field. */
#define SAADC_RESULT_PTR_PTR_Msk (0xFFFFFFFFUL << SAADC_RESULT_PTR_PTR_Pos) /*!< Bit mask of PTR field. */

/* Register: SAADC_RESULT_MAXCNT */
/* Description: Maximum number of buffer words to transfer */

/* Bits 14..0 : Maximum number of buffer words to transfer */
#define SAADC_RESULT_MAXCNT_MAXCNT_Pos (0UL) /*!< Position of MAXCNT field. */
#define SAADC_RESULT_MAXCNT_MAXCNT_Msk (0x7FFFUL << SAADC_RESULT_MAXCNT_MAXCNT_Pos) /*!< Bit mask of MAXCNT field. */

/* Register: SAADC_RESULT_AMOUNT */
/* Description: Number of buffer words transferred since last START */

/* Bits 14..0 : Number of buffer words transferred since last START. This register can be read after an END or STOPPED event. */
#define SAADC_RESULT_AMOUNT_AMOUNT_Pos (0UL) /*!< Position of AMOUNT field. */
#define SAADC_RESULT_AMOUNT_AMOUNT_Msk (0x7FFFUL << SAADC_RESULT_AMOUNT_AMOUNT_Pos) /*!< Bit mask of AMOUNT field. */


/* Peripheral: SPIM */
/* Description: Serial Peripheral Interface Master with EasyDMA 0 */

/* Register: SPIM_TASKS_START */
/* Description: Start SPI transaction */

/* Bit 0 : Start SPI transaction */
#define SPIM_TASKS_START_TASKS_START_Pos (0UL) /*!< Position of TASKS_START field. */
#define SPIM_TASKS_START_TASKS_START_Msk (0x1UL << SPIM_TASKS_START_TASKS_START_Pos) /*!< Bit mask of TASKS_START field. */
#define SPIM_TASKS_START_TASKS_START_Trigger (1UL) /*!< Trigger task */

/* Register: SPIM_TASKS_STOP */
/* Description: Stop SPI transaction */

/* Bit 0 : Stop SPI transaction */
#define SPIM_TASKS_STOP_TASKS_STOP_Pos (0UL) /*!< Position of TASKS_STOP field. */
#define SPIM_TASKS_STOP_TASKS_STOP_Msk (0x1UL << SPIM_TASKS_STOP_TASKS_STOP_Pos) /*!< Bit mask of TASKS_STOP field. */
#define SPIM_TASKS_STOP_TASKS_STOP_Trigger (1UL) /*!< Trigger task */

/* Register: SPIM_TASKS_SUSPEND */
/* Description: Suspend SPI transaction */

/* Bit 0 : Suspend SPI transaction */
#define SPIM_TASKS_SUSPEND_TASKS_SUSPEND_Pos (0UL) /*!< Position of TASKS_SUSPEND field. */
#define SPIM_TASKS_SUSPEND_TASKS_SUSPEND_Msk (0x1UL << SPIM_TASKS_SUSPEND_TASKS_SUSPEND_Pos) /*!< Bit mask of TASKS_SUSPEND field. */
#define SPIM_TASKS_SUSPEND_TASKS_SUSPEND_Trigger (1UL) /*!< Trigger task */

/* Register: SPIM_TASKS_RESUME */
/* Description: Resume SPI transaction */

/* Bit 0 : Resume SPI transaction */
#define SPIM_TASKS_RESUME_TASKS_RESUME_Pos (0UL) /*!< Position of TASKS_RESUME field. */
#define SPIM_TASKS_RESUME_TASKS_RESUME_Msk (0x1UL << SPIM_TASKS_RESUME_TASKS_RESUME_Pos) /*!< Bit mask of TASKS_RESUME field. */
#define SPIM_TASKS_RESUME_TASKS_RESUME_Trigger (1UL) /*!< Trigger task */

/* Register: SPIM_SUBSCRIBE_START */
/* Description: Subscribe configuration for task START */

/* Bit 31 :   */
#define SPIM_SUBSCRIBE_START_EN_Pos (31UL) /*!< Position of EN field. */
#define SPIM_SUBSCRIBE_START_EN_Msk (0x1UL << SPIM_SUBSCRIBE_START_EN_Pos) /*!< Bit mask of EN field. */
#define SPIM_SUBSCRIBE_START_EN_Disabled (0UL) /*!< Disable subscription */
#define SPIM_SUBSCRIBE_START_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task START will subscribe to */
#define SPIM_SUBSCRIBE_START_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SPIM_SUBSCRIBE_START_CHIDX_Msk (0xFUL << SPIM_SUBSCRIBE_START_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SPIM_SUBSCRIBE_STOP */
/* Description: Subscribe configuration for task STOP */

/* Bit 31 :   */
#define SPIM_SUBSCRIBE_STOP_EN_Pos (31UL) /*!< Position of EN field. */
#define SPIM_SUBSCRIBE_STOP_EN_Msk (0x1UL << SPIM_SUBSCRIBE_STOP_EN_Pos) /*!< Bit mask of EN field. */
#define SPIM_SUBSCRIBE_STOP_EN_Disabled (0UL) /*!< Disable subscription */
#define SPIM_SUBSCRIBE_STOP_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task STOP will subscribe to */
#define SPIM_SUBSCRIBE_STOP_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SPIM_SUBSCRIBE_STOP_CHIDX_Msk (0xFUL << SPIM_SUBSCRIBE_STOP_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SPIM_SUBSCRIBE_SUSPEND */
/* Description: Subscribe configuration for task SUSPEND */

/* Bit 31 :   */
#define SPIM_SUBSCRIBE_SUSPEND_EN_Pos (31UL) /*!< Position of EN field. */
#define SPIM_SUBSCRIBE_SUSPEND_EN_Msk (0x1UL << SPIM_SUBSCRIBE_SUSPEND_EN_Pos) /*!< Bit mask of EN field. */
#define SPIM_SUBSCRIBE_SUSPEND_EN_Disabled (0UL) /*!< Disable subscription */
#define SPIM_SUBSCRIBE_SUSPEND_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task SUSPEND will subscribe to */
#define SPIM_SUBSCRIBE_SUSPEND_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SPIM_SUBSCRIBE_SUSPEND_CHIDX_Msk (0xFUL << SPIM_SUBSCRIBE_SUSPEND_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SPIM_SUBSCRIBE_RESUME */
/* Description: Subscribe configuration for task RESUME */

/* Bit 31 :   */
#define SPIM_SUBSCRIBE_RESUME_EN_Pos (31UL) /*!< Position of EN field. */
#define SPIM_SUBSCRIBE_RESUME_EN_Msk (0x1UL << SPIM_SUBSCRIBE_RESUME_EN_Pos) /*!< Bit mask of EN field. */
#define SPIM_SUBSCRIBE_RESUME_EN_Disabled (0UL) /*!< Disable subscription */
#define SPIM_SUBSCRIBE_RESUME_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task RESUME will subscribe to */
#define SPIM_SUBSCRIBE_RESUME_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SPIM_SUBSCRIBE_RESUME_CHIDX_Msk (0xFUL << SPIM_SUBSCRIBE_RESUME_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SPIM_EVENTS_STOPPED */
/* Description: SPI transaction has stopped */

/* Bit 0 : SPI transaction has stopped */
#define SPIM_EVENTS_STOPPED_EVENTS_STOPPED_Pos (0UL) /*!< Position of EVENTS_STOPPED field. */
#define SPIM_EVENTS_STOPPED_EVENTS_STOPPED_Msk (0x1UL << SPIM_EVENTS_STOPPED_EVENTS_STOPPED_Pos) /*!< Bit mask of EVENTS_STOPPED field. */
#define SPIM_EVENTS_STOPPED_EVENTS_STOPPED_NotGenerated (0UL) /*!< Event not generated */
#define SPIM_EVENTS_STOPPED_EVENTS_STOPPED_Generated (1UL) /*!< Event generated */

/* Register: SPIM_EVENTS_ENDRX */
/* Description: End of RXD buffer reached */

/* Bit 0 : End of RXD buffer reached */
#define SPIM_EVENTS_ENDRX_EVENTS_ENDRX_Pos (0UL) /*!< Position of EVENTS_ENDRX field. */
#define SPIM_EVENTS_ENDRX_EVENTS_ENDRX_Msk (0x1UL << SPIM_EVENTS_ENDRX_EVENTS_ENDRX_Pos) /*!< Bit mask of EVENTS_ENDRX field. */
#define SPIM_EVENTS_ENDRX_EVENTS_ENDRX_NotGenerated (0UL) /*!< Event not generated */
#define SPIM_EVENTS_ENDRX_EVENTS_ENDRX_Generated (1UL) /*!< Event generated */

/* Register: SPIM_EVENTS_END */
/* Description: End of RXD buffer and TXD buffer reached */

/* Bit 0 : End of RXD buffer and TXD buffer reached */
#define SPIM_EVENTS_END_EVENTS_END_Pos (0UL) /*!< Position of EVENTS_END field. */
#define SPIM_EVENTS_END_EVENTS_END_Msk (0x1UL << SPIM_EVENTS_END_EVENTS_END_Pos) /*!< Bit mask of EVENTS_END field. */
#define SPIM_EVENTS_END_EVENTS_END_NotGenerated (0UL) /*!< Event not generated */
#define SPIM_EVENTS_END_EVENTS_END_Generated (1UL) /*!< Event generated */

/* Register: SPIM_EVENTS_ENDTX */
/* Description: End of TXD buffer reached */

/* Bit 0 : End of TXD buffer reached */
#define SPIM_EVENTS_ENDTX_EVENTS_ENDTX_Pos (0UL) /*!< Position of EVENTS_ENDTX field. */
#define SPIM_EVENTS_ENDTX_EVENTS_ENDTX_Msk (0x1UL << SPIM_EVENTS_ENDTX_EVENTS_ENDTX_Pos) /*!< Bit mask of EVENTS_ENDTX field. */
#define SPIM_EVENTS_ENDTX_EVENTS_ENDTX_NotGenerated (0UL) /*!< Event not generated */
#define SPIM_EVENTS_ENDTX_EVENTS_ENDTX_Generated (1UL) /*!< Event generated */

/* Register: SPIM_EVENTS_STARTED */
/* Description: Transaction started */

/* Bit 0 : Transaction started */
#define SPIM_EVENTS_STARTED_EVENTS_STARTED_Pos (0UL) /*!< Position of EVENTS_STARTED field. */
#define SPIM_EVENTS_STARTED_EVENTS_STARTED_Msk (0x1UL << SPIM_EVENTS_STARTED_EVENTS_STARTED_Pos) /*!< Bit mask of EVENTS_STARTED field. */
#define SPIM_EVENTS_STARTED_EVENTS_STARTED_NotGenerated (0UL) /*!< Event not generated */
#define SPIM_EVENTS_STARTED_EVENTS_STARTED_Generated (1UL) /*!< Event generated */

/* Register: SPIM_PUBLISH_STOPPED */
/* Description: Publish configuration for event STOPPED */

/* Bit 31 :   */
#define SPIM_PUBLISH_STOPPED_EN_Pos (31UL) /*!< Position of EN field. */
#define SPIM_PUBLISH_STOPPED_EN_Msk (0x1UL << SPIM_PUBLISH_STOPPED_EN_Pos) /*!< Bit mask of EN field. */
#define SPIM_PUBLISH_STOPPED_EN_Disabled (0UL) /*!< Disable publishing */
#define SPIM_PUBLISH_STOPPED_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event STOPPED will publish to. */
#define SPIM_PUBLISH_STOPPED_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SPIM_PUBLISH_STOPPED_CHIDX_Msk (0xFUL << SPIM_PUBLISH_STOPPED_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SPIM_PUBLISH_ENDRX */
/* Description: Publish configuration for event ENDRX */

/* Bit 31 :   */
#define SPIM_PUBLISH_ENDRX_EN_Pos (31UL) /*!< Position of EN field. */
#define SPIM_PUBLISH_ENDRX_EN_Msk (0x1UL << SPIM_PUBLISH_ENDRX_EN_Pos) /*!< Bit mask of EN field. */
#define SPIM_PUBLISH_ENDRX_EN_Disabled (0UL) /*!< Disable publishing */
#define SPIM_PUBLISH_ENDRX_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event ENDRX will publish to. */
#define SPIM_PUBLISH_ENDRX_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SPIM_PUBLISH_ENDRX_CHIDX_Msk (0xFUL << SPIM_PUBLISH_ENDRX_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SPIM_PUBLISH_END */
/* Description: Publish configuration for event END */

/* Bit 31 :   */
#define SPIM_PUBLISH_END_EN_Pos (31UL) /*!< Position of EN field. */
#define SPIM_PUBLISH_END_EN_Msk (0x1UL << SPIM_PUBLISH_END_EN_Pos) /*!< Bit mask of EN field. */
#define SPIM_PUBLISH_END_EN_Disabled (0UL) /*!< Disable publishing */
#define SPIM_PUBLISH_END_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event END will publish to. */
#define SPIM_PUBLISH_END_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SPIM_PUBLISH_END_CHIDX_Msk (0xFUL << SPIM_PUBLISH_END_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SPIM_PUBLISH_ENDTX */
/* Description: Publish configuration for event ENDTX */

/* Bit 31 :   */
#define SPIM_PUBLISH_ENDTX_EN_Pos (31UL) /*!< Position of EN field. */
#define SPIM_PUBLISH_ENDTX_EN_Msk (0x1UL << SPIM_PUBLISH_ENDTX_EN_Pos) /*!< Bit mask of EN field. */
#define SPIM_PUBLISH_ENDTX_EN_Disabled (0UL) /*!< Disable publishing */
#define SPIM_PUBLISH_ENDTX_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event ENDTX will publish to. */
#define SPIM_PUBLISH_ENDTX_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SPIM_PUBLISH_ENDTX_CHIDX_Msk (0xFUL << SPIM_PUBLISH_ENDTX_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SPIM_PUBLISH_STARTED */
/* Description: Publish configuration for event STARTED */

/* Bit 31 :   */
#define SPIM_PUBLISH_STARTED_EN_Pos (31UL) /*!< Position of EN field. */
#define SPIM_PUBLISH_STARTED_EN_Msk (0x1UL << SPIM_PUBLISH_STARTED_EN_Pos) /*!< Bit mask of EN field. */
#define SPIM_PUBLISH_STARTED_EN_Disabled (0UL) /*!< Disable publishing */
#define SPIM_PUBLISH_STARTED_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event STARTED will publish to. */
#define SPIM_PUBLISH_STARTED_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SPIM_PUBLISH_STARTED_CHIDX_Msk (0xFUL << SPIM_PUBLISH_STARTED_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SPIM_SHORTS */
/* Description: Shortcuts between local events and tasks */

/* Bit 17 : Shortcut between event END and task START */
#define SPIM_SHORTS_END_START_Pos (17UL) /*!< Position of END_START field. */
#define SPIM_SHORTS_END_START_Msk (0x1UL << SPIM_SHORTS_END_START_Pos) /*!< Bit mask of END_START field. */
#define SPIM_SHORTS_END_START_Disabled (0UL) /*!< Disable shortcut */
#define SPIM_SHORTS_END_START_Enabled (1UL) /*!< Enable shortcut */

/* Register: SPIM_INTENSET */
/* Description: Enable interrupt */

/* Bit 19 : Write '1' to enable interrupt for event STARTED */
#define SPIM_INTENSET_STARTED_Pos (19UL) /*!< Position of STARTED field. */
#define SPIM_INTENSET_STARTED_Msk (0x1UL << SPIM_INTENSET_STARTED_Pos) /*!< Bit mask of STARTED field. */
#define SPIM_INTENSET_STARTED_Disabled (0UL) /*!< Read: Disabled */
#define SPIM_INTENSET_STARTED_Enabled (1UL) /*!< Read: Enabled */
#define SPIM_INTENSET_STARTED_Set (1UL) /*!< Enable */

/* Bit 8 : Write '1' to enable interrupt for event ENDTX */
#define SPIM_INTENSET_ENDTX_Pos (8UL) /*!< Position of ENDTX field. */
#define SPIM_INTENSET_ENDTX_Msk (0x1UL << SPIM_INTENSET_ENDTX_Pos) /*!< Bit mask of ENDTX field. */
#define SPIM_INTENSET_ENDTX_Disabled (0UL) /*!< Read: Disabled */
#define SPIM_INTENSET_ENDTX_Enabled (1UL) /*!< Read: Enabled */
#define SPIM_INTENSET_ENDTX_Set (1UL) /*!< Enable */

/* Bit 6 : Write '1' to enable interrupt for event END */
#define SPIM_INTENSET_END_Pos (6UL) /*!< Position of END field. */
#define SPIM_INTENSET_END_Msk (0x1UL << SPIM_INTENSET_END_Pos) /*!< Bit mask of END field. */
#define SPIM_INTENSET_END_Disabled (0UL) /*!< Read: Disabled */
#define SPIM_INTENSET_END_Enabled (1UL) /*!< Read: Enabled */
#define SPIM_INTENSET_END_Set (1UL) /*!< Enable */

/* Bit 4 : Write '1' to enable interrupt for event ENDRX */
#define SPIM_INTENSET_ENDRX_Pos (4UL) /*!< Position of ENDRX field. */
#define SPIM_INTENSET_ENDRX_Msk (0x1UL << SPIM_INTENSET_ENDRX_Pos) /*!< Bit mask of ENDRX field. */
#define SPIM_INTENSET_ENDRX_Disabled (0UL) /*!< Read: Disabled */
#define SPIM_INTENSET_ENDRX_Enabled (1UL) /*!< Read: Enabled */
#define SPIM_INTENSET_ENDRX_Set (1UL) /*!< Enable */

/* Bit 1 : Write '1' to enable interrupt for event STOPPED */
#define SPIM_INTENSET_STOPPED_Pos (1UL) /*!< Position of STOPPED field. */
#define SPIM_INTENSET_STOPPED_Msk (0x1UL << SPIM_INTENSET_STOPPED_Pos) /*!< Bit mask of STOPPED field. */
#define SPIM_INTENSET_STOPPED_Disabled (0UL) /*!< Read: Disabled */
#define SPIM_INTENSET_STOPPED_Enabled (1UL) /*!< Read: Enabled */
#define SPIM_INTENSET_STOPPED_Set (1UL) /*!< Enable */

/* Register: SPIM_INTENCLR */
/* Description: Disable interrupt */

/* Bit 19 : Write '1' to disable interrupt for event STARTED */
#define SPIM_INTENCLR_STARTED_Pos (19UL) /*!< Position of STARTED field. */
#define SPIM_INTENCLR_STARTED_Msk (0x1UL << SPIM_INTENCLR_STARTED_Pos) /*!< Bit mask of STARTED field. */
#define SPIM_INTENCLR_STARTED_Disabled (0UL) /*!< Read: Disabled */
#define SPIM_INTENCLR_STARTED_Enabled (1UL) /*!< Read: Enabled */
#define SPIM_INTENCLR_STARTED_Clear (1UL) /*!< Disable */

/* Bit 8 : Write '1' to disable interrupt for event ENDTX */
#define SPIM_INTENCLR_ENDTX_Pos (8UL) /*!< Position of ENDTX field. */
#define SPIM_INTENCLR_ENDTX_Msk (0x1UL << SPIM_INTENCLR_ENDTX_Pos) /*!< Bit mask of ENDTX field. */
#define SPIM_INTENCLR_ENDTX_Disabled (0UL) /*!< Read: Disabled */
#define SPIM_INTENCLR_ENDTX_Enabled (1UL) /*!< Read: Enabled */
#define SPIM_INTENCLR_ENDTX_Clear (1UL) /*!< Disable */

/* Bit 6 : Write '1' to disable interrupt for event END */
#define SPIM_INTENCLR_END_Pos (6UL) /*!< Position of END field. */
#define SPIM_INTENCLR_END_Msk (0x1UL << SPIM_INTENCLR_END_Pos) /*!< Bit mask of END field. */
#define SPIM_INTENCLR_END_Disabled (0UL) /*!< Read: Disabled */
#define SPIM_INTENCLR_END_Enabled (1UL) /*!< Read: Enabled */
#define SPIM_INTENCLR_END_Clear (1UL) /*!< Disable */

/* Bit 4 : Write '1' to disable interrupt for event ENDRX */
#define SPIM_INTENCLR_ENDRX_Pos (4UL) /*!< Position of ENDRX field. */
#define SPIM_INTENCLR_ENDRX_Msk (0x1UL << SPIM_INTENCLR_ENDRX_Pos) /*!< Bit mask of ENDRX field. */
#define SPIM_INTENCLR_ENDRX_Disabled (0UL) /*!< Read: Disabled */
#define SPIM_INTENCLR_ENDRX_Enabled (1UL) /*!< Read: Enabled */
#define SPIM_INTENCLR_ENDRX_Clear (1UL) /*!< Disable */

/* Bit 1 : Write '1' to disable interrupt for event STOPPED */
#define SPIM_INTENCLR_STOPPED_Pos (1UL) /*!< Position of STOPPED field. */
#define SPIM_INTENCLR_STOPPED_Msk (0x1UL << SPIM_INTENCLR_STOPPED_Pos) /*!< Bit mask of STOPPED field. */
#define SPIM_INTENCLR_STOPPED_Disabled (0UL) /*!< Read: Disabled */
#define SPIM_INTENCLR_STOPPED_Enabled (1UL) /*!< Read: Enabled */
#define SPIM_INTENCLR_STOPPED_Clear (1UL) /*!< Disable */

/* Register: SPIM_ENABLE */
/* Description: Enable SPIM */

/* Bits 3..0 : Enable or disable SPIM */
#define SPIM_ENABLE_ENABLE_Pos (0UL) /*!< Position of ENABLE field. */
#define SPIM_ENABLE_ENABLE_Msk (0xFUL << SPIM_ENABLE_ENABLE_Pos) /*!< Bit mask of ENABLE field. */
#define SPIM_ENABLE_ENABLE_Disabled (0UL) /*!< Disable SPIM */
#define SPIM_ENABLE_ENABLE_Enabled (7UL) /*!< Enable SPIM */

/* Register: SPIM_PSEL_SCK */
/* Description: Pin select for SCK */

/* Bit 31 : Connection */
#define SPIM_PSEL_SCK_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define SPIM_PSEL_SCK_CONNECT_Msk (0x1UL << SPIM_PSEL_SCK_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define SPIM_PSEL_SCK_CONNECT_Connected (0UL) /*!< Connect */
#define SPIM_PSEL_SCK_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define SPIM_PSEL_SCK_PIN_Pos (0UL) /*!< Position of PIN field. */
#define SPIM_PSEL_SCK_PIN_Msk (0x1FUL << SPIM_PSEL_SCK_PIN_Pos) /*!< Bit mask of PIN field. */

/* Register: SPIM_PSEL_MOSI */
/* Description: Pin select for MOSI signal */

/* Bit 31 : Connection */
#define SPIM_PSEL_MOSI_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define SPIM_PSEL_MOSI_CONNECT_Msk (0x1UL << SPIM_PSEL_MOSI_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define SPIM_PSEL_MOSI_CONNECT_Connected (0UL) /*!< Connect */
#define SPIM_PSEL_MOSI_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define SPIM_PSEL_MOSI_PIN_Pos (0UL) /*!< Position of PIN field. */
#define SPIM_PSEL_MOSI_PIN_Msk (0x1FUL << SPIM_PSEL_MOSI_PIN_Pos) /*!< Bit mask of PIN field. */

/* Register: SPIM_PSEL_MISO */
/* Description: Pin select for MISO signal */

/* Bit 31 : Connection */
#define SPIM_PSEL_MISO_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define SPIM_PSEL_MISO_CONNECT_Msk (0x1UL << SPIM_PSEL_MISO_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define SPIM_PSEL_MISO_CONNECT_Connected (0UL) /*!< Connect */
#define SPIM_PSEL_MISO_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define SPIM_PSEL_MISO_PIN_Pos (0UL) /*!< Position of PIN field. */
#define SPIM_PSEL_MISO_PIN_Msk (0x1FUL << SPIM_PSEL_MISO_PIN_Pos) /*!< Bit mask of PIN field. */

/* Register: SPIM_FREQUENCY */
/* Description: SPI frequency. Accuracy depends on the HFCLK source selected. */

/* Bits 31..0 : SPI master data rate */
#define SPIM_FREQUENCY_FREQUENCY_Pos (0UL) /*!< Position of FREQUENCY field. */
#define SPIM_FREQUENCY_FREQUENCY_Msk (0xFFFFFFFFUL << SPIM_FREQUENCY_FREQUENCY_Pos) /*!< Bit mask of FREQUENCY field. */
#define SPIM_FREQUENCY_FREQUENCY_K125 (0x02000000UL) /*!< 125 kbps */
#define SPIM_FREQUENCY_FREQUENCY_K250 (0x04000000UL) /*!< 250 kbps */
#define SPIM_FREQUENCY_FREQUENCY_K500 (0x08000000UL) /*!< 500 kbps */
#define SPIM_FREQUENCY_FREQUENCY_M1 (0x10000000UL) /*!< 1 Mbps */
#define SPIM_FREQUENCY_FREQUENCY_M2 (0x20000000UL) /*!< 2 Mbps */
#define SPIM_FREQUENCY_FREQUENCY_M4 (0x40000000UL) /*!< 4 Mbps */
#define SPIM_FREQUENCY_FREQUENCY_M8 (0x80000000UL) /*!< 8 Mbps */

/* Register: SPIM_RXD_PTR */
/* Description: Data pointer */

/* Bits 31..0 : Data pointer */
#define SPIM_RXD_PTR_PTR_Pos (0UL) /*!< Position of PTR field. */
#define SPIM_RXD_PTR_PTR_Msk (0xFFFFFFFFUL << SPIM_RXD_PTR_PTR_Pos) /*!< Bit mask of PTR field. */

/* Register: SPIM_RXD_MAXCNT */
/* Description: Maximum number of bytes in receive buffer */

/* Bits 12..0 : Maximum number of bytes in receive buffer */
#define SPIM_RXD_MAXCNT_MAXCNT_Pos (0UL) /*!< Position of MAXCNT field. */
#define SPIM_RXD_MAXCNT_MAXCNT_Msk (0x1FFFUL << SPIM_RXD_MAXCNT_MAXCNT_Pos) /*!< Bit mask of MAXCNT field. */

/* Register: SPIM_RXD_AMOUNT */
/* Description: Number of bytes transferred in the last transaction */

/* Bits 12..0 : Number of bytes transferred in the last transaction */
#define SPIM_RXD_AMOUNT_AMOUNT_Pos (0UL) /*!< Position of AMOUNT field. */
#define SPIM_RXD_AMOUNT_AMOUNT_Msk (0x1FFFUL << SPIM_RXD_AMOUNT_AMOUNT_Pos) /*!< Bit mask of AMOUNT field. */

/* Register: SPIM_RXD_LIST */
/* Description: EasyDMA list type */

/* Bits 1..0 : List type */
#define SPIM_RXD_LIST_LIST_Pos (0UL) /*!< Position of LIST field. */
#define SPIM_RXD_LIST_LIST_Msk (0x3UL << SPIM_RXD_LIST_LIST_Pos) /*!< Bit mask of LIST field. */
#define SPIM_RXD_LIST_LIST_Disabled (0UL) /*!< Disable EasyDMA list */
#define SPIM_RXD_LIST_LIST_ArrayList (1UL) /*!< Use array list */

/* Register: SPIM_TXD_PTR */
/* Description: Data pointer */

/* Bits 31..0 : Data pointer */
#define SPIM_TXD_PTR_PTR_Pos (0UL) /*!< Position of PTR field. */
#define SPIM_TXD_PTR_PTR_Msk (0xFFFFFFFFUL << SPIM_TXD_PTR_PTR_Pos) /*!< Bit mask of PTR field. */

/* Register: SPIM_TXD_MAXCNT */
/* Description: Maximum number of bytes in transmit buffer */

/* Bits 12..0 : Maximum number of bytes in transmit buffer */
#define SPIM_TXD_MAXCNT_MAXCNT_Pos (0UL) /*!< Position of MAXCNT field. */
#define SPIM_TXD_MAXCNT_MAXCNT_Msk (0x1FFFUL << SPIM_TXD_MAXCNT_MAXCNT_Pos) /*!< Bit mask of MAXCNT field. */

/* Register: SPIM_TXD_AMOUNT */
/* Description: Number of bytes transferred in the last transaction */

/* Bits 12..0 : Number of bytes transferred in the last transaction */
#define SPIM_TXD_AMOUNT_AMOUNT_Pos (0UL) /*!< Position of AMOUNT field. */
#define SPIM_TXD_AMOUNT_AMOUNT_Msk (0x1FFFUL << SPIM_TXD_AMOUNT_AMOUNT_Pos) /*!< Bit mask of AMOUNT field. */

/* Register: SPIM_TXD_LIST */
/* Description: EasyDMA list type */

/* Bits 1..0 : List type */
#define SPIM_TXD_LIST_LIST_Pos (0UL) /*!< Position of LIST field. */
#define SPIM_TXD_LIST_LIST_Msk (0x3UL << SPIM_TXD_LIST_LIST_Pos) /*!< Bit mask of LIST field. */
#define SPIM_TXD_LIST_LIST_Disabled (0UL) /*!< Disable EasyDMA list */
#define SPIM_TXD_LIST_LIST_ArrayList (1UL) /*!< Use array list */

/* Register: SPIM_CONFIG */
/* Description: Configuration register */

/* Bit 2 : Serial clock (SCK) polarity */
#define SPIM_CONFIG_CPOL_Pos (2UL) /*!< Position of CPOL field. */
#define SPIM_CONFIG_CPOL_Msk (0x1UL << SPIM_CONFIG_CPOL_Pos) /*!< Bit mask of CPOL field. */
#define SPIM_CONFIG_CPOL_ActiveHigh (0UL) /*!< Active high */
#define SPIM_CONFIG_CPOL_ActiveLow (1UL) /*!< Active low */

/* Bit 1 : Serial clock (SCK) phase */
#define SPIM_CONFIG_CPHA_Pos (1UL) /*!< Position of CPHA field. */
#define SPIM_CONFIG_CPHA_Msk (0x1UL << SPIM_CONFIG_CPHA_Pos) /*!< Bit mask of CPHA field. */
#define SPIM_CONFIG_CPHA_Leading (0UL) /*!< Sample on leading edge of clock, shift serial data on trailing edge */
#define SPIM_CONFIG_CPHA_Trailing (1UL) /*!< Sample on trailing edge of clock, shift serial data on leading edge */

/* Bit 0 : Bit order */
#define SPIM_CONFIG_ORDER_Pos (0UL) /*!< Position of ORDER field. */
#define SPIM_CONFIG_ORDER_Msk (0x1UL << SPIM_CONFIG_ORDER_Pos) /*!< Bit mask of ORDER field. */
#define SPIM_CONFIG_ORDER_MsbFirst (0UL) /*!< Most significant bit shifted out first */
#define SPIM_CONFIG_ORDER_LsbFirst (1UL) /*!< Least significant bit shifted out first */

/* Register: SPIM_ORC */
/* Description: Over-read character. Character clocked out in case and over-read of the TXD buffer. */

/* Bits 7..0 : Over-read character. Character clocked out in case and over-read of the TXD buffer. */
#define SPIM_ORC_ORC_Pos (0UL) /*!< Position of ORC field. */
#define SPIM_ORC_ORC_Msk (0xFFUL << SPIM_ORC_ORC_Pos) /*!< Bit mask of ORC field. */


/* Peripheral: SPIS */
/* Description: SPI Slave 0 */

/* Register: SPIS_TASKS_ACQUIRE */
/* Description: Acquire SPI semaphore */

/* Bit 0 : Acquire SPI semaphore */
#define SPIS_TASKS_ACQUIRE_TASKS_ACQUIRE_Pos (0UL) /*!< Position of TASKS_ACQUIRE field. */
#define SPIS_TASKS_ACQUIRE_TASKS_ACQUIRE_Msk (0x1UL << SPIS_TASKS_ACQUIRE_TASKS_ACQUIRE_Pos) /*!< Bit mask of TASKS_ACQUIRE field. */
#define SPIS_TASKS_ACQUIRE_TASKS_ACQUIRE_Trigger (1UL) /*!< Trigger task */

/* Register: SPIS_TASKS_RELEASE */
/* Description: Release SPI semaphore, enabling the SPI slave to acquire it */

/* Bit 0 : Release SPI semaphore, enabling the SPI slave to acquire it */
#define SPIS_TASKS_RELEASE_TASKS_RELEASE_Pos (0UL) /*!< Position of TASKS_RELEASE field. */
#define SPIS_TASKS_RELEASE_TASKS_RELEASE_Msk (0x1UL << SPIS_TASKS_RELEASE_TASKS_RELEASE_Pos) /*!< Bit mask of TASKS_RELEASE field. */
#define SPIS_TASKS_RELEASE_TASKS_RELEASE_Trigger (1UL) /*!< Trigger task */

/* Register: SPIS_SUBSCRIBE_ACQUIRE */
/* Description: Subscribe configuration for task ACQUIRE */

/* Bit 31 :   */
#define SPIS_SUBSCRIBE_ACQUIRE_EN_Pos (31UL) /*!< Position of EN field. */
#define SPIS_SUBSCRIBE_ACQUIRE_EN_Msk (0x1UL << SPIS_SUBSCRIBE_ACQUIRE_EN_Pos) /*!< Bit mask of EN field. */
#define SPIS_SUBSCRIBE_ACQUIRE_EN_Disabled (0UL) /*!< Disable subscription */
#define SPIS_SUBSCRIBE_ACQUIRE_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task ACQUIRE will subscribe to */
#define SPIS_SUBSCRIBE_ACQUIRE_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SPIS_SUBSCRIBE_ACQUIRE_CHIDX_Msk (0xFUL << SPIS_SUBSCRIBE_ACQUIRE_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SPIS_SUBSCRIBE_RELEASE */
/* Description: Subscribe configuration for task RELEASE */

/* Bit 31 :   */
#define SPIS_SUBSCRIBE_RELEASE_EN_Pos (31UL) /*!< Position of EN field. */
#define SPIS_SUBSCRIBE_RELEASE_EN_Msk (0x1UL << SPIS_SUBSCRIBE_RELEASE_EN_Pos) /*!< Bit mask of EN field. */
#define SPIS_SUBSCRIBE_RELEASE_EN_Disabled (0UL) /*!< Disable subscription */
#define SPIS_SUBSCRIBE_RELEASE_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task RELEASE will subscribe to */
#define SPIS_SUBSCRIBE_RELEASE_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SPIS_SUBSCRIBE_RELEASE_CHIDX_Msk (0xFUL << SPIS_SUBSCRIBE_RELEASE_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SPIS_EVENTS_END */
/* Description: Granted transaction completed */

/* Bit 0 : Granted transaction completed */
#define SPIS_EVENTS_END_EVENTS_END_Pos (0UL) /*!< Position of EVENTS_END field. */
#define SPIS_EVENTS_END_EVENTS_END_Msk (0x1UL << SPIS_EVENTS_END_EVENTS_END_Pos) /*!< Bit mask of EVENTS_END field. */
#define SPIS_EVENTS_END_EVENTS_END_NotGenerated (0UL) /*!< Event not generated */
#define SPIS_EVENTS_END_EVENTS_END_Generated (1UL) /*!< Event generated */

/* Register: SPIS_EVENTS_ENDRX */
/* Description: End of RXD buffer reached */

/* Bit 0 : End of RXD buffer reached */
#define SPIS_EVENTS_ENDRX_EVENTS_ENDRX_Pos (0UL) /*!< Position of EVENTS_ENDRX field. */
#define SPIS_EVENTS_ENDRX_EVENTS_ENDRX_Msk (0x1UL << SPIS_EVENTS_ENDRX_EVENTS_ENDRX_Pos) /*!< Bit mask of EVENTS_ENDRX field. */
#define SPIS_EVENTS_ENDRX_EVENTS_ENDRX_NotGenerated (0UL) /*!< Event not generated */
#define SPIS_EVENTS_ENDRX_EVENTS_ENDRX_Generated (1UL) /*!< Event generated */

/* Register: SPIS_EVENTS_ACQUIRED */
/* Description: Semaphore acquired */

/* Bit 0 : Semaphore acquired */
#define SPIS_EVENTS_ACQUIRED_EVENTS_ACQUIRED_Pos (0UL) /*!< Position of EVENTS_ACQUIRED field. */
#define SPIS_EVENTS_ACQUIRED_EVENTS_ACQUIRED_Msk (0x1UL << SPIS_EVENTS_ACQUIRED_EVENTS_ACQUIRED_Pos) /*!< Bit mask of EVENTS_ACQUIRED field. */
#define SPIS_EVENTS_ACQUIRED_EVENTS_ACQUIRED_NotGenerated (0UL) /*!< Event not generated */
#define SPIS_EVENTS_ACQUIRED_EVENTS_ACQUIRED_Generated (1UL) /*!< Event generated */

/* Register: SPIS_PUBLISH_END */
/* Description: Publish configuration for event END */

/* Bit 31 :   */
#define SPIS_PUBLISH_END_EN_Pos (31UL) /*!< Position of EN field. */
#define SPIS_PUBLISH_END_EN_Msk (0x1UL << SPIS_PUBLISH_END_EN_Pos) /*!< Bit mask of EN field. */
#define SPIS_PUBLISH_END_EN_Disabled (0UL) /*!< Disable publishing */
#define SPIS_PUBLISH_END_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event END will publish to. */
#define SPIS_PUBLISH_END_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SPIS_PUBLISH_END_CHIDX_Msk (0xFUL << SPIS_PUBLISH_END_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SPIS_PUBLISH_ENDRX */
/* Description: Publish configuration for event ENDRX */

/* Bit 31 :   */
#define SPIS_PUBLISH_ENDRX_EN_Pos (31UL) /*!< Position of EN field. */
#define SPIS_PUBLISH_ENDRX_EN_Msk (0x1UL << SPIS_PUBLISH_ENDRX_EN_Pos) /*!< Bit mask of EN field. */
#define SPIS_PUBLISH_ENDRX_EN_Disabled (0UL) /*!< Disable publishing */
#define SPIS_PUBLISH_ENDRX_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event ENDRX will publish to. */
#define SPIS_PUBLISH_ENDRX_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SPIS_PUBLISH_ENDRX_CHIDX_Msk (0xFUL << SPIS_PUBLISH_ENDRX_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SPIS_PUBLISH_ACQUIRED */
/* Description: Publish configuration for event ACQUIRED */

/* Bit 31 :   */
#define SPIS_PUBLISH_ACQUIRED_EN_Pos (31UL) /*!< Position of EN field. */
#define SPIS_PUBLISH_ACQUIRED_EN_Msk (0x1UL << SPIS_PUBLISH_ACQUIRED_EN_Pos) /*!< Bit mask of EN field. */
#define SPIS_PUBLISH_ACQUIRED_EN_Disabled (0UL) /*!< Disable publishing */
#define SPIS_PUBLISH_ACQUIRED_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event ACQUIRED will publish to. */
#define SPIS_PUBLISH_ACQUIRED_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SPIS_PUBLISH_ACQUIRED_CHIDX_Msk (0xFUL << SPIS_PUBLISH_ACQUIRED_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SPIS_SHORTS */
/* Description: Shortcuts between local events and tasks */

/* Bit 2 : Shortcut between event END and task ACQUIRE */
#define SPIS_SHORTS_END_ACQUIRE_Pos (2UL) /*!< Position of END_ACQUIRE field. */
#define SPIS_SHORTS_END_ACQUIRE_Msk (0x1UL << SPIS_SHORTS_END_ACQUIRE_Pos) /*!< Bit mask of END_ACQUIRE field. */
#define SPIS_SHORTS_END_ACQUIRE_Disabled (0UL) /*!< Disable shortcut */
#define SPIS_SHORTS_END_ACQUIRE_Enabled (1UL) /*!< Enable shortcut */

/* Register: SPIS_INTENSET */
/* Description: Enable interrupt */

/* Bit 10 : Write '1' to enable interrupt for event ACQUIRED */
#define SPIS_INTENSET_ACQUIRED_Pos (10UL) /*!< Position of ACQUIRED field. */
#define SPIS_INTENSET_ACQUIRED_Msk (0x1UL << SPIS_INTENSET_ACQUIRED_Pos) /*!< Bit mask of ACQUIRED field. */
#define SPIS_INTENSET_ACQUIRED_Disabled (0UL) /*!< Read: Disabled */
#define SPIS_INTENSET_ACQUIRED_Enabled (1UL) /*!< Read: Enabled */
#define SPIS_INTENSET_ACQUIRED_Set (1UL) /*!< Enable */

/* Bit 4 : Write '1' to enable interrupt for event ENDRX */
#define SPIS_INTENSET_ENDRX_Pos (4UL) /*!< Position of ENDRX field. */
#define SPIS_INTENSET_ENDRX_Msk (0x1UL << SPIS_INTENSET_ENDRX_Pos) /*!< Bit mask of ENDRX field. */
#define SPIS_INTENSET_ENDRX_Disabled (0UL) /*!< Read: Disabled */
#define SPIS_INTENSET_ENDRX_Enabled (1UL) /*!< Read: Enabled */
#define SPIS_INTENSET_ENDRX_Set (1UL) /*!< Enable */

/* Bit 1 : Write '1' to enable interrupt for event END */
#define SPIS_INTENSET_END_Pos (1UL) /*!< Position of END field. */
#define SPIS_INTENSET_END_Msk (0x1UL << SPIS_INTENSET_END_Pos) /*!< Bit mask of END field. */
#define SPIS_INTENSET_END_Disabled (0UL) /*!< Read: Disabled */
#define SPIS_INTENSET_END_Enabled (1UL) /*!< Read: Enabled */
#define SPIS_INTENSET_END_Set (1UL) /*!< Enable */

/* Register: SPIS_INTENCLR */
/* Description: Disable interrupt */

/* Bit 10 : Write '1' to disable interrupt for event ACQUIRED */
#define SPIS_INTENCLR_ACQUIRED_Pos (10UL) /*!< Position of ACQUIRED field. */
#define SPIS_INTENCLR_ACQUIRED_Msk (0x1UL << SPIS_INTENCLR_ACQUIRED_Pos) /*!< Bit mask of ACQUIRED field. */
#define SPIS_INTENCLR_ACQUIRED_Disabled (0UL) /*!< Read: Disabled */
#define SPIS_INTENCLR_ACQUIRED_Enabled (1UL) /*!< Read: Enabled */
#define SPIS_INTENCLR_ACQUIRED_Clear (1UL) /*!< Disable */

/* Bit 4 : Write '1' to disable interrupt for event ENDRX */
#define SPIS_INTENCLR_ENDRX_Pos (4UL) /*!< Position of ENDRX field. */
#define SPIS_INTENCLR_ENDRX_Msk (0x1UL << SPIS_INTENCLR_ENDRX_Pos) /*!< Bit mask of ENDRX field. */
#define SPIS_INTENCLR_ENDRX_Disabled (0UL) /*!< Read: Disabled */
#define SPIS_INTENCLR_ENDRX_Enabled (1UL) /*!< Read: Enabled */
#define SPIS_INTENCLR_ENDRX_Clear (1UL) /*!< Disable */

/* Bit 1 : Write '1' to disable interrupt for event END */
#define SPIS_INTENCLR_END_Pos (1UL) /*!< Position of END field. */
#define SPIS_INTENCLR_END_Msk (0x1UL << SPIS_INTENCLR_END_Pos) /*!< Bit mask of END field. */
#define SPIS_INTENCLR_END_Disabled (0UL) /*!< Read: Disabled */
#define SPIS_INTENCLR_END_Enabled (1UL) /*!< Read: Enabled */
#define SPIS_INTENCLR_END_Clear (1UL) /*!< Disable */

/* Register: SPIS_SEMSTAT */
/* Description: Semaphore status register */

/* Bits 1..0 : Semaphore status */
#define SPIS_SEMSTAT_SEMSTAT_Pos (0UL) /*!< Position of SEMSTAT field. */
#define SPIS_SEMSTAT_SEMSTAT_Msk (0x3UL << SPIS_SEMSTAT_SEMSTAT_Pos) /*!< Bit mask of SEMSTAT field. */
#define SPIS_SEMSTAT_SEMSTAT_Free (0UL) /*!< Semaphore is free */
#define SPIS_SEMSTAT_SEMSTAT_CPU (1UL) /*!< Semaphore is assigned to CPU */
#define SPIS_SEMSTAT_SEMSTAT_SPIS (2UL) /*!< Semaphore is assigned to SPI slave */
#define SPIS_SEMSTAT_SEMSTAT_CPUPending (3UL) /*!< Semaphore is assigned to SPI but a handover to the CPU is pending */

/* Register: SPIS_STATUS */
/* Description: Status from last transaction */

/* Bit 1 : RX buffer overflow detected, and prevented */
#define SPIS_STATUS_OVERFLOW_Pos (1UL) /*!< Position of OVERFLOW field. */
#define SPIS_STATUS_OVERFLOW_Msk (0x1UL << SPIS_STATUS_OVERFLOW_Pos) /*!< Bit mask of OVERFLOW field. */
#define SPIS_STATUS_OVERFLOW_NotPresent (0UL) /*!< Read: error not present */
#define SPIS_STATUS_OVERFLOW_Present (1UL) /*!< Read: error present */
#define SPIS_STATUS_OVERFLOW_Clear (1UL) /*!< Write: clear error on writing '1' */

/* Bit 0 : TX buffer over-read detected, and prevented */
#define SPIS_STATUS_OVERREAD_Pos (0UL) /*!< Position of OVERREAD field. */
#define SPIS_STATUS_OVERREAD_Msk (0x1UL << SPIS_STATUS_OVERREAD_Pos) /*!< Bit mask of OVERREAD field. */
#define SPIS_STATUS_OVERREAD_NotPresent (0UL) /*!< Read: error not present */
#define SPIS_STATUS_OVERREAD_Present (1UL) /*!< Read: error present */
#define SPIS_STATUS_OVERREAD_Clear (1UL) /*!< Write: clear error on writing '1' */

/* Register: SPIS_ENABLE */
/* Description: Enable SPI slave */

/* Bits 3..0 : Enable or disable SPI slave */
#define SPIS_ENABLE_ENABLE_Pos (0UL) /*!< Position of ENABLE field. */
#define SPIS_ENABLE_ENABLE_Msk (0xFUL << SPIS_ENABLE_ENABLE_Pos) /*!< Bit mask of ENABLE field. */
#define SPIS_ENABLE_ENABLE_Disabled (0UL) /*!< Disable SPI slave */
#define SPIS_ENABLE_ENABLE_Enabled (2UL) /*!< Enable SPI slave */

/* Register: SPIS_PSEL_SCK */
/* Description: Pin select for SCK */

/* Bit 31 : Connection */
#define SPIS_PSEL_SCK_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define SPIS_PSEL_SCK_CONNECT_Msk (0x1UL << SPIS_PSEL_SCK_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define SPIS_PSEL_SCK_CONNECT_Connected (0UL) /*!< Connect */
#define SPIS_PSEL_SCK_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define SPIS_PSEL_SCK_PIN_Pos (0UL) /*!< Position of PIN field. */
#define SPIS_PSEL_SCK_PIN_Msk (0x1FUL << SPIS_PSEL_SCK_PIN_Pos) /*!< Bit mask of PIN field. */

/* Register: SPIS_PSEL_MISO */
/* Description: Pin select for MISO signal */

/* Bit 31 : Connection */
#define SPIS_PSEL_MISO_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define SPIS_PSEL_MISO_CONNECT_Msk (0x1UL << SPIS_PSEL_MISO_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define SPIS_PSEL_MISO_CONNECT_Connected (0UL) /*!< Connect */
#define SPIS_PSEL_MISO_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define SPIS_PSEL_MISO_PIN_Pos (0UL) /*!< Position of PIN field. */
#define SPIS_PSEL_MISO_PIN_Msk (0x1FUL << SPIS_PSEL_MISO_PIN_Pos) /*!< Bit mask of PIN field. */

/* Register: SPIS_PSEL_MOSI */
/* Description: Pin select for MOSI signal */

/* Bit 31 : Connection */
#define SPIS_PSEL_MOSI_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define SPIS_PSEL_MOSI_CONNECT_Msk (0x1UL << SPIS_PSEL_MOSI_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define SPIS_PSEL_MOSI_CONNECT_Connected (0UL) /*!< Connect */
#define SPIS_PSEL_MOSI_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define SPIS_PSEL_MOSI_PIN_Pos (0UL) /*!< Position of PIN field. */
#define SPIS_PSEL_MOSI_PIN_Msk (0x1FUL << SPIS_PSEL_MOSI_PIN_Pos) /*!< Bit mask of PIN field. */

/* Register: SPIS_PSEL_CSN */
/* Description: Pin select for CSN signal */

/* Bit 31 : Connection */
#define SPIS_PSEL_CSN_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define SPIS_PSEL_CSN_CONNECT_Msk (0x1UL << SPIS_PSEL_CSN_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define SPIS_PSEL_CSN_CONNECT_Connected (0UL) /*!< Connect */
#define SPIS_PSEL_CSN_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define SPIS_PSEL_CSN_PIN_Pos (0UL) /*!< Position of PIN field. */
#define SPIS_PSEL_CSN_PIN_Msk (0x1FUL << SPIS_PSEL_CSN_PIN_Pos) /*!< Bit mask of PIN field. */

/* Register: SPIS_RXD_PTR */
/* Description: RXD data pointer */

/* Bits 31..0 : RXD data pointer */
#define SPIS_RXD_PTR_PTR_Pos (0UL) /*!< Position of PTR field. */
#define SPIS_RXD_PTR_PTR_Msk (0xFFFFFFFFUL << SPIS_RXD_PTR_PTR_Pos) /*!< Bit mask of PTR field. */

/* Register: SPIS_RXD_MAXCNT */
/* Description: Maximum number of bytes in receive buffer */

/* Bits 12..0 : Maximum number of bytes in receive buffer */
#define SPIS_RXD_MAXCNT_MAXCNT_Pos (0UL) /*!< Position of MAXCNT field. */
#define SPIS_RXD_MAXCNT_MAXCNT_Msk (0x1FFFUL << SPIS_RXD_MAXCNT_MAXCNT_Pos) /*!< Bit mask of MAXCNT field. */

/* Register: SPIS_RXD_AMOUNT */
/* Description: Number of bytes received in last granted transaction */

/* Bits 12..0 : Number of bytes received in the last granted transaction */
#define SPIS_RXD_AMOUNT_AMOUNT_Pos (0UL) /*!< Position of AMOUNT field. */
#define SPIS_RXD_AMOUNT_AMOUNT_Msk (0x1FFFUL << SPIS_RXD_AMOUNT_AMOUNT_Pos) /*!< Bit mask of AMOUNT field. */

/* Register: SPIS_TXD_PTR */
/* Description: TXD data pointer */

/* Bits 31..0 : TXD data pointer */
#define SPIS_TXD_PTR_PTR_Pos (0UL) /*!< Position of PTR field. */
#define SPIS_TXD_PTR_PTR_Msk (0xFFFFFFFFUL << SPIS_TXD_PTR_PTR_Pos) /*!< Bit mask of PTR field. */

/* Register: SPIS_TXD_MAXCNT */
/* Description: Maximum number of bytes in transmit buffer */

/* Bits 12..0 : Maximum number of bytes in transmit buffer */
#define SPIS_TXD_MAXCNT_MAXCNT_Pos (0UL) /*!< Position of MAXCNT field. */
#define SPIS_TXD_MAXCNT_MAXCNT_Msk (0x1FFFUL << SPIS_TXD_MAXCNT_MAXCNT_Pos) /*!< Bit mask of MAXCNT field. */

/* Register: SPIS_TXD_AMOUNT */
/* Description: Number of bytes transmitted in last granted transaction */

/* Bits 12..0 : Number of bytes transmitted in last granted transaction */
#define SPIS_TXD_AMOUNT_AMOUNT_Pos (0UL) /*!< Position of AMOUNT field. */
#define SPIS_TXD_AMOUNT_AMOUNT_Msk (0x1FFFUL << SPIS_TXD_AMOUNT_AMOUNT_Pos) /*!< Bit mask of AMOUNT field. */

/* Register: SPIS_CONFIG */
/* Description: Configuration register */

/* Bit 2 : Serial clock (SCK) polarity */
#define SPIS_CONFIG_CPOL_Pos (2UL) /*!< Position of CPOL field. */
#define SPIS_CONFIG_CPOL_Msk (0x1UL << SPIS_CONFIG_CPOL_Pos) /*!< Bit mask of CPOL field. */
#define SPIS_CONFIG_CPOL_ActiveHigh (0UL) /*!< Active high */
#define SPIS_CONFIG_CPOL_ActiveLow (1UL) /*!< Active low */

/* Bit 1 : Serial clock (SCK) phase */
#define SPIS_CONFIG_CPHA_Pos (1UL) /*!< Position of CPHA field. */
#define SPIS_CONFIG_CPHA_Msk (0x1UL << SPIS_CONFIG_CPHA_Pos) /*!< Bit mask of CPHA field. */
#define SPIS_CONFIG_CPHA_Leading (0UL) /*!< Sample on leading edge of clock, shift serial data on trailing edge */
#define SPIS_CONFIG_CPHA_Trailing (1UL) /*!< Sample on trailing edge of clock, shift serial data on leading edge */

/* Bit 0 : Bit order */
#define SPIS_CONFIG_ORDER_Pos (0UL) /*!< Position of ORDER field. */
#define SPIS_CONFIG_ORDER_Msk (0x1UL << SPIS_CONFIG_ORDER_Pos) /*!< Bit mask of ORDER field. */
#define SPIS_CONFIG_ORDER_MsbFirst (0UL) /*!< Most significant bit shifted out first */
#define SPIS_CONFIG_ORDER_LsbFirst (1UL) /*!< Least significant bit shifted out first */

/* Register: SPIS_DEF */
/* Description: Default character. Character clocked out in case of an ignored transaction. */

/* Bits 7..0 : Default character. Character clocked out in case of an ignored transaction. */
#define SPIS_DEF_DEF_Pos (0UL) /*!< Position of DEF field. */
#define SPIS_DEF_DEF_Msk (0xFFUL << SPIS_DEF_DEF_Pos) /*!< Bit mask of DEF field. */

/* Register: SPIS_ORC */
/* Description: Over-read character */

/* Bits 7..0 : Over-read character. Character clocked out after an over-read of the transmit buffer. */
#define SPIS_ORC_ORC_Pos (0UL) /*!< Position of ORC field. */
#define SPIS_ORC_ORC_Msk (0xFFUL << SPIS_ORC_ORC_Pos) /*!< Bit mask of ORC field. */


/* Peripheral: SPU */
/* Description: System protection unit */

/* Register: SPU_EVENTS_RAMACCERR */
/* Description: A security violation has been detected for the RAM memory space */

/* Bit 0 : A security violation has been detected for the RAM memory space */
#define SPU_EVENTS_RAMACCERR_EVENTS_RAMACCERR_Pos (0UL) /*!< Position of EVENTS_RAMACCERR field. */
#define SPU_EVENTS_RAMACCERR_EVENTS_RAMACCERR_Msk (0x1UL << SPU_EVENTS_RAMACCERR_EVENTS_RAMACCERR_Pos) /*!< Bit mask of EVENTS_RAMACCERR field. */
#define SPU_EVENTS_RAMACCERR_EVENTS_RAMACCERR_NotGenerated (0UL) /*!< Event not generated */
#define SPU_EVENTS_RAMACCERR_EVENTS_RAMACCERR_Generated (1UL) /*!< Event generated */

/* Register: SPU_EVENTS_FLASHACCERR */
/* Description: A security violation has been detected for the flash memory space */

/* Bit 0 : A security violation has been detected for the flash memory space */
#define SPU_EVENTS_FLASHACCERR_EVENTS_FLASHACCERR_Pos (0UL) /*!< Position of EVENTS_FLASHACCERR field. */
#define SPU_EVENTS_FLASHACCERR_EVENTS_FLASHACCERR_Msk (0x1UL << SPU_EVENTS_FLASHACCERR_EVENTS_FLASHACCERR_Pos) /*!< Bit mask of EVENTS_FLASHACCERR field. */
#define SPU_EVENTS_FLASHACCERR_EVENTS_FLASHACCERR_NotGenerated (0UL) /*!< Event not generated */
#define SPU_EVENTS_FLASHACCERR_EVENTS_FLASHACCERR_Generated (1UL) /*!< Event generated */

/* Register: SPU_EVENTS_PERIPHACCERR */
/* Description: A security violation has been detected on one or several peripherals */

/* Bit 0 : A security violation has been detected on one or several peripherals */
#define SPU_EVENTS_PERIPHACCERR_EVENTS_PERIPHACCERR_Pos (0UL) /*!< Position of EVENTS_PERIPHACCERR field. */
#define SPU_EVENTS_PERIPHACCERR_EVENTS_PERIPHACCERR_Msk (0x1UL << SPU_EVENTS_PERIPHACCERR_EVENTS_PERIPHACCERR_Pos) /*!< Bit mask of EVENTS_PERIPHACCERR field. */
#define SPU_EVENTS_PERIPHACCERR_EVENTS_PERIPHACCERR_NotGenerated (0UL) /*!< Event not generated */
#define SPU_EVENTS_PERIPHACCERR_EVENTS_PERIPHACCERR_Generated (1UL) /*!< Event generated */

/* Register: SPU_PUBLISH_RAMACCERR */
/* Description: Publish configuration for event RAMACCERR */

/* Bit 31 :   */
#define SPU_PUBLISH_RAMACCERR_EN_Pos (31UL) /*!< Position of EN field. */
#define SPU_PUBLISH_RAMACCERR_EN_Msk (0x1UL << SPU_PUBLISH_RAMACCERR_EN_Pos) /*!< Bit mask of EN field. */
#define SPU_PUBLISH_RAMACCERR_EN_Disabled (0UL) /*!< Disable publishing */
#define SPU_PUBLISH_RAMACCERR_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event RAMACCERR will publish to. */
#define SPU_PUBLISH_RAMACCERR_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SPU_PUBLISH_RAMACCERR_CHIDX_Msk (0xFUL << SPU_PUBLISH_RAMACCERR_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SPU_PUBLISH_FLASHACCERR */
/* Description: Publish configuration for event FLASHACCERR */

/* Bit 31 :   */
#define SPU_PUBLISH_FLASHACCERR_EN_Pos (31UL) /*!< Position of EN field. */
#define SPU_PUBLISH_FLASHACCERR_EN_Msk (0x1UL << SPU_PUBLISH_FLASHACCERR_EN_Pos) /*!< Bit mask of EN field. */
#define SPU_PUBLISH_FLASHACCERR_EN_Disabled (0UL) /*!< Disable publishing */
#define SPU_PUBLISH_FLASHACCERR_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event FLASHACCERR will publish to. */
#define SPU_PUBLISH_FLASHACCERR_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SPU_PUBLISH_FLASHACCERR_CHIDX_Msk (0xFUL << SPU_PUBLISH_FLASHACCERR_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SPU_PUBLISH_PERIPHACCERR */
/* Description: Publish configuration for event PERIPHACCERR */

/* Bit 31 :   */
#define SPU_PUBLISH_PERIPHACCERR_EN_Pos (31UL) /*!< Position of EN field. */
#define SPU_PUBLISH_PERIPHACCERR_EN_Msk (0x1UL << SPU_PUBLISH_PERIPHACCERR_EN_Pos) /*!< Bit mask of EN field. */
#define SPU_PUBLISH_PERIPHACCERR_EN_Disabled (0UL) /*!< Disable publishing */
#define SPU_PUBLISH_PERIPHACCERR_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event PERIPHACCERR will publish to. */
#define SPU_PUBLISH_PERIPHACCERR_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define SPU_PUBLISH_PERIPHACCERR_CHIDX_Msk (0xFUL << SPU_PUBLISH_PERIPHACCERR_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: SPU_INTEN */
/* Description: Enable or disable interrupt */

/* Bit 2 : Enable or disable interrupt for event PERIPHACCERR */
#define SPU_INTEN_PERIPHACCERR_Pos (2UL) /*!< Position of PERIPHACCERR field. */
#define SPU_INTEN_PERIPHACCERR_Msk (0x1UL << SPU_INTEN_PERIPHACCERR_Pos) /*!< Bit mask of PERIPHACCERR field. */
#define SPU_INTEN_PERIPHACCERR_Disabled (0UL) /*!< Disable */
#define SPU_INTEN_PERIPHACCERR_Enabled (1UL) /*!< Enable */

/* Bit 1 : Enable or disable interrupt for event FLASHACCERR */
#define SPU_INTEN_FLASHACCERR_Pos (1UL) /*!< Position of FLASHACCERR field. */
#define SPU_INTEN_FLASHACCERR_Msk (0x1UL << SPU_INTEN_FLASHACCERR_Pos) /*!< Bit mask of FLASHACCERR field. */
#define SPU_INTEN_FLASHACCERR_Disabled (0UL) /*!< Disable */
#define SPU_INTEN_FLASHACCERR_Enabled (1UL) /*!< Enable */

/* Bit 0 : Enable or disable interrupt for event RAMACCERR */
#define SPU_INTEN_RAMACCERR_Pos (0UL) /*!< Position of RAMACCERR field. */
#define SPU_INTEN_RAMACCERR_Msk (0x1UL << SPU_INTEN_RAMACCERR_Pos) /*!< Bit mask of RAMACCERR field. */
#define SPU_INTEN_RAMACCERR_Disabled (0UL) /*!< Disable */
#define SPU_INTEN_RAMACCERR_Enabled (1UL) /*!< Enable */

/* Register: SPU_INTENSET */
/* Description: Enable interrupt */

/* Bit 2 : Write '1' to enable interrupt for event PERIPHACCERR */
#define SPU_INTENSET_PERIPHACCERR_Pos (2UL) /*!< Position of PERIPHACCERR field. */
#define SPU_INTENSET_PERIPHACCERR_Msk (0x1UL << SPU_INTENSET_PERIPHACCERR_Pos) /*!< Bit mask of PERIPHACCERR field. */
#define SPU_INTENSET_PERIPHACCERR_Disabled (0UL) /*!< Read: Disabled */
#define SPU_INTENSET_PERIPHACCERR_Enabled (1UL) /*!< Read: Enabled */
#define SPU_INTENSET_PERIPHACCERR_Set (1UL) /*!< Enable */

/* Bit 1 : Write '1' to enable interrupt for event FLASHACCERR */
#define SPU_INTENSET_FLASHACCERR_Pos (1UL) /*!< Position of FLASHACCERR field. */
#define SPU_INTENSET_FLASHACCERR_Msk (0x1UL << SPU_INTENSET_FLASHACCERR_Pos) /*!< Bit mask of FLASHACCERR field. */
#define SPU_INTENSET_FLASHACCERR_Disabled (0UL) /*!< Read: Disabled */
#define SPU_INTENSET_FLASHACCERR_Enabled (1UL) /*!< Read: Enabled */
#define SPU_INTENSET_FLASHACCERR_Set (1UL) /*!< Enable */

/* Bit 0 : Write '1' to enable interrupt for event RAMACCERR */
#define SPU_INTENSET_RAMACCERR_Pos (0UL) /*!< Position of RAMACCERR field. */
#define SPU_INTENSET_RAMACCERR_Msk (0x1UL << SPU_INTENSET_RAMACCERR_Pos) /*!< Bit mask of RAMACCERR field. */
#define SPU_INTENSET_RAMACCERR_Disabled (0UL) /*!< Read: Disabled */
#define SPU_INTENSET_RAMACCERR_Enabled (1UL) /*!< Read: Enabled */
#define SPU_INTENSET_RAMACCERR_Set (1UL) /*!< Enable */

/* Register: SPU_INTENCLR */
/* Description: Disable interrupt */

/* Bit 2 : Write '1' to disable interrupt for event PERIPHACCERR */
#define SPU_INTENCLR_PERIPHACCERR_Pos (2UL) /*!< Position of PERIPHACCERR field. */
#define SPU_INTENCLR_PERIPHACCERR_Msk (0x1UL << SPU_INTENCLR_PERIPHACCERR_Pos) /*!< Bit mask of PERIPHACCERR field. */
#define SPU_INTENCLR_PERIPHACCERR_Disabled (0UL) /*!< Read: Disabled */
#define SPU_INTENCLR_PERIPHACCERR_Enabled (1UL) /*!< Read: Enabled */
#define SPU_INTENCLR_PERIPHACCERR_Clear (1UL) /*!< Disable */

/* Bit 1 : Write '1' to disable interrupt for event FLASHACCERR */
#define SPU_INTENCLR_FLASHACCERR_Pos (1UL) /*!< Position of FLASHACCERR field. */
#define SPU_INTENCLR_FLASHACCERR_Msk (0x1UL << SPU_INTENCLR_FLASHACCERR_Pos) /*!< Bit mask of FLASHACCERR field. */
#define SPU_INTENCLR_FLASHACCERR_Disabled (0UL) /*!< Read: Disabled */
#define SPU_INTENCLR_FLASHACCERR_Enabled (1UL) /*!< Read: Enabled */
#define SPU_INTENCLR_FLASHACCERR_Clear (1UL) /*!< Disable */

/* Bit 0 : Write '1' to disable interrupt for event RAMACCERR */
#define SPU_INTENCLR_RAMACCERR_Pos (0UL) /*!< Position of RAMACCERR field. */
#define SPU_INTENCLR_RAMACCERR_Msk (0x1UL << SPU_INTENCLR_RAMACCERR_Pos) /*!< Bit mask of RAMACCERR field. */
#define SPU_INTENCLR_RAMACCERR_Disabled (0UL) /*!< Read: Disabled */
#define SPU_INTENCLR_RAMACCERR_Enabled (1UL) /*!< Read: Enabled */
#define SPU_INTENCLR_RAMACCERR_Clear (1UL) /*!< Disable */

/* Register: SPU_CAP */
/* Description: Show implemented features for the current device */

/* Bit 0 : Show ARM TrustZone status */
#define SPU_CAP_TZM_Pos (0UL) /*!< Position of TZM field. */
#define SPU_CAP_TZM_Msk (0x1UL << SPU_CAP_TZM_Pos) /*!< Bit mask of TZM field. */
#define SPU_CAP_TZM_NotAvailable (0UL) /*!< ARM TrustZone support not available */
#define SPU_CAP_TZM_Enabled (1UL) /*!< ARM TrustZone support is available */

/* Register: SPU_EXTDOMAIN_PERM */
/* Description: Description cluster: Access  for bus access generated from the external domain n List capabilities of the external domain  n */

/* Bit 8 :   */
#define SPU_EXTDOMAIN_PERM_LOCK_Pos (8UL) /*!< Position of LOCK field. */
#define SPU_EXTDOMAIN_PERM_LOCK_Msk (0x1UL << SPU_EXTDOMAIN_PERM_LOCK_Pos) /*!< Bit mask of LOCK field. */
#define SPU_EXTDOMAIN_PERM_LOCK_Unlocked (0UL) /*!< This register can be updated */
#define SPU_EXTDOMAIN_PERM_LOCK_Locked (1UL) /*!< The content of this register can't be changed until the next reset */

/* Bit 4 : Peripheral security mapping */
#define SPU_EXTDOMAIN_PERM_SECATTR_Pos (4UL) /*!< Position of SECATTR field. */
#define SPU_EXTDOMAIN_PERM_SECATTR_Msk (0x1UL << SPU_EXTDOMAIN_PERM_SECATTR_Pos) /*!< Bit mask of SECATTR field. */
#define SPU_EXTDOMAIN_PERM_SECATTR_NonSecure (0UL) /*!< Bus accesses from this domain have the non-secure attribute set */
#define SPU_EXTDOMAIN_PERM_SECATTR_Secure (1UL) /*!< Bus accesses from this domain have secure attribute set */

/* Bits 1..0 : Define configuration capabilities  for TrustZone Cortex-M secure attribute */
#define SPU_EXTDOMAIN_PERM_SECUREMAPPING_Pos (0UL) /*!< Position of SECUREMAPPING field. */
#define SPU_EXTDOMAIN_PERM_SECUREMAPPING_Msk (0x3UL << SPU_EXTDOMAIN_PERM_SECUREMAPPING_Pos) /*!< Bit mask of SECUREMAPPING field. */
#define SPU_EXTDOMAIN_PERM_SECUREMAPPING_NonSecure (0UL) /*!< The bus access from this external domain always have the non-secure attribute set */
#define SPU_EXTDOMAIN_PERM_SECUREMAPPING_Secure (1UL) /*!< The bus access from this external domain always have the secure attribute set */
#define SPU_EXTDOMAIN_PERM_SECUREMAPPING_UserSelectable (2UL) /*!< Non-secure or secure attribute for bus access from this domain is defined by the EXTDOMAIN[n].PERM register */

/* Register: SPU_DPPI_PERM */
/* Description: Description cluster: Select between secure and non-secure attribute  for the DPPI channels. */

/* Bit 15 : Select secure attribute. */
#define SPU_DPPI_PERM_CHANNEL15_Pos (15UL) /*!< Position of CHANNEL15 field. */
#define SPU_DPPI_PERM_CHANNEL15_Msk (0x1UL << SPU_DPPI_PERM_CHANNEL15_Pos) /*!< Bit mask of CHANNEL15 field. */
#define SPU_DPPI_PERM_CHANNEL15_NonSecure (0UL) /*!< Channel15 has its non-secure attribute set */
#define SPU_DPPI_PERM_CHANNEL15_Secure (1UL) /*!< Channel15 has its secure attribute set */

/* Bit 14 : Select secure attribute. */
#define SPU_DPPI_PERM_CHANNEL14_Pos (14UL) /*!< Position of CHANNEL14 field. */
#define SPU_DPPI_PERM_CHANNEL14_Msk (0x1UL << SPU_DPPI_PERM_CHANNEL14_Pos) /*!< Bit mask of CHANNEL14 field. */
#define SPU_DPPI_PERM_CHANNEL14_NonSecure (0UL) /*!< Channel14 has its non-secure attribute set */
#define SPU_DPPI_PERM_CHANNEL14_Secure (1UL) /*!< Channel14 has its secure attribute set */

/* Bit 13 : Select secure attribute. */
#define SPU_DPPI_PERM_CHANNEL13_Pos (13UL) /*!< Position of CHANNEL13 field. */
#define SPU_DPPI_PERM_CHANNEL13_Msk (0x1UL << SPU_DPPI_PERM_CHANNEL13_Pos) /*!< Bit mask of CHANNEL13 field. */
#define SPU_DPPI_PERM_CHANNEL13_NonSecure (0UL) /*!< Channel13 has its non-secure attribute set */
#define SPU_DPPI_PERM_CHANNEL13_Secure (1UL) /*!< Channel13 has its secure attribute set */

/* Bit 12 : Select secure attribute. */
#define SPU_DPPI_PERM_CHANNEL12_Pos (12UL) /*!< Position of CHANNEL12 field. */
#define SPU_DPPI_PERM_CHANNEL12_Msk (0x1UL << SPU_DPPI_PERM_CHANNEL12_Pos) /*!< Bit mask of CHANNEL12 field. */
#define SPU_DPPI_PERM_CHANNEL12_NonSecure (0UL) /*!< Channel12 has its non-secure attribute set */
#define SPU_DPPI_PERM_CHANNEL12_Secure (1UL) /*!< Channel12 has its secure attribute set */

/* Bit 11 : Select secure attribute. */
#define SPU_DPPI_PERM_CHANNEL11_Pos (11UL) /*!< Position of CHANNEL11 field. */
#define SPU_DPPI_PERM_CHANNEL11_Msk (0x1UL << SPU_DPPI_PERM_CHANNEL11_Pos) /*!< Bit mask of CHANNEL11 field. */
#define SPU_DPPI_PERM_CHANNEL11_NonSecure (0UL) /*!< Channel11 has its non-secure attribute set */
#define SPU_DPPI_PERM_CHANNEL11_Secure (1UL) /*!< Channel11 has its secure attribute set */

/* Bit 10 : Select secure attribute. */
#define SPU_DPPI_PERM_CHANNEL10_Pos (10UL) /*!< Position of CHANNEL10 field. */
#define SPU_DPPI_PERM_CHANNEL10_Msk (0x1UL << SPU_DPPI_PERM_CHANNEL10_Pos) /*!< Bit mask of CHANNEL10 field. */
#define SPU_DPPI_PERM_CHANNEL10_NonSecure (0UL) /*!< Channel10 has its non-secure attribute set */
#define SPU_DPPI_PERM_CHANNEL10_Secure (1UL) /*!< Channel10 has its secure attribute set */

/* Bit 9 : Select secure attribute. */
#define SPU_DPPI_PERM_CHANNEL9_Pos (9UL) /*!< Position of CHANNEL9 field. */
#define SPU_DPPI_PERM_CHANNEL9_Msk (0x1UL << SPU_DPPI_PERM_CHANNEL9_Pos) /*!< Bit mask of CHANNEL9 field. */
#define SPU_DPPI_PERM_CHANNEL9_NonSecure (0UL) /*!< Channel9 has its non-secure attribute set */
#define SPU_DPPI_PERM_CHANNEL9_Secure (1UL) /*!< Channel9 has its secure attribute set */

/* Bit 8 : Select secure attribute. */
#define SPU_DPPI_PERM_CHANNEL8_Pos (8UL) /*!< Position of CHANNEL8 field. */
#define SPU_DPPI_PERM_CHANNEL8_Msk (0x1UL << SPU_DPPI_PERM_CHANNEL8_Pos) /*!< Bit mask of CHANNEL8 field. */
#define SPU_DPPI_PERM_CHANNEL8_NonSecure (0UL) /*!< Channel8 has its non-secure attribute set */
#define SPU_DPPI_PERM_CHANNEL8_Secure (1UL) /*!< Channel8 has its secure attribute set */

/* Bit 7 : Select secure attribute. */
#define SPU_DPPI_PERM_CHANNEL7_Pos (7UL) /*!< Position of CHANNEL7 field. */
#define SPU_DPPI_PERM_CHANNEL7_Msk (0x1UL << SPU_DPPI_PERM_CHANNEL7_Pos) /*!< Bit mask of CHANNEL7 field. */
#define SPU_DPPI_PERM_CHANNEL7_NonSecure (0UL) /*!< Channel7 has its non-secure attribute set */
#define SPU_DPPI_PERM_CHANNEL7_Secure (1UL) /*!< Channel7 has its secure attribute set */

/* Bit 6 : Select secure attribute. */
#define SPU_DPPI_PERM_CHANNEL6_Pos (6UL) /*!< Position of CHANNEL6 field. */
#define SPU_DPPI_PERM_CHANNEL6_Msk (0x1UL << SPU_DPPI_PERM_CHANNEL6_Pos) /*!< Bit mask of CHANNEL6 field. */
#define SPU_DPPI_PERM_CHANNEL6_NonSecure (0UL) /*!< Channel6 has its non-secure attribute set */
#define SPU_DPPI_PERM_CHANNEL6_Secure (1UL) /*!< Channel6 has its secure attribute set */

/* Bit 5 : Select secure attribute. */
#define SPU_DPPI_PERM_CHANNEL5_Pos (5UL) /*!< Position of CHANNEL5 field. */
#define SPU_DPPI_PERM_CHANNEL5_Msk (0x1UL << SPU_DPPI_PERM_CHANNEL5_Pos) /*!< Bit mask of CHANNEL5 field. */
#define SPU_DPPI_PERM_CHANNEL5_NonSecure (0UL) /*!< Channel5 has its non-secure attribute set */
#define SPU_DPPI_PERM_CHANNEL5_Secure (1UL) /*!< Channel5 has its secure attribute set */

/* Bit 4 : Select secure attribute. */
#define SPU_DPPI_PERM_CHANNEL4_Pos (4UL) /*!< Position of CHANNEL4 field. */
#define SPU_DPPI_PERM_CHANNEL4_Msk (0x1UL << SPU_DPPI_PERM_CHANNEL4_Pos) /*!< Bit mask of CHANNEL4 field. */
#define SPU_DPPI_PERM_CHANNEL4_NonSecure (0UL) /*!< Channel4 has its non-secure attribute set */
#define SPU_DPPI_PERM_CHANNEL4_Secure (1UL) /*!< Channel4 has its secure attribute set */

/* Bit 3 : Select secure attribute. */
#define SPU_DPPI_PERM_CHANNEL3_Pos (3UL) /*!< Position of CHANNEL3 field. */
#define SPU_DPPI_PERM_CHANNEL3_Msk (0x1UL << SPU_DPPI_PERM_CHANNEL3_Pos) /*!< Bit mask of CHANNEL3 field. */
#define SPU_DPPI_PERM_CHANNEL3_NonSecure (0UL) /*!< Channel3 has its non-secure attribute set */
#define SPU_DPPI_PERM_CHANNEL3_Secure (1UL) /*!< Channel3 has its secure attribute set */

/* Bit 2 : Select secure attribute. */
#define SPU_DPPI_PERM_CHANNEL2_Pos (2UL) /*!< Position of CHANNEL2 field. */
#define SPU_DPPI_PERM_CHANNEL2_Msk (0x1UL << SPU_DPPI_PERM_CHANNEL2_Pos) /*!< Bit mask of CHANNEL2 field. */
#define SPU_DPPI_PERM_CHANNEL2_NonSecure (0UL) /*!< Channel2 has its non-secure attribute set */
#define SPU_DPPI_PERM_CHANNEL2_Secure (1UL) /*!< Channel2 has its secure attribute set */

/* Bit 1 : Select secure attribute. */
#define SPU_DPPI_PERM_CHANNEL1_Pos (1UL) /*!< Position of CHANNEL1 field. */
#define SPU_DPPI_PERM_CHANNEL1_Msk (0x1UL << SPU_DPPI_PERM_CHANNEL1_Pos) /*!< Bit mask of CHANNEL1 field. */
#define SPU_DPPI_PERM_CHANNEL1_NonSecure (0UL) /*!< Channel1 has its non-secure attribute set */
#define SPU_DPPI_PERM_CHANNEL1_Secure (1UL) /*!< Channel1 has its secure attribute set */

/* Bit 0 : Select secure attribute. */
#define SPU_DPPI_PERM_CHANNEL0_Pos (0UL) /*!< Position of CHANNEL0 field. */
#define SPU_DPPI_PERM_CHANNEL0_Msk (0x1UL << SPU_DPPI_PERM_CHANNEL0_Pos) /*!< Bit mask of CHANNEL0 field. */
#define SPU_DPPI_PERM_CHANNEL0_NonSecure (0UL) /*!< Channel0 has its non-secure attribute set */
#define SPU_DPPI_PERM_CHANNEL0_Secure (1UL) /*!< Channel0 has its secure attribute set */

/* Register: SPU_DPPI_LOCK */
/* Description: Description cluster: Prevent further modification of the corresponding PERM register */

/* Bit 0 :   */
#define SPU_DPPI_LOCK_LOCK_Pos (0UL) /*!< Position of LOCK field. */
#define SPU_DPPI_LOCK_LOCK_Msk (0x1UL << SPU_DPPI_LOCK_LOCK_Pos) /*!< Bit mask of LOCK field. */
#define SPU_DPPI_LOCK_LOCK_Unlocked (0UL) /*!< DPPI[n].PERM register content can be changed */
#define SPU_DPPI_LOCK_LOCK_Locked (1UL) /*!< DPPI[n].PERM register can't be changed until next reset */

/* Register: SPU_GPIOPORT_PERM */
/* Description: Description cluster: Select between secure and non-secure attribute  for pins 0 to 31  of port n. */

/* Bit 31 : Select secure attribute attribute for PIN 31. */
#define SPU_GPIOPORT_PERM_PIN31_Pos (31UL) /*!< Position of PIN31 field. */
#define SPU_GPIOPORT_PERM_PIN31_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN31_Pos) /*!< Bit mask of PIN31 field. */
#define SPU_GPIOPORT_PERM_PIN31_NonSecure (0UL) /*!< Pin 31 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN31_Secure (1UL) /*!< Pin 31 has its secure attribute set */

/* Bit 30 : Select secure attribute attribute for PIN 30. */
#define SPU_GPIOPORT_PERM_PIN30_Pos (30UL) /*!< Position of PIN30 field. */
#define SPU_GPIOPORT_PERM_PIN30_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN30_Pos) /*!< Bit mask of PIN30 field. */
#define SPU_GPIOPORT_PERM_PIN30_NonSecure (0UL) /*!< Pin 30 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN30_Secure (1UL) /*!< Pin 30 has its secure attribute set */

/* Bit 29 : Select secure attribute attribute for PIN 29. */
#define SPU_GPIOPORT_PERM_PIN29_Pos (29UL) /*!< Position of PIN29 field. */
#define SPU_GPIOPORT_PERM_PIN29_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN29_Pos) /*!< Bit mask of PIN29 field. */
#define SPU_GPIOPORT_PERM_PIN29_NonSecure (0UL) /*!< Pin 29 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN29_Secure (1UL) /*!< Pin 29 has its secure attribute set */

/* Bit 28 : Select secure attribute attribute for PIN 28. */
#define SPU_GPIOPORT_PERM_PIN28_Pos (28UL) /*!< Position of PIN28 field. */
#define SPU_GPIOPORT_PERM_PIN28_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN28_Pos) /*!< Bit mask of PIN28 field. */
#define SPU_GPIOPORT_PERM_PIN28_NonSecure (0UL) /*!< Pin 28 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN28_Secure (1UL) /*!< Pin 28 has its secure attribute set */

/* Bit 27 : Select secure attribute attribute for PIN 27. */
#define SPU_GPIOPORT_PERM_PIN27_Pos (27UL) /*!< Position of PIN27 field. */
#define SPU_GPIOPORT_PERM_PIN27_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN27_Pos) /*!< Bit mask of PIN27 field. */
#define SPU_GPIOPORT_PERM_PIN27_NonSecure (0UL) /*!< Pin 27 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN27_Secure (1UL) /*!< Pin 27 has its secure attribute set */

/* Bit 26 : Select secure attribute attribute for PIN 26. */
#define SPU_GPIOPORT_PERM_PIN26_Pos (26UL) /*!< Position of PIN26 field. */
#define SPU_GPIOPORT_PERM_PIN26_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN26_Pos) /*!< Bit mask of PIN26 field. */
#define SPU_GPIOPORT_PERM_PIN26_NonSecure (0UL) /*!< Pin 26 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN26_Secure (1UL) /*!< Pin 26 has its secure attribute set */

/* Bit 25 : Select secure attribute attribute for PIN 25. */
#define SPU_GPIOPORT_PERM_PIN25_Pos (25UL) /*!< Position of PIN25 field. */
#define SPU_GPIOPORT_PERM_PIN25_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN25_Pos) /*!< Bit mask of PIN25 field. */
#define SPU_GPIOPORT_PERM_PIN25_NonSecure (0UL) /*!< Pin 25 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN25_Secure (1UL) /*!< Pin 25 has its secure attribute set */

/* Bit 24 : Select secure attribute attribute for PIN 24. */
#define SPU_GPIOPORT_PERM_PIN24_Pos (24UL) /*!< Position of PIN24 field. */
#define SPU_GPIOPORT_PERM_PIN24_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN24_Pos) /*!< Bit mask of PIN24 field. */
#define SPU_GPIOPORT_PERM_PIN24_NonSecure (0UL) /*!< Pin 24 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN24_Secure (1UL) /*!< Pin 24 has its secure attribute set */

/* Bit 23 : Select secure attribute attribute for PIN 23. */
#define SPU_GPIOPORT_PERM_PIN23_Pos (23UL) /*!< Position of PIN23 field. */
#define SPU_GPIOPORT_PERM_PIN23_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN23_Pos) /*!< Bit mask of PIN23 field. */
#define SPU_GPIOPORT_PERM_PIN23_NonSecure (0UL) /*!< Pin 23 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN23_Secure (1UL) /*!< Pin 23 has its secure attribute set */

/* Bit 22 : Select secure attribute attribute for PIN 22. */
#define SPU_GPIOPORT_PERM_PIN22_Pos (22UL) /*!< Position of PIN22 field. */
#define SPU_GPIOPORT_PERM_PIN22_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN22_Pos) /*!< Bit mask of PIN22 field. */
#define SPU_GPIOPORT_PERM_PIN22_NonSecure (0UL) /*!< Pin 22 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN22_Secure (1UL) /*!< Pin 22 has its secure attribute set */

/* Bit 21 : Select secure attribute attribute for PIN 21. */
#define SPU_GPIOPORT_PERM_PIN21_Pos (21UL) /*!< Position of PIN21 field. */
#define SPU_GPIOPORT_PERM_PIN21_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN21_Pos) /*!< Bit mask of PIN21 field. */
#define SPU_GPIOPORT_PERM_PIN21_NonSecure (0UL) /*!< Pin 21 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN21_Secure (1UL) /*!< Pin 21 has its secure attribute set */

/* Bit 20 : Select secure attribute attribute for PIN 20. */
#define SPU_GPIOPORT_PERM_PIN20_Pos (20UL) /*!< Position of PIN20 field. */
#define SPU_GPIOPORT_PERM_PIN20_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN20_Pos) /*!< Bit mask of PIN20 field. */
#define SPU_GPIOPORT_PERM_PIN20_NonSecure (0UL) /*!< Pin 20 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN20_Secure (1UL) /*!< Pin 20 has its secure attribute set */

/* Bit 19 : Select secure attribute attribute for PIN 19. */
#define SPU_GPIOPORT_PERM_PIN19_Pos (19UL) /*!< Position of PIN19 field. */
#define SPU_GPIOPORT_PERM_PIN19_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN19_Pos) /*!< Bit mask of PIN19 field. */
#define SPU_GPIOPORT_PERM_PIN19_NonSecure (0UL) /*!< Pin 19 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN19_Secure (1UL) /*!< Pin 19 has its secure attribute set */

/* Bit 18 : Select secure attribute attribute for PIN 18. */
#define SPU_GPIOPORT_PERM_PIN18_Pos (18UL) /*!< Position of PIN18 field. */
#define SPU_GPIOPORT_PERM_PIN18_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN18_Pos) /*!< Bit mask of PIN18 field. */
#define SPU_GPIOPORT_PERM_PIN18_NonSecure (0UL) /*!< Pin 18 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN18_Secure (1UL) /*!< Pin 18 has its secure attribute set */

/* Bit 17 : Select secure attribute attribute for PIN 17. */
#define SPU_GPIOPORT_PERM_PIN17_Pos (17UL) /*!< Position of PIN17 field. */
#define SPU_GPIOPORT_PERM_PIN17_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN17_Pos) /*!< Bit mask of PIN17 field. */
#define SPU_GPIOPORT_PERM_PIN17_NonSecure (0UL) /*!< Pin 17 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN17_Secure (1UL) /*!< Pin 17 has its secure attribute set */

/* Bit 16 : Select secure attribute attribute for PIN 16. */
#define SPU_GPIOPORT_PERM_PIN16_Pos (16UL) /*!< Position of PIN16 field. */
#define SPU_GPIOPORT_PERM_PIN16_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN16_Pos) /*!< Bit mask of PIN16 field. */
#define SPU_GPIOPORT_PERM_PIN16_NonSecure (0UL) /*!< Pin 16 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN16_Secure (1UL) /*!< Pin 16 has its secure attribute set */

/* Bit 15 : Select secure attribute attribute for PIN 15. */
#define SPU_GPIOPORT_PERM_PIN15_Pos (15UL) /*!< Position of PIN15 field. */
#define SPU_GPIOPORT_PERM_PIN15_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN15_Pos) /*!< Bit mask of PIN15 field. */
#define SPU_GPIOPORT_PERM_PIN15_NonSecure (0UL) /*!< Pin 15 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN15_Secure (1UL) /*!< Pin 15 has its secure attribute set */

/* Bit 14 : Select secure attribute attribute for PIN 14. */
#define SPU_GPIOPORT_PERM_PIN14_Pos (14UL) /*!< Position of PIN14 field. */
#define SPU_GPIOPORT_PERM_PIN14_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN14_Pos) /*!< Bit mask of PIN14 field. */
#define SPU_GPIOPORT_PERM_PIN14_NonSecure (0UL) /*!< Pin 14 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN14_Secure (1UL) /*!< Pin 14 has its secure attribute set */

/* Bit 13 : Select secure attribute attribute for PIN 13. */
#define SPU_GPIOPORT_PERM_PIN13_Pos (13UL) /*!< Position of PIN13 field. */
#define SPU_GPIOPORT_PERM_PIN13_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN13_Pos) /*!< Bit mask of PIN13 field. */
#define SPU_GPIOPORT_PERM_PIN13_NonSecure (0UL) /*!< Pin 13 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN13_Secure (1UL) /*!< Pin 13 has its secure attribute set */

/* Bit 12 : Select secure attribute attribute for PIN 12. */
#define SPU_GPIOPORT_PERM_PIN12_Pos (12UL) /*!< Position of PIN12 field. */
#define SPU_GPIOPORT_PERM_PIN12_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN12_Pos) /*!< Bit mask of PIN12 field. */
#define SPU_GPIOPORT_PERM_PIN12_NonSecure (0UL) /*!< Pin 12 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN12_Secure (1UL) /*!< Pin 12 has its secure attribute set */

/* Bit 11 : Select secure attribute attribute for PIN 11. */
#define SPU_GPIOPORT_PERM_PIN11_Pos (11UL) /*!< Position of PIN11 field. */
#define SPU_GPIOPORT_PERM_PIN11_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN11_Pos) /*!< Bit mask of PIN11 field. */
#define SPU_GPIOPORT_PERM_PIN11_NonSecure (0UL) /*!< Pin 11 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN11_Secure (1UL) /*!< Pin 11 has its secure attribute set */

/* Bit 10 : Select secure attribute attribute for PIN 10. */
#define SPU_GPIOPORT_PERM_PIN10_Pos (10UL) /*!< Position of PIN10 field. */
#define SPU_GPIOPORT_PERM_PIN10_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN10_Pos) /*!< Bit mask of PIN10 field. */
#define SPU_GPIOPORT_PERM_PIN10_NonSecure (0UL) /*!< Pin 10 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN10_Secure (1UL) /*!< Pin 10 has its secure attribute set */

/* Bit 9 : Select secure attribute attribute for PIN 9. */
#define SPU_GPIOPORT_PERM_PIN9_Pos (9UL) /*!< Position of PIN9 field. */
#define SPU_GPIOPORT_PERM_PIN9_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN9_Pos) /*!< Bit mask of PIN9 field. */
#define SPU_GPIOPORT_PERM_PIN9_NonSecure (0UL) /*!< Pin 9 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN9_Secure (1UL) /*!< Pin 9 has its secure attribute set */

/* Bit 8 : Select secure attribute attribute for PIN 8. */
#define SPU_GPIOPORT_PERM_PIN8_Pos (8UL) /*!< Position of PIN8 field. */
#define SPU_GPIOPORT_PERM_PIN8_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN8_Pos) /*!< Bit mask of PIN8 field. */
#define SPU_GPIOPORT_PERM_PIN8_NonSecure (0UL) /*!< Pin 8 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN8_Secure (1UL) /*!< Pin 8 has its secure attribute set */

/* Bit 7 : Select secure attribute attribute for PIN 7. */
#define SPU_GPIOPORT_PERM_PIN7_Pos (7UL) /*!< Position of PIN7 field. */
#define SPU_GPIOPORT_PERM_PIN7_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN7_Pos) /*!< Bit mask of PIN7 field. */
#define SPU_GPIOPORT_PERM_PIN7_NonSecure (0UL) /*!< Pin 7 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN7_Secure (1UL) /*!< Pin 7 has its secure attribute set */

/* Bit 6 : Select secure attribute attribute for PIN 6. */
#define SPU_GPIOPORT_PERM_PIN6_Pos (6UL) /*!< Position of PIN6 field. */
#define SPU_GPIOPORT_PERM_PIN6_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN6_Pos) /*!< Bit mask of PIN6 field. */
#define SPU_GPIOPORT_PERM_PIN6_NonSecure (0UL) /*!< Pin 6 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN6_Secure (1UL) /*!< Pin 6 has its secure attribute set */

/* Bit 5 : Select secure attribute attribute for PIN 5. */
#define SPU_GPIOPORT_PERM_PIN5_Pos (5UL) /*!< Position of PIN5 field. */
#define SPU_GPIOPORT_PERM_PIN5_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN5_Pos) /*!< Bit mask of PIN5 field. */
#define SPU_GPIOPORT_PERM_PIN5_NonSecure (0UL) /*!< Pin 5 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN5_Secure (1UL) /*!< Pin 5 has its secure attribute set */

/* Bit 4 : Select secure attribute attribute for PIN 4. */
#define SPU_GPIOPORT_PERM_PIN4_Pos (4UL) /*!< Position of PIN4 field. */
#define SPU_GPIOPORT_PERM_PIN4_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN4_Pos) /*!< Bit mask of PIN4 field. */
#define SPU_GPIOPORT_PERM_PIN4_NonSecure (0UL) /*!< Pin 4 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN4_Secure (1UL) /*!< Pin 4 has its secure attribute set */

/* Bit 3 : Select secure attribute attribute for PIN 3. */
#define SPU_GPIOPORT_PERM_PIN3_Pos (3UL) /*!< Position of PIN3 field. */
#define SPU_GPIOPORT_PERM_PIN3_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN3_Pos) /*!< Bit mask of PIN3 field. */
#define SPU_GPIOPORT_PERM_PIN3_NonSecure (0UL) /*!< Pin 3 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN3_Secure (1UL) /*!< Pin 3 has its secure attribute set */

/* Bit 2 : Select secure attribute attribute for PIN 2. */
#define SPU_GPIOPORT_PERM_PIN2_Pos (2UL) /*!< Position of PIN2 field. */
#define SPU_GPIOPORT_PERM_PIN2_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN2_Pos) /*!< Bit mask of PIN2 field. */
#define SPU_GPIOPORT_PERM_PIN2_NonSecure (0UL) /*!< Pin 2 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN2_Secure (1UL) /*!< Pin 2 has its secure attribute set */

/* Bit 1 : Select secure attribute attribute for PIN 1. */
#define SPU_GPIOPORT_PERM_PIN1_Pos (1UL) /*!< Position of PIN1 field. */
#define SPU_GPIOPORT_PERM_PIN1_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN1_Pos) /*!< Bit mask of PIN1 field. */
#define SPU_GPIOPORT_PERM_PIN1_NonSecure (0UL) /*!< Pin 1 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN1_Secure (1UL) /*!< Pin 1 has its secure attribute set */

/* Bit 0 : Select secure attribute attribute for PIN 0. */
#define SPU_GPIOPORT_PERM_PIN0_Pos (0UL) /*!< Position of PIN0 field. */
#define SPU_GPIOPORT_PERM_PIN0_Msk (0x1UL << SPU_GPIOPORT_PERM_PIN0_Pos) /*!< Bit mask of PIN0 field. */
#define SPU_GPIOPORT_PERM_PIN0_NonSecure (0UL) /*!< Pin 0 has its non-secure attribute set */
#define SPU_GPIOPORT_PERM_PIN0_Secure (1UL) /*!< Pin 0 has its secure attribute set */

/* Register: SPU_GPIOPORT_LOCK */
/* Description: Description cluster: Prevent further modification of the corresponding PERM register */

/* Bit 0 :   */
#define SPU_GPIOPORT_LOCK_LOCK_Pos (0UL) /*!< Position of LOCK field. */
#define SPU_GPIOPORT_LOCK_LOCK_Msk (0x1UL << SPU_GPIOPORT_LOCK_LOCK_Pos) /*!< Bit mask of LOCK field. */
#define SPU_GPIOPORT_LOCK_LOCK_Unlocked (0UL) /*!< GPIOPORT[n].PERM register content can be changed */
#define SPU_GPIOPORT_LOCK_LOCK_Locked (1UL) /*!< GPIOPORT[n].PERM register can't be changed until next reset */

/* Register: SPU_FLASHNSC_REGION */
/* Description: Description cluster: Define which flash region can contain the non-secure callable (NSC) region n */

/* Bit 8 :   */
#define SPU_FLASHNSC_REGION_LOCK_Pos (8UL) /*!< Position of LOCK field. */
#define SPU_FLASHNSC_REGION_LOCK_Msk (0x1UL << SPU_FLASHNSC_REGION_LOCK_Pos) /*!< Bit mask of LOCK field. */
#define SPU_FLASHNSC_REGION_LOCK_Unlocked (0UL) /*!< This register can be updated */
#define SPU_FLASHNSC_REGION_LOCK_Locked (1UL) /*!< The content of this register can't be changed until the next reset */

/* Bits 4..0 : Region number */
#define SPU_FLASHNSC_REGION_REGION_Pos (0UL) /*!< Position of REGION field. */
#define SPU_FLASHNSC_REGION_REGION_Msk (0x1FUL << SPU_FLASHNSC_REGION_REGION_Pos) /*!< Bit mask of REGION field. */

/* Register: SPU_FLASHNSC_SIZE */
/* Description: Description cluster: Define the size of the non-secure callable (NSC) region n */

/* Bit 8 :   */
#define SPU_FLASHNSC_SIZE_LOCK_Pos (8UL) /*!< Position of LOCK field. */
#define SPU_FLASHNSC_SIZE_LOCK_Msk (0x1UL << SPU_FLASHNSC_SIZE_LOCK_Pos) /*!< Bit mask of LOCK field. */
#define SPU_FLASHNSC_SIZE_LOCK_Unlocked (0UL) /*!< This register can be updated */
#define SPU_FLASHNSC_SIZE_LOCK_Locked (1UL) /*!< The content of this register can't be changed until the next reset */

/* Bits 3..0 : Size of the non-secure callable (NSC) region n */
#define SPU_FLASHNSC_SIZE_SIZE_Pos (0UL) /*!< Position of SIZE field. */
#define SPU_FLASHNSC_SIZE_SIZE_Msk (0xFUL << SPU_FLASHNSC_SIZE_SIZE_Pos) /*!< Bit mask of SIZE field. */
#define SPU_FLASHNSC_SIZE_SIZE_Disabled (0UL) /*!< The region n is not defined as a non-secure callable region. Normal security attributes (secure or non-secure) are enforced. */
#define SPU_FLASHNSC_SIZE_SIZE_32 (1UL) /*!< The region n is defined as non-secure callable with a 32-byte size */
#define SPU_FLASHNSC_SIZE_SIZE_64 (2UL) /*!< The region n is defined as non-secure callable with a 64-byte size */
#define SPU_FLASHNSC_SIZE_SIZE_128 (3UL) /*!< The region n is defined as non-secure callable with a 128-byte size */
#define SPU_FLASHNSC_SIZE_SIZE_256 (4UL) /*!< The region n is defined as non-secure callable with a 256-byte size */
#define SPU_FLASHNSC_SIZE_SIZE_512 (5UL) /*!< The region n is defined as non-secure callable with a 512-byte size */
#define SPU_FLASHNSC_SIZE_SIZE_1024 (6UL) /*!< The region n is defined as non-secure callable with a 1024-byte size */
#define SPU_FLASHNSC_SIZE_SIZE_2048 (7UL) /*!< The region n is defined as non-secure callable with a 2048-byte size */
#define SPU_FLASHNSC_SIZE_SIZE_4096 (8UL) /*!< The region n is defined as non-secure callable with a 4096-byte size */

/* Register: SPU_RAMNSC_REGION */
/* Description: Description cluster: Define which RAM region can contain the non-secure callable (NSC) region n */

/* Bit 8 :   */
#define SPU_RAMNSC_REGION_LOCK_Pos (8UL) /*!< Position of LOCK field. */
#define SPU_RAMNSC_REGION_LOCK_Msk (0x1UL << SPU_RAMNSC_REGION_LOCK_Pos) /*!< Bit mask of LOCK field. */
#define SPU_RAMNSC_REGION_LOCK_Unlocked (0UL) /*!< This register can be updated */
#define SPU_RAMNSC_REGION_LOCK_Locked (1UL) /*!< The content of this register can't be changed until the next reset */

/* Bits 3..0 : Region number */
#define SPU_RAMNSC_REGION_REGION_Pos (0UL) /*!< Position of REGION field. */
#define SPU_RAMNSC_REGION_REGION_Msk (0xFUL << SPU_RAMNSC_REGION_REGION_Pos) /*!< Bit mask of REGION field. */

/* Register: SPU_RAMNSC_SIZE */
/* Description: Description cluster: Define the size of the non-secure callable (NSC) region n */

/* Bit 8 :   */
#define SPU_RAMNSC_SIZE_LOCK_Pos (8UL) /*!< Position of LOCK field. */
#define SPU_RAMNSC_SIZE_LOCK_Msk (0x1UL << SPU_RAMNSC_SIZE_LOCK_Pos) /*!< Bit mask of LOCK field. */
#define SPU_RAMNSC_SIZE_LOCK_Unlocked (0UL) /*!< This register can be updated */
#define SPU_RAMNSC_SIZE_LOCK_Locked (1UL) /*!< The content of this register can't be changed until the next reset */

/* Bits 3..0 : Size of the non-secure callable (NSC) region n */
#define SPU_RAMNSC_SIZE_SIZE_Pos (0UL) /*!< Position of SIZE field. */
#define SPU_RAMNSC_SIZE_SIZE_Msk (0xFUL << SPU_RAMNSC_SIZE_SIZE_Pos) /*!< Bit mask of SIZE field. */
#define SPU_RAMNSC_SIZE_SIZE_Disabled (0UL) /*!< The region n is not defined as a non-secure callable region. Normal security attributes (secure or non-secure) are enforced. */
#define SPU_RAMNSC_SIZE_SIZE_32 (1UL) /*!< The region n is defined as non-secure callable with a 32-byte size */
#define SPU_RAMNSC_SIZE_SIZE_64 (2UL) /*!< The region n is defined as non-secure callable with a 64-byte size */
#define SPU_RAMNSC_SIZE_SIZE_128 (3UL) /*!< The region n is defined as non-secure callable with a 128-byte size */
#define SPU_RAMNSC_SIZE_SIZE_256 (4UL) /*!< The region n is defined as non-secure callable with a 256-byte size */
#define SPU_RAMNSC_SIZE_SIZE_512 (5UL) /*!< The region n is defined as non-secure callable with a 512-byte size */
#define SPU_RAMNSC_SIZE_SIZE_1024 (6UL) /*!< The region n is defined as non-secure callable with a 1024-byte size */
#define SPU_RAMNSC_SIZE_SIZE_2048 (7UL) /*!< The region n is defined as non-secure callable with a 2048-byte size */
#define SPU_RAMNSC_SIZE_SIZE_4096 (8UL) /*!< The region n is defined as non-secure callable with a 4096-byte size */

/* Register: SPU_FLASHREGION_PERM */
/* Description: Description cluster: Access permissions for flash region n */

/* Bit 8 :   */
#define SPU_FLASHREGION_PERM_LOCK_Pos (8UL) /*!< Position of LOCK field. */
#define SPU_FLASHREGION_PERM_LOCK_Msk (0x1UL << SPU_FLASHREGION_PERM_LOCK_Pos) /*!< Bit mask of LOCK field. */
#define SPU_FLASHREGION_PERM_LOCK_Unlocked (0UL) /*!< This register can be updated */
#define SPU_FLASHREGION_PERM_LOCK_Locked (1UL) /*!< The content of this register can't be changed until the next reset */

/* Bit 4 : Security attribute for flash region n */
#define SPU_FLASHREGION_PERM_SECATTR_Pos (4UL) /*!< Position of SECATTR field. */
#define SPU_FLASHREGION_PERM_SECATTR_Msk (0x1UL << SPU_FLASHREGION_PERM_SECATTR_Pos) /*!< Bit mask of SECATTR field. */
#define SPU_FLASHREGION_PERM_SECATTR_Non_Secure (0UL) /*!< Flash region n security attribute is non-secure */
#define SPU_FLASHREGION_PERM_SECATTR_Secure (1UL) /*!< Flash region n security attribute is secure */

/* Bit 2 : Configure read permissions for flash region n */
#define SPU_FLASHREGION_PERM_READ_Pos (2UL) /*!< Position of READ field. */
#define SPU_FLASHREGION_PERM_READ_Msk (0x1UL << SPU_FLASHREGION_PERM_READ_Pos) /*!< Bit mask of READ field. */
#define SPU_FLASHREGION_PERM_READ_Disable (0UL) /*!< Block read operation from flash region n */
#define SPU_FLASHREGION_PERM_READ_Enable (1UL) /*!< Allow read operation from flash region n */

/* Bit 1 : Configure write permission for flash region n */
#define SPU_FLASHREGION_PERM_WRITE_Pos (1UL) /*!< Position of WRITE field. */
#define SPU_FLASHREGION_PERM_WRITE_Msk (0x1UL << SPU_FLASHREGION_PERM_WRITE_Pos) /*!< Bit mask of WRITE field. */
#define SPU_FLASHREGION_PERM_WRITE_Disable (0UL) /*!< Block write operation to region n */
#define SPU_FLASHREGION_PERM_WRITE_Enable (1UL) /*!< Allow write operation to region n */

/* Bit 0 : Configure instruction fetch permissions from flash region n */
#define SPU_FLASHREGION_PERM_EXECUTE_Pos (0UL) /*!< Position of EXECUTE field. */
#define SPU_FLASHREGION_PERM_EXECUTE_Msk (0x1UL << SPU_FLASHREGION_PERM_EXECUTE_Pos) /*!< Bit mask of EXECUTE field. */
#define SPU_FLASHREGION_PERM_EXECUTE_Disable (0UL) /*!< Block instruction fetches from flash region n */
#define SPU_FLASHREGION_PERM_EXECUTE_Enable (1UL) /*!< Allow instruction fetches from flash region n */

/* Register: SPU_RAMREGION_PERM */
/* Description: Description cluster: Access permissions for RAM region n */

/* Bit 8 :   */
#define SPU_RAMREGION_PERM_LOCK_Pos (8UL) /*!< Position of LOCK field. */
#define SPU_RAMREGION_PERM_LOCK_Msk (0x1UL << SPU_RAMREGION_PERM_LOCK_Pos) /*!< Bit mask of LOCK field. */
#define SPU_RAMREGION_PERM_LOCK_Unlocked (0UL) /*!< This register can be updated */
#define SPU_RAMREGION_PERM_LOCK_Locked (1UL) /*!< The content of this register can't be changed until the next reset */

/* Bit 4 : Security attribute for RAM region n */
#define SPU_RAMREGION_PERM_SECATTR_Pos (4UL) /*!< Position of SECATTR field. */
#define SPU_RAMREGION_PERM_SECATTR_Msk (0x1UL << SPU_RAMREGION_PERM_SECATTR_Pos) /*!< Bit mask of SECATTR field. */
#define SPU_RAMREGION_PERM_SECATTR_Non_Secure (0UL) /*!< RAM region n security attribute is non-secure */
#define SPU_RAMREGION_PERM_SECATTR_Secure (1UL) /*!< RAM region n security attribute is secure */

/* Bit 2 : Configure read permissions for RAM region n */
#define SPU_RAMREGION_PERM_READ_Pos (2UL) /*!< Position of READ field. */
#define SPU_RAMREGION_PERM_READ_Msk (0x1UL << SPU_RAMREGION_PERM_READ_Pos) /*!< Bit mask of READ field. */
#define SPU_RAMREGION_PERM_READ_Disable (0UL) /*!< Block read operation from RAM region n */
#define SPU_RAMREGION_PERM_READ_Enable (1UL) /*!< Allow read operation from RAM region n */

/* Bit 1 : Configure write permission for RAM region n */
#define SPU_RAMREGION_PERM_WRITE_Pos (1UL) /*!< Position of WRITE field. */
#define SPU_RAMREGION_PERM_WRITE_Msk (0x1UL << SPU_RAMREGION_PERM_WRITE_Pos) /*!< Bit mask of WRITE field. */
#define SPU_RAMREGION_PERM_WRITE_Disable (0UL) /*!< Block write operation to RAM region n */
#define SPU_RAMREGION_PERM_WRITE_Enable (1UL) /*!< Allow write operation to RAM region n */

/* Bit 0 : Configure instruction fetch permissions from RAM region n */
#define SPU_RAMREGION_PERM_EXECUTE_Pos (0UL) /*!< Position of EXECUTE field. */
#define SPU_RAMREGION_PERM_EXECUTE_Msk (0x1UL << SPU_RAMREGION_PERM_EXECUTE_Pos) /*!< Bit mask of EXECUTE field. */
#define SPU_RAMREGION_PERM_EXECUTE_Disable (0UL) /*!< Block instruction fetches from RAM region n */
#define SPU_RAMREGION_PERM_EXECUTE_Enable (1UL) /*!< Allow instruction fetches from RAM region n */

/* Register: SPU_PERIPHID_PERM */
/* Description: Description cluster: List capabilities and access permissions for the peripheral with ID n */

/* Bit 31 : Indicate if a peripheral is present with ID n */
#define SPU_PERIPHID_PERM_PRESENT_Pos (31UL) /*!< Position of PRESENT field. */
#define SPU_PERIPHID_PERM_PRESENT_Msk (0x1UL << SPU_PERIPHID_PERM_PRESENT_Pos) /*!< Bit mask of PRESENT field. */
#define SPU_PERIPHID_PERM_PRESENT_NotPresent (0UL) /*!< Peripheral is not present */
#define SPU_PERIPHID_PERM_PRESENT_IsPresent (1UL) /*!< Peripheral is present */

/* Bit 8 :   */
#define SPU_PERIPHID_PERM_LOCK_Pos (8UL) /*!< Position of LOCK field. */
#define SPU_PERIPHID_PERM_LOCK_Msk (0x1UL << SPU_PERIPHID_PERM_LOCK_Pos) /*!< Bit mask of LOCK field. */
#define SPU_PERIPHID_PERM_LOCK_Unlocked (0UL) /*!< This register can be updated */
#define SPU_PERIPHID_PERM_LOCK_Locked (1UL) /*!< The content of this register can't be changed until the next reset */

/* Bit 5 : Security attribution for the DMA transfer */
#define SPU_PERIPHID_PERM_DMASEC_Pos (5UL) /*!< Position of DMASEC field. */
#define SPU_PERIPHID_PERM_DMASEC_Msk (0x1UL << SPU_PERIPHID_PERM_DMASEC_Pos) /*!< Bit mask of DMASEC field. */
#define SPU_PERIPHID_PERM_DMASEC_NonSecure (0UL) /*!< DMA transfers initiated by this peripheral have the non-secure attribute set */
#define SPU_PERIPHID_PERM_DMASEC_Secure (1UL) /*!< DMA transfers initiated by this peripheral have the secure attribute set */

/* Bit 4 : Peripheral security mapping */
#define SPU_PERIPHID_PERM_SECATTR_Pos (4UL) /*!< Position of SECATTR field. */
#define SPU_PERIPHID_PERM_SECATTR_Msk (0x1UL << SPU_PERIPHID_PERM_SECATTR_Pos) /*!< Bit mask of SECATTR field. */
#define SPU_PERIPHID_PERM_SECATTR_NonSecure (0UL) /*!< If SECUREMAPPING == UserSelectable: Peripheral is mapped in non-secure peripheral address space. If SECUREMAPPING == Split: Peripheral is mapped in non-secure and secure peripheral address space. */
#define SPU_PERIPHID_PERM_SECATTR_Secure (1UL) /*!< Peripheral is mapped in secure peripheral address space */

/* Bits 3..2 : Indicate if the peripheral has DMA capabilities and if DMA transfer can be assigned to a different security attribute than the peripheral itself */
#define SPU_PERIPHID_PERM_DMA_Pos (2UL) /*!< Position of DMA field. */
#define SPU_PERIPHID_PERM_DMA_Msk (0x3UL << SPU_PERIPHID_PERM_DMA_Pos) /*!< Bit mask of DMA field. */
#define SPU_PERIPHID_PERM_DMA_NoDMA (0UL) /*!< Peripheral has no DMA capability */
#define SPU_PERIPHID_PERM_DMA_NoSeparateAttribute (1UL) /*!< Peripheral has DMA and DMA transfers always have the same security attribute as assigned to the peripheral */
#define SPU_PERIPHID_PERM_DMA_SeparateAttribute (2UL) /*!< Peripheral has DMA and DMA transfers can have a different security attribute than the one assigned to the peripheral */

/* Bits 1..0 : Define configuration capabilities for TrustZone Cortex-M secure attribute */
#define SPU_PERIPHID_PERM_SECUREMAPPING_Pos (0UL) /*!< Position of SECUREMAPPING field. */
#define SPU_PERIPHID_PERM_SECUREMAPPING_Msk (0x3UL << SPU_PERIPHID_PERM_SECUREMAPPING_Pos) /*!< Bit mask of SECUREMAPPING field. */
#define SPU_PERIPHID_PERM_SECUREMAPPING_NonSecure (0UL) /*!< This peripheral is always accessible as a non-secure peripheral */
#define SPU_PERIPHID_PERM_SECUREMAPPING_Secure (1UL) /*!< This peripheral is always accessible as a secure peripheral */
#define SPU_PERIPHID_PERM_SECUREMAPPING_UserSelectable (2UL) /*!< Non-secure or secure attribute for this peripheral is defined by the PERIPHID[n].PERM register */
#define SPU_PERIPHID_PERM_SECUREMAPPING_Split (3UL) /*!< This peripheral implements the split security mechanism. Non-secure or secure attribute for this peripheral is defined by the PERIPHID[n].PERM register. */


/* Peripheral: TAD */
/* Description: Trace and debug control */

/* Register: TAD_ENABLE */
/* Description: Enable debug domain and aquire selected GPIOs */

/* Bit 0 :   */
#define TAD_ENABLE_ENABLE_Pos (0UL) /*!< Position of ENABLE field. */
#define TAD_ENABLE_ENABLE_Msk (0x1UL << TAD_ENABLE_ENABLE_Pos) /*!< Bit mask of ENABLE field. */
#define TAD_ENABLE_ENABLE_DISABLED (0UL) /*!< Disable debug domain and release selected GPIOs */
#define TAD_ENABLE_ENABLE_ENABLED (1UL) /*!< Enable debug domain and aquire selected GPIOs */

/* Register: TAD_PSEL_TRACECLK */
/* Description: Pin number configuration for TRACECLK */

/* Bit 31 : Connection */
#define TAD_PSEL_TRACECLK_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define TAD_PSEL_TRACECLK_CONNECT_Msk (0x1UL << TAD_PSEL_TRACECLK_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define TAD_PSEL_TRACECLK_CONNECT_Connected (0UL) /*!< Connect */
#define TAD_PSEL_TRACECLK_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define TAD_PSEL_TRACECLK_PIN_Pos (0UL) /*!< Position of PIN field. */
#define TAD_PSEL_TRACECLK_PIN_Msk (0x1FUL << TAD_PSEL_TRACECLK_PIN_Pos) /*!< Bit mask of PIN field. */

/* Register: TAD_PSEL_TRACEDATA0 */
/* Description: Pin number configuration for TRACEDATA[0] */

/* Bit 31 : Connection */
#define TAD_PSEL_TRACEDATA0_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define TAD_PSEL_TRACEDATA0_CONNECT_Msk (0x1UL << TAD_PSEL_TRACEDATA0_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define TAD_PSEL_TRACEDATA0_CONNECT_Connected (0UL) /*!< Connect */
#define TAD_PSEL_TRACEDATA0_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define TAD_PSEL_TRACEDATA0_PIN_Pos (0UL) /*!< Position of PIN field. */
#define TAD_PSEL_TRACEDATA0_PIN_Msk (0x1FUL << TAD_PSEL_TRACEDATA0_PIN_Pos) /*!< Bit mask of PIN field. */

/* Register: TAD_PSEL_TRACEDATA1 */
/* Description: Pin number configuration for TRACEDATA[1] */

/* Bit 31 : Connection */
#define TAD_PSEL_TRACEDATA1_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define TAD_PSEL_TRACEDATA1_CONNECT_Msk (0x1UL << TAD_PSEL_TRACEDATA1_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define TAD_PSEL_TRACEDATA1_CONNECT_Connected (0UL) /*!< Connect */
#define TAD_PSEL_TRACEDATA1_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define TAD_PSEL_TRACEDATA1_PIN_Pos (0UL) /*!< Position of PIN field. */
#define TAD_PSEL_TRACEDATA1_PIN_Msk (0x1FUL << TAD_PSEL_TRACEDATA1_PIN_Pos) /*!< Bit mask of PIN field. */

/* Register: TAD_PSEL_TRACEDATA2 */
/* Description: Pin number configuration for TRACEDATA[2] */

/* Bit 31 : Connection */
#define TAD_PSEL_TRACEDATA2_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define TAD_PSEL_TRACEDATA2_CONNECT_Msk (0x1UL << TAD_PSEL_TRACEDATA2_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define TAD_PSEL_TRACEDATA2_CONNECT_Connected (0UL) /*!< Connect */
#define TAD_PSEL_TRACEDATA2_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define TAD_PSEL_TRACEDATA2_PIN_Pos (0UL) /*!< Position of PIN field. */
#define TAD_PSEL_TRACEDATA2_PIN_Msk (0x1FUL << TAD_PSEL_TRACEDATA2_PIN_Pos) /*!< Bit mask of PIN field. */

/* Register: TAD_PSEL_TRACEDATA3 */
/* Description: Pin number configuration for TRACEDATA[3] */

/* Bit 31 : Connection */
#define TAD_PSEL_TRACEDATA3_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define TAD_PSEL_TRACEDATA3_CONNECT_Msk (0x1UL << TAD_PSEL_TRACEDATA3_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define TAD_PSEL_TRACEDATA3_CONNECT_Connected (0UL) /*!< Connect */
#define TAD_PSEL_TRACEDATA3_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define TAD_PSEL_TRACEDATA3_PIN_Pos (0UL) /*!< Position of PIN field. */
#define TAD_PSEL_TRACEDATA3_PIN_Msk (0x1FUL << TAD_PSEL_TRACEDATA3_PIN_Pos) /*!< Bit mask of PIN field. */

/* Register: TAD_TRACEPORTSPEED */
/* Description: Clocking options for the Trace Port debug interface */

/* Bits 1..0 : Speed of Trace Port clock. Note that the TRACECLK pin will output this clock divided by two. */
#define TAD_TRACEPORTSPEED_TRACEPORTSPEED_Pos (0UL) /*!< Position of TRACEPORTSPEED field. */
#define TAD_TRACEPORTSPEED_TRACEPORTSPEED_Msk (0x3UL << TAD_TRACEPORTSPEED_TRACEPORTSPEED_Pos) /*!< Bit mask of TRACEPORTSPEED field. */
#define TAD_TRACEPORTSPEED_TRACEPORTSPEED_32MHz (0UL) /*!< 32 MHz Trace Port clock (TRACECLK = 16 MHz) */
#define TAD_TRACEPORTSPEED_TRACEPORTSPEED_16MHz (1UL) /*!< 16 MHz Trace Port clock (TRACECLK = 8 MHz) */
#define TAD_TRACEPORTSPEED_TRACEPORTSPEED_8MHz (2UL) /*!< 8 MHz Trace Port clock (TRACECLK = 4 MHz) */
#define TAD_TRACEPORTSPEED_TRACEPORTSPEED_4MHz (3UL) /*!< 4 MHz Trace Port clock (TRACECLK = 2 MHz) */


/* Peripheral: TIMER */
/* Description: Timer/Counter 0 */

/* Register: TIMER_TASKS_START */
/* Description: Start Timer */

/* Bit 0 : Start Timer */
#define TIMER_TASKS_START_TASKS_START_Pos (0UL) /*!< Position of TASKS_START field. */
#define TIMER_TASKS_START_TASKS_START_Msk (0x1UL << TIMER_TASKS_START_TASKS_START_Pos) /*!< Bit mask of TASKS_START field. */
#define TIMER_TASKS_START_TASKS_START_Trigger (1UL) /*!< Trigger task */

/* Register: TIMER_TASKS_STOP */
/* Description: Stop Timer */

/* Bit 0 : Stop Timer */
#define TIMER_TASKS_STOP_TASKS_STOP_Pos (0UL) /*!< Position of TASKS_STOP field. */
#define TIMER_TASKS_STOP_TASKS_STOP_Msk (0x1UL << TIMER_TASKS_STOP_TASKS_STOP_Pos) /*!< Bit mask of TASKS_STOP field. */
#define TIMER_TASKS_STOP_TASKS_STOP_Trigger (1UL) /*!< Trigger task */

/* Register: TIMER_TASKS_COUNT */
/* Description: Increment Timer (Counter mode only) */

/* Bit 0 : Increment Timer (Counter mode only) */
#define TIMER_TASKS_COUNT_TASKS_COUNT_Pos (0UL) /*!< Position of TASKS_COUNT field. */
#define TIMER_TASKS_COUNT_TASKS_COUNT_Msk (0x1UL << TIMER_TASKS_COUNT_TASKS_COUNT_Pos) /*!< Bit mask of TASKS_COUNT field. */
#define TIMER_TASKS_COUNT_TASKS_COUNT_Trigger (1UL) /*!< Trigger task */

/* Register: TIMER_TASKS_CLEAR */
/* Description: Clear time */

/* Bit 0 : Clear time */
#define TIMER_TASKS_CLEAR_TASKS_CLEAR_Pos (0UL) /*!< Position of TASKS_CLEAR field. */
#define TIMER_TASKS_CLEAR_TASKS_CLEAR_Msk (0x1UL << TIMER_TASKS_CLEAR_TASKS_CLEAR_Pos) /*!< Bit mask of TASKS_CLEAR field. */
#define TIMER_TASKS_CLEAR_TASKS_CLEAR_Trigger (1UL) /*!< Trigger task */

/* Register: TIMER_TASKS_SHUTDOWN */
/* Description: Deprecated register - Shut down timer */

/* Bit 0 : Deprecated field -  Shut down timer */
#define TIMER_TASKS_SHUTDOWN_TASKS_SHUTDOWN_Pos (0UL) /*!< Position of TASKS_SHUTDOWN field. */
#define TIMER_TASKS_SHUTDOWN_TASKS_SHUTDOWN_Msk (0x1UL << TIMER_TASKS_SHUTDOWN_TASKS_SHUTDOWN_Pos) /*!< Bit mask of TASKS_SHUTDOWN field. */
#define TIMER_TASKS_SHUTDOWN_TASKS_SHUTDOWN_Trigger (1UL) /*!< Trigger task */

/* Register: TIMER_TASKS_CAPTURE */
/* Description: Description collection: Capture Timer value to CC[n] register */

/* Bit 0 : Capture Timer value to CC[n] register */
#define TIMER_TASKS_CAPTURE_TASKS_CAPTURE_Pos (0UL) /*!< Position of TASKS_CAPTURE field. */
#define TIMER_TASKS_CAPTURE_TASKS_CAPTURE_Msk (0x1UL << TIMER_TASKS_CAPTURE_TASKS_CAPTURE_Pos) /*!< Bit mask of TASKS_CAPTURE field. */
#define TIMER_TASKS_CAPTURE_TASKS_CAPTURE_Trigger (1UL) /*!< Trigger task */

/* Register: TIMER_SUBSCRIBE_START */
/* Description: Subscribe configuration for task START */

/* Bit 31 :   */
#define TIMER_SUBSCRIBE_START_EN_Pos (31UL) /*!< Position of EN field. */
#define TIMER_SUBSCRIBE_START_EN_Msk (0x1UL << TIMER_SUBSCRIBE_START_EN_Pos) /*!< Bit mask of EN field. */
#define TIMER_SUBSCRIBE_START_EN_Disabled (0UL) /*!< Disable subscription */
#define TIMER_SUBSCRIBE_START_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task START will subscribe to */
#define TIMER_SUBSCRIBE_START_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TIMER_SUBSCRIBE_START_CHIDX_Msk (0xFUL << TIMER_SUBSCRIBE_START_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TIMER_SUBSCRIBE_STOP */
/* Description: Subscribe configuration for task STOP */

/* Bit 31 :   */
#define TIMER_SUBSCRIBE_STOP_EN_Pos (31UL) /*!< Position of EN field. */
#define TIMER_SUBSCRIBE_STOP_EN_Msk (0x1UL << TIMER_SUBSCRIBE_STOP_EN_Pos) /*!< Bit mask of EN field. */
#define TIMER_SUBSCRIBE_STOP_EN_Disabled (0UL) /*!< Disable subscription */
#define TIMER_SUBSCRIBE_STOP_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task STOP will subscribe to */
#define TIMER_SUBSCRIBE_STOP_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TIMER_SUBSCRIBE_STOP_CHIDX_Msk (0xFUL << TIMER_SUBSCRIBE_STOP_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TIMER_SUBSCRIBE_COUNT */
/* Description: Subscribe configuration for task COUNT */

/* Bit 31 :   */
#define TIMER_SUBSCRIBE_COUNT_EN_Pos (31UL) /*!< Position of EN field. */
#define TIMER_SUBSCRIBE_COUNT_EN_Msk (0x1UL << TIMER_SUBSCRIBE_COUNT_EN_Pos) /*!< Bit mask of EN field. */
#define TIMER_SUBSCRIBE_COUNT_EN_Disabled (0UL) /*!< Disable subscription */
#define TIMER_SUBSCRIBE_COUNT_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task COUNT will subscribe to */
#define TIMER_SUBSCRIBE_COUNT_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TIMER_SUBSCRIBE_COUNT_CHIDX_Msk (0xFUL << TIMER_SUBSCRIBE_COUNT_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TIMER_SUBSCRIBE_CLEAR */
/* Description: Subscribe configuration for task CLEAR */

/* Bit 31 :   */
#define TIMER_SUBSCRIBE_CLEAR_EN_Pos (31UL) /*!< Position of EN field. */
#define TIMER_SUBSCRIBE_CLEAR_EN_Msk (0x1UL << TIMER_SUBSCRIBE_CLEAR_EN_Pos) /*!< Bit mask of EN field. */
#define TIMER_SUBSCRIBE_CLEAR_EN_Disabled (0UL) /*!< Disable subscription */
#define TIMER_SUBSCRIBE_CLEAR_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task CLEAR will subscribe to */
#define TIMER_SUBSCRIBE_CLEAR_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TIMER_SUBSCRIBE_CLEAR_CHIDX_Msk (0xFUL << TIMER_SUBSCRIBE_CLEAR_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TIMER_SUBSCRIBE_SHUTDOWN */
/* Description: Deprecated register - Subscribe configuration for task SHUTDOWN */

/* Bit 31 :   */
#define TIMER_SUBSCRIBE_SHUTDOWN_EN_Pos (31UL) /*!< Position of EN field. */
#define TIMER_SUBSCRIBE_SHUTDOWN_EN_Msk (0x1UL << TIMER_SUBSCRIBE_SHUTDOWN_EN_Pos) /*!< Bit mask of EN field. */
#define TIMER_SUBSCRIBE_SHUTDOWN_EN_Disabled (0UL) /*!< Disable subscription */
#define TIMER_SUBSCRIBE_SHUTDOWN_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task SHUTDOWN will subscribe to */
#define TIMER_SUBSCRIBE_SHUTDOWN_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TIMER_SUBSCRIBE_SHUTDOWN_CHIDX_Msk (0xFUL << TIMER_SUBSCRIBE_SHUTDOWN_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TIMER_SUBSCRIBE_CAPTURE */
/* Description: Description collection: Subscribe configuration for task CAPTURE[n] */

/* Bit 31 :   */
#define TIMER_SUBSCRIBE_CAPTURE_EN_Pos (31UL) /*!< Position of EN field. */
#define TIMER_SUBSCRIBE_CAPTURE_EN_Msk (0x1UL << TIMER_SUBSCRIBE_CAPTURE_EN_Pos) /*!< Bit mask of EN field. */
#define TIMER_SUBSCRIBE_CAPTURE_EN_Disabled (0UL) /*!< Disable subscription */
#define TIMER_SUBSCRIBE_CAPTURE_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task CAPTURE[n] will subscribe to */
#define TIMER_SUBSCRIBE_CAPTURE_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TIMER_SUBSCRIBE_CAPTURE_CHIDX_Msk (0xFUL << TIMER_SUBSCRIBE_CAPTURE_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TIMER_EVENTS_COMPARE */
/* Description: Description collection: Compare event on CC[n] match */

/* Bit 0 : Compare event on CC[n] match */
#define TIMER_EVENTS_COMPARE_EVENTS_COMPARE_Pos (0UL) /*!< Position of EVENTS_COMPARE field. */
#define TIMER_EVENTS_COMPARE_EVENTS_COMPARE_Msk (0x1UL << TIMER_EVENTS_COMPARE_EVENTS_COMPARE_Pos) /*!< Bit mask of EVENTS_COMPARE field. */
#define TIMER_EVENTS_COMPARE_EVENTS_COMPARE_NotGenerated (0UL) /*!< Event not generated */
#define TIMER_EVENTS_COMPARE_EVENTS_COMPARE_Generated (1UL) /*!< Event generated */

/* Register: TIMER_PUBLISH_COMPARE */
/* Description: Description collection: Publish configuration for event COMPARE[n] */

/* Bit 31 :   */
#define TIMER_PUBLISH_COMPARE_EN_Pos (31UL) /*!< Position of EN field. */
#define TIMER_PUBLISH_COMPARE_EN_Msk (0x1UL << TIMER_PUBLISH_COMPARE_EN_Pos) /*!< Bit mask of EN field. */
#define TIMER_PUBLISH_COMPARE_EN_Disabled (0UL) /*!< Disable publishing */
#define TIMER_PUBLISH_COMPARE_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event COMPARE[n] will publish to. */
#define TIMER_PUBLISH_COMPARE_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TIMER_PUBLISH_COMPARE_CHIDX_Msk (0xFUL << TIMER_PUBLISH_COMPARE_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TIMER_SHORTS */
/* Description: Shortcuts between local events and tasks */

/* Bit 13 : Shortcut between event COMPARE[5] and task STOP */
#define TIMER_SHORTS_COMPARE5_STOP_Pos (13UL) /*!< Position of COMPARE5_STOP field. */
#define TIMER_SHORTS_COMPARE5_STOP_Msk (0x1UL << TIMER_SHORTS_COMPARE5_STOP_Pos) /*!< Bit mask of COMPARE5_STOP field. */
#define TIMER_SHORTS_COMPARE5_STOP_Disabled (0UL) /*!< Disable shortcut */
#define TIMER_SHORTS_COMPARE5_STOP_Enabled (1UL) /*!< Enable shortcut */

/* Bit 12 : Shortcut between event COMPARE[4] and task STOP */
#define TIMER_SHORTS_COMPARE4_STOP_Pos (12UL) /*!< Position of COMPARE4_STOP field. */
#define TIMER_SHORTS_COMPARE4_STOP_Msk (0x1UL << TIMER_SHORTS_COMPARE4_STOP_Pos) /*!< Bit mask of COMPARE4_STOP field. */
#define TIMER_SHORTS_COMPARE4_STOP_Disabled (0UL) /*!< Disable shortcut */
#define TIMER_SHORTS_COMPARE4_STOP_Enabled (1UL) /*!< Enable shortcut */

/* Bit 11 : Shortcut between event COMPARE[3] and task STOP */
#define TIMER_SHORTS_COMPARE3_STOP_Pos (11UL) /*!< Position of COMPARE3_STOP field. */
#define TIMER_SHORTS_COMPARE3_STOP_Msk (0x1UL << TIMER_SHORTS_COMPARE3_STOP_Pos) /*!< Bit mask of COMPARE3_STOP field. */
#define TIMER_SHORTS_COMPARE3_STOP_Disabled (0UL) /*!< Disable shortcut */
#define TIMER_SHORTS_COMPARE3_STOP_Enabled (1UL) /*!< Enable shortcut */

/* Bit 10 : Shortcut between event COMPARE[2] and task STOP */
#define TIMER_SHORTS_COMPARE2_STOP_Pos (10UL) /*!< Position of COMPARE2_STOP field. */
#define TIMER_SHORTS_COMPARE2_STOP_Msk (0x1UL << TIMER_SHORTS_COMPARE2_STOP_Pos) /*!< Bit mask of COMPARE2_STOP field. */
#define TIMER_SHORTS_COMPARE2_STOP_Disabled (0UL) /*!< Disable shortcut */
#define TIMER_SHORTS_COMPARE2_STOP_Enabled (1UL) /*!< Enable shortcut */

/* Bit 9 : Shortcut between event COMPARE[1] and task STOP */
#define TIMER_SHORTS_COMPARE1_STOP_Pos (9UL) /*!< Position of COMPARE1_STOP field. */
#define TIMER_SHORTS_COMPARE1_STOP_Msk (0x1UL << TIMER_SHORTS_COMPARE1_STOP_Pos) /*!< Bit mask of COMPARE1_STOP field. */
#define TIMER_SHORTS_COMPARE1_STOP_Disabled (0UL) /*!< Disable shortcut */
#define TIMER_SHORTS_COMPARE1_STOP_Enabled (1UL) /*!< Enable shortcut */

/* Bit 8 : Shortcut between event COMPARE[0] and task STOP */
#define TIMER_SHORTS_COMPARE0_STOP_Pos (8UL) /*!< Position of COMPARE0_STOP field. */
#define TIMER_SHORTS_COMPARE0_STOP_Msk (0x1UL << TIMER_SHORTS_COMPARE0_STOP_Pos) /*!< Bit mask of COMPARE0_STOP field. */
#define TIMER_SHORTS_COMPARE0_STOP_Disabled (0UL) /*!< Disable shortcut */
#define TIMER_SHORTS_COMPARE0_STOP_Enabled (1UL) /*!< Enable shortcut */

/* Bit 5 : Shortcut between event COMPARE[5] and task CLEAR */
#define TIMER_SHORTS_COMPARE5_CLEAR_Pos (5UL) /*!< Position of COMPARE5_CLEAR field. */
#define TIMER_SHORTS_COMPARE5_CLEAR_Msk (0x1UL << TIMER_SHORTS_COMPARE5_CLEAR_Pos) /*!< Bit mask of COMPARE5_CLEAR field. */
#define TIMER_SHORTS_COMPARE5_CLEAR_Disabled (0UL) /*!< Disable shortcut */
#define TIMER_SHORTS_COMPARE5_CLEAR_Enabled (1UL) /*!< Enable shortcut */

/* Bit 4 : Shortcut between event COMPARE[4] and task CLEAR */
#define TIMER_SHORTS_COMPARE4_CLEAR_Pos (4UL) /*!< Position of COMPARE4_CLEAR field. */
#define TIMER_SHORTS_COMPARE4_CLEAR_Msk (0x1UL << TIMER_SHORTS_COMPARE4_CLEAR_Pos) /*!< Bit mask of COMPARE4_CLEAR field. */
#define TIMER_SHORTS_COMPARE4_CLEAR_Disabled (0UL) /*!< Disable shortcut */
#define TIMER_SHORTS_COMPARE4_CLEAR_Enabled (1UL) /*!< Enable shortcut */

/* Bit 3 : Shortcut between event COMPARE[3] and task CLEAR */
#define TIMER_SHORTS_COMPARE3_CLEAR_Pos (3UL) /*!< Position of COMPARE3_CLEAR field. */
#define TIMER_SHORTS_COMPARE3_CLEAR_Msk (0x1UL << TIMER_SHORTS_COMPARE3_CLEAR_Pos) /*!< Bit mask of COMPARE3_CLEAR field. */
#define TIMER_SHORTS_COMPARE3_CLEAR_Disabled (0UL) /*!< Disable shortcut */
#define TIMER_SHORTS_COMPARE3_CLEAR_Enabled (1UL) /*!< Enable shortcut */

/* Bit 2 : Shortcut between event COMPARE[2] and task CLEAR */
#define TIMER_SHORTS_COMPARE2_CLEAR_Pos (2UL) /*!< Position of COMPARE2_CLEAR field. */
#define TIMER_SHORTS_COMPARE2_CLEAR_Msk (0x1UL << TIMER_SHORTS_COMPARE2_CLEAR_Pos) /*!< Bit mask of COMPARE2_CLEAR field. */
#define TIMER_SHORTS_COMPARE2_CLEAR_Disabled (0UL) /*!< Disable shortcut */
#define TIMER_SHORTS_COMPARE2_CLEAR_Enabled (1UL) /*!< Enable shortcut */

/* Bit 1 : Shortcut between event COMPARE[1] and task CLEAR */
#define TIMER_SHORTS_COMPARE1_CLEAR_Pos (1UL) /*!< Position of COMPARE1_CLEAR field. */
#define TIMER_SHORTS_COMPARE1_CLEAR_Msk (0x1UL << TIMER_SHORTS_COMPARE1_CLEAR_Pos) /*!< Bit mask of COMPARE1_CLEAR field. */
#define TIMER_SHORTS_COMPARE1_CLEAR_Disabled (0UL) /*!< Disable shortcut */
#define TIMER_SHORTS_COMPARE1_CLEAR_Enabled (1UL) /*!< Enable shortcut */

/* Bit 0 : Shortcut between event COMPARE[0] and task CLEAR */
#define TIMER_SHORTS_COMPARE0_CLEAR_Pos (0UL) /*!< Position of COMPARE0_CLEAR field. */
#define TIMER_SHORTS_COMPARE0_CLEAR_Msk (0x1UL << TIMER_SHORTS_COMPARE0_CLEAR_Pos) /*!< Bit mask of COMPARE0_CLEAR field. */
#define TIMER_SHORTS_COMPARE0_CLEAR_Disabled (0UL) /*!< Disable shortcut */
#define TIMER_SHORTS_COMPARE0_CLEAR_Enabled (1UL) /*!< Enable shortcut */

/* Register: TIMER_INTENSET */
/* Description: Enable interrupt */

/* Bit 21 : Write '1' to enable interrupt for event COMPARE[5] */
#define TIMER_INTENSET_COMPARE5_Pos (21UL) /*!< Position of COMPARE5 field. */
#define TIMER_INTENSET_COMPARE5_Msk (0x1UL << TIMER_INTENSET_COMPARE5_Pos) /*!< Bit mask of COMPARE5 field. */
#define TIMER_INTENSET_COMPARE5_Disabled (0UL) /*!< Read: Disabled */
#define TIMER_INTENSET_COMPARE5_Enabled (1UL) /*!< Read: Enabled */
#define TIMER_INTENSET_COMPARE5_Set (1UL) /*!< Enable */

/* Bit 20 : Write '1' to enable interrupt for event COMPARE[4] */
#define TIMER_INTENSET_COMPARE4_Pos (20UL) /*!< Position of COMPARE4 field. */
#define TIMER_INTENSET_COMPARE4_Msk (0x1UL << TIMER_INTENSET_COMPARE4_Pos) /*!< Bit mask of COMPARE4 field. */
#define TIMER_INTENSET_COMPARE4_Disabled (0UL) /*!< Read: Disabled */
#define TIMER_INTENSET_COMPARE4_Enabled (1UL) /*!< Read: Enabled */
#define TIMER_INTENSET_COMPARE4_Set (1UL) /*!< Enable */

/* Bit 19 : Write '1' to enable interrupt for event COMPARE[3] */
#define TIMER_INTENSET_COMPARE3_Pos (19UL) /*!< Position of COMPARE3 field. */
#define TIMER_INTENSET_COMPARE3_Msk (0x1UL << TIMER_INTENSET_COMPARE3_Pos) /*!< Bit mask of COMPARE3 field. */
#define TIMER_INTENSET_COMPARE3_Disabled (0UL) /*!< Read: Disabled */
#define TIMER_INTENSET_COMPARE3_Enabled (1UL) /*!< Read: Enabled */
#define TIMER_INTENSET_COMPARE3_Set (1UL) /*!< Enable */

/* Bit 18 : Write '1' to enable interrupt for event COMPARE[2] */
#define TIMER_INTENSET_COMPARE2_Pos (18UL) /*!< Position of COMPARE2 field. */
#define TIMER_INTENSET_COMPARE2_Msk (0x1UL << TIMER_INTENSET_COMPARE2_Pos) /*!< Bit mask of COMPARE2 field. */
#define TIMER_INTENSET_COMPARE2_Disabled (0UL) /*!< Read: Disabled */
#define TIMER_INTENSET_COMPARE2_Enabled (1UL) /*!< Read: Enabled */
#define TIMER_INTENSET_COMPARE2_Set (1UL) /*!< Enable */

/* Bit 17 : Write '1' to enable interrupt for event COMPARE[1] */
#define TIMER_INTENSET_COMPARE1_Pos (17UL) /*!< Position of COMPARE1 field. */
#define TIMER_INTENSET_COMPARE1_Msk (0x1UL << TIMER_INTENSET_COMPARE1_Pos) /*!< Bit mask of COMPARE1 field. */
#define TIMER_INTENSET_COMPARE1_Disabled (0UL) /*!< Read: Disabled */
#define TIMER_INTENSET_COMPARE1_Enabled (1UL) /*!< Read: Enabled */
#define TIMER_INTENSET_COMPARE1_Set (1UL) /*!< Enable */

/* Bit 16 : Write '1' to enable interrupt for event COMPARE[0] */
#define TIMER_INTENSET_COMPARE0_Pos (16UL) /*!< Position of COMPARE0 field. */
#define TIMER_INTENSET_COMPARE0_Msk (0x1UL << TIMER_INTENSET_COMPARE0_Pos) /*!< Bit mask of COMPARE0 field. */
#define TIMER_INTENSET_COMPARE0_Disabled (0UL) /*!< Read: Disabled */
#define TIMER_INTENSET_COMPARE0_Enabled (1UL) /*!< Read: Enabled */
#define TIMER_INTENSET_COMPARE0_Set (1UL) /*!< Enable */

/* Register: TIMER_INTENCLR */
/* Description: Disable interrupt */

/* Bit 21 : Write '1' to disable interrupt for event COMPARE[5] */
#define TIMER_INTENCLR_COMPARE5_Pos (21UL) /*!< Position of COMPARE5 field. */
#define TIMER_INTENCLR_COMPARE5_Msk (0x1UL << TIMER_INTENCLR_COMPARE5_Pos) /*!< Bit mask of COMPARE5 field. */
#define TIMER_INTENCLR_COMPARE5_Disabled (0UL) /*!< Read: Disabled */
#define TIMER_INTENCLR_COMPARE5_Enabled (1UL) /*!< Read: Enabled */
#define TIMER_INTENCLR_COMPARE5_Clear (1UL) /*!< Disable */

/* Bit 20 : Write '1' to disable interrupt for event COMPARE[4] */
#define TIMER_INTENCLR_COMPARE4_Pos (20UL) /*!< Position of COMPARE4 field. */
#define TIMER_INTENCLR_COMPARE4_Msk (0x1UL << TIMER_INTENCLR_COMPARE4_Pos) /*!< Bit mask of COMPARE4 field. */
#define TIMER_INTENCLR_COMPARE4_Disabled (0UL) /*!< Read: Disabled */
#define TIMER_INTENCLR_COMPARE4_Enabled (1UL) /*!< Read: Enabled */
#define TIMER_INTENCLR_COMPARE4_Clear (1UL) /*!< Disable */

/* Bit 19 : Write '1' to disable interrupt for event COMPARE[3] */
#define TIMER_INTENCLR_COMPARE3_Pos (19UL) /*!< Position of COMPARE3 field. */
#define TIMER_INTENCLR_COMPARE3_Msk (0x1UL << TIMER_INTENCLR_COMPARE3_Pos) /*!< Bit mask of COMPARE3 field. */
#define TIMER_INTENCLR_COMPARE3_Disabled (0UL) /*!< Read: Disabled */
#define TIMER_INTENCLR_COMPARE3_Enabled (1UL) /*!< Read: Enabled */
#define TIMER_INTENCLR_COMPARE3_Clear (1UL) /*!< Disable */

/* Bit 18 : Write '1' to disable interrupt for event COMPARE[2] */
#define TIMER_INTENCLR_COMPARE2_Pos (18UL) /*!< Position of COMPARE2 field. */
#define TIMER_INTENCLR_COMPARE2_Msk (0x1UL << TIMER_INTENCLR_COMPARE2_Pos) /*!< Bit mask of COMPARE2 field. */
#define TIMER_INTENCLR_COMPARE2_Disabled (0UL) /*!< Read: Disabled */
#define TIMER_INTENCLR_COMPARE2_Enabled (1UL) /*!< Read: Enabled */
#define TIMER_INTENCLR_COMPARE2_Clear (1UL) /*!< Disable */

/* Bit 17 : Write '1' to disable interrupt for event COMPARE[1] */
#define TIMER_INTENCLR_COMPARE1_Pos (17UL) /*!< Position of COMPARE1 field. */
#define TIMER_INTENCLR_COMPARE1_Msk (0x1UL << TIMER_INTENCLR_COMPARE1_Pos) /*!< Bit mask of COMPARE1 field. */
#define TIMER_INTENCLR_COMPARE1_Disabled (0UL) /*!< Read: Disabled */
#define TIMER_INTENCLR_COMPARE1_Enabled (1UL) /*!< Read: Enabled */
#define TIMER_INTENCLR_COMPARE1_Clear (1UL) /*!< Disable */

/* Bit 16 : Write '1' to disable interrupt for event COMPARE[0] */
#define TIMER_INTENCLR_COMPARE0_Pos (16UL) /*!< Position of COMPARE0 field. */
#define TIMER_INTENCLR_COMPARE0_Msk (0x1UL << TIMER_INTENCLR_COMPARE0_Pos) /*!< Bit mask of COMPARE0 field. */
#define TIMER_INTENCLR_COMPARE0_Disabled (0UL) /*!< Read: Disabled */
#define TIMER_INTENCLR_COMPARE0_Enabled (1UL) /*!< Read: Enabled */
#define TIMER_INTENCLR_COMPARE0_Clear (1UL) /*!< Disable */

/* Register: TIMER_MODE */
/* Description: Timer mode selection */

/* Bits 1..0 : Timer mode */
#define TIMER_MODE_MODE_Pos (0UL) /*!< Position of MODE field. */
#define TIMER_MODE_MODE_Msk (0x3UL << TIMER_MODE_MODE_Pos) /*!< Bit mask of MODE field. */
#define TIMER_MODE_MODE_Timer (0UL) /*!< Select Timer mode */
#define TIMER_MODE_MODE_Counter (1UL) /*!< Deprecated enumerator -  Select Counter mode */
#define TIMER_MODE_MODE_LowPowerCounter (2UL) /*!< Select Low Power Counter mode */

/* Register: TIMER_BITMODE */
/* Description: Configure the number of bits used by the TIMER */

/* Bits 1..0 : Timer bit width */
#define TIMER_BITMODE_BITMODE_Pos (0UL) /*!< Position of BITMODE field. */
#define TIMER_BITMODE_BITMODE_Msk (0x3UL << TIMER_BITMODE_BITMODE_Pos) /*!< Bit mask of BITMODE field. */
#define TIMER_BITMODE_BITMODE_16Bit (0UL) /*!< 16 bit timer bit width */
#define TIMER_BITMODE_BITMODE_08Bit (1UL) /*!< 8 bit timer bit width */
#define TIMER_BITMODE_BITMODE_24Bit (2UL) /*!< 24 bit timer bit width */
#define TIMER_BITMODE_BITMODE_32Bit (3UL) /*!< 32 bit timer bit width */

/* Register: TIMER_PRESCALER */
/* Description: Timer prescaler register */

/* Bits 3..0 : Prescaler value */
#define TIMER_PRESCALER_PRESCALER_Pos (0UL) /*!< Position of PRESCALER field. */
#define TIMER_PRESCALER_PRESCALER_Msk (0xFUL << TIMER_PRESCALER_PRESCALER_Pos) /*!< Bit mask of PRESCALER field. */

/* Register: TIMER_CC */
/* Description: Description collection: Capture/Compare register n */

/* Bits 31..0 : Capture/Compare value */
#define TIMER_CC_CC_Pos (0UL) /*!< Position of CC field. */
#define TIMER_CC_CC_Msk (0xFFFFFFFFUL << TIMER_CC_CC_Pos) /*!< Bit mask of CC field. */


/* Peripheral: TWIM */
/* Description: I2C compatible Two-Wire Master Interface with EasyDMA 0 */

/* Register: TWIM_TASKS_STARTRX */
/* Description: Start TWI receive sequence */

/* Bit 0 : Start TWI receive sequence */
#define TWIM_TASKS_STARTRX_TASKS_STARTRX_Pos (0UL) /*!< Position of TASKS_STARTRX field. */
#define TWIM_TASKS_STARTRX_TASKS_STARTRX_Msk (0x1UL << TWIM_TASKS_STARTRX_TASKS_STARTRX_Pos) /*!< Bit mask of TASKS_STARTRX field. */
#define TWIM_TASKS_STARTRX_TASKS_STARTRX_Trigger (1UL) /*!< Trigger task */

/* Register: TWIM_TASKS_STARTTX */
/* Description: Start TWI transmit sequence */

/* Bit 0 : Start TWI transmit sequence */
#define TWIM_TASKS_STARTTX_TASKS_STARTTX_Pos (0UL) /*!< Position of TASKS_STARTTX field. */
#define TWIM_TASKS_STARTTX_TASKS_STARTTX_Msk (0x1UL << TWIM_TASKS_STARTTX_TASKS_STARTTX_Pos) /*!< Bit mask of TASKS_STARTTX field. */
#define TWIM_TASKS_STARTTX_TASKS_STARTTX_Trigger (1UL) /*!< Trigger task */

/* Register: TWIM_TASKS_STOP */
/* Description: Stop TWI transaction. Must be issued while the TWI master is not suspended. */

/* Bit 0 : Stop TWI transaction. Must be issued while the TWI master is not suspended. */
#define TWIM_TASKS_STOP_TASKS_STOP_Pos (0UL) /*!< Position of TASKS_STOP field. */
#define TWIM_TASKS_STOP_TASKS_STOP_Msk (0x1UL << TWIM_TASKS_STOP_TASKS_STOP_Pos) /*!< Bit mask of TASKS_STOP field. */
#define TWIM_TASKS_STOP_TASKS_STOP_Trigger (1UL) /*!< Trigger task */

/* Register: TWIM_TASKS_SUSPEND */
/* Description: Suspend TWI transaction */

/* Bit 0 : Suspend TWI transaction */
#define TWIM_TASKS_SUSPEND_TASKS_SUSPEND_Pos (0UL) /*!< Position of TASKS_SUSPEND field. */
#define TWIM_TASKS_SUSPEND_TASKS_SUSPEND_Msk (0x1UL << TWIM_TASKS_SUSPEND_TASKS_SUSPEND_Pos) /*!< Bit mask of TASKS_SUSPEND field. */
#define TWIM_TASKS_SUSPEND_TASKS_SUSPEND_Trigger (1UL) /*!< Trigger task */

/* Register: TWIM_TASKS_RESUME */
/* Description: Resume TWI transaction */

/* Bit 0 : Resume TWI transaction */
#define TWIM_TASKS_RESUME_TASKS_RESUME_Pos (0UL) /*!< Position of TASKS_RESUME field. */
#define TWIM_TASKS_RESUME_TASKS_RESUME_Msk (0x1UL << TWIM_TASKS_RESUME_TASKS_RESUME_Pos) /*!< Bit mask of TASKS_RESUME field. */
#define TWIM_TASKS_RESUME_TASKS_RESUME_Trigger (1UL) /*!< Trigger task */

/* Register: TWIM_SUBSCRIBE_STARTRX */
/* Description: Subscribe configuration for task STARTRX */

/* Bit 31 :   */
#define TWIM_SUBSCRIBE_STARTRX_EN_Pos (31UL) /*!< Position of EN field. */
#define TWIM_SUBSCRIBE_STARTRX_EN_Msk (0x1UL << TWIM_SUBSCRIBE_STARTRX_EN_Pos) /*!< Bit mask of EN field. */
#define TWIM_SUBSCRIBE_STARTRX_EN_Disabled (0UL) /*!< Disable subscription */
#define TWIM_SUBSCRIBE_STARTRX_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task STARTRX will subscribe to */
#define TWIM_SUBSCRIBE_STARTRX_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TWIM_SUBSCRIBE_STARTRX_CHIDX_Msk (0xFUL << TWIM_SUBSCRIBE_STARTRX_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TWIM_SUBSCRIBE_STARTTX */
/* Description: Subscribe configuration for task STARTTX */

/* Bit 31 :   */
#define TWIM_SUBSCRIBE_STARTTX_EN_Pos (31UL) /*!< Position of EN field. */
#define TWIM_SUBSCRIBE_STARTTX_EN_Msk (0x1UL << TWIM_SUBSCRIBE_STARTTX_EN_Pos) /*!< Bit mask of EN field. */
#define TWIM_SUBSCRIBE_STARTTX_EN_Disabled (0UL) /*!< Disable subscription */
#define TWIM_SUBSCRIBE_STARTTX_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task STARTTX will subscribe to */
#define TWIM_SUBSCRIBE_STARTTX_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TWIM_SUBSCRIBE_STARTTX_CHIDX_Msk (0xFUL << TWIM_SUBSCRIBE_STARTTX_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TWIM_SUBSCRIBE_STOP */
/* Description: Subscribe configuration for task STOP */

/* Bit 31 :   */
#define TWIM_SUBSCRIBE_STOP_EN_Pos (31UL) /*!< Position of EN field. */
#define TWIM_SUBSCRIBE_STOP_EN_Msk (0x1UL << TWIM_SUBSCRIBE_STOP_EN_Pos) /*!< Bit mask of EN field. */
#define TWIM_SUBSCRIBE_STOP_EN_Disabled (0UL) /*!< Disable subscription */
#define TWIM_SUBSCRIBE_STOP_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task STOP will subscribe to */
#define TWIM_SUBSCRIBE_STOP_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TWIM_SUBSCRIBE_STOP_CHIDX_Msk (0xFUL << TWIM_SUBSCRIBE_STOP_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TWIM_SUBSCRIBE_SUSPEND */
/* Description: Subscribe configuration for task SUSPEND */

/* Bit 31 :   */
#define TWIM_SUBSCRIBE_SUSPEND_EN_Pos (31UL) /*!< Position of EN field. */
#define TWIM_SUBSCRIBE_SUSPEND_EN_Msk (0x1UL << TWIM_SUBSCRIBE_SUSPEND_EN_Pos) /*!< Bit mask of EN field. */
#define TWIM_SUBSCRIBE_SUSPEND_EN_Disabled (0UL) /*!< Disable subscription */
#define TWIM_SUBSCRIBE_SUSPEND_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task SUSPEND will subscribe to */
#define TWIM_SUBSCRIBE_SUSPEND_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TWIM_SUBSCRIBE_SUSPEND_CHIDX_Msk (0xFUL << TWIM_SUBSCRIBE_SUSPEND_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TWIM_SUBSCRIBE_RESUME */
/* Description: Subscribe configuration for task RESUME */

/* Bit 31 :   */
#define TWIM_SUBSCRIBE_RESUME_EN_Pos (31UL) /*!< Position of EN field. */
#define TWIM_SUBSCRIBE_RESUME_EN_Msk (0x1UL << TWIM_SUBSCRIBE_RESUME_EN_Pos) /*!< Bit mask of EN field. */
#define TWIM_SUBSCRIBE_RESUME_EN_Disabled (0UL) /*!< Disable subscription */
#define TWIM_SUBSCRIBE_RESUME_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task RESUME will subscribe to */
#define TWIM_SUBSCRIBE_RESUME_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TWIM_SUBSCRIBE_RESUME_CHIDX_Msk (0xFUL << TWIM_SUBSCRIBE_RESUME_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TWIM_EVENTS_STOPPED */
/* Description: TWI stopped */

/* Bit 0 : TWI stopped */
#define TWIM_EVENTS_STOPPED_EVENTS_STOPPED_Pos (0UL) /*!< Position of EVENTS_STOPPED field. */
#define TWIM_EVENTS_STOPPED_EVENTS_STOPPED_Msk (0x1UL << TWIM_EVENTS_STOPPED_EVENTS_STOPPED_Pos) /*!< Bit mask of EVENTS_STOPPED field. */
#define TWIM_EVENTS_STOPPED_EVENTS_STOPPED_NotGenerated (0UL) /*!< Event not generated */
#define TWIM_EVENTS_STOPPED_EVENTS_STOPPED_Generated (1UL) /*!< Event generated */

/* Register: TWIM_EVENTS_ERROR */
/* Description: TWI error */

/* Bit 0 : TWI error */
#define TWIM_EVENTS_ERROR_EVENTS_ERROR_Pos (0UL) /*!< Position of EVENTS_ERROR field. */
#define TWIM_EVENTS_ERROR_EVENTS_ERROR_Msk (0x1UL << TWIM_EVENTS_ERROR_EVENTS_ERROR_Pos) /*!< Bit mask of EVENTS_ERROR field. */
#define TWIM_EVENTS_ERROR_EVENTS_ERROR_NotGenerated (0UL) /*!< Event not generated */
#define TWIM_EVENTS_ERROR_EVENTS_ERROR_Generated (1UL) /*!< Event generated */

/* Register: TWIM_EVENTS_SUSPENDED */
/* Description: Last byte has been sent out after the SUSPEND task has been issued, TWI traffic is now suspended. */

/* Bit 0 : Last byte has been sent out after the SUSPEND task has been issued, TWI traffic is now suspended. */
#define TWIM_EVENTS_SUSPENDED_EVENTS_SUSPENDED_Pos (0UL) /*!< Position of EVENTS_SUSPENDED field. */
#define TWIM_EVENTS_SUSPENDED_EVENTS_SUSPENDED_Msk (0x1UL << TWIM_EVENTS_SUSPENDED_EVENTS_SUSPENDED_Pos) /*!< Bit mask of EVENTS_SUSPENDED field. */
#define TWIM_EVENTS_SUSPENDED_EVENTS_SUSPENDED_NotGenerated (0UL) /*!< Event not generated */
#define TWIM_EVENTS_SUSPENDED_EVENTS_SUSPENDED_Generated (1UL) /*!< Event generated */

/* Register: TWIM_EVENTS_RXSTARTED */
/* Description: Receive sequence started */

/* Bit 0 : Receive sequence started */
#define TWIM_EVENTS_RXSTARTED_EVENTS_RXSTARTED_Pos (0UL) /*!< Position of EVENTS_RXSTARTED field. */
#define TWIM_EVENTS_RXSTARTED_EVENTS_RXSTARTED_Msk (0x1UL << TWIM_EVENTS_RXSTARTED_EVENTS_RXSTARTED_Pos) /*!< Bit mask of EVENTS_RXSTARTED field. */
#define TWIM_EVENTS_RXSTARTED_EVENTS_RXSTARTED_NotGenerated (0UL) /*!< Event not generated */
#define TWIM_EVENTS_RXSTARTED_EVENTS_RXSTARTED_Generated (1UL) /*!< Event generated */

/* Register: TWIM_EVENTS_TXSTARTED */
/* Description: Transmit sequence started */

/* Bit 0 : Transmit sequence started */
#define TWIM_EVENTS_TXSTARTED_EVENTS_TXSTARTED_Pos (0UL) /*!< Position of EVENTS_TXSTARTED field. */
#define TWIM_EVENTS_TXSTARTED_EVENTS_TXSTARTED_Msk (0x1UL << TWIM_EVENTS_TXSTARTED_EVENTS_TXSTARTED_Pos) /*!< Bit mask of EVENTS_TXSTARTED field. */
#define TWIM_EVENTS_TXSTARTED_EVENTS_TXSTARTED_NotGenerated (0UL) /*!< Event not generated */
#define TWIM_EVENTS_TXSTARTED_EVENTS_TXSTARTED_Generated (1UL) /*!< Event generated */

/* Register: TWIM_EVENTS_LASTRX */
/* Description: Byte boundary, starting to receive the last byte */

/* Bit 0 : Byte boundary, starting to receive the last byte */
#define TWIM_EVENTS_LASTRX_EVENTS_LASTRX_Pos (0UL) /*!< Position of EVENTS_LASTRX field. */
#define TWIM_EVENTS_LASTRX_EVENTS_LASTRX_Msk (0x1UL << TWIM_EVENTS_LASTRX_EVENTS_LASTRX_Pos) /*!< Bit mask of EVENTS_LASTRX field. */
#define TWIM_EVENTS_LASTRX_EVENTS_LASTRX_NotGenerated (0UL) /*!< Event not generated */
#define TWIM_EVENTS_LASTRX_EVENTS_LASTRX_Generated (1UL) /*!< Event generated */

/* Register: TWIM_EVENTS_LASTTX */
/* Description: Byte boundary, starting to transmit the last byte */

/* Bit 0 : Byte boundary, starting to transmit the last byte */
#define TWIM_EVENTS_LASTTX_EVENTS_LASTTX_Pos (0UL) /*!< Position of EVENTS_LASTTX field. */
#define TWIM_EVENTS_LASTTX_EVENTS_LASTTX_Msk (0x1UL << TWIM_EVENTS_LASTTX_EVENTS_LASTTX_Pos) /*!< Bit mask of EVENTS_LASTTX field. */
#define TWIM_EVENTS_LASTTX_EVENTS_LASTTX_NotGenerated (0UL) /*!< Event not generated */
#define TWIM_EVENTS_LASTTX_EVENTS_LASTTX_Generated (1UL) /*!< Event generated */

/* Register: TWIM_PUBLISH_STOPPED */
/* Description: Publish configuration for event STOPPED */

/* Bit 31 :   */
#define TWIM_PUBLISH_STOPPED_EN_Pos (31UL) /*!< Position of EN field. */
#define TWIM_PUBLISH_STOPPED_EN_Msk (0x1UL << TWIM_PUBLISH_STOPPED_EN_Pos) /*!< Bit mask of EN field. */
#define TWIM_PUBLISH_STOPPED_EN_Disabled (0UL) /*!< Disable publishing */
#define TWIM_PUBLISH_STOPPED_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event STOPPED will publish to. */
#define TWIM_PUBLISH_STOPPED_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TWIM_PUBLISH_STOPPED_CHIDX_Msk (0xFUL << TWIM_PUBLISH_STOPPED_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TWIM_PUBLISH_ERROR */
/* Description: Publish configuration for event ERROR */

/* Bit 31 :   */
#define TWIM_PUBLISH_ERROR_EN_Pos (31UL) /*!< Position of EN field. */
#define TWIM_PUBLISH_ERROR_EN_Msk (0x1UL << TWIM_PUBLISH_ERROR_EN_Pos) /*!< Bit mask of EN field. */
#define TWIM_PUBLISH_ERROR_EN_Disabled (0UL) /*!< Disable publishing */
#define TWIM_PUBLISH_ERROR_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event ERROR will publish to. */
#define TWIM_PUBLISH_ERROR_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TWIM_PUBLISH_ERROR_CHIDX_Msk (0xFUL << TWIM_PUBLISH_ERROR_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TWIM_PUBLISH_SUSPENDED */
/* Description: Publish configuration for event SUSPENDED */

/* Bit 31 :   */
#define TWIM_PUBLISH_SUSPENDED_EN_Pos (31UL) /*!< Position of EN field. */
#define TWIM_PUBLISH_SUSPENDED_EN_Msk (0x1UL << TWIM_PUBLISH_SUSPENDED_EN_Pos) /*!< Bit mask of EN field. */
#define TWIM_PUBLISH_SUSPENDED_EN_Disabled (0UL) /*!< Disable publishing */
#define TWIM_PUBLISH_SUSPENDED_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event SUSPENDED will publish to. */
#define TWIM_PUBLISH_SUSPENDED_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TWIM_PUBLISH_SUSPENDED_CHIDX_Msk (0xFUL << TWIM_PUBLISH_SUSPENDED_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TWIM_PUBLISH_RXSTARTED */
/* Description: Publish configuration for event RXSTARTED */

/* Bit 31 :   */
#define TWIM_PUBLISH_RXSTARTED_EN_Pos (31UL) /*!< Position of EN field. */
#define TWIM_PUBLISH_RXSTARTED_EN_Msk (0x1UL << TWIM_PUBLISH_RXSTARTED_EN_Pos) /*!< Bit mask of EN field. */
#define TWIM_PUBLISH_RXSTARTED_EN_Disabled (0UL) /*!< Disable publishing */
#define TWIM_PUBLISH_RXSTARTED_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event RXSTARTED will publish to. */
#define TWIM_PUBLISH_RXSTARTED_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TWIM_PUBLISH_RXSTARTED_CHIDX_Msk (0xFUL << TWIM_PUBLISH_RXSTARTED_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TWIM_PUBLISH_TXSTARTED */
/* Description: Publish configuration for event TXSTARTED */

/* Bit 31 :   */
#define TWIM_PUBLISH_TXSTARTED_EN_Pos (31UL) /*!< Position of EN field. */
#define TWIM_PUBLISH_TXSTARTED_EN_Msk (0x1UL << TWIM_PUBLISH_TXSTARTED_EN_Pos) /*!< Bit mask of EN field. */
#define TWIM_PUBLISH_TXSTARTED_EN_Disabled (0UL) /*!< Disable publishing */
#define TWIM_PUBLISH_TXSTARTED_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event TXSTARTED will publish to. */
#define TWIM_PUBLISH_TXSTARTED_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TWIM_PUBLISH_TXSTARTED_CHIDX_Msk (0xFUL << TWIM_PUBLISH_TXSTARTED_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TWIM_PUBLISH_LASTRX */
/* Description: Publish configuration for event LASTRX */

/* Bit 31 :   */
#define TWIM_PUBLISH_LASTRX_EN_Pos (31UL) /*!< Position of EN field. */
#define TWIM_PUBLISH_LASTRX_EN_Msk (0x1UL << TWIM_PUBLISH_LASTRX_EN_Pos) /*!< Bit mask of EN field. */
#define TWIM_PUBLISH_LASTRX_EN_Disabled (0UL) /*!< Disable publishing */
#define TWIM_PUBLISH_LASTRX_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event LASTRX will publish to. */
#define TWIM_PUBLISH_LASTRX_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TWIM_PUBLISH_LASTRX_CHIDX_Msk (0xFUL << TWIM_PUBLISH_LASTRX_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TWIM_PUBLISH_LASTTX */
/* Description: Publish configuration for event LASTTX */

/* Bit 31 :   */
#define TWIM_PUBLISH_LASTTX_EN_Pos (31UL) /*!< Position of EN field. */
#define TWIM_PUBLISH_LASTTX_EN_Msk (0x1UL << TWIM_PUBLISH_LASTTX_EN_Pos) /*!< Bit mask of EN field. */
#define TWIM_PUBLISH_LASTTX_EN_Disabled (0UL) /*!< Disable publishing */
#define TWIM_PUBLISH_LASTTX_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event LASTTX will publish to. */
#define TWIM_PUBLISH_LASTTX_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TWIM_PUBLISH_LASTTX_CHIDX_Msk (0xFUL << TWIM_PUBLISH_LASTTX_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TWIM_SHORTS */
/* Description: Shortcuts between local events and tasks */

/* Bit 12 : Shortcut between event LASTRX and task STOP */
#define TWIM_SHORTS_LASTRX_STOP_Pos (12UL) /*!< Position of LASTRX_STOP field. */
#define TWIM_SHORTS_LASTRX_STOP_Msk (0x1UL << TWIM_SHORTS_LASTRX_STOP_Pos) /*!< Bit mask of LASTRX_STOP field. */
#define TWIM_SHORTS_LASTRX_STOP_Disabled (0UL) /*!< Disable shortcut */
#define TWIM_SHORTS_LASTRX_STOP_Enabled (1UL) /*!< Enable shortcut */

/* Bit 11 : Shortcut between event LASTRX and task SUSPEND */
#define TWIM_SHORTS_LASTRX_SUSPEND_Pos (11UL) /*!< Position of LASTRX_SUSPEND field. */
#define TWIM_SHORTS_LASTRX_SUSPEND_Msk (0x1UL << TWIM_SHORTS_LASTRX_SUSPEND_Pos) /*!< Bit mask of LASTRX_SUSPEND field. */
#define TWIM_SHORTS_LASTRX_SUSPEND_Disabled (0UL) /*!< Disable shortcut */
#define TWIM_SHORTS_LASTRX_SUSPEND_Enabled (1UL) /*!< Enable shortcut */

/* Bit 10 : Shortcut between event LASTRX and task STARTTX */
#define TWIM_SHORTS_LASTRX_STARTTX_Pos (10UL) /*!< Position of LASTRX_STARTTX field. */
#define TWIM_SHORTS_LASTRX_STARTTX_Msk (0x1UL << TWIM_SHORTS_LASTRX_STARTTX_Pos) /*!< Bit mask of LASTRX_STARTTX field. */
#define TWIM_SHORTS_LASTRX_STARTTX_Disabled (0UL) /*!< Disable shortcut */
#define TWIM_SHORTS_LASTRX_STARTTX_Enabled (1UL) /*!< Enable shortcut */

/* Bit 9 : Shortcut between event LASTTX and task STOP */
#define TWIM_SHORTS_LASTTX_STOP_Pos (9UL) /*!< Position of LASTTX_STOP field. */
#define TWIM_SHORTS_LASTTX_STOP_Msk (0x1UL << TWIM_SHORTS_LASTTX_STOP_Pos) /*!< Bit mask of LASTTX_STOP field. */
#define TWIM_SHORTS_LASTTX_STOP_Disabled (0UL) /*!< Disable shortcut */
#define TWIM_SHORTS_LASTTX_STOP_Enabled (1UL) /*!< Enable shortcut */

/* Bit 8 : Shortcut between event LASTTX and task SUSPEND */
#define TWIM_SHORTS_LASTTX_SUSPEND_Pos (8UL) /*!< Position of LASTTX_SUSPEND field. */
#define TWIM_SHORTS_LASTTX_SUSPEND_Msk (0x1UL << TWIM_SHORTS_LASTTX_SUSPEND_Pos) /*!< Bit mask of LASTTX_SUSPEND field. */
#define TWIM_SHORTS_LASTTX_SUSPEND_Disabled (0UL) /*!< Disable shortcut */
#define TWIM_SHORTS_LASTTX_SUSPEND_Enabled (1UL) /*!< Enable shortcut */

/* Bit 7 : Shortcut between event LASTTX and task STARTRX */
#define TWIM_SHORTS_LASTTX_STARTRX_Pos (7UL) /*!< Position of LASTTX_STARTRX field. */
#define TWIM_SHORTS_LASTTX_STARTRX_Msk (0x1UL << TWIM_SHORTS_LASTTX_STARTRX_Pos) /*!< Bit mask of LASTTX_STARTRX field. */
#define TWIM_SHORTS_LASTTX_STARTRX_Disabled (0UL) /*!< Disable shortcut */
#define TWIM_SHORTS_LASTTX_STARTRX_Enabled (1UL) /*!< Enable shortcut */

/* Register: TWIM_INTEN */
/* Description: Enable or disable interrupt */

/* Bit 24 : Enable or disable interrupt for event LASTTX */
#define TWIM_INTEN_LASTTX_Pos (24UL) /*!< Position of LASTTX field. */
#define TWIM_INTEN_LASTTX_Msk (0x1UL << TWIM_INTEN_LASTTX_Pos) /*!< Bit mask of LASTTX field. */
#define TWIM_INTEN_LASTTX_Disabled (0UL) /*!< Disable */
#define TWIM_INTEN_LASTTX_Enabled (1UL) /*!< Enable */

/* Bit 23 : Enable or disable interrupt for event LASTRX */
#define TWIM_INTEN_LASTRX_Pos (23UL) /*!< Position of LASTRX field. */
#define TWIM_INTEN_LASTRX_Msk (0x1UL << TWIM_INTEN_LASTRX_Pos) /*!< Bit mask of LASTRX field. */
#define TWIM_INTEN_LASTRX_Disabled (0UL) /*!< Disable */
#define TWIM_INTEN_LASTRX_Enabled (1UL) /*!< Enable */

/* Bit 20 : Enable or disable interrupt for event TXSTARTED */
#define TWIM_INTEN_TXSTARTED_Pos (20UL) /*!< Position of TXSTARTED field. */
#define TWIM_INTEN_TXSTARTED_Msk (0x1UL << TWIM_INTEN_TXSTARTED_Pos) /*!< Bit mask of TXSTARTED field. */
#define TWIM_INTEN_TXSTARTED_Disabled (0UL) /*!< Disable */
#define TWIM_INTEN_TXSTARTED_Enabled (1UL) /*!< Enable */

/* Bit 19 : Enable or disable interrupt for event RXSTARTED */
#define TWIM_INTEN_RXSTARTED_Pos (19UL) /*!< Position of RXSTARTED field. */
#define TWIM_INTEN_RXSTARTED_Msk (0x1UL << TWIM_INTEN_RXSTARTED_Pos) /*!< Bit mask of RXSTARTED field. */
#define TWIM_INTEN_RXSTARTED_Disabled (0UL) /*!< Disable */
#define TWIM_INTEN_RXSTARTED_Enabled (1UL) /*!< Enable */

/* Bit 18 : Enable or disable interrupt for event SUSPENDED */
#define TWIM_INTEN_SUSPENDED_Pos (18UL) /*!< Position of SUSPENDED field. */
#define TWIM_INTEN_SUSPENDED_Msk (0x1UL << TWIM_INTEN_SUSPENDED_Pos) /*!< Bit mask of SUSPENDED field. */
#define TWIM_INTEN_SUSPENDED_Disabled (0UL) /*!< Disable */
#define TWIM_INTEN_SUSPENDED_Enabled (1UL) /*!< Enable */

/* Bit 9 : Enable or disable interrupt for event ERROR */
#define TWIM_INTEN_ERROR_Pos (9UL) /*!< Position of ERROR field. */
#define TWIM_INTEN_ERROR_Msk (0x1UL << TWIM_INTEN_ERROR_Pos) /*!< Bit mask of ERROR field. */
#define TWIM_INTEN_ERROR_Disabled (0UL) /*!< Disable */
#define TWIM_INTEN_ERROR_Enabled (1UL) /*!< Enable */

/* Bit 1 : Enable or disable interrupt for event STOPPED */
#define TWIM_INTEN_STOPPED_Pos (1UL) /*!< Position of STOPPED field. */
#define TWIM_INTEN_STOPPED_Msk (0x1UL << TWIM_INTEN_STOPPED_Pos) /*!< Bit mask of STOPPED field. */
#define TWIM_INTEN_STOPPED_Disabled (0UL) /*!< Disable */
#define TWIM_INTEN_STOPPED_Enabled (1UL) /*!< Enable */

/* Register: TWIM_INTENSET */
/* Description: Enable interrupt */

/* Bit 24 : Write '1' to enable interrupt for event LASTTX */
#define TWIM_INTENSET_LASTTX_Pos (24UL) /*!< Position of LASTTX field. */
#define TWIM_INTENSET_LASTTX_Msk (0x1UL << TWIM_INTENSET_LASTTX_Pos) /*!< Bit mask of LASTTX field. */
#define TWIM_INTENSET_LASTTX_Disabled (0UL) /*!< Read: Disabled */
#define TWIM_INTENSET_LASTTX_Enabled (1UL) /*!< Read: Enabled */
#define TWIM_INTENSET_LASTTX_Set (1UL) /*!< Enable */

/* Bit 23 : Write '1' to enable interrupt for event LASTRX */
#define TWIM_INTENSET_LASTRX_Pos (23UL) /*!< Position of LASTRX field. */
#define TWIM_INTENSET_LASTRX_Msk (0x1UL << TWIM_INTENSET_LASTRX_Pos) /*!< Bit mask of LASTRX field. */
#define TWIM_INTENSET_LASTRX_Disabled (0UL) /*!< Read: Disabled */
#define TWIM_INTENSET_LASTRX_Enabled (1UL) /*!< Read: Enabled */
#define TWIM_INTENSET_LASTRX_Set (1UL) /*!< Enable */

/* Bit 20 : Write '1' to enable interrupt for event TXSTARTED */
#define TWIM_INTENSET_TXSTARTED_Pos (20UL) /*!< Position of TXSTARTED field. */
#define TWIM_INTENSET_TXSTARTED_Msk (0x1UL << TWIM_INTENSET_TXSTARTED_Pos) /*!< Bit mask of TXSTARTED field. */
#define TWIM_INTENSET_TXSTARTED_Disabled (0UL) /*!< Read: Disabled */
#define TWIM_INTENSET_TXSTARTED_Enabled (1UL) /*!< Read: Enabled */
#define TWIM_INTENSET_TXSTARTED_Set (1UL) /*!< Enable */

/* Bit 19 : Write '1' to enable interrupt for event RXSTARTED */
#define TWIM_INTENSET_RXSTARTED_Pos (19UL) /*!< Position of RXSTARTED field. */
#define TWIM_INTENSET_RXSTARTED_Msk (0x1UL << TWIM_INTENSET_RXSTARTED_Pos) /*!< Bit mask of RXSTARTED field. */
#define TWIM_INTENSET_RXSTARTED_Disabled (0UL) /*!< Read: Disabled */
#define TWIM_INTENSET_RXSTARTED_Enabled (1UL) /*!< Read: Enabled */
#define TWIM_INTENSET_RXSTARTED_Set (1UL) /*!< Enable */

/* Bit 18 : Write '1' to enable interrupt for event SUSPENDED */
#define TWIM_INTENSET_SUSPENDED_Pos (18UL) /*!< Position of SUSPENDED field. */
#define TWIM_INTENSET_SUSPENDED_Msk (0x1UL << TWIM_INTENSET_SUSPENDED_Pos) /*!< Bit mask of SUSPENDED field. */
#define TWIM_INTENSET_SUSPENDED_Disabled (0UL) /*!< Read: Disabled */
#define TWIM_INTENSET_SUSPENDED_Enabled (1UL) /*!< Read: Enabled */
#define TWIM_INTENSET_SUSPENDED_Set (1UL) /*!< Enable */

/* Bit 9 : Write '1' to enable interrupt for event ERROR */
#define TWIM_INTENSET_ERROR_Pos (9UL) /*!< Position of ERROR field. */
#define TWIM_INTENSET_ERROR_Msk (0x1UL << TWIM_INTENSET_ERROR_Pos) /*!< Bit mask of ERROR field. */
#define TWIM_INTENSET_ERROR_Disabled (0UL) /*!< Read: Disabled */
#define TWIM_INTENSET_ERROR_Enabled (1UL) /*!< Read: Enabled */
#define TWIM_INTENSET_ERROR_Set (1UL) /*!< Enable */

/* Bit 1 : Write '1' to enable interrupt for event STOPPED */
#define TWIM_INTENSET_STOPPED_Pos (1UL) /*!< Position of STOPPED field. */
#define TWIM_INTENSET_STOPPED_Msk (0x1UL << TWIM_INTENSET_STOPPED_Pos) /*!< Bit mask of STOPPED field. */
#define TWIM_INTENSET_STOPPED_Disabled (0UL) /*!< Read: Disabled */
#define TWIM_INTENSET_STOPPED_Enabled (1UL) /*!< Read: Enabled */
#define TWIM_INTENSET_STOPPED_Set (1UL) /*!< Enable */

/* Register: TWIM_INTENCLR */
/* Description: Disable interrupt */

/* Bit 24 : Write '1' to disable interrupt for event LASTTX */
#define TWIM_INTENCLR_LASTTX_Pos (24UL) /*!< Position of LASTTX field. */
#define TWIM_INTENCLR_LASTTX_Msk (0x1UL << TWIM_INTENCLR_LASTTX_Pos) /*!< Bit mask of LASTTX field. */
#define TWIM_INTENCLR_LASTTX_Disabled (0UL) /*!< Read: Disabled */
#define TWIM_INTENCLR_LASTTX_Enabled (1UL) /*!< Read: Enabled */
#define TWIM_INTENCLR_LASTTX_Clear (1UL) /*!< Disable */

/* Bit 23 : Write '1' to disable interrupt for event LASTRX */
#define TWIM_INTENCLR_LASTRX_Pos (23UL) /*!< Position of LASTRX field. */
#define TWIM_INTENCLR_LASTRX_Msk (0x1UL << TWIM_INTENCLR_LASTRX_Pos) /*!< Bit mask of LASTRX field. */
#define TWIM_INTENCLR_LASTRX_Disabled (0UL) /*!< Read: Disabled */
#define TWIM_INTENCLR_LASTRX_Enabled (1UL) /*!< Read: Enabled */
#define TWIM_INTENCLR_LASTRX_Clear (1UL) /*!< Disable */

/* Bit 20 : Write '1' to disable interrupt for event TXSTARTED */
#define TWIM_INTENCLR_TXSTARTED_Pos (20UL) /*!< Position of TXSTARTED field. */
#define TWIM_INTENCLR_TXSTARTED_Msk (0x1UL << TWIM_INTENCLR_TXSTARTED_Pos) /*!< Bit mask of TXSTARTED field. */
#define TWIM_INTENCLR_TXSTARTED_Disabled (0UL) /*!< Read: Disabled */
#define TWIM_INTENCLR_TXSTARTED_Enabled (1UL) /*!< Read: Enabled */
#define TWIM_INTENCLR_TXSTARTED_Clear (1UL) /*!< Disable */

/* Bit 19 : Write '1' to disable interrupt for event RXSTARTED */
#define TWIM_INTENCLR_RXSTARTED_Pos (19UL) /*!< Position of RXSTARTED field. */
#define TWIM_INTENCLR_RXSTARTED_Msk (0x1UL << TWIM_INTENCLR_RXSTARTED_Pos) /*!< Bit mask of RXSTARTED field. */
#define TWIM_INTENCLR_RXSTARTED_Disabled (0UL) /*!< Read: Disabled */
#define TWIM_INTENCLR_RXSTARTED_Enabled (1UL) /*!< Read: Enabled */
#define TWIM_INTENCLR_RXSTARTED_Clear (1UL) /*!< Disable */

/* Bit 18 : Write '1' to disable interrupt for event SUSPENDED */
#define TWIM_INTENCLR_SUSPENDED_Pos (18UL) /*!< Position of SUSPENDED field. */
#define TWIM_INTENCLR_SUSPENDED_Msk (0x1UL << TWIM_INTENCLR_SUSPENDED_Pos) /*!< Bit mask of SUSPENDED field. */
#define TWIM_INTENCLR_SUSPENDED_Disabled (0UL) /*!< Read: Disabled */
#define TWIM_INTENCLR_SUSPENDED_Enabled (1UL) /*!< Read: Enabled */
#define TWIM_INTENCLR_SUSPENDED_Clear (1UL) /*!< Disable */

/* Bit 9 : Write '1' to disable interrupt for event ERROR */
#define TWIM_INTENCLR_ERROR_Pos (9UL) /*!< Position of ERROR field. */
#define TWIM_INTENCLR_ERROR_Msk (0x1UL << TWIM_INTENCLR_ERROR_Pos) /*!< Bit mask of ERROR field. */
#define TWIM_INTENCLR_ERROR_Disabled (0UL) /*!< Read: Disabled */
#define TWIM_INTENCLR_ERROR_Enabled (1UL) /*!< Read: Enabled */
#define TWIM_INTENCLR_ERROR_Clear (1UL) /*!< Disable */

/* Bit 1 : Write '1' to disable interrupt for event STOPPED */
#define TWIM_INTENCLR_STOPPED_Pos (1UL) /*!< Position of STOPPED field. */
#define TWIM_INTENCLR_STOPPED_Msk (0x1UL << TWIM_INTENCLR_STOPPED_Pos) /*!< Bit mask of STOPPED field. */
#define TWIM_INTENCLR_STOPPED_Disabled (0UL) /*!< Read: Disabled */
#define TWIM_INTENCLR_STOPPED_Enabled (1UL) /*!< Read: Enabled */
#define TWIM_INTENCLR_STOPPED_Clear (1UL) /*!< Disable */

/* Register: TWIM_ERRORSRC */
/* Description: Error source */

/* Bit 2 : NACK received after sending a data byte (write '1' to clear) */
#define TWIM_ERRORSRC_DNACK_Pos (2UL) /*!< Position of DNACK field. */
#define TWIM_ERRORSRC_DNACK_Msk (0x1UL << TWIM_ERRORSRC_DNACK_Pos) /*!< Bit mask of DNACK field. */
#define TWIM_ERRORSRC_DNACK_NotReceived (0UL) /*!< Error did not occur */
#define TWIM_ERRORSRC_DNACK_Received (1UL) /*!< Error occurred */

/* Bit 1 : NACK received after sending the address (write '1' to clear) */
#define TWIM_ERRORSRC_ANACK_Pos (1UL) /*!< Position of ANACK field. */
#define TWIM_ERRORSRC_ANACK_Msk (0x1UL << TWIM_ERRORSRC_ANACK_Pos) /*!< Bit mask of ANACK field. */
#define TWIM_ERRORSRC_ANACK_NotReceived (0UL) /*!< Error did not occur */
#define TWIM_ERRORSRC_ANACK_Received (1UL) /*!< Error occurred */

/* Bit 0 : Overrun error */
#define TWIM_ERRORSRC_OVERRUN_Pos (0UL) /*!< Position of OVERRUN field. */
#define TWIM_ERRORSRC_OVERRUN_Msk (0x1UL << TWIM_ERRORSRC_OVERRUN_Pos) /*!< Bit mask of OVERRUN field. */
#define TWIM_ERRORSRC_OVERRUN_NotReceived (0UL) /*!< Error did not occur */
#define TWIM_ERRORSRC_OVERRUN_Received (1UL) /*!< Error occurred */

/* Register: TWIM_ENABLE */
/* Description: Enable TWIM */

/* Bits 3..0 : Enable or disable TWIM */
#define TWIM_ENABLE_ENABLE_Pos (0UL) /*!< Position of ENABLE field. */
#define TWIM_ENABLE_ENABLE_Msk (0xFUL << TWIM_ENABLE_ENABLE_Pos) /*!< Bit mask of ENABLE field. */
#define TWIM_ENABLE_ENABLE_Disabled (0UL) /*!< Disable TWIM */
#define TWIM_ENABLE_ENABLE_Enabled (6UL) /*!< Enable TWIM */

/* Register: TWIM_PSEL_SCL */
/* Description: Pin select for SCL signal */

/* Bit 31 : Connection */
#define TWIM_PSEL_SCL_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define TWIM_PSEL_SCL_CONNECT_Msk (0x1UL << TWIM_PSEL_SCL_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define TWIM_PSEL_SCL_CONNECT_Connected (0UL) /*!< Connect */
#define TWIM_PSEL_SCL_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define TWIM_PSEL_SCL_PIN_Pos (0UL) /*!< Position of PIN field. */
#define TWIM_PSEL_SCL_PIN_Msk (0x1FUL << TWIM_PSEL_SCL_PIN_Pos) /*!< Bit mask of PIN field. */

/* Register: TWIM_PSEL_SDA */
/* Description: Pin select for SDA signal */

/* Bit 31 : Connection */
#define TWIM_PSEL_SDA_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define TWIM_PSEL_SDA_CONNECT_Msk (0x1UL << TWIM_PSEL_SDA_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define TWIM_PSEL_SDA_CONNECT_Connected (0UL) /*!< Connect */
#define TWIM_PSEL_SDA_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define TWIM_PSEL_SDA_PIN_Pos (0UL) /*!< Position of PIN field. */
#define TWIM_PSEL_SDA_PIN_Msk (0x1FUL << TWIM_PSEL_SDA_PIN_Pos) /*!< Bit mask of PIN field. */

/* Register: TWIM_FREQUENCY */
/* Description: TWI frequency. Accuracy depends on the HFCLK source selected. */

/* Bits 31..0 : TWI master clock frequency */
#define TWIM_FREQUENCY_FREQUENCY_Pos (0UL) /*!< Position of FREQUENCY field. */
#define TWIM_FREQUENCY_FREQUENCY_Msk (0xFFFFFFFFUL << TWIM_FREQUENCY_FREQUENCY_Pos) /*!< Bit mask of FREQUENCY field. */
#define TWIM_FREQUENCY_FREQUENCY_K100 (0x01980000UL) /*!< 100 kbps */
#define TWIM_FREQUENCY_FREQUENCY_K250 (0x04000000UL) /*!< 250 kbps */
#define TWIM_FREQUENCY_FREQUENCY_K400 (0x06400000UL) /*!< 400 kbps */

/* Register: TWIM_RXD_PTR */
/* Description: Data pointer */

/* Bits 31..0 : Data pointer */
#define TWIM_RXD_PTR_PTR_Pos (0UL) /*!< Position of PTR field. */
#define TWIM_RXD_PTR_PTR_Msk (0xFFFFFFFFUL << TWIM_RXD_PTR_PTR_Pos) /*!< Bit mask of PTR field. */

/* Register: TWIM_RXD_MAXCNT */
/* Description: Maximum number of bytes in receive buffer */

/* Bits 12..0 : Maximum number of bytes in receive buffer */
#define TWIM_RXD_MAXCNT_MAXCNT_Pos (0UL) /*!< Position of MAXCNT field. */
#define TWIM_RXD_MAXCNT_MAXCNT_Msk (0x1FFFUL << TWIM_RXD_MAXCNT_MAXCNT_Pos) /*!< Bit mask of MAXCNT field. */

/* Register: TWIM_RXD_AMOUNT */
/* Description: Number of bytes transferred in the last transaction */

/* Bits 12..0 : Number of bytes transferred in the last transaction. In case of NACK error, includes the NACK'ed byte. */
#define TWIM_RXD_AMOUNT_AMOUNT_Pos (0UL) /*!< Position of AMOUNT field. */
#define TWIM_RXD_AMOUNT_AMOUNT_Msk (0x1FFFUL << TWIM_RXD_AMOUNT_AMOUNT_Pos) /*!< Bit mask of AMOUNT field. */

/* Register: TWIM_RXD_LIST */
/* Description: EasyDMA list type */

/* Bits 1..0 : List type */
#define TWIM_RXD_LIST_LIST_Pos (0UL) /*!< Position of LIST field. */
#define TWIM_RXD_LIST_LIST_Msk (0x3UL << TWIM_RXD_LIST_LIST_Pos) /*!< Bit mask of LIST field. */
#define TWIM_RXD_LIST_LIST_Disabled (0UL) /*!< Disable EasyDMA list */
#define TWIM_RXD_LIST_LIST_ArrayList (1UL) /*!< Use array list */

/* Register: TWIM_TXD_PTR */
/* Description: Data pointer */

/* Bits 31..0 : Data pointer */
#define TWIM_TXD_PTR_PTR_Pos (0UL) /*!< Position of PTR field. */
#define TWIM_TXD_PTR_PTR_Msk (0xFFFFFFFFUL << TWIM_TXD_PTR_PTR_Pos) /*!< Bit mask of PTR field. */

/* Register: TWIM_TXD_MAXCNT */
/* Description: Maximum number of bytes in transmit buffer */

/* Bits 12..0 : Maximum number of bytes in transmit buffer */
#define TWIM_TXD_MAXCNT_MAXCNT_Pos (0UL) /*!< Position of MAXCNT field. */
#define TWIM_TXD_MAXCNT_MAXCNT_Msk (0x1FFFUL << TWIM_TXD_MAXCNT_MAXCNT_Pos) /*!< Bit mask of MAXCNT field. */

/* Register: TWIM_TXD_AMOUNT */
/* Description: Number of bytes transferred in the last transaction */

/* Bits 12..0 : Number of bytes transferred in the last transaction. In case of NACK error, includes the NACK'ed byte. */
#define TWIM_TXD_AMOUNT_AMOUNT_Pos (0UL) /*!< Position of AMOUNT field. */
#define TWIM_TXD_AMOUNT_AMOUNT_Msk (0x1FFFUL << TWIM_TXD_AMOUNT_AMOUNT_Pos) /*!< Bit mask of AMOUNT field. */

/* Register: TWIM_TXD_LIST */
/* Description: EasyDMA list type */

/* Bits 1..0 : List type */
#define TWIM_TXD_LIST_LIST_Pos (0UL) /*!< Position of LIST field. */
#define TWIM_TXD_LIST_LIST_Msk (0x3UL << TWIM_TXD_LIST_LIST_Pos) /*!< Bit mask of LIST field. */
#define TWIM_TXD_LIST_LIST_Disabled (0UL) /*!< Disable EasyDMA list */
#define TWIM_TXD_LIST_LIST_ArrayList (1UL) /*!< Use array list */

/* Register: TWIM_ADDRESS */
/* Description: Address used in the TWI transfer */

/* Bits 6..0 : Address used in the TWI transfer */
#define TWIM_ADDRESS_ADDRESS_Pos (0UL) /*!< Position of ADDRESS field. */
#define TWIM_ADDRESS_ADDRESS_Msk (0x7FUL << TWIM_ADDRESS_ADDRESS_Pos) /*!< Bit mask of ADDRESS field. */


/* Peripheral: TWIS */
/* Description: I2C compatible Two-Wire Slave Interface with EasyDMA 0 */

/* Register: TWIS_TASKS_STOP */
/* Description: Stop TWI transaction */

/* Bit 0 : Stop TWI transaction */
#define TWIS_TASKS_STOP_TASKS_STOP_Pos (0UL) /*!< Position of TASKS_STOP field. */
#define TWIS_TASKS_STOP_TASKS_STOP_Msk (0x1UL << TWIS_TASKS_STOP_TASKS_STOP_Pos) /*!< Bit mask of TASKS_STOP field. */
#define TWIS_TASKS_STOP_TASKS_STOP_Trigger (1UL) /*!< Trigger task */

/* Register: TWIS_TASKS_SUSPEND */
/* Description: Suspend TWI transaction */

/* Bit 0 : Suspend TWI transaction */
#define TWIS_TASKS_SUSPEND_TASKS_SUSPEND_Pos (0UL) /*!< Position of TASKS_SUSPEND field. */
#define TWIS_TASKS_SUSPEND_TASKS_SUSPEND_Msk (0x1UL << TWIS_TASKS_SUSPEND_TASKS_SUSPEND_Pos) /*!< Bit mask of TASKS_SUSPEND field. */
#define TWIS_TASKS_SUSPEND_TASKS_SUSPEND_Trigger (1UL) /*!< Trigger task */

/* Register: TWIS_TASKS_RESUME */
/* Description: Resume TWI transaction */

/* Bit 0 : Resume TWI transaction */
#define TWIS_TASKS_RESUME_TASKS_RESUME_Pos (0UL) /*!< Position of TASKS_RESUME field. */
#define TWIS_TASKS_RESUME_TASKS_RESUME_Msk (0x1UL << TWIS_TASKS_RESUME_TASKS_RESUME_Pos) /*!< Bit mask of TASKS_RESUME field. */
#define TWIS_TASKS_RESUME_TASKS_RESUME_Trigger (1UL) /*!< Trigger task */

/* Register: TWIS_TASKS_PREPARERX */
/* Description: Prepare the TWI slave to respond to a write command */

/* Bit 0 : Prepare the TWI slave to respond to a write command */
#define TWIS_TASKS_PREPARERX_TASKS_PREPARERX_Pos (0UL) /*!< Position of TASKS_PREPARERX field. */
#define TWIS_TASKS_PREPARERX_TASKS_PREPARERX_Msk (0x1UL << TWIS_TASKS_PREPARERX_TASKS_PREPARERX_Pos) /*!< Bit mask of TASKS_PREPARERX field. */
#define TWIS_TASKS_PREPARERX_TASKS_PREPARERX_Trigger (1UL) /*!< Trigger task */

/* Register: TWIS_TASKS_PREPARETX */
/* Description: Prepare the TWI slave to respond to a read command */

/* Bit 0 : Prepare the TWI slave to respond to a read command */
#define TWIS_TASKS_PREPARETX_TASKS_PREPARETX_Pos (0UL) /*!< Position of TASKS_PREPARETX field. */
#define TWIS_TASKS_PREPARETX_TASKS_PREPARETX_Msk (0x1UL << TWIS_TASKS_PREPARETX_TASKS_PREPARETX_Pos) /*!< Bit mask of TASKS_PREPARETX field. */
#define TWIS_TASKS_PREPARETX_TASKS_PREPARETX_Trigger (1UL) /*!< Trigger task */

/* Register: TWIS_SUBSCRIBE_STOP */
/* Description: Subscribe configuration for task STOP */

/* Bit 31 :   */
#define TWIS_SUBSCRIBE_STOP_EN_Pos (31UL) /*!< Position of EN field. */
#define TWIS_SUBSCRIBE_STOP_EN_Msk (0x1UL << TWIS_SUBSCRIBE_STOP_EN_Pos) /*!< Bit mask of EN field. */
#define TWIS_SUBSCRIBE_STOP_EN_Disabled (0UL) /*!< Disable subscription */
#define TWIS_SUBSCRIBE_STOP_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task STOP will subscribe to */
#define TWIS_SUBSCRIBE_STOP_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TWIS_SUBSCRIBE_STOP_CHIDX_Msk (0xFUL << TWIS_SUBSCRIBE_STOP_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TWIS_SUBSCRIBE_SUSPEND */
/* Description: Subscribe configuration for task SUSPEND */

/* Bit 31 :   */
#define TWIS_SUBSCRIBE_SUSPEND_EN_Pos (31UL) /*!< Position of EN field. */
#define TWIS_SUBSCRIBE_SUSPEND_EN_Msk (0x1UL << TWIS_SUBSCRIBE_SUSPEND_EN_Pos) /*!< Bit mask of EN field. */
#define TWIS_SUBSCRIBE_SUSPEND_EN_Disabled (0UL) /*!< Disable subscription */
#define TWIS_SUBSCRIBE_SUSPEND_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task SUSPEND will subscribe to */
#define TWIS_SUBSCRIBE_SUSPEND_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TWIS_SUBSCRIBE_SUSPEND_CHIDX_Msk (0xFUL << TWIS_SUBSCRIBE_SUSPEND_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TWIS_SUBSCRIBE_RESUME */
/* Description: Subscribe configuration for task RESUME */

/* Bit 31 :   */
#define TWIS_SUBSCRIBE_RESUME_EN_Pos (31UL) /*!< Position of EN field. */
#define TWIS_SUBSCRIBE_RESUME_EN_Msk (0x1UL << TWIS_SUBSCRIBE_RESUME_EN_Pos) /*!< Bit mask of EN field. */
#define TWIS_SUBSCRIBE_RESUME_EN_Disabled (0UL) /*!< Disable subscription */
#define TWIS_SUBSCRIBE_RESUME_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task RESUME will subscribe to */
#define TWIS_SUBSCRIBE_RESUME_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TWIS_SUBSCRIBE_RESUME_CHIDX_Msk (0xFUL << TWIS_SUBSCRIBE_RESUME_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TWIS_SUBSCRIBE_PREPARERX */
/* Description: Subscribe configuration for task PREPARERX */

/* Bit 31 :   */
#define TWIS_SUBSCRIBE_PREPARERX_EN_Pos (31UL) /*!< Position of EN field. */
#define TWIS_SUBSCRIBE_PREPARERX_EN_Msk (0x1UL << TWIS_SUBSCRIBE_PREPARERX_EN_Pos) /*!< Bit mask of EN field. */
#define TWIS_SUBSCRIBE_PREPARERX_EN_Disabled (0UL) /*!< Disable subscription */
#define TWIS_SUBSCRIBE_PREPARERX_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task PREPARERX will subscribe to */
#define TWIS_SUBSCRIBE_PREPARERX_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TWIS_SUBSCRIBE_PREPARERX_CHIDX_Msk (0xFUL << TWIS_SUBSCRIBE_PREPARERX_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TWIS_SUBSCRIBE_PREPARETX */
/* Description: Subscribe configuration for task PREPARETX */

/* Bit 31 :   */
#define TWIS_SUBSCRIBE_PREPARETX_EN_Pos (31UL) /*!< Position of EN field. */
#define TWIS_SUBSCRIBE_PREPARETX_EN_Msk (0x1UL << TWIS_SUBSCRIBE_PREPARETX_EN_Pos) /*!< Bit mask of EN field. */
#define TWIS_SUBSCRIBE_PREPARETX_EN_Disabled (0UL) /*!< Disable subscription */
#define TWIS_SUBSCRIBE_PREPARETX_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task PREPARETX will subscribe to */
#define TWIS_SUBSCRIBE_PREPARETX_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TWIS_SUBSCRIBE_PREPARETX_CHIDX_Msk (0xFUL << TWIS_SUBSCRIBE_PREPARETX_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TWIS_EVENTS_STOPPED */
/* Description: TWI stopped */

/* Bit 0 : TWI stopped */
#define TWIS_EVENTS_STOPPED_EVENTS_STOPPED_Pos (0UL) /*!< Position of EVENTS_STOPPED field. */
#define TWIS_EVENTS_STOPPED_EVENTS_STOPPED_Msk (0x1UL << TWIS_EVENTS_STOPPED_EVENTS_STOPPED_Pos) /*!< Bit mask of EVENTS_STOPPED field. */
#define TWIS_EVENTS_STOPPED_EVENTS_STOPPED_NotGenerated (0UL) /*!< Event not generated */
#define TWIS_EVENTS_STOPPED_EVENTS_STOPPED_Generated (1UL) /*!< Event generated */

/* Register: TWIS_EVENTS_ERROR */
/* Description: TWI error */

/* Bit 0 : TWI error */
#define TWIS_EVENTS_ERROR_EVENTS_ERROR_Pos (0UL) /*!< Position of EVENTS_ERROR field. */
#define TWIS_EVENTS_ERROR_EVENTS_ERROR_Msk (0x1UL << TWIS_EVENTS_ERROR_EVENTS_ERROR_Pos) /*!< Bit mask of EVENTS_ERROR field. */
#define TWIS_EVENTS_ERROR_EVENTS_ERROR_NotGenerated (0UL) /*!< Event not generated */
#define TWIS_EVENTS_ERROR_EVENTS_ERROR_Generated (1UL) /*!< Event generated */

/* Register: TWIS_EVENTS_RXSTARTED */
/* Description: Receive sequence started */

/* Bit 0 : Receive sequence started */
#define TWIS_EVENTS_RXSTARTED_EVENTS_RXSTARTED_Pos (0UL) /*!< Position of EVENTS_RXSTARTED field. */
#define TWIS_EVENTS_RXSTARTED_EVENTS_RXSTARTED_Msk (0x1UL << TWIS_EVENTS_RXSTARTED_EVENTS_RXSTARTED_Pos) /*!< Bit mask of EVENTS_RXSTARTED field. */
#define TWIS_EVENTS_RXSTARTED_EVENTS_RXSTARTED_NotGenerated (0UL) /*!< Event not generated */
#define TWIS_EVENTS_RXSTARTED_EVENTS_RXSTARTED_Generated (1UL) /*!< Event generated */

/* Register: TWIS_EVENTS_TXSTARTED */
/* Description: Transmit sequence started */

/* Bit 0 : Transmit sequence started */
#define TWIS_EVENTS_TXSTARTED_EVENTS_TXSTARTED_Pos (0UL) /*!< Position of EVENTS_TXSTARTED field. */
#define TWIS_EVENTS_TXSTARTED_EVENTS_TXSTARTED_Msk (0x1UL << TWIS_EVENTS_TXSTARTED_EVENTS_TXSTARTED_Pos) /*!< Bit mask of EVENTS_TXSTARTED field. */
#define TWIS_EVENTS_TXSTARTED_EVENTS_TXSTARTED_NotGenerated (0UL) /*!< Event not generated */
#define TWIS_EVENTS_TXSTARTED_EVENTS_TXSTARTED_Generated (1UL) /*!< Event generated */

/* Register: TWIS_EVENTS_WRITE */
/* Description: Write command received */

/* Bit 0 : Write command received */
#define TWIS_EVENTS_WRITE_EVENTS_WRITE_Pos (0UL) /*!< Position of EVENTS_WRITE field. */
#define TWIS_EVENTS_WRITE_EVENTS_WRITE_Msk (0x1UL << TWIS_EVENTS_WRITE_EVENTS_WRITE_Pos) /*!< Bit mask of EVENTS_WRITE field. */
#define TWIS_EVENTS_WRITE_EVENTS_WRITE_NotGenerated (0UL) /*!< Event not generated */
#define TWIS_EVENTS_WRITE_EVENTS_WRITE_Generated (1UL) /*!< Event generated */

/* Register: TWIS_EVENTS_READ */
/* Description: Read command received */

/* Bit 0 : Read command received */
#define TWIS_EVENTS_READ_EVENTS_READ_Pos (0UL) /*!< Position of EVENTS_READ field. */
#define TWIS_EVENTS_READ_EVENTS_READ_Msk (0x1UL << TWIS_EVENTS_READ_EVENTS_READ_Pos) /*!< Bit mask of EVENTS_READ field. */
#define TWIS_EVENTS_READ_EVENTS_READ_NotGenerated (0UL) /*!< Event not generated */
#define TWIS_EVENTS_READ_EVENTS_READ_Generated (1UL) /*!< Event generated */

/* Register: TWIS_PUBLISH_STOPPED */
/* Description: Publish configuration for event STOPPED */

/* Bit 31 :   */
#define TWIS_PUBLISH_STOPPED_EN_Pos (31UL) /*!< Position of EN field. */
#define TWIS_PUBLISH_STOPPED_EN_Msk (0x1UL << TWIS_PUBLISH_STOPPED_EN_Pos) /*!< Bit mask of EN field. */
#define TWIS_PUBLISH_STOPPED_EN_Disabled (0UL) /*!< Disable publishing */
#define TWIS_PUBLISH_STOPPED_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event STOPPED will publish to. */
#define TWIS_PUBLISH_STOPPED_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TWIS_PUBLISH_STOPPED_CHIDX_Msk (0xFUL << TWIS_PUBLISH_STOPPED_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TWIS_PUBLISH_ERROR */
/* Description: Publish configuration for event ERROR */

/* Bit 31 :   */
#define TWIS_PUBLISH_ERROR_EN_Pos (31UL) /*!< Position of EN field. */
#define TWIS_PUBLISH_ERROR_EN_Msk (0x1UL << TWIS_PUBLISH_ERROR_EN_Pos) /*!< Bit mask of EN field. */
#define TWIS_PUBLISH_ERROR_EN_Disabled (0UL) /*!< Disable publishing */
#define TWIS_PUBLISH_ERROR_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event ERROR will publish to. */
#define TWIS_PUBLISH_ERROR_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TWIS_PUBLISH_ERROR_CHIDX_Msk (0xFUL << TWIS_PUBLISH_ERROR_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TWIS_PUBLISH_RXSTARTED */
/* Description: Publish configuration for event RXSTARTED */

/* Bit 31 :   */
#define TWIS_PUBLISH_RXSTARTED_EN_Pos (31UL) /*!< Position of EN field. */
#define TWIS_PUBLISH_RXSTARTED_EN_Msk (0x1UL << TWIS_PUBLISH_RXSTARTED_EN_Pos) /*!< Bit mask of EN field. */
#define TWIS_PUBLISH_RXSTARTED_EN_Disabled (0UL) /*!< Disable publishing */
#define TWIS_PUBLISH_RXSTARTED_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event RXSTARTED will publish to. */
#define TWIS_PUBLISH_RXSTARTED_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TWIS_PUBLISH_RXSTARTED_CHIDX_Msk (0xFUL << TWIS_PUBLISH_RXSTARTED_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TWIS_PUBLISH_TXSTARTED */
/* Description: Publish configuration for event TXSTARTED */

/* Bit 31 :   */
#define TWIS_PUBLISH_TXSTARTED_EN_Pos (31UL) /*!< Position of EN field. */
#define TWIS_PUBLISH_TXSTARTED_EN_Msk (0x1UL << TWIS_PUBLISH_TXSTARTED_EN_Pos) /*!< Bit mask of EN field. */
#define TWIS_PUBLISH_TXSTARTED_EN_Disabled (0UL) /*!< Disable publishing */
#define TWIS_PUBLISH_TXSTARTED_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event TXSTARTED will publish to. */
#define TWIS_PUBLISH_TXSTARTED_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TWIS_PUBLISH_TXSTARTED_CHIDX_Msk (0xFUL << TWIS_PUBLISH_TXSTARTED_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TWIS_PUBLISH_WRITE */
/* Description: Publish configuration for event WRITE */

/* Bit 31 :   */
#define TWIS_PUBLISH_WRITE_EN_Pos (31UL) /*!< Position of EN field. */
#define TWIS_PUBLISH_WRITE_EN_Msk (0x1UL << TWIS_PUBLISH_WRITE_EN_Pos) /*!< Bit mask of EN field. */
#define TWIS_PUBLISH_WRITE_EN_Disabled (0UL) /*!< Disable publishing */
#define TWIS_PUBLISH_WRITE_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event WRITE will publish to. */
#define TWIS_PUBLISH_WRITE_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TWIS_PUBLISH_WRITE_CHIDX_Msk (0xFUL << TWIS_PUBLISH_WRITE_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TWIS_PUBLISH_READ */
/* Description: Publish configuration for event READ */

/* Bit 31 :   */
#define TWIS_PUBLISH_READ_EN_Pos (31UL) /*!< Position of EN field. */
#define TWIS_PUBLISH_READ_EN_Msk (0x1UL << TWIS_PUBLISH_READ_EN_Pos) /*!< Bit mask of EN field. */
#define TWIS_PUBLISH_READ_EN_Disabled (0UL) /*!< Disable publishing */
#define TWIS_PUBLISH_READ_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event READ will publish to. */
#define TWIS_PUBLISH_READ_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define TWIS_PUBLISH_READ_CHIDX_Msk (0xFUL << TWIS_PUBLISH_READ_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: TWIS_SHORTS */
/* Description: Shortcuts between local events and tasks */

/* Bit 14 : Shortcut between event READ and task SUSPEND */
#define TWIS_SHORTS_READ_SUSPEND_Pos (14UL) /*!< Position of READ_SUSPEND field. */
#define TWIS_SHORTS_READ_SUSPEND_Msk (0x1UL << TWIS_SHORTS_READ_SUSPEND_Pos) /*!< Bit mask of READ_SUSPEND field. */
#define TWIS_SHORTS_READ_SUSPEND_Disabled (0UL) /*!< Disable shortcut */
#define TWIS_SHORTS_READ_SUSPEND_Enabled (1UL) /*!< Enable shortcut */

/* Bit 13 : Shortcut between event WRITE and task SUSPEND */
#define TWIS_SHORTS_WRITE_SUSPEND_Pos (13UL) /*!< Position of WRITE_SUSPEND field. */
#define TWIS_SHORTS_WRITE_SUSPEND_Msk (0x1UL << TWIS_SHORTS_WRITE_SUSPEND_Pos) /*!< Bit mask of WRITE_SUSPEND field. */
#define TWIS_SHORTS_WRITE_SUSPEND_Disabled (0UL) /*!< Disable shortcut */
#define TWIS_SHORTS_WRITE_SUSPEND_Enabled (1UL) /*!< Enable shortcut */

/* Register: TWIS_INTEN */
/* Description: Enable or disable interrupt */

/* Bit 26 : Enable or disable interrupt for event READ */
#define TWIS_INTEN_READ_Pos (26UL) /*!< Position of READ field. */
#define TWIS_INTEN_READ_Msk (0x1UL << TWIS_INTEN_READ_Pos) /*!< Bit mask of READ field. */
#define TWIS_INTEN_READ_Disabled (0UL) /*!< Disable */
#define TWIS_INTEN_READ_Enabled (1UL) /*!< Enable */

/* Bit 25 : Enable or disable interrupt for event WRITE */
#define TWIS_INTEN_WRITE_Pos (25UL) /*!< Position of WRITE field. */
#define TWIS_INTEN_WRITE_Msk (0x1UL << TWIS_INTEN_WRITE_Pos) /*!< Bit mask of WRITE field. */
#define TWIS_INTEN_WRITE_Disabled (0UL) /*!< Disable */
#define TWIS_INTEN_WRITE_Enabled (1UL) /*!< Enable */

/* Bit 20 : Enable or disable interrupt for event TXSTARTED */
#define TWIS_INTEN_TXSTARTED_Pos (20UL) /*!< Position of TXSTARTED field. */
#define TWIS_INTEN_TXSTARTED_Msk (0x1UL << TWIS_INTEN_TXSTARTED_Pos) /*!< Bit mask of TXSTARTED field. */
#define TWIS_INTEN_TXSTARTED_Disabled (0UL) /*!< Disable */
#define TWIS_INTEN_TXSTARTED_Enabled (1UL) /*!< Enable */

/* Bit 19 : Enable or disable interrupt for event RXSTARTED */
#define TWIS_INTEN_RXSTARTED_Pos (19UL) /*!< Position of RXSTARTED field. */
#define TWIS_INTEN_RXSTARTED_Msk (0x1UL << TWIS_INTEN_RXSTARTED_Pos) /*!< Bit mask of RXSTARTED field. */
#define TWIS_INTEN_RXSTARTED_Disabled (0UL) /*!< Disable */
#define TWIS_INTEN_RXSTARTED_Enabled (1UL) /*!< Enable */

/* Bit 9 : Enable or disable interrupt for event ERROR */
#define TWIS_INTEN_ERROR_Pos (9UL) /*!< Position of ERROR field. */
#define TWIS_INTEN_ERROR_Msk (0x1UL << TWIS_INTEN_ERROR_Pos) /*!< Bit mask of ERROR field. */
#define TWIS_INTEN_ERROR_Disabled (0UL) /*!< Disable */
#define TWIS_INTEN_ERROR_Enabled (1UL) /*!< Enable */

/* Bit 1 : Enable or disable interrupt for event STOPPED */
#define TWIS_INTEN_STOPPED_Pos (1UL) /*!< Position of STOPPED field. */
#define TWIS_INTEN_STOPPED_Msk (0x1UL << TWIS_INTEN_STOPPED_Pos) /*!< Bit mask of STOPPED field. */
#define TWIS_INTEN_STOPPED_Disabled (0UL) /*!< Disable */
#define TWIS_INTEN_STOPPED_Enabled (1UL) /*!< Enable */

/* Register: TWIS_INTENSET */
/* Description: Enable interrupt */

/* Bit 26 : Write '1' to enable interrupt for event READ */
#define TWIS_INTENSET_READ_Pos (26UL) /*!< Position of READ field. */
#define TWIS_INTENSET_READ_Msk (0x1UL << TWIS_INTENSET_READ_Pos) /*!< Bit mask of READ field. */
#define TWIS_INTENSET_READ_Disabled (0UL) /*!< Read: Disabled */
#define TWIS_INTENSET_READ_Enabled (1UL) /*!< Read: Enabled */
#define TWIS_INTENSET_READ_Set (1UL) /*!< Enable */

/* Bit 25 : Write '1' to enable interrupt for event WRITE */
#define TWIS_INTENSET_WRITE_Pos (25UL) /*!< Position of WRITE field. */
#define TWIS_INTENSET_WRITE_Msk (0x1UL << TWIS_INTENSET_WRITE_Pos) /*!< Bit mask of WRITE field. */
#define TWIS_INTENSET_WRITE_Disabled (0UL) /*!< Read: Disabled */
#define TWIS_INTENSET_WRITE_Enabled (1UL) /*!< Read: Enabled */
#define TWIS_INTENSET_WRITE_Set (1UL) /*!< Enable */

/* Bit 20 : Write '1' to enable interrupt for event TXSTARTED */
#define TWIS_INTENSET_TXSTARTED_Pos (20UL) /*!< Position of TXSTARTED field. */
#define TWIS_INTENSET_TXSTARTED_Msk (0x1UL << TWIS_INTENSET_TXSTARTED_Pos) /*!< Bit mask of TXSTARTED field. */
#define TWIS_INTENSET_TXSTARTED_Disabled (0UL) /*!< Read: Disabled */
#define TWIS_INTENSET_TXSTARTED_Enabled (1UL) /*!< Read: Enabled */
#define TWIS_INTENSET_TXSTARTED_Set (1UL) /*!< Enable */

/* Bit 19 : Write '1' to enable interrupt for event RXSTARTED */
#define TWIS_INTENSET_RXSTARTED_Pos (19UL) /*!< Position of RXSTARTED field. */
#define TWIS_INTENSET_RXSTARTED_Msk (0x1UL << TWIS_INTENSET_RXSTARTED_Pos) /*!< Bit mask of RXSTARTED field. */
#define TWIS_INTENSET_RXSTARTED_Disabled (0UL) /*!< Read: Disabled */
#define TWIS_INTENSET_RXSTARTED_Enabled (1UL) /*!< Read: Enabled */
#define TWIS_INTENSET_RXSTARTED_Set (1UL) /*!< Enable */

/* Bit 9 : Write '1' to enable interrupt for event ERROR */
#define TWIS_INTENSET_ERROR_Pos (9UL) /*!< Position of ERROR field. */
#define TWIS_INTENSET_ERROR_Msk (0x1UL << TWIS_INTENSET_ERROR_Pos) /*!< Bit mask of ERROR field. */
#define TWIS_INTENSET_ERROR_Disabled (0UL) /*!< Read: Disabled */
#define TWIS_INTENSET_ERROR_Enabled (1UL) /*!< Read: Enabled */
#define TWIS_INTENSET_ERROR_Set (1UL) /*!< Enable */

/* Bit 1 : Write '1' to enable interrupt for event STOPPED */
#define TWIS_INTENSET_STOPPED_Pos (1UL) /*!< Position of STOPPED field. */
#define TWIS_INTENSET_STOPPED_Msk (0x1UL << TWIS_INTENSET_STOPPED_Pos) /*!< Bit mask of STOPPED field. */
#define TWIS_INTENSET_STOPPED_Disabled (0UL) /*!< Read: Disabled */
#define TWIS_INTENSET_STOPPED_Enabled (1UL) /*!< Read: Enabled */
#define TWIS_INTENSET_STOPPED_Set (1UL) /*!< Enable */

/* Register: TWIS_INTENCLR */
/* Description: Disable interrupt */

/* Bit 26 : Write '1' to disable interrupt for event READ */
#define TWIS_INTENCLR_READ_Pos (26UL) /*!< Position of READ field. */
#define TWIS_INTENCLR_READ_Msk (0x1UL << TWIS_INTENCLR_READ_Pos) /*!< Bit mask of READ field. */
#define TWIS_INTENCLR_READ_Disabled (0UL) /*!< Read: Disabled */
#define TWIS_INTENCLR_READ_Enabled (1UL) /*!< Read: Enabled */
#define TWIS_INTENCLR_READ_Clear (1UL) /*!< Disable */

/* Bit 25 : Write '1' to disable interrupt for event WRITE */
#define TWIS_INTENCLR_WRITE_Pos (25UL) /*!< Position of WRITE field. */
#define TWIS_INTENCLR_WRITE_Msk (0x1UL << TWIS_INTENCLR_WRITE_Pos) /*!< Bit mask of WRITE field. */
#define TWIS_INTENCLR_WRITE_Disabled (0UL) /*!< Read: Disabled */
#define TWIS_INTENCLR_WRITE_Enabled (1UL) /*!< Read: Enabled */
#define TWIS_INTENCLR_WRITE_Clear (1UL) /*!< Disable */

/* Bit 20 : Write '1' to disable interrupt for event TXSTARTED */
#define TWIS_INTENCLR_TXSTARTED_Pos (20UL) /*!< Position of TXSTARTED field. */
#define TWIS_INTENCLR_TXSTARTED_Msk (0x1UL << TWIS_INTENCLR_TXSTARTED_Pos) /*!< Bit mask of TXSTARTED field. */
#define TWIS_INTENCLR_TXSTARTED_Disabled (0UL) /*!< Read: Disabled */
#define TWIS_INTENCLR_TXSTARTED_Enabled (1UL) /*!< Read: Enabled */
#define TWIS_INTENCLR_TXSTARTED_Clear (1UL) /*!< Disable */

/* Bit 19 : Write '1' to disable interrupt for event RXSTARTED */
#define TWIS_INTENCLR_RXSTARTED_Pos (19UL) /*!< Position of RXSTARTED field. */
#define TWIS_INTENCLR_RXSTARTED_Msk (0x1UL << TWIS_INTENCLR_RXSTARTED_Pos) /*!< Bit mask of RXSTARTED field. */
#define TWIS_INTENCLR_RXSTARTED_Disabled (0UL) /*!< Read: Disabled */
#define TWIS_INTENCLR_RXSTARTED_Enabled (1UL) /*!< Read: Enabled */
#define TWIS_INTENCLR_RXSTARTED_Clear (1UL) /*!< Disable */

/* Bit 9 : Write '1' to disable interrupt for event ERROR */
#define TWIS_INTENCLR_ERROR_Pos (9UL) /*!< Position of ERROR field. */
#define TWIS_INTENCLR_ERROR_Msk (0x1UL << TWIS_INTENCLR_ERROR_Pos) /*!< Bit mask of ERROR field. */
#define TWIS_INTENCLR_ERROR_Disabled (0UL) /*!< Read: Disabled */
#define TWIS_INTENCLR_ERROR_Enabled (1UL) /*!< Read: Enabled */
#define TWIS_INTENCLR_ERROR_Clear (1UL) /*!< Disable */

/* Bit 1 : Write '1' to disable interrupt for event STOPPED */
#define TWIS_INTENCLR_STOPPED_Pos (1UL) /*!< Position of STOPPED field. */
#define TWIS_INTENCLR_STOPPED_Msk (0x1UL << TWIS_INTENCLR_STOPPED_Pos) /*!< Bit mask of STOPPED field. */
#define TWIS_INTENCLR_STOPPED_Disabled (0UL) /*!< Read: Disabled */
#define TWIS_INTENCLR_STOPPED_Enabled (1UL) /*!< Read: Enabled */
#define TWIS_INTENCLR_STOPPED_Clear (1UL) /*!< Disable */

/* Register: TWIS_ERRORSRC */
/* Description: Error source */

/* Bit 3 : TX buffer over-read detected, and prevented */
#define TWIS_ERRORSRC_OVERREAD_Pos (3UL) /*!< Position of OVERREAD field. */
#define TWIS_ERRORSRC_OVERREAD_Msk (0x1UL << TWIS_ERRORSRC_OVERREAD_Pos) /*!< Bit mask of OVERREAD field. */
#define TWIS_ERRORSRC_OVERREAD_NotDetected (0UL) /*!< Error did not occur */
#define TWIS_ERRORSRC_OVERREAD_Detected (1UL) /*!< Error occurred */

/* Bit 2 : NACK sent after receiving a data byte */
#define TWIS_ERRORSRC_DNACK_Pos (2UL) /*!< Position of DNACK field. */
#define TWIS_ERRORSRC_DNACK_Msk (0x1UL << TWIS_ERRORSRC_DNACK_Pos) /*!< Bit mask of DNACK field. */
#define TWIS_ERRORSRC_DNACK_NotReceived (0UL) /*!< Error did not occur */
#define TWIS_ERRORSRC_DNACK_Received (1UL) /*!< Error occurred */

/* Bit 0 : RX buffer overflow detected, and prevented */
#define TWIS_ERRORSRC_OVERFLOW_Pos (0UL) /*!< Position of OVERFLOW field. */
#define TWIS_ERRORSRC_OVERFLOW_Msk (0x1UL << TWIS_ERRORSRC_OVERFLOW_Pos) /*!< Bit mask of OVERFLOW field. */
#define TWIS_ERRORSRC_OVERFLOW_NotDetected (0UL) /*!< Error did not occur */
#define TWIS_ERRORSRC_OVERFLOW_Detected (1UL) /*!< Error occurred */

/* Register: TWIS_MATCH */
/* Description: Status register indicating which address had a match */

/* Bit 0 : Which of the addresses in {ADDRESS} matched the incoming address */
#define TWIS_MATCH_MATCH_Pos (0UL) /*!< Position of MATCH field. */
#define TWIS_MATCH_MATCH_Msk (0x1UL << TWIS_MATCH_MATCH_Pos) /*!< Bit mask of MATCH field. */

/* Register: TWIS_ENABLE */
/* Description: Enable TWIS */

/* Bits 3..0 : Enable or disable TWIS */
#define TWIS_ENABLE_ENABLE_Pos (0UL) /*!< Position of ENABLE field. */
#define TWIS_ENABLE_ENABLE_Msk (0xFUL << TWIS_ENABLE_ENABLE_Pos) /*!< Bit mask of ENABLE field. */
#define TWIS_ENABLE_ENABLE_Disabled (0UL) /*!< Disable TWIS */
#define TWIS_ENABLE_ENABLE_Enabled (9UL) /*!< Enable TWIS */

/* Register: TWIS_PSEL_SCL */
/* Description: Pin select for SCL signal */

/* Bit 31 : Connection */
#define TWIS_PSEL_SCL_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define TWIS_PSEL_SCL_CONNECT_Msk (0x1UL << TWIS_PSEL_SCL_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define TWIS_PSEL_SCL_CONNECT_Connected (0UL) /*!< Connect */
#define TWIS_PSEL_SCL_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define TWIS_PSEL_SCL_PIN_Pos (0UL) /*!< Position of PIN field. */
#define TWIS_PSEL_SCL_PIN_Msk (0x1FUL << TWIS_PSEL_SCL_PIN_Pos) /*!< Bit mask of PIN field. */

/* Register: TWIS_PSEL_SDA */
/* Description: Pin select for SDA signal */

/* Bit 31 : Connection */
#define TWIS_PSEL_SDA_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define TWIS_PSEL_SDA_CONNECT_Msk (0x1UL << TWIS_PSEL_SDA_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define TWIS_PSEL_SDA_CONNECT_Connected (0UL) /*!< Connect */
#define TWIS_PSEL_SDA_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define TWIS_PSEL_SDA_PIN_Pos (0UL) /*!< Position of PIN field. */
#define TWIS_PSEL_SDA_PIN_Msk (0x1FUL << TWIS_PSEL_SDA_PIN_Pos) /*!< Bit mask of PIN field. */

/* Register: TWIS_RXD_PTR */
/* Description: RXD Data pointer */

/* Bits 31..0 : RXD Data pointer */
#define TWIS_RXD_PTR_PTR_Pos (0UL) /*!< Position of PTR field. */
#define TWIS_RXD_PTR_PTR_Msk (0xFFFFFFFFUL << TWIS_RXD_PTR_PTR_Pos) /*!< Bit mask of PTR field. */

/* Register: TWIS_RXD_MAXCNT */
/* Description: Maximum number of bytes in RXD buffer */

/* Bits 12..0 : Maximum number of bytes in RXD buffer */
#define TWIS_RXD_MAXCNT_MAXCNT_Pos (0UL) /*!< Position of MAXCNT field. */
#define TWIS_RXD_MAXCNT_MAXCNT_Msk (0x1FFFUL << TWIS_RXD_MAXCNT_MAXCNT_Pos) /*!< Bit mask of MAXCNT field. */

/* Register: TWIS_RXD_AMOUNT */
/* Description: Number of bytes transferred in the last RXD transaction */

/* Bits 12..0 : Number of bytes transferred in the last RXD transaction */
#define TWIS_RXD_AMOUNT_AMOUNT_Pos (0UL) /*!< Position of AMOUNT field. */
#define TWIS_RXD_AMOUNT_AMOUNT_Msk (0x1FFFUL << TWIS_RXD_AMOUNT_AMOUNT_Pos) /*!< Bit mask of AMOUNT field. */

/* Register: TWIS_TXD_PTR */
/* Description: TXD Data pointer */

/* Bits 31..0 : TXD Data pointer */
#define TWIS_TXD_PTR_PTR_Pos (0UL) /*!< Position of PTR field. */
#define TWIS_TXD_PTR_PTR_Msk (0xFFFFFFFFUL << TWIS_TXD_PTR_PTR_Pos) /*!< Bit mask of PTR field. */

/* Register: TWIS_TXD_MAXCNT */
/* Description: Maximum number of bytes in TXD buffer */

/* Bits 12..0 : Maximum number of bytes in TXD buffer */
#define TWIS_TXD_MAXCNT_MAXCNT_Pos (0UL) /*!< Position of MAXCNT field. */
#define TWIS_TXD_MAXCNT_MAXCNT_Msk (0x1FFFUL << TWIS_TXD_MAXCNT_MAXCNT_Pos) /*!< Bit mask of MAXCNT field. */

/* Register: TWIS_TXD_AMOUNT */
/* Description: Number of bytes transferred in the last TXD transaction */

/* Bits 12..0 : Number of bytes transferred in the last TXD transaction */
#define TWIS_TXD_AMOUNT_AMOUNT_Pos (0UL) /*!< Position of AMOUNT field. */
#define TWIS_TXD_AMOUNT_AMOUNT_Msk (0x1FFFUL << TWIS_TXD_AMOUNT_AMOUNT_Pos) /*!< Bit mask of AMOUNT field. */

/* Register: TWIS_ADDRESS */
/* Description: Description collection: TWI slave address n */

/* Bits 6..0 : TWI slave address */
#define TWIS_ADDRESS_ADDRESS_Pos (0UL) /*!< Position of ADDRESS field. */
#define TWIS_ADDRESS_ADDRESS_Msk (0x7FUL << TWIS_ADDRESS_ADDRESS_Pos) /*!< Bit mask of ADDRESS field. */

/* Register: TWIS_CONFIG */
/* Description: Configuration register for the address match mechanism */

/* Bit 1 : Enable or disable address matching on ADDRESS[1] */
#define TWIS_CONFIG_ADDRESS1_Pos (1UL) /*!< Position of ADDRESS1 field. */
#define TWIS_CONFIG_ADDRESS1_Msk (0x1UL << TWIS_CONFIG_ADDRESS1_Pos) /*!< Bit mask of ADDRESS1 field. */
#define TWIS_CONFIG_ADDRESS1_Disabled (0UL) /*!< Disabled */
#define TWIS_CONFIG_ADDRESS1_Enabled (1UL) /*!< Enabled */

/* Bit 0 : Enable or disable address matching on ADDRESS[0] */
#define TWIS_CONFIG_ADDRESS0_Pos (0UL) /*!< Position of ADDRESS0 field. */
#define TWIS_CONFIG_ADDRESS0_Msk (0x1UL << TWIS_CONFIG_ADDRESS0_Pos) /*!< Bit mask of ADDRESS0 field. */
#define TWIS_CONFIG_ADDRESS0_Disabled (0UL) /*!< Disabled */
#define TWIS_CONFIG_ADDRESS0_Enabled (1UL) /*!< Enabled */

/* Register: TWIS_ORC */
/* Description: Over-read character. Character sent out in case of an over-read of the transmit buffer. */

/* Bits 7..0 : Over-read character. Character sent out in case of an over-read of the transmit buffer. */
#define TWIS_ORC_ORC_Pos (0UL) /*!< Position of ORC field. */
#define TWIS_ORC_ORC_Msk (0xFFUL << TWIS_ORC_ORC_Pos) /*!< Bit mask of ORC field. */


/* Peripheral: UARTE */
/* Description: UART with EasyDMA 0 */

/* Register: UARTE_TASKS_STARTRX */
/* Description: Start UART receiver */

/* Bit 0 : Start UART receiver */
#define UARTE_TASKS_STARTRX_TASKS_STARTRX_Pos (0UL) /*!< Position of TASKS_STARTRX field. */
#define UARTE_TASKS_STARTRX_TASKS_STARTRX_Msk (0x1UL << UARTE_TASKS_STARTRX_TASKS_STARTRX_Pos) /*!< Bit mask of TASKS_STARTRX field. */
#define UARTE_TASKS_STARTRX_TASKS_STARTRX_Trigger (1UL) /*!< Trigger task */

/* Register: UARTE_TASKS_STOPRX */
/* Description: Stop UART receiver */

/* Bit 0 : Stop UART receiver */
#define UARTE_TASKS_STOPRX_TASKS_STOPRX_Pos (0UL) /*!< Position of TASKS_STOPRX field. */
#define UARTE_TASKS_STOPRX_TASKS_STOPRX_Msk (0x1UL << UARTE_TASKS_STOPRX_TASKS_STOPRX_Pos) /*!< Bit mask of TASKS_STOPRX field. */
#define UARTE_TASKS_STOPRX_TASKS_STOPRX_Trigger (1UL) /*!< Trigger task */

/* Register: UARTE_TASKS_STARTTX */
/* Description: Start UART transmitter */

/* Bit 0 : Start UART transmitter */
#define UARTE_TASKS_STARTTX_TASKS_STARTTX_Pos (0UL) /*!< Position of TASKS_STARTTX field. */
#define UARTE_TASKS_STARTTX_TASKS_STARTTX_Msk (0x1UL << UARTE_TASKS_STARTTX_TASKS_STARTTX_Pos) /*!< Bit mask of TASKS_STARTTX field. */
#define UARTE_TASKS_STARTTX_TASKS_STARTTX_Trigger (1UL) /*!< Trigger task */

/* Register: UARTE_TASKS_STOPTX */
/* Description: Stop UART transmitter */

/* Bit 0 : Stop UART transmitter */
#define UARTE_TASKS_STOPTX_TASKS_STOPTX_Pos (0UL) /*!< Position of TASKS_STOPTX field. */
#define UARTE_TASKS_STOPTX_TASKS_STOPTX_Msk (0x1UL << UARTE_TASKS_STOPTX_TASKS_STOPTX_Pos) /*!< Bit mask of TASKS_STOPTX field. */
#define UARTE_TASKS_STOPTX_TASKS_STOPTX_Trigger (1UL) /*!< Trigger task */

/* Register: UARTE_TASKS_FLUSHRX */
/* Description: Flush RX FIFO into RX buffer */

/* Bit 0 : Flush RX FIFO into RX buffer */
#define UARTE_TASKS_FLUSHRX_TASKS_FLUSHRX_Pos (0UL) /*!< Position of TASKS_FLUSHRX field. */
#define UARTE_TASKS_FLUSHRX_TASKS_FLUSHRX_Msk (0x1UL << UARTE_TASKS_FLUSHRX_TASKS_FLUSHRX_Pos) /*!< Bit mask of TASKS_FLUSHRX field. */
#define UARTE_TASKS_FLUSHRX_TASKS_FLUSHRX_Trigger (1UL) /*!< Trigger task */

/* Register: UARTE_SUBSCRIBE_STARTRX */
/* Description: Subscribe configuration for task STARTRX */

/* Bit 31 :   */
#define UARTE_SUBSCRIBE_STARTRX_EN_Pos (31UL) /*!< Position of EN field. */
#define UARTE_SUBSCRIBE_STARTRX_EN_Msk (0x1UL << UARTE_SUBSCRIBE_STARTRX_EN_Pos) /*!< Bit mask of EN field. */
#define UARTE_SUBSCRIBE_STARTRX_EN_Disabled (0UL) /*!< Disable subscription */
#define UARTE_SUBSCRIBE_STARTRX_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task STARTRX will subscribe to */
#define UARTE_SUBSCRIBE_STARTRX_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define UARTE_SUBSCRIBE_STARTRX_CHIDX_Msk (0xFUL << UARTE_SUBSCRIBE_STARTRX_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: UARTE_SUBSCRIBE_STOPRX */
/* Description: Subscribe configuration for task STOPRX */

/* Bit 31 :   */
#define UARTE_SUBSCRIBE_STOPRX_EN_Pos (31UL) /*!< Position of EN field. */
#define UARTE_SUBSCRIBE_STOPRX_EN_Msk (0x1UL << UARTE_SUBSCRIBE_STOPRX_EN_Pos) /*!< Bit mask of EN field. */
#define UARTE_SUBSCRIBE_STOPRX_EN_Disabled (0UL) /*!< Disable subscription */
#define UARTE_SUBSCRIBE_STOPRX_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task STOPRX will subscribe to */
#define UARTE_SUBSCRIBE_STOPRX_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define UARTE_SUBSCRIBE_STOPRX_CHIDX_Msk (0xFUL << UARTE_SUBSCRIBE_STOPRX_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: UARTE_SUBSCRIBE_STARTTX */
/* Description: Subscribe configuration for task STARTTX */

/* Bit 31 :   */
#define UARTE_SUBSCRIBE_STARTTX_EN_Pos (31UL) /*!< Position of EN field. */
#define UARTE_SUBSCRIBE_STARTTX_EN_Msk (0x1UL << UARTE_SUBSCRIBE_STARTTX_EN_Pos) /*!< Bit mask of EN field. */
#define UARTE_SUBSCRIBE_STARTTX_EN_Disabled (0UL) /*!< Disable subscription */
#define UARTE_SUBSCRIBE_STARTTX_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task STARTTX will subscribe to */
#define UARTE_SUBSCRIBE_STARTTX_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define UARTE_SUBSCRIBE_STARTTX_CHIDX_Msk (0xFUL << UARTE_SUBSCRIBE_STARTTX_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: UARTE_SUBSCRIBE_STOPTX */
/* Description: Subscribe configuration for task STOPTX */

/* Bit 31 :   */
#define UARTE_SUBSCRIBE_STOPTX_EN_Pos (31UL) /*!< Position of EN field. */
#define UARTE_SUBSCRIBE_STOPTX_EN_Msk (0x1UL << UARTE_SUBSCRIBE_STOPTX_EN_Pos) /*!< Bit mask of EN field. */
#define UARTE_SUBSCRIBE_STOPTX_EN_Disabled (0UL) /*!< Disable subscription */
#define UARTE_SUBSCRIBE_STOPTX_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task STOPTX will subscribe to */
#define UARTE_SUBSCRIBE_STOPTX_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define UARTE_SUBSCRIBE_STOPTX_CHIDX_Msk (0xFUL << UARTE_SUBSCRIBE_STOPTX_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: UARTE_SUBSCRIBE_FLUSHRX */
/* Description: Subscribe configuration for task FLUSHRX */

/* Bit 31 :   */
#define UARTE_SUBSCRIBE_FLUSHRX_EN_Pos (31UL) /*!< Position of EN field. */
#define UARTE_SUBSCRIBE_FLUSHRX_EN_Msk (0x1UL << UARTE_SUBSCRIBE_FLUSHRX_EN_Pos) /*!< Bit mask of EN field. */
#define UARTE_SUBSCRIBE_FLUSHRX_EN_Disabled (0UL) /*!< Disable subscription */
#define UARTE_SUBSCRIBE_FLUSHRX_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task FLUSHRX will subscribe to */
#define UARTE_SUBSCRIBE_FLUSHRX_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define UARTE_SUBSCRIBE_FLUSHRX_CHIDX_Msk (0xFUL << UARTE_SUBSCRIBE_FLUSHRX_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: UARTE_EVENTS_CTS */
/* Description: CTS is activated (set low). Clear To Send. */

/* Bit 0 : CTS is activated (set low). Clear To Send. */
#define UARTE_EVENTS_CTS_EVENTS_CTS_Pos (0UL) /*!< Position of EVENTS_CTS field. */
#define UARTE_EVENTS_CTS_EVENTS_CTS_Msk (0x1UL << UARTE_EVENTS_CTS_EVENTS_CTS_Pos) /*!< Bit mask of EVENTS_CTS field. */
#define UARTE_EVENTS_CTS_EVENTS_CTS_NotGenerated (0UL) /*!< Event not generated */
#define UARTE_EVENTS_CTS_EVENTS_CTS_Generated (1UL) /*!< Event generated */

/* Register: UARTE_EVENTS_NCTS */
/* Description: CTS is deactivated (set high). Not Clear To Send. */

/* Bit 0 : CTS is deactivated (set high). Not Clear To Send. */
#define UARTE_EVENTS_NCTS_EVENTS_NCTS_Pos (0UL) /*!< Position of EVENTS_NCTS field. */
#define UARTE_EVENTS_NCTS_EVENTS_NCTS_Msk (0x1UL << UARTE_EVENTS_NCTS_EVENTS_NCTS_Pos) /*!< Bit mask of EVENTS_NCTS field. */
#define UARTE_EVENTS_NCTS_EVENTS_NCTS_NotGenerated (0UL) /*!< Event not generated */
#define UARTE_EVENTS_NCTS_EVENTS_NCTS_Generated (1UL) /*!< Event generated */

/* Register: UARTE_EVENTS_RXDRDY */
/* Description: Data received in RXD (but potentially not yet transferred to Data RAM) */

/* Bit 0 : Data received in RXD (but potentially not yet transferred to Data RAM) */
#define UARTE_EVENTS_RXDRDY_EVENTS_RXDRDY_Pos (0UL) /*!< Position of EVENTS_RXDRDY field. */
#define UARTE_EVENTS_RXDRDY_EVENTS_RXDRDY_Msk (0x1UL << UARTE_EVENTS_RXDRDY_EVENTS_RXDRDY_Pos) /*!< Bit mask of EVENTS_RXDRDY field. */
#define UARTE_EVENTS_RXDRDY_EVENTS_RXDRDY_NotGenerated (0UL) /*!< Event not generated */
#define UARTE_EVENTS_RXDRDY_EVENTS_RXDRDY_Generated (1UL) /*!< Event generated */

/* Register: UARTE_EVENTS_ENDRX */
/* Description: Receive buffer is filled up */

/* Bit 0 : Receive buffer is filled up */
#define UARTE_EVENTS_ENDRX_EVENTS_ENDRX_Pos (0UL) /*!< Position of EVENTS_ENDRX field. */
#define UARTE_EVENTS_ENDRX_EVENTS_ENDRX_Msk (0x1UL << UARTE_EVENTS_ENDRX_EVENTS_ENDRX_Pos) /*!< Bit mask of EVENTS_ENDRX field. */
#define UARTE_EVENTS_ENDRX_EVENTS_ENDRX_NotGenerated (0UL) /*!< Event not generated */
#define UARTE_EVENTS_ENDRX_EVENTS_ENDRX_Generated (1UL) /*!< Event generated */

/* Register: UARTE_EVENTS_TXDRDY */
/* Description: Data sent from TXD */

/* Bit 0 : Data sent from TXD */
#define UARTE_EVENTS_TXDRDY_EVENTS_TXDRDY_Pos (0UL) /*!< Position of EVENTS_TXDRDY field. */
#define UARTE_EVENTS_TXDRDY_EVENTS_TXDRDY_Msk (0x1UL << UARTE_EVENTS_TXDRDY_EVENTS_TXDRDY_Pos) /*!< Bit mask of EVENTS_TXDRDY field. */
#define UARTE_EVENTS_TXDRDY_EVENTS_TXDRDY_NotGenerated (0UL) /*!< Event not generated */
#define UARTE_EVENTS_TXDRDY_EVENTS_TXDRDY_Generated (1UL) /*!< Event generated */

/* Register: UARTE_EVENTS_ENDTX */
/* Description: Last TX byte transmitted */

/* Bit 0 : Last TX byte transmitted */
#define UARTE_EVENTS_ENDTX_EVENTS_ENDTX_Pos (0UL) /*!< Position of EVENTS_ENDTX field. */
#define UARTE_EVENTS_ENDTX_EVENTS_ENDTX_Msk (0x1UL << UARTE_EVENTS_ENDTX_EVENTS_ENDTX_Pos) /*!< Bit mask of EVENTS_ENDTX field. */
#define UARTE_EVENTS_ENDTX_EVENTS_ENDTX_NotGenerated (0UL) /*!< Event not generated */
#define UARTE_EVENTS_ENDTX_EVENTS_ENDTX_Generated (1UL) /*!< Event generated */

/* Register: UARTE_EVENTS_ERROR */
/* Description: Error detected */

/* Bit 0 : Error detected */
#define UARTE_EVENTS_ERROR_EVENTS_ERROR_Pos (0UL) /*!< Position of EVENTS_ERROR field. */
#define UARTE_EVENTS_ERROR_EVENTS_ERROR_Msk (0x1UL << UARTE_EVENTS_ERROR_EVENTS_ERROR_Pos) /*!< Bit mask of EVENTS_ERROR field. */
#define UARTE_EVENTS_ERROR_EVENTS_ERROR_NotGenerated (0UL) /*!< Event not generated */
#define UARTE_EVENTS_ERROR_EVENTS_ERROR_Generated (1UL) /*!< Event generated */

/* Register: UARTE_EVENTS_RXTO */
/* Description: Receiver timeout */

/* Bit 0 : Receiver timeout */
#define UARTE_EVENTS_RXTO_EVENTS_RXTO_Pos (0UL) /*!< Position of EVENTS_RXTO field. */
#define UARTE_EVENTS_RXTO_EVENTS_RXTO_Msk (0x1UL << UARTE_EVENTS_RXTO_EVENTS_RXTO_Pos) /*!< Bit mask of EVENTS_RXTO field. */
#define UARTE_EVENTS_RXTO_EVENTS_RXTO_NotGenerated (0UL) /*!< Event not generated */
#define UARTE_EVENTS_RXTO_EVENTS_RXTO_Generated (1UL) /*!< Event generated */

/* Register: UARTE_EVENTS_RXSTARTED */
/* Description: UART receiver has started */

/* Bit 0 : UART receiver has started */
#define UARTE_EVENTS_RXSTARTED_EVENTS_RXSTARTED_Pos (0UL) /*!< Position of EVENTS_RXSTARTED field. */
#define UARTE_EVENTS_RXSTARTED_EVENTS_RXSTARTED_Msk (0x1UL << UARTE_EVENTS_RXSTARTED_EVENTS_RXSTARTED_Pos) /*!< Bit mask of EVENTS_RXSTARTED field. */
#define UARTE_EVENTS_RXSTARTED_EVENTS_RXSTARTED_NotGenerated (0UL) /*!< Event not generated */
#define UARTE_EVENTS_RXSTARTED_EVENTS_RXSTARTED_Generated (1UL) /*!< Event generated */

/* Register: UARTE_EVENTS_TXSTARTED */
/* Description: UART transmitter has started */

/* Bit 0 : UART transmitter has started */
#define UARTE_EVENTS_TXSTARTED_EVENTS_TXSTARTED_Pos (0UL) /*!< Position of EVENTS_TXSTARTED field. */
#define UARTE_EVENTS_TXSTARTED_EVENTS_TXSTARTED_Msk (0x1UL << UARTE_EVENTS_TXSTARTED_EVENTS_TXSTARTED_Pos) /*!< Bit mask of EVENTS_TXSTARTED field. */
#define UARTE_EVENTS_TXSTARTED_EVENTS_TXSTARTED_NotGenerated (0UL) /*!< Event not generated */
#define UARTE_EVENTS_TXSTARTED_EVENTS_TXSTARTED_Generated (1UL) /*!< Event generated */

/* Register: UARTE_EVENTS_TXSTOPPED */
/* Description: Transmitter stopped */

/* Bit 0 : Transmitter stopped */
#define UARTE_EVENTS_TXSTOPPED_EVENTS_TXSTOPPED_Pos (0UL) /*!< Position of EVENTS_TXSTOPPED field. */
#define UARTE_EVENTS_TXSTOPPED_EVENTS_TXSTOPPED_Msk (0x1UL << UARTE_EVENTS_TXSTOPPED_EVENTS_TXSTOPPED_Pos) /*!< Bit mask of EVENTS_TXSTOPPED field. */
#define UARTE_EVENTS_TXSTOPPED_EVENTS_TXSTOPPED_NotGenerated (0UL) /*!< Event not generated */
#define UARTE_EVENTS_TXSTOPPED_EVENTS_TXSTOPPED_Generated (1UL) /*!< Event generated */

/* Register: UARTE_PUBLISH_CTS */
/* Description: Publish configuration for event CTS */

/* Bit 31 :   */
#define UARTE_PUBLISH_CTS_EN_Pos (31UL) /*!< Position of EN field. */
#define UARTE_PUBLISH_CTS_EN_Msk (0x1UL << UARTE_PUBLISH_CTS_EN_Pos) /*!< Bit mask of EN field. */
#define UARTE_PUBLISH_CTS_EN_Disabled (0UL) /*!< Disable publishing */
#define UARTE_PUBLISH_CTS_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event CTS will publish to. */
#define UARTE_PUBLISH_CTS_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define UARTE_PUBLISH_CTS_CHIDX_Msk (0xFUL << UARTE_PUBLISH_CTS_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: UARTE_PUBLISH_NCTS */
/* Description: Publish configuration for event NCTS */

/* Bit 31 :   */
#define UARTE_PUBLISH_NCTS_EN_Pos (31UL) /*!< Position of EN field. */
#define UARTE_PUBLISH_NCTS_EN_Msk (0x1UL << UARTE_PUBLISH_NCTS_EN_Pos) /*!< Bit mask of EN field. */
#define UARTE_PUBLISH_NCTS_EN_Disabled (0UL) /*!< Disable publishing */
#define UARTE_PUBLISH_NCTS_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event NCTS will publish to. */
#define UARTE_PUBLISH_NCTS_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define UARTE_PUBLISH_NCTS_CHIDX_Msk (0xFUL << UARTE_PUBLISH_NCTS_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: UARTE_PUBLISH_RXDRDY */
/* Description: Publish configuration for event RXDRDY */

/* Bit 31 :   */
#define UARTE_PUBLISH_RXDRDY_EN_Pos (31UL) /*!< Position of EN field. */
#define UARTE_PUBLISH_RXDRDY_EN_Msk (0x1UL << UARTE_PUBLISH_RXDRDY_EN_Pos) /*!< Bit mask of EN field. */
#define UARTE_PUBLISH_RXDRDY_EN_Disabled (0UL) /*!< Disable publishing */
#define UARTE_PUBLISH_RXDRDY_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event RXDRDY will publish to. */
#define UARTE_PUBLISH_RXDRDY_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define UARTE_PUBLISH_RXDRDY_CHIDX_Msk (0xFUL << UARTE_PUBLISH_RXDRDY_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: UARTE_PUBLISH_ENDRX */
/* Description: Publish configuration for event ENDRX */

/* Bit 31 :   */
#define UARTE_PUBLISH_ENDRX_EN_Pos (31UL) /*!< Position of EN field. */
#define UARTE_PUBLISH_ENDRX_EN_Msk (0x1UL << UARTE_PUBLISH_ENDRX_EN_Pos) /*!< Bit mask of EN field. */
#define UARTE_PUBLISH_ENDRX_EN_Disabled (0UL) /*!< Disable publishing */
#define UARTE_PUBLISH_ENDRX_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event ENDRX will publish to. */
#define UARTE_PUBLISH_ENDRX_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define UARTE_PUBLISH_ENDRX_CHIDX_Msk (0xFUL << UARTE_PUBLISH_ENDRX_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: UARTE_PUBLISH_TXDRDY */
/* Description: Publish configuration for event TXDRDY */

/* Bit 31 :   */
#define UARTE_PUBLISH_TXDRDY_EN_Pos (31UL) /*!< Position of EN field. */
#define UARTE_PUBLISH_TXDRDY_EN_Msk (0x1UL << UARTE_PUBLISH_TXDRDY_EN_Pos) /*!< Bit mask of EN field. */
#define UARTE_PUBLISH_TXDRDY_EN_Disabled (0UL) /*!< Disable publishing */
#define UARTE_PUBLISH_TXDRDY_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event TXDRDY will publish to. */
#define UARTE_PUBLISH_TXDRDY_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define UARTE_PUBLISH_TXDRDY_CHIDX_Msk (0xFUL << UARTE_PUBLISH_TXDRDY_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: UARTE_PUBLISH_ENDTX */
/* Description: Publish configuration for event ENDTX */

/* Bit 31 :   */
#define UARTE_PUBLISH_ENDTX_EN_Pos (31UL) /*!< Position of EN field. */
#define UARTE_PUBLISH_ENDTX_EN_Msk (0x1UL << UARTE_PUBLISH_ENDTX_EN_Pos) /*!< Bit mask of EN field. */
#define UARTE_PUBLISH_ENDTX_EN_Disabled (0UL) /*!< Disable publishing */
#define UARTE_PUBLISH_ENDTX_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event ENDTX will publish to. */
#define UARTE_PUBLISH_ENDTX_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define UARTE_PUBLISH_ENDTX_CHIDX_Msk (0xFUL << UARTE_PUBLISH_ENDTX_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: UARTE_PUBLISH_ERROR */
/* Description: Publish configuration for event ERROR */

/* Bit 31 :   */
#define UARTE_PUBLISH_ERROR_EN_Pos (31UL) /*!< Position of EN field. */
#define UARTE_PUBLISH_ERROR_EN_Msk (0x1UL << UARTE_PUBLISH_ERROR_EN_Pos) /*!< Bit mask of EN field. */
#define UARTE_PUBLISH_ERROR_EN_Disabled (0UL) /*!< Disable publishing */
#define UARTE_PUBLISH_ERROR_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event ERROR will publish to. */
#define UARTE_PUBLISH_ERROR_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define UARTE_PUBLISH_ERROR_CHIDX_Msk (0xFUL << UARTE_PUBLISH_ERROR_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: UARTE_PUBLISH_RXTO */
/* Description: Publish configuration for event RXTO */

/* Bit 31 :   */
#define UARTE_PUBLISH_RXTO_EN_Pos (31UL) /*!< Position of EN field. */
#define UARTE_PUBLISH_RXTO_EN_Msk (0x1UL << UARTE_PUBLISH_RXTO_EN_Pos) /*!< Bit mask of EN field. */
#define UARTE_PUBLISH_RXTO_EN_Disabled (0UL) /*!< Disable publishing */
#define UARTE_PUBLISH_RXTO_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event RXTO will publish to. */
#define UARTE_PUBLISH_RXTO_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define UARTE_PUBLISH_RXTO_CHIDX_Msk (0xFUL << UARTE_PUBLISH_RXTO_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: UARTE_PUBLISH_RXSTARTED */
/* Description: Publish configuration for event RXSTARTED */

/* Bit 31 :   */
#define UARTE_PUBLISH_RXSTARTED_EN_Pos (31UL) /*!< Position of EN field. */
#define UARTE_PUBLISH_RXSTARTED_EN_Msk (0x1UL << UARTE_PUBLISH_RXSTARTED_EN_Pos) /*!< Bit mask of EN field. */
#define UARTE_PUBLISH_RXSTARTED_EN_Disabled (0UL) /*!< Disable publishing */
#define UARTE_PUBLISH_RXSTARTED_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event RXSTARTED will publish to. */
#define UARTE_PUBLISH_RXSTARTED_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define UARTE_PUBLISH_RXSTARTED_CHIDX_Msk (0xFUL << UARTE_PUBLISH_RXSTARTED_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: UARTE_PUBLISH_TXSTARTED */
/* Description: Publish configuration for event TXSTARTED */

/* Bit 31 :   */
#define UARTE_PUBLISH_TXSTARTED_EN_Pos (31UL) /*!< Position of EN field. */
#define UARTE_PUBLISH_TXSTARTED_EN_Msk (0x1UL << UARTE_PUBLISH_TXSTARTED_EN_Pos) /*!< Bit mask of EN field. */
#define UARTE_PUBLISH_TXSTARTED_EN_Disabled (0UL) /*!< Disable publishing */
#define UARTE_PUBLISH_TXSTARTED_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event TXSTARTED will publish to. */
#define UARTE_PUBLISH_TXSTARTED_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define UARTE_PUBLISH_TXSTARTED_CHIDX_Msk (0xFUL << UARTE_PUBLISH_TXSTARTED_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: UARTE_PUBLISH_TXSTOPPED */
/* Description: Publish configuration for event TXSTOPPED */

/* Bit 31 :   */
#define UARTE_PUBLISH_TXSTOPPED_EN_Pos (31UL) /*!< Position of EN field. */
#define UARTE_PUBLISH_TXSTOPPED_EN_Msk (0x1UL << UARTE_PUBLISH_TXSTOPPED_EN_Pos) /*!< Bit mask of EN field. */
#define UARTE_PUBLISH_TXSTOPPED_EN_Disabled (0UL) /*!< Disable publishing */
#define UARTE_PUBLISH_TXSTOPPED_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event TXSTOPPED will publish to. */
#define UARTE_PUBLISH_TXSTOPPED_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define UARTE_PUBLISH_TXSTOPPED_CHIDX_Msk (0xFUL << UARTE_PUBLISH_TXSTOPPED_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: UARTE_SHORTS */
/* Description: Shortcuts between local events and tasks */

/* Bit 6 : Shortcut between event ENDRX and task STOPRX */
#define UARTE_SHORTS_ENDRX_STOPRX_Pos (6UL) /*!< Position of ENDRX_STOPRX field. */
#define UARTE_SHORTS_ENDRX_STOPRX_Msk (0x1UL << UARTE_SHORTS_ENDRX_STOPRX_Pos) /*!< Bit mask of ENDRX_STOPRX field. */
#define UARTE_SHORTS_ENDRX_STOPRX_Disabled (0UL) /*!< Disable shortcut */
#define UARTE_SHORTS_ENDRX_STOPRX_Enabled (1UL) /*!< Enable shortcut */

/* Bit 5 : Shortcut between event ENDRX and task STARTRX */
#define UARTE_SHORTS_ENDRX_STARTRX_Pos (5UL) /*!< Position of ENDRX_STARTRX field. */
#define UARTE_SHORTS_ENDRX_STARTRX_Msk (0x1UL << UARTE_SHORTS_ENDRX_STARTRX_Pos) /*!< Bit mask of ENDRX_STARTRX field. */
#define UARTE_SHORTS_ENDRX_STARTRX_Disabled (0UL) /*!< Disable shortcut */
#define UARTE_SHORTS_ENDRX_STARTRX_Enabled (1UL) /*!< Enable shortcut */

/* Register: UARTE_INTEN */
/* Description: Enable or disable interrupt */

/* Bit 22 : Enable or disable interrupt for event TXSTOPPED */
#define UARTE_INTEN_TXSTOPPED_Pos (22UL) /*!< Position of TXSTOPPED field. */
#define UARTE_INTEN_TXSTOPPED_Msk (0x1UL << UARTE_INTEN_TXSTOPPED_Pos) /*!< Bit mask of TXSTOPPED field. */
#define UARTE_INTEN_TXSTOPPED_Disabled (0UL) /*!< Disable */
#define UARTE_INTEN_TXSTOPPED_Enabled (1UL) /*!< Enable */

/* Bit 20 : Enable or disable interrupt for event TXSTARTED */
#define UARTE_INTEN_TXSTARTED_Pos (20UL) /*!< Position of TXSTARTED field. */
#define UARTE_INTEN_TXSTARTED_Msk (0x1UL << UARTE_INTEN_TXSTARTED_Pos) /*!< Bit mask of TXSTARTED field. */
#define UARTE_INTEN_TXSTARTED_Disabled (0UL) /*!< Disable */
#define UARTE_INTEN_TXSTARTED_Enabled (1UL) /*!< Enable */

/* Bit 19 : Enable or disable interrupt for event RXSTARTED */
#define UARTE_INTEN_RXSTARTED_Pos (19UL) /*!< Position of RXSTARTED field. */
#define UARTE_INTEN_RXSTARTED_Msk (0x1UL << UARTE_INTEN_RXSTARTED_Pos) /*!< Bit mask of RXSTARTED field. */
#define UARTE_INTEN_RXSTARTED_Disabled (0UL) /*!< Disable */
#define UARTE_INTEN_RXSTARTED_Enabled (1UL) /*!< Enable */

/* Bit 17 : Enable or disable interrupt for event RXTO */
#define UARTE_INTEN_RXTO_Pos (17UL) /*!< Position of RXTO field. */
#define UARTE_INTEN_RXTO_Msk (0x1UL << UARTE_INTEN_RXTO_Pos) /*!< Bit mask of RXTO field. */
#define UARTE_INTEN_RXTO_Disabled (0UL) /*!< Disable */
#define UARTE_INTEN_RXTO_Enabled (1UL) /*!< Enable */

/* Bit 9 : Enable or disable interrupt for event ERROR */
#define UARTE_INTEN_ERROR_Pos (9UL) /*!< Position of ERROR field. */
#define UARTE_INTEN_ERROR_Msk (0x1UL << UARTE_INTEN_ERROR_Pos) /*!< Bit mask of ERROR field. */
#define UARTE_INTEN_ERROR_Disabled (0UL) /*!< Disable */
#define UARTE_INTEN_ERROR_Enabled (1UL) /*!< Enable */

/* Bit 8 : Enable or disable interrupt for event ENDTX */
#define UARTE_INTEN_ENDTX_Pos (8UL) /*!< Position of ENDTX field. */
#define UARTE_INTEN_ENDTX_Msk (0x1UL << UARTE_INTEN_ENDTX_Pos) /*!< Bit mask of ENDTX field. */
#define UARTE_INTEN_ENDTX_Disabled (0UL) /*!< Disable */
#define UARTE_INTEN_ENDTX_Enabled (1UL) /*!< Enable */

/* Bit 7 : Enable or disable interrupt for event TXDRDY */
#define UARTE_INTEN_TXDRDY_Pos (7UL) /*!< Position of TXDRDY field. */
#define UARTE_INTEN_TXDRDY_Msk (0x1UL << UARTE_INTEN_TXDRDY_Pos) /*!< Bit mask of TXDRDY field. */
#define UARTE_INTEN_TXDRDY_Disabled (0UL) /*!< Disable */
#define UARTE_INTEN_TXDRDY_Enabled (1UL) /*!< Enable */

/* Bit 4 : Enable or disable interrupt for event ENDRX */
#define UARTE_INTEN_ENDRX_Pos (4UL) /*!< Position of ENDRX field. */
#define UARTE_INTEN_ENDRX_Msk (0x1UL << UARTE_INTEN_ENDRX_Pos) /*!< Bit mask of ENDRX field. */
#define UARTE_INTEN_ENDRX_Disabled (0UL) /*!< Disable */
#define UARTE_INTEN_ENDRX_Enabled (1UL) /*!< Enable */

/* Bit 2 : Enable or disable interrupt for event RXDRDY */
#define UARTE_INTEN_RXDRDY_Pos (2UL) /*!< Position of RXDRDY field. */
#define UARTE_INTEN_RXDRDY_Msk (0x1UL << UARTE_INTEN_RXDRDY_Pos) /*!< Bit mask of RXDRDY field. */
#define UARTE_INTEN_RXDRDY_Disabled (0UL) /*!< Disable */
#define UARTE_INTEN_RXDRDY_Enabled (1UL) /*!< Enable */

/* Bit 1 : Enable or disable interrupt for event NCTS */
#define UARTE_INTEN_NCTS_Pos (1UL) /*!< Position of NCTS field. */
#define UARTE_INTEN_NCTS_Msk (0x1UL << UARTE_INTEN_NCTS_Pos) /*!< Bit mask of NCTS field. */
#define UARTE_INTEN_NCTS_Disabled (0UL) /*!< Disable */
#define UARTE_INTEN_NCTS_Enabled (1UL) /*!< Enable */

/* Bit 0 : Enable or disable interrupt for event CTS */
#define UARTE_INTEN_CTS_Pos (0UL) /*!< Position of CTS field. */
#define UARTE_INTEN_CTS_Msk (0x1UL << UARTE_INTEN_CTS_Pos) /*!< Bit mask of CTS field. */
#define UARTE_INTEN_CTS_Disabled (0UL) /*!< Disable */
#define UARTE_INTEN_CTS_Enabled (1UL) /*!< Enable */

/* Register: UARTE_INTENSET */
/* Description: Enable interrupt */

/* Bit 22 : Write '1' to enable interrupt for event TXSTOPPED */
#define UARTE_INTENSET_TXSTOPPED_Pos (22UL) /*!< Position of TXSTOPPED field. */
#define UARTE_INTENSET_TXSTOPPED_Msk (0x1UL << UARTE_INTENSET_TXSTOPPED_Pos) /*!< Bit mask of TXSTOPPED field. */
#define UARTE_INTENSET_TXSTOPPED_Disabled (0UL) /*!< Read: Disabled */
#define UARTE_INTENSET_TXSTOPPED_Enabled (1UL) /*!< Read: Enabled */
#define UARTE_INTENSET_TXSTOPPED_Set (1UL) /*!< Enable */

/* Bit 20 : Write '1' to enable interrupt for event TXSTARTED */
#define UARTE_INTENSET_TXSTARTED_Pos (20UL) /*!< Position of TXSTARTED field. */
#define UARTE_INTENSET_TXSTARTED_Msk (0x1UL << UARTE_INTENSET_TXSTARTED_Pos) /*!< Bit mask of TXSTARTED field. */
#define UARTE_INTENSET_TXSTARTED_Disabled (0UL) /*!< Read: Disabled */
#define UARTE_INTENSET_TXSTARTED_Enabled (1UL) /*!< Read: Enabled */
#define UARTE_INTENSET_TXSTARTED_Set (1UL) /*!< Enable */

/* Bit 19 : Write '1' to enable interrupt for event RXSTARTED */
#define UARTE_INTENSET_RXSTARTED_Pos (19UL) /*!< Position of RXSTARTED field. */
#define UARTE_INTENSET_RXSTARTED_Msk (0x1UL << UARTE_INTENSET_RXSTARTED_Pos) /*!< Bit mask of RXSTARTED field. */
#define UARTE_INTENSET_RXSTARTED_Disabled (0UL) /*!< Read: Disabled */
#define UARTE_INTENSET_RXSTARTED_Enabled (1UL) /*!< Read: Enabled */
#define UARTE_INTENSET_RXSTARTED_Set (1UL) /*!< Enable */

/* Bit 17 : Write '1' to enable interrupt for event RXTO */
#define UARTE_INTENSET_RXTO_Pos (17UL) /*!< Position of RXTO field. */
#define UARTE_INTENSET_RXTO_Msk (0x1UL << UARTE_INTENSET_RXTO_Pos) /*!< Bit mask of RXTO field. */
#define UARTE_INTENSET_RXTO_Disabled (0UL) /*!< Read: Disabled */
#define UARTE_INTENSET_RXTO_Enabled (1UL) /*!< Read: Enabled */
#define UARTE_INTENSET_RXTO_Set (1UL) /*!< Enable */

/* Bit 9 : Write '1' to enable interrupt for event ERROR */
#define UARTE_INTENSET_ERROR_Pos (9UL) /*!< Position of ERROR field. */
#define UARTE_INTENSET_ERROR_Msk (0x1UL << UARTE_INTENSET_ERROR_Pos) /*!< Bit mask of ERROR field. */
#define UARTE_INTENSET_ERROR_Disabled (0UL) /*!< Read: Disabled */
#define UARTE_INTENSET_ERROR_Enabled (1UL) /*!< Read: Enabled */
#define UARTE_INTENSET_ERROR_Set (1UL) /*!< Enable */

/* Bit 8 : Write '1' to enable interrupt for event ENDTX */
#define UARTE_INTENSET_ENDTX_Pos (8UL) /*!< Position of ENDTX field. */
#define UARTE_INTENSET_ENDTX_Msk (0x1UL << UARTE_INTENSET_ENDTX_Pos) /*!< Bit mask of ENDTX field. */
#define UARTE_INTENSET_ENDTX_Disabled (0UL) /*!< Read: Disabled */
#define UARTE_INTENSET_ENDTX_Enabled (1UL) /*!< Read: Enabled */
#define UARTE_INTENSET_ENDTX_Set (1UL) /*!< Enable */

/* Bit 7 : Write '1' to enable interrupt for event TXDRDY */
#define UARTE_INTENSET_TXDRDY_Pos (7UL) /*!< Position of TXDRDY field. */
#define UARTE_INTENSET_TXDRDY_Msk (0x1UL << UARTE_INTENSET_TXDRDY_Pos) /*!< Bit mask of TXDRDY field. */
#define UARTE_INTENSET_TXDRDY_Disabled (0UL) /*!< Read: Disabled */
#define UARTE_INTENSET_TXDRDY_Enabled (1UL) /*!< Read: Enabled */
#define UARTE_INTENSET_TXDRDY_Set (1UL) /*!< Enable */

/* Bit 4 : Write '1' to enable interrupt for event ENDRX */
#define UARTE_INTENSET_ENDRX_Pos (4UL) /*!< Position of ENDRX field. */
#define UARTE_INTENSET_ENDRX_Msk (0x1UL << UARTE_INTENSET_ENDRX_Pos) /*!< Bit mask of ENDRX field. */
#define UARTE_INTENSET_ENDRX_Disabled (0UL) /*!< Read: Disabled */
#define UARTE_INTENSET_ENDRX_Enabled (1UL) /*!< Read: Enabled */
#define UARTE_INTENSET_ENDRX_Set (1UL) /*!< Enable */

/* Bit 2 : Write '1' to enable interrupt for event RXDRDY */
#define UARTE_INTENSET_RXDRDY_Pos (2UL) /*!< Position of RXDRDY field. */
#define UARTE_INTENSET_RXDRDY_Msk (0x1UL << UARTE_INTENSET_RXDRDY_Pos) /*!< Bit mask of RXDRDY field. */
#define UARTE_INTENSET_RXDRDY_Disabled (0UL) /*!< Read: Disabled */
#define UARTE_INTENSET_RXDRDY_Enabled (1UL) /*!< Read: Enabled */
#define UARTE_INTENSET_RXDRDY_Set (1UL) /*!< Enable */

/* Bit 1 : Write '1' to enable interrupt for event NCTS */
#define UARTE_INTENSET_NCTS_Pos (1UL) /*!< Position of NCTS field. */
#define UARTE_INTENSET_NCTS_Msk (0x1UL << UARTE_INTENSET_NCTS_Pos) /*!< Bit mask of NCTS field. */
#define UARTE_INTENSET_NCTS_Disabled (0UL) /*!< Read: Disabled */
#define UARTE_INTENSET_NCTS_Enabled (1UL) /*!< Read: Enabled */
#define UARTE_INTENSET_NCTS_Set (1UL) /*!< Enable */

/* Bit 0 : Write '1' to enable interrupt for event CTS */
#define UARTE_INTENSET_CTS_Pos (0UL) /*!< Position of CTS field. */
#define UARTE_INTENSET_CTS_Msk (0x1UL << UARTE_INTENSET_CTS_Pos) /*!< Bit mask of CTS field. */
#define UARTE_INTENSET_CTS_Disabled (0UL) /*!< Read: Disabled */
#define UARTE_INTENSET_CTS_Enabled (1UL) /*!< Read: Enabled */
#define UARTE_INTENSET_CTS_Set (1UL) /*!< Enable */

/* Register: UARTE_INTENCLR */
/* Description: Disable interrupt */

/* Bit 22 : Write '1' to disable interrupt for event TXSTOPPED */
#define UARTE_INTENCLR_TXSTOPPED_Pos (22UL) /*!< Position of TXSTOPPED field. */
#define UARTE_INTENCLR_TXSTOPPED_Msk (0x1UL << UARTE_INTENCLR_TXSTOPPED_Pos) /*!< Bit mask of TXSTOPPED field. */
#define UARTE_INTENCLR_TXSTOPPED_Disabled (0UL) /*!< Read: Disabled */
#define UARTE_INTENCLR_TXSTOPPED_Enabled (1UL) /*!< Read: Enabled */
#define UARTE_INTENCLR_TXSTOPPED_Clear (1UL) /*!< Disable */

/* Bit 20 : Write '1' to disable interrupt for event TXSTARTED */
#define UARTE_INTENCLR_TXSTARTED_Pos (20UL) /*!< Position of TXSTARTED field. */
#define UARTE_INTENCLR_TXSTARTED_Msk (0x1UL << UARTE_INTENCLR_TXSTARTED_Pos) /*!< Bit mask of TXSTARTED field. */
#define UARTE_INTENCLR_TXSTARTED_Disabled (0UL) /*!< Read: Disabled */
#define UARTE_INTENCLR_TXSTARTED_Enabled (1UL) /*!< Read: Enabled */
#define UARTE_INTENCLR_TXSTARTED_Clear (1UL) /*!< Disable */

/* Bit 19 : Write '1' to disable interrupt for event RXSTARTED */
#define UARTE_INTENCLR_RXSTARTED_Pos (19UL) /*!< Position of RXSTARTED field. */
#define UARTE_INTENCLR_RXSTARTED_Msk (0x1UL << UARTE_INTENCLR_RXSTARTED_Pos) /*!< Bit mask of RXSTARTED field. */
#define UARTE_INTENCLR_RXSTARTED_Disabled (0UL) /*!< Read: Disabled */
#define UARTE_INTENCLR_RXSTARTED_Enabled (1UL) /*!< Read: Enabled */
#define UARTE_INTENCLR_RXSTARTED_Clear (1UL) /*!< Disable */

/* Bit 17 : Write '1' to disable interrupt for event RXTO */
#define UARTE_INTENCLR_RXTO_Pos (17UL) /*!< Position of RXTO field. */
#define UARTE_INTENCLR_RXTO_Msk (0x1UL << UARTE_INTENCLR_RXTO_Pos) /*!< Bit mask of RXTO field. */
#define UARTE_INTENCLR_RXTO_Disabled (0UL) /*!< Read: Disabled */
#define UARTE_INTENCLR_RXTO_Enabled (1UL) /*!< Read: Enabled */
#define UARTE_INTENCLR_RXTO_Clear (1UL) /*!< Disable */

/* Bit 9 : Write '1' to disable interrupt for event ERROR */
#define UARTE_INTENCLR_ERROR_Pos (9UL) /*!< Position of ERROR field. */
#define UARTE_INTENCLR_ERROR_Msk (0x1UL << UARTE_INTENCLR_ERROR_Pos) /*!< Bit mask of ERROR field. */
#define UARTE_INTENCLR_ERROR_Disabled (0UL) /*!< Read: Disabled */
#define UARTE_INTENCLR_ERROR_Enabled (1UL) /*!< Read: Enabled */
#define UARTE_INTENCLR_ERROR_Clear (1UL) /*!< Disable */

/* Bit 8 : Write '1' to disable interrupt for event ENDTX */
#define UARTE_INTENCLR_ENDTX_Pos (8UL) /*!< Position of ENDTX field. */
#define UARTE_INTENCLR_ENDTX_Msk (0x1UL << UARTE_INTENCLR_ENDTX_Pos) /*!< Bit mask of ENDTX field. */
#define UARTE_INTENCLR_ENDTX_Disabled (0UL) /*!< Read: Disabled */
#define UARTE_INTENCLR_ENDTX_Enabled (1UL) /*!< Read: Enabled */
#define UARTE_INTENCLR_ENDTX_Clear (1UL) /*!< Disable */

/* Bit 7 : Write '1' to disable interrupt for event TXDRDY */
#define UARTE_INTENCLR_TXDRDY_Pos (7UL) /*!< Position of TXDRDY field. */
#define UARTE_INTENCLR_TXDRDY_Msk (0x1UL << UARTE_INTENCLR_TXDRDY_Pos) /*!< Bit mask of TXDRDY field. */
#define UARTE_INTENCLR_TXDRDY_Disabled (0UL) /*!< Read: Disabled */
#define UARTE_INTENCLR_TXDRDY_Enabled (1UL) /*!< Read: Enabled */
#define UARTE_INTENCLR_TXDRDY_Clear (1UL) /*!< Disable */

/* Bit 4 : Write '1' to disable interrupt for event ENDRX */
#define UARTE_INTENCLR_ENDRX_Pos (4UL) /*!< Position of ENDRX field. */
#define UARTE_INTENCLR_ENDRX_Msk (0x1UL << UARTE_INTENCLR_ENDRX_Pos) /*!< Bit mask of ENDRX field. */
#define UARTE_INTENCLR_ENDRX_Disabled (0UL) /*!< Read: Disabled */
#define UARTE_INTENCLR_ENDRX_Enabled (1UL) /*!< Read: Enabled */
#define UARTE_INTENCLR_ENDRX_Clear (1UL) /*!< Disable */

/* Bit 2 : Write '1' to disable interrupt for event RXDRDY */
#define UARTE_INTENCLR_RXDRDY_Pos (2UL) /*!< Position of RXDRDY field. */
#define UARTE_INTENCLR_RXDRDY_Msk (0x1UL << UARTE_INTENCLR_RXDRDY_Pos) /*!< Bit mask of RXDRDY field. */
#define UARTE_INTENCLR_RXDRDY_Disabled (0UL) /*!< Read: Disabled */
#define UARTE_INTENCLR_RXDRDY_Enabled (1UL) /*!< Read: Enabled */
#define UARTE_INTENCLR_RXDRDY_Clear (1UL) /*!< Disable */

/* Bit 1 : Write '1' to disable interrupt for event NCTS */
#define UARTE_INTENCLR_NCTS_Pos (1UL) /*!< Position of NCTS field. */
#define UARTE_INTENCLR_NCTS_Msk (0x1UL << UARTE_INTENCLR_NCTS_Pos) /*!< Bit mask of NCTS field. */
#define UARTE_INTENCLR_NCTS_Disabled (0UL) /*!< Read: Disabled */
#define UARTE_INTENCLR_NCTS_Enabled (1UL) /*!< Read: Enabled */
#define UARTE_INTENCLR_NCTS_Clear (1UL) /*!< Disable */

/* Bit 0 : Write '1' to disable interrupt for event CTS */
#define UARTE_INTENCLR_CTS_Pos (0UL) /*!< Position of CTS field. */
#define UARTE_INTENCLR_CTS_Msk (0x1UL << UARTE_INTENCLR_CTS_Pos) /*!< Bit mask of CTS field. */
#define UARTE_INTENCLR_CTS_Disabled (0UL) /*!< Read: Disabled */
#define UARTE_INTENCLR_CTS_Enabled (1UL) /*!< Read: Enabled */
#define UARTE_INTENCLR_CTS_Clear (1UL) /*!< Disable */

/* Register: UARTE_ERRORSRC */
/* Description: Error source Note : this register is read / write one to clear. */

/* Bit 3 : Break condition */
#define UARTE_ERRORSRC_BREAK_Pos (3UL) /*!< Position of BREAK field. */
#define UARTE_ERRORSRC_BREAK_Msk (0x1UL << UARTE_ERRORSRC_BREAK_Pos) /*!< Bit mask of BREAK field. */
#define UARTE_ERRORSRC_BREAK_NotPresent (0UL) /*!< Read: error not present */
#define UARTE_ERRORSRC_BREAK_Present (1UL) /*!< Read: error present */

/* Bit 2 : Framing error occurred */
#define UARTE_ERRORSRC_FRAMING_Pos (2UL) /*!< Position of FRAMING field. */
#define UARTE_ERRORSRC_FRAMING_Msk (0x1UL << UARTE_ERRORSRC_FRAMING_Pos) /*!< Bit mask of FRAMING field. */
#define UARTE_ERRORSRC_FRAMING_NotPresent (0UL) /*!< Read: error not present */
#define UARTE_ERRORSRC_FRAMING_Present (1UL) /*!< Read: error present */

/* Bit 1 : Parity error */
#define UARTE_ERRORSRC_PARITY_Pos (1UL) /*!< Position of PARITY field. */
#define UARTE_ERRORSRC_PARITY_Msk (0x1UL << UARTE_ERRORSRC_PARITY_Pos) /*!< Bit mask of PARITY field. */
#define UARTE_ERRORSRC_PARITY_NotPresent (0UL) /*!< Read: error not present */
#define UARTE_ERRORSRC_PARITY_Present (1UL) /*!< Read: error present */

/* Bit 0 : Overrun error */
#define UARTE_ERRORSRC_OVERRUN_Pos (0UL) /*!< Position of OVERRUN field. */
#define UARTE_ERRORSRC_OVERRUN_Msk (0x1UL << UARTE_ERRORSRC_OVERRUN_Pos) /*!< Bit mask of OVERRUN field. */
#define UARTE_ERRORSRC_OVERRUN_NotPresent (0UL) /*!< Read: error not present */
#define UARTE_ERRORSRC_OVERRUN_Present (1UL) /*!< Read: error present */

/* Register: UARTE_ENABLE */
/* Description: Enable UART */

/* Bits 3..0 : Enable or disable UARTE */
#define UARTE_ENABLE_ENABLE_Pos (0UL) /*!< Position of ENABLE field. */
#define UARTE_ENABLE_ENABLE_Msk (0xFUL << UARTE_ENABLE_ENABLE_Pos) /*!< Bit mask of ENABLE field. */
#define UARTE_ENABLE_ENABLE_Disabled (0UL) /*!< Disable UARTE */
#define UARTE_ENABLE_ENABLE_Enabled (8UL) /*!< Enable UARTE */

/* Register: UARTE_PSEL_RTS */
/* Description: Pin select for RTS signal */

/* Bit 31 : Connection */
#define UARTE_PSEL_RTS_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define UARTE_PSEL_RTS_CONNECT_Msk (0x1UL << UARTE_PSEL_RTS_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define UARTE_PSEL_RTS_CONNECT_Connected (0UL) /*!< Connect */
#define UARTE_PSEL_RTS_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define UARTE_PSEL_RTS_PIN_Pos (0UL) /*!< Position of PIN field. */
#define UARTE_PSEL_RTS_PIN_Msk (0x1FUL << UARTE_PSEL_RTS_PIN_Pos) /*!< Bit mask of PIN field. */

/* Register: UARTE_PSEL_TXD */
/* Description: Pin select for TXD signal */

/* Bit 31 : Connection */
#define UARTE_PSEL_TXD_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define UARTE_PSEL_TXD_CONNECT_Msk (0x1UL << UARTE_PSEL_TXD_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define UARTE_PSEL_TXD_CONNECT_Connected (0UL) /*!< Connect */
#define UARTE_PSEL_TXD_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define UARTE_PSEL_TXD_PIN_Pos (0UL) /*!< Position of PIN field. */
#define UARTE_PSEL_TXD_PIN_Msk (0x1FUL << UARTE_PSEL_TXD_PIN_Pos) /*!< Bit mask of PIN field. */

/* Register: UARTE_PSEL_CTS */
/* Description: Pin select for CTS signal */

/* Bit 31 : Connection */
#define UARTE_PSEL_CTS_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define UARTE_PSEL_CTS_CONNECT_Msk (0x1UL << UARTE_PSEL_CTS_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define UARTE_PSEL_CTS_CONNECT_Connected (0UL) /*!< Connect */
#define UARTE_PSEL_CTS_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define UARTE_PSEL_CTS_PIN_Pos (0UL) /*!< Position of PIN field. */
#define UARTE_PSEL_CTS_PIN_Msk (0x1FUL << UARTE_PSEL_CTS_PIN_Pos) /*!< Bit mask of PIN field. */

/* Register: UARTE_PSEL_RXD */
/* Description: Pin select for RXD signal */

/* Bit 31 : Connection */
#define UARTE_PSEL_RXD_CONNECT_Pos (31UL) /*!< Position of CONNECT field. */
#define UARTE_PSEL_RXD_CONNECT_Msk (0x1UL << UARTE_PSEL_RXD_CONNECT_Pos) /*!< Bit mask of CONNECT field. */
#define UARTE_PSEL_RXD_CONNECT_Connected (0UL) /*!< Connect */
#define UARTE_PSEL_RXD_CONNECT_Disconnected (1UL) /*!< Disconnect */

/* Bits 4..0 : Pin number */
#define UARTE_PSEL_RXD_PIN_Pos (0UL) /*!< Position of PIN field. */
#define UARTE_PSEL_RXD_PIN_Msk (0x1FUL << UARTE_PSEL_RXD_PIN_Pos) /*!< Bit mask of PIN field. */

/* Register: UARTE_BAUDRATE */
/* Description: Baud rate. Accuracy depends on the HFCLK source selected. */

/* Bits 31..0 : Baud rate */
#define UARTE_BAUDRATE_BAUDRATE_Pos (0UL) /*!< Position of BAUDRATE field. */
#define UARTE_BAUDRATE_BAUDRATE_Msk (0xFFFFFFFFUL << UARTE_BAUDRATE_BAUDRATE_Pos) /*!< Bit mask of BAUDRATE field. */
#define UARTE_BAUDRATE_BAUDRATE_Baud1200 (0x0004F000UL) /*!< 1200 baud (actual rate: 1205) */
#define UARTE_BAUDRATE_BAUDRATE_Baud2400 (0x0009D000UL) /*!< 2400 baud (actual rate: 2396) */
#define UARTE_BAUDRATE_BAUDRATE_Baud4800 (0x0013B000UL) /*!< 4800 baud (actual rate: 4808) */
#define UARTE_BAUDRATE_BAUDRATE_Baud9600 (0x00275000UL) /*!< 9600 baud (actual rate: 9598) */
#define UARTE_BAUDRATE_BAUDRATE_Baud14400 (0x003AF000UL) /*!< 14400 baud (actual rate: 14401) */
#define UARTE_BAUDRATE_BAUDRATE_Baud19200 (0x004EA000UL) /*!< 19200 baud (actual rate: 19208) */
#define UARTE_BAUDRATE_BAUDRATE_Baud28800 (0x0075C000UL) /*!< 28800 baud (actual rate: 28777) */
#define UARTE_BAUDRATE_BAUDRATE_Baud31250 (0x00800000UL) /*!< 31250 baud */
#define UARTE_BAUDRATE_BAUDRATE_Baud38400 (0x009D0000UL) /*!< 38400 baud (actual rate: 38369) */
#define UARTE_BAUDRATE_BAUDRATE_Baud56000 (0x00E50000UL) /*!< 56000 baud (actual rate: 55944) */
#define UARTE_BAUDRATE_BAUDRATE_Baud57600 (0x00EB0000UL) /*!< 57600 baud (actual rate: 57554) */
#define UARTE_BAUDRATE_BAUDRATE_Baud76800 (0x013A9000UL) /*!< 76800 baud (actual rate: 76923) */
#define UARTE_BAUDRATE_BAUDRATE_Baud115200 (0x01D60000UL) /*!< 115200 baud (actual rate: 115108) */
#define UARTE_BAUDRATE_BAUDRATE_Baud230400 (0x03B00000UL) /*!< 230400 baud (actual rate: 231884) */
#define UARTE_BAUDRATE_BAUDRATE_Baud250000 (0x04000000UL) /*!< 250000 baud */
#define UARTE_BAUDRATE_BAUDRATE_Baud460800 (0x07400000UL) /*!< 460800 baud (actual rate: 457143) */
#define UARTE_BAUDRATE_BAUDRATE_Baud921600 (0x0F000000UL) /*!< 921600 baud (actual rate: 941176) */
#define UARTE_BAUDRATE_BAUDRATE_Baud1M (0x10000000UL) /*!< 1Mega baud */

/* Register: UARTE_RXD_PTR */
/* Description: Data pointer */

/* Bits 31..0 : Data pointer */
#define UARTE_RXD_PTR_PTR_Pos (0UL) /*!< Position of PTR field. */
#define UARTE_RXD_PTR_PTR_Msk (0xFFFFFFFFUL << UARTE_RXD_PTR_PTR_Pos) /*!< Bit mask of PTR field. */

/* Register: UARTE_RXD_MAXCNT */
/* Description: Maximum number of bytes in receive buffer */

/* Bits 12..0 : Maximum number of bytes in receive buffer */
#define UARTE_RXD_MAXCNT_MAXCNT_Pos (0UL) /*!< Position of MAXCNT field. */
#define UARTE_RXD_MAXCNT_MAXCNT_Msk (0x1FFFUL << UARTE_RXD_MAXCNT_MAXCNT_Pos) /*!< Bit mask of MAXCNT field. */

/* Register: UARTE_RXD_AMOUNT */
/* Description: Number of bytes transferred in the last transaction */

/* Bits 12..0 : Number of bytes transferred in the last transaction */
#define UARTE_RXD_AMOUNT_AMOUNT_Pos (0UL) /*!< Position of AMOUNT field. */
#define UARTE_RXD_AMOUNT_AMOUNT_Msk (0x1FFFUL << UARTE_RXD_AMOUNT_AMOUNT_Pos) /*!< Bit mask of AMOUNT field. */

/* Register: UARTE_TXD_PTR */
/* Description: Data pointer */

/* Bits 31..0 : Data pointer */
#define UARTE_TXD_PTR_PTR_Pos (0UL) /*!< Position of PTR field. */
#define UARTE_TXD_PTR_PTR_Msk (0xFFFFFFFFUL << UARTE_TXD_PTR_PTR_Pos) /*!< Bit mask of PTR field. */

/* Register: UARTE_TXD_MAXCNT */
/* Description: Maximum number of bytes in transmit buffer */

/* Bits 12..0 : Maximum number of bytes in transmit buffer */
#define UARTE_TXD_MAXCNT_MAXCNT_Pos (0UL) /*!< Position of MAXCNT field. */
#define UARTE_TXD_MAXCNT_MAXCNT_Msk (0x1FFFUL << UARTE_TXD_MAXCNT_MAXCNT_Pos) /*!< Bit mask of MAXCNT field. */

/* Register: UARTE_TXD_AMOUNT */
/* Description: Number of bytes transferred in the last transaction */

/* Bits 12..0 : Number of bytes transferred in the last transaction */
#define UARTE_TXD_AMOUNT_AMOUNT_Pos (0UL) /*!< Position of AMOUNT field. */
#define UARTE_TXD_AMOUNT_AMOUNT_Msk (0x1FFFUL << UARTE_TXD_AMOUNT_AMOUNT_Pos) /*!< Bit mask of AMOUNT field. */

/* Register: UARTE_CONFIG */
/* Description: Configuration of parity and hardware flow control */

/* Bit 4 : Stop bits */
#define UARTE_CONFIG_STOP_Pos (4UL) /*!< Position of STOP field. */
#define UARTE_CONFIG_STOP_Msk (0x1UL << UARTE_CONFIG_STOP_Pos) /*!< Bit mask of STOP field. */
#define UARTE_CONFIG_STOP_One (0UL) /*!< One stop bit */
#define UARTE_CONFIG_STOP_Two (1UL) /*!< Two stop bits */

/* Bits 3..1 : Parity */
#define UARTE_CONFIG_PARITY_Pos (1UL) /*!< Position of PARITY field. */
#define UARTE_CONFIG_PARITY_Msk (0x7UL << UARTE_CONFIG_PARITY_Pos) /*!< Bit mask of PARITY field. */
#define UARTE_CONFIG_PARITY_Excluded (0x0UL) /*!< Exclude parity bit */
#define UARTE_CONFIG_PARITY_Included (0x7UL) /*!< Include even parity bit */

/* Bit 0 : Hardware flow control */
#define UARTE_CONFIG_HWFC_Pos (0UL) /*!< Position of HWFC field. */
#define UARTE_CONFIG_HWFC_Msk (0x1UL << UARTE_CONFIG_HWFC_Pos) /*!< Bit mask of HWFC field. */
#define UARTE_CONFIG_HWFC_Disabled (0UL) /*!< Disabled */
#define UARTE_CONFIG_HWFC_Enabled (1UL) /*!< Enabled */


/* Peripheral: UICR */
/* Description: User information configuration registers User information configuration registers */

/* Register: UICR_APPROTECT */
/* Description: Access port protection */

/* Bits 31..0 : Blocks debugger read/write access to all CPU registers and
          memory mapped addresses */
#define UICR_APPROTECT_PALL_Pos (0UL) /*!< Position of PALL field. */
#define UICR_APPROTECT_PALL_Msk (0xFFFFFFFFUL << UICR_APPROTECT_PALL_Pos) /*!< Bit mask of PALL field. */
#define UICR_APPROTECT_PALL_Protected (0x00000000UL) /*!< Protected */
#define UICR_APPROTECT_PALL_Unprotected (0xFFFFFFFFUL) /*!< Unprotected */

/* Register: UICR_XOSC32M */
/* Description: Oscillator control */

/* Bits 5..0 : Pierce current DAC control signals */
#define UICR_XOSC32M_CTRL_Pos (0UL) /*!< Position of CTRL field. */
#define UICR_XOSC32M_CTRL_Msk (0x3FUL << UICR_XOSC32M_CTRL_Pos) /*!< Bit mask of CTRL field. */

/* Register: UICR_HFXOSRC */
/* Description: HFXO clock source selection */

/* Bit 0 : HFXO clock source selection */
#define UICR_HFXOSRC_HFXOSRC_Pos (0UL) /*!< Position of HFXOSRC field. */
#define UICR_HFXOSRC_HFXOSRC_Msk (0x1UL << UICR_HFXOSRC_HFXOSRC_Pos) /*!< Bit mask of HFXOSRC field. */
#define UICR_HFXOSRC_HFXOSRC_TCXO (0UL) /*!< 32 MHz temperature compensated crystal oscillator (TCXO) */
#define UICR_HFXOSRC_HFXOSRC_XTAL (1UL) /*!< 32 MHz crystal oscillator */

/* Register: UICR_HFXOCNT */
/* Description: HFXO startup counter */

/* Bits 7..0 : HFXO startup counter. Total debounce time = HFXOCNT*64 us + 0.5 us */
#define UICR_HFXOCNT_HFXOCNT_Pos (0UL) /*!< Position of HFXOCNT field. */
#define UICR_HFXOCNT_HFXOCNT_Msk (0xFFUL << UICR_HFXOCNT_HFXOCNT_Pos) /*!< Bit mask of HFXOCNT field. */
#define UICR_HFXOCNT_HFXOCNT_MinDebounceTime (0UL) /*!< Min debounce time = (0*64 us + 0.5 us) */
#define UICR_HFXOCNT_HFXOCNT_MaxDebounceTime (255UL) /*!< Max debounce time = (255*64 us + 0.5 us) */

/* Register: UICR_SECUREAPPROTECT */
/* Description: Secure access port protection */

/* Bits 31..0 : Blocks debugger read/write access to all secure CPU registers and secure
          memory mapped addresses */
#define UICR_SECUREAPPROTECT_PALL_Pos (0UL) /*!< Position of PALL field. */
#define UICR_SECUREAPPROTECT_PALL_Msk (0xFFFFFFFFUL << UICR_SECUREAPPROTECT_PALL_Pos) /*!< Bit mask of PALL field. */
#define UICR_SECUREAPPROTECT_PALL_Protected (0x00000000UL) /*!< Protected */
#define UICR_SECUREAPPROTECT_PALL_Unprotected (0xFFFFFFFFUL) /*!< Unprotected */

/* Register: UICR_ERASEPROTECT */
/* Description: Erase protection */

/* Bits 31..0 : Blocks NVMC ERASEALL and CTRLAP ERASEALL functionality */
#define UICR_ERASEPROTECT_PALL_Pos (0UL) /*!< Position of PALL field. */
#define UICR_ERASEPROTECT_PALL_Msk (0xFFFFFFFFUL << UICR_ERASEPROTECT_PALL_Pos) /*!< Bit mask of PALL field. */
#define UICR_ERASEPROTECT_PALL_Protected (0x00000000UL) /*!< Protected */
#define UICR_ERASEPROTECT_PALL_Unprotected (0xFFFFFFFFUL) /*!< Unprotected */

/* Register: UICR_OTP */
/* Description: Description collection: OTP bits [31+n*32:0+n*32]. */

/* Bits 31..0 : Bits [31+n*32:0+n*32] of OTP region */
#define UICR_OTP_OTP_Pos (0UL) /*!< Position of OTP field. */
#define UICR_OTP_OTP_Msk (0xFFFFFFFFUL << UICR_OTP_OTP_Pos) /*!< Bit mask of OTP field. */

/* Register: UICR_KEYSLOT_CONFIG_DEST */
/* Description: Description cluster: Destination address where content of the key value registers (KEYSLOT.KEYn.VALUE[0-3])
          will be pushed by KMU. Note that this address MUST match that of a peripherals
          APB mapped write-only key registers, else the KMU can push this key value into
          an address range which the CPU can potentially read! */

/* Bits 31..0 : Secure APB destination address */
#define UICR_KEYSLOT_CONFIG_DEST_DEST_Pos (0UL) /*!< Position of DEST field. */
#define UICR_KEYSLOT_CONFIG_DEST_DEST_Msk (0xFFFFFFFFUL << UICR_KEYSLOT_CONFIG_DEST_DEST_Pos) /*!< Bit mask of DEST field. */

/* Register: UICR_KEYSLOT_CONFIG_PERM */
/* Description: Description cluster: Define permissions for the key slot with ID=n+1. Bits 0-15 and 16-31 can only be written once. */

/* Bit 16 : Revocation state for the key slot */
#define UICR_KEYSLOT_CONFIG_PERM_STATE_Pos (16UL) /*!< Position of STATE field. */
#define UICR_KEYSLOT_CONFIG_PERM_STATE_Msk (0x1UL << UICR_KEYSLOT_CONFIG_PERM_STATE_Pos) /*!< Bit mask of STATE field. */
#define UICR_KEYSLOT_CONFIG_PERM_STATE_Revoked (0UL) /*!< Key value registers can no longer be read or pushed */
#define UICR_KEYSLOT_CONFIG_PERM_STATE_Active (1UL) /*!< Key value registers are readable (if enabled) and can be pushed (if enabled) */

/* Bit 2 : Push permission for key slot */
#define UICR_KEYSLOT_CONFIG_PERM_PUSH_Pos (2UL) /*!< Position of PUSH field. */
#define UICR_KEYSLOT_CONFIG_PERM_PUSH_Msk (0x1UL << UICR_KEYSLOT_CONFIG_PERM_PUSH_Pos) /*!< Bit mask of PUSH field. */
#define UICR_KEYSLOT_CONFIG_PERM_PUSH_Disabled (0UL) /*!< Disable pushing of key value registers over secure APB, but can be read if field READ is Enabled */
#define UICR_KEYSLOT_CONFIG_PERM_PUSH_Enabled (1UL) /*!< Enable pushing of key value registers over secure APB. Register KEYSLOT.CONFIGn.DEST must contain a valid destination address! */

/* Bit 1 : Read permission for key slot */
#define UICR_KEYSLOT_CONFIG_PERM_READ_Pos (1UL) /*!< Position of READ field. */
#define UICR_KEYSLOT_CONFIG_PERM_READ_Msk (0x1UL << UICR_KEYSLOT_CONFIG_PERM_READ_Pos) /*!< Bit mask of READ field. */
#define UICR_KEYSLOT_CONFIG_PERM_READ_Disabled (0UL) /*!< Disable read from key value registers */
#define UICR_KEYSLOT_CONFIG_PERM_READ_Enabled (1UL) /*!< Enable read from key value registers */

/* Bit 0 : Write permission for key slot */
#define UICR_KEYSLOT_CONFIG_PERM_WRITE_Pos (0UL) /*!< Position of WRITE field. */
#define UICR_KEYSLOT_CONFIG_PERM_WRITE_Msk (0x1UL << UICR_KEYSLOT_CONFIG_PERM_WRITE_Pos) /*!< Bit mask of WRITE field. */
#define UICR_KEYSLOT_CONFIG_PERM_WRITE_Disabled (0UL) /*!< Disable write to the key value registers */
#define UICR_KEYSLOT_CONFIG_PERM_WRITE_Enabled (1UL) /*!< Enable write to the key value registers */

/* Register: UICR_KEYSLOT_KEY_VALUE */
/* Description: Description collection: Define bits [31+o*32:0+o*32] of value assigned to KMU key slot ID=n+1 */

/* Bits 31..0 : Define bits [31+o*32:0+o*32] of value assigned to KMU key slot ID=n+1 */
#define UICR_KEYSLOT_KEY_VALUE_VALUE_Pos (0UL) /*!< Position of VALUE field. */
#define UICR_KEYSLOT_KEY_VALUE_VALUE_Msk (0xFFFFFFFFUL << UICR_KEYSLOT_KEY_VALUE_VALUE_Pos) /*!< Bit mask of VALUE field. */


/* Peripheral: VMC */
/* Description: Volatile Memory controller 0 */

/* Register: VMC_RAM_POWER */
/* Description: Description cluster: RAMn power control register */

/* Bit 19 : Keep retention on RAM section S3 of RAM n when RAM section is switched off */
#define VMC_RAM_POWER_S3RETENTION_Pos (19UL) /*!< Position of S3RETENTION field. */
#define VMC_RAM_POWER_S3RETENTION_Msk (0x1UL << VMC_RAM_POWER_S3RETENTION_Pos) /*!< Bit mask of S3RETENTION field. */
#define VMC_RAM_POWER_S3RETENTION_Off (0UL) /*!< Off */
#define VMC_RAM_POWER_S3RETENTION_On (1UL) /*!< On */

/* Bit 18 : Keep retention on RAM section S2 of RAM n when RAM section is switched off */
#define VMC_RAM_POWER_S2RETENTION_Pos (18UL) /*!< Position of S2RETENTION field. */
#define VMC_RAM_POWER_S2RETENTION_Msk (0x1UL << VMC_RAM_POWER_S2RETENTION_Pos) /*!< Bit mask of S2RETENTION field. */
#define VMC_RAM_POWER_S2RETENTION_Off (0UL) /*!< Off */
#define VMC_RAM_POWER_S2RETENTION_On (1UL) /*!< On */

/* Bit 17 : Keep retention on RAM section S1 of RAM n when RAM section is switched off */
#define VMC_RAM_POWER_S1RETENTION_Pos (17UL) /*!< Position of S1RETENTION field. */
#define VMC_RAM_POWER_S1RETENTION_Msk (0x1UL << VMC_RAM_POWER_S1RETENTION_Pos) /*!< Bit mask of S1RETENTION field. */
#define VMC_RAM_POWER_S1RETENTION_Off (0UL) /*!< Off */
#define VMC_RAM_POWER_S1RETENTION_On (1UL) /*!< On */

/* Bit 16 : Keep retention on RAM section S0 of RAM n when RAM section is switched off */
#define VMC_RAM_POWER_S0RETENTION_Pos (16UL) /*!< Position of S0RETENTION field. */
#define VMC_RAM_POWER_S0RETENTION_Msk (0x1UL << VMC_RAM_POWER_S0RETENTION_Pos) /*!< Bit mask of S0RETENTION field. */
#define VMC_RAM_POWER_S0RETENTION_Off (0UL) /*!< Off */
#define VMC_RAM_POWER_S0RETENTION_On (1UL) /*!< On */

/* Bit 3 : Keep RAM section S3 of RAM n on or off in System ON mode */
#define VMC_RAM_POWER_S3POWER_Pos (3UL) /*!< Position of S3POWER field. */
#define VMC_RAM_POWER_S3POWER_Msk (0x1UL << VMC_RAM_POWER_S3POWER_Pos) /*!< Bit mask of S3POWER field. */
#define VMC_RAM_POWER_S3POWER_Off (0UL) /*!< Off */
#define VMC_RAM_POWER_S3POWER_On (1UL) /*!< On */

/* Bit 2 : Keep RAM section S2 of RAM n on or off in System ON mode */
#define VMC_RAM_POWER_S2POWER_Pos (2UL) /*!< Position of S2POWER field. */
#define VMC_RAM_POWER_S2POWER_Msk (0x1UL << VMC_RAM_POWER_S2POWER_Pos) /*!< Bit mask of S2POWER field. */
#define VMC_RAM_POWER_S2POWER_Off (0UL) /*!< Off */
#define VMC_RAM_POWER_S2POWER_On (1UL) /*!< On */

/* Bit 1 : Keep RAM section S1 of RAM n on or off in System ON mode */
#define VMC_RAM_POWER_S1POWER_Pos (1UL) /*!< Position of S1POWER field. */
#define VMC_RAM_POWER_S1POWER_Msk (0x1UL << VMC_RAM_POWER_S1POWER_Pos) /*!< Bit mask of S1POWER field. */
#define VMC_RAM_POWER_S1POWER_Off (0UL) /*!< Off */
#define VMC_RAM_POWER_S1POWER_On (1UL) /*!< On */

/* Bit 0 : Keep RAM section S0 of RAM n on or off in System ON mode */
#define VMC_RAM_POWER_S0POWER_Pos (0UL) /*!< Position of S0POWER field. */
#define VMC_RAM_POWER_S0POWER_Msk (0x1UL << VMC_RAM_POWER_S0POWER_Pos) /*!< Bit mask of S0POWER field. */
#define VMC_RAM_POWER_S0POWER_Off (0UL) /*!< Off */
#define VMC_RAM_POWER_S0POWER_On (1UL) /*!< On */

/* Register: VMC_RAM_POWERSET */
/* Description: Description cluster: RAMn power control set register */

/* Bit 19 : Keep retention on RAM section S3 of RAM n when RAM section is switched off */
#define VMC_RAM_POWERSET_S3RETENTION_Pos (19UL) /*!< Position of S3RETENTION field. */
#define VMC_RAM_POWERSET_S3RETENTION_Msk (0x1UL << VMC_RAM_POWERSET_S3RETENTION_Pos) /*!< Bit mask of S3RETENTION field. */
#define VMC_RAM_POWERSET_S3RETENTION_On (1UL) /*!< On */

/* Bit 18 : Keep retention on RAM section S2 of RAM n when RAM section is switched off */
#define VMC_RAM_POWERSET_S2RETENTION_Pos (18UL) /*!< Position of S2RETENTION field. */
#define VMC_RAM_POWERSET_S2RETENTION_Msk (0x1UL << VMC_RAM_POWERSET_S2RETENTION_Pos) /*!< Bit mask of S2RETENTION field. */
#define VMC_RAM_POWERSET_S2RETENTION_On (1UL) /*!< On */

/* Bit 17 : Keep retention on RAM section S1 of RAM n when RAM section is switched off */
#define VMC_RAM_POWERSET_S1RETENTION_Pos (17UL) /*!< Position of S1RETENTION field. */
#define VMC_RAM_POWERSET_S1RETENTION_Msk (0x1UL << VMC_RAM_POWERSET_S1RETENTION_Pos) /*!< Bit mask of S1RETENTION field. */
#define VMC_RAM_POWERSET_S1RETENTION_On (1UL) /*!< On */

/* Bit 16 : Keep retention on RAM section S0 of RAM n when RAM section is switched off */
#define VMC_RAM_POWERSET_S0RETENTION_Pos (16UL) /*!< Position of S0RETENTION field. */
#define VMC_RAM_POWERSET_S0RETENTION_Msk (0x1UL << VMC_RAM_POWERSET_S0RETENTION_Pos) /*!< Bit mask of S0RETENTION field. */
#define VMC_RAM_POWERSET_S0RETENTION_On (1UL) /*!< On */

/* Bit 3 : Keep RAM section S3 of RAM n on or off in System ON mode */
#define VMC_RAM_POWERSET_S3POWER_Pos (3UL) /*!< Position of S3POWER field. */
#define VMC_RAM_POWERSET_S3POWER_Msk (0x1UL << VMC_RAM_POWERSET_S3POWER_Pos) /*!< Bit mask of S3POWER field. */
#define VMC_RAM_POWERSET_S3POWER_On (1UL) /*!< On */

/* Bit 2 : Keep RAM section S2 of RAM n on or off in System ON mode */
#define VMC_RAM_POWERSET_S2POWER_Pos (2UL) /*!< Position of S2POWER field. */
#define VMC_RAM_POWERSET_S2POWER_Msk (0x1UL << VMC_RAM_POWERSET_S2POWER_Pos) /*!< Bit mask of S2POWER field. */
#define VMC_RAM_POWERSET_S2POWER_On (1UL) /*!< On */

/* Bit 1 : Keep RAM section S1 of RAM n on or off in System ON mode */
#define VMC_RAM_POWERSET_S1POWER_Pos (1UL) /*!< Position of S1POWER field. */
#define VMC_RAM_POWERSET_S1POWER_Msk (0x1UL << VMC_RAM_POWERSET_S1POWER_Pos) /*!< Bit mask of S1POWER field. */
#define VMC_RAM_POWERSET_S1POWER_On (1UL) /*!< On */

/* Bit 0 : Keep RAM section S0 of RAM n on or off in System ON mode */
#define VMC_RAM_POWERSET_S0POWER_Pos (0UL) /*!< Position of S0POWER field. */
#define VMC_RAM_POWERSET_S0POWER_Msk (0x1UL << VMC_RAM_POWERSET_S0POWER_Pos) /*!< Bit mask of S0POWER field. */
#define VMC_RAM_POWERSET_S0POWER_On (1UL) /*!< On */

/* Register: VMC_RAM_POWERCLR */
/* Description: Description cluster: RAMn power control clear register */

/* Bit 19 : Keep retention on RAM section S3 of RAM n when RAM section is switched off */
#define VMC_RAM_POWERCLR_S3RETENTION_Pos (19UL) /*!< Position of S3RETENTION field. */
#define VMC_RAM_POWERCLR_S3RETENTION_Msk (0x1UL << VMC_RAM_POWERCLR_S3RETENTION_Pos) /*!< Bit mask of S3RETENTION field. */
#define VMC_RAM_POWERCLR_S3RETENTION_Off (1UL) /*!< Off */

/* Bit 18 : Keep retention on RAM section S2 of RAM n when RAM section is switched off */
#define VMC_RAM_POWERCLR_S2RETENTION_Pos (18UL) /*!< Position of S2RETENTION field. */
#define VMC_RAM_POWERCLR_S2RETENTION_Msk (0x1UL << VMC_RAM_POWERCLR_S2RETENTION_Pos) /*!< Bit mask of S2RETENTION field. */
#define VMC_RAM_POWERCLR_S2RETENTION_Off (1UL) /*!< Off */

/* Bit 17 : Keep retention on RAM section S1 of RAM n when RAM section is switched off */
#define VMC_RAM_POWERCLR_S1RETENTION_Pos (17UL) /*!< Position of S1RETENTION field. */
#define VMC_RAM_POWERCLR_S1RETENTION_Msk (0x1UL << VMC_RAM_POWERCLR_S1RETENTION_Pos) /*!< Bit mask of S1RETENTION field. */
#define VMC_RAM_POWERCLR_S1RETENTION_Off (1UL) /*!< Off */

/* Bit 16 : Keep retention on RAM section S0 of RAM n when RAM section is switched off */
#define VMC_RAM_POWERCLR_S0RETENTION_Pos (16UL) /*!< Position of S0RETENTION field. */
#define VMC_RAM_POWERCLR_S0RETENTION_Msk (0x1UL << VMC_RAM_POWERCLR_S0RETENTION_Pos) /*!< Bit mask of S0RETENTION field. */
#define VMC_RAM_POWERCLR_S0RETENTION_Off (1UL) /*!< Off */

/* Bit 3 : Keep RAM section S3 of RAM n on or off in System ON mode */
#define VMC_RAM_POWERCLR_S3POWER_Pos (3UL) /*!< Position of S3POWER field. */
#define VMC_RAM_POWERCLR_S3POWER_Msk (0x1UL << VMC_RAM_POWERCLR_S3POWER_Pos) /*!< Bit mask of S3POWER field. */
#define VMC_RAM_POWERCLR_S3POWER_Off (1UL) /*!< Off */

/* Bit 2 : Keep RAM section S2 of RAM n on or off in System ON mode */
#define VMC_RAM_POWERCLR_S2POWER_Pos (2UL) /*!< Position of S2POWER field. */
#define VMC_RAM_POWERCLR_S2POWER_Msk (0x1UL << VMC_RAM_POWERCLR_S2POWER_Pos) /*!< Bit mask of S2POWER field. */
#define VMC_RAM_POWERCLR_S2POWER_Off (1UL) /*!< Off */

/* Bit 1 : Keep RAM section S1 of RAM n on or off in System ON mode */
#define VMC_RAM_POWERCLR_S1POWER_Pos (1UL) /*!< Position of S1POWER field. */
#define VMC_RAM_POWERCLR_S1POWER_Msk (0x1UL << VMC_RAM_POWERCLR_S1POWER_Pos) /*!< Bit mask of S1POWER field. */
#define VMC_RAM_POWERCLR_S1POWER_Off (1UL) /*!< Off */

/* Bit 0 : Keep RAM section S0 of RAM n on or off in System ON mode */
#define VMC_RAM_POWERCLR_S0POWER_Pos (0UL) /*!< Position of S0POWER field. */
#define VMC_RAM_POWERCLR_S0POWER_Msk (0x1UL << VMC_RAM_POWERCLR_S0POWER_Pos) /*!< Bit mask of S0POWER field. */
#define VMC_RAM_POWERCLR_S0POWER_Off (1UL) /*!< Off */


/* Peripheral: WDT */
/* Description: Watchdog Timer 0 */

/* Register: WDT_TASKS_START */
/* Description: Start the watchdog */

/* Bit 0 : Start the watchdog */
#define WDT_TASKS_START_TASKS_START_Pos (0UL) /*!< Position of TASKS_START field. */
#define WDT_TASKS_START_TASKS_START_Msk (0x1UL << WDT_TASKS_START_TASKS_START_Pos) /*!< Bit mask of TASKS_START field. */
#define WDT_TASKS_START_TASKS_START_Trigger (1UL) /*!< Trigger task */

/* Register: WDT_SUBSCRIBE_START */
/* Description: Subscribe configuration for task START */

/* Bit 31 :   */
#define WDT_SUBSCRIBE_START_EN_Pos (31UL) /*!< Position of EN field. */
#define WDT_SUBSCRIBE_START_EN_Msk (0x1UL << WDT_SUBSCRIBE_START_EN_Pos) /*!< Bit mask of EN field. */
#define WDT_SUBSCRIBE_START_EN_Disabled (0UL) /*!< Disable subscription */
#define WDT_SUBSCRIBE_START_EN_Enabled (1UL) /*!< Enable subscription */

/* Bits 3..0 : Channel that task START will subscribe to */
#define WDT_SUBSCRIBE_START_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define WDT_SUBSCRIBE_START_CHIDX_Msk (0xFUL << WDT_SUBSCRIBE_START_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: WDT_EVENTS_TIMEOUT */
/* Description: Watchdog timeout */

/* Bit 0 : Watchdog timeout */
#define WDT_EVENTS_TIMEOUT_EVENTS_TIMEOUT_Pos (0UL) /*!< Position of EVENTS_TIMEOUT field. */
#define WDT_EVENTS_TIMEOUT_EVENTS_TIMEOUT_Msk (0x1UL << WDT_EVENTS_TIMEOUT_EVENTS_TIMEOUT_Pos) /*!< Bit mask of EVENTS_TIMEOUT field. */
#define WDT_EVENTS_TIMEOUT_EVENTS_TIMEOUT_NotGenerated (0UL) /*!< Event not generated */
#define WDT_EVENTS_TIMEOUT_EVENTS_TIMEOUT_Generated (1UL) /*!< Event generated */

/* Register: WDT_PUBLISH_TIMEOUT */
/* Description: Publish configuration for event TIMEOUT */

/* Bit 31 :   */
#define WDT_PUBLISH_TIMEOUT_EN_Pos (31UL) /*!< Position of EN field. */
#define WDT_PUBLISH_TIMEOUT_EN_Msk (0x1UL << WDT_PUBLISH_TIMEOUT_EN_Pos) /*!< Bit mask of EN field. */
#define WDT_PUBLISH_TIMEOUT_EN_Disabled (0UL) /*!< Disable publishing */
#define WDT_PUBLISH_TIMEOUT_EN_Enabled (1UL) /*!< Enable publishing */

/* Bits 3..0 : Channel that event TIMEOUT will publish to. */
#define WDT_PUBLISH_TIMEOUT_CHIDX_Pos (0UL) /*!< Position of CHIDX field. */
#define WDT_PUBLISH_TIMEOUT_CHIDX_Msk (0xFUL << WDT_PUBLISH_TIMEOUT_CHIDX_Pos) /*!< Bit mask of CHIDX field. */

/* Register: WDT_INTENSET */
/* Description: Enable interrupt */

/* Bit 0 : Write '1' to enable interrupt for event TIMEOUT */
#define WDT_INTENSET_TIMEOUT_Pos (0UL) /*!< Position of TIMEOUT field. */
#define WDT_INTENSET_TIMEOUT_Msk (0x1UL << WDT_INTENSET_TIMEOUT_Pos) /*!< Bit mask of TIMEOUT field. */
#define WDT_INTENSET_TIMEOUT_Disabled (0UL) /*!< Read: Disabled */
#define WDT_INTENSET_TIMEOUT_Enabled (1UL) /*!< Read: Enabled */
#define WDT_INTENSET_TIMEOUT_Set (1UL) /*!< Enable */

/* Register: WDT_INTENCLR */
/* Description: Disable interrupt */

/* Bit 0 : Write '1' to disable interrupt for event TIMEOUT */
#define WDT_INTENCLR_TIMEOUT_Pos (0UL) /*!< Position of TIMEOUT field. */
#define WDT_INTENCLR_TIMEOUT_Msk (0x1UL << WDT_INTENCLR_TIMEOUT_Pos) /*!< Bit mask of TIMEOUT field. */
#define WDT_INTENCLR_TIMEOUT_Disabled (0UL) /*!< Read: Disabled */
#define WDT_INTENCLR_TIMEOUT_Enabled (1UL) /*!< Read: Enabled */
#define WDT_INTENCLR_TIMEOUT_Clear (1UL) /*!< Disable */

/* Register: WDT_RUNSTATUS */
/* Description: Run status */

/* Bit 0 : Indicates whether or not the watchdog is running */
#define WDT_RUNSTATUS_RUNSTATUSWDT_Pos (0UL) /*!< Position of RUNSTATUSWDT field. */
#define WDT_RUNSTATUS_RUNSTATUSWDT_Msk (0x1UL << WDT_RUNSTATUS_RUNSTATUSWDT_Pos) /*!< Bit mask of RUNSTATUSWDT field. */
#define WDT_RUNSTATUS_RUNSTATUSWDT_NotRunning (0UL) /*!< Watchdog not running */
#define WDT_RUNSTATUS_RUNSTATUSWDT_Running (1UL) /*!< Watchdog is running */

/* Register: WDT_REQSTATUS */
/* Description: Request status */

/* Bit 7 : Request status for RR[7] register */
#define WDT_REQSTATUS_RR7_Pos (7UL) /*!< Position of RR7 field. */
#define WDT_REQSTATUS_RR7_Msk (0x1UL << WDT_REQSTATUS_RR7_Pos) /*!< Bit mask of RR7 field. */
#define WDT_REQSTATUS_RR7_DisabledOrRequested (0UL) /*!< RR[7] register is not enabled, or are already requesting reload */
#define WDT_REQSTATUS_RR7_EnabledAndUnrequested (1UL) /*!< RR[7] register is enabled, and are not yet requesting reload */

/* Bit 6 : Request status for RR[6] register */
#define WDT_REQSTATUS_RR6_Pos (6UL) /*!< Position of RR6 field. */
#define WDT_REQSTATUS_RR6_Msk (0x1UL << WDT_REQSTATUS_RR6_Pos) /*!< Bit mask of RR6 field. */
#define WDT_REQSTATUS_RR6_DisabledOrRequested (0UL) /*!< RR[6] register is not enabled, or are already requesting reload */
#define WDT_REQSTATUS_RR6_EnabledAndUnrequested (1UL) /*!< RR[6] register is enabled, and are not yet requesting reload */

/* Bit 5 : Request status for RR[5] register */
#define WDT_REQSTATUS_RR5_Pos (5UL) /*!< Position of RR5 field. */
#define WDT_REQSTATUS_RR5_Msk (0x1UL << WDT_REQSTATUS_RR5_Pos) /*!< Bit mask of RR5 field. */
#define WDT_REQSTATUS_RR5_DisabledOrRequested (0UL) /*!< RR[5] register is not enabled, or are already requesting reload */
#define WDT_REQSTATUS_RR5_EnabledAndUnrequested (1UL) /*!< RR[5] register is enabled, and are not yet requesting reload */

/* Bit 4 : Request status for RR[4] register */
#define WDT_REQSTATUS_RR4_Pos (4UL) /*!< Position of RR4 field. */
#define WDT_REQSTATUS_RR4_Msk (0x1UL << WDT_REQSTATUS_RR4_Pos) /*!< Bit mask of RR4 field. */
#define WDT_REQSTATUS_RR4_DisabledOrRequested (0UL) /*!< RR[4] register is not enabled, or are already requesting reload */
#define WDT_REQSTATUS_RR4_EnabledAndUnrequested (1UL) /*!< RR[4] register is enabled, and are not yet requesting reload */

/* Bit 3 : Request status for RR[3] register */
#define WDT_REQSTATUS_RR3_Pos (3UL) /*!< Position of RR3 field. */
#define WDT_REQSTATUS_RR3_Msk (0x1UL << WDT_REQSTATUS_RR3_Pos) /*!< Bit mask of RR3 field. */
#define WDT_REQSTATUS_RR3_DisabledOrRequested (0UL) /*!< RR[3] register is not enabled, or are already requesting reload */
#define WDT_REQSTATUS_RR3_EnabledAndUnrequested (1UL) /*!< RR[3] register is enabled, and are not yet requesting reload */

/* Bit 2 : Request status for RR[2] register */
#define WDT_REQSTATUS_RR2_Pos (2UL) /*!< Position of RR2 field. */
#define WDT_REQSTATUS_RR2_Msk (0x1UL << WDT_REQSTATUS_RR2_Pos) /*!< Bit mask of RR2 field. */
#define WDT_REQSTATUS_RR2_DisabledOrRequested (0UL) /*!< RR[2] register is not enabled, or are already requesting reload */
#define WDT_REQSTATUS_RR2_EnabledAndUnrequested (1UL) /*!< RR[2] register is enabled, and are not yet requesting reload */

/* Bit 1 : Request status for RR[1] register */
#define WDT_REQSTATUS_RR1_Pos (1UL) /*!< Position of RR1 field. */
#define WDT_REQSTATUS_RR1_Msk (0x1UL << WDT_REQSTATUS_RR1_Pos) /*!< Bit mask of RR1 field. */
#define WDT_REQSTATUS_RR1_DisabledOrRequested (0UL) /*!< RR[1] register is not enabled, or are already requesting reload */
#define WDT_REQSTATUS_RR1_EnabledAndUnrequested (1UL) /*!< RR[1] register is enabled, and are not yet requesting reload */

/* Bit 0 : Request status for RR[0] register */
#define WDT_REQSTATUS_RR0_Pos (0UL) /*!< Position of RR0 field. */
#define WDT_REQSTATUS_RR0_Msk (0x1UL << WDT_REQSTATUS_RR0_Pos) /*!< Bit mask of RR0 field. */
#define WDT_REQSTATUS_RR0_DisabledOrRequested (0UL) /*!< RR[0] register is not enabled, or are already requesting reload */
#define WDT_REQSTATUS_RR0_EnabledAndUnrequested (1UL) /*!< RR[0] register is enabled, and are not yet requesting reload */

/* Register: WDT_CRV */
/* Description: Counter reload value */

/* Bits 31..0 : Counter reload value in number of cycles of the 32.768 kHz clock */
#define WDT_CRV_CRV_Pos (0UL) /*!< Position of CRV field. */
#define WDT_CRV_CRV_Msk (0xFFFFFFFFUL << WDT_CRV_CRV_Pos) /*!< Bit mask of CRV field. */

/* Register: WDT_RREN */
/* Description: Enable register for reload request registers */

/* Bit 7 : Enable or disable RR[7] register */
#define WDT_RREN_RR7_Pos (7UL) /*!< Position of RR7 field. */
#define WDT_RREN_RR7_Msk (0x1UL << WDT_RREN_RR7_Pos) /*!< Bit mask of RR7 field. */
#define WDT_RREN_RR7_Disabled (0UL) /*!< Disable RR[7] register */
#define WDT_RREN_RR7_Enabled (1UL) /*!< Enable RR[7] register */

/* Bit 6 : Enable or disable RR[6] register */
#define WDT_RREN_RR6_Pos (6UL) /*!< Position of RR6 field. */
#define WDT_RREN_RR6_Msk (0x1UL << WDT_RREN_RR6_Pos) /*!< Bit mask of RR6 field. */
#define WDT_RREN_RR6_Disabled (0UL) /*!< Disable RR[6] register */
#define WDT_RREN_RR6_Enabled (1UL) /*!< Enable RR[6] register */

/* Bit 5 : Enable or disable RR[5] register */
#define WDT_RREN_RR5_Pos (5UL) /*!< Position of RR5 field. */
#define WDT_RREN_RR5_Msk (0x1UL << WDT_RREN_RR5_Pos) /*!< Bit mask of RR5 field. */
#define WDT_RREN_RR5_Disabled (0UL) /*!< Disable RR[5] register */
#define WDT_RREN_RR5_Enabled (1UL) /*!< Enable RR[5] register */

/* Bit 4 : Enable or disable RR[4] register */
#define WDT_RREN_RR4_Pos (4UL) /*!< Position of RR4 field. */
#define WDT_RREN_RR4_Msk (0x1UL << WDT_RREN_RR4_Pos) /*!< Bit mask of RR4 field. */
#define WDT_RREN_RR4_Disabled (0UL) /*!< Disable RR[4] register */
#define WDT_RREN_RR4_Enabled (1UL) /*!< Enable RR[4] register */

/* Bit 3 : Enable or disable RR[3] register */
#define WDT_RREN_RR3_Pos (3UL) /*!< Position of RR3 field. */
#define WDT_RREN_RR3_Msk (0x1UL << WDT_RREN_RR3_Pos) /*!< Bit mask of RR3 field. */
#define WDT_RREN_RR3_Disabled (0UL) /*!< Disable RR[3] register */
#define WDT_RREN_RR3_Enabled (1UL) /*!< Enable RR[3] register */

/* Bit 2 : Enable or disable RR[2] register */
#define WDT_RREN_RR2_Pos (2UL) /*!< Position of RR2 field. */
#define WDT_RREN_RR2_Msk (0x1UL << WDT_RREN_RR2_Pos) /*!< Bit mask of RR2 field. */
#define WDT_RREN_RR2_Disabled (0UL) /*!< Disable RR[2] register */
#define WDT_RREN_RR2_Enabled (1UL) /*!< Enable RR[2] register */

/* Bit 1 : Enable or disable RR[1] register */
#define WDT_RREN_RR1_Pos (1UL) /*!< Position of RR1 field. */
#define WDT_RREN_RR1_Msk (0x1UL << WDT_RREN_RR1_Pos) /*!< Bit mask of RR1 field. */
#define WDT_RREN_RR1_Disabled (0UL) /*!< Disable RR[1] register */
#define WDT_RREN_RR1_Enabled (1UL) /*!< Enable RR[1] register */

/* Bit 0 : Enable or disable RR[0] register */
#define WDT_RREN_RR0_Pos (0UL) /*!< Position of RR0 field. */
#define WDT_RREN_RR0_Msk (0x1UL << WDT_RREN_RR0_Pos) /*!< Bit mask of RR0 field. */
#define WDT_RREN_RR0_Disabled (0UL) /*!< Disable RR[0] register */
#define WDT_RREN_RR0_Enabled (1UL) /*!< Enable RR[0] register */

/* Register: WDT_CONFIG */
/* Description: Configuration register */

/* Bit 3 : Configure the watchdog to either be paused, or kept running, while the CPU is halted by the debugger */
#define WDT_CONFIG_HALT_Pos (3UL) /*!< Position of HALT field. */
#define WDT_CONFIG_HALT_Msk (0x1UL << WDT_CONFIG_HALT_Pos) /*!< Bit mask of HALT field. */
#define WDT_CONFIG_HALT_Pause (0UL) /*!< Pause watchdog while the CPU is halted by the debugger */
#define WDT_CONFIG_HALT_Run (1UL) /*!< Keep the watchdog running while the CPU is halted by the debugger */

/* Bit 0 : Configure the watchdog to either be paused, or kept running, while the CPU is sleeping */
#define WDT_CONFIG_SLEEP_Pos (0UL) /*!< Position of SLEEP field. */
#define WDT_CONFIG_SLEEP_Msk (0x1UL << WDT_CONFIG_SLEEP_Pos) /*!< Bit mask of SLEEP field. */
#define WDT_CONFIG_SLEEP_Pause (0UL) /*!< Pause watchdog while the CPU is sleeping */
#define WDT_CONFIG_SLEEP_Run (1UL) /*!< Keep the watchdog running while the CPU is sleeping */

/* Register: WDT_RR */
/* Description: Description collection: Reload request n */

/* Bits 31..0 : Reload request register */
#define WDT_RR_RR_Pos (0UL) /*!< Position of RR field. */
#define WDT_RR_RR_Msk (0xFFFFFFFFUL << WDT_RR_RR_Pos) /*!< Bit mask of RR field. */
#define WDT_RR_RR_Reload (0x6E524635UL) /*!< Value to request a reload of the watchdog timer */


/*lint --flb "Leave library region" */
#endif
