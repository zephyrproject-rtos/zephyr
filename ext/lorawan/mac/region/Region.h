/*!
 * \file      Region.h
 *
 * \brief     Region implementation.
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013 Semtech
 *
 *               ___ _____ _   ___ _  _____ ___  ___  ___ ___
 *              / __|_   _/_\ / __| |/ / __/ _ \| _ \/ __| __|
 *              \__ \ | |/ _ \ (__| ' <| _| (_) |   / (__| _|
 *              |___/ |_/_/ \_\___|_|\_\_| \___/|_|_\\___|___|
 *              embedded.connectivity.solutions===============
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 *
 * \author    Daniel Jaeckle ( STACKFORCE )
 *
 * \defgroup  REGION Region implementation
 *            This is the common API to access the specific
 *            regional implementations.
 *
 *            Preprocessor options:
 *            - LoRaWAN regions can be activated by defining the related preprocessor
 *              definition. It is possible to define more than one region.
 *              The following regions are supported:
 *              - #define REGION_AS923
 *              - #define REGION_AU915
 *              - #define REGION_CN470
 *              - #define REGION_CN779
 *              - #define REGION_EU433
 *              - #define REGION_EU868
 *              - #define REGION_KR920
 *              - #define REGION_IN865
 *              - #define REGION_US915
 *              - #define REGION_US915_HYBRID
 *
 * \{
 */
#ifndef __REGION_H__
#define __REGION_H__




/*!
 * Macro to compute bit of a channel index.
 */
#define LC( channelIndex )                          ( uint16_t )( 1 << ( channelIndex - 1 ) )

/*!
 * Region       | SF
 * ------------ | :-----:
 * AS923        | SF12 - BW125
 * AU915        | SF10 - BW125
 * CN470        | SF12 - BW125
 * CN779        | SF12 - BW125
 * EU433        | SF12 - BW125
 * EU868        | SF12 - BW125
 * IN865        | SF12 - BW125
 * KR920        | SF12 - BW125
 * US915        | SF10 - BW125
 * US915_HYBRID | SF10 - BW125
 */
#define DR_0                                        0

/*!
 * Region       | SF
 * ------------ | :-----:
 * AS923        | SF11 - BW125
 * AU915        | SF9  - BW125
 * CN470        | SF11 - BW125
 * CN779        | SF11 - BW125
 * EU433        | SF11 - BW125
 * EU868        | SF11 - BW125
 * IN865        | SF11 - BW125
 * KR920        | SF11 - BW125
 * US915        | SF9  - BW125
 * US915_HYBRID | SF9  - BW125
 */
#define DR_1                                        1

/*!
 * Region       | SF
 * ------------ | :-----:
 * AS923        | SF10 - BW125
 * AU915        | SF8  - BW125
 * CN470        | SF10 - BW125
 * CN779        | SF10 - BW125
 * EU433        | SF10 - BW125
 * EU868        | SF10 - BW125
 * IN865        | SF10 - BW125
 * KR920        | SF10 - BW125
 * US915        | SF8  - BW125
 * US915_HYBRID | SF8  - BW125
 */
#define DR_2                                        2

/*!
 * Region       | SF
 * ------------ | :-----:
 * AS923        | SF9  - BW125
 * AU915        | SF7  - BW125
 * CN470        | SF9  - BW125
 * CN779        | SF9  - BW125
 * EU433        | SF9  - BW125
 * EU868        | SF9  - BW125
 * IN865        | SF9  - BW125
 * KR920        | SF9  - BW125
 * US915        | SF7  - BW125
 * US915_HYBRID | SF7  - BW125
 */
#define DR_3                                        3

/*!
 * Region       | SF
 * ------------ | :-----:
 * AS923        | SF8  - BW125
 * AU915        | SF8  - BW500
 * CN470        | SF8  - BW125
 * CN779        | SF8  - BW125
 * EU433        | SF8  - BW125
 * EU868        | SF8  - BW125
 * IN865        | SF8  - BW125
 * KR920        | SF8  - BW125
 * US915        | SF8  - BW500
 * US915_HYBRID | SF8  - BW500
 */
#define DR_4                                        4

/*!
 * Region       | SF
 * ------------ | :-----:
 * AS923        | SF7  - BW125
 * AU915        | RFU
 * CN470        | SF7  - BW125
 * CN779        | SF7  - BW125
 * EU433        | SF7  - BW125
 * EU868        | SF7  - BW125
 * IN865        | SF7  - BW125
 * KR920        | SF7  - BW125
 * US915        | RFU
 * US915_HYBRID | RFU
 */
#define DR_5                                        5

/*!
 * Region       | SF
 * ------------ | :-----:
 * AS923        | SF7  - BW250
 * AU915        | RFU
 * CN470        | SF12 - BW125
 * CN779        | SF7  - BW250
 * EU433        | SF7  - BW250
 * EU868        | SF7  - BW250
 * IN865        | SF7  - BW250
 * KR920        | RFU
 * US915        | RFU
 * US915_HYBRID | RFU
 */
#define DR_6                                        6

