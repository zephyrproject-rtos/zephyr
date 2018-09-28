/***************************************************************************//**
* \file cyip_ble.h
*
* \brief
* BLE IP definitions
*
* \note
* Generator version: 1.3.0.1146
* Database revision: rev#1050929
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#ifndef _CYIP_BLE_H_
#define _CYIP_BLE_H_

#include "cyip_headers.h"

/*******************************************************************************
*                                     BLE
*******************************************************************************/

#define BLE_RCB_RCBLL_SECTION_SIZE              0x00000100UL
#define BLE_RCB_SECTION_SIZE                    0x00000200UL
#define BLE_BLELL_SECTION_SIZE                  0x0001E000UL
#define BLE_BLESS_SECTION_SIZE                  0x00001000UL
#define BLE_SECTION_SIZE                        0x00020000UL

/**
  * \brief Radio Control Bus (RCB) & Link Layer controller (BLE_RCB_RCBLL)
  */
typedef struct {
  __IOM uint32_t CTRL;                          /*!< 0x00000000 RCB LL control register. */
   __IM uint32_t RESERVED[3];
  __IOM uint32_t INTR;                          /*!< 0x00000010 Master interrupt request register. */
  __IOM uint32_t INTR_SET;                      /*!< 0x00000014 Master interrupt set request register */
  __IOM uint32_t INTR_MASK;                     /*!< 0x00000018 Master interrupt mask register. */
   __IM uint32_t INTR_MASKED;                   /*!< 0x0000001C Master interrupt masked request register */
  __IOM uint32_t RADIO_REG1_ADDR;               /*!< 0x00000020 Address of Register#1 in Radio  (MDON) */
  __IOM uint32_t RADIO_REG2_ADDR;               /*!< 0x00000024 Address of Register#2 in Radio  (RSSI) */
  __IOM uint32_t RADIO_REG3_ADDR;               /*!< 0x00000028 Address of Register#3 in Radio  (ACCL) */
  __IOM uint32_t RADIO_REG4_ADDR;               /*!< 0x0000002C Address of Register#4 in Radio  (ACCH) */
  __IOM uint32_t RADIO_REG5_ADDR;               /*!< 0x00000030 Address of Register#5 in Radio  (RSSI ENERGY) */
   __IM uint32_t RESERVED1[3];
  __IOM uint32_t CPU_WRITE_REG;                 /*!< 0x00000040 N/A */
  __IOM uint32_t CPU_READ_REG;                  /*!< 0x00000044 N/A */
   __IM uint32_t RESERVED2[46];
} BLE_RCB_RCBLL_V1_Type;                        /*!< Size = 256 (0x100) */

/**
  * \brief Radio Control Bus (RCB) controller (BLE_RCB)
  */
typedef struct {
  __IOM uint32_t CTRL;                          /*!< 0x00000000 RCB control register. */
   __IM uint32_t STATUS;                        /*!< 0x00000004 RCB status register. */
   __IM uint32_t RESERVED[2];
  __IOM uint32_t TX_CTRL;                       /*!< 0x00000010 Transmitter control register. */
  __IOM uint32_t TX_FIFO_CTRL;                  /*!< 0x00000014 Transmitter FIFO control register. */
   __IM uint32_t TX_FIFO_STATUS;                /*!< 0x00000018 Transmitter FIFO status register. */
   __OM uint32_t TX_FIFO_WR;                    /*!< 0x0000001C Transmitter FIFO write register. */
  __IOM uint32_t RX_CTRL;                       /*!< 0x00000020 Receiver control register. */
  __IOM uint32_t RX_FIFO_CTRL;                  /*!< 0x00000024 Receiver FIFO control register. */
   __IM uint32_t RX_FIFO_STATUS;                /*!< 0x00000028 Receiver FIFO status register. */
   __IM uint32_t RX_FIFO_RD;                    /*!< 0x0000002C Receiver FIFO read register. */
   __IM uint32_t RX_FIFO_RD_SILENT;             /*!< 0x00000030 Receiver FIFO read register. */
   __IM uint32_t RESERVED1[3];
  __IOM uint32_t INTR;                          /*!< 0x00000040 Master interrupt request register. */
  __IOM uint32_t INTR_SET;                      /*!< 0x00000044 Master interrupt set request register */
  __IOM uint32_t INTR_MASK;                     /*!< 0x00000048 Master interrupt mask register. */
   __IM uint32_t INTR_MASKED;                   /*!< 0x0000004C Master interrupt masked request register */
   __IM uint32_t RESERVED2[44];
        BLE_RCB_RCBLL_V1_Type RCBLL;            /*!< 0x00000100 Radio Control Bus (RCB) & Link Layer controller */
} BLE_RCB_V1_Type;                              /*!< Size = 512 (0x200) */

/**
  * \brief Bluetooth Low Energy Link Layer (BLE_BLELL)
  */
typedef struct {
   __OM uint32_t COMMAND_REGISTER;              /*!< 0x00000000 Instruction Register */
   __IM uint32_t RESERVED;
  __IOM uint32_t EVENT_INTR;                    /*!< 0x00000008 Event(Interrupt) status and Clear register */
   __IM uint32_t RESERVED1;
  __IOM uint32_t EVENT_ENABLE;                  /*!< 0x00000010 Event indications enable. */
   __IM uint32_t RESERVED2;
  __IOM uint32_t ADV_PARAMS;                    /*!< 0x00000018 Advertising parameters register. */
  __IOM uint32_t ADV_INTERVAL_TIMEOUT;          /*!< 0x0000001C Advertising interval register. */
  __IOM uint32_t ADV_INTR;                      /*!< 0x00000020 Advertising interrupt status and Clear register */
   __IM uint32_t ADV_NEXT_INSTANT;              /*!< 0x00000024 Advertising next instant. */
  __IOM uint32_t SCAN_INTERVAL;                 /*!< 0x00000028 Scan Interval Register */
  __IOM uint32_t SCAN_WINDOW;                   /*!< 0x0000002C Scan window Register */
  __IOM uint32_t SCAN_PARAM;                    /*!< 0x00000030 Scanning parameters register */
   __IM uint32_t RESERVED3;
  __IOM uint32_t SCAN_INTR;                     /*!< 0x00000038 Scan interrupt status and Clear register */
   __IM uint32_t SCAN_NEXT_INSTANT;             /*!< 0x0000003C Advertising next instant. */
  __IOM uint32_t INIT_INTERVAL;                 /*!< 0x00000040 Initiator Interval Register */
  __IOM uint32_t INIT_WINDOW;                   /*!< 0x00000044 Initiator window Register */
  __IOM uint32_t INIT_PARAM;                    /*!< 0x00000048 Initiator parameters register */
   __IM uint32_t RESERVED4;
  __IOM uint32_t INIT_INTR;                     /*!< 0x00000050 Scan interrupt status and Clear register */
   __IM uint32_t INIT_NEXT_INSTANT;             /*!< 0x00000054 Initiator next instant. */
  __IOM uint32_t DEVICE_RAND_ADDR_L;            /*!< 0x00000058 Lower 16 bit random address of the device. */
  __IOM uint32_t DEVICE_RAND_ADDR_M;            /*!< 0x0000005C Middle 16 bit random address of the device. */
  __IOM uint32_t DEVICE_RAND_ADDR_H;            /*!< 0x00000060 Higher 16 bit random address of the device. */
   __IM uint32_t RESERVED5;
  __IOM uint32_t PEER_ADDR_L;                   /*!< 0x00000068 Lower 16 bit address of the peer device. */
  __IOM uint32_t PEER_ADDR_M;                   /*!< 0x0000006C Middle 16 bit address of the peer device. */
  __IOM uint32_t PEER_ADDR_H;                   /*!< 0x00000070 Higher 16 bit address of the peer device. */
   __IM uint32_t RESERVED6;
  __IOM uint32_t WL_ADDR_TYPE;                  /*!< 0x00000078 whitelist address type */
  __IOM uint32_t WL_ENABLE;                     /*!< 0x0000007C whitelist valid entry bit */
  __IOM uint32_t TRANSMIT_WINDOW_OFFSET;        /*!< 0x00000080 Transmit window offset */
  __IOM uint32_t TRANSMIT_WINDOW_SIZE;          /*!< 0x00000084 Transmit window size */
  __IOM uint32_t DATA_CHANNELS_L0;              /*!< 0x00000088 Data channel map 0 (lower word) */
  __IOM uint32_t DATA_CHANNELS_M0;              /*!< 0x0000008C Data channel map 0 (middle word) */
  __IOM uint32_t DATA_CHANNELS_H0;              /*!< 0x00000090 Data channel map 0 (upper word) */
   __IM uint32_t RESERVED7;
  __IOM uint32_t DATA_CHANNELS_L1;              /*!< 0x00000098 Data channel map 1 (lower word) */
  __IOM uint32_t DATA_CHANNELS_M1;              /*!< 0x0000009C Data channel map 1 (middle word) */
  __IOM uint32_t DATA_CHANNELS_H1;              /*!< 0x000000A0 Data channel map 1 (upper word) */
   __IM uint32_t RESERVED8;
  __IOM uint32_t CONN_INTR;                     /*!< 0x000000A8 Connection interrupt status and Clear register */
   __IM uint32_t CONN_STATUS;                   /*!< 0x000000AC Connection channel status */
  __IOM uint32_t CONN_INDEX;                    /*!< 0x000000B0 Connection Index register */
   __IM uint32_t RESERVED9;
  __IOM uint32_t WAKEUP_CONFIG;                 /*!< 0x000000B8 Wakeup configuration */
   __IM uint32_t RESERVED10;
  __IOM uint32_t WAKEUP_CONTROL;                /*!< 0x000000C0 Wakeup control */
  __IOM uint32_t CLOCK_CONFIG;                  /*!< 0x000000C4 Clock control */
   __IM uint32_t TIM_COUNTER_L;                 /*!< 0x000000C8 Reference Clock */
  __IOM uint32_t WAKEUP_CONFIG_EXTD;            /*!< 0x000000CC Wakeup configuration extended */
   __IM uint32_t RESERVED11[2];
  __IOM uint32_t POC_REG__TIM_CONTROL;          /*!< 0x000000D8 BLE Time Control */
   __IM uint32_t RESERVED12;
  __IOM uint32_t ADV_TX_DATA_FIFO;              /*!< 0x000000E0 Advertising data transmit FIFO. Access ADVCH_TX_FIFO. */
   __IM uint32_t RESERVED13;
  __IOM uint32_t ADV_SCN_RSP_TX_FIFO;           /*!< 0x000000E8 Advertising scan response data transmit FIFO. Access
                                                                ADVCH_TX_FIFO. */
   __IM uint32_t RESERVED14[3];
   __IM uint32_t INIT_SCN_ADV_RX_FIFO;          /*!< 0x000000F8 advertising scan response data receive data FIFO. Access
                                                                ADVRX_FIFO. */
   __IM uint32_t RESERVED15;
  __IOM uint32_t CONN_INTERVAL;                 /*!< 0x00000100 Connection Interval */
  __IOM uint32_t SUP_TIMEOUT;                   /*!< 0x00000104 Supervision timeout */
  __IOM uint32_t SLAVE_LATENCY;                 /*!< 0x00000108 Slave Latency */
  __IOM uint32_t CE_LENGTH;                     /*!< 0x0000010C Connection event length */
  __IOM uint32_t PDU_ACCESS_ADDR_L_REGISTER;    /*!< 0x00000110 Access address (lower) */
  __IOM uint32_t PDU_ACCESS_ADDR_H_REGISTER;    /*!< 0x00000114 Access address (upper) */
  __IOM uint32_t CONN_CE_INSTANT;               /*!< 0x00000118 Connection event instant */
  __IOM uint32_t CE_CNFG_STS_REGISTER;          /*!< 0x0000011C connection configuration & status register */
   __IM uint32_t NEXT_CE_INSTANT;               /*!< 0x00000120 Next connection event instant */
   __IM uint32_t CONN_CE_COUNTER;               /*!< 0x00000124 connection event counter */
  __IOM uint32_t DATA_LIST_SENT_UPDATE__STATUS; /*!< 0x00000128 data list sent update and status */
  __IOM uint32_t DATA_LIST_ACK_UPDATE__STATUS;  /*!< 0x0000012C data list ack update and status */
  __IOM uint32_t CE_CNFG_STS_REGISTER_EXT;      /*!< 0x00000130 connection configuration & status register */
  __IOM uint32_t CONN_EXT_INTR;                 /*!< 0x00000134 Connection extended interrupt status and Clear register */
  __IOM uint32_t CONN_EXT_INTR_MASK;            /*!< 0x00000138 Connection Extended Interrupt mask */
   __IM uint32_t RESERVED16;
  __IOM uint32_t DATA_MEM_DESCRIPTOR[5];        /*!< 0x00000140 Data buffer descriptor 0 to 4 */
   __IM uint32_t RESERVED17[3];
  __IOM uint32_t WINDOW_WIDEN_INTVL;            /*!< 0x00000160 Window widen for interval */
  __IOM uint32_t WINDOW_WIDEN_WINOFF;           /*!< 0x00000164 Window widen for offset */
   __IM uint32_t RESERVED18[2];
  __IOM uint32_t LE_RF_TEST_MODE;               /*!< 0x00000170 Direct Test Mode control */
   __IM uint32_t DTM_RX_PKT_COUNT;              /*!< 0x00000174 Direct Test Mode receive packet count */
  __IOM uint32_t LE_RF_TEST_MODE_EXT;           /*!< 0x00000178 Direct Test Mode control */
   __IM uint32_t RESERVED19[3];
   __IM uint32_t TXRX_HOP;                      /*!< 0x00000188 Channel Address register */
   __IM uint32_t RESERVED20;
  __IOM uint32_t TX_RX_ON_DELAY;                /*!< 0x00000190 Transmit/Receive data delay */
   __IM uint32_t RESERVED21[5];
  __IOM uint32_t ADV_ACCADDR_L;                 /*!< 0x000001A8 ADV packet access code low word */
  __IOM uint32_t ADV_ACCADDR_H;                 /*!< 0x000001AC ADV packet access code high word */
  __IOM uint32_t ADV_CH_TX_POWER_LVL_LS;        /*!< 0x000001B0 Advertising channel transmit power setting */
  __IOM uint32_t ADV_CH_TX_POWER_LVL_MS;        /*!< 0x000001B4 Advertising channel transmit power setting extension */
  __IOM uint32_t CONN_CH_TX_POWER_LVL_LS;       /*!< 0x000001B8 Connection channel transmit power setting */
  __IOM uint32_t CONN_CH_TX_POWER_LVL_MS;       /*!< 0x000001BC Connection channel transmit power setting extension */
  __IOM uint32_t DEV_PUB_ADDR_L;                /*!< 0x000001C0 Device public address lower register */
  __IOM uint32_t DEV_PUB_ADDR_M;                /*!< 0x000001C4 Device public address middle register */
  __IOM uint32_t DEV_PUB_ADDR_H;                /*!< 0x000001C8 Device public address higher register */
   __IM uint32_t RESERVED22;
  __IOM uint32_t OFFSET_TO_FIRST_INSTANT;       /*!< 0x000001D0 Offset to first instant */
  __IOM uint32_t ADV_CONFIG;                    /*!< 0x000001D4 Advertiser configuration register */
  __IOM uint32_t SCAN_CONFIG;                   /*!< 0x000001D8 Scan configuration register */
  __IOM uint32_t INIT_CONFIG;                   /*!< 0x000001DC Initiator configuration register */
  __IOM uint32_t CONN_CONFIG;                   /*!< 0x000001E0 Connection configuration register */
   __IM uint32_t RESERVED23;
  __IOM uint32_t CONN_PARAM1;                   /*!< 0x000001E8 Connection parameter 1 */
  __IOM uint32_t CONN_PARAM2;                   /*!< 0x000001EC Connection parameter 2 */
  __IOM uint32_t CONN_INTR_MASK;                /*!< 0x000001F0 Connection Interrupt mask */
  __IOM uint32_t SLAVE_TIMING_CONTROL;          /*!< 0x000001F4 slave timing control */
  __IOM uint32_t RECEIVE_TRIG_CTRL;             /*!< 0x000001F8 Receive trigger control */
   __IM uint32_t RESERVED24;
   __IM uint32_t LL_DBG_1;                      /*!< 0x00000200 LL debug register 1 */
   __IM uint32_t LL_DBG_2;                      /*!< 0x00000204 LL debug register 2 */
   __IM uint32_t LL_DBG_3;                      /*!< 0x00000208 LL debug register 3 */
   __IM uint32_t LL_DBG_4;                      /*!< 0x0000020C LL debug register 4 */
   __IM uint32_t LL_DBG_5;                      /*!< 0x00000210 LL debug register 5 */
   __IM uint32_t LL_DBG_6;                      /*!< 0x00000214 LL debug register 6 */
   __IM uint32_t LL_DBG_7;                      /*!< 0x00000218 LL debug register 7 */
   __IM uint32_t LL_DBG_8;                      /*!< 0x0000021C LL debug register 8 */
   __IM uint32_t LL_DBG_9;                      /*!< 0x00000220 LL debug register 9 */
   __IM uint32_t LL_DBG_10;                     /*!< 0x00000224 LL debug register 10 */
   __IM uint32_t RESERVED25[2];
  __IOM uint32_t PEER_ADDR_INIT_L;              /*!< 0x00000230 Lower 16 bit address of the peer device for INIT. */
  __IOM uint32_t PEER_ADDR_INIT_M;              /*!< 0x00000234 Middle 16 bit address of the peer device for INIT. */
  __IOM uint32_t PEER_ADDR_INIT_H;              /*!< 0x00000238 Higher 16 bit address of the peer device for INIT. */
  __IOM uint32_t PEER_SEC_ADDR_ADV_L;           /*!< 0x0000023C Lower 16 bits of the secondary address of the peer device for
                                                                ADV_DIR. */
  __IOM uint32_t PEER_SEC_ADDR_ADV_M;           /*!< 0x00000240 Middle 16 bits of the secondary address of the peer device for
                                                                ADV_DIR. */
  __IOM uint32_t PEER_SEC_ADDR_ADV_H;           /*!< 0x00000244 Higher 16 bits of the secondary address of the peer device for
                                                                ADV_DIR. */
  __IOM uint32_t INIT_WINDOW_TIMER_CTRL;        /*!< 0x00000248 Initiator Window NI timer control */
  __IOM uint32_t CONN_CONFIG_EXT;               /*!< 0x0000024C Connection extended configuration register */
   __IM uint32_t RESERVED26[2];
  __IOM uint32_t DPLL_CONFIG;                   /*!< 0x00000258 DPLL & CY Correlator configuration register */
   __IM uint32_t RESERVED27;
  __IOM uint32_t INIT_NI_VAL;                   /*!< 0x00000260 Initiator Window NI instant */
   __IM uint32_t INIT_WINDOW_OFFSET;            /*!< 0x00000264 Initiator Window offset captured at conn request */
   __IM uint32_t INIT_WINDOW_NI_ANCHOR_PT;      /*!< 0x00000268 Initiator Window NI anchor point captured at conn request */
   __IM uint32_t RESERVED28[78];
  __IOM uint32_t CONN_UPDATE_NEW_INTERVAL;      /*!< 0x000003A4 Connection update new interval */
  __IOM uint32_t CONN_UPDATE_NEW_LATENCY;       /*!< 0x000003A8 Connection update new latency */
  __IOM uint32_t CONN_UPDATE_NEW_SUP_TO;        /*!< 0x000003AC Connection update new supervision timeout */
  __IOM uint32_t CONN_UPDATE_NEW_SL_INTERVAL;   /*!< 0x000003B0 Connection update new Slave Latency X Conn interval Value */
   __IM uint32_t RESERVED29[3];
  __IOM uint32_t CONN_REQ_WORD0;                /*!< 0x000003C0 Connection request address word 0 */
  __IOM uint32_t CONN_REQ_WORD1;                /*!< 0x000003C4 Connection request address word 1 */
  __IOM uint32_t CONN_REQ_WORD2;                /*!< 0x000003C8 Connection request address word 2 */
  __IOM uint32_t CONN_REQ_WORD3;                /*!< 0x000003CC Connection request address word 3 */
  __IOM uint32_t CONN_REQ_WORD4;                /*!< 0x000003D0 Connection request address word 4 */
  __IOM uint32_t CONN_REQ_WORD5;                /*!< 0x000003D4 Connection request address word 5 */
  __IOM uint32_t CONN_REQ_WORD6;                /*!< 0x000003D8 Connection request address word 6 */
  __IOM uint32_t CONN_REQ_WORD7;                /*!< 0x000003DC Connection request address word 7 */
  __IOM uint32_t CONN_REQ_WORD8;                /*!< 0x000003E0 Connection request address word 8 */
  __IOM uint32_t CONN_REQ_WORD9;                /*!< 0x000003E4 Connection request address word 9 */
  __IOM uint32_t CONN_REQ_WORD10;               /*!< 0x000003E8 Connection request address word 10 */
  __IOM uint32_t CONN_REQ_WORD11;               /*!< 0x000003EC Connection request address word 11 */
   __IM uint32_t RESERVED30[389];
  __IOM uint32_t PDU_RESP_TIMER;                /*!< 0x00000A04 PDU response timer/Generic Timer (MMMS mode) */
   __IM uint32_t NEXT_RESP_TIMER_EXP;           /*!< 0x00000A08 Next response timeout instant */
   __IM uint32_t NEXT_SUP_TO;                   /*!< 0x00000A0C Next supervision timeout instant */
  __IOM uint32_t LLH_FEATURE_CONFIG;            /*!< 0x00000A10 Feature enable */
  __IOM uint32_t WIN_MIN_STEP_SIZE;             /*!< 0x00000A14 Window minimum step size */
  __IOM uint32_t SLV_WIN_ADJ;                   /*!< 0x00000A18 Slave window adjustment */
  __IOM uint32_t SL_CONN_INTERVAL;              /*!< 0x00000A1C Slave Latency X Conn Interval Value */
  __IOM uint32_t LE_PING_TIMER_ADDR;            /*!< 0x00000A20 LE Ping connection timer address */
  __IOM uint32_t LE_PING_TIMER_OFFSET;          /*!< 0x00000A24 LE Ping connection timer offset */
   __IM uint32_t LE_PING_TIMER_NEXT_EXP;        /*!< 0x00000A28 LE Ping timer next expiry instant */
   __IM uint32_t LE_PING_TIMER_WRAP_COUNT;      /*!< 0x00000A2C LE Ping Timer wrap count */
   __IM uint32_t RESERVED31[244];
  __IOM uint32_t TX_EN_EXT_DELAY;               /*!< 0x00000E00 Transmit enable extension delay */
  __IOM uint32_t TX_RX_SYNTH_DELAY;             /*!< 0x00000E04 Transmit/Receive enable delay */
  __IOM uint32_t EXT_PA_LNA_DLY_CNFG;           /*!< 0x00000E08 External TX PA and RX LNA delay configuration */
   __IM uint32_t RESERVED32;
  __IOM uint32_t LL_CONFIG;                     /*!< 0x00000E10 Link Layer additional configuration */
   __IM uint32_t RESERVED33[59];
  __IOM uint32_t LL_CONTROL;                    /*!< 0x00000F00 LL Backward compatibility */
  __IOM uint32_t DEV_PA_ADDR_L;                 /*!< 0x00000F04 Device Resolvable/Non-Resolvable Private address lower register */
  __IOM uint32_t DEV_PA_ADDR_M;                 /*!< 0x00000F08 Device Resolvable/Non-Resolvable Private address middle
                                                                register */
  __IOM uint32_t DEV_PA_ADDR_H;                 /*!< 0x00000F0C Device Resolvable/Non-Resolvable Private address higher
                                                                register */
  __IOM uint32_t RSLV_LIST_ENABLE[16];          /*!< 0x00000F10 Resolving list entry control bit */
   __IM uint32_t RESERVED34[20];
  __IOM uint32_t WL_CONNECTION_STATUS;          /*!< 0x00000FA0 whitelist valid entry bit */
   __IM uint32_t RESERVED35[535];
  __IOM uint32_t CONN_RXMEM_BASE_ADDR_DLE;      /*!< 0x00001800 DLE Connection RX memory base address */
   __IM uint32_t RESERVED36[1023];
  __IOM uint32_t CONN_TXMEM_BASE_ADDR_DLE;      /*!< 0x00002800 DLE Connection TX memory base address */
   __IM uint32_t RESERVED37[16383];
  __IOM uint32_t CONN_1_PARAM_MEM_BASE_ADDR;    /*!< 0x00012800 Connection Parameter memory base address for connection 1 */
   __IM uint32_t RESERVED38[31];
  __IOM uint32_t CONN_2_PARAM_MEM_BASE_ADDR;    /*!< 0x00012880 Connection Parameter memory base address for connection 2 */
   __IM uint32_t RESERVED39[31];
  __IOM uint32_t CONN_3_PARAM_MEM_BASE_ADDR;    /*!< 0x00012900 Connection Parameter memory base address for connection 3 */
   __IM uint32_t RESERVED40[31];
  __IOM uint32_t CONN_4_PARAM_MEM_BASE_ADDR;    /*!< 0x00012980 Connection Parameter memory base address for connection 4 */
   __IM uint32_t RESERVED41[1439];
  __IOM uint32_t NI_TIMER;                      /*!< 0x00014000 Next Instant Timer */
  __IOM uint32_t US_OFFSET;                     /*!< 0x00014004 Micro-second Offset */
  __IOM uint32_t NEXT_CONN;                     /*!< 0x00014008 Next Connection */
  __IOM uint32_t NI_ABORT;                      /*!< 0x0001400C Abort next scheduled connection */
   __IM uint32_t RESERVED42[4];
   __IM uint32_t CONN_NI_STATUS;                /*!< 0x00014020 Connection NI Status */
   __IM uint32_t NEXT_SUP_TO_STATUS;            /*!< 0x00014024 Next Supervision timeout Status */
   __IM uint32_t MMMS_CONN_STATUS;              /*!< 0x00014028 Connection Status */
   __IM uint32_t BT_SLOT_CAPT_STATUS;           /*!< 0x0001402C BT Slot Captured Status */
   __IM uint32_t US_CAPT_STATUS;                /*!< 0x00014030 Micro-second Capture Status */
   __IM uint32_t US_OFFSET_STATUS;              /*!< 0x00014034 Micro-second Offset Status */
   __IM uint32_t ACCU_WINDOW_WIDEN_STATUS;      /*!< 0x00014038 Accumulated Window Widen Status */
   __IM uint32_t EARLY_INTR_STATUS;             /*!< 0x0001403C Status when early interrupt is raised */
  __IOM uint32_t MMMS_CONFIG;                   /*!< 0x00014040 Multi-Master Multi-Slave Config */
   __IM uint32_t US_COUNTER;                    /*!< 0x00014044 Running US of the current BT Slot */
  __IOM uint32_t US_CAPT_PREV;                  /*!< 0x00014048 Previous captured US of the BT Slot */
   __IM uint32_t EARLY_INTR_NI;                 /*!< 0x0001404C NI at early interrupt */
   __IM uint32_t RESERVED43[12];
   __IM uint32_t MMMS_MASTER_CREATE_BT_CAPT;    /*!< 0x00014080 BT slot capture for master connection creation */
   __IM uint32_t MMMS_SLAVE_CREATE_BT_CAPT;     /*!< 0x00014084 BT slot capture for slave connection creation */
   __IM uint32_t MMMS_SLAVE_CREATE_US_CAPT;     /*!< 0x00014088 Micro second capture for slave connection creation */
   __IM uint32_t RESERVED44[29];
  __IOM uint32_t MMMS_DATA_MEM_DESCRIPTOR[16];  /*!< 0x00014100 Data buffer descriptor 0 to 15 */
   __IM uint32_t RESERVED45[48];
  __IOM uint32_t CONN_1_DATA_LIST_SENT;         /*!< 0x00014200 data list sent update and status for connection 1 */
  __IOM uint32_t CONN_1_DATA_LIST_ACK;          /*!< 0x00014204 data list ack update and status for connection 1 */
  __IOM uint32_t CONN_1_CE_DATA_LIST_CFG;       /*!< 0x00014208 Connection specific pause resume for connection 1 */
   __IM uint32_t RESERVED46;
  __IOM uint32_t CONN_2_DATA_LIST_SENT;         /*!< 0x00014210 data list sent update and status for connection 2 */
  __IOM uint32_t CONN_2_DATA_LIST_ACK;          /*!< 0x00014214 data list ack update and status for connection 2 */
  __IOM uint32_t CONN_2_CE_DATA_LIST_CFG;       /*!< 0x00014218 Connection specific pause resume for connection 2 */
   __IM uint32_t RESERVED47;
  __IOM uint32_t CONN_3_DATA_LIST_SENT;         /*!< 0x00014220 data list sent update and status for connection 3 */
  __IOM uint32_t CONN_3_DATA_LIST_ACK;          /*!< 0x00014224 data list ack update and status for connection 3 */
  __IOM uint32_t CONN_3_CE_DATA_LIST_CFG;       /*!< 0x00014228 Connection specific pause resume for connection 3 */
   __IM uint32_t RESERVED48;
  __IOM uint32_t CONN_4_DATA_LIST_SENT;         /*!< 0x00014230 data list sent update and status for connection 4 */
  __IOM uint32_t CONN_4_DATA_LIST_ACK;          /*!< 0x00014234 data list ack update and status for connection 4 */
  __IOM uint32_t CONN_4_CE_DATA_LIST_CFG;       /*!< 0x00014238 Connection specific pause resume for connection 4 */
   __IM uint32_t RESERVED49[113];
  __IOM uint32_t MMMS_ADVCH_NI_ENABLE;          /*!< 0x00014400 Enable bits for ADV_NI, SCAN_NI and INIT_NI */
  __IOM uint32_t MMMS_ADVCH_NI_VALID;           /*!< 0x00014404 Next instant valid for ADV, SCAN, INIT */
  __IOM uint32_t MMMS_ADVCH_NI_ABORT;           /*!< 0x00014408 Abort the next instant of ADV, SCAN, INIT */
   __IM uint32_t RESERVED50;
  __IOM uint32_t CONN_PARAM_NEXT_SUP_TO;        /*!< 0x00014410 Register to configure the supervision timeout for next
                                                                scheduled connection */
  __IOM uint32_t CONN_PARAM_ACC_WIN_WIDEN;      /*!< 0x00014414 Register to configure Accumulated window widening for next
                                                                scheduled connection */
   __IM uint32_t RESERVED51[2];
  __IOM uint32_t HW_LOAD_OFFSET;                /*!< 0x00014420 Register to configure offset from connection anchor point at
                                                                which connection parameter memory should be read */
   __IM uint32_t ADV_RAND;                      /*!< 0x00014424 Random number generated by Hardware for ADV NI calculation */
   __IM uint32_t MMMS_RX_PKT_CNTR;              /*!< 0x00014428 Packet Counter of packets in RX FIFO in MMMS mode */
   __IM uint32_t RESERVED52;
   __IM uint32_t CONN_RX_PKT_CNTR[8];           /*!< 0x00014430 Packet Counter for Individual connection index */
   __IM uint32_t RESERVED53[236];
  __IOM uint32_t WHITELIST_BASE_ADDR;           /*!< 0x00014800 Whitelist base address */
   __IM uint32_t RESERVED54[47];
  __IOM uint32_t RSLV_LIST_PEER_IDNTT_BASE_ADDR; /*!< 0x000148C0 Resolving list base address for storing Peer Identity address */
   __IM uint32_t RESERVED55[47];
  __IOM uint32_t RSLV_LIST_PEER_RPA_BASE_ADDR;  /*!< 0x00014980 Resolving list base address for storing resolved Peer RPA
                                                                address */
   __IM uint32_t RESERVED56[47];
  __IOM uint32_t RSLV_LIST_RCVD_INIT_RPA_BASE_ADDR; /*!< 0x00014A40 Resolving list base address for storing Resolved received INITA
                                                                RPA */
   __IM uint32_t RESERVED57[47];
  __IOM uint32_t RSLV_LIST_TX_INIT_RPA_BASE_ADDR; /*!< 0x00014B00 Resolving list base address for storing generated TX INITA RPA */
   __IM uint32_t RESERVED58[9535];
} BLE_BLELL_V1_Type;                            /*!< Size = 122880 (0x1E000) */

