/*! *********************************************************************************
* \defgroup AspInterface ASP Interface
* The Application Support Package (ASP) contains XCVR-specific functionality. 
* It can be used for XCVR testing, and for accessing XCVR-specific features (for example FAD).
* The ASP layer can be interfaced using the SAP handler, or using direct function calls.
* @{
********************************************************************************** */
/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* Copyright 2016-2017 NXP
*
* \file
*
* This is a header file for the ASP module.
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

#ifndef __ASP_H__
#define __ASP_H__

/************************************************************************************
*************************************************************************************
* Include
*************************************************************************************
************************************************************************************/

#include "EmbeddedTypes.h"
#include "fsl_os_abstraction.h"

/************************************************************************************
*************************************************************************************
* Public macros
*************************************************************************************
************************************************************************************/

/*! Enable/Disable the ASP module */
#ifndef gAspCapability_d
#define gAspCapability_d (0)
#endif

/*! \cond */
#define gEnablePA2_Boost_c             (0x01 << 5)
#define gEnablePA1_Boost_c             (0x01 << 6)
#define gEnablePA0_Boost_c             (0x01 << 7)
#define gEnablePABoth_Boost_c          (gEnablePA2_Boost_c | gEnablePA1_Boost_c)
#define gDisablePA_Boost_c             ( 0x00 )

#define gEnablePa_PaBoost_c            (0x01 << 7)
#define gDisablePa_PaBoost_c           (0x00 << 7) 
/*! \endcond */


/************************************************************************************
*************************************************************************************
* Public type definitions
*************************************************************************************
************************************************************************************/

/*! Clock rates enum for MKW01 (used by SMAC) */
typedef enum clkoFrequency_tag 
{
  gAspClko32MHz_c = 0, 
  gAspClko16MHz_c, 
  gAspClko8MHz_c,
  gAspClko4MHz_c,
  gAspClko2MHz_c,
  gAspClko1MHz_c,
  gAspClkoOutOfRange_c
} clkoFrequency_t;

/*! Listen resolution MKW01 (used by SMAC) */
typedef enum listenResolution_tag
{
  gAspListenRes_01_c = 0x10,
  gAspListenRes_02_c = 0x20,
  gAspListenRes_03_c = 0x30
}listenResolution_t;

/*! LNA gain enum for MKW01 (used by SMAC) */
typedef enum lnaGainValues_tag
{
  gAspLnaGain_0_c = 0,
  gAspLnaGain_1_c,
  gAspLnaGain_2_c,
  gAspLnaGain_3_c,
  gAspLnaGain_4_c,
  gAspLnaGain_5_c,
  gAspLnaGain_6_c,
  gAspLnaGain_7_c,	
}lnaGainValues_t;

/*! ASP status messages */
typedef enum{
    gAspSuccess_c          = 0x00, /*!< The ASP command execution was successful. */
    gAspInvalidRequest_c   = 0xC2, /*!< The ASP request does not exist. */
    gAspDenied_c           = 0xE2, /*!< The request is not allowed at this point. */
    gAspTooLong_c          = 0xE5, /*!< The payload of the ASP request is too long. */
    gAspInvalidParameter_c = 0xE8  /*!< The request contains an invalid parameter. */
}AspStatus_t;


/*! Radio test modes. These modes are hardware specific.
    Some modes may not be available on certain HW platforms. */
enum {
    gTestForceIdle_c               = 0, /*!< Force XCVR to idle. */
    gTestPulseTxPrbs9_c            = 1, /*!< Send a continuous PRBS9 payload. */
    gTestContinuousRx_c            = 2, /*!< Set XCVR in continuous Rx mode. */
    gTestContinuousTxMod_c         = 3, /*!< Transmit the 802.15.4 carrier. */
    gTestContinuousTxNoMod_c       = 4, /*!< Transmit an unmodulated signal. */
    gTestContinuousTx2Mhz_c        = 5, /*!< Transmit a 2 MHz clock signal. */
    gTestContinuousTx200Khz_c      = 6, /*!< Transmit a 200 KHz clock signal. */
    gTestContinuousTx1MbpsPRBS9_c  = 7, /*!< Transmit a continuous PRBS9 signal at 1Mbps. */
    gTestContinuousTxExternalSrc_c = 8, /*!< Transmit pattern from an external source. */
    /* @connectivity test, test modes */
    gTestContinuousTxModOne_c      = 9, /*!< Transmit a continuous ONE signal */
    gTestContinuousTxModZero_c     = 10,/*!< Transmit a continuous ZERO signal */
    gTestContinuousTxContPN9_c     = 11,/*!< Transmit a continuous PN9 signal */
    gTestTxPacketPRBS9_c           = 12 /*!< Send a PRBS9 payload in packet mode, at a specific interval. */
};