/*!
 * Region       | SF
 * ------------ | :-----:
 * AS923        | FSK
 * AU915        | RFU
 * CN470        | SF12 - BW125
 * CN779        | FSK
 * EU433        | FSK
 * EU868        | FSK
 * IN865        | FSK
 * KR920        | RFU
 * US915        | RFU
 * US915_HYBRID | RFU
 */
#define DR_7                                        7

/*!
 * Region       | SF
 * ------------ | :-----:
 * AS923        | RFU
 * AU915        | SF12 - BW500
 * CN470        | RFU
 * CN779        | RFU
 * EU433        | RFU
 * EU868        | RFU
 * IN865        | RFU
 * KR920        | RFU
 * US915        | SF12 - BW500
 * US915_HYBRID | SF12 - BW500
 */
#define DR_8                                        8

/*!
 * Region       | SF
 * ------------ | :-----:
 * AS923        | RFU
 * AU915        | SF11 - BW500
 * CN470        | RFU
 * CN779        | RFU
 * EU433        | RFU
 * EU868        | RFU
 * IN865        | RFU
 * KR920        | RFU
 * US915        | SF11 - BW500
 * US915_HYBRID | SF11 - BW500
 */
#define DR_9                                        9

/*!
 * Region       | SF
 * ------------ | :-----:
 * AS923        | RFU
 * AU915        | SF10 - BW500
 * CN470        | RFU
 * CN779        | RFU
 * EU433        | RFU
 * EU868        | RFU
 * IN865        | RFU
 * KR920        | RFU
 * US915        | SF10 - BW500
 * US915_HYBRID | SF10 - BW500
 */
#define DR_10                                       10

/*!
 * Region       | SF
 * ------------ | :-----:
 * AS923        | RFU
 * AU915        | SF9  - BW500
 * CN470        | RFU
 * CN779        | RFU
 * EU433        | RFU
 * EU868        | RFU
 * IN865        | RFU
 * KR920        | RFU
 * US915        | SF9  - BW500
 * US915_HYBRID | SF9  - BW500
 */
#define DR_11                                       11

/*!
 * Region       | SF
 * ------------ | :-----:
 * AS923        | RFU
 * AU915        | SF8  - BW500
 * CN470        | RFU
 * CN779        | RFU
 * EU433        | RFU
 * EU868        | RFU
 * IN865        | RFU
 * KR920        | RFU
 * US915        | SF8  - BW500
 * US915_HYBRID | SF8  - BW500
 */
#define DR_12                                       12

/*!
 * Region       | SF
 * ------------ | :-----:
 * AS923        | RFU
 * AU915        | SF7  - BW500
 * CN470        | RFU
 * CN779        | RFU
 * EU433        | RFU
 * EU868        | RFU
 * IN865        | RFU
 * KR920        | RFU
 * US915        | SF7  - BW500
 * US915_HYBRID | SF7  - BW500
 */
#define DR_13                                       13

/*!
 * Region       | SF
 * ------------ | :-----:
 * AS923        | RFU
 * AU915        | RFU
 * CN470        | RFU
 * CN779        | RFU
 * EU433        | RFU
 * EU868        | RFU
 * IN865        | RFU
 * KR920        | RFU
 * US915        | RFU
 * US915_HYBRID | RFU
 */
#define DR_14                                       14

/*!
 * Region       | SF
 * ------------ | :-----:
 * AS923        | RFU
 * AU915        | RFU
 * CN470        | RFU
 * CN779        | RFU
 * EU433        | RFU
 * EU868        | RFU
 * IN865        | RFU
 * KR920        | RFU
 * US915        | RFU
 * US915_HYBRID | RFU
 */
#define DR_15                                       15



/*!
 * Region       | dBM
 * ------------ | :-----:
 * AS923        | Max EIRP
 * AU915        | Max EIRP
 * CN470        | Max EIRP
 * CN779        | Max EIRP
 * EU433        | Max EIRP
 * EU868        | Max EIRP
 * IN865        | Max EIRP
 * KR920        | Max EIRP
 * US915        | Max ERP
 * US915_HYBRID | Max ERP
 */
#define TX_POWER_0                                  0

/*!
 * Region       | dBM
 * ------------ | :-----:
 * AS923        | Max EIRP - 2
 * AU915        | Max EIRP - 2
 * CN470        | Max EIRP - 2
 * CN779        | Max EIRP - 2
 * EU433        | Max EIRP - 2
 * EU868        | Max EIRP - 2
 * IN865        | Max EIRP - 2
 * KR920        | Max EIRP - 2
 * US915        | Max ERP - 2
 * US915_HYBRID | Max ERP - 2
 */
#define TX_POWER_1                                  1

/*!
 * Region       | dBM
 * ------------ | :-----:
 * AS923        | Max EIRP - 4
 * AU915        | Max EIRP - 4
 * CN470        | Max EIRP - 4
 * CN779        | Max EIRP - 4
 * EU433        | Max EIRP - 4
 * EU868        | Max EIRP - 4
 * IN865        | Max EIRP - 4
 * KR920        | Max EIRP - 4
 * US915        | Max ERP - 4
 * US915_HYBRID | Max ERP - 4
 */
#define TX_POWER_2                                  2

