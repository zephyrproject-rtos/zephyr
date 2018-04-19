/*!
 * \file      RegionEU433.h
 *
 * \brief     Region definition for EU433
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
 * \defgroup  REGIONEU433 Region EU433
 *            Implementation according to LoRaWAN Specification v1.0.2.
 * \{
 */
#ifndef __REGION_EU433_H__
#define __REGION_EU433_H__

/*!
 * LoRaMac maximum number of channels
 */
#define EU433_MAX_NB_CHANNELS                       16

/*!
 * Number of default channels
 */
#define EU433_NUMB_DEFAULT_CHANNELS                 3

/*!
 * Number of channels to apply for the CF list
 */
#define EU433_NUMB_CHANNELS_CF_LIST                 5

/*!
 * Minimal datarate that can be used by the node
 */
#define EU433_TX_MIN_DATARATE                       DR_0

/*!
 * Maximal datarate that can be used by the node
 */
#define EU433_TX_MAX_DATARATE                       DR_7

/*!
 * Minimal datarate that can be used by the node
 */
#define EU433_RX_MIN_DATARATE                       DR_0

/*!
 * Maximal datarate that can be used by the node
 */
#define EU433_RX_MAX_DATARATE                       DR_7

/*!
 * Default datarate used by the node
 */
#define EU433_DEFAULT_DATARATE                      DR_0

/*!
 * Minimal Rx1 receive datarate offset
 */
#define EU433_MIN_RX1_DR_OFFSET                     0

/*!
 * Maximal Rx1 receive datarate offset
 */
#define EU433_MAX_RX1_DR_OFFSET                     5

/*!
 * Default Rx1 receive datarate offset
 */
#define EU433_DEFAULT_RX1_DR_OFFSET                 0

/*!
 * Minimal Tx output power that can be used by the node
 */
#define EU433_MIN_TX_POWER                          TX_POWER_5

/*!
 * Maximal Tx output power that can be used by the node
 */
#define EU433_MAX_TX_POWER                          TX_POWER_0

/*!
 * Default Tx output power used by the node
 */
#define EU433_DEFAULT_TX_POWER                      TX_POWER_0

/*!
 * Default Max EIRP
 */
#define EU433_DEFAULT_MAX_EIRP                      12.15f

/*!
 * Default antenna gain
 */
#define EU433_DEFAULT_ANTENNA_GAIN                  2.15f

/*!
 * ADR Ack limit
 */
#define EU433_ADR_ACK_LIMIT                         64

/*!
 * ADR Ack delay
 */
#define EU433_ADR_ACK_DELAY                         32

/*!
 * Enabled or disabled the duty cycle
 */
#define EU433_DUTY_CYCLE_ENABLED                    1

/*!
 * Maximum RX window duration
 */
#define EU433_MAX_RX_WINDOW                         3000

/*!
 * Receive delay 1
 */
#define EU433_RECEIVE_DELAY1                        1000

/*!
 * Receive delay 2
 */
#define EU433_RECEIVE_DELAY2                        2000

/*!
 * Join accept delay 1
 */
#define EU433_JOIN_ACCEPT_DELAY1                    5000

/*!
 * Join accept delay 2
 */
#define EU433_JOIN_ACCEPT_DELAY2                    6000

/*!
 * Maximum frame counter gap
 */
#define EU433_MAX_FCNT_GAP                          16384

/*!
 * Ack timeout
 */
#define EU433_ACKTIMEOUT                            2000

/*!
 * Random ack timeout limits
 */
#define EU433_ACK_TIMEOUT_RND                       1000

/*!
 * Verification of default datarate
 */
#if ( EU433_DEFAULT_DATARATE > DR_5 )
#error "A default DR higher than DR_5 may lead to connectivity loss."
#endif

/*!
 * Second reception window channel frequency definition.
 */
#define EU433_RX_WND_2_FREQ                         434665000

/*!
 * Second reception window channel datarate definition.
 */
#define EU433_RX_WND_2_DR                           DR_0

/*!
 * LoRaMac maximum number of bands
 */
#define EU433_MAX_NB_BANDS                          1

/*!
 * Band 0 definition
 * { DutyCycle, TxMaxPower, LastTxDoneTime, TimeOff }
 */
#define EU433_BAND0                                 { 100, EU433_MAX_TX_POWER, 0,  0 } //  1.0 %