/*! ASP messages. This enum matches with the FSCI OpCode used by ASP*/
typedef enum {
    aspMsgTypeGetTimeReq_c          = 0x00,
    aspMsgTypeGetInactiveTimeReq_c  = 0x01,
    aspMsgTypeGetMacStateReq_c      = 0x02,
    aspMsgTypeDozeReq_c             = 0x03,
    aspMsgTypeAutoDozeReq_c         = 0x04,
    aspMsgTypeAcomaReq_c            = 0x05,
    aspMsgTypeHibernateReq_c        = 0x06,
    aspMsgTypeWakeReq_c             = 0x07,
    aspMsgTypeEventReq_c            = 0x08,
    aspMsgTypeClkoReq_c             = 0x09,
    aspMsgTypeSetXtalTrimReq_c      = 0x0A,
    aspMsgTypeDdrReq_c              = 0x0B,
    aspMsgTypePortReq_c             = 0x0C,
    aspMsgTypeSetMinDozeTimeReq_c   = 0x0D,
    aspMsgTypeSetNotifyReq_c        = 0x0E,
    aspMsgTypeSetPowerLevel_c       = 0x0F,
    aspMsgTypeTelecTest_c           = 0x10,
    aspMsgTypeTelecSetFreq_c        = 0x11,
    aspMsgTypeGetInactiveTimeCnf_c  = 0x12,
    aspMsgTypeGetMacStateCnf_c      = 0x13,
    aspMsgTypeDozeCnf_c             = 0x14,
    aspMsgTypeAutoDozeCnf_c         = 0x15,
    aspMsgTypeTelecSendRawData_c    = 0x16,
    aspMsgTypeSetFADState_c         = 0x17,
    aspMsgTypeSetFADThreshold_c     = 0x18,
    aspMsgTypeGetFADThreshold_c     = 0x19,
    aspMsgTypeGetFADState_c         = 0x1A,
    aspMsgTypeSetActivePromState_c  = 0x1B,
    aspMsgTypeXcvrWriteReq_c        = 0x1C,
    aspMsgTypeXcvrReadReq_c         = 0x1D,
    aspMsgTypeGetPowerLevel_c       = 0x1F,
    aspMsgTypeSetANTXState_c        = 0x20,
    aspMsgTypeGetANTXState_c        = 0x21,
    aspMsgTypeSetLQIMode_c          = 0x22,
    aspMsgTypeGetRSSILevel_c        = 0x23,
    aspMsgTypeSetMpmConfig_c        = 0x24,
    aspMsgTypeGetMpmConfig_c        = 0x25,
    aspMsgTypeSetTxInterval_c       = 0x30,
    aspMsgTypeGetXtalTrimReq_c      = 0x31
}AppAspMsgType_t;

/*! AspEvent.Request - (Unused, legacy) */
typedef PACKED_STRUCT aspEventReq_tag
{
    uint32_t eventTime; /*!< Event time-out value */
} aspEventReq_t;

/*! AspGetTime.Request            */
typedef PACKED_STRUCT aspGetTimeReq_tag
{
    uint64_t time; /*!< Current time in PHY symbols */
} aspGetTimeReq_t;

/*! AspXtalTrim.Request        */
typedef PACKED_STRUCT aspXtalTrimReq_tag
{
    uint8_t trim; /*!< XCVR XTAL trim value */
} aspXtalTrimReq_t;

/*! AspSetPowerLevel.Request      */
typedef PACKED_STRUCT aspSetPowerLevelReq_tag
{
    uint8_t powerLevel; /*!< Platform specific */
} aspSetPowerLevelReq_t;