/*!
 * Region       | dBM
 * ------------ | :-----:
 * AS923        | Max EIRP - 6
 * AU915        | Max EIRP - 6
 * CN470        | Max EIRP - 6
 * CN779        | Max EIRP - 6
 * EU433        | Max EIRP - 6
 * EU868        | Max EIRP - 6
 * IN865        | Max EIRP - 6
 * KR920        | Max EIRP - 6
 * US915        | Max ERP - 6
 * US915_HYBRID | Max ERP - 6
 */
#define TX_POWER_3                                  3

/*!
 * Region       | dBM
 * ------------ | :-----:
 * AS923        | Max EIRP - 8
 * AU915        | Max EIRP - 8
 * CN470        | Max EIRP - 8
 * CN779        | Max EIRP - 8
 * EU433        | Max EIRP - 8
 * EU868        | Max EIRP - 8
 * IN865        | Max EIRP - 8
 * KR920        | Max EIRP - 8
 * US915        | Max ERP - 8
 * US915_HYBRID | Max ERP - 8
 */
#define TX_POWER_4                                  4

/*!
 * Region       | dBM
 * ------------ | :-----:
 * AS923        | Max EIRP - 10
 * AU915        | Max EIRP - 10
 * CN470        | Max EIRP - 10
 * CN779        | Max EIRP - 10
 * EU433        | Max EIRP - 10
 * EU868        | Max EIRP - 10
 * IN865        | Max EIRP - 10
 * KR920        | Max EIRP - 10
 * US915        | Max ERP - 10
 * US915_HYBRID | Max ERP - 10
 */
#define TX_POWER_5                                  5

/*!
 * Region       | dBM
 * ------------ | :-----:
 * AS923        | Max EIRP - 12
 * AU915        | Max EIRP - 12
 * CN470        | Max EIRP - 12
 * CN779        | -
 * EU433        | -
 * EU868        | Max EIRP - 12
 * IN865        | Max EIRP - 12
 * KR920        | Max EIRP - 12
 * US915        | Max ERP - 12
 * US915_HYBRID | Max ERP - 12
 */
#define TX_POWER_6                                  6

/*!
 * Region       | dBM
 * ------------ | :-----:
 * AS923        | Max EIRP - 14
 * AU915        | Max EIRP - 14
 * CN470        | Max EIRP - 14
 * CN779        | -
 * EU433        | -
 * EU868        | Max EIRP - 14
 * IN865        | Max EIRP - 14
 * KR920        | Max EIRP - 14
 * US915        | Max ERP - 14
 * US915_HYBRID | Max ERP - 14
 */
#define TX_POWER_7                                  7

/*!
 * Region       | dBM
 * ------------ | :-----:
 * AS923        | -
 * AU915        | Max EIRP - 16
 * CN470        | -
 * CN779        | -
 * EU433        | -
 * EU868        | -
 * IN865        | Max EIRP - 16
 * KR920        | -
 * US915        | Max ERP - 16
 * US915_HYBRID | Max ERP -16
 */
#define TX_POWER_8                                  8

/*!
 * Region       | dBM
 * ------------ | :-----:
 * AS923        | -
 * AU915        | Max EIRP - 18
 * CN470        | -
 * CN779        | -
 * EU433        | -
 * EU868        | -
 * IN865        | Max EIRP - 18
 * KR920        | -
 * US915        | Max ERP - 16
 * US915_HYBRID | Max ERP - 16
 */
#define TX_POWER_9                                  9

/*!
 * Region       | dBM
 * ------------ | :-----:
 * AS923        | -
 * AU915        | Max EIRP - 20
 * CN470        | -
 * CN779        | -
 * EU433        | -
 * EU868        | -
 * IN865        | Max EIRP - 20
 * KR920        | -
 * US915        | Max ERP - 10
 * US915_HYBRID | Max ERP - 10
 */
#define TX_POWER_10                                 10

/*!
 * RFU
 */
#define TX_POWER_11                                 11

/*!
 * RFU
 */
#define TX_POWER_12                                 12

/*!
 * RFU
 */
#define TX_POWER_13                                 13

/*!
 * RFU
 */
#define TX_POWER_14                                 14

/*!
 * RFU
 */
#define TX_POWER_15                                 15



/*!
 * Enumeration of phy attributes.
 */
