/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
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
 * o Neither the name of the copyright holder nor the names of its
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

#include "fsl_enet.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief IPv4 PTP message IP version offset. */
#define ENET_PTP1588_IPVERSION_OFFSET 0x0EU
/*! @brief IPv4 PTP message UDP protocol offset. */
#define ENET_PTP1588_IPV4_UDP_PROTOCOL_OFFSET 0x17U
/*! @brief IPv4 PTP message UDP port offset. */
#define ENET_PTP1588_IPV4_UDP_PORT_OFFSET 0x24U
/*! @brief IPv4 PTP message UDP message type offset. */
#define ENET_PTP1588_IPV4_UDP_MSGTYPE_OFFSET 0x2AU
/*! @brief IPv4 PTP message UDP version offset. */
#define ENET_PTP1588_IPV4_UDP_VERSION_OFFSET 0x2BU
/*! @brief IPv4 PTP message UDP clock id offset. */
#define ENET_PTP1588_IPV4_UDP_CLKID_OFFSET 0x3EU
/*! @brief IPv4 PTP message UDP sequence id offset. */
#define ENET_PTP1588_IPV4_UDP_SEQUENCEID_OFFSET 0x48U
/*! @brief IPv4 PTP message UDP control offset. */
#define ENET_PTP1588_IPV4_UDP_CTL_OFFSET 0x4AU
/*! @brief IPv6 PTP message UDP protocol offset. */
#define ENET_PTP1588_IPV6_UDP_PROTOCOL_OFFSET 0x14U
/*! @brief IPv6 PTP message UDP port offset. */
#define ENET_PTP1588_IPV6_UDP_PORT_OFFSET 0x38U
/*! @brief IPv6 PTP message UDP message type offset. */
#define ENET_PTP1588_IPV6_UDP_MSGTYPE_OFFSET 0x3EU
/*! @brief IPv6 PTP message UDP version offset. */
#define ENET_PTP1588_IPV6_UDP_VERSION_OFFSET 0x3FU
/*! @brief IPv6 PTP message UDP clock id offset. */
#define ENET_PTP1588_IPV6_UDP_CLKID_OFFSET 0x52U
/*! @brief IPv6 PTP message UDP sequence id offset. */
#define ENET_PTP1588_IPV6_UDP_SEQUENCEID_OFFSET 0x5CU
/*! @brief IPv6 PTP message UDP control offset. */
#define ENET_PTP1588_IPV6_UDP_CTL_OFFSET 0x5EU
/*! @brief PTPv2 message Ethernet packet type offset. */
#define ENET_PTP1588_ETHL2_PACKETTYPE_OFFSET 0x0CU
/*! @brief PTPv2 message Ethernet message type offset. */
#define ENET_PTP1588_ETHL2_MSGTYPE_OFFSET 0x0EU
/*! @brief PTPv2 message Ethernet version type offset. */
#define ENET_PTP1588_ETHL2_VERSION_OFFSET 0X0FU
/*! @brief PTPv2 message Ethernet clock id offset. */
#define ENET_PTP1588_ETHL2_CLOCKID_OFFSET 0x22
/*! @brief PTPv2 message Ethernet sequence id offset. */
#define ENET_PTP1588_ETHL2_SEQUENCEID_OFFSET 0x2c
/*! @brief Packet type Ethernet IEEE802.3 for PTPv2. */
#define ENET_ETHERNETL2 0x88F7U
/*! @brief Packet type IPv4. */
#define ENET_IPV4 0x0800U
/*! @brief Packet type IPv6. */
#define ENET_IPV6 0x86ddU
/*! @brief Packet type VLAN. */
#define ENET_8021QVLAN 0x8100U
/*! @brief UDP protocol type. */
#define ENET_UDPVERSION 0x0011U
/*! @brief Packet IP version IPv4. */
#define ENET_IPV4VERSION 0x0004U
/*! @brief Packet IP version IPv6. */
#define ENET_IPV6VERSION 0x0006U
/*! @brief Ethernet mac address length. */
#define ENET_FRAME_MACLEN 6U
/*! @brief Ethernet VLAN header length. */
#define ENET_FRAME_VLAN_TAGLEN 4U
/*! @brief MDC frequency. */
#define ENET_MDC_FREQUENCY 2500000U
/*! @brief NanoSecond in one second. */
#define ENET_NANOSECOND_ONE_SECOND 1000000000U
/*! @brief Define a common clock cycle delays used for time stamp capture. */
#define ENET_1588TIME_DELAY_COUNT 10U
/*! @brief Defines the macro for converting constants from host byte order to network byte order. */
#define ENET_HTONS(n) __REV16(n)
#define ENET_HTONL(n) __REV(n)
#define ENET_NTOHS(n) __REV16(n)
#define ENET_NTOHL(n) __REV(n)

/* Typedef for interrupt handler. */
typedef void (*enet_isr_t)(ENET_Type *base, enet_handle_t *handle);
/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief Get the ENET instance from peripheral base address.
 *
 * @param base ENET peripheral base address.
 * @return ENET instance.
 */
uint32_t ENET_GetInstance(ENET_Type *base);

/*!
 * @brief Set ENET MAC controller with the configuration.
 *
 * @param base ENET peripheral base address.
 * @param config ENET Mac configuration.
 * @param bufferConfig ENET buffer configuration.
 * @param macAddr ENET six-byte mac address.
 * @param srcClock_Hz ENET module clock source, normally it's system clock.
 */
static void ENET_SetMacController(ENET_Type *base,
                                  const enet_config_t *config,
                                  const enet_buffer_config_t *bufferConfig,
                                  uint8_t *macAddr,
                                  uint32_t srcClock_Hz);
/*!
 * @brief Set ENET handler.
 *
 * @param base ENET peripheral base address.
 * @param handle The ENET handle pointer.
 * @param config ENET configuration stucture pointer.
 * @param bufferConfig ENET buffer configuration.
 */
static void ENET_SetHandler(ENET_Type *base,
                            enet_handle_t *handle,
                            const enet_config_t *config,
                            const enet_buffer_config_t *bufferConfig);
/*!
 * @brief Set ENET MAC transmit buffer descriptors.
 *
 * @param txBdStartAlign The aligned start address of ENET transmit buffer descriptors.
 *        is recommended to evenly divisible by 16.
 * @param txBuffStartAlign The aligned start address of ENET transmit buffers, must be evenly divisible by 16.
 * @param txBuffSizeAlign The aligned ENET transmit buffer size, must be evenly divisible by 16.
 * @param txBdNumber The number of ENET transmit buffers.
 */
static void ENET_SetTxBufferDescriptors(volatile enet_tx_bd_struct_t *txBdStartAlign,
                                        uint8_t *txBuffStartAlign,
                                        uint32_t txBuffSizeAlign,
                                        uint32_t txBdNumber);

/*!
 * @brief Set ENET MAC receive buffer descriptors.
 *
 * @param rxBdStartAlign The aligned start address of ENET receive buffer descriptors.
 *        is recommended to evenly divisible by 16.
 * @param rxBuffStartAlign The aligned start address of ENET receive buffers, must be evenly divisible by 16.
 * @param rxBuffSizeAlign The aligned ENET receive buffer size, must be evenly divisible by 16.
 * @param rxBdNumber The number of ENET receive buffers.
 * @param enableInterrupt Enable/disables to generate the receive byte and frame interrupt.
 *        It's used for ENET_ENHANCEDBUFFERDESCRIPTOR_MODE enabled case.
 */
static void ENET_SetRxBufferDescriptors(volatile enet_rx_bd_struct_t *rxBdStartAlign,
                                        uint8_t *rxBuffStartAlign,
                                        uint32_t rxBuffSizeAlign,
                                        uint32_t rxBdNumber,
                                        bool enableInterrupt);

/*!
 * @brief Updates the ENET read buffer descriptors.
 *
 * @param base ENET peripheral base address.
 * @param handle The ENET handle pointer.
 */
static void ENET_UpdateReadBuffers(ENET_Type *base, enet_handle_t *handle);

#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
/*!
 * @brief Parses the ENET frame for time-stamp process of PTP 1588 frame.
 *
 * @param data  The ENET read data for frame parse.
 * @param ptpTsData The ENET PTP message and time-stamp data pointer.
 * @param isFastEnabled The fast parse flag.
 *        - true , Fast processing, only check if this is a PTP message.
 *        - false, Store the PTP message data after check the PTP message.
 */
static bool ENET_Ptp1588ParseFrame(uint8_t *data, enet_ptp_time_data_t *ptpTsData, bool isFastEnabled);

/*!
 * @brief Updates the new PTP 1588 time-stamp to the time-stamp buffer ring.
 *
 * @param ptpTsDataRing The PTP message and time-stamp data ring pointer.
 * @param ptpTimeData   The new PTP 1588 time-stamp data pointer.
 */
static status_t ENET_Ptp1588UpdateTimeRing(enet_ptp_time_data_ring_t *ptpTsDataRing, enet_ptp_time_data_t *ptpTimeData);

/*!
 * @brief Search up the right PTP 1588 time-stamp from the time-stamp buffer ring.
 *
 * @param ptpTsDataRing The PTP message and time-stamp data ring pointer.
 * @param ptpTimeData   The find out right PTP 1588 time-stamp data pointer with the specific PTP message.
 */