/**
  * \brief Bluetooth Low Energy Subsystem Miscellaneous (BLE_BLESS)
  */
typedef struct {
   __IM uint32_t RESERVED[24];
  __IOM uint32_t DDFT_CONFIG;                   /*!< 0x00000060 BLESS DDFT configuration register */
  __IOM uint32_t XTAL_CLK_DIV_CONFIG;           /*!< 0x00000064 Crystal clock divider configuration register */
  __IOM uint32_t INTR_STAT;                     /*!< 0x00000068 Link Layer interrupt status register */
  __IOM uint32_t INTR_MASK;                     /*!< 0x0000006C Link Layer interrupt mask register */
  __IOM uint32_t LL_CLK_EN;                     /*!< 0x00000070 Link Layer primary clock enable */
  __IOM uint32_t LF_CLK_CTRL;                   /*!< 0x00000074 BLESS LF clock control and BLESS revision ID indicator */
  __IOM uint32_t EXT_PA_LNA_CTRL;               /*!< 0x00000078 External TX PA and RX LNA control */
   __IM uint32_t RESERVED1;
   __IM uint32_t LL_PKT_RSSI_CH_ENERGY;         /*!< 0x00000080 Link Layer Last Received packet RSSI/Channel energy and channel
                                                                number */
   __IM uint32_t BT_CLOCK_CAPT;                 /*!< 0x00000084 BT clock captured on an LL DSM exit */
   __IM uint32_t RESERVED2[6];
  __IOM uint32_t MT_CFG;                        /*!< 0x000000A0 MT Configuration Register */
  __IOM uint32_t MT_DELAY_CFG;                  /*!< 0x000000A4 MT Delay configuration for state transitions */
  __IOM uint32_t MT_DELAY_CFG2;                 /*!< 0x000000A8 MT Delay configuration for state transitions */
  __IOM uint32_t MT_DELAY_CFG3;                 /*!< 0x000000AC MT Delay configuration for state transitions */
  __IOM uint32_t MT_VIO_CTRL;                   /*!< 0x000000B0 MT Configuration Register to control VIO switches */
   __IM uint32_t MT_STATUS;                     /*!< 0x000000B4 MT Status Register */
   __IM uint32_t PWR_CTRL_SM_ST;                /*!< 0x000000B8 Link Layer Power Control FSM Status Register */
   __IM uint32_t RESERVED3;
  __IOM uint32_t HVLDO_CTRL;                    /*!< 0x000000C0 HVLDO Configuration register */
  __IOM uint32_t MISC_EN_CTRL;                  /*!< 0x000000C4 Radio Buck and Active regulator enable control */
   __IM uint32_t RESERVED4[2];
  __IOM uint32_t EFUSE_CONFIG;                  /*!< 0x000000D0 EFUSE mode configuration register */
  __IOM uint32_t EFUSE_TIM_CTRL1;               /*!< 0x000000D4 EFUSE timing control register (common for Program and Read
                                                                modes) */
  __IOM uint32_t EFUSE_TIM_CTRL2;               /*!< 0x000000D8 EFUSE timing control Register (for Read) */
  __IOM uint32_t EFUSE_TIM_CTRL3;               /*!< 0x000000DC EFUSE timing control Register (for Program) */
   __IM uint32_t EFUSE_RDATA_L;                 /*!< 0x000000E0 EFUSE Lower read data */
   __IM uint32_t EFUSE_RDATA_H;                 /*!< 0x000000E4 EFUSE higher read data */
  __IOM uint32_t EFUSE_WDATA_L;                 /*!< 0x000000E8 EFUSE lower write word */
  __IOM uint32_t EFUSE_WDATA_H;                 /*!< 0x000000EC EFUSE higher write word */
  __IOM uint32_t DIV_BY_625_CFG;                /*!< 0x000000F0 Divide by 625 for FW Use */
   __IM uint32_t DIV_BY_625_STS;                /*!< 0x000000F4 Output of divide by 625 divider */
   __IM uint32_t RESERVED5[2];
  __IOM uint32_t PACKET_COUNTER0;               /*!< 0x00000100 Packet counter 0 */
  __IOM uint32_t PACKET_COUNTER2;               /*!< 0x00000104 Packet counter 2 */
  __IOM uint32_t IV_MASTER0;                    /*!< 0x00000108 Master Initialization Vector 0 */
  __IOM uint32_t IV_SLAVE0;                     /*!< 0x0000010C Slave Initialization Vector 0 */
   __OM uint32_t ENC_KEY[4];                    /*!< 0x00000110 Encryption Key register 0-3 */
  __IOM uint32_t MIC_IN0;                       /*!< 0x00000120 MIC input register */
   __IM uint32_t MIC_OUT0;                      /*!< 0x00000124 MIC output register */
  __IOM uint32_t ENC_PARAMS;                    /*!< 0x00000128 Encryption Parameter register */
  __IOM uint32_t ENC_CONFIG;                    /*!< 0x0000012C Encryption Configuration */
  __IOM uint32_t ENC_INTR_EN;                   /*!< 0x00000130 Encryption Interrupt enable */
  __IOM uint32_t ENC_INTR;                      /*!< 0x00000134 Encryption Interrupt status and clear register */
   __IM uint32_t RESERVED6[2];
  __IOM uint32_t B1_DATA_REG[4];                /*!< 0x00000140 Programmable B1 Data register (0-3) */
  __IOM uint32_t ENC_MEM_BASE_ADDR;             /*!< 0x00000150 Encryption memory base address */
   __IM uint32_t RESERVED7[875];
  __IOM uint32_t TRIM_LDO_0;                    /*!< 0x00000F00 LDO Trim register 0 */
  __IOM uint32_t TRIM_LDO_1;                    /*!< 0x00000F04 LDO Trim register 1 */
  __IOM uint32_t TRIM_LDO_2;                    /*!< 0x00000F08 LDO Trim register 2 */
  __IOM uint32_t TRIM_LDO_3;                    /*!< 0x00000F0C LDO Trim register 3 */
  __IOM uint32_t TRIM_MXD[4];                   /*!< 0x00000F10 MXD die Trim registers */
   __IM uint32_t RESERVED8[4];
  __IOM uint32_t TRIM_LDO_4;                    /*!< 0x00000F30 LDO Trim register 4 */
  __IOM uint32_t TRIM_LDO_5;                    /*!< 0x00000F34 LDO Trim register 5 */
   __IM uint32_t RESERVED9[50];
} BLE_BLESS_V1_Type;                            /*!< Size = 4096 (0x1000) */

/**
  * \brief Bluetooth Low Energy Subsystem (BLE)
  */
typedef struct {
        BLE_RCB_V1_Type RCB;                    /*!< 0x00000000 Radio Control Bus (RCB) controller */
   __IM uint32_t RESERVED[896];
        BLE_BLELL_V1_Type BLELL;                /*!< 0x00001000 Bluetooth Low Energy Link Layer */
        BLE_BLESS_V1_Type BLESS;                /*!< 0x0001F000 Bluetooth Low Energy Subsystem Miscellaneous */
} BLE_V1_Type;                                  /*!< Size = 131072 (0x20000) */


/* BLE_RCB_RCBLL.CTRL */
#define BLE_RCB_RCBLL_CTRL_RCBLL_CTRL_Pos       0UL
#define BLE_RCB_RCBLL_CTRL_RCBLL_CTRL_Msk       0x1UL
#define BLE_RCB_RCBLL_CTRL_RCBLL_CPU_REQ_Pos    1UL
#define BLE_RCB_RCBLL_CTRL_RCBLL_CPU_REQ_Msk    0x2UL
#define BLE_RCB_RCBLL_CTRL_CPU_SINGLE_WRITE_Pos 2UL
#define BLE_RCB_RCBLL_CTRL_CPU_SINGLE_WRITE_Msk 0x4UL
#define BLE_RCB_RCBLL_CTRL_CPU_SINGLE_READ_Pos  3UL
#define BLE_RCB_RCBLL_CTRL_CPU_SINGLE_READ_Msk  0x8UL
#define BLE_RCB_RCBLL_CTRL_ALLOW_CPU_ACCESS_TX_RX_Pos 4UL
#define BLE_RCB_RCBLL_CTRL_ALLOW_CPU_ACCESS_TX_RX_Msk 0x10UL
#define BLE_RCB_RCBLL_CTRL_ENABLE_RADIO_BOD_Pos 5UL
#define BLE_RCB_RCBLL_CTRL_ENABLE_RADIO_BOD_Msk 0x20UL
/* BLE_RCB_RCBLL.INTR */
#define BLE_RCB_RCBLL_INTR_RCB_LL_DONE_Pos      0UL
#define BLE_RCB_RCBLL_INTR_RCB_LL_DONE_Msk      0x1UL
#define BLE_RCB_RCBLL_INTR_SINGLE_WRITE_DONE_Pos 2UL
#define BLE_RCB_RCBLL_INTR_SINGLE_WRITE_DONE_Msk 0x4UL
#define BLE_RCB_RCBLL_INTR_SINGLE_READ_DONE_Pos 3UL
#define BLE_RCB_RCBLL_INTR_SINGLE_READ_DONE_Msk 0x8UL
/* BLE_RCB_RCBLL.INTR_SET */
#define BLE_RCB_RCBLL_INTR_SET_RCB_LL_DONE_Pos  0UL
#define BLE_RCB_RCBLL_INTR_SET_RCB_LL_DONE_Msk  0x1UL
#define BLE_RCB_RCBLL_INTR_SET_SINGLE_WRITE_DONE_Pos 2UL
#define BLE_RCB_RCBLL_INTR_SET_SINGLE_WRITE_DONE_Msk 0x4UL
#define BLE_RCB_RCBLL_INTR_SET_SINGLE_READ_DONE_Pos 3UL
#define BLE_RCB_RCBLL_INTR_SET_SINGLE_READ_DONE_Msk 0x8UL
/* BLE_RCB_RCBLL.INTR_MASK */
#define BLE_RCB_RCBLL_INTR_MASK_RCB_LL_DONE_Pos 0UL
#define BLE_RCB_RCBLL_INTR_MASK_RCB_LL_DONE_Msk 0x1UL
#define BLE_RCB_RCBLL_INTR_MASK_SINGLE_WRITE_DONE_Pos 2UL
#define BLE_RCB_RCBLL_INTR_MASK_SINGLE_WRITE_DONE_Msk 0x4UL
#define BLE_RCB_RCBLL_INTR_MASK_SINGLE_READ_DONE_Pos 3UL
#define BLE_RCB_RCBLL_INTR_MASK_SINGLE_READ_DONE_Msk 0x8UL
/* BLE_RCB_RCBLL.INTR_MASKED */
#define BLE_RCB_RCBLL_INTR_MASKED_RCB_LL_DONE_Pos 0UL
#define BLE_RCB_RCBLL_INTR_MASKED_RCB_LL_DONE_Msk 0x1UL
#define BLE_RCB_RCBLL_INTR_MASKED_SINGLE_WRITE_DONE_Pos 2UL
#define BLE_RCB_RCBLL_INTR_MASKED_SINGLE_WRITE_DONE_Msk 0x4UL
#define BLE_RCB_RCBLL_INTR_MASKED_SINGLE_READ_DONE_Pos 3UL
#define BLE_RCB_RCBLL_INTR_MASKED_SINGLE_READ_DONE_Msk 0x8UL
/* BLE_RCB_RCBLL.RADIO_REG1_ADDR */
#define BLE_RCB_RCBLL_RADIO_REG1_ADDR_REG_ADDR_Pos 0UL
#define BLE_RCB_RCBLL_RADIO_REG1_ADDR_REG_ADDR_Msk 0xFFFFUL
/* BLE_RCB_RCBLL.RADIO_REG2_ADDR */
#define BLE_RCB_RCBLL_RADIO_REG2_ADDR_REG_ADDR_Pos 0UL
#define BLE_RCB_RCBLL_RADIO_REG2_ADDR_REG_ADDR_Msk 0xFFFFUL
/* BLE_RCB_RCBLL.RADIO_REG3_ADDR */
#define BLE_RCB_RCBLL_RADIO_REG3_ADDR_REG_ADDR_Pos 0UL
#define BLE_RCB_RCBLL_RADIO_REG3_ADDR_REG_ADDR_Msk 0xFFFFUL
/* BLE_RCB_RCBLL.RADIO_REG4_ADDR */
#define BLE_RCB_RCBLL_RADIO_REG4_ADDR_REG_ADDR_Pos 0UL
#define BLE_RCB_RCBLL_RADIO_REG4_ADDR_REG_ADDR_Msk 0xFFFFUL
/* BLE_RCB_RCBLL.RADIO_REG5_ADDR */
#define BLE_RCB_RCBLL_RADIO_REG5_ADDR_REG_ADDR_Pos 0UL
#define BLE_RCB_RCBLL_RADIO_REG5_ADDR_REG_ADDR_Msk 0xFFFFUL
/* BLE_RCB_RCBLL.CPU_WRITE_REG */
#define BLE_RCB_RCBLL_CPU_WRITE_REG_ADDR_Pos    0UL
#define BLE_RCB_RCBLL_CPU_WRITE_REG_ADDR_Msk    0xFFFFUL
#define BLE_RCB_RCBLL_CPU_WRITE_REG_WRITE_DATA_Pos 16UL
#define BLE_RCB_RCBLL_CPU_WRITE_REG_WRITE_DATA_Msk 0xFFFF0000UL
/* BLE_RCB_RCBLL.CPU_READ_REG */
#define BLE_RCB_RCBLL_CPU_READ_REG_ADDR_Pos     0UL
#define BLE_RCB_RCBLL_CPU_READ_REG_ADDR_Msk     0xFFFFUL
#define BLE_RCB_RCBLL_CPU_READ_REG_READ_DATA_Pos 16UL
#define BLE_RCB_RCBLL_CPU_READ_REG_READ_DATA_Msk 0xFFFF0000UL


/* BLE_RCB.CTRL */
#define BLE_RCB_CTRL_TX_CLK_EDGE_Pos            1UL
#define BLE_RCB_CTRL_TX_CLK_EDGE_Msk            0x2UL
#define BLE_RCB_CTRL_RX_CLK_EDGE_Pos            2UL
#define BLE_RCB_CTRL_RX_CLK_EDGE_Msk            0x4UL
#define BLE_RCB_CTRL_RX_CLK_SRC_Pos             3UL
#define BLE_RCB_CTRL_RX_CLK_SRC_Msk             0x8UL
#define BLE_RCB_CTRL_SCLK_CONTINUOUS_Pos        4UL
#define BLE_RCB_CTRL_SCLK_CONTINUOUS_Msk        0x10UL
#define BLE_RCB_CTRL_SSEL_POLARITY_Pos          5UL
#define BLE_RCB_CTRL_SSEL_POLARITY_Msk          0x20UL
#define BLE_RCB_CTRL_LEAD_Pos                   8UL
#define BLE_RCB_CTRL_LEAD_Msk                   0x300UL
#define BLE_RCB_CTRL_LAG_Pos                    10UL
#define BLE_RCB_CTRL_LAG_Msk                    0xC00UL
#define BLE_RCB_CTRL_DIV_ENABLED_Pos            12UL
#define BLE_RCB_CTRL_DIV_ENABLED_Msk            0x1000UL
#define BLE_RCB_CTRL_DIV_Pos                    13UL
#define BLE_RCB_CTRL_DIV_Msk                    0x7E000UL
#define BLE_RCB_CTRL_ADDR_WIDTH_Pos             19UL
#define BLE_RCB_CTRL_ADDR_WIDTH_Msk             0x780000UL
#define BLE_RCB_CTRL_DATA_WIDTH_Pos             23UL
#define BLE_RCB_CTRL_DATA_WIDTH_Msk             0x800000UL
#define BLE_RCB_CTRL_ENABLED_Pos                31UL
#define BLE_RCB_CTRL_ENABLED_Msk                0x80000000UL
/* BLE_RCB.STATUS */
#define BLE_RCB_STATUS_BUS_BUSY_Pos             0UL
#define BLE_RCB_STATUS_BUS_BUSY_Msk             0x1UL
/* BLE_RCB.TX_CTRL */
#define BLE_RCB_TX_CTRL_MSB_FIRST_Pos           0UL
#define BLE_RCB_TX_CTRL_MSB_FIRST_Msk           0x1UL
#define BLE_RCB_TX_CTRL_FIFO_RECONFIG_Pos       1UL
#define BLE_RCB_TX_CTRL_FIFO_RECONFIG_Msk       0x2UL
#define BLE_RCB_TX_CTRL_TX_ENTRIES_Pos          2UL
#define BLE_RCB_TX_CTRL_TX_ENTRIES_Msk          0x7CUL
/* BLE_RCB.TX_FIFO_CTRL */
#define BLE_RCB_TX_FIFO_CTRL_TX_TRIGGER_LEVEL_Pos 0UL
#define BLE_RCB_TX_FIFO_CTRL_TX_TRIGGER_LEVEL_Msk 0x1FUL
#define BLE_RCB_TX_FIFO_CTRL_CLEAR_Pos          16UL
#define BLE_RCB_TX_FIFO_CTRL_CLEAR_Msk          0x10000UL
/* BLE_RCB.TX_FIFO_STATUS */
#define BLE_RCB_TX_FIFO_STATUS_USED_Pos         0UL
#define BLE_RCB_TX_FIFO_STATUS_USED_Msk         0x1FUL
#define BLE_RCB_TX_FIFO_STATUS_SR_VALID_Pos     15UL
#define BLE_RCB_TX_FIFO_STATUS_SR_VALID_Msk     0x8000UL
#define BLE_RCB_TX_FIFO_STATUS_RD_PTR_Pos       16UL
#define BLE_RCB_TX_FIFO_STATUS_RD_PTR_Msk       0xF0000UL
#define BLE_RCB_TX_FIFO_STATUS_WR_PTR_Pos       24UL
#define BLE_RCB_TX_FIFO_STATUS_WR_PTR_Msk       0xF000000UL
/* BLE_RCB.TX_FIFO_WR */
#define BLE_RCB_TX_FIFO_WR_DATA_Pos             0UL
#define BLE_RCB_TX_FIFO_WR_DATA_Msk             0xFFFFFFFFUL
/* BLE_RCB.RX_CTRL */
#define BLE_RCB_RX_CTRL_MSB_FIRST_Pos           0UL
#define BLE_RCB_RX_CTRL_MSB_FIRST_Msk           0x1UL
/* BLE_RCB.RX_FIFO_CTRL */
#define BLE_RCB_RX_FIFO_CTRL_TRIGGER_LEVEL_Pos  0UL
#define BLE_RCB_RX_FIFO_CTRL_TRIGGER_LEVEL_Msk  0xFUL
#define BLE_RCB_RX_FIFO_CTRL_CLEAR_Pos          16UL
#define BLE_RCB_RX_FIFO_CTRL_CLEAR_Msk          0x10000UL
/* BLE_RCB.RX_FIFO_STATUS */
#define BLE_RCB_RX_FIFO_STATUS_USED_Pos         0UL
#define BLE_RCB_RX_FIFO_STATUS_USED_Msk         0x1FUL
#define BLE_RCB_RX_FIFO_STATUS_SR_VALID_Pos     15UL
#define BLE_RCB_RX_FIFO_STATUS_SR_VALID_Msk     0x8000UL
#define BLE_RCB_RX_FIFO_STATUS_RD_PTR_Pos       16UL
#define BLE_RCB_RX_FIFO_STATUS_RD_PTR_Msk       0xF0000UL
#define BLE_RCB_RX_FIFO_STATUS_WR_PTR_Pos       24UL
#define BLE_RCB_RX_FIFO_STATUS_WR_PTR_Msk       0xF000000UL
/* BLE_RCB.RX_FIFO_RD */
#define BLE_RCB_RX_FIFO_RD_DATA_Pos             0UL
#define BLE_RCB_RX_FIFO_RD_DATA_Msk             0xFFFFFFFFUL
/* BLE_RCB.RX_FIFO_RD_SILENT */
#define BLE_RCB_RX_FIFO_RD_SILENT_DATA_Pos      0UL
#define BLE_RCB_RX_FIFO_RD_SILENT_DATA_Msk      0xFFFFFFFFUL
/* BLE_RCB.INTR */
#define BLE_RCB_INTR_RCB_DONE_Pos               0UL
#define BLE_RCB_INTR_RCB_DONE_Msk               0x1UL
#define BLE_RCB_INTR_TX_FIFO_TRIGGER_Pos        8UL
#define BLE_RCB_INTR_TX_FIFO_TRIGGER_Msk        0x100UL
#define BLE_RCB_INTR_TX_FIFO_NOT_FULL_Pos       9UL
#define BLE_RCB_INTR_TX_FIFO_NOT_FULL_Msk       0x200UL
#define BLE_RCB_INTR_TX_FIFO_EMPTY_Pos          10UL
#define BLE_RCB_INTR_TX_FIFO_EMPTY_Msk          0x400UL
#define BLE_RCB_INTR_TX_FIFO_OVERFLOW_Pos       11UL
#define BLE_RCB_INTR_TX_FIFO_OVERFLOW_Msk       0x800UL
#define BLE_RCB_INTR_TX_FIFO_UNDERFLOW_Pos      12UL
#define BLE_RCB_INTR_TX_FIFO_UNDERFLOW_Msk      0x1000UL
#define BLE_RCB_INTR_RX_FIFO_TRIGGER_Pos        16UL
#define BLE_RCB_INTR_RX_FIFO_TRIGGER_Msk        0x10000UL
#define BLE_RCB_INTR_RX_FIFO_NOT_EMPTY_Pos      17UL
#define BLE_RCB_INTR_RX_FIFO_NOT_EMPTY_Msk      0x20000UL
#define BLE_RCB_INTR_RX_FIFO_FULL_Pos           18UL
#define BLE_RCB_INTR_RX_FIFO_FULL_Msk           0x40000UL
#define BLE_RCB_INTR_RX_FIFO_OVERFLOW_Pos       19UL
#define BLE_RCB_INTR_RX_FIFO_OVERFLOW_Msk       0x80000UL
#define BLE_RCB_INTR_RX_FIFO_UNDERFLOW_Pos      20UL
#define BLE_RCB_INTR_RX_FIFO_UNDERFLOW_Msk      0x100000UL
/* BLE_RCB.INTR_SET */
#define BLE_RCB_INTR_SET_RCB_DONE_Pos           0UL
#define BLE_RCB_INTR_SET_RCB_DONE_Msk           0x1UL
#define BLE_RCB_INTR_SET_TX_FIFO_TRIGGER_Pos    8UL
#define BLE_RCB_INTR_SET_TX_FIFO_TRIGGER_Msk    0x100UL
#define BLE_RCB_INTR_SET_TX_FIFO_NOT_FULL_Pos   9UL
#define BLE_RCB_INTR_SET_TX_FIFO_NOT_FULL_Msk   0x200UL
#define BLE_RCB_INTR_SET_TX_FIFO_EMPTY_Pos      10UL
#define BLE_RCB_INTR_SET_TX_FIFO_EMPTY_Msk      0x400UL
#define BLE_RCB_INTR_SET_TX_FIFO_OVERFLOW_Pos   11UL
#define BLE_RCB_INTR_SET_TX_FIFO_OVERFLOW_Msk   0x800UL
#define BLE_RCB_INTR_SET_TX_FIFO_UNDERFLOW_Pos  12UL
#define BLE_RCB_INTR_SET_TX_FIFO_UNDERFLOW_Msk  0x1000UL
#define BLE_RCB_INTR_SET_RX_FIFO_TRIGGER_Pos    16UL
#define BLE_RCB_INTR_SET_RX_FIFO_TRIGGER_Msk    0x10000UL
#define BLE_RCB_INTR_SET_RX_FIFO_NOT_EMPTY_Pos  17UL
#define BLE_RCB_INTR_SET_RX_FIFO_NOT_EMPTY_Msk  0x20000UL
#define BLE_RCB_INTR_SET_RX_FIFO_FULL_Pos       18UL
#define BLE_RCB_INTR_SET_RX_FIFO_FULL_Msk       0x40000UL
#define BLE_RCB_INTR_SET_RX_FIFO_OVERFLOW_Pos   19UL
#define BLE_RCB_INTR_SET_RX_FIFO_OVERFLOW_Msk   0x80000UL
#define BLE_RCB_INTR_SET_RX_FIFO_UNDERFLOW_Pos  20UL
#define BLE_RCB_INTR_SET_RX_FIFO_UNDERFLOW_Msk  0x100000UL
/* BLE_RCB.INTR_MASK */
#define BLE_RCB_INTR_MASK_RCB_DONE_Pos          0UL
#define BLE_RCB_INTR_MASK_RCB_DONE_Msk          0x1UL
#define BLE_RCB_INTR_MASK_TX_FIFO_TRIGGER_Pos   8UL
#define BLE_RCB_INTR_MASK_TX_FIFO_TRIGGER_Msk   0x100UL
#define BLE_RCB_INTR_MASK_TX_FIFO_NOT_FULL_Pos  9UL
#define BLE_RCB_INTR_MASK_TX_FIFO_NOT_FULL_Msk  0x200UL
#define BLE_RCB_INTR_MASK_TX_FIFO_EMPTY_Pos     10UL
#define BLE_RCB_INTR_MASK_TX_FIFO_EMPTY_Msk     0x400UL
#define BLE_RCB_INTR_MASK_TX_FIFO_OVERFLOW_Pos  11UL
#define BLE_RCB_INTR_MASK_TX_FIFO_OVERFLOW_Msk  0x800UL
#define BLE_RCB_INTR_MASK_TX_FIFO_UNDERFLOW_Pos 12UL
#define BLE_RCB_INTR_MASK_TX_FIFO_UNDERFLOW_Msk 0x1000UL
#define BLE_RCB_INTR_MASK_RX_FIFO_TRIGGER_Pos   16UL
#define BLE_RCB_INTR_MASK_RX_FIFO_TRIGGER_Msk   0x10000UL
#define BLE_RCB_INTR_MASK_RX_FIFO_NOT_EMPTY_Pos 17UL
#define BLE_RCB_INTR_MASK_RX_FIFO_NOT_EMPTY_Msk 0x20000UL
#define BLE_RCB_INTR_MASK_RX_FIFO_FULL_Pos      18UL
#define BLE_RCB_INTR_MASK_RX_FIFO_FULL_Msk      0x40000UL
#define BLE_RCB_INTR_MASK_RX_FIFO_OVERFLOW_Pos  19UL
#define BLE_RCB_INTR_MASK_RX_FIFO_OVERFLOW_Msk  0x80000UL
#define BLE_RCB_INTR_MASK_RX_FIFO_UNDERFLOW_Pos 20UL
#define BLE_RCB_INTR_MASK_RX_FIFO_UNDERFLOW_Msk 0x100000UL
/* BLE_RCB.INTR_MASKED */
#define BLE_RCB_INTR_MASKED_RCB_DONE_Pos        0UL
#define BLE_RCB_INTR_MASKED_RCB_DONE_Msk        0x1UL
#define BLE_RCB_INTR_MASKED_TX_FIFO_TRIGGER_Pos 8UL
#define BLE_RCB_INTR_MASKED_TX_FIFO_TRIGGER_Msk 0x100UL
#define BLE_RCB_INTR_MASKED_TX_FIFO_NOT_FULL_Pos 9UL
#define BLE_RCB_INTR_MASKED_TX_FIFO_NOT_FULL_Msk 0x200UL
#define BLE_RCB_INTR_MASKED_TX_FIFO_EMPTY_Pos   10UL
#define BLE_RCB_INTR_MASKED_TX_FIFO_EMPTY_Msk   0x400UL
#define BLE_RCB_INTR_MASKED_TX_FIFO_OVERFLOW_Pos 11UL
#define BLE_RCB_INTR_MASKED_TX_FIFO_OVERFLOW_Msk 0x800UL
#define BLE_RCB_INTR_MASKED_TX_FIFO_UNDERFLOW_Pos 12UL
#define BLE_RCB_INTR_MASKED_TX_FIFO_UNDERFLOW_Msk 0x1000UL
#define BLE_RCB_INTR_MASKED_RX_FIFO_TRIGGER_Pos 16UL
#define BLE_RCB_INTR_MASKED_RX_FIFO_TRIGGER_Msk 0x10000UL
#define BLE_RCB_INTR_MASKED_RX_FIFO_NOT_EMPTY_Pos 17UL
#define BLE_RCB_INTR_MASKED_RX_FIFO_NOT_EMPTY_Msk 0x20000UL
#define BLE_RCB_INTR_MASKED_RX_FIFO_FULL_Pos    18UL
#define BLE_RCB_INTR_MASKED_RX_FIFO_FULL_Msk    0x40000UL
#define BLE_RCB_INTR_MASKED_RX_FIFO_OVERFLOW_Pos 19UL
#define BLE_RCB_INTR_MASKED_RX_FIFO_OVERFLOW_Msk 0x80000UL
#define BLE_RCB_INTR_MASKED_RX_FIFO_UNDERFLOW_Pos 20UL
#define BLE_RCB_INTR_MASKED_RX_FIFO_UNDERFLOW_Msk 0x100000UL