typedef enum ePhyAttribute
{
    /*!
     * Minimum RX datarate.
     */
    PHY_MIN_RX_DR,
    /*!
     * Minimum TX datarate.
     */
    PHY_MIN_TX_DR,
    /*!
     * Maximum RX datarate.
     */
    PHY_MAX_RX_DR,
    /*!
     * Maximum TX datarate.
     */
    PHY_MAX_TX_DR,
    /*!
     * TX datarate.
     */
    PHY_TX_DR,
    /*!
     * Default TX datarate.
     */
    PHY_DEF_TX_DR,
    /*!
     * RX datarate.
     */
    PHY_RX_DR,
    /*!
     * TX power.
     */
    PHY_TX_POWER,
    /*!
     * Default TX power.
     */
    PHY_DEF_TX_POWER,
    /*!
     * Maximum payload possible.
     */
    PHY_MAX_PAYLOAD,
    /*!
     * Maximum payload possible when repeater support is enabled.
     */
    PHY_MAX_PAYLOAD_REPEATER,
    /*!
     * Duty cycle.
     */
    PHY_DUTY_CYCLE,
    /*!
     * Maximum receive window duration.
     */
    PHY_MAX_RX_WINDOW,
    /*!
     * Receive delay for window 1.
     */
    PHY_RECEIVE_DELAY1,
    /*!
     * Receive delay for window 2.
     */
    PHY_RECEIVE_DELAY2,
    /*!
     * Join accept delay for window 1.
     */
    PHY_JOIN_ACCEPT_DELAY1,
    /*!
     * Join accept delay for window 2.
     */
    PHY_JOIN_ACCEPT_DELAY2,
    /*!
     * Maximum frame counter gap.
     */
    PHY_MAX_FCNT_GAP,
    /*!
     * Acknowledgement time out.
     */
    PHY_ACK_TIMEOUT,
    /*!
     * Default datarate offset for window 1.
     */
    PHY_DEF_DR1_OFFSET,
    /*!
     * Default receive window 2 frequency.
     */
    PHY_DEF_RX2_FREQUENCY,
    /*!
     * Default receive window 2 datarate.
     */
    PHY_DEF_RX2_DR,
    /*!
     * Channels mask.
     */
    PHY_CHANNELS_MASK,
    /*!
     * Channels default mask.
     */
    PHY_CHANNELS_DEFAULT_MASK,
    /*!
     * Maximum number of supported channels
     */
    PHY_MAX_NB_CHANNELS,
    /*!
     * Channels.
     */
    PHY_CHANNELS,
    /*!
     * Default value of the uplink dwell time.
     */
    PHY_DEF_UPLINK_DWELL_TIME,
    /*!
     * Default value of the downlink dwell time.
     */
    PHY_DEF_DOWNLINK_DWELL_TIME,
    /*!
     * Default value of the MaxEIRP.
     */
    PHY_DEF_MAX_EIRP,
    /*!
     * Default value of the antenna gain.
     */
    PHY_DEF_ANTENNA_GAIN,
    /*!
     * Value for the number of join trials.
     */
    PHY_NB_JOIN_TRIALS,
    /*!
     * Default value for the number of join trials.
     */
    PHY_DEF_NB_JOIN_TRIALS,
    /*!
     * Next lower datarate.
     */
    PHY_NEXT_LOWER_TX_DR
}PhyAttribute_t;

/*!
 * Enumeration of initialization types.
 */
typedef enum eInitType
{
    /*!
     * Performs an initialization and overwrites all existing data.
     */
    INIT_TYPE_INIT,
    /*!
     * Restores default channels only.
     */
    INIT_TYPE_RESTORE
}InitType_t;

typedef enum eChannelsMask
{
    /*!
     * The channels mask.
     */
    CHANNELS_MASK,
    /*!
     * The channels default mask.
     */
    CHANNELS_DEFAULT_MASK
}ChannelsMask_t;

/*!
 * Union for the structure uGetPhyParams
 */
typedef union uPhyParam
{
    /*!
     * A parameter value.
     */
    uint32_t Value;
    /*!
     * A floating point value.
     */
    float fValue;
    /*!
     * Pointer to the channels mask.
     */
    uint16_t* ChannelsMask;
    /*!
     * Pointer to the channels.
     */
    ChannelParams_t* Channels;
}PhyParam_t;

/*!
 * Parameter structure for the function RegionGetPhyParam.
 */
typedef struct sGetPhyParams
{
    /*!
     * Setup the parameter to get.
     */
    PhyAttribute_t Attribute;
    /*!
     * Datarate.
     * The parameter is needed for the following queries:
     * PHY_MAX_PAYLOAD, PHY_MAX_PAYLOAD_REPEATER, PHY_NEXT_LOWER_TX_DR.
     */
    int8_t Datarate;
    /*!
     * Uplink dwell time.
     * The parameter is needed for the following queries:
     * PHY_MIN_TX_DR, PHY_MAX_PAYLOAD, PHY_MAX_PAYLOAD_REPEATER, PHY_NEXT_LOWER_TX_DR.
     */
    uint8_t UplinkDwellTime;
    /*!
     * Downlink dwell time.
     * The parameter is needed for the following queries:
     * PHY_MIN_RX_DR, PHY_MAX_PAYLOAD, PHY_MAX_PAYLOAD_REPEATER.
     */
    uint8_t DownlinkDwellTime;
}GetPhyParams_t;

/*!
 * Parameter structure for the function RegionSetBandTxDone.
 */
typedef struct sSetBandTxDoneParams
{
    /*!
     * Channel to update.
     */
    uint8_t Channel;
    /*!
     * Joined Set to true, if the node has joined the network
     */
    bool Joined;
    /*!
     * Last TX done time.
     */
    TimerTime_t LastTxDoneTime;
}SetBandTxDoneParams_t;

/*!
 * Parameter structure for the function RegionVerify.
 */
typedef union uVerifyParams
{
    /*!
     * TX power to verify.
     */
    int8_t TxPower;
    /*!
     * Set to true, if the duty cycle is enabled, otherwise false.
     */
    bool DutyCycle;
    /*!
     * The number of join trials.
     */
    uint8_t NbJoinTrials;
    /*!
     * Datarate to verify.
     */
    struct sDatarateParams
    {
        /*!
        * Datarate to verify.
        */
        int8_t Datarate;
        /*!
        * The downlink dwell time.
        */
        uint8_t DownlinkDwellTime;
        /*!
        * The up link dwell time.
        */
        uint8_t UplinkDwellTime;
    }DatarateParams;
}VerifyParams_t;

