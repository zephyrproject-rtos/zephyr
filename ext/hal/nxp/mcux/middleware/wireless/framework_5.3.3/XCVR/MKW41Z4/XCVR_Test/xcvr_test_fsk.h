/*!
* Copyright 2016-2017 NXP
*
* \file
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

#ifndef __XCVR_TEST_FSK_H__
#define __XCVR_TEST_FSK_H__

/*! *********************************************************************************
*************************************************************************************
* Public type definitions
*************************************************************************************
********************************************************************************** */

/*! *********************************************************************************
*************************************************************************************
* Public prototypes
*************************************************************************************
********************************************************************************** */
/*! *********************************************************************************
* \brief  This function returns instant RSSI value and returns it as unsigned byte.
*
* \param[in] None.
*
* \ingroup TestFunctions
*
* \details Initialization of the xcvr is necessary prior to calling this function
*
***********************************************************************************/
extern uint8_t XcvrFskGetInstantRssi(void);

/*! *********************************************************************************
* \brief  This function sets the transceiver into continuous modulated transmission.
*
* \param[in] None.
*
* \ingroup TestFunctions
*
* \details Initialization of the xcvr and calling XcvrFskLoadPattern are necessary
*         prior to calling this function
*
***********************************************************************************/
extern void XcvrFskModTx(void);

/*! *********************************************************************************
* \brief  This function sets the transceiver into continuous unmodulated transmission.
*
* \param[in] None.
*
* \ingroup TestFunctions
*
* \details 
*
***********************************************************************************/
extern void XcvrFskNoModTx(void);

/*! *********************************************************************************
* \brief  This function sets the transceiver into idle.
*
* \param[in] None.
*
* \ingroup TestFunctions
*
* \details 
*
***********************************************************************************/
extern void XcvrFskIdle(void);

/*! *********************************************************************************
* \brief  This function sets the transceiver into continuous modulated transmission.
*
* \param[in] None.
*
* \ingroup TestFunctions
*
* \details The modulation used is a pseudo-random pattern generated using a LFSR.
*
***********************************************************************************/
extern void XcvrFskTxRand(void);

/*! *********************************************************************************
* \brief  This function loads a 32 bit value into the pattern register used by XcvrFskModTx.
*
* \param[in] u32Pattern The pattern to be loaded.
*
* \ingroup TestFunctions
*
* \details 
*
***********************************************************************************/
extern void XcvrFskLoadPattern(uint32_t u32Pattern);

/*! *********************************************************************************
* \brief  This function gives tx power control to xcvr and sets the power to u8TxPow.
*
* \param[in] u8TxPow Values should be between 0x00 and 0x0F.
*
* \ingroup TestFunctions
*
* \details 
*
***********************************************************************************/
extern void XcvrFskSetTxPower(uint8_t u8TxPow);

/*! *********************************************************************************
* \brief  This function gives tx channel control to xcvr and sets the channel to u8TxChan.
*
* \param[in] u8TxChan Values should be between 0 and 39.
*
* \ingroup TestFunctions
*
* \details 
*
***********************************************************************************/
extern void XcvrFskSetTxChannel(uint8_t u8TxChan);

/*! *********************************************************************************
* \brief  This function gives tx channel control and power to the upper layer.
*
* \param[in] None.
*
* \ingroup TestFunctions
*
* \details Call this function only if XcvrFskSetTxChannel or XcvrFskSetTxPower were called
*         previously.
*
***********************************************************************************/
extern void XcvrFskRestoreTXControl(void);

#endif