/*!
 * LoRaMac default channel 1
 * Channel = { Frequency [Hz], RX1 Frequency [Hz], { ( ( DrMax << 4 ) | DrMin ) }, Band }
 */
#define EU433_LC1                                   { 433175000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }

/*!
 * LoRaMac default channel 2
 * Channel = { Frequency [Hz], RX1 Frequency [Hz], { ( ( DrMax << 4 ) | DrMin ) }, Band }
 */
#define EU433_LC2                                   { 433375000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }

/*!
 * LoRaMac default channel 3
 * Channel = { Frequency [Hz], RX1 Frequency [Hz], { ( ( DrMax << 4 ) | DrMin ) }, Band }
 */
#define EU433_LC3                                   { 433575000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }

/*!
 * LoRaMac channels which are allowed for the join procedure
 */
#define EU433_JOIN_CHANNELS                         ( uint16_t )( LC( 1 ) | LC( 2 ) | LC( 3 ) )

/*!
 * Data rates table definition
 */
static const uint8_t DataratesEU433[] = { 12, 11, 10,  9,  8,  7,  7, 50 };

/*!
 * Bandwidths table definition in Hz
 */
static const uint32_t BandwidthsEU433[] = { 125000, 125000, 125000, 125000, 125000, 125000, 250000, 0 };

/*!
 * Maximum payload with respect to the datarate index. Cannot operate with repeater.
 */
static const uint8_t MaxPayloadOfDatarateEU433[] = { 51, 51, 51, 115, 242, 242, 242, 242 };

/*!
 * Maximum payload with respect to the datarate index. Can operate with repeater.
 */
static const uint8_t MaxPayloadOfDatarateRepeaterEU433[] = { 51, 51, 51, 115, 222, 222, 222, 222 };

/*!
 * \brief The function gets a value of a specific phy attribute.
 *
 * \param [IN] getPhy Pointer to the function parameters.
 *
 * \retval Returns a structure containing the PHY parameter.
 */
PhyParam_t RegionEU433GetPhyParam( GetPhyParams_t* getPhy );

/*!
 * \brief Updates the last TX done parameters of the current channel.
 *
 * \param [IN] txDone Pointer to the function parameters.
 */
void RegionEU433SetBandTxDone( SetBandTxDoneParams_t* txDone );

/*!
 * \brief Initializes the channels masks and the channels.
 *
 * \param [IN] type Sets the initialization type.
 */
void RegionEU433InitDefaults( InitType_t type );

/*!
 * \brief Verifies a parameter.
 *
 * \param [IN] verify Pointer to the function parameters.
 *
 * \param [IN] type Sets the initialization type.
 *
 * \retval Returns true, if the parameter is valid.
 */
bool RegionEU433Verify( VerifyParams_t* verify, PhyAttribute_t phyAttribute );

/*!
 * \brief The function parses the input buffer and sets up the channels of the
 *        CF list.
 *
 * \param [IN] applyCFList Pointer to the function parameters.
 */
void RegionEU433ApplyCFList( ApplyCFListParams_t* applyCFList );

/*!
 * \brief Sets a channels mask.
 *
 * \param [IN] chanMaskSet Pointer to the function parameters.
 *
 * \retval Returns true, if the channels mask could be set.
 */
bool RegionEU433ChanMaskSet( ChanMaskSetParams_t* chanMaskSet );

/*!
 * \brief Calculates the next datarate to set, when ADR is on or off.
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
bool RegionEU433AdrNext( AdrNextParams_t* adrNext, int8_t* drOut, int8_t* txPowOut, uint32_t* adrAckCounter );

/*!
 * Computes the Rx window timeout and offset.
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
void RegionEU433ComputeRxWindowParameters( int8_t datarate, uint8_t minRxSymbols, uint32_t rxError, RxConfigParams_t *rxConfigParams );

/*!
 * \brief Configuration of the RX windows.
 *
 * \param [IN] rxConfig Pointer to the function parameters.
 *
 * \param [OUT] datarate The datarate index which was set.
 *
 * \retval Returns true, if the configuration was applied successfully.
 */
bool RegionEU433RxConfig( RxConfigParams_t* rxConfig, int8_t* datarate );