/*! AspGetPowerLevel.Request      */
typedef PACKED_STRUCT aspGetPowerLevelReq_tag
{
    uint8_t powerLevel; /*!< Platform specific */
} aspGetPowerLevelReq_t;

/*! AspTelecTest.Request          */
typedef PACKED_STRUCT aspTelecTest_tag
{
    uint8_t mode; /*!< Desired Radio test mode */
} aspTelecTest_t;

/*! AspTelecSetFreq.Request       */
typedef PACKED_STRUCT aspTelecsetFreq_tag
{
    uint8_t channel; /*!< logical channel number */
} aspTelecsetFreq_t;

/*! AspTelecSendRawData.Request   */
typedef PACKED_STRUCT aspTelecSendRawData_tag
{
    uint8_t  length;
    uint8_t* dataPtr;
} aspTelecSendRawData_t;

/*! Set the TX interval for the /ref gTestTxPacketPRBS9_c test mode */
typedef PACKED_STRUCT aspSetTxInterval_tag
{
    uint32_t intervalMs; /*!< miliseconds */
} aspSetTxInterval_t;

/*! AspSetFADThreshold.Request   */
typedef uint8_t aspFADThreshold_t;

/*! AspSetLQIMode.Request    */
typedef uint8_t aspLQIMode_t;

/*! AspXcvrWrite.Request / AspXcvrRead.Request   */
typedef PACKED_STRUCT aspXcvrReq_tag
{
    uint8_t  mode;
    uint16_t addr;
    uint8_t  len;
    uint8_t  data[4]; /*!< more than 4 bytes can be read/written */
} aspXcvrReq_t;

/*! Multiple PAN Manager configuration structure */
typedef PACKED_STRUCT aspMpmConfig_tag
{
    uint8_t autoMode;
    uint8_t dwellTime;
    uint8_t activeMAC;
} aspMpmConfig_t;

/*! This union is used to send ASP request using the APP_ASP_SapHandler(). */
typedef PACKED_STRUCT AppToAspMessage_tag
{
    AppAspMsgType_t msgType;
    PACKED_UNION
    {
        aspEventReq_t           aspEventReq;
        aspXtalTrimReq_t        aspXtalTrim;
        aspGetTimeReq_t         aspGetTimeReq;
        aspSetPowerLevelReq_t   aspSetPowerLevelReq;
        aspGetPowerLevelReq_t   aspGetPowerLevelReq;
        aspTelecTest_t          aspTelecTest;
        aspTelecsetFreq_t       aspTelecsetFreq;
        aspTelecSendRawData_t   aspTelecSendRawData;
        aspFADThreshold_t       aspFADThreshold;
        bool_t                  aspFADState;
        bool_t                  aspANTXState;
        aspLQIMode_t            aspLQIMode;
        bool_t                  aspActivePromState;
        aspXcvrReq_t            aspXcvrData;
        aspMpmConfig_t          MpmConfig;
        aspSetTxInterval_t      aspSetTxInterval;
    }msgData;
} AppToAspMessage_t;