/*!
 * Parameter structure for the function RegionApplyCFList.
 */
typedef struct sApplyCFListParams
{
    /*!
     * Payload which contains the CF list.
     */
    uint8_t* Payload;
    /*!
     * Size of the payload.
     */
    uint8_t Size;
}ApplyCFListParams_t;

/*!
 * Parameter structure for the function RegionChanMaskSet.
 */
typedef struct sChanMaskSetParams
{
    /*!
     * Pointer to the channels mask which should be set.
     */
    uint16_t* ChannelsMaskIn;
    /*!
     * Pointer to the channels mask which should be set.
     */
    ChannelsMask_t ChannelsMaskType;
}ChanMaskSetParams_t;

/*!
 * Parameter structure for the function RegionAdrNext.
 */
typedef struct sAdrNextParams
{
    /*!
     * Set to true, if the function should update the channels mask.
     */
    bool UpdateChanMask;
    /*!
     * Set to true, if ADR is enabled.
     */
    bool AdrEnabled;
    /*!
     * ADR ack counter.
     */
    uint32_t AdrAckCounter;
    /*!
     * Datarate used currently.
     */
    int8_t Datarate;
    /*!
     * TX power used currently.
     */
    int8_t TxPower;
    /*!
     * UplinkDwellTime
     */
    uint8_t UplinkDwellTime;
}AdrNextParams_t;

/*!
 * Parameter structure for the function RegionRxConfig.
 */
typedef struct sRxConfigParams
{
    /*!
     * The RX channel.
     */
    uint8_t Channel;
    /*!
     * RX datarate.
     */
    int8_t Datarate;
    /*!
     * RX bandwidth.
     */
    uint8_t Bandwidth;
    /*!
     * RX datarate offset.
     */
    int8_t DrOffset;
    /*!
     * RX frequency.
     */
    uint32_t Frequency;
    /*!
     * RX window timeout
     */
     uint32_t WindowTimeout;
    /*!
     * RX window offset
     */
    int32_t WindowOffset;
    /*!
     * Downlink dwell time.
     */
    uint8_t DownlinkDwellTime;
    /*!
     * Set to true, if a repeater is supported.
     */
    bool RepeaterSupport;
    /*!
     * Set to true, if RX should be continuous.
     */
    bool RxContinuous;
    /*!
     * Sets the RX window. 0: RX window 1, 1: RX window 2.
     */
    bool Window;
}RxConfigParams_t;

/*!
 * Parameter structure for the function RegionTxConfig.
 */
typedef struct sTxConfigParams
{
    /*!
     * The TX channel.
     */
    uint8_t Channel;
    /*!
     * The TX datarate.
     */
    int8_t Datarate;
    /*!
     * The TX power.
     */
    int8_t TxPower;
    /*!
     * The Max EIRP, if applicable.
     */
    float MaxEirp;
    /*!
     * The antenna gain, if applicable.
     */
    float AntennaGain;
    /*!
     * Frame length to setup.
     */
    uint16_t PktLen;
}TxConfigParams_t;

/*!
 * Parameter structure for the function RegionLinkAdrReq.
 */
typedef struct sLinkAdrReqParams
{
    /*!
     * Pointer to the payload which contains the MAC commands.
     */
    uint8_t* Payload;
    /*!
     * Size of the payload.
     */
    uint8_t PayloadSize;
    /*!
     * Uplink dwell time.
     */
    uint8_t UplinkDwellTime;
    /*!
     * Set to true, if ADR is enabled.
     */
    bool AdrEnabled;
    /*!
     * The current datarate.
     */
    int8_t CurrentDatarate;
    /*!
     * The current TX power.
     */
    int8_t CurrentTxPower;
    /*!
     * The current number of repetitions.
     */
    uint8_t CurrentNbRep;
}LinkAdrReqParams_t;

/*!
 * Parameter structure for the function RegionRxParamSetupReq.
 */
typedef struct sRxParamSetupReqParams
{
    /*!
     * The datarate to setup.
     */
    int8_t Datarate;
    /*!
     * Datarate offset.
     */
    int8_t DrOffset;
    /*!
     * The frequency to setup.
     */
    uint32_t Frequency;
}RxParamSetupReqParams_t;

/*!
 * Parameter structure for the function RegionNewChannelReq.
 */
typedef struct sNewChannelReqParams
{
    /*!
     * Pointer to the new channels.
     */
    ChannelParams_t* NewChannel;
    /*!
     * Channel id.
     */
    int8_t ChannelId;
}NewChannelReqParams_t;

/*!
 * Parameter structure for the function RegionTxParamSetupReq.
 */
typedef struct sTxParamSetupReqParams
{
    /*!
     * Uplink dwell time.
     */
    uint8_t UplinkDwellTime;
    /*!
     * Downlink dwell time.
     */
    uint8_t DownlinkDwellTime;
    /*!
     * Max EIRP.
     */
    uint8_t MaxEirp;
}TxParamSetupReqParams_t;

