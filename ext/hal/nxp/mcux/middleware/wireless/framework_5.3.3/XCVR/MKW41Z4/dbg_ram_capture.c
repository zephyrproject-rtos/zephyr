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
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
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
#include "fsl_device_registers.h"
#include "fsl_xcvr.h"
#include "dbg_ram_capture.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#if RADIO_IS_GEN_3P0
#define PKT_RAM_SIZE_16B_WORDS  (1152) /* Number of 16bit entries in each Packet RAM bank */
#else
#define PKT_RAM_SIZE_16B_WORDS  (544) /* Number of 16bit entries in each Packet RAM bank */
#endif /* RADIO_IS_GEN_3P0 */
#define SIGN_EXTND_12_16(x)     ((x) | (((x) & 0x800) ? 0xF000 : 0x0))
#define SIGN_EXTND_5_8(x)       ((x) | (((x) & 0x10) ? 0xE0 : 0x0))

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/
void dbg_ram_init(void)
{
    XCVR_RX_DIG->RX_DIG_CTRL |= XCVR_RX_DIG_RX_DIG_CTRL_RX_DMA_DTEST_EN_MASK;  /* Turns on clocking to DMA/DBG blocks */
    XCVR_MISC->PACKET_RAM_CTRL |= XCVR_CTRL_PACKET_RAM_CTRL_XCVR_RAM_ALLOW_MASK;  /* Make PKT RAM available to just XCVR */

    /* Some external code must perform the RX warmup request. */
}


dbgRamStatus_t dbg_ram_capture(uint8_t dbg_page, uint16_t buffer_sz_bytes, void * result_buffer)
{
    dbgRamStatus_t status = DBG_RAM_SUCCESS;
    uint32_t temp;
    volatile uint8_t *pkt_ram_ptr0, *pkt_ram_ptr1;
    uint8_t * output_ptr;
    uint16_t i;

    /* Some external code must perform the RX warmup request after the dbg_ram_init() call */

    if (result_buffer == NULL)
    {
        status = DBG_RAM_FAIL_NULL_POINTER;
    }
    else
    {
        if (buffer_sz_bytes > (544*2*2))
        {
            status = DBG_RAM_FAIL_SAMPLE_NUM_LIMIT;
        }
        else
        {
            temp = XCVR_MISC->PACKET_RAM_CTRL & ~XCVR_CTRL_PACKET_RAM_CTRL_DBG_PAGE_MASK;
            switch (dbg_page)
            {
                case DBG_PAGE_RXDIGIQ:
                case DBG_PAGE_RAWADCIQ:
                case DBG_PAGE_DCESTIQ:
                    XCVR_MISC->PACKET_RAM_CTRL =  temp | XCVR_CTRL_PACKET_RAM_CTRL_DBG_PAGE(dbg_page);

                    while (!(XCVR_MISC->PACKET_RAM_CTRL & XCVR_CTRL_PACKET_RAM_CTRL_DBG_RAM_FULL(2)))
                    {
                        /* Waiting for PKT_RAM to fill, wait for PKT_RAM_1 full to ensure complete memory is filled. */
                    }   
                    /* Copy to output by bytes to avoid any access size problems in 16 bit packet RAM. */
                    output_ptr = result_buffer;
#if !RADIO_IS_GEN_2P1
                    pkt_ram_ptr0 = (volatile uint8_t *)&(XCVR_PKT_RAM->PACKET_RAM_0[0]);
                    pkt_ram_ptr1 = (volatile uint8_t *)&(XCVR_PKT_RAM->PACKET_RAM_1[0]);
#else
                    pkt_ram_ptr0 = (volatile uint8_t *)&(XCVR_PKT_RAM->PACKET_RAM[0]);
                    pkt_ram_ptr1 = (volatile uint8_t *)&(XCVR_PKT_RAM->PACKET_RAM[XCVR_PKT_RAM_PACKET_RAM_COUNT>>1]); /* Second packet RAM starts halfway through */
#endif /* !RADIO_IS_GEN_2P1 */
                    /* For *IQ pages I and Q are stored alternately in packet ram 0 & 1 */
                    for (i = 0; i < buffer_sz_bytes / 4; i++)
                    {
                        *output_ptr++ = *pkt_ram_ptr0++;
                        *output_ptr++ = *pkt_ram_ptr0++;
                        *output_ptr++ = *pkt_ram_ptr1++;
                        *output_ptr++ = *pkt_ram_ptr1++;
                    }

                    break;
                case DBG_PAGE_RXINPH:
                case DBG_PAGE_DEMOD_HARD:
                case DBG_PAGE_DEMOD_SOFT:
                case DBG_PAGE_DEMOD_DATA:
                case DBG_PAGE_DEMOD_CFO_PH:
                    XCVR_MISC->PACKET_RAM_CTRL =  temp | XCVR_CTRL_PACKET_RAM_CTRL_DBG_PAGE(dbg_page);
                    while (!(XCVR_MISC->PACKET_RAM_CTRL & XCVR_CTRL_PACKET_RAM_CTRL_DBG_RAM_FULL(2)))
                    {
                        /* Waiting for PKT_RAM to fill, wait for PKT_RAM_1 full to ensure complete memory is filled. */
                    }   
                    /* Copy to output by bytes to avoid any access size problems in 16 bit packet RAM. */
                    output_ptr = result_buffer;
#if !RADIO_IS_GEN_2P1
                    pkt_ram_ptr0 = (volatile uint8_t *)&(XCVR_PKT_RAM->PACKET_RAM_0[0]);
#else
                    pkt_ram_ptr0 = (volatile uint8_t *)&(XCVR_PKT_RAM->PACKET_RAM[0]);
#endif /* !RADIO_IS_GEN_2P1 */
                    /* This is for non I/Q */
                    for (i = 0; i < buffer_sz_bytes; i++)
                    {
                        *output_ptr = *pkt_ram_ptr0;
                        pkt_ram_ptr0++;
                        output_ptr++;
                    }
                    break;
                case DBG_PAGE_IDLE:
                default:    
                    status = DBG_RAM_FAIL_PAGE_ERROR; /* Illegal capture page request. */
                    break;
            }
        }
    }

    XCVR_MISC->PACKET_RAM_CTRL &= ~XCVR_CTRL_PACKET_RAM_CTRL_DBG_PAGE_MASK; /* Clear DBG_PAGE to terminate the acquisition */

    /* Process the samples and copy to output pointer */

    XCVR_MISC->PACKET_RAM_CTRL &= ~XCVR_CTRL_PACKET_RAM_CTRL_XCVR_RAM_ALLOW_MASK; /* Make PKT RAM available to protocol blocks */
    XCVR_RX_DIG->RX_DIG_CTRL &= ~XCVR_RX_DIG_RX_DIG_CTRL_RX_DMA_DTEST_EN_MASK; /* Turns off clocking to DMA/DBG blocks */

    return status;
}