#ifdef __cplusplus
extern "C" {
#endif 

/************************************************************************************
*************************************************************************************
* Public memory declarations
*************************************************************************************
************************************************************************************/

/************************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
************************************************************************************/

#if gAspCapability_d
/*! *********************************************************************************
 * \brief initialize the ASP module
 *
 * \param[in]  phyInstance  Instance of the PHY layer
 *
 ********************************************************************************** */
void ASP_Init( instanceId_t phyInstance );

/*! *********************************************************************************
*  \brief This function represents the Service Access Point (SAP) of the ASP layer. 
*         This SAP is used by other layers to send requests to the ASP layer.
*
* \param[in]  pMsg         Pointer to the ASP request message
* \param[in]  phyInstance  Instance of the PHY layer
*
* \return  AspStatus_t
*
********************************************************************************** */
AspStatus_t APP_ASP_SapHandler(AppToAspMessage_t *pMsg, instanceId_t phyInstance);

/*! *********************************************************************************
 * \brief Get the absolute time from the XCVR module.
 *
 * \param[out] time Pointer where to store the value
 *
 ********************************************************************************** */
void Asp_GetTimeReq(uint64_t *time);

/*! *********************************************************************************
*   Write XCVR registers
*
* \param[in]  mode   Write mode (hardware specific)
* \param[in]  addr   XCVR address
* \param[in]  len    Number of bytes to write
* \param[in]  pData  Pointer to the data  
*
* \return  AspStatus_t
*
********************************************************************************** */
AspStatus_t Asp_XcvrWriteReq (uint8_t mode, uint16_t addr, uint8_t len, uint8_t* pData);

/*! *********************************************************************************
*   Read XCVR registers
*
* \param[in]  mode   Read mode (hardware specific)
* \param[in]  addr   XCVR address
* \param[in]  len    Number of bytes to read
* \param[in]  pData  Pointer where to store the data  
*
* \return  AspStatus_t
*
********************************************************************************** */
AspStatus_t Asp_XcvrReadReq  (uint8_t mode, uint16_t addr, uint8_t len, uint8_t* pData);

/*! *********************************************************************************
*   Set TX power level
*
* \param[in]  powerLevel   TX power level (hardware specific)
*
* \return  AspStatus_t
*
********************************************************************************** */
AspStatus_t Asp_SetPowerLevel(uint8_t powerLevel);

/*! *********************************************************************************
*   Get TX power level
*
* \return  TX power level (hardware specific)
*
********************************************************************************** */
uint8_t     Asp_GetPowerLevel(void);

/*! *********************************************************************************
*   Set Receiver in Active Promiscuous mode
*
* \param[in]  state   TRUE/FALSE
*
* \return  AspStatus_t
*
********************************************************************************** */
AspStatus_t Asp_SetActivePromState(bool_t state);

/*! *********************************************************************************
*   Set Receiver Fast Antenna Diversity
*
* \param[in]  state   TRUE/FALSE
*
* \return  AspStatus_t
*
********************************************************************************** */
AspStatus_t Asp_SetFADState(bool_t state);

/*! *********************************************************************************
*   Set Fast Antenna Diversity correlator threshold
*
* \param[in]  thresholdFAD   Hardware specific
*
* \return  AspStatus_t
*
********************************************************************************** */
AspStatus_t Asp_SetFADThreshold(uint8_t thresholdFAD);

/*! *********************************************************************************
* \brief  If FAD is enabled, this function sets the starting antenna for the FAD feature, 
*         else it simply selects the active antenna.
*
* \param[in]  state   1 – use ANT_B; 0 – use ANT_A
*
* \return  AspStatus_t
*
********************************************************************************** */
AspStatus_t Asp_SetANTXState(bool_t state);

/*! *********************************************************************************
* \brief Returns the active antenna (in FAD and non-FAD modes).
*
* \return  1 – ANT_B; 0 – ANT_A
*
********************************************************************************** */
uint8_t     Asp_GetANTXState(void);

/*! *********************************************************************************
*   Set ANT pad states
*
* \param[in] antAB_on      Enable ANT_A and ANT_B pads
* \param[in] rxtxSwitch_on Enable RX_SWITCH and TX_SWITCH pads
*
* \return  error code
*
********************************************************************************** */
uint8_t     Asp_SetANTPadStateRequest(bool_t antAB_on, bool_t rxtxSwitch_on);

/*! *********************************************************************************
*   Enable Hi drive strength on ANT pads
*
* \param[in] hiStrength      TRUE/FALSE
*
* \return  error code
*
********************************************************************************** */
uint8_t     Asp_SetANTPadStrengthRequest(bool_t hiStrength);

/*! *********************************************************************************
*   Set ANT pads in inverted mode
*
* \param[in] invAntA      Inverted ANT_A pad: TRUE/FALSE
* \param[in] invAntB      Inverted ANT_B pad: TRUE/FALSE
* \param[in] invTx        Inverted TX_SWITCH pad: TRUE/FALSE
* \param[in] invRx        Inverted RX_SWITCH pad: TRUE/FALSE
*
* \return  error code
*
********************************************************************************** */
uint8_t     Asp_SetANTPadInvertedRequest(bool_t invAntA, bool_t invAntB, bool_t invTx, bool_t invRx);

/*! *********************************************************************************
*   Set the LQI mode: 1 – based on RSSI; 0 – based on correlator peaks.
*
* \param[in]  mode   TRUE/FALSE
*
* \return  AspStatus_t
*
********************************************************************************** */
AspStatus_t Asp_SetLQIMode(bool_t mode);

/*! *********************************************************************************
*   Read current XCVR RSSI level.
*
* \return  RSSI value
*
********************************************************************************** */
uint8_t     Asp_GetRSSILevel(void);

/*! *********************************************************************************
*   Set the 802.15.4 logical channel for the Telec module
*
* \param[in]  channel   Logical channel
*
* \return  AspStatus_t
*
********************************************************************************** */
AspStatus_t ASP_TelecSetFreq    (uint8_t channel);

/*! *********************************************************************************
*   Send a raw data packet over-the-air
*
* \param[in]  dataPtr   Pointer to the data. The first data byte represents the payload length
*
* \return  AspStatus_t
*
********************************************************************************** */
AspStatus_t ASP_TelecSendRawData(uint8_t* dataPtr);

/*! *********************************************************************************
*   Set XCVR in test mode
*
* \param[in]  mode   Radio test mode
*
* \return  AspStatus_t
*
********************************************************************************** */
AspStatus_t ASP_TelecTest       (uint8_t mode);

/*! Reset XCVR (used by SMAC)*/
void Asp_XCVRContReset(void); 
/*! Restart XCVR (used by SMAC)*/
void Asp_XCVRRestart(void);   
/*! Enable XCVR Interrupts (used by SMAC)*/
void Asp_EnableXCVRInterrupts(void);
/*! Disable XCVR Interrupts (used by SMAC)*/
void Asp_DisableXCVRInterrupts(void);
/*! Enables PA Boost MKW01 (used by SMAC)*/
AspStatus_t Asp_EnablePABoost(uint8_t u8PABoostCfg); 
/*! Disables PA Boost MKW01 (used by SMAC)*/
AspStatus_t Asp_DisablePABoost(void);
/*! Sets XCVR clock MKW01 (used by SMAC)*/
AspStatus_t Asp_SetClockRate(clkoFrequency_t clock);
/*! Sets Lna Gain MKW01 (used by SMAC)*/
AspStatus_t Asp_SetLnaGainAdjust(lnaGainValues_t gainValue);
/*! Sets XCVR in listen mode for MKW01 (used by SMAC)*/
AspStatus_t Asp_ListenRequest( listenResolution_t listenResolution, uint8_t listenCoef);
/*! Sets XCVR in stand-by mode MKW01 (used by SMAC)*/
AspStatus_t Asp_SetStandByModeRequest(void);
/*! Sets XCVR in sleep mode MKW01 (used by SMAC)*/
AspStatus_t Asp_SetSleepModeRequest(void);
/*! Enables Afc MKW01 (used by SMAC)*/
AspStatus_t Asp_EnableAfc(bool_t lowBetaOn, bool_t autoClearOn);
/*! Disables Afc MKW01 (used by SMAC)*/
AspStatus_t Asp_DisableAfc(void);

#else /* gAspCapability_d */

#define ASP_Init(phyInstance)
#define Asp_GetTimeReq(time)

#define APP_ASP_SapHandler(pMsg, phyInstance)     (gAspDenied_c)
#define Asp_XcvrWriteReq(mode, addr, len, pData)  (gAspDenied_c)
#define Asp_XcvrReadReq(mode, addr, len, pData)   (gAspDenied_c)
#define Asp_SetPowerLevel(powerLevel)             (gAspDenied_c)
#define Asp_SetActivePromState(state)             (gAspDenied_c)
#define Asp_SetFADState(state)                    (gAspDenied_c)
#define Asp_SetFADThreshold(thresholdFAD)         (gAspDenied_c)
#define Asp_SetANTXState(state)                   (gAspDenied_c)
#define Asp_SetLQIMode(mode)                      (gAspDenied_c)
#define ASP_TelecSetFreq(channel)                 (gAspDenied_c)
#define ASP_TelecSendRawData(dataPtr)             (gAspDenied_c)
#define ASP_TelecTest(mode)                       (gAspDenied_c)

#define Asp_GetPowerLevel() (0)
#define Asp_GetANTXState()  (0)
#define Asp_GetRSSILevel()  (0)
#endif /* gAspCapability_d */

#ifdef __cplusplus
}
#endif 

#endif /*__ASP_H__ */
/*! *********************************************************************************
* @}
********************************************************************************** */