/*!
 * Parameter structure for the function RegionDlChannelReq.
 */
typedef struct sDlChannelReqParams
{
    /*!
     * Channel Id to add the frequency.
     */
    uint8_t ChannelId;
    /*!
     * Alternative frequency for the Rx1 window.
     */
    uint32_t Rx1Frequency;
}DlChannelReqParams_t;

/*!
 * Parameter structure for the function RegionAlternateDr.
 */
typedef struct sAlternateDrParams
{
    /*!
     * Number of trials.
     */
    uint16_t NbTrials;
}AlternateDrParams_t;

/*!
 * Parameter structure for the function RegionCalcBackOff.
 */
typedef struct sCalcBackOffParams
{
    /*!
     * Set to true, if the node has already joined a network, otherwise false.
     */
    bool Joined;
    /*!
     * Joined Set to true, if the last uplink was a join request
     */
    bool LastTxIsJoinRequest;
    /*!
     * Set to true, if the duty cycle is enabled, otherwise false.
     */
    bool DutyCycleEnabled;
    /*!
     * Current channel index.
     */
    uint8_t Channel;
    /*!
     * Elapsed time since the start of the node.
     */
    TimerTime_t ElapsedTime;
    /*!
     * Time-on-air of the last transmission.
     */
    TimerTime_t TxTimeOnAir;
}CalcBackOffParams_t;

/*!
 * Parameter structure for the function RegionNextChannel.
 */
typedef struct sNextChanParams
{
    /*!
     * Aggregated time-off time.
     */
    TimerTime_t AggrTimeOff;
    /*!
     * Time of the last aggregated TX.
     */
    TimerTime_t LastAggrTx;
    /*!
     * Current datarate.
     */
    int8_t Datarate;
    /*!
     * Set to true, if the node has already joined a network, otherwise false.
     */
    bool Joined;
    /*!
     * Set to true, if the duty cycle is enabled, otherwise false.
     */
    bool DutyCycleEnabled;
}NextChanParams_t;

/*!
 * Parameter structure for the function RegionChannelsAdd.
 */
typedef struct sChannelAddParams
{
    /*!
     * Pointer to the new channel to add.
     */
    ChannelParams_t* NewChannel;
    /*!
     * Channel id to add.
     */
    uint8_t ChannelId;
}ChannelAddParams_t;

/*!
 * Parameter structure for the function RegionChannelsRemove.
 */
typedef struct sChannelRemoveParams
{
    /*!
     * Channel id to remove.
     */
    uint8_t ChannelId;
}ChannelRemoveParams_t;

/*!
 * Parameter structure for the function RegionContinuousWave.
 */
typedef struct sContinuousWaveParams
{
    /*!
     * Current channel index.
     */
    uint8_t Channel;
    /*!
     * Datarate. Used to limit the TX power.
     */
    int8_t Datarate;
    /*!
     * The TX power to setup.
     */
    int8_t TxPower;
    /*!
     * Max EIRP, if applicable.
     */
    float MaxEirp;
    /*!
     * The antenna gain, if applicable.
     */
    float AntennaGain;
    /*!
     * Specifies the time the radio will stay in CW mode.
     */
    uint16_t Timeout;
}ContinuousWaveParams_t;



/*!
 * \brief The function verifies if a region is active or not. If a region
 *        is not active, it cannot be used.
 *
 * \param [IN] region LoRaWAN region.
 *
 * \retval Return true, if the region is supported.
 */
bool RegionIsActive( LoRaMacRegion_t region );

/*!
 * \brief The function gets a value of a specific phy attribute.
 *
 * \param [IN] region LoRaWAN region.
 *
 * \param [IN] getPhy Pointer to the function parameters.
 *
 * \retval Returns a structure containing the PHY parameter.
 */
PhyParam_t RegionGetPhyParam( LoRaMacRegion_t region, GetPhyParams_t* getPhy );

/*!
 * \brief Updates the last TX done parameters of the current channel.
 *
 * \param [IN] region LoRaWAN region.
 *
 * \param [IN] txDone Pointer to the function parameters.
 */
void RegionSetBandTxDone( LoRaMacRegion_t region, SetBandTxDoneParams_t* txDone );

/*!
 * \brief Initializes the channels masks and the channels.
 *
 * \param [IN] region LoRaWAN region.
 *
 * \param [IN] type Sets the initialization type.
 */
void RegionInitDefaults( LoRaMacRegion_t region, InitType_t type );

/*!
 * \brief Verifies a parameter.
 *
 * \param [IN] region LoRaWAN region.
 *
 * \param [IN] verify Pointer to the function parameters.
 *
 * \param [IN] type Sets the initialization type.
 *
 * \retval Returns true, if the parameter is valid.
 */
bool RegionVerify( LoRaMacRegion_t region, VerifyParams_t* verify, PhyAttribute_t phyAttribute );

/*!
 * \brief The function parses the input buffer and sets up the channels of the
 *        CF list.
 *
 * \param [IN] region LoRaWAN region.
 *
 * \param [IN] applyCFList Pointer to the function parameters.
 */