/* BLE_BLELL.COMMAND_REGISTER */
#define BLE_BLELL_COMMAND_REGISTER_COMMAND_Pos  0UL
#define BLE_BLELL_COMMAND_REGISTER_COMMAND_Msk  0xFFUL
/* BLE_BLELL.EVENT_INTR */
#define BLE_BLELL_EVENT_INTR_ADV_INTR_Pos       0UL
#define BLE_BLELL_EVENT_INTR_ADV_INTR_Msk       0x1UL
#define BLE_BLELL_EVENT_INTR_SCAN_INTR_Pos      1UL
#define BLE_BLELL_EVENT_INTR_SCAN_INTR_Msk      0x2UL
#define BLE_BLELL_EVENT_INTR_INIT_INTR_Pos      2UL
#define BLE_BLELL_EVENT_INTR_INIT_INTR_Msk      0x4UL
#define BLE_BLELL_EVENT_INTR_CONN_INTR_Pos      3UL
#define BLE_BLELL_EVENT_INTR_CONN_INTR_Msk      0x8UL
#define BLE_BLELL_EVENT_INTR_SM_INTR_Pos        4UL
#define BLE_BLELL_EVENT_INTR_SM_INTR_Msk        0x10UL
#define BLE_BLELL_EVENT_INTR_DSM_INTR_Pos       5UL
#define BLE_BLELL_EVENT_INTR_DSM_INTR_Msk       0x20UL
#define BLE_BLELL_EVENT_INTR_ENC_INTR_Pos       6UL
#define BLE_BLELL_EVENT_INTR_ENC_INTR_Msk       0x40UL
#define BLE_BLELL_EVENT_INTR_RSSI_RX_DONE_INTR_Pos 7UL
#define BLE_BLELL_EVENT_INTR_RSSI_RX_DONE_INTR_Msk 0x80UL
/* BLE_BLELL.EVENT_ENABLE */
#define BLE_BLELL_EVENT_ENABLE_ADV_INT_EN_Pos   0UL
#define BLE_BLELL_EVENT_ENABLE_ADV_INT_EN_Msk   0x1UL
#define BLE_BLELL_EVENT_ENABLE_SCN_INT_EN_Pos   1UL
#define BLE_BLELL_EVENT_ENABLE_SCN_INT_EN_Msk   0x2UL
#define BLE_BLELL_EVENT_ENABLE_INIT_INT_EN_Pos  2UL
#define BLE_BLELL_EVENT_ENABLE_INIT_INT_EN_Msk  0x4UL
#define BLE_BLELL_EVENT_ENABLE_CONN_INT_EN_Pos  3UL
#define BLE_BLELL_EVENT_ENABLE_CONN_INT_EN_Msk  0x8UL
#define BLE_BLELL_EVENT_ENABLE_SM_INT_EN_Pos    4UL
#define BLE_BLELL_EVENT_ENABLE_SM_INT_EN_Msk    0x10UL
#define BLE_BLELL_EVENT_ENABLE_DSM_INT_EN_Pos   5UL
#define BLE_BLELL_EVENT_ENABLE_DSM_INT_EN_Msk   0x20UL
#define BLE_BLELL_EVENT_ENABLE_ENC_INT_EN_Pos   6UL
#define BLE_BLELL_EVENT_ENABLE_ENC_INT_EN_Msk   0x40UL
#define BLE_BLELL_EVENT_ENABLE_RSSI_RX_DONE_INT_EN_Pos 7UL
#define BLE_BLELL_EVENT_ENABLE_RSSI_RX_DONE_INT_EN_Msk 0x80UL
/* BLE_BLELL.ADV_PARAMS */
#define BLE_BLELL_ADV_PARAMS_TX_ADDR_Pos        0UL
#define BLE_BLELL_ADV_PARAMS_TX_ADDR_Msk        0x1UL
#define BLE_BLELL_ADV_PARAMS_ADV_TYPE_Pos       1UL
#define BLE_BLELL_ADV_PARAMS_ADV_TYPE_Msk       0x6UL
#define BLE_BLELL_ADV_PARAMS_ADV_FILT_POLICY_Pos 3UL
#define BLE_BLELL_ADV_PARAMS_ADV_FILT_POLICY_Msk 0x18UL
#define BLE_BLELL_ADV_PARAMS_ADV_CHANNEL_MAP_Pos 5UL
#define BLE_BLELL_ADV_PARAMS_ADV_CHANNEL_MAP_Msk 0xE0UL
#define BLE_BLELL_ADV_PARAMS_RX_ADDR_Pos        8UL
#define BLE_BLELL_ADV_PARAMS_RX_ADDR_Msk        0x100UL
#define BLE_BLELL_ADV_PARAMS_RX_SEC_ADDR_Pos    9UL
#define BLE_BLELL_ADV_PARAMS_RX_SEC_ADDR_Msk    0x200UL
#define BLE_BLELL_ADV_PARAMS_ADV_LOW_DUTY_CYCLE_Pos 10UL
#define BLE_BLELL_ADV_PARAMS_ADV_LOW_DUTY_CYCLE_Msk 0x400UL
#define BLE_BLELL_ADV_PARAMS_INITA_RPA_CHECK_Pos 11UL
#define BLE_BLELL_ADV_PARAMS_INITA_RPA_CHECK_Msk 0x800UL
#define BLE_BLELL_ADV_PARAMS_TX_ADDR_PRIV_Pos   12UL
#define BLE_BLELL_ADV_PARAMS_TX_ADDR_PRIV_Msk   0x1000UL
#define BLE_BLELL_ADV_PARAMS_ADV_RCV_IA_IN_PRIV_Pos 13UL
#define BLE_BLELL_ADV_PARAMS_ADV_RCV_IA_IN_PRIV_Msk 0x2000UL
#define BLE_BLELL_ADV_PARAMS_ADV_RPT_PEER_NRPA_ADDR_IN_PRIV_Pos 14UL
#define BLE_BLELL_ADV_PARAMS_ADV_RPT_PEER_NRPA_ADDR_IN_PRIV_Msk 0x4000UL
#define BLE_BLELL_ADV_PARAMS_RCV_TX_ADDR_Pos    15UL
#define BLE_BLELL_ADV_PARAMS_RCV_TX_ADDR_Msk    0x8000UL
/* BLE_BLELL.ADV_INTERVAL_TIMEOUT */
#define BLE_BLELL_ADV_INTERVAL_TIMEOUT_ADV_INTERVAL_Pos 0UL
#define BLE_BLELL_ADV_INTERVAL_TIMEOUT_ADV_INTERVAL_Msk 0x7FFFUL
/* BLE_BLELL.ADV_INTR */
#define BLE_BLELL_ADV_INTR_ADV_STRT_INTR_Pos    0UL
#define BLE_BLELL_ADV_INTR_ADV_STRT_INTR_Msk    0x1UL
#define BLE_BLELL_ADV_INTR_ADV_CLOSE_INTR_Pos   1UL
#define BLE_BLELL_ADV_INTR_ADV_CLOSE_INTR_Msk   0x2UL
#define BLE_BLELL_ADV_INTR_ADV_TX_INTR_Pos      2UL
#define BLE_BLELL_ADV_INTR_ADV_TX_INTR_Msk      0x4UL
#define BLE_BLELL_ADV_INTR_SCAN_RSP_TX_INTR_Pos 3UL
#define BLE_BLELL_ADV_INTR_SCAN_RSP_TX_INTR_Msk 0x8UL
#define BLE_BLELL_ADV_INTR_SCAN_REQ_RX_INTR_Pos 4UL
#define BLE_BLELL_ADV_INTR_SCAN_REQ_RX_INTR_Msk 0x10UL
#define BLE_BLELL_ADV_INTR_CONN_REQ_RX_INTR_Pos 5UL
#define BLE_BLELL_ADV_INTR_CONN_REQ_RX_INTR_Msk 0x20UL
#define BLE_BLELL_ADV_INTR_SLV_CONNECTED_Pos    6UL
#define BLE_BLELL_ADV_INTR_SLV_CONNECTED_Msk    0x40UL
#define BLE_BLELL_ADV_INTR_ADV_TIMEOUT_Pos      7UL
#define BLE_BLELL_ADV_INTR_ADV_TIMEOUT_Msk      0x80UL
#define BLE_BLELL_ADV_INTR_ADV_ON_Pos           8UL
#define BLE_BLELL_ADV_INTR_ADV_ON_Msk           0x100UL
#define BLE_BLELL_ADV_INTR_SLV_CONN_PEER_RPA_UNMCH_INTR_Pos 9UL
#define BLE_BLELL_ADV_INTR_SLV_CONN_PEER_RPA_UNMCH_INTR_Msk 0x200UL
#define BLE_BLELL_ADV_INTR_SCAN_REQ_RX_PEER_RPA_UNMCH_INTR_Pos 10UL
#define BLE_BLELL_ADV_INTR_SCAN_REQ_RX_PEER_RPA_UNMCH_INTR_Msk 0x400UL
#define BLE_BLELL_ADV_INTR_INIT_ADDR_MATCH_PRIV_MISMATCH_INTR_Pos 11UL
#define BLE_BLELL_ADV_INTR_INIT_ADDR_MATCH_PRIV_MISMATCH_INTR_Msk 0x800UL
#define BLE_BLELL_ADV_INTR_SCAN_ADDR_MATCH_PRIV_MISMATCH_INTR_Pos 12UL
#define BLE_BLELL_ADV_INTR_SCAN_ADDR_MATCH_PRIV_MISMATCH_INTR_Msk 0x1000UL
/* BLE_BLELL.ADV_NEXT_INSTANT */
#define BLE_BLELL_ADV_NEXT_INSTANT_ADV_NEXT_INSTANT_Pos 0UL
#define BLE_BLELL_ADV_NEXT_INSTANT_ADV_NEXT_INSTANT_Msk 0xFFFFUL
/* BLE_BLELL.SCAN_INTERVAL */
#define BLE_BLELL_SCAN_INTERVAL_SCAN_INTERVAL_Pos 0UL
#define BLE_BLELL_SCAN_INTERVAL_SCAN_INTERVAL_Msk 0xFFFFUL
/* BLE_BLELL.SCAN_WINDOW */
#define BLE_BLELL_SCAN_WINDOW_SCAN_WINDOW_Pos   0UL
#define BLE_BLELL_SCAN_WINDOW_SCAN_WINDOW_Msk   0xFFFFUL
/* BLE_BLELL.SCAN_PARAM */
#define BLE_BLELL_SCAN_PARAM_TX_ADDR_Pos        0UL
#define BLE_BLELL_SCAN_PARAM_TX_ADDR_Msk        0x1UL
#define BLE_BLELL_SCAN_PARAM_SCAN_TYPE_Pos      1UL
#define BLE_BLELL_SCAN_PARAM_SCAN_TYPE_Msk      0x6UL
#define BLE_BLELL_SCAN_PARAM_SCAN_FILT_POLICY_Pos 3UL
#define BLE_BLELL_SCAN_PARAM_SCAN_FILT_POLICY_Msk 0x18UL
#define BLE_BLELL_SCAN_PARAM_DUP_FILT_EN_Pos    5UL
#define BLE_BLELL_SCAN_PARAM_DUP_FILT_EN_Msk    0x20UL
#define BLE_BLELL_SCAN_PARAM_DUP_FILT_CHK_ADV_DIR_Pos 6UL
#define BLE_BLELL_SCAN_PARAM_DUP_FILT_CHK_ADV_DIR_Msk 0x40UL
#define BLE_BLELL_SCAN_PARAM_SCAN_RSP_ADVA_CHECK_Pos 7UL
#define BLE_BLELL_SCAN_PARAM_SCAN_RSP_ADVA_CHECK_Msk 0x80UL
#define BLE_BLELL_SCAN_PARAM_SCAN_RCV_IA_IN_PRIV_Pos 8UL
#define BLE_BLELL_SCAN_PARAM_SCAN_RCV_IA_IN_PRIV_Msk 0x100UL
#define BLE_BLELL_SCAN_PARAM_SCAN_RPT_PEER_NRPA_ADDR_IN_PRIV_Pos 9UL
#define BLE_BLELL_SCAN_PARAM_SCAN_RPT_PEER_NRPA_ADDR_IN_PRIV_Msk 0x200UL
/* BLE_BLELL.SCAN_INTR */
#define BLE_BLELL_SCAN_INTR_SCAN_STRT_INTR_Pos  0UL
#define BLE_BLELL_SCAN_INTR_SCAN_STRT_INTR_Msk  0x1UL
#define BLE_BLELL_SCAN_INTR_SCAN_CLOSE_INTR_Pos 1UL
#define BLE_BLELL_SCAN_INTR_SCAN_CLOSE_INTR_Msk 0x2UL
#define BLE_BLELL_SCAN_INTR_SCAN_TX_INTR_Pos    2UL
#define BLE_BLELL_SCAN_INTR_SCAN_TX_INTR_Msk    0x4UL
#define BLE_BLELL_SCAN_INTR_ADV_RX_INTR_Pos     3UL
#define BLE_BLELL_SCAN_INTR_ADV_RX_INTR_Msk     0x8UL
#define BLE_BLELL_SCAN_INTR_SCAN_RSP_RX_INTR_Pos 4UL
#define BLE_BLELL_SCAN_INTR_SCAN_RSP_RX_INTR_Msk 0x10UL
#define BLE_BLELL_SCAN_INTR_ADV_RX_PEER_RPA_UNMCH_INTR_Pos 5UL
#define BLE_BLELL_SCAN_INTR_ADV_RX_PEER_RPA_UNMCH_INTR_Msk 0x20UL
#define BLE_BLELL_SCAN_INTR_ADV_RX_SELF_RPA_UNMCH_INTR_Pos 6UL
#define BLE_BLELL_SCAN_INTR_ADV_RX_SELF_RPA_UNMCH_INTR_Msk 0x40UL
#define BLE_BLELL_SCAN_INTR_SCANA_TX_ADDR_NOT_SET_INTR_Pos 7UL
#define BLE_BLELL_SCAN_INTR_SCANA_TX_ADDR_NOT_SET_INTR_Msk 0x80UL
#define BLE_BLELL_SCAN_INTR_SCAN_ON_Pos         8UL
#define BLE_BLELL_SCAN_INTR_SCAN_ON_Msk         0x100UL
#define BLE_BLELL_SCAN_INTR_PEER_ADDR_MATCH_PRIV_MISMATCH_INTR_Pos 9UL
#define BLE_BLELL_SCAN_INTR_PEER_ADDR_MATCH_PRIV_MISMATCH_INTR_Msk 0x200UL
#define BLE_BLELL_SCAN_INTR_SELF_ADDR_MATCH_PRIV_MISMATCH_INTR_Pos 10UL
#define BLE_BLELL_SCAN_INTR_SELF_ADDR_MATCH_PRIV_MISMATCH_INTR_Msk 0x400UL
/* BLE_BLELL.SCAN_NEXT_INSTANT */
#define BLE_BLELL_SCAN_NEXT_INSTANT_NEXT_SCAN_INSTANT_Pos 0UL
#define BLE_BLELL_SCAN_NEXT_INSTANT_NEXT_SCAN_INSTANT_Msk 0xFFFFUL
/* BLE_BLELL.INIT_INTERVAL */
#define BLE_BLELL_INIT_INTERVAL_INIT_SCAN_INTERVAL_Pos 0UL
#define BLE_BLELL_INIT_INTERVAL_INIT_SCAN_INTERVAL_Msk 0xFFFFUL
/* BLE_BLELL.INIT_WINDOW */
#define BLE_BLELL_INIT_WINDOW_INIT_SCAN_WINDOW_Pos 0UL
#define BLE_BLELL_INIT_WINDOW_INIT_SCAN_WINDOW_Msk 0xFFFFUL
/* BLE_BLELL.INIT_PARAM */
#define BLE_BLELL_INIT_PARAM_TX_ADDR_Pos        0UL
#define BLE_BLELL_INIT_PARAM_TX_ADDR_Msk        0x1UL
#define BLE_BLELL_INIT_PARAM_RX_ADDR__RX_TX_ADDR_Pos 1UL
#define BLE_BLELL_INIT_PARAM_RX_ADDR__RX_TX_ADDR_Msk 0x2UL
#define BLE_BLELL_INIT_PARAM_INIT_FILT_POLICY_Pos 3UL
#define BLE_BLELL_INIT_PARAM_INIT_FILT_POLICY_Msk 0x8UL
#define BLE_BLELL_INIT_PARAM_INIT_RCV_IA_IN_PRIV_Pos 4UL
#define BLE_BLELL_INIT_PARAM_INIT_RCV_IA_IN_PRIV_Msk 0x10UL
/* BLE_BLELL.INIT_INTR */
#define BLE_BLELL_INIT_INTR_INIT_INTERVAL_EXPIRE_INTR_Pos 0UL
#define BLE_BLELL_INIT_INTR_INIT_INTERVAL_EXPIRE_INTR_Msk 0x1UL
#define BLE_BLELL_INIT_INTR_INIT_CLOSE_WINDOW_INR_Pos 1UL
#define BLE_BLELL_INIT_INTR_INIT_CLOSE_WINDOW_INR_Msk 0x2UL
#define BLE_BLELL_INIT_INTR_INIT_TX_START_INTR_Pos 2UL
#define BLE_BLELL_INIT_INTR_INIT_TX_START_INTR_Msk 0x4UL
#define BLE_BLELL_INIT_INTR_MASTER_CONN_CREATED_Pos 4UL
#define BLE_BLELL_INIT_INTR_MASTER_CONN_CREATED_Msk 0x10UL
#define BLE_BLELL_INIT_INTR_ADV_RX_SELF_ADDR_UNMCH_INTR_Pos 5UL
#define BLE_BLELL_INIT_INTR_ADV_RX_SELF_ADDR_UNMCH_INTR_Msk 0x20UL
#define BLE_BLELL_INIT_INTR_ADV_RX_PEER_ADDR_UNMCH_INTR_Pos 6UL
#define BLE_BLELL_INIT_INTR_ADV_RX_PEER_ADDR_UNMCH_INTR_Msk 0x40UL
#define BLE_BLELL_INIT_INTR_INITA_TX_ADDR_NOT_SET_INTR_Pos 7UL
#define BLE_BLELL_INIT_INTR_INITA_TX_ADDR_NOT_SET_INTR_Msk 0x80UL
#define BLE_BLELL_INIT_INTR_INI_PEER_ADDR_MATCH_PRIV_MISMATCH_INTR_Pos 8UL
#define BLE_BLELL_INIT_INTR_INI_PEER_ADDR_MATCH_PRIV_MISMATCH_INTR_Msk 0x100UL
#define BLE_BLELL_INIT_INTR_INI_SELF_ADDR_MATCH_PRIV_MISMATCH_INTR_Pos 9UL
#define BLE_BLELL_INIT_INTR_INI_SELF_ADDR_MATCH_PRIV_MISMATCH_INTR_Msk 0x200UL
/* BLE_BLELL.INIT_NEXT_INSTANT */
#define BLE_BLELL_INIT_NEXT_INSTANT_INIT_NEXT_INSTANT_Pos 0UL
#define BLE_BLELL_INIT_NEXT_INSTANT_INIT_NEXT_INSTANT_Msk 0xFFFFUL
/* BLE_BLELL.DEVICE_RAND_ADDR_L */
#define BLE_BLELL_DEVICE_RAND_ADDR_L_DEVICE_RAND_ADDR_L_Pos 0UL
#define BLE_BLELL_DEVICE_RAND_ADDR_L_DEVICE_RAND_ADDR_L_Msk 0xFFFFUL
/* BLE_BLELL.DEVICE_RAND_ADDR_M */
#define BLE_BLELL_DEVICE_RAND_ADDR_M_DEVICE_RAND_ADDR_M_Pos 0UL
#define BLE_BLELL_DEVICE_RAND_ADDR_M_DEVICE_RAND_ADDR_M_Msk 0xFFFFUL
/* BLE_BLELL.DEVICE_RAND_ADDR_H */
#define BLE_BLELL_DEVICE_RAND_ADDR_H_DEVICE_RAND_ADDR_H_Pos 0UL
#define BLE_BLELL_DEVICE_RAND_ADDR_H_DEVICE_RAND_ADDR_H_Msk 0xFFFFUL
/* BLE_BLELL.PEER_ADDR_L */
#define BLE_BLELL_PEER_ADDR_L_PEER_ADDR_L_Pos   0UL
#define BLE_BLELL_PEER_ADDR_L_PEER_ADDR_L_Msk   0xFFFFUL
/* BLE_BLELL.PEER_ADDR_M */
#define BLE_BLELL_PEER_ADDR_M_PEER_ADDR_M_Pos   0UL
#define BLE_BLELL_PEER_ADDR_M_PEER_ADDR_M_Msk   0xFFFFUL
/* BLE_BLELL.PEER_ADDR_H */
#define BLE_BLELL_PEER_ADDR_H_PEER_ADDR_H_Pos   0UL
#define BLE_BLELL_PEER_ADDR_H_PEER_ADDR_H_Msk   0xFFFFUL
/* BLE_BLELL.WL_ADDR_TYPE */
#define BLE_BLELL_WL_ADDR_TYPE_WL_ADDR_TYPE_Pos 0UL
#define BLE_BLELL_WL_ADDR_TYPE_WL_ADDR_TYPE_Msk 0xFFFFUL
/* BLE_BLELL.WL_ENABLE */
#define BLE_BLELL_WL_ENABLE_WL_ENABLE_Pos       0UL
#define BLE_BLELL_WL_ENABLE_WL_ENABLE_Msk       0xFFFFUL
/* BLE_BLELL.TRANSMIT_WINDOW_OFFSET */
#define BLE_BLELL_TRANSMIT_WINDOW_OFFSET_TX_WINDOW_OFFSET_Pos 0UL
#define BLE_BLELL_TRANSMIT_WINDOW_OFFSET_TX_WINDOW_OFFSET_Msk 0xFFFFUL
/* BLE_BLELL.TRANSMIT_WINDOW_SIZE */
#define BLE_BLELL_TRANSMIT_WINDOW_SIZE_TX_WINDOW_SIZE_Pos 0UL
#define BLE_BLELL_TRANSMIT_WINDOW_SIZE_TX_WINDOW_SIZE_Msk 0xFFUL
/* BLE_BLELL.DATA_CHANNELS_L0 */
#define BLE_BLELL_DATA_CHANNELS_L0_DATA_CHANNELS_L0_Pos 0UL
#define BLE_BLELL_DATA_CHANNELS_L0_DATA_CHANNELS_L0_Msk 0xFFFFUL
/* BLE_BLELL.DATA_CHANNELS_M0 */
#define BLE_BLELL_DATA_CHANNELS_M0_DATA_CHANNELS_M0_Pos 0UL
#define BLE_BLELL_DATA_CHANNELS_M0_DATA_CHANNELS_M0_Msk 0xFFFFUL
/* BLE_BLELL.DATA_CHANNELS_H0 */
#define BLE_BLELL_DATA_CHANNELS_H0_DATA_CHANNELS_H0_Pos 0UL
#define BLE_BLELL_DATA_CHANNELS_H0_DATA_CHANNELS_H0_Msk 0x1FUL
/* BLE_BLELL.DATA_CHANNELS_L1 */
#define BLE_BLELL_DATA_CHANNELS_L1_DATA_CHANNELS_L1_Pos 0UL
#define BLE_BLELL_DATA_CHANNELS_L1_DATA_CHANNELS_L1_Msk 0xFFFFUL
/* BLE_BLELL.DATA_CHANNELS_M1 */
#define BLE_BLELL_DATA_CHANNELS_M1_DATA_CHANNELS_M1_Pos 0UL
#define BLE_BLELL_DATA_CHANNELS_M1_DATA_CHANNELS_M1_Msk 0xFFFFUL
/* BLE_BLELL.DATA_CHANNELS_H1 */
#define BLE_BLELL_DATA_CHANNELS_H1_DATA_CHANNELS_H1_Pos 0UL
#define BLE_BLELL_DATA_CHANNELS_H1_DATA_CHANNELS_H1_Msk 0x1FUL
/* BLE_BLELL.CONN_INTR */
#define BLE_BLELL_CONN_INTR_CONN_CLOSED_Pos     0UL
#define BLE_BLELL_CONN_INTR_CONN_CLOSED_Msk     0x1UL
#define BLE_BLELL_CONN_INTR_CONN_ESTB_Pos       1UL
#define BLE_BLELL_CONN_INTR_CONN_ESTB_Msk       0x2UL
#define BLE_BLELL_CONN_INTR_MAP_UPDT_DONE_Pos   2UL
#define BLE_BLELL_CONN_INTR_MAP_UPDT_DONE_Msk   0x4UL
#define BLE_BLELL_CONN_INTR_START_CE_Pos        3UL
#define BLE_BLELL_CONN_INTR_START_CE_Msk        0x8UL
#define BLE_BLELL_CONN_INTR_CLOSE_CE_Pos        4UL
#define BLE_BLELL_CONN_INTR_CLOSE_CE_Msk        0x10UL
#define BLE_BLELL_CONN_INTR_CE_TX_ACK_Pos       5UL
#define BLE_BLELL_CONN_INTR_CE_TX_ACK_Msk       0x20UL
#define BLE_BLELL_CONN_INTR_CE_RX_Pos           6UL
#define BLE_BLELL_CONN_INTR_CE_RX_Msk           0x40UL
#define BLE_BLELL_CONN_INTR_CON_UPDT_DONE_Pos   7UL
#define BLE_BLELL_CONN_INTR_CON_UPDT_DONE_Msk   0x80UL
#define BLE_BLELL_CONN_INTR_DISCON_STATUS_Pos   8UL
#define BLE_BLELL_CONN_INTR_DISCON_STATUS_Msk   0x700UL
#define BLE_BLELL_CONN_INTR_RX_PDU_STATUS_Pos   11UL
#define BLE_BLELL_CONN_INTR_RX_PDU_STATUS_Msk   0x3800UL
#define BLE_BLELL_CONN_INTR_PING_TIMER_EXPIRD_INTR_Pos 14UL
#define BLE_BLELL_CONN_INTR_PING_TIMER_EXPIRD_INTR_Msk 0x4000UL
#define BLE_BLELL_CONN_INTR_PING_NEARLY_EXPIRD_INTR_Pos 15UL
#define BLE_BLELL_CONN_INTR_PING_NEARLY_EXPIRD_INTR_Msk 0x8000UL
/* BLE_BLELL.CONN_STATUS */
#define BLE_BLELL_CONN_STATUS_RECEIVE_PACKET_COUNT_Pos 12UL
#define BLE_BLELL_CONN_STATUS_RECEIVE_PACKET_COUNT_Msk 0xF000UL
/* BLE_BLELL.CONN_INDEX */
#define BLE_BLELL_CONN_INDEX_CONN_INDEX_Pos     0UL
#define BLE_BLELL_CONN_INDEX_CONN_INDEX_Msk     0xFFFFUL
/* BLE_BLELL.WAKEUP_CONFIG */
#define BLE_BLELL_WAKEUP_CONFIG_OSC_STARTUP_DELAY_Pos 0UL
#define BLE_BLELL_WAKEUP_CONFIG_OSC_STARTUP_DELAY_Msk 0xFFUL
#define BLE_BLELL_WAKEUP_CONFIG_DSM_OFFSET_TO_WAKEUP_INSTANT_Pos 10UL
#define BLE_BLELL_WAKEUP_CONFIG_DSM_OFFSET_TO_WAKEUP_INSTANT_Msk 0xFC00UL
/* BLE_BLELL.WAKEUP_CONTROL */
#define BLE_BLELL_WAKEUP_CONTROL_WAKEUP_INSTANT_Pos 0UL
#define BLE_BLELL_WAKEUP_CONTROL_WAKEUP_INSTANT_Msk 0xFFFFUL
/* BLE_BLELL.CLOCK_CONFIG */
#define BLE_BLELL_CLOCK_CONFIG_ADV_CLK_GATE_EN_Pos 0UL
#define BLE_BLELL_CLOCK_CONFIG_ADV_CLK_GATE_EN_Msk 0x1UL
#define BLE_BLELL_CLOCK_CONFIG_SCAN_CLK_GATE_EN_Pos 1UL
#define BLE_BLELL_CLOCK_CONFIG_SCAN_CLK_GATE_EN_Msk 0x2UL
#define BLE_BLELL_CLOCK_CONFIG_INIT_CLK_GATE_EN_Pos 2UL
#define BLE_BLELL_CLOCK_CONFIG_INIT_CLK_GATE_EN_Msk 0x4UL
#define BLE_BLELL_CLOCK_CONFIG_CONN_CLK_GATE_EN_Pos 3UL
#define BLE_BLELL_CLOCK_CONFIG_CONN_CLK_GATE_EN_Msk 0x8UL
#define BLE_BLELL_CLOCK_CONFIG_CORECLK_GATE_EN_Pos 4UL
#define BLE_BLELL_CLOCK_CONFIG_CORECLK_GATE_EN_Msk 0x10UL
#define BLE_BLELL_CLOCK_CONFIG_SYSCLK_GATE_EN_Pos 5UL
#define BLE_BLELL_CLOCK_CONFIG_SYSCLK_GATE_EN_Msk 0x20UL
#define BLE_BLELL_CLOCK_CONFIG_PHY_CLK_GATE_EN_Pos 6UL
#define BLE_BLELL_CLOCK_CONFIG_PHY_CLK_GATE_EN_Msk 0x40UL
#define BLE_BLELL_CLOCK_CONFIG_LLH_IDLE_Pos     7UL
#define BLE_BLELL_CLOCK_CONFIG_LLH_IDLE_Msk     0x80UL
#define BLE_BLELL_CLOCK_CONFIG_LPO_CLK_FREQ_SEL_Pos 8UL
#define BLE_BLELL_CLOCK_CONFIG_LPO_CLK_FREQ_SEL_Msk 0x100UL
#define BLE_BLELL_CLOCK_CONFIG_LPO_SEL_EXTERNAL_Pos 9UL
#define BLE_BLELL_CLOCK_CONFIG_LPO_SEL_EXTERNAL_Msk 0x200UL
#define BLE_BLELL_CLOCK_CONFIG_SM_AUTO_WKUP_EN_Pos 10UL
#define BLE_BLELL_CLOCK_CONFIG_SM_AUTO_WKUP_EN_Msk 0x400UL
#define BLE_BLELL_CLOCK_CONFIG_SM_INTR_EN_Pos   12UL
#define BLE_BLELL_CLOCK_CONFIG_SM_INTR_EN_Msk   0x1000UL
#define BLE_BLELL_CLOCK_CONFIG_DEEP_SLEEP_AUTO_WKUP_DISABLE_Pos 13UL
#define BLE_BLELL_CLOCK_CONFIG_DEEP_SLEEP_AUTO_WKUP_DISABLE_Msk 0x2000UL
#define BLE_BLELL_CLOCK_CONFIG_SLEEP_MODE_EN_Pos 14UL
#define BLE_BLELL_CLOCK_CONFIG_SLEEP_MODE_EN_Msk 0x4000UL
#define BLE_BLELL_CLOCK_CONFIG_DEEP_SLEEP_MODE_EN_Pos 15UL
#define BLE_BLELL_CLOCK_CONFIG_DEEP_SLEEP_MODE_EN_Msk 0x8000UL
/* BLE_BLELL.TIM_COUNTER_L */
#define BLE_BLELL_TIM_COUNTER_L_TIM_REF_CLOCK_Pos 0UL
#define BLE_BLELL_TIM_COUNTER_L_TIM_REF_CLOCK_Msk 0xFFFFUL
/* BLE_BLELL.WAKEUP_CONFIG_EXTD */
#define BLE_BLELL_WAKEUP_CONFIG_EXTD_DSM_LF_OFFSET_Pos 0UL
#define BLE_BLELL_WAKEUP_CONFIG_EXTD_DSM_LF_OFFSET_Msk 0x1FUL
/* BLE_BLELL.POC_REG__TIM_CONTROL */
#define BLE_BLELL_POC_REG__TIM_CONTROL_BB_CLK_FREQ_MINUS_1_Pos 3UL
#define BLE_BLELL_POC_REG__TIM_CONTROL_BB_CLK_FREQ_MINUS_1_Msk 0xF8UL
#define BLE_BLELL_POC_REG__TIM_CONTROL_START_SLOT_OFFSET_Pos 8UL
#define BLE_BLELL_POC_REG__TIM_CONTROL_START_SLOT_OFFSET_Msk 0xF00UL
/* BLE_BLELL.ADV_TX_DATA_FIFO */
#define BLE_BLELL_ADV_TX_DATA_FIFO_ADV_TX_DATA_Pos 0UL
#define BLE_BLELL_ADV_TX_DATA_FIFO_ADV_TX_DATA_Msk 0xFFFFUL
/* BLE_BLELL.ADV_SCN_RSP_TX_FIFO */
#define BLE_BLELL_ADV_SCN_RSP_TX_FIFO_SCAN_RSP_DATA_Pos 0UL
#define BLE_BLELL_ADV_SCN_RSP_TX_FIFO_SCAN_RSP_DATA_Msk 0xFFFFUL
/* BLE_BLELL.INIT_SCN_ADV_RX_FIFO */
#define BLE_BLELL_INIT_SCN_ADV_RX_FIFO_ADV_SCAN_RSP_RX_DATA_Pos 0UL
#define BLE_BLELL_INIT_SCN_ADV_RX_FIFO_ADV_SCAN_RSP_RX_DATA_Msk 0xFFFFUL
/* BLE_BLELL.CONN_INTERVAL */
#define BLE_BLELL_CONN_INTERVAL_CONNECTION_INTERVAL_Pos 0UL
#define BLE_BLELL_CONN_INTERVAL_CONNECTION_INTERVAL_Msk 0xFFFFUL
/* BLE_BLELL.SUP_TIMEOUT */
#define BLE_BLELL_SUP_TIMEOUT_SUPERVISION_TIMEOUT_Pos 0UL
#define BLE_BLELL_SUP_TIMEOUT_SUPERVISION_TIMEOUT_Msk 0xFFFFUL
/* BLE_BLELL.SLAVE_LATENCY */
#define BLE_BLELL_SLAVE_LATENCY_SLAVE_LATENCY_Pos 0UL
#define BLE_BLELL_SLAVE_LATENCY_SLAVE_LATENCY_Msk 0xFFFFUL
/* BLE_BLELL.CE_LENGTH */
#define BLE_BLELL_CE_LENGTH_CONNECTION_EVENT_LENGTH_Pos 0UL
#define BLE_BLELL_CE_LENGTH_CONNECTION_EVENT_LENGTH_Msk 0xFFFFUL
/* BLE_BLELL.PDU_ACCESS_ADDR_L_REGISTER */
#define BLE_BLELL_PDU_ACCESS_ADDR_L_REGISTER_PDU_ACCESS_ADDRESS_LOWER_BITS_Pos 0UL
#define BLE_BLELL_PDU_ACCESS_ADDR_L_REGISTER_PDU_ACCESS_ADDRESS_LOWER_BITS_Msk 0xFFFFUL
/* BLE_BLELL.PDU_ACCESS_ADDR_H_REGISTER */
#define BLE_BLELL_PDU_ACCESS_ADDR_H_REGISTER_PDU_ACCESS_ADDRESS_HIGHER_BITS_Pos 0UL
#define BLE_BLELL_PDU_ACCESS_ADDR_H_REGISTER_PDU_ACCESS_ADDRESS_HIGHER_BITS_Msk 0xFFFFUL
/* BLE_BLELL.CONN_CE_INSTANT */
#define BLE_BLELL_CONN_CE_INSTANT_CE_INSTANT_Pos 0UL
#define BLE_BLELL_CONN_CE_INSTANT_CE_INSTANT_Msk 0xFFFFUL
/* BLE_BLELL.CE_CNFG_STS_REGISTER */
#define BLE_BLELL_CE_CNFG_STS_REGISTER_DATA_LIST_INDEX_LAST_ACK_INDEX_Pos 0UL
#define BLE_BLELL_CE_CNFG_STS_REGISTER_DATA_LIST_INDEX_LAST_ACK_INDEX_Msk 0xFUL
#define BLE_BLELL_CE_CNFG_STS_REGISTER_DATA_LIST_HEAD_UP_Pos 4UL
#define BLE_BLELL_CE_CNFG_STS_REGISTER_DATA_LIST_HEAD_UP_Msk 0x10UL
#define BLE_BLELL_CE_CNFG_STS_REGISTER_SPARE_Pos 5UL
#define BLE_BLELL_CE_CNFG_STS_REGISTER_SPARE_Msk 0x20UL
#define BLE_BLELL_CE_CNFG_STS_REGISTER_MD_Pos   6UL
#define BLE_BLELL_CE_CNFG_STS_REGISTER_MD_Msk   0x40UL
#define BLE_BLELL_CE_CNFG_STS_REGISTER_MAP_INDEX__CURR_INDEX_Pos 7UL
#define BLE_BLELL_CE_CNFG_STS_REGISTER_MAP_INDEX__CURR_INDEX_Msk 0x80UL
#define BLE_BLELL_CE_CNFG_STS_REGISTER_PAUSE_DATA_Pos 8UL
#define BLE_BLELL_CE_CNFG_STS_REGISTER_PAUSE_DATA_Msk 0x100UL
#define BLE_BLELL_CE_CNFG_STS_REGISTER_CONN_ACTIVE_Pos 10UL
#define BLE_BLELL_CE_CNFG_STS_REGISTER_CONN_ACTIVE_Msk 0x400UL
#define BLE_BLELL_CE_CNFG_STS_REGISTER_CURRENT_PDU_INDEX_Pos 12UL
#define BLE_BLELL_CE_CNFG_STS_REGISTER_CURRENT_PDU_INDEX_Msk 0xF000UL
/* BLE_BLELL.NEXT_CE_INSTANT */
#define BLE_BLELL_NEXT_CE_INSTANT_NEXT_CE_INSTANT_Pos 0UL
#define BLE_BLELL_NEXT_CE_INSTANT_NEXT_CE_INSTANT_Msk 0xFFFFUL
/* BLE_BLELL.CONN_CE_COUNTER */
#define BLE_BLELL_CONN_CE_COUNTER_CONNECTION_EVENT_COUNTER_Pos 0UL
#define BLE_BLELL_CONN_CE_COUNTER_CONNECTION_EVENT_COUNTER_Msk 0xFFFFUL
/* BLE_BLELL.DATA_LIST_SENT_UPDATE__STATUS */
#define BLE_BLELL_DATA_LIST_SENT_UPDATE__STATUS_LIST_INDEX__TX_SENT_3_0_Pos 0UL
#define BLE_BLELL_DATA_LIST_SENT_UPDATE__STATUS_LIST_INDEX__TX_SENT_3_0_Msk 0xFUL
#define BLE_BLELL_DATA_LIST_SENT_UPDATE__STATUS_SET_CLEAR_Pos 7UL
#define BLE_BLELL_DATA_LIST_SENT_UPDATE__STATUS_SET_CLEAR_Msk 0x80UL
/* BLE_BLELL.DATA_LIST_ACK_UPDATE__STATUS */
#define BLE_BLELL_DATA_LIST_ACK_UPDATE__STATUS_LIST_INDEX__TX_ACK_3_0_Pos 0UL
#define BLE_BLELL_DATA_LIST_ACK_UPDATE__STATUS_LIST_INDEX__TX_ACK_3_0_Msk 0xFUL
#define BLE_BLELL_DATA_LIST_ACK_UPDATE__STATUS_SET_CLEAR_Pos 7UL
#define BLE_BLELL_DATA_LIST_ACK_UPDATE__STATUS_SET_CLEAR_Msk 0x80UL
/* BLE_BLELL.CE_CNFG_STS_REGISTER_EXT */
#define BLE_BLELL_CE_CNFG_STS_REGISTER_EXT_TX_2M_Pos 0UL
#define BLE_BLELL_CE_CNFG_STS_REGISTER_EXT_TX_2M_Msk 0x1UL
#define BLE_BLELL_CE_CNFG_STS_REGISTER_EXT_RX_2M_Pos 1UL
#define BLE_BLELL_CE_CNFG_STS_REGISTER_EXT_RX_2M_Msk 0x2UL
#define BLE_BLELL_CE_CNFG_STS_REGISTER_EXT_SN_Pos 2UL
#define BLE_BLELL_CE_CNFG_STS_REGISTER_EXT_SN_Msk 0x4UL
#define BLE_BLELL_CE_CNFG_STS_REGISTER_EXT_NESN_Pos 3UL
#define BLE_BLELL_CE_CNFG_STS_REGISTER_EXT_NESN_Msk 0x8UL
#define BLE_BLELL_CE_CNFG_STS_REGISTER_EXT_LAST_UNMAPPED_CHANNEL_Pos 8UL
#define BLE_BLELL_CE_CNFG_STS_REGISTER_EXT_LAST_UNMAPPED_CHANNEL_Msk 0x3F00UL
/* BLE_BLELL.CONN_EXT_INTR */
#define BLE_BLELL_CONN_EXT_INTR_DATARATE_UPDATE_Pos 0UL
#define BLE_BLELL_CONN_EXT_INTR_DATARATE_UPDATE_Msk 0x1UL
#define BLE_BLELL_CONN_EXT_INTR_EARLY_INTR_Pos  1UL
#define BLE_BLELL_CONN_EXT_INTR_EARLY_INTR_Msk  0x2UL
#define BLE_BLELL_CONN_EXT_INTR_GEN_TIMER_INTR_Pos 2UL
#define BLE_BLELL_CONN_EXT_INTR_GEN_TIMER_INTR_Msk 0x4UL
/* BLE_BLELL.CONN_EXT_INTR_MASK */
#define BLE_BLELL_CONN_EXT_INTR_MASK_DATARATE_UPDATE_Pos 0UL
#define BLE_BLELL_CONN_EXT_INTR_MASK_DATARATE_UPDATE_Msk 0x1UL
#define BLE_BLELL_CONN_EXT_INTR_MASK_EARLY_INTR_Pos 1UL
#define BLE_BLELL_CONN_EXT_INTR_MASK_EARLY_INTR_Msk 0x2UL
#define BLE_BLELL_CONN_EXT_INTR_MASK_GEN_TIMER_INTR_Pos 2UL
#define BLE_BLELL_CONN_EXT_INTR_MASK_GEN_TIMER_INTR_Msk 0x4UL
/* BLE_BLELL.DATA_MEM_DESCRIPTOR */
#define BLE_BLELL_DATA_MEM_DESCRIPTOR_LLID_Pos  0UL
#define BLE_BLELL_DATA_MEM_DESCRIPTOR_LLID_Msk  0x3UL
#define BLE_BLELL_DATA_MEM_DESCRIPTOR_DATA_LENGTH_Pos 2UL
#define BLE_BLELL_DATA_MEM_DESCRIPTOR_DATA_LENGTH_Msk 0x3FCUL
/* BLE_BLELL.WINDOW_WIDEN_INTVL */
#define BLE_BLELL_WINDOW_WIDEN_INTVL_WINDOW_WIDEN_INTVL_Pos 0UL
#define BLE_BLELL_WINDOW_WIDEN_INTVL_WINDOW_WIDEN_INTVL_Msk 0xFFFUL
/* BLE_BLELL.WINDOW_WIDEN_WINOFF */
#define BLE_BLELL_WINDOW_WIDEN_WINOFF_WINDOW_WIDEN_WINOFF_Pos 0UL
#define BLE_BLELL_WINDOW_WIDEN_WINOFF_WINDOW_WIDEN_WINOFF_Msk 0xFFFUL
/* BLE_BLELL.LE_RF_TEST_MODE */
#define BLE_BLELL_LE_RF_TEST_MODE_TEST_FREQUENCY_Pos 0UL
#define BLE_BLELL_LE_RF_TEST_MODE_TEST_FREQUENCY_Msk 0x3FUL
#define BLE_BLELL_LE_RF_TEST_MODE_DTM_STATUS__DTM_CONT_RXEN_Pos 6UL
#define BLE_BLELL_LE_RF_TEST_MODE_DTM_STATUS__DTM_CONT_RXEN_Msk 0x40UL
#define BLE_BLELL_LE_RF_TEST_MODE_PKT_PAYLOAD_Pos 7UL
#define BLE_BLELL_LE_RF_TEST_MODE_PKT_PAYLOAD_Msk 0x380UL
#define BLE_BLELL_LE_RF_TEST_MODE_DTM_CONT_TXEN_Pos 13UL
#define BLE_BLELL_LE_RF_TEST_MODE_DTM_CONT_TXEN_Msk 0x2000UL
#define BLE_BLELL_LE_RF_TEST_MODE_DTM_DATA_2MBPS_Pos 15UL
#define BLE_BLELL_LE_RF_TEST_MODE_DTM_DATA_2MBPS_Msk 0x8000UL
/* BLE_BLELL.DTM_RX_PKT_COUNT */
#define BLE_BLELL_DTM_RX_PKT_COUNT_RX_PACKET_COUNT_Pos 0UL
#define BLE_BLELL_DTM_RX_PKT_COUNT_RX_PACKET_COUNT_Msk 0xFFFFUL
/* BLE_BLELL.LE_RF_TEST_MODE_EXT */
#define BLE_BLELL_LE_RF_TEST_MODE_EXT_DTM_PACKET_LENGTH_Pos 0UL
#define BLE_BLELL_LE_RF_TEST_MODE_EXT_DTM_PACKET_LENGTH_Msk 0xFFUL
/* BLE_BLELL.TXRX_HOP */
#define BLE_BLELL_TXRX_HOP_HOP_CH_TX_Pos        0UL
#define BLE_BLELL_TXRX_HOP_HOP_CH_TX_Msk        0x7FUL
#define BLE_BLELL_TXRX_HOP_HOP_CH_RX_Pos        8UL
#define BLE_BLELL_TXRX_HOP_HOP_CH_RX_Msk        0x7F00UL
/* BLE_BLELL.TX_RX_ON_DELAY */
#define BLE_BLELL_TX_RX_ON_DELAY_RXON_DELAY_Pos 0UL
#define BLE_BLELL_TX_RX_ON_DELAY_RXON_DELAY_Msk 0xFFUL
#define BLE_BLELL_TX_RX_ON_DELAY_TXON_DELAY_Pos 8UL
#define BLE_BLELL_TX_RX_ON_DELAY_TXON_DELAY_Msk 0xFF00UL
/* BLE_BLELL.ADV_ACCADDR_L */
#define BLE_BLELL_ADV_ACCADDR_L_ADV_ACCADDR_L_Pos 0UL
#define BLE_BLELL_ADV_ACCADDR_L_ADV_ACCADDR_L_Msk 0xFFFFUL
/* BLE_BLELL.ADV_ACCADDR_H */
#define BLE_BLELL_ADV_ACCADDR_H_ADV_ACCADDR_H_Pos 0UL
#define BLE_BLELL_ADV_ACCADDR_H_ADV_ACCADDR_H_Msk 0xFFFFUL
/* BLE_BLELL.ADV_CH_TX_POWER_LVL_LS */
#define BLE_BLELL_ADV_CH_TX_POWER_LVL_LS_ADV_TRANSMIT_POWER_LVL_LS_Pos 0UL
#define BLE_BLELL_ADV_CH_TX_POWER_LVL_LS_ADV_TRANSMIT_POWER_LVL_LS_Msk 0xFFFFUL
/* BLE_BLELL.ADV_CH_TX_POWER_LVL_MS */
#define BLE_BLELL_ADV_CH_TX_POWER_LVL_MS_ADV_TRANSMIT_POWER_LVL_MS_Pos 0UL
#define BLE_BLELL_ADV_CH_TX_POWER_LVL_MS_ADV_TRANSMIT_POWER_LVL_MS_Msk 0x3UL
/* BLE_BLELL.CONN_CH_TX_POWER_LVL_LS */
#define BLE_BLELL_CONN_CH_TX_POWER_LVL_LS_CONNCH_TRANSMIT_POWER_LVL_LS_Pos 0UL
#define BLE_BLELL_CONN_CH_TX_POWER_LVL_LS_CONNCH_TRANSMIT_POWER_LVL_LS_Msk 0xFFFFUL
/* BLE_BLELL.CONN_CH_TX_POWER_LVL_MS */
#define BLE_BLELL_CONN_CH_TX_POWER_LVL_MS_CONNCH_TRANSMIT_POWER_LVL_MS_Pos 0UL
#define BLE_BLELL_CONN_CH_TX_POWER_LVL_MS_CONNCH_TRANSMIT_POWER_LVL_MS_Msk 0x3UL
/* BLE_BLELL.DEV_PUB_ADDR_L */
#define BLE_BLELL_DEV_PUB_ADDR_L_DEV_PUB_ADDR_L_Pos 0UL
#define BLE_BLELL_DEV_PUB_ADDR_L_DEV_PUB_ADDR_L_Msk 0xFFFFUL
/* BLE_BLELL.DEV_PUB_ADDR_M */
#define BLE_BLELL_DEV_PUB_ADDR_M_DEV_PUB_ADDR_M_Pos 0UL
#define BLE_BLELL_DEV_PUB_ADDR_M_DEV_PUB_ADDR_M_Msk 0xFFFFUL
/* BLE_BLELL.DEV_PUB_ADDR_H */
#define BLE_BLELL_DEV_PUB_ADDR_H_DEV_PUB_ADDR_H_Pos 0UL
#define BLE_BLELL_DEV_PUB_ADDR_H_DEV_PUB_ADDR_H_Msk 0xFFFFUL
/* BLE_BLELL.OFFSET_TO_FIRST_INSTANT */
#define BLE_BLELL_OFFSET_TO_FIRST_INSTANT_OFFSET_TO_FIRST_EVENT_Pos 0UL
#define BLE_BLELL_OFFSET_TO_FIRST_INSTANT_OFFSET_TO_FIRST_EVENT_Msk 0xFFFFUL
/* BLE_BLELL.ADV_CONFIG */
#define BLE_BLELL_ADV_CONFIG_ADV_STRT_EN_Pos    0UL
#define BLE_BLELL_ADV_CONFIG_ADV_STRT_EN_Msk    0x1UL
#define BLE_BLELL_ADV_CONFIG_ADV_CLS_EN_Pos     1UL
#define BLE_BLELL_ADV_CONFIG_ADV_CLS_EN_Msk     0x2UL
#define BLE_BLELL_ADV_CONFIG_ADV_TX_EN_Pos      2UL
#define BLE_BLELL_ADV_CONFIG_ADV_TX_EN_Msk      0x4UL
#define BLE_BLELL_ADV_CONFIG_SCN_RSP_TX_EN_Pos  3UL
#define BLE_BLELL_ADV_CONFIG_SCN_RSP_TX_EN_Msk  0x8UL
#define BLE_BLELL_ADV_CONFIG_ADV_SCN_REQ_RX_EN_Pos 4UL
#define BLE_BLELL_ADV_CONFIG_ADV_SCN_REQ_RX_EN_Msk 0x10UL
#define BLE_BLELL_ADV_CONFIG_ADV_CONN_REQ_RX_EN_Pos 5UL
#define BLE_BLELL_ADV_CONFIG_ADV_CONN_REQ_RX_EN_Msk 0x20UL
#define BLE_BLELL_ADV_CONFIG_SLV_CONNECTED_EN_Pos 6UL
#define BLE_BLELL_ADV_CONFIG_SLV_CONNECTED_EN_Msk 0x40UL
#define BLE_BLELL_ADV_CONFIG_ADV_TIMEOUT_EN_Pos 7UL
#define BLE_BLELL_ADV_CONFIG_ADV_TIMEOUT_EN_Msk 0x80UL
#define BLE_BLELL_ADV_CONFIG_ADV_RAND_DISABLE_Pos 8UL
#define BLE_BLELL_ADV_CONFIG_ADV_RAND_DISABLE_Msk 0x100UL
#define BLE_BLELL_ADV_CONFIG_ADV_SCN_PEER_RPA_UNMCH_EN_Pos 9UL
#define BLE_BLELL_ADV_CONFIG_ADV_SCN_PEER_RPA_UNMCH_EN_Msk 0x200UL
#define BLE_BLELL_ADV_CONFIG_ADV_CONN_PEER_RPA_UNMCH_EN_Pos 10UL
#define BLE_BLELL_ADV_CONFIG_ADV_CONN_PEER_RPA_UNMCH_EN_Msk 0x400UL
#define BLE_BLELL_ADV_CONFIG_ADV_PKT_INTERVAL_Pos 11UL
#define BLE_BLELL_ADV_CONFIG_ADV_PKT_INTERVAL_Msk 0xF800UL
/* BLE_BLELL.SCAN_CONFIG */
#define BLE_BLELL_SCAN_CONFIG_SCN_STRT_EN_Pos   0UL
#define BLE_BLELL_SCAN_CONFIG_SCN_STRT_EN_Msk   0x1UL
#define BLE_BLELL_SCAN_CONFIG_SCN_CLOSE_EN_Pos  1UL
#define BLE_BLELL_SCAN_CONFIG_SCN_CLOSE_EN_Msk  0x2UL
#define BLE_BLELL_SCAN_CONFIG_SCN_TX_EN_Pos     2UL
#define BLE_BLELL_SCAN_CONFIG_SCN_TX_EN_Msk     0x4UL
#define BLE_BLELL_SCAN_CONFIG_ADV_RX_EN_Pos     3UL
#define BLE_BLELL_SCAN_CONFIG_ADV_RX_EN_Msk     0x8UL
#define BLE_BLELL_SCAN_CONFIG_SCN_RSP_RX_EN_Pos 4UL
#define BLE_BLELL_SCAN_CONFIG_SCN_RSP_RX_EN_Msk 0x10UL
#define BLE_BLELL_SCAN_CONFIG_SCN_ADV_RX_INTR_PEER_RPA_UNMCH_EN_Pos 5UL
#define BLE_BLELL_SCAN_CONFIG_SCN_ADV_RX_INTR_PEER_RPA_UNMCH_EN_Msk 0x20UL
#define BLE_BLELL_SCAN_CONFIG_SCN_ADV_RX_INTR_SELF_RPA_UNMCH_EN_Pos 6UL
#define BLE_BLELL_SCAN_CONFIG_SCN_ADV_RX_INTR_SELF_RPA_UNMCH_EN_Msk 0x40UL
#define BLE_BLELL_SCAN_CONFIG_SCANA_TX_ADDR_NOT_SET_INTR_EN_Pos 7UL
#define BLE_BLELL_SCAN_CONFIG_SCANA_TX_ADDR_NOT_SET_INTR_EN_Msk 0x80UL
#define BLE_BLELL_SCAN_CONFIG_RPT_SELF_ADDR_MATCH_PRIV_MISMATCH_SCN_Pos 8UL
#define BLE_BLELL_SCAN_CONFIG_RPT_SELF_ADDR_MATCH_PRIV_MISMATCH_SCN_Msk 0x100UL
#define BLE_BLELL_SCAN_CONFIG_BACKOFF_ENABLE_Pos 11UL
#define BLE_BLELL_SCAN_CONFIG_BACKOFF_ENABLE_Msk 0x800UL
#define BLE_BLELL_SCAN_CONFIG_SCAN_CHANNEL_MAP_Pos 13UL
#define BLE_BLELL_SCAN_CONFIG_SCAN_CHANNEL_MAP_Msk 0xE000UL
/* BLE_BLELL.INIT_CONFIG */
#define BLE_BLELL_INIT_CONFIG_INIT_STRT_EN_Pos  0UL
#define BLE_BLELL_INIT_CONFIG_INIT_STRT_EN_Msk  0x1UL
#define BLE_BLELL_INIT_CONFIG_INIT_CLOSE_EN_Pos 1UL
#define BLE_BLELL_INIT_CONFIG_INIT_CLOSE_EN_Msk 0x2UL
#define BLE_BLELL_INIT_CONFIG_CONN_REQ_TX_EN_Pos 2UL
#define BLE_BLELL_INIT_CONFIG_CONN_REQ_TX_EN_Msk 0x4UL
#define BLE_BLELL_INIT_CONFIG_CONN_CREATED_Pos  4UL
#define BLE_BLELL_INIT_CONFIG_CONN_CREATED_Msk  0x10UL
#define BLE_BLELL_INIT_CONFIG_INIT_ADV_RX_INTR_SELF_RPA_UNRES_EN_Pos 5UL
#define BLE_BLELL_INIT_CONFIG_INIT_ADV_RX_INTR_SELF_RPA_UNRES_EN_Msk 0x20UL
#define BLE_BLELL_INIT_CONFIG_INIT_ADV_RX_INTR_PEER_RPA_UNRES_EN_Pos 6UL
#define BLE_BLELL_INIT_CONFIG_INIT_ADV_RX_INTR_PEER_RPA_UNRES_EN_Msk 0x40UL
#define BLE_BLELL_INIT_CONFIG_INITA_TX_ADDR_NOT_SET_INTR_EN_Pos 7UL
#define BLE_BLELL_INIT_CONFIG_INITA_TX_ADDR_NOT_SET_INTR_EN_Msk 0x80UL
#define BLE_BLELL_INIT_CONFIG_INIT_CHANNEL_MAP_Pos 13UL
#define BLE_BLELL_INIT_CONFIG_INIT_CHANNEL_MAP_Msk 0xE000UL
/* BLE_BLELL.CONN_CONFIG */
#define BLE_BLELL_CONN_CONFIG_RX_PKT_LIMIT_Pos  0UL
#define BLE_BLELL_CONN_CONFIG_RX_PKT_LIMIT_Msk  0xFUL
#define BLE_BLELL_CONN_CONFIG_RX_INTR_THRESHOLD_Pos 4UL
#define BLE_BLELL_CONN_CONFIG_RX_INTR_THRESHOLD_Msk 0xF0UL
#define BLE_BLELL_CONN_CONFIG_MD_BIT_CLEAR_Pos  8UL
#define BLE_BLELL_CONN_CONFIG_MD_BIT_CLEAR_Msk  0x100UL
#define BLE_BLELL_CONN_CONFIG_DSM_SLOT_VARIANCE_Pos 11UL
#define BLE_BLELL_CONN_CONFIG_DSM_SLOT_VARIANCE_Msk 0x800UL
#define BLE_BLELL_CONN_CONFIG_SLV_MD_CONFIG_Pos 12UL
#define BLE_BLELL_CONN_CONFIG_SLV_MD_CONFIG_Msk 0x1000UL
#define BLE_BLELL_CONN_CONFIG_EXTEND_CU_TX_WIN_Pos 13UL
#define BLE_BLELL_CONN_CONFIG_EXTEND_CU_TX_WIN_Msk 0x2000UL
#define BLE_BLELL_CONN_CONFIG_MASK_SUTO_AT_UPDT_Pos 14UL
#define BLE_BLELL_CONN_CONFIG_MASK_SUTO_AT_UPDT_Msk 0x4000UL
#define BLE_BLELL_CONN_CONFIG_CONN_REQ_1SLOT_EARLY_Pos 15UL
#define BLE_BLELL_CONN_CONFIG_CONN_REQ_1SLOT_EARLY_Msk 0x8000UL
/* BLE_BLELL.CONN_PARAM1 */
#define BLE_BLELL_CONN_PARAM1_SCA_PARAM_Pos     0UL
#define BLE_BLELL_CONN_PARAM1_SCA_PARAM_Msk     0x7UL
#define BLE_BLELL_CONN_PARAM1_HOP_INCREMENT_PARAM_Pos 3UL
#define BLE_BLELL_CONN_PARAM1_HOP_INCREMENT_PARAM_Msk 0xF8UL
#define BLE_BLELL_CONN_PARAM1_CRC_INIT_L_Pos    8UL
#define BLE_BLELL_CONN_PARAM1_CRC_INIT_L_Msk    0xFF00UL
/* BLE_BLELL.CONN_PARAM2 */
#define BLE_BLELL_CONN_PARAM2_CRC_INIT_H_Pos    0UL
#define BLE_BLELL_CONN_PARAM2_CRC_INIT_H_Msk    0xFFFFUL
/* BLE_BLELL.CONN_INTR_MASK */
#define BLE_BLELL_CONN_INTR_MASK_CONN_CL_INT_EN_Pos 0UL
#define BLE_BLELL_CONN_INTR_MASK_CONN_CL_INT_EN_Msk 0x1UL
#define BLE_BLELL_CONN_INTR_MASK_CONN_ESTB_INT_EN_Pos 1UL
#define BLE_BLELL_CONN_INTR_MASK_CONN_ESTB_INT_EN_Msk 0x2UL
#define BLE_BLELL_CONN_INTR_MASK_MAP_UPDT_INT_EN_Pos 2UL
#define BLE_BLELL_CONN_INTR_MASK_MAP_UPDT_INT_EN_Msk 0x4UL
#define BLE_BLELL_CONN_INTR_MASK_START_CE_INT_EN_Pos 3UL
#define BLE_BLELL_CONN_INTR_MASK_START_CE_INT_EN_Msk 0x8UL
#define BLE_BLELL_CONN_INTR_MASK_CLOSE_CE_INT_EN_Pos 4UL
#define BLE_BLELL_CONN_INTR_MASK_CLOSE_CE_INT_EN_Msk 0x10UL
#define BLE_BLELL_CONN_INTR_MASK_CE_TX_ACK_INT_EN_Pos 5UL
#define BLE_BLELL_CONN_INTR_MASK_CE_TX_ACK_INT_EN_Msk 0x20UL
#define BLE_BLELL_CONN_INTR_MASK_CE_RX_INT_EN_Pos 6UL
#define BLE_BLELL_CONN_INTR_MASK_CE_RX_INT_EN_Msk 0x40UL
#define BLE_BLELL_CONN_INTR_MASK_CONN_UPDATE_INTR_EN_Pos 7UL
#define BLE_BLELL_CONN_INTR_MASK_CONN_UPDATE_INTR_EN_Msk 0x80UL
#define BLE_BLELL_CONN_INTR_MASK_RX_GOOD_PDU_INT_EN_Pos 8UL
#define BLE_BLELL_CONN_INTR_MASK_RX_GOOD_PDU_INT_EN_Msk 0x100UL
#define BLE_BLELL_CONN_INTR_MASK_RX_BAD_PDU_INT_EN_Pos 9UL
#define BLE_BLELL_CONN_INTR_MASK_RX_BAD_PDU_INT_EN_Msk 0x200UL
#define BLE_BLELL_CONN_INTR_MASK_CE_CLOSE_NULL_RX_INT_EN_Pos 13UL
#define BLE_BLELL_CONN_INTR_MASK_CE_CLOSE_NULL_RX_INT_EN_Msk 0x2000UL
#define BLE_BLELL_CONN_INTR_MASK_PING_TIMER_EXPIRD_INTR_Pos 14UL
#define BLE_BLELL_CONN_INTR_MASK_PING_TIMER_EXPIRD_INTR_Msk 0x4000UL
#define BLE_BLELL_CONN_INTR_MASK_PING_NEARLY_EXPIRD_INTR_Pos 15UL
#define BLE_BLELL_CONN_INTR_MASK_PING_NEARLY_EXPIRD_INTR_Msk 0x8000UL
/* BLE_BLELL.SLAVE_TIMING_CONTROL */
#define BLE_BLELL_SLAVE_TIMING_CONTROL_SLAVE_TIME_SET_VAL_Pos 0UL
#define BLE_BLELL_SLAVE_TIMING_CONTROL_SLAVE_TIME_SET_VAL_Msk 0xFFUL
#define BLE_BLELL_SLAVE_TIMING_CONTROL_SLAVE_TIME_ADJ_VAL_Pos 8UL
#define BLE_BLELL_SLAVE_TIMING_CONTROL_SLAVE_TIME_ADJ_VAL_Msk 0xFF00UL
/* BLE_BLELL.RECEIVE_TRIG_CTRL */
#define BLE_BLELL_RECEIVE_TRIG_CTRL_ACC_TRIGGER_THRESHOLD_Pos 0UL
#define BLE_BLELL_RECEIVE_TRIG_CTRL_ACC_TRIGGER_THRESHOLD_Msk 0x3FUL
#define BLE_BLELL_RECEIVE_TRIG_CTRL_ACC_TRIGGER_TIMEOUT_Pos 8UL
#define BLE_BLELL_RECEIVE_TRIG_CTRL_ACC_TRIGGER_TIMEOUT_Msk 0xFF00UL
/* BLE_BLELL.LL_DBG_1 */
#define BLE_BLELL_LL_DBG_1_CONN_RX_WR_PTR_Pos   0UL
#define BLE_BLELL_LL_DBG_1_CONN_RX_WR_PTR_Msk   0x3FFUL
/* BLE_BLELL.LL_DBG_2 */
#define BLE_BLELL_LL_DBG_2_CONN_RX_RD_PTR_Pos   0UL
#define BLE_BLELL_LL_DBG_2_CONN_RX_RD_PTR_Msk   0x3FFUL
/* BLE_BLELL.LL_DBG_3 */
#define BLE_BLELL_LL_DBG_3_CONN_RX_WR_PTR_STORE_Pos 0UL
#define BLE_BLELL_LL_DBG_3_CONN_RX_WR_PTR_STORE_Msk 0x3FFUL
/* BLE_BLELL.LL_DBG_4 */
#define BLE_BLELL_LL_DBG_4_CONNECTION_FSM_STATE_Pos 0UL
#define BLE_BLELL_LL_DBG_4_CONNECTION_FSM_STATE_Msk 0xFUL
#define BLE_BLELL_LL_DBG_4_SLAVE_LATENCY_FSM_STATE_Pos 4UL
#define BLE_BLELL_LL_DBG_4_SLAVE_LATENCY_FSM_STATE_Msk 0x30UL
#define BLE_BLELL_LL_DBG_4_ADVERTISER_FSM_STATE_Pos 6UL
#define BLE_BLELL_LL_DBG_4_ADVERTISER_FSM_STATE_Msk 0x7C0UL
/* BLE_BLELL.LL_DBG_5 */
#define BLE_BLELL_LL_DBG_5_INIT_FSM_STATE_Pos   0UL
#define BLE_BLELL_LL_DBG_5_INIT_FSM_STATE_Msk   0x1FUL
#define BLE_BLELL_LL_DBG_5_SCAN_FSM_STATE_Pos   5UL
#define BLE_BLELL_LL_DBG_5_SCAN_FSM_STATE_Msk   0x3E0UL
/* BLE_BLELL.LL_DBG_6 */
#define BLE_BLELL_LL_DBG_6_ADV_TX_WR_PTR_Pos    0UL
#define BLE_BLELL_LL_DBG_6_ADV_TX_WR_PTR_Msk    0xFUL
#define BLE_BLELL_LL_DBG_6_SCAN_RSP_TX_WR_PTR_Pos 4UL
#define BLE_BLELL_LL_DBG_6_SCAN_RSP_TX_WR_PTR_Msk 0xF0UL
#define BLE_BLELL_LL_DBG_6_ADV_TX_RD_PTR_Pos    8UL
#define BLE_BLELL_LL_DBG_6_ADV_TX_RD_PTR_Msk    0x3F00UL
/* BLE_BLELL.LL_DBG_7 */
#define BLE_BLELL_LL_DBG_7_ADV_RX_WR_PTR_Pos    0UL
#define BLE_BLELL_LL_DBG_7_ADV_RX_WR_PTR_Msk    0x7FUL
#define BLE_BLELL_LL_DBG_7_ADV_RX_RD_PTR_Pos    7UL
#define BLE_BLELL_LL_DBG_7_ADV_RX_RD_PTR_Msk    0x3F80UL
/* BLE_BLELL.LL_DBG_8 */
#define BLE_BLELL_LL_DBG_8_ADV_RX_WR_PTR_STORE_Pos 0UL
#define BLE_BLELL_LL_DBG_8_ADV_RX_WR_PTR_STORE_Msk 0x7FUL
#define BLE_BLELL_LL_DBG_8_WLF_PTR_Pos          7UL
#define BLE_BLELL_LL_DBG_8_WLF_PTR_Msk          0x3F80UL
/* BLE_BLELL.LL_DBG_9 */
#define BLE_BLELL_LL_DBG_9_WINDOW_WIDEN_Pos     0UL
#define BLE_BLELL_LL_DBG_9_WINDOW_WIDEN_Msk     0xFFFFUL
/* BLE_BLELL.LL_DBG_10 */
#define BLE_BLELL_LL_DBG_10_RF_CHANNEL_NUM_Pos  0UL
#define BLE_BLELL_LL_DBG_10_RF_CHANNEL_NUM_Msk  0x3FUL
/* BLE_BLELL.PEER_ADDR_INIT_L */
#define BLE_BLELL_PEER_ADDR_INIT_L_PEER_ADDR_L_Pos 0UL
#define BLE_BLELL_PEER_ADDR_INIT_L_PEER_ADDR_L_Msk 0xFFFFUL
/* BLE_BLELL.PEER_ADDR_INIT_M */
#define BLE_BLELL_PEER_ADDR_INIT_M_PEER_ADDR_M_Pos 0UL
#define BLE_BLELL_PEER_ADDR_INIT_M_PEER_ADDR_M_Msk 0xFFFFUL
/* BLE_BLELL.PEER_ADDR_INIT_H */
#define BLE_BLELL_PEER_ADDR_INIT_H_PEER_ADDR_H_Pos 0UL
#define BLE_BLELL_PEER_ADDR_INIT_H_PEER_ADDR_H_Msk 0xFFFFUL
/* BLE_BLELL.PEER_SEC_ADDR_ADV_L */
#define BLE_BLELL_PEER_SEC_ADDR_ADV_L_PEER_SEC_ADDR_L_Pos 0UL
#define BLE_BLELL_PEER_SEC_ADDR_ADV_L_PEER_SEC_ADDR_L_Msk 0xFFFFUL
/* BLE_BLELL.PEER_SEC_ADDR_ADV_M */
#define BLE_BLELL_PEER_SEC_ADDR_ADV_M_PEER_SEC_ADDR_M_Pos 0UL
#define BLE_BLELL_PEER_SEC_ADDR_ADV_M_PEER_SEC_ADDR_M_Msk 0xFFFFUL
/* BLE_BLELL.PEER_SEC_ADDR_ADV_H */
#define BLE_BLELL_PEER_SEC_ADDR_ADV_H_PEER_SEC_ADDR_H_Pos 0UL
#define BLE_BLELL_PEER_SEC_ADDR_ADV_H_PEER_SEC_ADDR_H_Msk 0xFFFFUL
/* BLE_BLELL.INIT_WINDOW_TIMER_CTRL */
#define BLE_BLELL_INIT_WINDOW_TIMER_CTRL_INIT_WINDOW_OFFSET_SEL_Pos 0UL
#define BLE_BLELL_INIT_WINDOW_TIMER_CTRL_INIT_WINDOW_OFFSET_SEL_Msk 0x1UL
/* BLE_BLELL.CONN_CONFIG_EXT */
#define BLE_BLELL_CONN_CONFIG_EXT_CONN_REQ_2SLOT_EARLY_Pos 0UL
#define BLE_BLELL_CONN_CONFIG_EXT_CONN_REQ_2SLOT_EARLY_Msk 0x1UL
#define BLE_BLELL_CONN_CONFIG_EXT_CONN_REQ_3SLOT_EARLY_Pos 1UL
#define BLE_BLELL_CONN_CONFIG_EXT_CONN_REQ_3SLOT_EARLY_Msk 0x2UL
#define BLE_BLELL_CONN_CONFIG_EXT_FW_PKT_RCV_CONN_INDEX_Pos 2UL
#define BLE_BLELL_CONN_CONFIG_EXT_FW_PKT_RCV_CONN_INDEX_Msk 0x7CUL
#define BLE_BLELL_CONN_CONFIG_EXT_MMMS_RX_PKT_LIMIT_Pos 8UL
#define BLE_BLELL_CONN_CONFIG_EXT_MMMS_RX_PKT_LIMIT_Msk 0x3F00UL
#define BLE_BLELL_CONN_CONFIG_EXT_DEBUG_CE_EXPIRE_Pos 14UL
#define BLE_BLELL_CONN_CONFIG_EXT_DEBUG_CE_EXPIRE_Msk 0x4000UL
#define BLE_BLELL_CONN_CONFIG_EXT_MT_PDU_CE_EXPIRE_Pos 15UL
#define BLE_BLELL_CONN_CONFIG_EXT_MT_PDU_CE_EXPIRE_Msk 0x8000UL
/* BLE_BLELL.DPLL_CONFIG */
#define BLE_BLELL_DPLL_CONFIG_DPLL_CORREL_CONFIG_Pos 0UL
#define BLE_BLELL_DPLL_CONFIG_DPLL_CORREL_CONFIG_Msk 0xFFFFUL
/* BLE_BLELL.INIT_NI_VAL */
#define BLE_BLELL_INIT_NI_VAL_INIT_NI_VAL_Pos   0UL
#define BLE_BLELL_INIT_NI_VAL_INIT_NI_VAL_Msk   0xFFFFUL
/* BLE_BLELL.INIT_WINDOW_OFFSET */
#define BLE_BLELL_INIT_WINDOW_OFFSET_INIT_WINDOW_NI_Pos 0UL
#define BLE_BLELL_INIT_WINDOW_OFFSET_INIT_WINDOW_NI_Msk 0xFFFFUL
/* BLE_BLELL.INIT_WINDOW_NI_ANCHOR_PT */
#define BLE_BLELL_INIT_WINDOW_NI_ANCHOR_PT_INIT_INT_OFF_CAPT_Pos 0UL
#define BLE_BLELL_INIT_WINDOW_NI_ANCHOR_PT_INIT_INT_OFF_CAPT_Msk 0xFFFFUL
/* BLE_BLELL.CONN_UPDATE_NEW_INTERVAL */
#define BLE_BLELL_CONN_UPDATE_NEW_INTERVAL_CONN_UPDT_INTERVAL_Pos 0UL
#define BLE_BLELL_CONN_UPDATE_NEW_INTERVAL_CONN_UPDT_INTERVAL_Msk 0xFFFFUL
/* BLE_BLELL.CONN_UPDATE_NEW_LATENCY */
#define BLE_BLELL_CONN_UPDATE_NEW_LATENCY_CONN_UPDT_SLV_LATENCY_Pos 0UL
#define BLE_BLELL_CONN_UPDATE_NEW_LATENCY_CONN_UPDT_SLV_LATENCY_Msk 0xFFFFUL
/* BLE_BLELL.CONN_UPDATE_NEW_SUP_TO */
#define BLE_BLELL_CONN_UPDATE_NEW_SUP_TO_CONN_UPDT_SUP_TO_Pos 0UL
#define BLE_BLELL_CONN_UPDATE_NEW_SUP_TO_CONN_UPDT_SUP_TO_Msk 0xFFFFUL
/* BLE_BLELL.CONN_UPDATE_NEW_SL_INTERVAL */
#define BLE_BLELL_CONN_UPDATE_NEW_SL_INTERVAL_SL_CONN_INTERVAL_VAL_Pos 0UL
#define BLE_BLELL_CONN_UPDATE_NEW_SL_INTERVAL_SL_CONN_INTERVAL_VAL_Msk 0xFFFFUL
/* BLE_BLELL.CONN_REQ_WORD0 */
#define BLE_BLELL_CONN_REQ_WORD0_ACCESS_ADDR_LOWER_Pos 0UL
#define BLE_BLELL_CONN_REQ_WORD0_ACCESS_ADDR_LOWER_Msk 0xFFFFUL
/* BLE_BLELL.CONN_REQ_WORD1 */
#define BLE_BLELL_CONN_REQ_WORD1_ACCESS_ADDR_UPPER_Pos 0UL
#define BLE_BLELL_CONN_REQ_WORD1_ACCESS_ADDR_UPPER_Msk 0xFFFFUL
/* BLE_BLELL.CONN_REQ_WORD2 */
#define BLE_BLELL_CONN_REQ_WORD2_TX_WINDOW_SIZE_VAL_Pos 0UL
#define BLE_BLELL_CONN_REQ_WORD2_TX_WINDOW_SIZE_VAL_Msk 0xFFUL
#define BLE_BLELL_CONN_REQ_WORD2_CRC_INIT_LOWER_Pos 8UL
#define BLE_BLELL_CONN_REQ_WORD2_CRC_INIT_LOWER_Msk 0xFF00UL
/* BLE_BLELL.CONN_REQ_WORD3 */
#define BLE_BLELL_CONN_REQ_WORD3_CRC_INIT_UPPER_Pos 0UL
#define BLE_BLELL_CONN_REQ_WORD3_CRC_INIT_UPPER_Msk 0xFFFFUL
/* BLE_BLELL.CONN_REQ_WORD4 */
#define BLE_BLELL_CONN_REQ_WORD4_TX_WINDOW_OFFSET_Pos 0UL
#define BLE_BLELL_CONN_REQ_WORD4_TX_WINDOW_OFFSET_Msk 0xFFFFUL
/* BLE_BLELL.CONN_REQ_WORD5 */
#define BLE_BLELL_CONN_REQ_WORD5_CONNECTION_INTERVAL_VAL_Pos 0UL
#define BLE_BLELL_CONN_REQ_WORD5_CONNECTION_INTERVAL_VAL_Msk 0xFFFFUL
/* BLE_BLELL.CONN_REQ_WORD6 */
#define BLE_BLELL_CONN_REQ_WORD6_SLAVE_LATENCY_VAL_Pos 0UL
#define BLE_BLELL_CONN_REQ_WORD6_SLAVE_LATENCY_VAL_Msk 0xFFFFUL
/* BLE_BLELL.CONN_REQ_WORD7 */
#define BLE_BLELL_CONN_REQ_WORD7_SUPERVISION_TIMEOUT_VAL_Pos 0UL
#define BLE_BLELL_CONN_REQ_WORD7_SUPERVISION_TIMEOUT_VAL_Msk 0xFFFFUL
/* BLE_BLELL.CONN_REQ_WORD8 */
#define BLE_BLELL_CONN_REQ_WORD8_DATA_CHANNELS_LOWER_Pos 0UL
#define BLE_BLELL_CONN_REQ_WORD8_DATA_CHANNELS_LOWER_Msk 0xFFFFUL
/* BLE_BLELL.CONN_REQ_WORD9 */
#define BLE_BLELL_CONN_REQ_WORD9_DATA_CHANNELS_MID_Pos 0UL
#define BLE_BLELL_CONN_REQ_WORD9_DATA_CHANNELS_MID_Msk 0xFFFFUL
/* BLE_BLELL.CONN_REQ_WORD10 */
#define BLE_BLELL_CONN_REQ_WORD10_DATA_CHANNELS_UPPER_Pos 0UL
#define BLE_BLELL_CONN_REQ_WORD10_DATA_CHANNELS_UPPER_Msk 0x1FUL
/* BLE_BLELL.CONN_REQ_WORD11 */
#define BLE_BLELL_CONN_REQ_WORD11_HOP_INCREMENT_2_Pos 0UL
#define BLE_BLELL_CONN_REQ_WORD11_HOP_INCREMENT_2_Msk 0x1FUL
#define BLE_BLELL_CONN_REQ_WORD11_SCA_2_Pos     5UL
#define BLE_BLELL_CONN_REQ_WORD11_SCA_2_Msk     0xE0UL
/* BLE_BLELL.PDU_RESP_TIMER */
#define BLE_BLELL_PDU_RESP_TIMER_PDU_RESP_TIME_VAL_Pos 0UL
#define BLE_BLELL_PDU_RESP_TIMER_PDU_RESP_TIME_VAL_Msk 0xFFFFUL
/* BLE_BLELL.NEXT_RESP_TIMER_EXP */
#define BLE_BLELL_NEXT_RESP_TIMER_EXP_NEXT_RESPONSE_INSTANT_Pos 0UL
#define BLE_BLELL_NEXT_RESP_TIMER_EXP_NEXT_RESPONSE_INSTANT_Msk 0xFFFFUL
/* BLE_BLELL.NEXT_SUP_TO */
#define BLE_BLELL_NEXT_SUP_TO_NEXT_TIMEOUT_INSTANT_Pos 0UL
#define BLE_BLELL_NEXT_SUP_TO_NEXT_TIMEOUT_INSTANT_Msk 0xFFFFUL
/* BLE_BLELL.LLH_FEATURE_CONFIG */
#define BLE_BLELL_LLH_FEATURE_CONFIG_QUICK_TRANSMIT_Pos 0UL
#define BLE_BLELL_LLH_FEATURE_CONFIG_QUICK_TRANSMIT_Msk 0x1UL
#define BLE_BLELL_LLH_FEATURE_CONFIG_SL_DSM_EN_Pos 1UL
#define BLE_BLELL_LLH_FEATURE_CONFIG_SL_DSM_EN_Msk 0x2UL
#define BLE_BLELL_LLH_FEATURE_CONFIG_US_COUNTER_OFFSET_ADJ_Pos 2UL
#define BLE_BLELL_LLH_FEATURE_CONFIG_US_COUNTER_OFFSET_ADJ_Msk 0x4UL
/* BLE_BLELL.WIN_MIN_STEP_SIZE */
#define BLE_BLELL_WIN_MIN_STEP_SIZE_STEPDN_Pos  0UL
#define BLE_BLELL_WIN_MIN_STEP_SIZE_STEPDN_Msk  0xFUL
#define BLE_BLELL_WIN_MIN_STEP_SIZE_STEPUP_Pos  4UL
#define BLE_BLELL_WIN_MIN_STEP_SIZE_STEPUP_Msk  0xF0UL
#define BLE_BLELL_WIN_MIN_STEP_SIZE_WINDOW_MIN_FW_Pos 8UL
#define BLE_BLELL_WIN_MIN_STEP_SIZE_WINDOW_MIN_FW_Msk 0xFF00UL
/* BLE_BLELL.SLV_WIN_ADJ */
#define BLE_BLELL_SLV_WIN_ADJ_SLV_WIN_ADJ_Pos   0UL
#define BLE_BLELL_SLV_WIN_ADJ_SLV_WIN_ADJ_Msk   0x7FFUL
/* BLE_BLELL.SL_CONN_INTERVAL */
#define BLE_BLELL_SL_CONN_INTERVAL_SL_CONN_INTERVAL_VAL_Pos 0UL
#define BLE_BLELL_SL_CONN_INTERVAL_SL_CONN_INTERVAL_VAL_Msk 0xFFFFUL
/* BLE_BLELL.LE_PING_TIMER_ADDR */
#define BLE_BLELL_LE_PING_TIMER_ADDR_CONN_PING_TIMER_ADDR_Pos 0UL
#define BLE_BLELL_LE_PING_TIMER_ADDR_CONN_PING_TIMER_ADDR_Msk 0xFFFFUL
/* BLE_BLELL.LE_PING_TIMER_OFFSET */
#define BLE_BLELL_LE_PING_TIMER_OFFSET_CONN_PING_TIMER_OFFSET_Pos 0UL
#define BLE_BLELL_LE_PING_TIMER_OFFSET_CONN_PING_TIMER_OFFSET_Msk 0xFFFFUL
/* BLE_BLELL.LE_PING_TIMER_NEXT_EXP */
#define BLE_BLELL_LE_PING_TIMER_NEXT_EXP_CONN_PING_TIMER_NEXT_EXP_Pos 0UL
#define BLE_BLELL_LE_PING_TIMER_NEXT_EXP_CONN_PING_TIMER_NEXT_EXP_Msk 0xFFFFUL
/* BLE_BLELL.LE_PING_TIMER_WRAP_COUNT */
#define BLE_BLELL_LE_PING_TIMER_WRAP_COUNT_CONN_SEC_CURRENT_WRAP_Pos 0UL
#define BLE_BLELL_LE_PING_TIMER_WRAP_COUNT_CONN_SEC_CURRENT_WRAP_Msk 0xFFFFUL
/* BLE_BLELL.TX_EN_EXT_DELAY */
#define BLE_BLELL_TX_EN_EXT_DELAY_TXEN_EXT_DELAY_Pos 0UL
#define BLE_BLELL_TX_EN_EXT_DELAY_TXEN_EXT_DELAY_Msk 0xFUL
#define BLE_BLELL_TX_EN_EXT_DELAY_RXEN_EXT_DELAY_Pos 4UL
#define BLE_BLELL_TX_EN_EXT_DELAY_RXEN_EXT_DELAY_Msk 0xF0UL
#define BLE_BLELL_TX_EN_EXT_DELAY_DEMOD_2M_COMP_DLY_Pos 8UL
#define BLE_BLELL_TX_EN_EXT_DELAY_DEMOD_2M_COMP_DLY_Msk 0xF00UL
#define BLE_BLELL_TX_EN_EXT_DELAY_MOD_2M_COMP_DLY_Pos 12UL
#define BLE_BLELL_TX_EN_EXT_DELAY_MOD_2M_COMP_DLY_Msk 0xF000UL
/* BLE_BLELL.TX_RX_SYNTH_DELAY */
#define BLE_BLELL_TX_RX_SYNTH_DELAY_RX_EN_DELAY_Pos 0UL
#define BLE_BLELL_TX_RX_SYNTH_DELAY_RX_EN_DELAY_Msk 0xFFUL
#define BLE_BLELL_TX_RX_SYNTH_DELAY_TX_EN_DELAY_Pos 8UL
#define BLE_BLELL_TX_RX_SYNTH_DELAY_TX_EN_DELAY_Msk 0xFF00UL
/* BLE_BLELL.EXT_PA_LNA_DLY_CNFG */
#define BLE_BLELL_EXT_PA_LNA_DLY_CNFG_LNA_CTL_DELAY_Pos 0UL
#define BLE_BLELL_EXT_PA_LNA_DLY_CNFG_LNA_CTL_DELAY_Msk 0xFFUL
#define BLE_BLELL_EXT_PA_LNA_DLY_CNFG_PA_CTL_DELAY_Pos 8UL
#define BLE_BLELL_EXT_PA_LNA_DLY_CNFG_PA_CTL_DELAY_Msk 0xFF00UL
/* BLE_BLELL.LL_CONFIG */
#define BLE_BLELL_LL_CONFIG_RSSI_SEL_Pos        0UL
#define BLE_BLELL_LL_CONFIG_RSSI_SEL_Msk        0x1UL
#define BLE_BLELL_LL_CONFIG_TX_RX_CTRL_SEL_Pos  1UL
#define BLE_BLELL_LL_CONFIG_TX_RX_CTRL_SEL_Msk  0x2UL
#define BLE_BLELL_LL_CONFIG_TIFS_ENABLE_Pos     2UL
#define BLE_BLELL_LL_CONFIG_TIFS_ENABLE_Msk     0x4UL
#define BLE_BLELL_LL_CONFIG_TIMER_LF_SLOT_ENABLE_Pos 3UL
#define BLE_BLELL_LL_CONFIG_TIMER_LF_SLOT_ENABLE_Msk 0x8UL
#define BLE_BLELL_LL_CONFIG_RSSI_INTR_SEL_Pos   5UL
#define BLE_BLELL_LL_CONFIG_RSSI_INTR_SEL_Msk   0x20UL
#define BLE_BLELL_LL_CONFIG_RSSI_EARLY_CNFG_Pos 6UL
#define BLE_BLELL_LL_CONFIG_RSSI_EARLY_CNFG_Msk 0x40UL
#define BLE_BLELL_LL_CONFIG_TX_RX_PIN_DLY_Pos   7UL
#define BLE_BLELL_LL_CONFIG_TX_RX_PIN_DLY_Msk   0x80UL
#define BLE_BLELL_LL_CONFIG_TX_PA_PWR_LVL_TYPE_Pos 8UL
#define BLE_BLELL_LL_CONFIG_TX_PA_PWR_LVL_TYPE_Msk 0x100UL
#define BLE_BLELL_LL_CONFIG_RSSI_ENERGY_RD_Pos  9UL
#define BLE_BLELL_LL_CONFIG_RSSI_ENERGY_RD_Msk  0x200UL
#define BLE_BLELL_LL_CONFIG_RSSI_EACH_PKT_Pos   10UL
#define BLE_BLELL_LL_CONFIG_RSSI_EACH_PKT_Msk   0x400UL
#define BLE_BLELL_LL_CONFIG_FORCE_TRIG_RCB_UPDATE_Pos 11UL
#define BLE_BLELL_LL_CONFIG_FORCE_TRIG_RCB_UPDATE_Msk 0x800UL
#define BLE_BLELL_LL_CONFIG_CHECK_DUP_CONN_Pos  12UL
#define BLE_BLELL_LL_CONFIG_CHECK_DUP_CONN_Msk  0x1000UL
#define BLE_BLELL_LL_CONFIG_MULTI_ENGINE_LPM_Pos 13UL
#define BLE_BLELL_LL_CONFIG_MULTI_ENGINE_LPM_Msk 0x2000UL
#define BLE_BLELL_LL_CONFIG_ADV_DIR_DEVICE_PRIV_EN_Pos 14UL
#define BLE_BLELL_LL_CONFIG_ADV_DIR_DEVICE_PRIV_EN_Msk 0x4000UL
/* BLE_BLELL.LL_CONTROL */
#define BLE_BLELL_LL_CONTROL_PRIV_1_2_Pos       0UL
#define BLE_BLELL_LL_CONTROL_PRIV_1_2_Msk       0x1UL
#define BLE_BLELL_LL_CONTROL_DLE_Pos            1UL
#define BLE_BLELL_LL_CONTROL_DLE_Msk            0x2UL
#define BLE_BLELL_LL_CONTROL_WL_READ_AS_MEM_Pos 2UL
#define BLE_BLELL_LL_CONTROL_WL_READ_AS_MEM_Msk 0x4UL
#define BLE_BLELL_LL_CONTROL_ADVCH_FIFO_PRIV_1_2_FLUSH_CTRL_Pos 3UL
#define BLE_BLELL_LL_CONTROL_ADVCH_FIFO_PRIV_1_2_FLUSH_CTRL_Msk 0x8UL
#define BLE_BLELL_LL_CONTROL_HW_RSLV_LIST_FULL_Pos 4UL
#define BLE_BLELL_LL_CONTROL_HW_RSLV_LIST_FULL_Msk 0x10UL
#define BLE_BLELL_LL_CONTROL_RPT_INIT_ADDR_MATCH_PRIV_MISMATCH_ADV_Pos 5UL
#define BLE_BLELL_LL_CONTROL_RPT_INIT_ADDR_MATCH_PRIV_MISMATCH_ADV_Msk 0x20UL
#define BLE_BLELL_LL_CONTROL_RPT_SCAN_ADDR_MATCH_PRIV_MISMATCH_ADV_Pos 6UL
#define BLE_BLELL_LL_CONTROL_RPT_SCAN_ADDR_MATCH_PRIV_MISMATCH_ADV_Msk 0x40UL
#define BLE_BLELL_LL_CONTROL_RPT_PEER_ADDR_MATCH_PRIV_MISMATCH_SCN_Pos 7UL
#define BLE_BLELL_LL_CONTROL_RPT_PEER_ADDR_MATCH_PRIV_MISMATCH_SCN_Msk 0x80UL
#define BLE_BLELL_LL_CONTROL_RPT_PEER_ADDR_MATCH_PRIV_MISMATCH_INI_Pos 8UL
#define BLE_BLELL_LL_CONTROL_RPT_PEER_ADDR_MATCH_PRIV_MISMATCH_INI_Msk 0x100UL
#define BLE_BLELL_LL_CONTROL_RPT_SELF_ADDR_MATCH_PRIV_MISMATCH_INI_Pos 9UL
#define BLE_BLELL_LL_CONTROL_RPT_SELF_ADDR_MATCH_PRIV_MISMATCH_INI_Msk 0x200UL
#define BLE_BLELL_LL_CONTROL_PRIV_1_2_ADV_Pos   10UL
#define BLE_BLELL_LL_CONTROL_PRIV_1_2_ADV_Msk   0x400UL
#define BLE_BLELL_LL_CONTROL_PRIV_1_2_SCAN_Pos  11UL
#define BLE_BLELL_LL_CONTROL_PRIV_1_2_SCAN_Msk  0x800UL
#define BLE_BLELL_LL_CONTROL_PRIV_1_2_INIT_Pos  12UL
#define BLE_BLELL_LL_CONTROL_PRIV_1_2_INIT_Msk  0x1000UL
#define BLE_BLELL_LL_CONTROL_EN_CONN_RX_EN_MOD_Pos 13UL
#define BLE_BLELL_LL_CONTROL_EN_CONN_RX_EN_MOD_Msk 0x2000UL
#define BLE_BLELL_LL_CONTROL_SLV_CONN_PEER_RPA_NOT_RSLVD_Pos 14UL
#define BLE_BLELL_LL_CONTROL_SLV_CONN_PEER_RPA_NOT_RSLVD_Msk 0x4000UL
#define BLE_BLELL_LL_CONTROL_ADVCH_FIFO_FLUSH_Pos 15UL
#define BLE_BLELL_LL_CONTROL_ADVCH_FIFO_FLUSH_Msk 0x8000UL
/* BLE_BLELL.DEV_PA_ADDR_L */
#define BLE_BLELL_DEV_PA_ADDR_L_DEV_PA_ADDR_L_Pos 0UL
#define BLE_BLELL_DEV_PA_ADDR_L_DEV_PA_ADDR_L_Msk 0xFFFFUL
/* BLE_BLELL.DEV_PA_ADDR_M */
#define BLE_BLELL_DEV_PA_ADDR_M_DEV_PA_ADDR_M_Pos 0UL
#define BLE_BLELL_DEV_PA_ADDR_M_DEV_PA_ADDR_M_Msk 0xFFFFUL
/* BLE_BLELL.DEV_PA_ADDR_H */
#define BLE_BLELL_DEV_PA_ADDR_H_DEV_PA_ADDR_H_Pos 0UL
#define BLE_BLELL_DEV_PA_ADDR_H_DEV_PA_ADDR_H_Msk 0xFFFFUL
/* BLE_BLELL.RSLV_LIST_ENABLE */
#define BLE_BLELL_RSLV_LIST_ENABLE_VALID_ENTRY_Pos 0UL
#define BLE_BLELL_RSLV_LIST_ENABLE_VALID_ENTRY_Msk 0x1UL
#define BLE_BLELL_RSLV_LIST_ENABLE_PEER_ADDR_IRK_SET_Pos 1UL
#define BLE_BLELL_RSLV_LIST_ENABLE_PEER_ADDR_IRK_SET_Msk 0x2UL
#define BLE_BLELL_RSLV_LIST_ENABLE_SELF_ADDR_IRK_SET_RX_Pos 2UL
#define BLE_BLELL_RSLV_LIST_ENABLE_SELF_ADDR_IRK_SET_RX_Msk 0x4UL
#define BLE_BLELL_RSLV_LIST_ENABLE_WHITELISTED_PEER_Pos 3UL
#define BLE_BLELL_RSLV_LIST_ENABLE_WHITELISTED_PEER_Msk 0x8UL
#define BLE_BLELL_RSLV_LIST_ENABLE_PEER_ADDR_TYPE_Pos 4UL
#define BLE_BLELL_RSLV_LIST_ENABLE_PEER_ADDR_TYPE_Msk 0x10UL
#define BLE_BLELL_RSLV_LIST_ENABLE_PEER_ADDR_RPA_VAL_Pos 5UL
#define BLE_BLELL_RSLV_LIST_ENABLE_PEER_ADDR_RPA_VAL_Msk 0x20UL
#define BLE_BLELL_RSLV_LIST_ENABLE_SELF_ADDR_RXD_RPA_VAL_Pos 6UL
#define BLE_BLELL_RSLV_LIST_ENABLE_SELF_ADDR_RXD_RPA_VAL_Msk 0x40UL
#define BLE_BLELL_RSLV_LIST_ENABLE_SELF_ADDR_TX_RPA_VAL_Pos 7UL
#define BLE_BLELL_RSLV_LIST_ENABLE_SELF_ADDR_TX_RPA_VAL_Msk 0x80UL
#define BLE_BLELL_RSLV_LIST_ENABLE_SELF_ADDR_INIT_RPA_SEL_Pos 8UL
#define BLE_BLELL_RSLV_LIST_ENABLE_SELF_ADDR_INIT_RPA_SEL_Msk 0x100UL
#define BLE_BLELL_RSLV_LIST_ENABLE_SELF_ADDR_TYPE_TX_Pos 9UL
#define BLE_BLELL_RSLV_LIST_ENABLE_SELF_ADDR_TYPE_TX_Msk 0x200UL
#define BLE_BLELL_RSLV_LIST_ENABLE_ENTRY_CONNECTED_Pos 10UL
#define BLE_BLELL_RSLV_LIST_ENABLE_ENTRY_CONNECTED_Msk 0x400UL
/* BLE_BLELL.WL_CONNECTION_STATUS */
#define BLE_BLELL_WL_CONNECTION_STATUS_WL_ENTRY_CONNECTED_Pos 0UL
#define BLE_BLELL_WL_CONNECTION_STATUS_WL_ENTRY_CONNECTED_Msk 0xFFFFUL
/* BLE_BLELL.CONN_RXMEM_BASE_ADDR_DLE */
#define BLE_BLELL_CONN_RXMEM_BASE_ADDR_DLE_CONN_RX_MEM_BASE_ADDR_DLE_Pos 0UL
#define BLE_BLELL_CONN_RXMEM_BASE_ADDR_DLE_CONN_RX_MEM_BASE_ADDR_DLE_Msk 0xFFFFFFFFUL
/* BLE_BLELL.CONN_TXMEM_BASE_ADDR_DLE */
#define BLE_BLELL_CONN_TXMEM_BASE_ADDR_DLE_CONN_TX_MEM_BASE_ADDR_DLE_Pos 0UL
#define BLE_BLELL_CONN_TXMEM_BASE_ADDR_DLE_CONN_TX_MEM_BASE_ADDR_DLE_Msk 0xFFFFFFFFUL
/* BLE_BLELL.CONN_1_PARAM_MEM_BASE_ADDR */
#define BLE_BLELL_CONN_1_PARAM_MEM_BASE_ADDR_CONN_1_PARAM_Pos 0UL
#define BLE_BLELL_CONN_1_PARAM_MEM_BASE_ADDR_CONN_1_PARAM_Msk 0xFFFFUL
/* BLE_BLELL.CONN_2_PARAM_MEM_BASE_ADDR */
#define BLE_BLELL_CONN_2_PARAM_MEM_BASE_ADDR_CONN_2_PARAM_Pos 0UL
#define BLE_BLELL_CONN_2_PARAM_MEM_BASE_ADDR_CONN_2_PARAM_Msk 0xFFFFUL
/* BLE_BLELL.CONN_3_PARAM_MEM_BASE_ADDR */
#define BLE_BLELL_CONN_3_PARAM_MEM_BASE_ADDR_CONN_3_PARAM_Pos 0UL
#define BLE_BLELL_CONN_3_PARAM_MEM_BASE_ADDR_CONN_3_PARAM_Msk 0xFFFFUL
/* BLE_BLELL.CONN_4_PARAM_MEM_BASE_ADDR */
#define BLE_BLELL_CONN_4_PARAM_MEM_BASE_ADDR_CONN_4_PARAM_Pos 0UL
#define BLE_BLELL_CONN_4_PARAM_MEM_BASE_ADDR_CONN_4_PARAM_Msk 0xFFFFUL
/* BLE_BLELL.NI_TIMER */
#define BLE_BLELL_NI_TIMER_NI_TIMER_Pos         0UL
#define BLE_BLELL_NI_TIMER_NI_TIMER_Msk         0xFFFFUL
/* BLE_BLELL.US_OFFSET */
#define BLE_BLELL_US_OFFSET_US_OFFSET_SLOT_BOUNDARY_Pos 0UL
#define BLE_BLELL_US_OFFSET_US_OFFSET_SLOT_BOUNDARY_Msk 0x3FFUL
/* BLE_BLELL.NEXT_CONN */
#define BLE_BLELL_NEXT_CONN_NEXT_CONN_INDEX_Pos 0UL
#define BLE_BLELL_NEXT_CONN_NEXT_CONN_INDEX_Msk 0x1FUL
#define BLE_BLELL_NEXT_CONN_NEXT_CONN_TYPE_Pos  5UL
#define BLE_BLELL_NEXT_CONN_NEXT_CONN_TYPE_Msk  0x20UL
#define BLE_BLELL_NEXT_CONN_NI_VALID_Pos        6UL
#define BLE_BLELL_NEXT_CONN_NI_VALID_Msk        0x40UL
/* BLE_BLELL.NI_ABORT */
#define BLE_BLELL_NI_ABORT_NI_ABORT_Pos         0UL
#define BLE_BLELL_NI_ABORT_NI_ABORT_Msk         0x1UL
#define BLE_BLELL_NI_ABORT_ABORT_ACK_Pos        1UL
#define BLE_BLELL_NI_ABORT_ABORT_ACK_Msk        0x2UL
/* BLE_BLELL.CONN_NI_STATUS */
#define BLE_BLELL_CONN_NI_STATUS_CONN_NI_Pos    0UL
#define BLE_BLELL_CONN_NI_STATUS_CONN_NI_Msk    0xFFFFUL
/* BLE_BLELL.NEXT_SUP_TO_STATUS */
#define BLE_BLELL_NEXT_SUP_TO_STATUS_NEXT_SUP_TO_Pos 0UL
#define BLE_BLELL_NEXT_SUP_TO_STATUS_NEXT_SUP_TO_Msk 0xFFFFUL
/* BLE_BLELL.MMMS_CONN_STATUS */
#define BLE_BLELL_MMMS_CONN_STATUS_CURR_CONN_INDEX_Pos 0UL
#define BLE_BLELL_MMMS_CONN_STATUS_CURR_CONN_INDEX_Msk 0x1FUL
#define BLE_BLELL_MMMS_CONN_STATUS_CURR_CONN_TYPE_Pos 5UL
#define BLE_BLELL_MMMS_CONN_STATUS_CURR_CONN_TYPE_Msk 0x20UL
#define BLE_BLELL_MMMS_CONN_STATUS_SN_CURR_Pos  6UL
#define BLE_BLELL_MMMS_CONN_STATUS_SN_CURR_Msk  0x40UL
#define BLE_BLELL_MMMS_CONN_STATUS_NESN_CURR_Pos 7UL
#define BLE_BLELL_MMMS_CONN_STATUS_NESN_CURR_Msk 0x80UL
#define BLE_BLELL_MMMS_CONN_STATUS_LAST_UNMAPPED_CHANNEL_Pos 8UL
#define BLE_BLELL_MMMS_CONN_STATUS_LAST_UNMAPPED_CHANNEL_Msk 0x3F00UL
#define BLE_BLELL_MMMS_CONN_STATUS_PKT_MISS_Pos 14UL
#define BLE_BLELL_MMMS_CONN_STATUS_PKT_MISS_Msk 0x4000UL
#define BLE_BLELL_MMMS_CONN_STATUS_ANCHOR_PT_STATE_Pos 15UL
#define BLE_BLELL_MMMS_CONN_STATUS_ANCHOR_PT_STATE_Msk 0x8000UL
/* BLE_BLELL.BT_SLOT_CAPT_STATUS */
#define BLE_BLELL_BT_SLOT_CAPT_STATUS_BT_SLOT_Pos 0UL
#define BLE_BLELL_BT_SLOT_CAPT_STATUS_BT_SLOT_Msk 0xFFFFUL
/* BLE_BLELL.US_CAPT_STATUS */
#define BLE_BLELL_US_CAPT_STATUS_US_CAPT_Pos    0UL
#define BLE_BLELL_US_CAPT_STATUS_US_CAPT_Msk    0x3FFUL
/* BLE_BLELL.US_OFFSET_STATUS */
#define BLE_BLELL_US_OFFSET_STATUS_US_OFFSET_Pos 0UL
#define BLE_BLELL_US_OFFSET_STATUS_US_OFFSET_Msk 0xFFFFUL
/* BLE_BLELL.ACCU_WINDOW_WIDEN_STATUS */
#define BLE_BLELL_ACCU_WINDOW_WIDEN_STATUS_ACCU_WINDOW_WIDEN_Pos 0UL
#define BLE_BLELL_ACCU_WINDOW_WIDEN_STATUS_ACCU_WINDOW_WIDEN_Msk 0xFFFFUL
/* BLE_BLELL.EARLY_INTR_STATUS */
#define BLE_BLELL_EARLY_INTR_STATUS_CONN_INDEX_FOR_EARLY_INTR_Pos 0UL
#define BLE_BLELL_EARLY_INTR_STATUS_CONN_INDEX_FOR_EARLY_INTR_Msk 0x1FUL
#define BLE_BLELL_EARLY_INTR_STATUS_CONN_TYPE_FOR_EARLY_INTR_Pos 5UL
#define BLE_BLELL_EARLY_INTR_STATUS_CONN_TYPE_FOR_EARLY_INTR_Msk 0x20UL
#define BLE_BLELL_EARLY_INTR_STATUS_US_FOR_EARLY_INTR_Pos 6UL
#define BLE_BLELL_EARLY_INTR_STATUS_US_FOR_EARLY_INTR_Msk 0xFFC0UL
/* BLE_BLELL.MMMS_CONFIG */
#define BLE_BLELL_MMMS_CONFIG_MMMS_ENABLE_Pos   0UL
#define BLE_BLELL_MMMS_CONFIG_MMMS_ENABLE_Msk   0x1UL
#define BLE_BLELL_MMMS_CONFIG_DISABLE_CONN_REQ_PARAM_IN_MEM_Pos 1UL
#define BLE_BLELL_MMMS_CONFIG_DISABLE_CONN_REQ_PARAM_IN_MEM_Msk 0x2UL
#define BLE_BLELL_MMMS_CONFIG_DISABLE_CONN_PARAM_MEM_WR_Pos 2UL
#define BLE_BLELL_MMMS_CONFIG_DISABLE_CONN_PARAM_MEM_WR_Msk 0x4UL
#define BLE_BLELL_MMMS_CONFIG_CONN_PARAM_FROM_REG_Pos 3UL
#define BLE_BLELL_MMMS_CONFIG_CONN_PARAM_FROM_REG_Msk 0x8UL
#define BLE_BLELL_MMMS_CONFIG_ADV_CONN_INDEX_Pos 4UL
#define BLE_BLELL_MMMS_CONFIG_ADV_CONN_INDEX_Msk 0x1F0UL
#define BLE_BLELL_MMMS_CONFIG_CE_LEN_IMMEDIATE_EXPIRE_Pos 9UL
#define BLE_BLELL_MMMS_CONFIG_CE_LEN_IMMEDIATE_EXPIRE_Msk 0x200UL
#define BLE_BLELL_MMMS_CONFIG_RESET_RX_FIFO_PTR_Pos 10UL
#define BLE_BLELL_MMMS_CONFIG_RESET_RX_FIFO_PTR_Msk 0x400UL
/* BLE_BLELL.US_COUNTER */
#define BLE_BLELL_US_COUNTER_US_COUNTER_Pos     0UL
#define BLE_BLELL_US_COUNTER_US_COUNTER_Msk     0x3FFUL
/* BLE_BLELL.US_CAPT_PREV */
#define BLE_BLELL_US_CAPT_PREV_US_CAPT_LOAD_Pos 0UL
#define BLE_BLELL_US_CAPT_PREV_US_CAPT_LOAD_Msk 0x3FFUL
/* BLE_BLELL.EARLY_INTR_NI */
#define BLE_BLELL_EARLY_INTR_NI_EARLY_INTR_NI_Pos 0UL
#define BLE_BLELL_EARLY_INTR_NI_EARLY_INTR_NI_Msk 0xFFFFUL
/* BLE_BLELL.MMMS_MASTER_CREATE_BT_CAPT */
#define BLE_BLELL_MMMS_MASTER_CREATE_BT_CAPT_BT_SLOT_Pos 0UL
#define BLE_BLELL_MMMS_MASTER_CREATE_BT_CAPT_BT_SLOT_Msk 0xFFFFUL
/* BLE_BLELL.MMMS_SLAVE_CREATE_BT_CAPT */
#define BLE_BLELL_MMMS_SLAVE_CREATE_BT_CAPT_US_CAPT_Pos 0UL
#define BLE_BLELL_MMMS_SLAVE_CREATE_BT_CAPT_US_CAPT_Msk 0x3FFUL
/* BLE_BLELL.MMMS_SLAVE_CREATE_US_CAPT */
#define BLE_BLELL_MMMS_SLAVE_CREATE_US_CAPT_US_OFFSET_SLAVE_CREATED_Pos 0UL
#define BLE_BLELL_MMMS_SLAVE_CREATE_US_CAPT_US_OFFSET_SLAVE_CREATED_Msk 0xFFFFUL
/* BLE_BLELL.MMMS_DATA_MEM_DESCRIPTOR */
#define BLE_BLELL_MMMS_DATA_MEM_DESCRIPTOR_LLID_C1_Pos 0UL
#define BLE_BLELL_MMMS_DATA_MEM_DESCRIPTOR_LLID_C1_Msk 0x3UL
#define BLE_BLELL_MMMS_DATA_MEM_DESCRIPTOR_DATA_LENGTH_C1_Pos 2UL
#define BLE_BLELL_MMMS_DATA_MEM_DESCRIPTOR_DATA_LENGTH_C1_Msk 0x3FCUL
/* BLE_BLELL.CONN_1_DATA_LIST_SENT */
#define BLE_BLELL_CONN_1_DATA_LIST_SENT_LIST_INDEX__TX_SENT_3_0_C1_Pos 0UL
#define BLE_BLELL_CONN_1_DATA_LIST_SENT_LIST_INDEX__TX_SENT_3_0_C1_Msk 0xFUL
#define BLE_BLELL_CONN_1_DATA_LIST_SENT_SET_CLEAR_C1_Pos 7UL
#define BLE_BLELL_CONN_1_DATA_LIST_SENT_SET_CLEAR_C1_Msk 0x80UL
#define BLE_BLELL_CONN_1_DATA_LIST_SENT_BUFFER_NUM_TX_SENT_3_0_C1_Pos 8UL
#define BLE_BLELL_CONN_1_DATA_LIST_SENT_BUFFER_NUM_TX_SENT_3_0_C1_Msk 0xF00UL
/* BLE_BLELL.CONN_1_DATA_LIST_ACK */
#define BLE_BLELL_CONN_1_DATA_LIST_ACK_LIST_INDEX__TX_ACK_3_0_C1_Pos 0UL
#define BLE_BLELL_CONN_1_DATA_LIST_ACK_LIST_INDEX__TX_ACK_3_0_C1_Msk 0xFUL
#define BLE_BLELL_CONN_1_DATA_LIST_ACK_SET_CLEAR_C1_Pos 7UL
#define BLE_BLELL_CONN_1_DATA_LIST_ACK_SET_CLEAR_C1_Msk 0x80UL
/* BLE_BLELL.CONN_1_CE_DATA_LIST_CFG */
#define BLE_BLELL_CONN_1_CE_DATA_LIST_CFG_DATA_LIST_INDEX_LAST_ACK_INDEX_C1_Pos 0UL
#define BLE_BLELL_CONN_1_CE_DATA_LIST_CFG_DATA_LIST_INDEX_LAST_ACK_INDEX_C1_Msk 0xFUL
#define BLE_BLELL_CONN_1_CE_DATA_LIST_CFG_DATA_LIST_HEAD_UP_C1_Pos 4UL
#define BLE_BLELL_CONN_1_CE_DATA_LIST_CFG_DATA_LIST_HEAD_UP_C1_Msk 0x10UL
#define BLE_BLELL_CONN_1_CE_DATA_LIST_CFG_SLV_MD_CONFIG_C1_Pos 5UL
#define BLE_BLELL_CONN_1_CE_DATA_LIST_CFG_SLV_MD_CONFIG_C1_Msk 0x20UL
#define BLE_BLELL_CONN_1_CE_DATA_LIST_CFG_MD_C1_Pos 6UL
#define BLE_BLELL_CONN_1_CE_DATA_LIST_CFG_MD_C1_Msk 0x40UL
#define BLE_BLELL_CONN_1_CE_DATA_LIST_CFG_MD_BIT_CLEAR_C1_Pos 7UL
#define BLE_BLELL_CONN_1_CE_DATA_LIST_CFG_MD_BIT_CLEAR_C1_Msk 0x80UL
#define BLE_BLELL_CONN_1_CE_DATA_LIST_CFG_PAUSE_DATA_C1_Pos 8UL
#define BLE_BLELL_CONN_1_CE_DATA_LIST_CFG_PAUSE_DATA_C1_Msk 0x100UL
#define BLE_BLELL_CONN_1_CE_DATA_LIST_CFG_KILL_CONN_Pos 9UL
#define BLE_BLELL_CONN_1_CE_DATA_LIST_CFG_KILL_CONN_Msk 0x200UL
#define BLE_BLELL_CONN_1_CE_DATA_LIST_CFG_KILL_CONN_AFTER_TX_Pos 10UL
#define BLE_BLELL_CONN_1_CE_DATA_LIST_CFG_KILL_CONN_AFTER_TX_Msk 0x400UL
#define BLE_BLELL_CONN_1_CE_DATA_LIST_CFG_EMPTYPDU_SENT_Pos 11UL
#define BLE_BLELL_CONN_1_CE_DATA_LIST_CFG_EMPTYPDU_SENT_Msk 0x800UL
#define BLE_BLELL_CONN_1_CE_DATA_LIST_CFG_CURRENT_PDU_INDEX_C1_Pos 12UL
#define BLE_BLELL_CONN_1_CE_DATA_LIST_CFG_CURRENT_PDU_INDEX_C1_Msk 0xF000UL
/* BLE_BLELL.CONN_2_DATA_LIST_SENT */
#define BLE_BLELL_CONN_2_DATA_LIST_SENT_LIST_INDEX__TX_SENT_3_0_C1_Pos 0UL
#define BLE_BLELL_CONN_2_DATA_LIST_SENT_LIST_INDEX__TX_SENT_3_0_C1_Msk 0xFUL
#define BLE_BLELL_CONN_2_DATA_LIST_SENT_SET_CLEAR_C1_Pos 7UL
#define BLE_BLELL_CONN_2_DATA_LIST_SENT_SET_CLEAR_C1_Msk 0x80UL
#define BLE_BLELL_CONN_2_DATA_LIST_SENT_BUFFER_NUM_TX_SENT_3_0_C1_Pos 8UL
#define BLE_BLELL_CONN_2_DATA_LIST_SENT_BUFFER_NUM_TX_SENT_3_0_C1_Msk 0xF00UL
/* BLE_BLELL.CONN_2_DATA_LIST_ACK */
#define BLE_BLELL_CONN_2_DATA_LIST_ACK_LIST_INDEX__TX_ACK_3_0_C1_Pos 0UL
#define BLE_BLELL_CONN_2_DATA_LIST_ACK_LIST_INDEX__TX_ACK_3_0_C1_Msk 0xFUL
#define BLE_BLELL_CONN_2_DATA_LIST_ACK_SET_CLEAR_C1_Pos 7UL
#define BLE_BLELL_CONN_2_DATA_LIST_ACK_SET_CLEAR_C1_Msk 0x80UL
/* BLE_BLELL.CONN_2_CE_DATA_LIST_CFG */
#define BLE_BLELL_CONN_2_CE_DATA_LIST_CFG_DATA_LIST_INDEX_LAST_ACK_INDEX_C1_Pos 0UL
#define BLE_BLELL_CONN_2_CE_DATA_LIST_CFG_DATA_LIST_INDEX_LAST_ACK_INDEX_C1_Msk 0xFUL
#define BLE_BLELL_CONN_2_CE_DATA_LIST_CFG_DATA_LIST_HEAD_UP_C1_Pos 4UL
#define BLE_BLELL_CONN_2_CE_DATA_LIST_CFG_DATA_LIST_HEAD_UP_C1_Msk 0x10UL
#define BLE_BLELL_CONN_2_CE_DATA_LIST_CFG_SLV_MD_CONFIG_C1_Pos 5UL
#define BLE_BLELL_CONN_2_CE_DATA_LIST_CFG_SLV_MD_CONFIG_C1_Msk 0x20UL
#define BLE_BLELL_CONN_2_CE_DATA_LIST_CFG_MD_C1_Pos 6UL
#define BLE_BLELL_CONN_2_CE_DATA_LIST_CFG_MD_C1_Msk 0x40UL
#define BLE_BLELL_CONN_2_CE_DATA_LIST_CFG_MD_BIT_CLEAR_C1_Pos 7UL
#define BLE_BLELL_CONN_2_CE_DATA_LIST_CFG_MD_BIT_CLEAR_C1_Msk 0x80UL
#define BLE_BLELL_CONN_2_CE_DATA_LIST_CFG_PAUSE_DATA_C1_Pos 8UL
#define BLE_BLELL_CONN_2_CE_DATA_LIST_CFG_PAUSE_DATA_C1_Msk 0x100UL
#define BLE_BLELL_CONN_2_CE_DATA_LIST_CFG_KILL_CONN_Pos 9UL
#define BLE_BLELL_CONN_2_CE_DATA_LIST_CFG_KILL_CONN_Msk 0x200UL
#define BLE_BLELL_CONN_2_CE_DATA_LIST_CFG_KILL_CONN_AFTER_TX_Pos 10UL
#define BLE_BLELL_CONN_2_CE_DATA_LIST_CFG_KILL_CONN_AFTER_TX_Msk 0x400UL
#define BLE_BLELL_CONN_2_CE_DATA_LIST_CFG_EMPTYPDU_SENT_Pos 11UL
#define BLE_BLELL_CONN_2_CE_DATA_LIST_CFG_EMPTYPDU_SENT_Msk 0x800UL
#define BLE_BLELL_CONN_2_CE_DATA_LIST_CFG_CURRENT_PDU_INDEX_C1_Pos 12UL
#define BLE_BLELL_CONN_2_CE_DATA_LIST_CFG_CURRENT_PDU_INDEX_C1_Msk 0xF000UL
/* BLE_BLELL.CONN_3_DATA_LIST_SENT */
#define BLE_BLELL_CONN_3_DATA_LIST_SENT_LIST_INDEX__TX_SENT_3_0_C1_Pos 0UL
#define BLE_BLELL_CONN_3_DATA_LIST_SENT_LIST_INDEX__TX_SENT_3_0_C1_Msk 0xFUL
#define BLE_BLELL_CONN_3_DATA_LIST_SENT_SET_CLEAR_C1_Pos 7UL
#define BLE_BLELL_CONN_3_DATA_LIST_SENT_SET_CLEAR_C1_Msk 0x80UL
#define BLE_BLELL_CONN_3_DATA_LIST_SENT_BUFFER_NUM_TX_SENT_3_0_C1_Pos 8UL
#define BLE_BLELL_CONN_3_DATA_LIST_SENT_BUFFER_NUM_TX_SENT_3_0_C1_Msk 0xF00UL
/* BLE_BLELL.CONN_3_DATA_LIST_ACK */
#define BLE_BLELL_CONN_3_DATA_LIST_ACK_LIST_INDEX__TX_ACK_3_0_C1_Pos 0UL
#define BLE_BLELL_CONN_3_DATA_LIST_ACK_LIST_INDEX__TX_ACK_3_0_C1_Msk 0xFUL
#define BLE_BLELL_CONN_3_DATA_LIST_ACK_SET_CLEAR_C1_Pos 7UL
#define BLE_BLELL_CONN_3_DATA_LIST_ACK_SET_CLEAR_C1_Msk 0x80UL
/* BLE_BLELL.CONN_3_CE_DATA_LIST_CFG */
#define BLE_BLELL_CONN_3_CE_DATA_LIST_CFG_DATA_LIST_INDEX_LAST_ACK_INDEX_C1_Pos 0UL
#define BLE_BLELL_CONN_3_CE_DATA_LIST_CFG_DATA_LIST_INDEX_LAST_ACK_INDEX_C1_Msk 0xFUL
#define BLE_BLELL_CONN_3_CE_DATA_LIST_CFG_DATA_LIST_HEAD_UP_C1_Pos 4UL
#define BLE_BLELL_CONN_3_CE_DATA_LIST_CFG_DATA_LIST_HEAD_UP_C1_Msk 0x10UL
#define BLE_BLELL_CONN_3_CE_DATA_LIST_CFG_SLV_MD_CONFIG_C1_Pos 5UL
#define BLE_BLELL_CONN_3_CE_DATA_LIST_CFG_SLV_MD_CONFIG_C1_Msk 0x20UL
#define BLE_BLELL_CONN_3_CE_DATA_LIST_CFG_MD_C1_Pos 6UL
#define BLE_BLELL_CONN_3_CE_DATA_LIST_CFG_MD_C1_Msk 0x40UL
#define BLE_BLELL_CONN_3_CE_DATA_LIST_CFG_MD_BIT_CLEAR_C1_Pos 7UL
#define BLE_BLELL_CONN_3_CE_DATA_LIST_CFG_MD_BIT_CLEAR_C1_Msk 0x80UL
#define BLE_BLELL_CONN_3_CE_DATA_LIST_CFG_PAUSE_DATA_C1_Pos 8UL
#define BLE_BLELL_CONN_3_CE_DATA_LIST_CFG_PAUSE_DATA_C1_Msk 0x100UL
#define BLE_BLELL_CONN_3_CE_DATA_LIST_CFG_KILL_CONN_Pos 9UL
#define BLE_BLELL_CONN_3_CE_DATA_LIST_CFG_KILL_CONN_Msk 0x200UL
#define BLE_BLELL_CONN_3_CE_DATA_LIST_CFG_KILL_CONN_AFTER_TX_Pos 10UL
#define BLE_BLELL_CONN_3_CE_DATA_LIST_CFG_KILL_CONN_AFTER_TX_Msk 0x400UL
#define BLE_BLELL_CONN_3_CE_DATA_LIST_CFG_EMPTYPDU_SENT_Pos 11UL
#define BLE_BLELL_CONN_3_CE_DATA_LIST_CFG_EMPTYPDU_SENT_Msk 0x800UL
#define BLE_BLELL_CONN_3_CE_DATA_LIST_CFG_CURRENT_PDU_INDEX_C1_Pos 12UL
#define BLE_BLELL_CONN_3_CE_DATA_LIST_CFG_CURRENT_PDU_INDEX_C1_Msk 0xF000UL
/* BLE_BLELL.CONN_4_DATA_LIST_SENT */
#define BLE_BLELL_CONN_4_DATA_LIST_SENT_LIST_INDEX__TX_SENT_3_0_C1_Pos 0UL
#define BLE_BLELL_CONN_4_DATA_LIST_SENT_LIST_INDEX__TX_SENT_3_0_C1_Msk 0xFUL
#define BLE_BLELL_CONN_4_DATA_LIST_SENT_SET_CLEAR_C1_Pos 7UL
#define BLE_BLELL_CONN_4_DATA_LIST_SENT_SET_CLEAR_C1_Msk 0x80UL
#define BLE_BLELL_CONN_4_DATA_LIST_SENT_BUFFER_NUM_TX_SENT_3_0_C1_Pos 8UL
#define BLE_BLELL_CONN_4_DATA_LIST_SENT_BUFFER_NUM_TX_SENT_3_0_C1_Msk 0xF00UL
/* BLE_BLELL.CONN_4_DATA_LIST_ACK */
#define BLE_BLELL_CONN_4_DATA_LIST_ACK_LIST_INDEX__TX_ACK_3_0_C1_Pos 0UL
#define BLE_BLELL_CONN_4_DATA_LIST_ACK_LIST_INDEX__TX_ACK_3_0_C1_Msk 0xFUL
#define BLE_BLELL_CONN_4_DATA_LIST_ACK_SET_CLEAR_C1_Pos 7UL
#define BLE_BLELL_CONN_4_DATA_LIST_ACK_SET_CLEAR_C1_Msk 0x80UL
/* BLE_BLELL.CONN_4_CE_DATA_LIST_CFG */
#define BLE_BLELL_CONN_4_CE_DATA_LIST_CFG_DATA_LIST_INDEX_LAST_ACK_INDEX_C1_Pos 0UL
#define BLE_BLELL_CONN_4_CE_DATA_LIST_CFG_DATA_LIST_INDEX_LAST_ACK_INDEX_C1_Msk 0xFUL
#define BLE_BLELL_CONN_4_CE_DATA_LIST_CFG_DATA_LIST_HEAD_UP_C1_Pos 4UL
#define BLE_BLELL_CONN_4_CE_DATA_LIST_CFG_DATA_LIST_HEAD_UP_C1_Msk 0x10UL
#define BLE_BLELL_CONN_4_CE_DATA_LIST_CFG_SLV_MD_CONFIG_C1_Pos 5UL
#define BLE_BLELL_CONN_4_CE_DATA_LIST_CFG_SLV_MD_CONFIG_C1_Msk 0x20UL
#define BLE_BLELL_CONN_4_CE_DATA_LIST_CFG_MD_C1_Pos 6UL
#define BLE_BLELL_CONN_4_CE_DATA_LIST_CFG_MD_C1_Msk 0x40UL
#define BLE_BLELL_CONN_4_CE_DATA_LIST_CFG_MD_BIT_CLEAR_C1_Pos 7UL
#define BLE_BLELL_CONN_4_CE_DATA_LIST_CFG_MD_BIT_CLEAR_C1_Msk 0x80UL
#define BLE_BLELL_CONN_4_CE_DATA_LIST_CFG_PAUSE_DATA_C1_Pos 8UL
#define BLE_BLELL_CONN_4_CE_DATA_LIST_CFG_PAUSE_DATA_C1_Msk 0x100UL
#define BLE_BLELL_CONN_4_CE_DATA_LIST_CFG_KILL_CONN_Pos 9UL
#define BLE_BLELL_CONN_4_CE_DATA_LIST_CFG_KILL_CONN_Msk 0x200UL
#define BLE_BLELL_CONN_4_CE_DATA_LIST_CFG_KILL_CONN_AFTER_TX_Pos 10UL
#define BLE_BLELL_CONN_4_CE_DATA_LIST_CFG_KILL_CONN_AFTER_TX_Msk 0x400UL
#define BLE_BLELL_CONN_4_CE_DATA_LIST_CFG_EMPTYPDU_SENT_Pos 11UL
#define BLE_BLELL_CONN_4_CE_DATA_LIST_CFG_EMPTYPDU_SENT_Msk 0x800UL
#define BLE_BLELL_CONN_4_CE_DATA_LIST_CFG_CURRENT_PDU_INDEX_C1_Pos 12UL
#define BLE_BLELL_CONN_4_CE_DATA_LIST_CFG_CURRENT_PDU_INDEX_C1_Msk 0xF000UL
/* BLE_BLELL.MMMS_ADVCH_NI_ENABLE */
#define BLE_BLELL_MMMS_ADVCH_NI_ENABLE_ADV_NI_ENABLE_Pos 0UL
#define BLE_BLELL_MMMS_ADVCH_NI_ENABLE_ADV_NI_ENABLE_Msk 0x1UL
#define BLE_BLELL_MMMS_ADVCH_NI_ENABLE_SCAN_NI_ENABLE_Pos 1UL
#define BLE_BLELL_MMMS_ADVCH_NI_ENABLE_SCAN_NI_ENABLE_Msk 0x2UL
#define BLE_BLELL_MMMS_ADVCH_NI_ENABLE_INIT_NI_ENABLE_Pos 2UL
#define BLE_BLELL_MMMS_ADVCH_NI_ENABLE_INIT_NI_ENABLE_Msk 0x4UL
/* BLE_BLELL.MMMS_ADVCH_NI_VALID */
#define BLE_BLELL_MMMS_ADVCH_NI_VALID_ADV_NI_VALID_Pos 0UL
#define BLE_BLELL_MMMS_ADVCH_NI_VALID_ADV_NI_VALID_Msk 0x1UL
#define BLE_BLELL_MMMS_ADVCH_NI_VALID_SCAN_NI_VALID_Pos 1UL
#define BLE_BLELL_MMMS_ADVCH_NI_VALID_SCAN_NI_VALID_Msk 0x2UL
#define BLE_BLELL_MMMS_ADVCH_NI_VALID_INIT_NI_VALID_Pos 2UL
#define BLE_BLELL_MMMS_ADVCH_NI_VALID_INIT_NI_VALID_Msk 0x4UL
/* BLE_BLELL.MMMS_ADVCH_NI_ABORT */
#define BLE_BLELL_MMMS_ADVCH_NI_ABORT_ADVCH_NI_ABORT_Pos 0UL
#define BLE_BLELL_MMMS_ADVCH_NI_ABORT_ADVCH_NI_ABORT_Msk 0x1UL
#define BLE_BLELL_MMMS_ADVCH_NI_ABORT_ADVCH_ABORT_STATUS_Pos 1UL
#define BLE_BLELL_MMMS_ADVCH_NI_ABORT_ADVCH_ABORT_STATUS_Msk 0x2UL
/* BLE_BLELL.CONN_PARAM_NEXT_SUP_TO */
#define BLE_BLELL_CONN_PARAM_NEXT_SUP_TO_NEXT_SUP_TO_LOAD_Pos 0UL
#define BLE_BLELL_CONN_PARAM_NEXT_SUP_TO_NEXT_SUP_TO_LOAD_Msk 0xFFFFUL
/* BLE_BLELL.CONN_PARAM_ACC_WIN_WIDEN */
#define BLE_BLELL_CONN_PARAM_ACC_WIN_WIDEN_ACC_WINDOW_WIDEN_Pos 0UL
#define BLE_BLELL_CONN_PARAM_ACC_WIN_WIDEN_ACC_WINDOW_WIDEN_Msk 0x3FFUL
/* BLE_BLELL.HW_LOAD_OFFSET */
#define BLE_BLELL_HW_LOAD_OFFSET_LOAD_OFFSET_Pos 0UL
#define BLE_BLELL_HW_LOAD_OFFSET_LOAD_OFFSET_Msk 0x1FUL
/* BLE_BLELL.ADV_RAND */
#define BLE_BLELL_ADV_RAND_ADV_RAND_Pos         0UL
#define BLE_BLELL_ADV_RAND_ADV_RAND_Msk         0xFUL
/* BLE_BLELL.MMMS_RX_PKT_CNTR */
#define BLE_BLELL_MMMS_RX_PKT_CNTR_MMMS_RX_PKT_CNT_Pos 0UL
#define BLE_BLELL_MMMS_RX_PKT_CNTR_MMMS_RX_PKT_CNT_Msk 0x3FUL
/* BLE_BLELL.CONN_RX_PKT_CNTR */
#define BLE_BLELL_CONN_RX_PKT_CNTR_RX_PKT_CNT_Pos 0UL
#define BLE_BLELL_CONN_RX_PKT_CNTR_RX_PKT_CNT_Msk 0x3FUL
/* BLE_BLELL.WHITELIST_BASE_ADDR */
#define BLE_BLELL_WHITELIST_BASE_ADDR_WL_BASE_ADDR_Pos 0UL
#define BLE_BLELL_WHITELIST_BASE_ADDR_WL_BASE_ADDR_Msk 0xFFFFUL
/* BLE_BLELL.RSLV_LIST_PEER_IDNTT_BASE_ADDR */
#define BLE_BLELL_RSLV_LIST_PEER_IDNTT_BASE_ADDR_RSLV_LIST_PEER_IDNTT_BASE_ADDR_Pos 0UL
#define BLE_BLELL_RSLV_LIST_PEER_IDNTT_BASE_ADDR_RSLV_LIST_PEER_IDNTT_BASE_ADDR_Msk 0xFFFFUL
/* BLE_BLELL.RSLV_LIST_PEER_RPA_BASE_ADDR */
#define BLE_BLELL_RSLV_LIST_PEER_RPA_BASE_ADDR_RSLV_LIST_PEER_RPA_BASE_ADDR_Pos 0UL
#define BLE_BLELL_RSLV_LIST_PEER_RPA_BASE_ADDR_RSLV_LIST_PEER_RPA_BASE_ADDR_Msk 0xFFFFUL
/* BLE_BLELL.RSLV_LIST_RCVD_INIT_RPA_BASE_ADDR */
#define BLE_BLELL_RSLV_LIST_RCVD_INIT_RPA_BASE_ADDR_RSLV_LIST_RCVD_INIT_RPA_BASE_ADDR_Pos 0UL
#define BLE_BLELL_RSLV_LIST_RCVD_INIT_RPA_BASE_ADDR_RSLV_LIST_RCVD_INIT_RPA_BASE_ADDR_Msk 0xFFFFUL
/* BLE_BLELL.RSLV_LIST_TX_INIT_RPA_BASE_ADDR */
#define BLE_BLELL_RSLV_LIST_TX_INIT_RPA_BASE_ADDR_RSLV_LIST_TX_INIT_RPA_BASE_ADDR_Pos 0UL
#define BLE_BLELL_RSLV_LIST_TX_INIT_RPA_BASE_ADDR_RSLV_LIST_TX_INIT_RPA_BASE_ADDR_Msk 0xFFFFUL