/*!
 * \brief TX configuration.
 *
 * \param [IN] txConfig Pointer to the function parameters.
 *
 * \param [OUT] txPower The tx power index which was set.
 *
 * \param [OUT] txTimeOnAir The time-on-air of the frame.
 *
 * \retval Returns true, if the configuration was applied successfully.
 */
bool RegionEU433TxConfig( TxConfigParams_t* txConfig, int8_t* txPower, TimerTime_t* txTimeOnAir );

/*!
 * \brief The function processes a Link ADR Request.
 *
 * \param [IN] linkAdrReq Pointer to the function parameters.
 *
 * \retval Returns the status of the operation, according to the LoRaMAC specification.
 */
uint8_t RegionEU433LinkAdrReq( LinkAdrReqParams_t* linkAdrReq, int8_t* drOut, int8_t* txPowOut, uint8_t* nbRepOut, uint8_t* nbBytesParsed );

/*!
 * \brief The function processes a RX Parameter Setup Request.
 *
 * \param [IN] rxParamSetupReq Pointer to the function parameters.
 *
 * \retval Returns the status of the operation, according to the LoRaMAC specification.
 */
uint8_t RegionEU433RxParamSetupReq( RxParamSetupReqParams_t* rxParamSetupReq );

/*!
 * \brief The function processes a Channel Request.
 *
 * \param [IN] newChannelReq Pointer to the function parameters.
 *
 * \retval Returns the status of the operation, according to the LoRaMAC specification.
 */
uint8_t RegionEU433NewChannelReq( NewChannelReqParams_t* newChannelReq );

/*!
 * \brief The function processes a TX ParamSetup Request.
 *
 * \param [IN] txParamSetupReq Pointer to the function parameters.
 *
 * \retval Returns the status of the operation, according to the LoRaMAC specification.
 *         Returns -1, if the functionality is not implemented. In this case, the end node
 *         shall not process the command.
 */
int8_t RegionEU433TxParamSetupReq( TxParamSetupReqParams_t* txParamSetupReq );

/*!
 * \brief The function processes a DlChannel Request.
 *
 * \param [IN] dlChannelReq Pointer to the function parameters.
 *
 * \retval Returns the status of the operation, according to the LoRaMAC specification.
 */
uint8_t RegionEU433DlChannelReq( DlChannelReqParams_t* dlChannelReq );

/*!
 * \brief Alternates the datarate of the channel for the join request.
 *
 * \param [IN] alternateDr Pointer to the function parameters.
 *
 * \retval Datarate to apply.
 */
int8_t RegionEU433AlternateDr( AlternateDrParams_t* alternateDr );

/*!
 * \brief Calculates the back-off time.
 *
 * \param [IN] calcBackOff Pointer to the function parameters.
 */
void RegionEU433CalcBackOff( CalcBackOffParams_t* calcBackOff );

/*!
 * \brief Searches and set the next random available channel
 *
 * \param [OUT] channel Next channel to use for TX.
 *
 * \param [OUT] time Time to wait for the next transmission according to the duty
 *              cycle.
 *
 * \param [OUT] aggregatedTimeOff Updates the aggregated time off.
 *
 * \retval Function status [1: OK, 0: Unable to find a channel on the current datarate]
 */
bool RegionEU433NextChannel( NextChanParams_t* nextChanParams, uint8_t* channel, TimerTime_t* time, TimerTime_t* aggregatedTimeOff );

/*!
 * \brief Adds a channel.
 *
 * \param [IN] channelAdd Pointer to the function parameters.
 *
 * \retval Status of the operation.
 */
LoRaMacStatus_t RegionEU433ChannelAdd( ChannelAddParams_t* channelAdd );

/*!
 * \brief Removes a channel.
 *
 * \param [IN] channelRemove Pointer to the function parameters.
 *
 * \retval Returns true, if the channel was removed successfully.
 */
bool RegionEU433ChannelsRemove( ChannelRemoveParams_t* channelRemove  );

/*!
 * \brief Sets the radio into continuous wave mode.
 *
 * \param [IN] continuousWave Pointer to the function parameters.
 */
void RegionEU433SetContinuousWave( ContinuousWaveParams_t* continuousWave );

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
uint8_t RegionEU433ApplyDrOffset( uint8_t downlinkDwellTime, int8_t dr, int8_t drOffset );

/*! \} defgroup REGIONEU433 */

#endif // __REGION_EU433_H__