void RegionApplyCFList( LoRaMacRegion_t region, ApplyCFListParams_t* applyCFList );

/*!
 * \brief Sets a channels mask.
 *
 * \param [IN] region LoRaWAN region.
 *
 * \param [IN] chanMaskSet Pointer to the function parameters.
 *
 * \retval Returns true, if the channels mask could be set.
 */
bool RegionChanMaskSet( LoRaMacRegion_t region, ChanMaskSetParams_t* chanMaskSet );

/*!
 * \brief Calculates the next datarate to set, when ADR is on or off.
 *
 * \param [IN] region LoRaWAN region.
 *
 * \param [IN] adrNext Pointer to the function parameters.
 *
 * \param [OUT] drOut The calculated datarate for the next TX.
 *
 * \param [OUT] txPowOut The TX power for the next TX.
 *
 * \param [OUT] adrAckCounter The calculated ADR acknowledgement counter.
 *
 * \retval Returns true, if an ADR request should be performed.
 */
bool RegionAdrNext( LoRaMacRegion_t region, AdrNextParams_t* adrNext, int8_t* drOut, int8_t* txPowOut, uint32_t* adrAckCounter );

/*!
 * \brief Configuration of the RX windows.
 *
 * \param [IN] region LoRaWAN region.
 *
 * \param [IN] rxConfig Pointer to the function parameters.
 *
 * \param [OUT] datarate The datarate index which was set.
 *
 * \retval Returns true, if the configuration was applied successfully.
 */
bool RegionRxConfig( LoRaMacRegion_t region, RxConfigParams_t* rxConfig, int8_t* datarate );

/*
 * Rx window precise timing
 *
 * For more details please consult the following document, chapter 3.1.2.
 * http://www.semtech.com/images/datasheet/SX1272_settings_for_LoRaWAN_v2.0.pdf
 * or
 * http://www.semtech.com/images/datasheet/SX1276_settings_for_LoRaWAN_v2.0.pdf
 *
 *                 Downlink start: T = Tx + 1s (+/- 20 us)
 *                            |
 *             TRxEarly       |        TRxLate
 *                |           |           |
 *                |           |           +---+---+---+---+---+---+---+---+
 *                |           |           |       Latest Rx window        |
 *                |           |           +---+---+---+---+---+---+---+---+
 *                |           |           |
 *                +---+---+---+---+---+---+---+---+
 *                |       Earliest Rx window      |
 *                +---+---+---+---+---+---+---+---+
 *                            |
 *                            +---+---+---+---+---+---+---+---+
 *Downlink preamble 8 symbols |   |   |   |   |   |   |   |   |
 *                            +---+---+---+---+---+---+---+---+
 *
 *                     Worst case Rx window timings
 *
 * TRxLate  = DEFAULT_MIN_RX_SYMBOLS * tSymbol - RADIO_WAKEUP_TIME
 * TRxEarly = 8 - DEFAULT_MIN_RX_SYMBOLS * tSymbol - RxWindowTimeout - RADIO_WAKEUP_TIME
 *
 * TRxLate - TRxEarly = 2 * DEFAULT_SYSTEM_MAX_RX_ERROR
 *
 * RxOffset = ( TRxLate + TRxEarly ) / 2
 *
 * RxWindowTimeout = ( 2 * DEFAULT_MIN_RX_SYMBOLS - 8 ) * tSymbol + 2 * DEFAULT_SYSTEM_MAX_RX_ERROR
 * RxOffset = 4 * tSymbol - RxWindowTimeout / 2 - RADIO_WAKE_UP_TIME
 *
 * Minimal value of RxWindowTimeout must be 5 symbols which implies that the system always tolerates at least an error of 1.5 * tSymbol
 */
/*!
 * Computes the Rx window timeout and offset.
 *
 * \param [IN] region       LoRaWAN region.
 *
 * \param [IN] datarate     Rx window datarate index to be used
 *
 * \param [IN] minRxSymbols Minimum required number of symbols to detect an Rx frame.
 *
 * \param [IN] rxError      System maximum timing error of the receiver. In milliseconds
 *                          The receiver will turn on in a [-rxError : +rxError] ms
 *                          interval around RxOffset
 *
 * \param [OUT]rxConfigParams Returns updated WindowTimeout and WindowOffset fields.
 */
void RegionComputeRxWindowParameters( LoRaMacRegion_t region, int8_t datarate, uint8_t minRxSymbols, uint32_t rxError, RxConfigParams_t *rxConfigParams );

/*!
 * \brief TX configuration.
 *
 * \param [IN] region LoRaWAN region.
 *
 * \param [IN] txConfig Pointer to the function parameters.
 *
 * \param [OUT] txPower The tx power index which was set.
 *
 * \param [OUT] txTimeOnAir The time-on-air of the frame.
 *
 * \retval Returns true, if the configuration was applied successfully.
 */
bool RegionTxConfig( LoRaMacRegion_t region, TxConfigParams_t* txConfig, int8_t* txPower, TimerTime_t* txTimeOnAir );