static status_t ENET_Ptp1588SearchTimeRing(enet_ptp_time_data_ring_t *ptpTsDataRing, enet_ptp_time_data_t *ptpTimedata);

/*!
 * @brief Store the transmit time-stamp for event PTP frame in the time-stamp buffer ring.
 *
 * @param base   ENET peripheral base address.
 * @param handle The ENET handle pointer.
 */
static status_t ENET_StoreTxFrameTime(ENET_Type *base, enet_handle_t *handle);

/*!
 * @brief Store the receive time-stamp for event PTP frame in the time-stamp buffer ring.
 *
 * @param base   ENET peripheral base address.
 * @param handle The ENET handle pointer.
 * @param ptpTimeData The PTP 1588 time-stamp data pointer.
 */
static status_t ENET_StoreRxFrameTime(ENET_Type *base, enet_handle_t *handle, enet_ptp_time_data_t *ptpTimeData);
#endif /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*! @brief Pointers to enet handles for each instance. */
static enet_handle_t *s_ENETHandle[FSL_FEATURE_SOC_ENET_COUNT] = {NULL};

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to enet clocks for each instance. */
const clock_ip_name_t s_enetClock[] = ENET_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*! @brief Pointers to enet transmit IRQ number for each instance. */
static const IRQn_Type s_enetTxIrqId[] = ENET_Transmit_IRQS;
/*! @brief Pointers to enet receive IRQ number for each instance. */
static const IRQn_Type s_enetRxIrqId[] = ENET_Receive_IRQS;
#if defined(ENET_ENHANCEDBUFFERDESCRIPTOR_MODE) && ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
/*! @brief Pointers to enet timestamp IRQ number for each instance. */
static const IRQn_Type s_enetTsIrqId[] = ENET_1588_Timer_IRQS;
#endif /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */
/*! @brief Pointers to enet error IRQ number for each instance. */
static const IRQn_Type s_enetErrIrqId[] = ENET_Error_IRQS;

/*! @brief Pointers to enet bases for each instance. */
static ENET_Type *const s_enetBases[] = ENET_BASE_PTRS;

/* ENET ISR for transactional APIs. */
static enet_isr_t s_enetTxIsr;
static enet_isr_t s_enetRxIsr;
static enet_isr_t s_enetErrIsr;
static enet_isr_t s_enetTsIsr;
/*******************************************************************************
 * Code
 ******************************************************************************/

uint32_t ENET_GetInstance(ENET_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_enetBases); instance++)
    {
        if (s_enetBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_enetBases));

    return instance;
}

void ENET_GetDefaultConfig(enet_config_t *config)
{
    /* Checks input parameter. */
    assert(config);

    /* Initializes the MAC configure structure to zero. */
    memset(config, 0, sizeof(enet_config_t));

    /* Sets MII mode, full duplex, 100Mbps for MAC and PHY data interface. */
    config->miiMode = kENET_RmiiMode;
    config->miiSpeed = kENET_MiiSpeed100M;
    config->miiDuplex = kENET_MiiFullDuplex;

    /* Sets the maximum receive frame length. */
    config->rxMaxFrameLen = ENET_FRAME_MAX_FRAMELEN;
}

