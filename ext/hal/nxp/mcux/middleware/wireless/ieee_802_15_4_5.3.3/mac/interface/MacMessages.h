/*! *********************************************************************************
* \defgroup MacMessages Mac Messages
* @{
********************************************************************************** */
/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
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

#ifndef _MAC_MESSAGES_H
/*! \cond */
#define _MAC_MESSAGES_H
/*! \endcond */

typedef enum
{
    gMcpsDataReq_c,          /*!< MCPS-DATA.Request */
    gMcpsDataCnf_c,          /*!< MCPS-DATA.Confirm */
    gMcpsDataInd_c,          /*!< MCPS-DATA.Indication */

    gMcpsPurgeReq_c,         /*!< MCPS-PURGE.Request */
    gMcpsPurgeCnf_c,         /*!< MCPS-PURGE.Confirm */

    gMcpsPromInd_c,          /*!< MCPS-PROMISCUOUS.Indication (NOT in spec) */

    gMlmeAssociateReq_c,     /*!< MLME-ASSOCIATE.Request */
    gMlmeAssociateInd_c,     /*!< MLME-ASSOCIATE.Indication */
    gMlmeAssociateRes_c,     /*!< MLME-ASSOCIATE.Response */
    gMlmeAssociateCnf_c,     /*!< MLME-ASSOCIATE.Confirm */

    gMlmeDisassociateReq_c,  /*!< MLME-DISASSOCIATE.Request */
    gMlmeDisassociateInd_c,  /*!< MLME-DISASSOCIATE.Indication */
    gMlmeDisassociateCnf_c,  /*!< MLME-DISASSOCIATE.Confirm */
    gMlmeBeaconNotifyInd_c,  /*!< MLME-BEACON-NOTIFY.Indication */

    gMlmeGetReq_c,           /*!< MLME-GET.Request */
    gMlmeGetCnf_c,           /*!< MLME-GET.Confirm */

    gMlmeGtsReq_c,           /*!< MLME-GTS.Request */
    gMlmeGtsCnf_c,           /*!< MLME-GTS.Confirm */
    gMlmeGtsInd_c,           /*!< MLME-GTS.Indication */

    gMlmeOrphanInd_c,        /*!< MLME-ORPHAN.Indication */
    gMlmeOrphanRes_c,        /*!< MLME-ORPHAN.Response */

    gMlmeResetReq_c,         /*!< MLME-RESET.Request */
    gMlmeResetCnf_c,         /*!< MLME-RESET.Confirm */

    gMlmeRxEnableReq_c,      /*!< MLME-RX-ENABLE.Request */
    gMlmeRxEnableCnf_c,      /*!< MLME-RX-ENABLE.Confirm */

    gMlmeScanReq_c,          /*!< MLME-SCAN.Request */
    gMlmeScanCnf_c,          /*!< MLME-SCAN.Confirm */

    gMlmeCommStatusInd_c,    /*!< MLME-COMM-STATUS.Indication */

    gMlmeSetReq_c,           /*!< MLME-SET.Request */
    gMlmeSetCnf_c,           /*!< MLME-SET.Confirm */

    gMlmeStartReq_c,         /*!< MLME-START.Request */
    gMlmeStartCnf_c,         /*!< MLME-START.Confirm */

    gMlmeSyncReq_c,          /*!< MLME-SYNC.Request */

    gMlmeSyncLossInd_c,      /*!< MLME-SYNC-LOSS.Indication */

    gAutoPollReq_c,          /*!< Internal use */
    gMlmePollReq_c,          /*!< MLME-POLL.Request */
    gMlmePollCnf_c,          /*!< MLME-POLL.Confirm */

    gMlmePollNotifyInd_c,    /*!< MLME-POLL-NOTIFY.Indication (NOT in spec) */
    
    gMlmeSetSlotframeReq_c,  /*!< MLME-SET-SLOTFRAME.Request */
    gMlmeSetSlotframeCnf_c,  /*!< MLME-SET-SLOTFRAME.Confirm */
    
    gMlmeSetLinkReq_c,       /*!< MLME-SET-LINK.Request */
    gMlmeSetLinkCnf_c,       /*!< MLME-SET-LINK.Confirm */
    
    gMlmeTschModeReq_c,      /*!< MLME-TSCH-MODE.Request */
    gMlmeTschModeCnf_c,      /*!< MLME-TSCH-MODE.Confirm */
    
    gMlmeKeepAliveReq_c,     /*!< MLME-KEEP-ALIVE.Request */
    gMlmeKeepAliveCnf_c,     /*!< MLME-KEEP-ALIVE.Confirm */
    
    gMlmeBeaconReq_c,        /*!< MLME-BEACON.Request */
    gMlmeBeaconCnf_c,        /*!< MLME-BEACON.Confirm */
}macMessageId_t;

#endif  /* _MAC_MESSAGES_H */

/*! *********************************************************************************
* @}
********************************************************************************** */