/* BLE_BLESS.DDFT_CONFIG */
#define BLE_BLESS_DDFT_CONFIG_DDFT_ENABLE_Pos   0UL
#define BLE_BLESS_DDFT_CONFIG_DDFT_ENABLE_Msk   0x1UL
#define BLE_BLESS_DDFT_CONFIG_BLERD_DDFT_EN_Pos 1UL
#define BLE_BLESS_DDFT_CONFIG_BLERD_DDFT_EN_Msk 0x2UL
#define BLE_BLESS_DDFT_CONFIG_DDFT_MUX_CFG1_Pos 8UL
#define BLE_BLESS_DDFT_CONFIG_DDFT_MUX_CFG1_Msk 0x1F00UL
#define BLE_BLESS_DDFT_CONFIG_DDFT_MUX_CFG2_Pos 16UL
#define BLE_BLESS_DDFT_CONFIG_DDFT_MUX_CFG2_Msk 0x1F0000UL
/* BLE_BLESS.XTAL_CLK_DIV_CONFIG */
#define BLE_BLESS_XTAL_CLK_DIV_CONFIG_SYSCLK_DIV_Pos 0UL
#define BLE_BLESS_XTAL_CLK_DIV_CONFIG_SYSCLK_DIV_Msk 0x3UL
#define BLE_BLESS_XTAL_CLK_DIV_CONFIG_LLCLK_DIV_Pos 2UL
#define BLE_BLESS_XTAL_CLK_DIV_CONFIG_LLCLK_DIV_Msk 0xCUL
/* BLE_BLESS.INTR_STAT */
#define BLE_BLESS_INTR_STAT_DSM_ENTERED_INTR_Pos 0UL
#define BLE_BLESS_INTR_STAT_DSM_ENTERED_INTR_Msk 0x1UL
#define BLE_BLESS_INTR_STAT_DSM_EXITED_INTR_Pos 1UL
#define BLE_BLESS_INTR_STAT_DSM_EXITED_INTR_Msk 0x2UL
#define BLE_BLESS_INTR_STAT_RCBLL_DONE_INTR_Pos 2UL
#define BLE_BLESS_INTR_STAT_RCBLL_DONE_INTR_Msk 0x4UL
#define BLE_BLESS_INTR_STAT_BLERD_ACTIVE_INTR_Pos 3UL
#define BLE_BLESS_INTR_STAT_BLERD_ACTIVE_INTR_Msk 0x8UL
#define BLE_BLESS_INTR_STAT_RCB_INTR_Pos        4UL
#define BLE_BLESS_INTR_STAT_RCB_INTR_Msk        0x10UL
#define BLE_BLESS_INTR_STAT_LL_INTR_Pos         5UL
#define BLE_BLESS_INTR_STAT_LL_INTR_Msk         0x20UL
#define BLE_BLESS_INTR_STAT_GPIO_INTR_Pos       6UL
#define BLE_BLESS_INTR_STAT_GPIO_INTR_Msk       0x40UL
#define BLE_BLESS_INTR_STAT_EFUSE_INTR_Pos      7UL
#define BLE_BLESS_INTR_STAT_EFUSE_INTR_Msk      0x80UL
#define BLE_BLESS_INTR_STAT_XTAL_ON_INTR_Pos    8UL
#define BLE_BLESS_INTR_STAT_XTAL_ON_INTR_Msk    0x100UL
#define BLE_BLESS_INTR_STAT_ENC_INTR_Pos        9UL
#define BLE_BLESS_INTR_STAT_ENC_INTR_Msk        0x200UL
#define BLE_BLESS_INTR_STAT_HVLDO_LV_DETECT_POS_Pos 10UL
#define BLE_BLESS_INTR_STAT_HVLDO_LV_DETECT_POS_Msk 0x400UL
#define BLE_BLESS_INTR_STAT_HVLDO_LV_DETECT_NEG_Pos 11UL
#define BLE_BLESS_INTR_STAT_HVLDO_LV_DETECT_NEG_Msk 0x800UL
/* BLE_BLESS.INTR_MASK */
#define BLE_BLESS_INTR_MASK_DSM_EXIT_Pos        0UL
#define BLE_BLESS_INTR_MASK_DSM_EXIT_Msk        0x1UL
#define BLE_BLESS_INTR_MASK_DSM_ENTERED_INTR_MASK_Pos 1UL
#define BLE_BLESS_INTR_MASK_DSM_ENTERED_INTR_MASK_Msk 0x2UL
#define BLE_BLESS_INTR_MASK_DSM_EXITED_INTR_MASK_Pos 2UL
#define BLE_BLESS_INTR_MASK_DSM_EXITED_INTR_MASK_Msk 0x4UL
#define BLE_BLESS_INTR_MASK_XTAL_ON_INTR_MASK_Pos 3UL
#define BLE_BLESS_INTR_MASK_XTAL_ON_INTR_MASK_Msk 0x8UL
#define BLE_BLESS_INTR_MASK_RCBLL_INTR_MASK_Pos 4UL
#define BLE_BLESS_INTR_MASK_RCBLL_INTR_MASK_Msk 0x10UL
#define BLE_BLESS_INTR_MASK_BLERD_ACTIVE_INTR_MASK_Pos 5UL
#define BLE_BLESS_INTR_MASK_BLERD_ACTIVE_INTR_MASK_Msk 0x20UL
#define BLE_BLESS_INTR_MASK_RCB_INTR_MASK_Pos   6UL
#define BLE_BLESS_INTR_MASK_RCB_INTR_MASK_Msk   0x40UL
#define BLE_BLESS_INTR_MASK_LL_INTR_MASK_Pos    7UL
#define BLE_BLESS_INTR_MASK_LL_INTR_MASK_Msk    0x80UL
#define BLE_BLESS_INTR_MASK_GPIO_INTR_MASK_Pos  8UL
#define BLE_BLESS_INTR_MASK_GPIO_INTR_MASK_Msk  0x100UL
#define BLE_BLESS_INTR_MASK_EFUSE_INTR_MASK_Pos 9UL
#define BLE_BLESS_INTR_MASK_EFUSE_INTR_MASK_Msk 0x200UL
#define BLE_BLESS_INTR_MASK_ENC_INTR_MASK_Pos   10UL
#define BLE_BLESS_INTR_MASK_ENC_INTR_MASK_Msk   0x400UL
#define BLE_BLESS_INTR_MASK_HVLDO_LV_DETECT_POS_MASK_Pos 11UL
#define BLE_BLESS_INTR_MASK_HVLDO_LV_DETECT_POS_MASK_Msk 0x800UL
#define BLE_BLESS_INTR_MASK_HVLDO_LV_DETECT_NEG_MASK_Pos 12UL
#define BLE_BLESS_INTR_MASK_HVLDO_LV_DETECT_NEG_MASK_Msk 0x1000UL
/* BLE_BLESS.LL_CLK_EN */
#define BLE_BLESS_LL_CLK_EN_CLK_EN_Pos          0UL
#define BLE_BLESS_LL_CLK_EN_CLK_EN_Msk          0x1UL
#define BLE_BLESS_LL_CLK_EN_CY_CORREL_EN_Pos    1UL
#define BLE_BLESS_LL_CLK_EN_CY_CORREL_EN_Msk    0x2UL
#define BLE_BLESS_LL_CLK_EN_MXD_IF_OPTION_Pos   2UL
#define BLE_BLESS_LL_CLK_EN_MXD_IF_OPTION_Msk   0x4UL
#define BLE_BLESS_LL_CLK_EN_SEL_RCB_CLK_Pos     3UL
#define BLE_BLESS_LL_CLK_EN_SEL_RCB_CLK_Msk     0x8UL
#define BLE_BLESS_LL_CLK_EN_BLESS_RESET_Pos     4UL
#define BLE_BLESS_LL_CLK_EN_BLESS_RESET_Msk     0x10UL
#define BLE_BLESS_LL_CLK_EN_DPSLP_HWRCB_EN_Pos  5UL
#define BLE_BLESS_LL_CLK_EN_DPSLP_HWRCB_EN_Msk  0x20UL
/* BLE_BLESS.LF_CLK_CTRL */
#define BLE_BLESS_LF_CLK_CTRL_DISABLE_LF_CLK_Pos 0UL
#define BLE_BLESS_LF_CLK_CTRL_DISABLE_LF_CLK_Msk 0x1UL
#define BLE_BLESS_LF_CLK_CTRL_ENABLE_ENC_CLK_Pos 1UL
#define BLE_BLESS_LF_CLK_CTRL_ENABLE_ENC_CLK_Msk 0x2UL
#define BLE_BLESS_LF_CLK_CTRL_M0S8BLESS_REV_ID_Pos 29UL
#define BLE_BLESS_LF_CLK_CTRL_M0S8BLESS_REV_ID_Msk 0xE0000000UL
/* BLE_BLESS.EXT_PA_LNA_CTRL */
#define BLE_BLESS_EXT_PA_LNA_CTRL_ENABLE_EXT_PA_LNA_Pos 1UL
#define BLE_BLESS_EXT_PA_LNA_CTRL_ENABLE_EXT_PA_LNA_Msk 0x2UL
#define BLE_BLESS_EXT_PA_LNA_CTRL_CHIP_EN_POL_Pos 2UL
#define BLE_BLESS_EXT_PA_LNA_CTRL_CHIP_EN_POL_Msk 0x4UL
#define BLE_BLESS_EXT_PA_LNA_CTRL_PA_CTRL_POL_Pos 3UL
#define BLE_BLESS_EXT_PA_LNA_CTRL_PA_CTRL_POL_Msk 0x8UL
#define BLE_BLESS_EXT_PA_LNA_CTRL_LNA_CTRL_POL_Pos 4UL
#define BLE_BLESS_EXT_PA_LNA_CTRL_LNA_CTRL_POL_Msk 0x10UL
#define BLE_BLESS_EXT_PA_LNA_CTRL_OUT_EN_DRIVE_VAL_Pos 5UL
#define BLE_BLESS_EXT_PA_LNA_CTRL_OUT_EN_DRIVE_VAL_Msk 0x20UL
/* BLE_BLESS.LL_PKT_RSSI_CH_ENERGY */
#define BLE_BLESS_LL_PKT_RSSI_CH_ENERGY_RSSI_Pos 0UL
#define BLE_BLESS_LL_PKT_RSSI_CH_ENERGY_RSSI_Msk 0xFFFFUL
#define BLE_BLESS_LL_PKT_RSSI_CH_ENERGY_RX_CHANNEL_Pos 16UL
#define BLE_BLESS_LL_PKT_RSSI_CH_ENERGY_RX_CHANNEL_Msk 0x3F0000UL
#define BLE_BLESS_LL_PKT_RSSI_CH_ENERGY_PKT_RSSI_OR_CH_ENERGY_Pos 22UL
#define BLE_BLESS_LL_PKT_RSSI_CH_ENERGY_PKT_RSSI_OR_CH_ENERGY_Msk 0x400000UL
/* BLE_BLESS.BT_CLOCK_CAPT */
#define BLE_BLESS_BT_CLOCK_CAPT_BT_CLOCK_Pos    0UL
#define BLE_BLESS_BT_CLOCK_CAPT_BT_CLOCK_Msk    0xFFFFUL
/* BLE_BLESS.MT_CFG */
#define BLE_BLESS_MT_CFG_ENABLE_BLERD_Pos       0UL
#define BLE_BLESS_MT_CFG_ENABLE_BLERD_Msk       0x1UL
#define BLE_BLESS_MT_CFG_DEEPSLEEP_EXIT_CFG_Pos 1UL
#define BLE_BLESS_MT_CFG_DEEPSLEEP_EXIT_CFG_Msk 0x2UL
#define BLE_BLESS_MT_CFG_DEEPSLEEP_EXITED_Pos   2UL
#define BLE_BLESS_MT_CFG_DEEPSLEEP_EXITED_Msk   0x4UL
#define BLE_BLESS_MT_CFG_ACT_LDO_NOT_BUCK_Pos   3UL
#define BLE_BLESS_MT_CFG_ACT_LDO_NOT_BUCK_Msk   0x8UL
#define BLE_BLESS_MT_CFG_OVERRIDE_HVLDO_BYPASS_Pos 4UL
#define BLE_BLESS_MT_CFG_OVERRIDE_HVLDO_BYPASS_Msk 0x10UL
#define BLE_BLESS_MT_CFG_HVLDO_BYPASS_Pos       5UL
#define BLE_BLESS_MT_CFG_HVLDO_BYPASS_Msk       0x20UL
#define BLE_BLESS_MT_CFG_OVERRIDE_ACT_REGULATOR_Pos 6UL
#define BLE_BLESS_MT_CFG_OVERRIDE_ACT_REGULATOR_Msk 0x40UL
#define BLE_BLESS_MT_CFG_ACT_REGULATOR_EN_Pos   7UL
#define BLE_BLESS_MT_CFG_ACT_REGULATOR_EN_Msk   0x80UL
#define BLE_BLESS_MT_CFG_OVERRIDE_DIG_REGULATOR_Pos 8UL
#define BLE_BLESS_MT_CFG_OVERRIDE_DIG_REGULATOR_Msk 0x100UL
#define BLE_BLESS_MT_CFG_DIG_REGULATOR_EN_Pos   9UL
#define BLE_BLESS_MT_CFG_DIG_REGULATOR_EN_Msk   0x200UL
#define BLE_BLESS_MT_CFG_OVERRIDE_RET_SWITCH_Pos 10UL
#define BLE_BLESS_MT_CFG_OVERRIDE_RET_SWITCH_Msk 0x400UL
#define BLE_BLESS_MT_CFG_RET_SWITCH_Pos         11UL
#define BLE_BLESS_MT_CFG_RET_SWITCH_Msk         0x800UL
#define BLE_BLESS_MT_CFG_OVERRIDE_ISOLATE_Pos   12UL
#define BLE_BLESS_MT_CFG_OVERRIDE_ISOLATE_Msk   0x1000UL
#define BLE_BLESS_MT_CFG_ISOLATE_N_Pos          13UL
#define BLE_BLESS_MT_CFG_ISOLATE_N_Msk          0x2000UL
#define BLE_BLESS_MT_CFG_OVERRIDE_LL_CLK_EN_Pos 14UL
#define BLE_BLESS_MT_CFG_OVERRIDE_LL_CLK_EN_Msk 0x4000UL
#define BLE_BLESS_MT_CFG_LL_CLK_EN_Pos          15UL
#define BLE_BLESS_MT_CFG_LL_CLK_EN_Msk          0x8000UL
#define BLE_BLESS_MT_CFG_OVERRIDE_HVLDO_EN_Pos  16UL
#define BLE_BLESS_MT_CFG_OVERRIDE_HVLDO_EN_Msk  0x10000UL
#define BLE_BLESS_MT_CFG_HVLDO_EN_Pos           17UL
#define BLE_BLESS_MT_CFG_HVLDO_EN_Msk           0x20000UL
#define BLE_BLESS_MT_CFG_DPSLP_ECO_ON_Pos       18UL
#define BLE_BLESS_MT_CFG_DPSLP_ECO_ON_Msk       0x40000UL
#define BLE_BLESS_MT_CFG_OVERRIDE_RESET_N_Pos   19UL
#define BLE_BLESS_MT_CFG_OVERRIDE_RESET_N_Msk   0x80000UL
#define BLE_BLESS_MT_CFG_RESET_N_Pos            20UL
#define BLE_BLESS_MT_CFG_RESET_N_Msk            0x100000UL
#define BLE_BLESS_MT_CFG_OVERRIDE_XTAL_EN_Pos   21UL
#define BLE_BLESS_MT_CFG_OVERRIDE_XTAL_EN_Msk   0x200000UL
#define BLE_BLESS_MT_CFG_XTAL_EN_Pos            22UL
#define BLE_BLESS_MT_CFG_XTAL_EN_Msk            0x400000UL
#define BLE_BLESS_MT_CFG_OVERRIDE_CLK_EN_Pos    23UL
#define BLE_BLESS_MT_CFG_OVERRIDE_CLK_EN_Msk    0x800000UL
#define BLE_BLESS_MT_CFG_BLERD_CLK_EN_Pos       24UL
#define BLE_BLESS_MT_CFG_BLERD_CLK_EN_Msk       0x1000000UL
#define BLE_BLESS_MT_CFG_OVERRIDE_RET_LDO_OL_Pos 25UL
#define BLE_BLESS_MT_CFG_OVERRIDE_RET_LDO_OL_Msk 0x2000000UL
#define BLE_BLESS_MT_CFG_RET_LDO_OL_Pos         26UL
#define BLE_BLESS_MT_CFG_RET_LDO_OL_Msk         0x4000000UL
#define BLE_BLESS_MT_CFG_HVLDO_POR_HV_Pos       27UL
#define BLE_BLESS_MT_CFG_HVLDO_POR_HV_Msk       0x8000000UL
/* BLE_BLESS.MT_DELAY_CFG */
#define BLE_BLESS_MT_DELAY_CFG_HVLDO_STARTUP_DELAY_Pos 0UL
#define BLE_BLESS_MT_DELAY_CFG_HVLDO_STARTUP_DELAY_Msk 0xFFUL
#define BLE_BLESS_MT_DELAY_CFG_ISOLATE_DEASSERT_DELAY_Pos 8UL
#define BLE_BLESS_MT_DELAY_CFG_ISOLATE_DEASSERT_DELAY_Msk 0xFF00UL
#define BLE_BLESS_MT_DELAY_CFG_ACT_TO_SWITCH_DELAY_Pos 16UL
#define BLE_BLESS_MT_DELAY_CFG_ACT_TO_SWITCH_DELAY_Msk 0xFF0000UL
#define BLE_BLESS_MT_DELAY_CFG_HVLDO_DISABLE_DELAY_Pos 24UL
#define BLE_BLESS_MT_DELAY_CFG_HVLDO_DISABLE_DELAY_Msk 0xFF000000UL
/* BLE_BLESS.MT_DELAY_CFG2 */
#define BLE_BLESS_MT_DELAY_CFG2_OSC_STARTUP_DELAY_LF_Pos 0UL
#define BLE_BLESS_MT_DELAY_CFG2_OSC_STARTUP_DELAY_LF_Msk 0xFFUL
#define BLE_BLESS_MT_DELAY_CFG2_DSM_OFFSET_TO_WAKEUP_INSTANT_LF_Pos 8UL
#define BLE_BLESS_MT_DELAY_CFG2_DSM_OFFSET_TO_WAKEUP_INSTANT_LF_Msk 0xFF00UL
#define BLE_BLESS_MT_DELAY_CFG2_ACT_STARTUP_DELAY_Pos 16UL
#define BLE_BLESS_MT_DELAY_CFG2_ACT_STARTUP_DELAY_Msk 0xFF0000UL
#define BLE_BLESS_MT_DELAY_CFG2_DIG_LDO_STARTUP_DELAY_Pos 24UL
#define BLE_BLESS_MT_DELAY_CFG2_DIG_LDO_STARTUP_DELAY_Msk 0xFF000000UL
/* BLE_BLESS.MT_DELAY_CFG3 */
#define BLE_BLESS_MT_DELAY_CFG3_XTAL_DISABLE_DELAY_Pos 0UL
#define BLE_BLESS_MT_DELAY_CFG3_XTAL_DISABLE_DELAY_Msk 0xFFUL
#define BLE_BLESS_MT_DELAY_CFG3_DIG_LDO_DISABLE_DELAY_Pos 8UL
#define BLE_BLESS_MT_DELAY_CFG3_DIG_LDO_DISABLE_DELAY_Msk 0xFF00UL
#define BLE_BLESS_MT_DELAY_CFG3_VDDR_STABLE_DELAY_Pos 16UL
#define BLE_BLESS_MT_DELAY_CFG3_VDDR_STABLE_DELAY_Msk 0xFF0000UL
/* BLE_BLESS.MT_VIO_CTRL */
#define BLE_BLESS_MT_VIO_CTRL_SRSS_SWITCH_EN_Pos 0UL
#define BLE_BLESS_MT_VIO_CTRL_SRSS_SWITCH_EN_Msk 0x1UL
#define BLE_BLESS_MT_VIO_CTRL_SRSS_SWITCH_EN_DLY_Pos 1UL
#define BLE_BLESS_MT_VIO_CTRL_SRSS_SWITCH_EN_DLY_Msk 0x2UL
/* BLE_BLESS.MT_STATUS */
#define BLE_BLESS_MT_STATUS_BLESS_STATE_Pos     0UL
#define BLE_BLESS_MT_STATUS_BLESS_STATE_Msk     0x1UL
#define BLE_BLESS_MT_STATUS_MT_CURR_STATE_Pos   1UL
#define BLE_BLESS_MT_STATUS_MT_CURR_STATE_Msk   0x1EUL
#define BLE_BLESS_MT_STATUS_HVLDO_STARTUP_CURR_STATE_Pos 5UL
#define BLE_BLESS_MT_STATUS_HVLDO_STARTUP_CURR_STATE_Msk 0xE0UL
#define BLE_BLESS_MT_STATUS_LL_CLK_STATE_Pos    8UL
#define BLE_BLESS_MT_STATUS_LL_CLK_STATE_Msk    0x100UL
/* BLE_BLESS.PWR_CTRL_SM_ST */
#define BLE_BLESS_PWR_CTRL_SM_ST_PWR_CTRL_SM_CURR_STATE_Pos 0UL
#define BLE_BLESS_PWR_CTRL_SM_ST_PWR_CTRL_SM_CURR_STATE_Msk 0xFUL
/* BLE_BLESS.HVLDO_CTRL */
#define BLE_BLESS_HVLDO_CTRL_ADFT_EN_Pos        0UL
#define BLE_BLESS_HVLDO_CTRL_ADFT_EN_Msk        0x1UL
#define BLE_BLESS_HVLDO_CTRL_ADFT_CTRL_Pos      1UL
#define BLE_BLESS_HVLDO_CTRL_ADFT_CTRL_Msk      0x1EUL
#define BLE_BLESS_HVLDO_CTRL_VREF_EXT_EN_Pos    6UL
#define BLE_BLESS_HVLDO_CTRL_VREF_EXT_EN_Msk    0x40UL
#define BLE_BLESS_HVLDO_CTRL_STATUS_Pos         31UL
#define BLE_BLESS_HVLDO_CTRL_STATUS_Msk         0x80000000UL
/* BLE_BLESS.MISC_EN_CTRL */
#define BLE_BLESS_MISC_EN_CTRL_BUCK_EN_CTRL_Pos 0UL
#define BLE_BLESS_MISC_EN_CTRL_BUCK_EN_CTRL_Msk 0x1UL
#define BLE_BLESS_MISC_EN_CTRL_ACT_REG_EN_CTRL_Pos 1UL
#define BLE_BLESS_MISC_EN_CTRL_ACT_REG_EN_CTRL_Msk 0x2UL
#define BLE_BLESS_MISC_EN_CTRL_LPM_DRIFT_EN_Pos 2UL
#define BLE_BLESS_MISC_EN_CTRL_LPM_DRIFT_EN_Msk 0x4UL
#define BLE_BLESS_MISC_EN_CTRL_LPM_DRIFT_MULTI_Pos 3UL
#define BLE_BLESS_MISC_EN_CTRL_LPM_DRIFT_MULTI_Msk 0x8UL
#define BLE_BLESS_MISC_EN_CTRL_LPM_ENTRY_CTRL_MODE_Pos 4UL
#define BLE_BLESS_MISC_EN_CTRL_LPM_ENTRY_CTRL_MODE_Msk 0x10UL
/* BLE_BLESS.EFUSE_CONFIG */
#define BLE_BLESS_EFUSE_CONFIG_EFUSE_MODE_Pos   0UL
#define BLE_BLESS_EFUSE_CONFIG_EFUSE_MODE_Msk   0x1UL
#define BLE_BLESS_EFUSE_CONFIG_EFUSE_READ_Pos   1UL
#define BLE_BLESS_EFUSE_CONFIG_EFUSE_READ_Msk   0x2UL
#define BLE_BLESS_EFUSE_CONFIG_EFUSE_WRITE_Pos  2UL
#define BLE_BLESS_EFUSE_CONFIG_EFUSE_WRITE_Msk  0x4UL
/* BLE_BLESS.EFUSE_TIM_CTRL1 */
#define BLE_BLESS_EFUSE_TIM_CTRL1_SCLK_HIGH_Pos 0UL
#define BLE_BLESS_EFUSE_TIM_CTRL1_SCLK_HIGH_Msk 0xFFUL
#define BLE_BLESS_EFUSE_TIM_CTRL1_SCLK_LOW_Pos  8UL
#define BLE_BLESS_EFUSE_TIM_CTRL1_SCLK_LOW_Msk  0xFF00UL
#define BLE_BLESS_EFUSE_TIM_CTRL1_CS_SCLK_SETUP_TIME_Pos 16UL
#define BLE_BLESS_EFUSE_TIM_CTRL1_CS_SCLK_SETUP_TIME_Msk 0xF0000UL
#define BLE_BLESS_EFUSE_TIM_CTRL1_CS_SCLK_HOLD_TIME_Pos 20UL
#define BLE_BLESS_EFUSE_TIM_CTRL1_CS_SCLK_HOLD_TIME_Msk 0xF00000UL
#define BLE_BLESS_EFUSE_TIM_CTRL1_RW_CS_SETUP_TIME_Pos 24UL
#define BLE_BLESS_EFUSE_TIM_CTRL1_RW_CS_SETUP_TIME_Msk 0xF000000UL
#define BLE_BLESS_EFUSE_TIM_CTRL1_RW_CS_HOLD_TIME_Pos 28UL
#define BLE_BLESS_EFUSE_TIM_CTRL1_RW_CS_HOLD_TIME_Msk 0xF0000000UL
/* BLE_BLESS.EFUSE_TIM_CTRL2 */
#define BLE_BLESS_EFUSE_TIM_CTRL2_DATA_SAMPLE_TIME_Pos 0UL
#define BLE_BLESS_EFUSE_TIM_CTRL2_DATA_SAMPLE_TIME_Msk 0xFFUL
#define BLE_BLESS_EFUSE_TIM_CTRL2_DOUT_CS_HOLD_TIME_Pos 8UL
#define BLE_BLESS_EFUSE_TIM_CTRL2_DOUT_CS_HOLD_TIME_Msk 0xF00UL
/* BLE_BLESS.EFUSE_TIM_CTRL3 */
#define BLE_BLESS_EFUSE_TIM_CTRL3_PGM_SCLK_SETUP_TIME_Pos 0UL
#define BLE_BLESS_EFUSE_TIM_CTRL3_PGM_SCLK_SETUP_TIME_Msk 0xFUL
#define BLE_BLESS_EFUSE_TIM_CTRL3_PGM_SCLK_HOLD_TIME_Pos 4UL
#define BLE_BLESS_EFUSE_TIM_CTRL3_PGM_SCLK_HOLD_TIME_Msk 0xF0UL
#define BLE_BLESS_EFUSE_TIM_CTRL3_AVDD_CS_SETUP_TIME_Pos 8UL
#define BLE_BLESS_EFUSE_TIM_CTRL3_AVDD_CS_SETUP_TIME_Msk 0xFF00UL
#define BLE_BLESS_EFUSE_TIM_CTRL3_AVDD_CS_HOLD_TIME_Pos 16UL
#define BLE_BLESS_EFUSE_TIM_CTRL3_AVDD_CS_HOLD_TIME_Msk 0xFF0000UL
/* BLE_BLESS.EFUSE_RDATA_L */
#define BLE_BLESS_EFUSE_RDATA_L_DATA_Pos        0UL
#define BLE_BLESS_EFUSE_RDATA_L_DATA_Msk        0xFFFFFFFFUL
/* BLE_BLESS.EFUSE_RDATA_H */
#define BLE_BLESS_EFUSE_RDATA_H_DATA_Pos        0UL
#define BLE_BLESS_EFUSE_RDATA_H_DATA_Msk        0xFFFFFFFFUL
/* BLE_BLESS.EFUSE_WDATA_L */
#define BLE_BLESS_EFUSE_WDATA_L_DATA_Pos        0UL
#define BLE_BLESS_EFUSE_WDATA_L_DATA_Msk        0xFFFFFFFFUL
/* BLE_BLESS.EFUSE_WDATA_H */
#define BLE_BLESS_EFUSE_WDATA_H_DATA_Pos        0UL
#define BLE_BLESS_EFUSE_WDATA_H_DATA_Msk        0xFFFFFFFFUL
/* BLE_BLESS.DIV_BY_625_CFG */
#define BLE_BLESS_DIV_BY_625_CFG_ENABLE_Pos     1UL
#define BLE_BLESS_DIV_BY_625_CFG_ENABLE_Msk     0x2UL
#define BLE_BLESS_DIV_BY_625_CFG_DIVIDEND_Pos   8UL
#define BLE_BLESS_DIV_BY_625_CFG_DIVIDEND_Msk   0xFFFF00UL
/* BLE_BLESS.DIV_BY_625_STS */
#define BLE_BLESS_DIV_BY_625_STS_QUOTIENT_Pos   0UL
#define BLE_BLESS_DIV_BY_625_STS_QUOTIENT_Msk   0x3FUL
#define BLE_BLESS_DIV_BY_625_STS_REMAINDER_Pos  8UL
#define BLE_BLESS_DIV_BY_625_STS_REMAINDER_Msk  0x3FF00UL
/* BLE_BLESS.PACKET_COUNTER0 */
#define BLE_BLESS_PACKET_COUNTER0_PACKET_COUNTER_LOWER_Pos 0UL
#define BLE_BLESS_PACKET_COUNTER0_PACKET_COUNTER_LOWER_Msk 0xFFFFFFFFUL
/* BLE_BLESS.PACKET_COUNTER2 */
#define BLE_BLESS_PACKET_COUNTER2_PACKET_COUNTER_UPPER_Pos 0UL
#define BLE_BLESS_PACKET_COUNTER2_PACKET_COUNTER_UPPER_Msk 0xFFUL
/* BLE_BLESS.IV_MASTER0 */
#define BLE_BLESS_IV_MASTER0_IV_MASTER_Pos      0UL
#define BLE_BLESS_IV_MASTER0_IV_MASTER_Msk      0xFFFFFFFFUL
/* BLE_BLESS.IV_SLAVE0 */
#define BLE_BLESS_IV_SLAVE0_IV_SLAVE_Pos        0UL
#define BLE_BLESS_IV_SLAVE0_IV_SLAVE_Msk        0xFFFFFFFFUL
/* BLE_BLESS.ENC_KEY */
#define BLE_BLESS_ENC_KEY_ENC_KEY_Pos           0UL
#define BLE_BLESS_ENC_KEY_ENC_KEY_Msk           0xFFFFFFFFUL
/* BLE_BLESS.MIC_IN0 */
#define BLE_BLESS_MIC_IN0_MIC_IN_Pos            0UL
#define BLE_BLESS_MIC_IN0_MIC_IN_Msk            0xFFFFFFFFUL
/* BLE_BLESS.MIC_OUT0 */
#define BLE_BLESS_MIC_OUT0_MIC_OUT_Pos          0UL
#define BLE_BLESS_MIC_OUT0_MIC_OUT_Msk          0xFFFFFFFFUL
/* BLE_BLESS.ENC_PARAMS */
#define BLE_BLESS_ENC_PARAMS_DATA_PDU_HEADER_Pos 0UL
#define BLE_BLESS_ENC_PARAMS_DATA_PDU_HEADER_Msk 0x3UL
#define BLE_BLESS_ENC_PARAMS_PAYLOAD_LENGTH_LSB_Pos 2UL
#define BLE_BLESS_ENC_PARAMS_PAYLOAD_LENGTH_LSB_Msk 0x7CUL
#define BLE_BLESS_ENC_PARAMS_DIRECTION_Pos      7UL
#define BLE_BLESS_ENC_PARAMS_DIRECTION_Msk      0x80UL
#define BLE_BLESS_ENC_PARAMS_PAYLOAD_LENGTH_LSB_EXT_Pos 8UL
#define BLE_BLESS_ENC_PARAMS_PAYLOAD_LENGTH_LSB_EXT_Msk 0x700UL
#define BLE_BLESS_ENC_PARAMS_MEM_LATENCY_HIDE_Pos 11UL
#define BLE_BLESS_ENC_PARAMS_MEM_LATENCY_HIDE_Msk 0x800UL
/* BLE_BLESS.ENC_CONFIG */
#define BLE_BLESS_ENC_CONFIG_START_PROC_Pos     0UL
#define BLE_BLESS_ENC_CONFIG_START_PROC_Msk     0x1UL
#define BLE_BLESS_ENC_CONFIG_ECB_CCM_Pos        1UL
#define BLE_BLESS_ENC_CONFIG_ECB_CCM_Msk        0x2UL
#define BLE_BLESS_ENC_CONFIG_DEC_ENC_Pos        2UL
#define BLE_BLESS_ENC_CONFIG_DEC_ENC_Msk        0x4UL
#define BLE_BLESS_ENC_CONFIG_PAYLOAD_LENGTH_MSB_Pos 8UL
#define BLE_BLESS_ENC_CONFIG_PAYLOAD_LENGTH_MSB_Msk 0xFF00UL
#define BLE_BLESS_ENC_CONFIG_B0_FLAGS_Pos       16UL
#define BLE_BLESS_ENC_CONFIG_B0_FLAGS_Msk       0xFF0000UL
#define BLE_BLESS_ENC_CONFIG_AES_B0_DATA_OVERRIDE_Pos 24UL
#define BLE_BLESS_ENC_CONFIG_AES_B0_DATA_OVERRIDE_Msk 0x1000000UL
/* BLE_BLESS.ENC_INTR_EN */
#define BLE_BLESS_ENC_INTR_EN_AUTH_PASS_INTR_EN_Pos 0UL
#define BLE_BLESS_ENC_INTR_EN_AUTH_PASS_INTR_EN_Msk 0x1UL
#define BLE_BLESS_ENC_INTR_EN_ECB_PROC_INTR_EN_Pos 1UL
#define BLE_BLESS_ENC_INTR_EN_ECB_PROC_INTR_EN_Msk 0x2UL
#define BLE_BLESS_ENC_INTR_EN_CCM_PROC_INTR_EN_Pos 2UL
#define BLE_BLESS_ENC_INTR_EN_CCM_PROC_INTR_EN_Msk 0x4UL
/* BLE_BLESS.ENC_INTR */
#define BLE_BLESS_ENC_INTR_AUTH_PASS_INTR_Pos   0UL
#define BLE_BLESS_ENC_INTR_AUTH_PASS_INTR_Msk   0x1UL
#define BLE_BLESS_ENC_INTR_ECB_PROC_INTR_Pos    1UL
#define BLE_BLESS_ENC_INTR_ECB_PROC_INTR_Msk    0x2UL
#define BLE_BLESS_ENC_INTR_CCM_PROC_INTR_Pos    2UL
#define BLE_BLESS_ENC_INTR_CCM_PROC_INTR_Msk    0x4UL
#define BLE_BLESS_ENC_INTR_IN_DATA_CLEAR_Pos    3UL
#define BLE_BLESS_ENC_INTR_IN_DATA_CLEAR_Msk    0x8UL
/* BLE_BLESS.B1_DATA_REG */
#define BLE_BLESS_B1_DATA_REG_B1_DATA_Pos       0UL
#define BLE_BLESS_B1_DATA_REG_B1_DATA_Msk       0xFFFFFFFFUL
/* BLE_BLESS.ENC_MEM_BASE_ADDR */
#define BLE_BLESS_ENC_MEM_BASE_ADDR_ENC_MEM_Pos 0UL
#define BLE_BLESS_ENC_MEM_BASE_ADDR_ENC_MEM_Msk 0xFFFFFFFFUL
/* BLE_BLESS.TRIM_LDO_0 */
#define BLE_BLESS_TRIM_LDO_0_ACT_LDO_VREG_Pos   0UL
#define BLE_BLESS_TRIM_LDO_0_ACT_LDO_VREG_Msk   0xFUL
#define BLE_BLESS_TRIM_LDO_0_ACT_LDO_ITAIL_Pos  4UL
#define BLE_BLESS_TRIM_LDO_0_ACT_LDO_ITAIL_Msk  0xF0UL
/* BLE_BLESS.TRIM_LDO_1 */
#define BLE_BLESS_TRIM_LDO_1_ACT_REF_BGR_Pos    0UL
#define BLE_BLESS_TRIM_LDO_1_ACT_REF_BGR_Msk    0xFUL
#define BLE_BLESS_TRIM_LDO_1_SB_BGRES_Pos       4UL
#define BLE_BLESS_TRIM_LDO_1_SB_BGRES_Msk       0xF0UL
/* BLE_BLESS.TRIM_LDO_2 */
#define BLE_BLESS_TRIM_LDO_2_SB_BMULT_RES_Pos   0UL
#define BLE_BLESS_TRIM_LDO_2_SB_BMULT_RES_Msk   0x1FUL
#define BLE_BLESS_TRIM_LDO_2_SB_BMULT_NBIAS_Pos 5UL
#define BLE_BLESS_TRIM_LDO_2_SB_BMULT_NBIAS_Msk 0x60UL
/* BLE_BLESS.TRIM_LDO_3 */
#define BLE_BLESS_TRIM_LDO_3_LVDET_Pos          0UL
#define BLE_BLESS_TRIM_LDO_3_LVDET_Msk          0x1FUL
#define BLE_BLESS_TRIM_LDO_3_SLOPE_SB_BMULT_Pos 5UL
#define BLE_BLESS_TRIM_LDO_3_SLOPE_SB_BMULT_Msk 0x60UL
/* BLE_BLESS.TRIM_MXD */
#define BLE_BLESS_TRIM_MXD_MXD_TRIM_BITS_Pos    0UL
#define BLE_BLESS_TRIM_MXD_MXD_TRIM_BITS_Msk    0xFFUL
/* BLE_BLESS.TRIM_LDO_4 */
#define BLE_BLESS_TRIM_LDO_4_T_LDO_Pos          0UL
#define BLE_BLESS_TRIM_LDO_4_T_LDO_Msk          0xFFUL
/* BLE_BLESS.TRIM_LDO_5 */
#define BLE_BLESS_TRIM_LDO_5_RESERVED_Pos       0UL
#define BLE_BLESS_TRIM_LDO_5_RESERVED_Msk       0xFFUL


#endif /* _CYIP_BLE_H_ */


/* [] END OF FILE */