void ENET_Init(ENET_Type *base,
               enet_handle_t *handle,
               const enet_config_t *config,
               const enet_buffer_config_t *bufferConfig,
               uint8_t *macAddr,
               uint32_t srcClock_Hz)
{
    /* Checks input parameters. */
    assert(handle);
    assert(config);
    assert(bufferConfig);
    assert(bufferConfig->rxBdStartAddrAlign);
    assert(bufferConfig->txBdStartAddrAlign);
    assert(bufferConfig->rxBufferAlign);
    assert(bufferConfig->txBufferAlign);
    assert(macAddr);
    assert(bufferConfig->rxBuffSizeAlign >= ENET_RX_MIN_BUFFERSIZE);

    /* Make sure the buffers should be have the capability of process at least one maximum frame. */
    if (config->macSpecialConfig & kENET_ControlVLANTagEnable)
    {
        assert(bufferConfig->txBuffSizeAlign * bufferConfig->txBdNumber > (ENET_FRAME_MAX_FRAMELEN + ENET_FRAME_VLAN_TAGLEN));
    }
    else
    {
        assert(bufferConfig->txBuffSizeAlign * bufferConfig->txBdNumber > ENET_FRAME_MAX_FRAMELEN);
        assert(bufferConfig->rxBuffSizeAlign * bufferConfig->rxBdNumber > config->rxMaxFrameLen);
    }

    uint32_t instance = ENET_GetInstance(base);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Ungate ENET clock. */
    CLOCK_EnableClock(s_enetClock[instance]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* Reset ENET module. */
    ENET_Reset(base);

    /* Initializes the ENET transmit buffer descriptors. */
    ENET_SetTxBufferDescriptors(bufferConfig->txBdStartAddrAlign, bufferConfig->txBufferAlign,
                                bufferConfig->txBuffSizeAlign, bufferConfig->txBdNumber);

    /* Initializes the ENET receive buffer descriptors. */
    ENET_SetRxBufferDescriptors(bufferConfig->rxBdStartAddrAlign, bufferConfig->rxBufferAlign,
                                bufferConfig->rxBuffSizeAlign, bufferConfig->rxBdNumber,
                                !!(config->interrupt & (kENET_RxFrameInterrupt | kENET_RxBufferInterrupt)));

    /* Initializes the ENET MAC controller. */
    ENET_SetMacController(base, config, bufferConfig, macAddr, srcClock_Hz);

    /* Set all buffers or data in handler for data transmit/receive process. */
    ENET_SetHandler(base, handle, config, bufferConfig);
}

void ENET_Deinit(ENET_Type *base)
{
    /* Disable interrupt. */
    base->EIMR = 0;

    /* Disable ENET. */
    base->ECR &= ~ENET_ECR_ETHEREN_MASK;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Disables the clock source. */
    CLOCK_DisableClock(s_enetClock[ENET_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void ENET_SetCallback(enet_handle_t *handle, enet_callback_t callback, void *userData)
{
    assert(handle);

    /* Set callback and userData. */
    handle->callback = callback;
    handle->userData = userData;
}

static void ENET_SetHandler(ENET_Type *base,
                            enet_handle_t *handle,
                            const enet_config_t *config,
                            const enet_buffer_config_t *bufferConfig)
{
    uint32_t instance = ENET_GetInstance(base);

    memset(handle, 0, sizeof(enet_handle_t));

    handle->rxBdBase = bufferConfig->rxBdStartAddrAlign;
    handle->rxBdCurrent = bufferConfig->rxBdStartAddrAlign;
    handle->txBdBase = bufferConfig->txBdStartAddrAlign;
    handle->txBdCurrent = bufferConfig->txBdStartAddrAlign;
    handle->rxBuffSizeAlign = bufferConfig->rxBuffSizeAlign;
    handle->txBuffSizeAlign = bufferConfig->txBuffSizeAlign;

    /* Save the handle pointer in the global variables. */
    s_ENETHandle[instance] = handle;

    /* Set the IRQ handler when the interrupt is enabled. */
    if (config->interrupt & ENET_TX_INTERRUPT)
    {
        s_enetTxIsr = ENET_TransmitIRQHandler;
        EnableIRQ(s_enetTxIrqId[instance]);
    }
    if (config->interrupt & ENET_RX_INTERRUPT)
    {
        s_enetRxIsr = ENET_ReceiveIRQHandler;
        EnableIRQ(s_enetRxIrqId[instance]);
    }
    if (config->interrupt & ENET_ERR_INTERRUPT)
    {
        s_enetErrIsr = ENET_ErrorIRQHandler;
        EnableIRQ(s_enetErrIrqId[instance]);
    }
}

static void ENET_SetMacController(ENET_Type *base,
                                  const enet_config_t *config,
                                  const enet_buffer_config_t *bufferConfig,
                                  uint8_t *macAddr,
                                  uint32_t srcClock_Hz)
{
    uint32_t rcr = 0;
    uint32_t tcr = 0;
    uint32_t ecr = 0;
    uint32_t macSpecialConfig = config->macSpecialConfig;
    uint32_t maxFrameLen = config->rxMaxFrameLen;

    /* Maximum frame length check. */
    if ((macSpecialConfig & kENET_ControlVLANTagEnable) && (maxFrameLen <= ENET_FRAME_MAX_FRAMELEN))
    {
        maxFrameLen = (ENET_FRAME_MAX_FRAMELEN + ENET_FRAME_VLAN_TAGLEN);
    }

    /* Configures MAC receive controller with user configure structure. */
    rcr = ENET_RCR_NLC(!!(macSpecialConfig & kENET_ControlRxPayloadCheckEnable)) |
          ENET_RCR_CFEN(!!(macSpecialConfig & kENET_ControlFlowControlEnable)) |
          ENET_RCR_FCE(!!(macSpecialConfig & kENET_ControlFlowControlEnable)) |
          ENET_RCR_PADEN(!!(macSpecialConfig & kENET_ControlRxPadRemoveEnable)) |
          ENET_RCR_BC_REJ(!!(macSpecialConfig & kENET_ControlRxBroadCastRejectEnable)) |
          ENET_RCR_PROM(!!(macSpecialConfig & kENET_ControlPromiscuousEnable)) | ENET_RCR_MII_MODE(1) |
          ENET_RCR_RMII_MODE(config->miiMode) | ENET_RCR_RMII_10T(!config->miiSpeed) |
          ENET_RCR_MAX_FL(maxFrameLen) | ENET_RCR_CRCFWD(1);
    /* Receive setting for half duplex. */
    if (config->miiDuplex == kENET_MiiHalfDuplex)
    {
        rcr |= ENET_RCR_DRT_MASK;
    }
    /* Sets internal loop only for MII mode. */
    if ((config->macSpecialConfig & kENET_ControlMIILoopEnable) && (config->miiMode == kENET_MiiMode))
    {
        rcr |= ENET_RCR_LOOP_MASK;
        rcr &= ~ENET_RCR_DRT_MASK;
    }
    base->RCR = rcr;

    /* Configures MAC transmit controller: duplex mode, mac address insertion. */
    tcr = base->TCR & ~(ENET_TCR_FDEN_MASK | ENET_TCR_ADDINS_MASK);
    tcr |= ENET_TCR_FDEN(config->miiDuplex) | ENET_TCR_ADDINS(!!(macSpecialConfig & kENET_ControlMacAddrInsert));
    base->TCR = tcr;

    /* Configures receive and transmit accelerator. */
    base->TACC = config->txAccelerConfig;
    base->RACC = config->rxAccelerConfig;

    /* Sets the pause duration and FIFO threshold for the flow control enabled case. */
    if (macSpecialConfig & kENET_ControlFlowControlEnable)
    {
        uint32_t reemReg;
        base->OPD = config->pauseDuration;
        reemReg = ENET_RSEM_RX_SECTION_EMPTY(config->rxFifoEmptyThreshold);
#if defined (FSL_FEATURE_ENET_HAS_RECEIVE_STATUS_THRESHOLD) && FSL_FEATURE_ENET_HAS_RECEIVE_STATUS_THRESHOLD
        reemReg |= ENET_RSEM_STAT_SECTION_EMPTY(config->rxFifoStatEmptyThreshold);
#endif /* FSL_FEATURE_ENET_HAS_RECEIVE_STATUS_THRESHOLD */
        base->RSEM = reemReg;
    }

    /* FIFO threshold setting for store and forward enable/disable case. */
    if (macSpecialConfig & kENET_ControlStoreAndFwdDisable)
    {
        /* Transmit fifo watermark settings. */
        base->TFWR = config->txFifoWatermark & ENET_TFWR_TFWR_MASK;
        /* Receive fifo full threshold settings. */
        base->RSFL = config->rxFifoFullThreshold & ENET_RSFL_RX_SECTION_FULL_MASK;
    }
    else
    {
        /* Transmit fifo watermark settings. */
        base->TFWR = ENET_TFWR_STRFWD_MASK;
        base->RSFL = 0;
    }

    /* Enable store and forward when accelerator is enabled */
    if (config->txAccelerConfig & (kENET_TxAccelIpCheckEnabled | kENET_TxAccelProtoCheckEnabled))
    {
        base->TFWR = ENET_TFWR_STRFWD_MASK;
    }
    if (config->rxAccelerConfig & (kENET_RxAccelIpCheckEnabled | kENET_RxAccelProtoCheckEnabled))
    {
        base->RSFL = 0;
    }

    /* Initializes transmit buffer descriptor rings start address, two start address should be aligned. */
    base->TDSR = (uint32_t)bufferConfig->txBdStartAddrAlign;
    base->RDSR = (uint32_t)bufferConfig->rxBdStartAddrAlign;
    /* Initializes the maximum buffer size, the buffer size should be aligned. */
    base->MRBR = bufferConfig->rxBuffSizeAlign;

    /* Configures the Mac address. */
    ENET_SetMacAddr(base, macAddr);

    /* Initialize the SMI if uninitialized. */
    if (!ENET_GetSMI(base))
    {
        ENET_SetSMI(base, srcClock_Hz, !!(config->macSpecialConfig & kENET_ControlSMIPreambleDisable));
    }

/* Enables Ethernet interrupt and NVIC. */
#if defined(FSL_FEATURE_ENET_HAS_INTERRUPT_COALESCE) && FSL_FEATURE_ENET_HAS_INTERRUPT_COALESCE
    if (config->intCoalesceCfg)
    {
        uint32_t intMask = (ENET_EIMR_TXB_MASK | ENET_EIMR_RXB_MASK);

        /* Clear all buffer interrupts. */
        base->EIMR &= ~intMask;

        /* Set the interrupt coalescence. */
        base->TXIC = ENET_TXIC_ICFT(config->intCoalesceCfg->txCoalesceFrameCount[0]) |
                     config->intCoalesceCfg->txCoalesceTimeCount[0] | ENET_TXIC_ICCS_MASK | ENET_TXIC_ICEN_MASK;
        base->RXIC = ENET_RXIC_ICFT(config->intCoalesceCfg->rxCoalesceFrameCount[0]) |
                     config->intCoalesceCfg->rxCoalesceTimeCount[0] | ENET_RXIC_ICCS_MASK | ENET_RXIC_ICEN_MASK;
    }
#endif /* FSL_FEATURE_ENET_HAS_INTERRUPT_COALESCE */
    ENET_EnableInterrupts(base, config->interrupt);

    /* ENET control register setting. */
    ecr = base->ECR;
#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
    /* Sets the 1588 enhanced feature. */
    ecr |= ENET_ECR_EN1588_MASK;
#endif /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */
    /* Enables Ethernet module after all configuration except the buffer descriptor active. */
    ecr |= ENET_ECR_ETHEREN_MASK | ENET_ECR_DBSWP_MASK;
    base->ECR = ecr;
}

static void ENET_SetTxBufferDescriptors(volatile enet_tx_bd_struct_t *txBdStartAlign,
                                        uint8_t *txBuffStartAlign,
                                        uint32_t txBuffSizeAlign,
                                        uint32_t txBdNumber)
{
    assert(txBdStartAlign);
    assert(txBuffStartAlign);

    uint32_t count;
    volatile enet_tx_bd_struct_t *curBuffDescrip = txBdStartAlign;

    for (count = 0; count < txBdNumber; count++)
    {
        /* Set data buffer address. */
        curBuffDescrip->buffer = (uint8_t *)((uint32_t)&txBuffStartAlign[count * txBuffSizeAlign]);
        /* Initializes data length. */
        curBuffDescrip->length = 0;
        /* Sets the crc. */
        curBuffDescrip->control = ENET_BUFFDESCRIPTOR_TX_TRANMITCRC_MASK;
        /* Sets the last buffer descriptor with the wrap flag. */
        if (count == txBdNumber - 1)
        {
            curBuffDescrip->control |= ENET_BUFFDESCRIPTOR_TX_WRAP_MASK;
        }

#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
        /* Enable transmit interrupt for store the transmit timestamp. */
        curBuffDescrip->controlExtend1 |= ENET_BUFFDESCRIPTOR_TX_INTERRUPT_MASK;
#endif /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */
        /* Increase the index. */
        curBuffDescrip++;
    }
}

static void ENET_SetRxBufferDescriptors(volatile enet_rx_bd_struct_t *rxBdStartAlign,
                                        uint8_t *rxBuffStartAlign,
                                        uint32_t rxBuffSizeAlign,
                                        uint32_t rxBdNumber,
                                        bool enableInterrupt)
{
    assert(rxBdStartAlign);
    assert(rxBuffStartAlign);

    volatile enet_rx_bd_struct_t *curBuffDescrip = rxBdStartAlign;
    uint32_t count = 0;

    /* Initializes receive buffer descriptors. */
    for (count = 0; count < rxBdNumber; count++)
    {
        /* Set data buffer and the length. */
        curBuffDescrip->buffer = (uint8_t *)((uint32_t)&rxBuffStartAlign[count * rxBuffSizeAlign]);
        curBuffDescrip->length = 0;

        /* Initializes the buffer descriptors with empty bit. */
        curBuffDescrip->control = ENET_BUFFDESCRIPTOR_RX_EMPTY_MASK;
        /* Sets the last buffer descriptor with the wrap flag. */
        if (count == rxBdNumber - 1)
        {
            curBuffDescrip->control |= ENET_BUFFDESCRIPTOR_RX_WRAP_MASK;
        }

#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
        if (enableInterrupt)
        {
            /* Enable receive interrupt. */
            curBuffDescrip->controlExtend1 |= ENET_BUFFDESCRIPTOR_RX_INTERRUPT_MASK;
        }
        else
        {
            curBuffDescrip->controlExtend1 = 0;
        }
#endif /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */
        /* Increase the index. */
        curBuffDescrip++;
    }
}

void ENET_SetMII(ENET_Type *base, enet_mii_speed_t speed, enet_mii_duplex_t duplex)
{
    uint32_t rcr = base->RCR;
    uint32_t tcr = base->TCR;
    /* Sets speed mode. */
    if (kENET_MiiSpeed10M == speed)
    {
        rcr |= ENET_RCR_RMII_10T_MASK;
    }
    else
    {
        rcr &= ~ENET_RCR_RMII_10T_MASK;
    }
    /* Set duplex mode. */
    if (duplex == kENET_MiiHalfDuplex)
    {
        rcr |= ENET_RCR_DRT_MASK;
        tcr &= ~ENET_TCR_FDEN_MASK;
    }
    else
    {
        rcr &= ~ENET_RCR_DRT_MASK;
        tcr |= ENET_TCR_FDEN_MASK;
    }

    base->RCR = rcr;
    base->TCR = tcr;
}

void ENET_SetMacAddr(ENET_Type *base, uint8_t *macAddr)
{
    uint32_t address;

    /* Set physical address lower register. */
    address = (uint32_t)(((uint32_t)macAddr[0] << 24U) | ((uint32_t)macAddr[1] << 16U) | ((uint32_t)macAddr[2] << 8U) |
                         (uint32_t)macAddr[3]);
    base->PALR = address;
    /* Set physical address high register. */
    address = (uint32_t)(((uint32_t)macAddr[4] << 8U) | ((uint32_t)macAddr[5]));
    base->PAUR = address << ENET_PAUR_PADDR2_SHIFT;
}

void ENET_GetMacAddr(ENET_Type *base, uint8_t *macAddr)
{
    assert(macAddr);

    uint32_t address;

    /* Get from physical address lower register. */
    address = base->PALR;
    macAddr[0] = 0xFFU & (address >> 24U);
    macAddr[1] = 0xFFU & (address >> 16U);
    macAddr[2] = 0xFFU & (address >> 8U);
    macAddr[3] = 0xFFU & address;

    /* Get from physical address high register. */
    address = (base->PAUR & ENET_PAUR_PADDR2_MASK) >> ENET_PAUR_PADDR2_SHIFT;
    macAddr[4] = 0xFFU & (address >> 8U);
    macAddr[5] = 0xFFU & address;
}

void ENET_SetSMI(ENET_Type *base, uint32_t srcClock_Hz, bool isPreambleDisabled)
{
    assert(srcClock_Hz);

    uint32_t clkCycle = 0;
    uint32_t speed = 0;
    uint32_t mscr = 0;

    /* Calculate the MII speed which controls the frequency of the MDC. */
    speed = srcClock_Hz / (2 * ENET_MDC_FREQUENCY);
    /* Calculate the hold time on the MDIO output. */
    clkCycle = (10 + ENET_NANOSECOND_ONE_SECOND / srcClock_Hz - 1) / (ENET_NANOSECOND_ONE_SECOND / srcClock_Hz) - 1;
    /* Build the configuration for MDC/MDIO control. */
    mscr = ENET_MSCR_MII_SPEED(speed) | ENET_MSCR_DIS_PRE(isPreambleDisabled) | ENET_MSCR_HOLDTIME(clkCycle);
    base->MSCR = mscr;
}

void ENET_StartSMIWrite(ENET_Type *base, uint32_t phyAddr, uint32_t phyReg, enet_mii_write_t operation, uint32_t data)
{
    uint32_t mmfr = 0;

    /* Build MII write command. */
    mmfr = ENET_MMFR_ST(1) | ENET_MMFR_OP(operation) | ENET_MMFR_PA(phyAddr) | ENET_MMFR_RA(phyReg) | ENET_MMFR_TA(2) |
           (data & 0xFFFF);
    base->MMFR = mmfr;
}

void ENET_StartSMIRead(ENET_Type *base, uint32_t phyAddr, uint32_t phyReg, enet_mii_read_t operation)
{
    uint32_t mmfr = 0;

    /* Build MII read command. */
    mmfr = ENET_MMFR_ST(1) | ENET_MMFR_OP(operation) | ENET_MMFR_PA(phyAddr) | ENET_MMFR_RA(phyReg) | ENET_MMFR_TA(2);
    base->MMFR = mmfr;
}

#if defined(FSL_FEATURE_ENET_HAS_EXTEND_MDIO) && FSL_FEATURE_ENET_HAS_EXTEND_MDIO
void ENET_StartExtC45SMIWrite(ENET_Type *base, uint32_t phyAddr, uint32_t phyReg, uint32_t data)
{
    uint32_t mmfr = 0;

    /* Parse the address from the input register. */
    uint16_t devAddr = (phyReg >> ENET_MMFR_TA_SHIFT) & 0x1FU;
    uint16_t regAddr = (uint16_t)(phyReg & 0xFFFFU);

    /* Address write firstly. */
    mmfr = ENET_MMFR_ST(0) | ENET_MMFR_OP(kENET_MiiAddrWrite_C45) | ENET_MMFR_PA(phyAddr) | ENET_MMFR_RA(devAddr) |
           ENET_MMFR_TA(2) | ENET_MMFR_DATA(regAddr);
    base->MMFR = mmfr;

    /* Build MII write command. */
    mmfr = ENET_MMFR_ST(0) | ENET_MMFR_OP(kENET_MiiWriteFrame_C45) | ENET_MMFR_PA(phyAddr) | ENET_MMFR_RA(devAddr) |
           ENET_MMFR_TA(2) | ENET_MMFR_DATA(data);
    base->MMFR = mmfr;
}

void ENET_StartExtC45SMIRead(ENET_Type *base, uint32_t phyAddr, uint32_t phyReg)
{
    uint32_t mmfr = 0;

    /* Parse the address from the input register. */
    uint16_t devAddr = (phyReg >> ENET_MMFR_TA_SHIFT) & 0x1FU;
    uint16_t regAddr = (uint16_t)(phyReg & 0xFFFFU);

    /* Address write firstly. */
    mmfr = ENET_MMFR_ST(0) | ENET_MMFR_OP(kENET_MiiAddrWrite_C45) | ENET_MMFR_PA(phyAddr) | ENET_MMFR_RA(devAddr) |
           ENET_MMFR_TA(2) | ENET_MMFR_DATA(regAddr);
    base->MMFR = mmfr;

    /* Build MII read command. */
    mmfr = ENET_MMFR_ST(0) | ENET_MMFR_OP(kENET_MiiReadFrame_C45) | ENET_MMFR_PA(phyAddr) | ENET_MMFR_RA(devAddr) |
           ENET_MMFR_TA(2);
    base->MMFR = mmfr;
}
#endif /* FSL_FEATURE_ENET_HAS_EXTEND_MDIO */

void ENET_GetRxErrBeforeReadFrame(enet_handle_t *handle, enet_data_error_stats_t *eErrorStatic)
{
    assert(handle);
    assert(handle->rxBdCurrent);
    assert(eErrorStatic);

    uint16_t control = 0;
    volatile enet_rx_bd_struct_t *curBuffDescrip = handle->rxBdCurrent;

    do
    {
        /* The last buffer descriptor of a frame. */
        if (curBuffDescrip->control & ENET_BUFFDESCRIPTOR_RX_LAST_MASK)
        {
            control = curBuffDescrip->control;
            if (control & ENET_BUFFDESCRIPTOR_RX_TRUNC_MASK)
            {
                /* The receive truncate error. */
                eErrorStatic->statsRxTruncateErr++;
            }
            if (control & ENET_BUFFDESCRIPTOR_RX_OVERRUN_MASK)
            {
                /* The receive over run error. */
                eErrorStatic->statsRxOverRunErr++;
            }
            if (control & ENET_BUFFDESCRIPTOR_RX_LENVLIOLATE_MASK)
            {
                /* The receive length violation error. */
                eErrorStatic->statsRxLenGreaterErr++;
            }
            if (control & ENET_BUFFDESCRIPTOR_RX_NOOCTET_MASK)
            {
                /* The receive alignment error. */
                eErrorStatic->statsRxAlignErr++;
            }
            if (control & ENET_BUFFDESCRIPTOR_RX_CRC_MASK)
            {
                /* The receive CRC error. */
                eErrorStatic->statsRxFcsErr++;
            }
#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
            uint16_t controlExt = curBuffDescrip->controlExtend1;
            if (controlExt & ENET_BUFFDESCRIPTOR_RX_MACERR_MASK)
            {
                /* The MAC error. */
                eErrorStatic->statsRxMacErr++;
            }
            if (controlExt & ENET_BUFFDESCRIPTOR_RX_PHYERR_MASK)
            {
                /* The PHY error. */
                eErrorStatic->statsRxPhyErr++;
            }
            if (controlExt & ENET_BUFFDESCRIPTOR_RX_COLLISION_MASK)
            {
                /* The receive collision error. */
                eErrorStatic->statsRxCollisionErr++;
            }
#endif /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */

            break;
        }

        /* Increase the buffer descriptor, if it is the last one, increase to first one of the ring buffer. */
        if (curBuffDescrip->control & ENET_BUFFDESCRIPTOR_RX_WRAP_MASK)
        {
            curBuffDescrip = handle->rxBdBase;
        }
        else
        {
            curBuffDescrip++;
        }

    } while (curBuffDescrip != handle->rxBdCurrent);
}

status_t ENET_GetRxFrameSize(enet_handle_t *handle, uint32_t *length)
{
    assert(handle);
    assert(handle->rxBdCurrent);
    assert(length);

    /* Reset the length to zero. */
    *length = 0;

    uint16_t validLastMask = ENET_BUFFDESCRIPTOR_RX_LAST_MASK | ENET_BUFFDESCRIPTOR_RX_EMPTY_MASK;
    volatile enet_rx_bd_struct_t *curBuffDescrip = handle->rxBdCurrent;

    /* Check the current buffer descriptor's empty flag.  if empty means there is no frame received. */
    if (curBuffDescrip->control & ENET_BUFFDESCRIPTOR_RX_EMPTY_MASK)
    {
        return kStatus_ENET_RxFrameEmpty;
    }

    do
    {
        /* Find the last buffer descriptor. */
        if ((curBuffDescrip->control & validLastMask) == ENET_BUFFDESCRIPTOR_RX_LAST_MASK)
        {
            /* The last buffer descriptor in the frame check the status of the received frame. */
            if ((curBuffDescrip->control & ENET_BUFFDESCRIPTOR_RX_ERR_MASK)
#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
                || (curBuffDescrip->controlExtend1 & ENET_BUFFDESCRIPTOR_RX_EXT_ERR_MASK)
#endif /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */
                    )
            {
                return kStatus_ENET_RxFrameError;
            }
            /* FCS is removed by MAC. */
            *length = curBuffDescrip->length;
            return kStatus_Success;
        }
        /* Increase the buffer descriptor, if it is the last one, increase to first one of the ring buffer. */
        if (curBuffDescrip->control & ENET_BUFFDESCRIPTOR_RX_WRAP_MASK)
        {
            curBuffDescrip = handle->rxBdBase;
        }
        else
        {
            curBuffDescrip++;
        }

    } while (curBuffDescrip != handle->rxBdCurrent);

    /* The frame is on processing - set to empty status to make application to receive it next time. */
    return kStatus_ENET_RxFrameEmpty;
}

status_t ENET_ReadFrame(ENET_Type *base, enet_handle_t *handle, uint8_t *data, uint32_t length)
{
    assert(handle);
    assert(handle->rxBdCurrent);

    uint32_t len = 0;
    uint32_t offset = 0;
    uint16_t control;
    bool isLastBuff = false;
    volatile enet_rx_bd_struct_t *curBuffDescrip = handle->rxBdCurrent;
    status_t result = kStatus_Success;

    /* For data-NULL input, only update the buffer descriptor. */
    if (!data)
    {
        do
        {
            /* Update the control flag. */
            control = handle->rxBdCurrent->control;
            /* Updates the receive buffer descriptors. */
            ENET_UpdateReadBuffers(base, handle);

            /* Find the last buffer descriptor for the frame. */
            if (control & ENET_BUFFDESCRIPTOR_RX_LAST_MASK)
            {
                break;
            }

        } while (handle->rxBdCurrent != curBuffDescrip);

        return result;
    }
    else
    {
/* A frame on one buffer or several receive buffers are both considered. */
#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
        enet_ptp_time_data_t ptpTimestamp;
        bool isPtpEventMessage = false;

        /* Parse the PTP message according to the header message. */
        isPtpEventMessage = ENET_Ptp1588ParseFrame(curBuffDescrip->buffer, &ptpTimestamp, false);
#endif /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */

        while (!isLastBuff)
        {
            /* The last buffer descriptor of a frame. */
            if (curBuffDescrip->control & ENET_BUFFDESCRIPTOR_RX_LAST_MASK)
            {
                /* This is a valid frame. */
                isLastBuff = true;
                if (length == curBuffDescrip->length)
                {
                    /* Copy the frame to user's buffer without FCS. */
                    len = curBuffDescrip->length - offset;
                    memcpy(data + offset, curBuffDescrip->buffer, len);
#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
                    /* Store the PTP 1588 timestamp for received PTP event frame. */
                    if (isPtpEventMessage)
                    {
                        /* Set the timestamp to the timestamp ring. */
                        ptpTimestamp.timeStamp.nanosecond = curBuffDescrip->timestamp;
                        result = ENET_StoreRxFrameTime(base, handle, &ptpTimestamp);
                    }
#endif /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */

                    /* Updates the receive buffer descriptors. */
                    ENET_UpdateReadBuffers(base, handle);
                    return result;
                }
                else
                {
                    /* Updates the receive buffer descriptors. */
                    ENET_UpdateReadBuffers(base, handle);
                }
            }
            else
            {
                /* Store a frame on several buffer descriptors. */
                isLastBuff = false;
                /* Length check. */
                if (offset >= length)
                {
                    break;
                }

                memcpy(data + offset, curBuffDescrip->buffer, handle->rxBuffSizeAlign);
                offset += handle->rxBuffSizeAlign;

                /* Updates the receive buffer descriptors. */
                ENET_UpdateReadBuffers(base, handle);
            }

            /* Get the current buffer descriptor. */
            curBuffDescrip = handle->rxBdCurrent;
        }
    }

    return kStatus_ENET_RxFrameFail;
}

static void ENET_UpdateReadBuffers(ENET_Type *base, enet_handle_t *handle)
{
    assert(handle);

    /* Clears status. */
    handle->rxBdCurrent->control &= ENET_BUFFDESCRIPTOR_RX_WRAP_MASK;
    /* Sets the receive buffer descriptor with the empty flag. */
    handle->rxBdCurrent->control |= ENET_BUFFDESCRIPTOR_RX_EMPTY_MASK;

    /* Increase current buffer descriptor to the next one. */
    if (handle->rxBdCurrent->control & ENET_BUFFDESCRIPTOR_RX_WRAP_MASK)
    {
        handle->rxBdCurrent = handle->rxBdBase;
    }
    else
    {
        handle->rxBdCurrent++;
    }

    /* Actives the receive buffer descriptor. */
    base->RDAR = ENET_RDAR_RDAR_MASK;
}

status_t ENET_SendFrame(ENET_Type *base, enet_handle_t *handle, uint8_t *data, uint32_t length)
{
    assert(handle);
    assert(handle->txBdCurrent);
    assert(data);
    assert(length <= ENET_FRAME_MAX_FRAMELEN);

    volatile enet_tx_bd_struct_t *curBuffDescrip = handle->txBdCurrent;
    uint32_t len = 0;
    uint32_t sizeleft = 0;

    /* Check if the transmit buffer is ready. */
    if (curBuffDescrip->control & ENET_BUFFDESCRIPTOR_TX_READY_MASK)
    {
        return kStatus_ENET_TxFrameBusy;
    }
#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
    bool isPtpEventMessage = false;
    /* Check PTP message with the PTP header. */
    isPtpEventMessage = ENET_Ptp1588ParseFrame(data, NULL, true);
#endif /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */
    /* One transmit buffer is enough for one frame. */
    if (handle->txBuffSizeAlign >= length)
    {
        /* Copy data to the buffer for uDMA transfer. */
        memcpy(curBuffDescrip->buffer, data, length);
        /* Set data length. */
        curBuffDescrip->length = length;
#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
        /* For enable the timestamp. */
        if (isPtpEventMessage)
        {
            curBuffDescrip->controlExtend1 |= ENET_BUFFDESCRIPTOR_TX_TIMESTAMP_MASK;
        }
        else
        {
            curBuffDescrip->controlExtend1 &= ~ENET_BUFFDESCRIPTOR_TX_TIMESTAMP_MASK;
        }

#endif /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */
        curBuffDescrip->control |= (ENET_BUFFDESCRIPTOR_TX_READY_MASK | ENET_BUFFDESCRIPTOR_TX_LAST_MASK);

        /* Increase the buffer descriptor address. */
        if (curBuffDescrip->control & ENET_BUFFDESCRIPTOR_TX_WRAP_MASK)
        {
            handle->txBdCurrent = handle->txBdBase;
        }
        else
        {
            handle->txBdCurrent++;
        }

        /* Active the transmit buffer descriptor. */
        base->TDAR = ENET_TDAR_TDAR_MASK;
        return kStatus_Success;
    }
    else
    {
        /* One frame requires more than one transmit buffers. */
        do
        {
#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
            /* For enable the timestamp. */
            if (isPtpEventMessage)
            {
                curBuffDescrip->controlExtend1 |= ENET_BUFFDESCRIPTOR_TX_TIMESTAMP_MASK;
            }
            else
            {
                curBuffDescrip->controlExtend1 &= ~ENET_BUFFDESCRIPTOR_TX_TIMESTAMP_MASK;
            }
#endif /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */

            /* Increase the buffer descriptor address. */
            if (curBuffDescrip->control & ENET_BUFFDESCRIPTOR_TX_WRAP_MASK)
            {
                handle->txBdCurrent = handle->txBdBase;
            }
            else
            {
                handle->txBdCurrent++;
            }
            /* update the size left to be transmit. */
            sizeleft = length - len;
            if (sizeleft > handle->txBuffSizeAlign)
            {
                /* Data copy. */
                memcpy(curBuffDescrip->buffer, data + len, handle->txBuffSizeAlign);
                /* Data length update. */
                curBuffDescrip->length = handle->txBuffSizeAlign;
                len += handle->txBuffSizeAlign;
                /* Sets the control flag. */
                curBuffDescrip->control &= ~ENET_BUFFDESCRIPTOR_TX_LAST_MASK;
                curBuffDescrip->control |= ENET_BUFFDESCRIPTOR_TX_READY_MASK;
                /* Active the transmit buffer descriptor*/
                base->TDAR = ENET_TDAR_TDAR_MASK;
            }
            else
            {
                memcpy(curBuffDescrip->buffer, data + len, sizeleft);
                curBuffDescrip->length = sizeleft;
                /* Set Last buffer wrap flag. */
                curBuffDescrip->control |= ENET_BUFFDESCRIPTOR_TX_READY_MASK | ENET_BUFFDESCRIPTOR_TX_LAST_MASK;
                /* Active the transmit buffer descriptor. */
                base->TDAR = ENET_TDAR_TDAR_MASK;
                return kStatus_Success;
            }

            /* Get the current buffer descriptor address. */
            curBuffDescrip = handle->txBdCurrent;

        } while (!(curBuffDescrip->control & ENET_BUFFDESCRIPTOR_TX_READY_MASK));

        return kStatus_ENET_TxFrameBusy;
    }
}

void ENET_AddMulticastGroup(ENET_Type *base, uint8_t *address)
{
    assert(address);

    uint32_t crc = 0xFFFFFFFFU;
    uint32_t count1 = 0;
    uint32_t count2 = 0;

    /* Calculates the CRC-32 polynomial on the multicast group address. */
    for (count1 = 0; count1 < ENET_FRAME_MACLEN; count1++)
    {
        uint8_t c = address[count1];
        for (count2 = 0; count2 < 0x08U; count2++)
        {
            if ((c ^ crc) & 1U)
            {
                crc >>= 1U;
                c >>= 1U;
                crc ^= 0xEDB88320U;
            }
            else
            {
                crc >>= 1U;
                c >>= 1U;
            }
        }
    }

    /* Enable a multicast group address. */
    if (!((crc >> 0x1FU) & 1U))
    {
        base->GALR |= 1U << ((crc >> 0x1AU) & 0x1FU);
    }
    else
    {
        base->GAUR |= 1U << ((crc >> 0x1AU) & 0x1FU);
    }
}

void ENET_LeaveMulticastGroup(ENET_Type *base, uint8_t *address)
{
    assert(address);

    uint32_t crc = 0xFFFFFFFFU;
    uint32_t count1 = 0;
    uint32_t count2 = 0;

    /* Calculates the CRC-32 polynomial on the multicast group address. */
    for (count1 = 0; count1 < ENET_FRAME_MACLEN; count1++)
    {
        uint8_t c = address[count1];
        for (count2 = 0; count2 < 0x08U; count2++)
        {
            if ((c ^ crc) & 1U)
            {
                crc >>= 1U;
                c >>= 1U;
                crc ^= 0xEDB88320U;
            }
            else
            {
                crc >>= 1U;
                c >>= 1U;
            }
        }
    }

    /* Set the hash table. */
    if (!((crc >> 0x1FU) & 1U))
    {
        base->GALR &= ~(1U << ((crc >> 0x1AU) & 0x1FU));
    }
    else
    {
        base->GAUR &= ~(1U << ((crc >> 0x1AU) & 0x1FU));
    }
}

#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
status_t ENET_GetTxErrAfterSendFrame(enet_handle_t *handle, enet_data_error_stats_t *eErrorStatic)
{
    assert(handle);
    assert(eErrorStatic);

    uint16_t control = 0;
    uint16_t controlExt = 0;

    do
    {
        /* Get the current dirty transmit buffer descriptor. */
        control = handle->txBdDirtyStatic->control;
        controlExt = handle->txBdDirtyStatic->controlExtend0;
        /* Get the control status data, If the buffer descriptor has not been processed break out. */
        if (control & ENET_BUFFDESCRIPTOR_TX_READY_MASK)
        {
            return kStatus_ENET_TxFrameBusy;
        }
        /* Increase the transmit dirty static pointer. */
        if (handle->txBdDirtyStatic->control & ENET_BUFFDESCRIPTOR_TX_WRAP_MASK)
        {
            handle->txBdDirtyStatic = handle->txBdBase;
        }
        else
        {
            handle->txBdDirtyStatic++;
        }

        /* If the transmit buffer descriptor is ready and the last buffer descriptor, store packet statistic. */
        if (control & ENET_BUFFDESCRIPTOR_TX_LAST_MASK)
        {
            if (controlExt & ENET_BUFFDESCRIPTOR_TX_ERR_MASK)
            {
                /* Transmit error. */
                eErrorStatic->statsTxErr++;
            }
            if (controlExt & ENET_BUFFDESCRIPTOR_TX_EXCCOLLISIONERR_MASK)
            {
                /* Transmit excess collision error. */
                eErrorStatic->statsTxExcessCollisionErr++;
            }
            if (controlExt & ENET_BUFFDESCRIPTOR_TX_LATECOLLISIONERR_MASK)
            {
                /* Transmit late collision error. */
                eErrorStatic->statsTxLateCollisionErr++;
            }
            if (controlExt & ENET_BUFFDESCRIPTOR_TX_UNDERFLOWERR_MASK)
            {
                /* Transmit under flow error. */
                eErrorStatic->statsTxUnderFlowErr++;
            }
            if (controlExt & ENET_BUFFDESCRIPTOR_TX_OVERFLOWERR_MASK)
            {
                /* Transmit over flow error. */
                eErrorStatic->statsTxOverFlowErr++;
            }
            return kStatus_Success;
        }

    } while (handle->txBdDirtyStatic != handle->txBdCurrent);

    return kStatus_ENET_TxFrameFail;
}

static bool ENET_Ptp1588ParseFrame(uint8_t *data, enet_ptp_time_data_t *ptpTsData, bool isFastEnabled)
{
    assert(data);
    if (!isFastEnabled)
    {
        assert(ptpTsData);
    }

    bool isPtpMsg = false;
    uint8_t *buffer = data;
    uint16_t ptpType;

    /* Check for VLAN frame. */
    if (*(uint16_t *)(buffer + ENET_PTP1588_ETHL2_PACKETTYPE_OFFSET) == ENET_HTONS(ENET_8021QVLAN))
    {
        buffer += ENET_FRAME_VLAN_TAGLEN;
    }

    ptpType = *(uint16_t *)(buffer + ENET_PTP1588_ETHL2_PACKETTYPE_OFFSET);
    switch (ENET_HTONS(ptpType))
    { /* Ethernet layer 2. */
        case ENET_ETHERNETL2:
            if (*(uint8_t *)(buffer + ENET_PTP1588_ETHL2_MSGTYPE_OFFSET) <= kENET_PtpEventMsgType)
            {
                isPtpMsg = true;
                if (!isFastEnabled)
                {
                    /* It's a ptpv2 message and store the ptp header information. */
                    ptpTsData->version = (*(uint8_t *)(buffer + ENET_PTP1588_ETHL2_VERSION_OFFSET)) & 0x0F;
                    ptpTsData->messageType = (*(uint8_t *)(buffer + ENET_PTP1588_ETHL2_MSGTYPE_OFFSET)) & 0x0F;
                    ptpTsData->sequenceId = ENET_HTONS(*(uint16_t *)(buffer + ENET_PTP1588_ETHL2_SEQUENCEID_OFFSET));
                    memcpy((void *)&ptpTsData->sourcePortId[0], (void *)(buffer + ENET_PTP1588_ETHL2_CLOCKID_OFFSET),
                           kENET_PtpSrcPortIdLen);
                }
            }
            break;
        /* IPV4. */
        case ENET_IPV4:
            if ((*(uint8_t *)(buffer + ENET_PTP1588_IPVERSION_OFFSET) >> 4) == ENET_IPV4VERSION)
            {
                if (((*(uint16_t *)(buffer + ENET_PTP1588_IPV4_UDP_PORT_OFFSET)) == ENET_HTONS(kENET_PtpEventPort)) &&
                    (*(uint8_t *)(buffer + ENET_PTP1588_IPV4_UDP_PROTOCOL_OFFSET) == ENET_UDPVERSION))
                {
                    /* Set the PTP message flag. */
                    isPtpMsg = true;
                    if (!isFastEnabled)
                    {
                        /* It's a IPV4 ptp message and store the ptp header information. */
                        ptpTsData->version = (*(uint8_t *)(buffer + ENET_PTP1588_IPV4_UDP_VERSION_OFFSET)) & 0x0F;
                        ptpTsData->messageType = (*(uint8_t *)(buffer + ENET_PTP1588_IPV4_UDP_MSGTYPE_OFFSET)) & 0x0F;
                        ptpTsData->sequenceId =
                            ENET_HTONS(*(uint16_t *)(buffer + ENET_PTP1588_IPV4_UDP_SEQUENCEID_OFFSET));
                        memcpy((void *)&ptpTsData->sourcePortId[0],
                               (void *)(buffer + ENET_PTP1588_IPV4_UDP_CLKID_OFFSET), kENET_PtpSrcPortIdLen);
                    }
                }
            }
            break;
        /* IPV6. */
        case ENET_IPV6:
            if ((*(uint8_t *)(buffer + ENET_PTP1588_IPVERSION_OFFSET) >> 4) == ENET_IPV6VERSION)
            {
                if (((*(uint16_t *)(buffer + ENET_PTP1588_IPV6_UDP_PORT_OFFSET)) == ENET_HTONS(kENET_PtpEventPort)) &&
                    (*(uint8_t *)(buffer + ENET_PTP1588_IPV6_UDP_PROTOCOL_OFFSET) == ENET_UDPVERSION))
                {
                    /* Set the PTP message flag. */
                    isPtpMsg = true;
                    if (!isFastEnabled)
                    {
                        /* It's a IPV6 ptp message and store the ptp header information. */
                        ptpTsData->version = (*(uint8_t *)(buffer + ENET_PTP1588_IPV6_UDP_VERSION_OFFSET)) & 0x0F;
                        ptpTsData->messageType = (*(uint8_t *)(buffer + ENET_PTP1588_IPV6_UDP_MSGTYPE_OFFSET)) & 0x0F;
                        ptpTsData->sequenceId =
                            ENET_HTONS(*(uint16_t *)(buffer + ENET_PTP1588_IPV6_UDP_SEQUENCEID_OFFSET));
                        memcpy((void *)&ptpTsData->sourcePortId[0],
                               (void *)(buffer + ENET_PTP1588_IPV6_UDP_CLKID_OFFSET), kENET_PtpSrcPortIdLen);
                    }
                }
            }
            break;
        default:
            break;
    }
    return isPtpMsg;
}

void ENET_Ptp1588Configure(ENET_Type *base, enet_handle_t *handle, enet_ptp_config_t *ptpConfig)
{
    assert(handle);
    assert(ptpConfig);

    uint32_t instance = ENET_GetInstance(base);

    /* Start the 1588 timer. */
    ENET_Ptp1588StartTimer(base, ptpConfig->ptp1588ClockSrc_Hz);

    /* Enables the time stamp interrupt for the master clock on a device. */
    ENET_EnableInterrupts(base, kENET_TsTimerInterrupt);
    /* Enables only frame interrupt for transmit side to store the transmit
    frame time-stamp when the whole frame is transmitted out. */
    ENET_EnableInterrupts(base, kENET_TxFrameInterrupt);
    ENET_DisableInterrupts(base, kENET_TxBufferInterrupt);

    /* Setting the receive and transmit state for transaction. */
    handle->rxPtpTsDataRing.ptpTsData = ptpConfig->rxPtpTsData;
    handle->rxPtpTsDataRing.size = ptpConfig->ptpTsRxBuffNum;
    handle->rxPtpTsDataRing.front = 0;
    handle->rxPtpTsDataRing.end = 0;
    handle->txPtpTsDataRing.ptpTsData = ptpConfig->txPtpTsData;
    handle->txPtpTsDataRing.size = ptpConfig->ptpTsTxBuffNum;
    handle->txPtpTsDataRing.front = 0;
    handle->txPtpTsDataRing.end = 0;
    handle->msTimerSecond = 0;
    handle->txBdDirtyTime = handle->txBdBase;
    handle->txBdDirtyStatic = handle->txBdBase;

    /* Set the IRQ handler when the interrupt is enabled. */
    s_enetTxIsr = ENET_TransmitIRQHandler;
    s_enetTsIsr = ENET_Ptp1588TimerIRQHandler;
    EnableIRQ(s_enetTsIrqId[instance]);
    EnableIRQ(s_enetTxIrqId[instance]);
}

void ENET_Ptp1588StartTimer(ENET_Type *base, uint32_t ptpClkSrc)
{
    /* Restart PTP 1588 timer, master clock. */
    base->ATCR = ENET_ATCR_RESTART_MASK;

    /* Initializes PTP 1588 timer. */
    base->ATINC = ENET_ATINC_INC(ENET_NANOSECOND_ONE_SECOND / ptpClkSrc);
    base->ATPER = ENET_NANOSECOND_ONE_SECOND;
    /* Sets periodical event and the event signal output assertion and Actives PTP 1588 timer.  */
    base->ATCR = ENET_ATCR_PEREN_MASK | ENET_ATCR_PINPER_MASK | ENET_ATCR_EN_MASK;
}

void ENET_Ptp1588GetTimer(ENET_Type *base, enet_handle_t *handle, enet_ptp_time_t *ptpTime)
{
    assert(handle);
    assert(ptpTime);
    uint16_t count = ENET_1588TIME_DELAY_COUNT;
    uint32_t primask;

    /* Disables the interrupt. */
    primask = DisableGlobalIRQ();

    /* Get the current PTP time. */
    ptpTime->second = handle->msTimerSecond;
    /* Get the nanosecond from the master timer. */
    base->ATCR |= ENET_ATCR_CAPTURE_MASK;
    /* Add at least six clock cycle delay to get accurate time.
       It's the requirement when the 1588 clock source is slower
       than the register clock.
    */
    while (count--)
    {
        __NOP();
    }
    /* Get the captured time. */
    ptpTime->nanosecond = base->ATVR;

    /* Enables the interrupt. */
    EnableGlobalIRQ(primask);
}

void ENET_Ptp1588SetTimer(ENET_Type *base, enet_handle_t *handle, enet_ptp_time_t *ptpTime)
{
    assert(handle);
    assert(ptpTime);

    uint32_t primask;

    /* Disables the interrupt. */
    primask = DisableGlobalIRQ();

    /* Sets PTP timer. */
    handle->msTimerSecond = ptpTime->second;
    base->ATVR = ptpTime->nanosecond;

    /* Enables the interrupt. */
    EnableGlobalIRQ(primask);
}

void ENET_Ptp1588AdjustTimer(ENET_Type *base, uint32_t corrIncrease, uint32_t corrPeriod)
{
    /* Set correction for PTP timer increment. */
    base->ATINC = (base->ATINC & ~ENET_ATINC_INC_CORR_MASK) | (corrIncrease << ENET_ATINC_INC_CORR_SHIFT);
    /* Set correction for PTP timer period. */
    base->ATCOR = (base->ATCOR & ~ENET_ATCOR_COR_MASK) | (corrPeriod << ENET_ATCOR_COR_SHIFT);
}

static status_t ENET_Ptp1588UpdateTimeRing(enet_ptp_time_data_ring_t *ptpTsDataRing, enet_ptp_time_data_t *ptpTimeData)
{
    assert(ptpTsDataRing);
    assert(ptpTsDataRing->ptpTsData);
    assert(ptpTimeData);

    uint16_t usedBuffer = 0;

    /* Check if the buffers ring is full. */
    if (ptpTsDataRing->end >= ptpTsDataRing->front)
    {
        usedBuffer = ptpTsDataRing->end - ptpTsDataRing->front;
    }
    else
    {
        usedBuffer = ptpTsDataRing->size - (ptpTsDataRing->front - ptpTsDataRing->end);
    }

    if (usedBuffer == ptpTsDataRing->size)
    {
        return kStatus_ENET_PtpTsRingFull;
    }

    /* Copy the new data into the buffer. */
    memcpy((ptpTsDataRing->ptpTsData + ptpTsDataRing->end), ptpTimeData, sizeof(enet_ptp_time_data_t));

    /* Increase the buffer pointer to the next empty one. */
    ptpTsDataRing->end = (ptpTsDataRing->end + 1) % ptpTsDataRing->size;

    return kStatus_Success;
}

static status_t ENET_Ptp1588SearchTimeRing(enet_ptp_time_data_ring_t *ptpTsDataRing, enet_ptp_time_data_t *ptpTimedata)
{
    assert(ptpTsDataRing);
    assert(ptpTsDataRing->ptpTsData);
    assert(ptpTimedata);

    uint32_t index;
    uint32_t size;
    uint16_t usedBuffer = 0;

    /* Check the PTP 1588 timestamp ring. */
    if (ptpTsDataRing->front == ptpTsDataRing->end)
    {
        return kStatus_ENET_PtpTsRingEmpty;
    }

    /* Search the element in the ring buffer */
    index = ptpTsDataRing->front;
    size = ptpTsDataRing->size;
    while (index != ptpTsDataRing->end)
    {
        if (((ptpTsDataRing->ptpTsData + index)->sequenceId == ptpTimedata->sequenceId) &&
            (!memcmp(((void *)&(ptpTsDataRing->ptpTsData + index)->sourcePortId[0]),
                     (void *)&ptpTimedata->sourcePortId[0], kENET_PtpSrcPortIdLen)) &&
            ((ptpTsDataRing->ptpTsData + index)->version == ptpTimedata->version) &&
            ((ptpTsDataRing->ptpTsData + index)->messageType == ptpTimedata->messageType))
        {
            break;
        }

        /* Increase the ptp ring index. */
        index = (index + 1) % size;
    }

    if (index == ptpTsDataRing->end)
    {
        /* Check if buffers is full. */
        if (ptpTsDataRing->end >= ptpTsDataRing->front)
        {
            usedBuffer = ptpTsDataRing->end - ptpTsDataRing->front;
        }
        else
        {
            usedBuffer = ptpTsDataRing->size - (ptpTsDataRing->front - ptpTsDataRing->end);
        }

        if (usedBuffer == ptpTsDataRing->size)
        { /* Drop one in the front. */
            ptpTsDataRing->front = (ptpTsDataRing->front + 1) % size;
        }
        return kStatus_ENET_PtpTsRingFull;
    }

    /* Get the right timestamp of the required ptp messag. */
    ptpTimedata->timeStamp.second = (ptpTsDataRing->ptpTsData + index)->timeStamp.second;
    ptpTimedata->timeStamp.nanosecond = (ptpTsDataRing->ptpTsData + index)->timeStamp.nanosecond;

    /* Increase the index. */
    ptpTsDataRing->front = (ptpTsDataRing->front + 1) % size;

    return kStatus_Success;
}

static status_t ENET_StoreRxFrameTime(ENET_Type *base, enet_handle_t *handle, enet_ptp_time_data_t *ptpTimeData)
{
    assert(handle);
    assert(ptpTimeData);

    bool ptpTimerWrap = false;
    enet_ptp_time_t ptpTimer;
    uint32_t primask;

    /* Disables the interrupt. */
    primask = DisableGlobalIRQ();

    /* Get current PTP timer nanosecond value. */
    ENET_Ptp1588GetTimer(base, handle, &ptpTimer);

    /* Get PTP timer wrap event. */
    ptpTimerWrap = base->EIR & kENET_TsTimerInterrupt;

    /* Get transmit time stamp second. */
    if ((ptpTimer.nanosecond > ptpTimeData->timeStamp.nanosecond) ||
        ((ptpTimer.nanosecond < ptpTimeData->timeStamp.nanosecond) && ptpTimerWrap))
    {
        ptpTimeData->timeStamp.second = handle->msTimerSecond;
    }
    else
    {
        ptpTimeData->timeStamp.second = handle->msTimerSecond - 1;
    }
    /* Enable the interrupt. */
    EnableGlobalIRQ(primask);

    /* Store the timestamp to the receive time stamp ring. */
    /* Check if the buffers ring is full. */
    return ENET_Ptp1588UpdateTimeRing(&handle->rxPtpTsDataRing, ptpTimeData);
}

static status_t ENET_StoreTxFrameTime(ENET_Type *base, enet_handle_t *handle)
{
    assert(handle);

    uint32_t primask;
    bool ptpTimerWrap;
    bool isPtpEventMessage = false;
    enet_ptp_time_data_t ptpTimeData;
    volatile enet_tx_bd_struct_t *curBuffDescrip = handle->txBdDirtyTime;

    /* Get the control status data, If the buffer descriptor has not been processed break out. */
    if (curBuffDescrip->control & ENET_BUFFDESCRIPTOR_TX_READY_MASK)
    {
        return kStatus_ENET_TxFrameBusy;
    }

    /* Parse the PTP message. */
    isPtpEventMessage = ENET_Ptp1588ParseFrame(curBuffDescrip->buffer, &ptpTimeData, false);
    if (isPtpEventMessage)
    {
        do
        {
            /* Increase current buffer descriptor to the next one. */
            if (handle->txBdDirtyTime->control & ENET_BUFFDESCRIPTOR_TX_WRAP_MASK)
            {
                handle->txBdDirtyTime = handle->txBdBase;
            }
            else
            {
                handle->txBdDirtyTime++;
            }

            /* Do time stamp check on the last buffer descriptor of the frame. */
            if (curBuffDescrip->control & ENET_BUFFDESCRIPTOR_TX_LAST_MASK)
            {
                /* Disables the interrupt. */
                primask = DisableGlobalIRQ();

                /* Get current PTP timer nanosecond value. */
                ENET_Ptp1588GetTimer(base, handle, &ptpTimeData.timeStamp);

                /* Get PTP timer wrap event. */
                ptpTimerWrap = base->EIR & kENET_TsTimerInterrupt;

                /* Get transmit time stamp second. */
                if ((ptpTimeData.timeStamp.nanosecond > curBuffDescrip->timestamp) ||
                    ((ptpTimeData.timeStamp.nanosecond < curBuffDescrip->timestamp) && ptpTimerWrap))
                {
                    ptpTimeData.timeStamp.second = handle->msTimerSecond;
                }
                else
                {
                    ptpTimeData.timeStamp.second = handle->msTimerSecond - 1;
                }

                /* Enable the interrupt. */
                EnableGlobalIRQ(primask);

                /* Store the timestamp to the transmit timestamp ring. */
                return ENET_Ptp1588UpdateTimeRing(&handle->txPtpTsDataRing, &ptpTimeData);
            }

            /* Get the current transmit buffer descriptor. */
            curBuffDescrip = handle->txBdDirtyTime;

            /* Get the control status data, If the buffer descriptor has not been processed break out. */
            if (curBuffDescrip->control & ENET_BUFFDESCRIPTOR_TX_READY_MASK)
            {
                return kStatus_ENET_TxFrameBusy;
            }
        } while (handle->txBdDirtyTime != handle->txBdCurrent);
        return kStatus_ENET_TxFrameFail;
    }
    return kStatus_Success;
}

status_t ENET_GetTxFrameTime(enet_handle_t *handle, enet_ptp_time_data_t *ptpTimeData)
{
    assert(handle);
    assert(ptpTimeData);

    return ENET_Ptp1588SearchTimeRing(&handle->txPtpTsDataRing, ptpTimeData);
}

status_t ENET_GetRxFrameTime(enet_handle_t *handle, enet_ptp_time_data_t *ptpTimeData)
{
    assert(handle);
    assert(ptpTimeData);

    return ENET_Ptp1588SearchTimeRing(&handle->rxPtpTsDataRing, ptpTimeData);
}

#endif /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */

void ENET_TransmitIRQHandler(ENET_Type *base, enet_handle_t *handle)
{
    assert(handle);

    /* Check if the transmit interrupt happen. */
    while ((kENET_TxBufferInterrupt | kENET_TxFrameInterrupt) & base->EIR)
    {
#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
        if (base->EIR & kENET_TxFrameInterrupt)
        {
            /* Store the transmit timestamp from the buffer descriptor should be done here. */
            ENET_StoreTxFrameTime(base, handle);
        }
#endif /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */

        /* Clear the transmit interrupt event. */
        base->EIR = kENET_TxFrameInterrupt | kENET_TxBufferInterrupt;

        /* Callback function. */
        if (handle->callback)
        {
            handle->callback(base, handle, kENET_TxEvent, handle->userData);
        }
    }
}

void ENET_ReceiveIRQHandler(ENET_Type *base, enet_handle_t *handle)
{
    assert(handle);

    /* Check if the receive interrupt happen. */
    while ((kENET_RxBufferInterrupt | kENET_RxFrameInterrupt) & base->EIR)
    {
        /* Clear the transmit interrupt event. */
        base->EIR = kENET_RxFrameInterrupt | kENET_RxBufferInterrupt;

        /* Callback function. */
        if (handle->callback)
        {
            handle->callback(base, handle, kENET_RxEvent, handle->userData);
        }
    }
}

void ENET_ErrorIRQHandler(ENET_Type *base, enet_handle_t *handle)
{
    assert(handle);

    uint32_t errMask = kENET_BabrInterrupt | kENET_BabtInterrupt | kENET_EBusERInterrupt | kENET_PayloadRxInterrupt |
                       kENET_LateCollisionInterrupt | kENET_RetryLimitInterrupt | kENET_UnderrunInterrupt;

    /* Check if the error interrupt happen. */
    if (kENET_WakeupInterrupt & base->EIR)
    {
        /* Clear the wakeup interrupt. */
        base->EIR = kENET_WakeupInterrupt;
        /* wake up and enter the normal mode. */
        ENET_EnableSleepMode(base, false);
        /* Callback function. */
        if (handle->callback)
        {
            handle->callback(base, handle, kENET_WakeUpEvent, handle->userData);
        }
    }
    else
    {
        /* Clear the error interrupt event status. */
        errMask &= base->EIR;
        base->EIR = errMask;
        /* Callback function. */
        if (handle->callback)
        {
            handle->callback(base, handle, kENET_ErrEvent, handle->userData);
        }
    }
}
#ifdef ENET_ENHANCEDBUFFERDESCRIPTOR_MODE
void ENET_Ptp1588TimerIRQHandler(ENET_Type *base, enet_handle_t *handle)
{
    assert(handle);

    /* Check if the PTP time stamp interrupt happen. */
    if (kENET_TsTimerInterrupt & base->EIR)
    {
        /* Clear the time stamp interrupt. */
        base->EIR = kENET_TsTimerInterrupt;

        /* Increase timer second counter. */
        handle->msTimerSecond++;

        /* Callback function. */
        if (handle->callback)
        {
            handle->callback(base, handle, kENET_TimeStampEvent, handle->userData);
        }
    }
    else
    {
        /* Clear the time stamp interrupt. */
        base->EIR = kENET_TsAvailInterrupt;
        /* Callback function. */
        if (handle->callback)
        {
            handle->callback(base, handle, kENET_TimeStampAvailEvent, handle->userData);
        }
    }
}
#endif /* ENET_ENHANCEDBUFFERDESCRIPTOR_MODE */

void ENET_CommonFrame0IRQHandler(ENET_Type *base)
{
    uint32_t event = base->EIR;
    uint32_t instance = ENET_GetInstance(base);

    if (event & ENET_TX_INTERRUPT)
    {
        s_enetTxIsr(base, s_ENETHandle[instance]);
    }

    if (event & ENET_RX_INTERRUPT)
    {
        s_enetRxIsr(base, s_ENETHandle[instance]);
    }

    if (event & ENET_TS_INTERRUPT)
    {
        s_enetTsIsr(base, s_ENETHandle[instance]);
    }
    if (event & ENET_ERR_INTERRUPT)
    {
        s_enetErrIsr(base, s_ENETHandle[instance]);
    }
}

#if defined(ENET)
void ENET_Transmit_IRQHandler(void)
{
    s_enetTxIsr(ENET, s_ENETHandle[0]);
}

void ENET_Receive_IRQHandler(void)
{
    s_enetRxIsr(ENET, s_ENETHandle[0]);
}

void ENET_Error_IRQHandler(void)
{
    s_enetErrIsr(ENET, s_ENETHandle[0]);
}

void ENET_1588_Timer_IRQHandler(void)
{
    s_enetTsIsr(ENET, s_ENETHandle[0]);
}
#endif