/*!
 * \brief The function processes a Link ADR Request.
 *
 * \param [IN] region LoRaWAN region.
 *
 * \param [IN] linkAdrReq Pointer to the function parameters.
 *
 * \param [OUT] drOut The datarate which was applied.
 *
 * \param [OUT] txPowOut The TX power which was applied.
 *
 * \param [OUT] nbRepOut The number of repetitions to apply.
 *
 * \param [OUT] nbBytesParsed The number bytes which were parsed.
 *
 * \retval Returns the status of the operation, according to the LoRaMAC specification.
 */
uint8_t RegionLinkAdrReq( LoRaMacRegion_t region, LinkAdrReqParams_t* linkAdrReq, int8_t* drOut, int8_t* txPowOut, uint8_t* nbRepOut, uint8_t* nbBytesParsed );

/*!
 * \brief The function processes a RX Parameter Setup Request.
 *
 * \param [IN] region LoRaWAN region.
 *
 * \param [IN] rxParamSetupReq Pointer to the function parameters.
 *
 * \retval Returns the status of the operation, according to the LoRaMAC specification.
 */
uint8_t RegionRxParamSetupReq( LoRaMacRegion_t region, RxParamSetupReqParams_t* rxParamSetupReq );

/*!
 * \brief The function processes a New Channel Request.
 *
 * \param [IN] region LoRaWAN region.
 *
 * \param [IN] newChannelReq Pointer to the function parameters.
 *
 * \retval Returns the status of the operation, according to the LoRaMAC specification.
 */
uint8_t RegionNewChannelReq( LoRaMacRegion_t region, NewChannelReqParams_t* newChannelReq );

/*!
 * \brief The function processes a TX ParamSetup Request.
 *
 * \param [IN] region LoRaWAN region.
 *
 * \param [IN] txParamSetupReq Pointer to the function parameters.
 *
 * \retval Returns the status of the operation, according to the LoRaMAC specification.
 *         Returns -1, if the functionality is not implemented. In this case, the end node
 *         shall ignore the command.
 */
int8_t RegionTxParamSetupReq( LoRaMacRegion_t region, TxParamSetupReqParams_t* txParamSetupReq );

/*!
 * \brief The function processes a DlChannel Request.
 *
 * \param [IN] region LoRaWAN region.
 *
 * \param [IN] dlChannelReq Pointer to the function parameters.
 *
 * \retval Returns the status of the operation, according to the LoRaMAC specification.
 */
uint8_t RegionDlChannelReq( LoRaMacRegion_t region, DlChannelReqParams_t* dlChannelReq );

/*!
 * \brief Alternates the datarate of the channel for the join request.
 *
 * \param [IN] region LoRaWAN region.
 *
 * \param [IN] alternateDr Pointer to the function parameters.
 *
 * \retval Datarate to apply.
 */
int8_t RegionAlternateDr( LoRaMacRegion_t region, AlternateDrParams_t* alternateDr );

/*!
 * \brief Calculates the back-off time.
 *
 * \param [IN] region LoRaWAN region.
 *
 * \param [IN] calcBackOff Pointer to the function parameters.
 */
void RegionCalcBackOff( LoRaMacRegion_t region, CalcBackOffParams_t* calcBackOff );

/*!
 * \brief Searches and set the next random available channel
 *
 * \param [IN] region LoRaWAN region.
 *
 * \param [OUT] channel Next channel to use for TX.
 *
 * \param [OUT] time Time to wait for the next transmission according to the duty
 *              cycle.
 *
 * \param [OUT] aggregatedTimeOff Updates the aggregated time off.
 *
 * \retval Function status [1: OK, 0: Unable to find a channel on the current datarate].
 */
bool RegionNextChannel( LoRaMacRegion_t region, NextChanParams_t* nextChanParams, uint8_t* channel, TimerTime_t* time, TimerTime_t* aggregatedTimeOff );

/*!
 * \brief Adds a channel.
 *
 * \param [IN] region LoRaWAN region.
 *
 * \param [IN] channelAdd Pointer to the function parameters.
 *
 * \retval Status of the operation.
 */
LoRaMacStatus_t RegionChannelAdd( LoRaMacRegion_t region, ChannelAddParams_t* channelAdd );

/*!
 * \brief Removes a channel.
 *
 * \param [IN] region LoRaWAN region.
 *
 * \param [IN] channelRemove Pointer to the function parameters.
 *
 * \retval Returns true, if the channel was removed successfully.
 */
bool RegionChannelsRemove( LoRaMacRegion_t region, ChannelRemoveParams_t* channelRemove );

/*!
 * \brief Sets the radio into continuous wave mode.
 *
 * \param [IN] region LoRaWAN region.
 *
 * \param [IN] continuousWave Pointer to the function parameters.
 */
void RegionSetContinuousWave( LoRaMacRegion_t region, ContinuousWaveParams_t* continuousWave );

/*!
 * \brief Computes new datarate according to the given offset
 *
 * \param [IN] downlinkDwellTime Downlink dwell time configuration. 0: No limit, 1: 400ms
 *
 * \param [IN] dr Current datarate
 *
 * \param [IN] drOffset Offset to be applied
 *
 * \retval newDr Computed datarate.
 */
uint8_t RegionApplyDrOffset( LoRaMacRegion_t region, uint8_t downlinkDwellTime, int8_t dr, int8_t drOffset );

/*! \} defgroup REGION */

#endif // __REGION_H__
