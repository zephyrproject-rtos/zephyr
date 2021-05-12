/*
  ______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2016 Semtech

Description: Driver for SX1280 devices

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis, Gregory Cristian and Matthieu Verdy
*/
#ifndef __SX1280_H__
#define __SX1280_H__

#include "radio.h"

/*!
 * \brief Enables/disables driver debug features
 */
#define SX1280_DEBUG                                0

/*!
 * \brief Hardware IO IRQ callback function definition
 */
class SX1280;
typedef void ( SX1280::*DioIrqHandler )( void );

/*!
 * \brief IRQ triggers callback function definition
 */
class SX1280Hal;
typedef void ( SX1280Hal::*Trigger )( void );

/*!
 * \brief Provides the frequency of the chip running on the radio and the frequency step
 *
 * \remark These defines are used for computing the frequency divider to set the RF frequency
 */
#define XTAL_FREQ                                   52000000
#define FREQ_STEP                                   ( ( double )( XTAL_FREQ / pow( 2.0, 18.0 ) ) )

/*!
 * \brief Compensation delay for SetAutoTx method in microseconds
 */
#define AUTO_TX_OFFSET                              33

/*!
 * \brief The address of the register holding the firmware version MSB
 */
#define REG_LR_FIRMWARE_VERSION_MSB                 0x0153

/*!
 * \brief The address of the register holding the first byte defining the CRC seed
 *
 * \remark Only used for packet types GFSK and Flrc
 */
#define REG_LR_CRCSEEDBASEADDR                      0x09C8

/*!
 * \brief The address of the register holding the first byte defining the CRC polynomial
 *
 * \remark Only used for packet types GFSK and Flrc
 */
#define REG_LR_CRCPOLYBASEADDR                      0x09C6

/*!
 * \brief The address of the register holding the first byte defining the whitening seed
 *
 * \remark Only used for packet types GFSK, FLRC and BLE
 */
#define REG_LR_WHITSEEDBASEADDR                     0x09C5

/*!
 * \brief The address of the register holding the ranging id check length
 *
 * \remark Only used for packet type Ranging
 */
#define REG_LR_RANGINGIDCHECKLENGTH                 0x0931

/*!
 * \brief The address of the register holding the device ranging id
 *
 * \remark Only used for packet type Ranging
 */
#define REG_LR_DEVICERANGINGADDR                    0x0916

/*!
 * \brief The address of the register holding the device ranging id
 *
 * \remark Only used for packet type Ranging
 */
#define REG_LR_REQUESTRANGINGADDR                   0x0912

/*!
 * \brief The address of the register holding ranging results configuration
 * and the corresponding mask
 *
 * \remark Only used for packet type Ranging
 */
#define REG_LR_RANGINGRESULTCONFIG                  0x0924
#define MASK_RANGINGMUXSEL                          0xCF

/*!
 * \brief The address of the register holding the first byte of ranging results
 * Only used for packet type Ranging
 */
#define REG_LR_RANGINGRESULTBASEADDR                0x0961

/*!
 * \brief The address of the register allowing to read ranging results
 *
 * \remark Only used for packet type Ranging
 */
#define REG_LR_RANGINGRESULTSFREEZE                 0x097F

/*!
 * \brief The address of the register holding the first byte of ranging calibration
 *
 * \remark Only used for packet type Ranging
 */
#define REG_LR_RANGINGRERXTXDELAYCAL                0x092C

/*!
 *\brief The address of the register holding the ranging filter window size
 *
 * \remark Only used for packet type Ranging
 */
#define REG_LR_RANGINGFILTERWINDOWSIZE              0x091E

/*!
 *\brief The address of the register to reset for clearing ranging filter
 *
 * \remark Only used for packet type Ranging
 */
#define REG_LR_RANGINGRESULTCLEARREG                0x0923


#define REG_RANGING_RSSI                            0x0964

/*!
 * \brief The default number of samples considered in built-in ranging filter
 */
#define DEFAULT_RANGING_FILTER_SIZE                 127

/*!
 * \brief The address of the register holding LORA packet parameters
 */
#define REG_LR_PACKETPARAMS                         0x903

/*!
 * \brief The address of the register holding payload length
 *
 * \remark Do NOT try to read it directly. Use GetRxBuffer( ) instead.
 */
#define REG_LR_PAYLOADLENGTH                        0x901

/*!
 * \brief The addresses of the registers holding SyncWords values
 *
 * \remark The addresses depends on the Packet Type in use, and not all
 *         SyncWords are available for every Packet Type
 */
#define REG_LR_SYNCWORDBASEADDRESS1                 0x09CE
#define REG_LR_SYNCWORDBASEADDRESS2                 0x09D3
#define REG_LR_SYNCWORDBASEADDRESS3                 0x09D8

/*!
 * \brief The MSB address and mask used to read the estimated frequency
 * error
 */
#define REG_LR_ESTIMATED_FREQUENCY_ERROR_MSB        0x0954
#define REG_LR_ESTIMATED_FREQUENCY_ERROR_MASK       0x0FFFFF

/*!
 * \brief Defines how many bit errors are tolerated in sync word detection
 */
#define REG_LR_SYNCWORDTOLERANCE                    0x09CD

/*!
 * \brief Register and mask for GFSK and BLE preamble length forcing
 */
#define REG_LR_PREAMBLELENGTH                       0x09C1
#define MASK_FORCE_PREAMBLELENGTH                   0x8F

/*!
 * \brief Register for MSB Access Address (BLE)
 */
#define REG_LR_BLE_ACCESS_ADDRESS                   0x09CF
#define BLE_ADVERTIZER_ACCESS_ADDRESS               0x8E89BED6

/*!
 * \brief Select high sensitivity versus power consumption
 */
#define REG_LNA_REGIME                              0x0891
#define MASK_LNA_REGIME                             0xC0

/*
 * \brief Register and mask controling the enabling of manual gain control
 */
#define REG_ENABLE_MANUAL_GAIN_CONTROL     0x089F
#define MASK_MANUAL_GAIN_CONTROL           0x80

/*!
 * \brief Register and mask controling the demodulation detection
 */
#define REG_DEMOD_DETECTION                0x0895
#define MASK_DEMOD_DETECTION               0xFE

/*!
 * Register and mask to set the manual gain parameter
 */
#define REG_MANUAL_GAIN_VALUE              0x089E
#define MASK_MANUAL_GAIN_VALUE             0xF0

/*!
 * \brief Represents the states of the radio
 */
typedef enum
{
    RF_IDLE                                 = 0x00,         //!< The radio is idle
    RF_RX_RUNNING,                                          //!< The radio is in reception state
    RF_TX_RUNNING,                                          //!< The radio is in transmission state
    RF_CAD,                                                 //!< The radio is doing channel activity detection
}RadioStates_t;

/*!
 * \brief Represents the operating mode the radio is actually running
 */
typedef enum
{
    MODE_SLEEP                              = 0x00,         //! The radio is in sleep mode
    MODE_CALIBRATION,                                       //! The radio is in calibration mode
    MODE_STDBY_RC,                                          //! The radio is in standby mode with RC oscillator
    MODE_STDBY_XOSC,                                        //! The radio is in standby mode with XOSC oscillator
    MODE_FS,                                                //! The radio is in frequency synthesis mode
    MODE_RX,                                                //! The radio is in receive mode
    MODE_TX,                                                //! The radio is in transmit mode
    MODE_CAD                                                //! The radio is in channel activity detection mode
}RadioOperatingModes_t;

/*!
 * \brief Declares the oscillator in use while in standby mode
 *
 * Using the STDBY_RC standby mode allow to reduce the energy consumption
 * STDBY_XOSC should be used for time critical applications
 */
typedef enum
{
    STDBY_RC                                = 0x00,
    STDBY_XOSC                              = 0x01,
}RadioStandbyModes_t;

/*!
 * \brief Declares the power regulation used to power the device
 *
 * This command allows the user to specify if DC-DC or LDO is used for power regulation.
 * Using only LDO implies that the Rx or Tx current is doubled
 */
typedef enum
{
    USE_LDO                                 = 0x00,         //! Use LDO (default value)
    USE_DCDC                                = 0x01,         //! Use DCDC
}RadioRegulatorModes_t;

/*!
 * \brief Represents the possible packet type (i.e. modem) used
 */
typedef enum
{
    PACKET_TYPE_GFSK                        = 0x00,
    PACKET_TYPE_LORA,
    PACKET_TYPE_RANGING,
    PACKET_TYPE_FLRC,
    PACKET_TYPE_BLE,
    PACKET_TYPE_NONE                        = 0x0F,
}RadioPacketTypes_t;

/*!
 * \brief Represents the ramping time for power amplifier
 */
typedef enum
{
    RADIO_RAMP_02_US                        = 0x00,
    RADIO_RAMP_04_US                        = 0x20,
    RADIO_RAMP_06_US                        = 0x40,
    RADIO_RAMP_08_US                        = 0x60,
    RADIO_RAMP_10_US                        = 0x80,
    RADIO_RAMP_12_US                        = 0xA0,
    RADIO_RAMP_16_US                        = 0xC0,
    RADIO_RAMP_20_US                        = 0xE0,
}RadioRampTimes_t;

/*!
 * \brief Represents the number of symbols to be used for channel activity detection operation
 */
typedef enum
{
    LORA_CAD_01_SYMBOL                      = 0x00,
    LORA_CAD_02_SYMBOLS                     = 0x20,
    LORA_CAD_04_SYMBOLS                     = 0x40,
    LORA_CAD_08_SYMBOLS                     = 0x60,
    LORA_CAD_16_SYMBOLS                     = 0x80,
}RadioLoRaCadSymbols_t;

/*!
 * \brief Represents the possible combinations of bitrate and bandwidth for
 *        GFSK and BLE packet types
 *
 * The bitrate is expressed in Mb/s and the bandwidth in MHz
 */
typedef enum
{
    GFSK_BLE_BR_2_000_BW_2_4                = 0x04,
    GFSK_BLE_BR_1_600_BW_2_4                = 0x28,
    GFSK_BLE_BR_1_000_BW_2_4                = 0x4C,
    GFSK_BLE_BR_1_000_BW_1_2                = 0x45,
    GFSK_BLE_BR_0_800_BW_2_4                = 0x70,
    GFSK_BLE_BR_0_800_BW_1_2                = 0x69,
    GFSK_BLE_BR_0_500_BW_1_2                = 0x8D,
    GFSK_BLE_BR_0_500_BW_0_6                = 0x86,
    GFSK_BLE_BR_0_400_BW_1_2                = 0xB1,
    GFSK_BLE_BR_0_400_BW_0_6                = 0xAA,
    GFSK_BLE_BR_0_250_BW_0_6                = 0xCE,
    GFSK_BLE_BR_0_250_BW_0_3                = 0xC7,
    GFSK_BLE_BR_0_125_BW_0_3                = 0xEF,
}RadioGfskBleBitrates_t;

/*!
 * \brief Represents the modulation index used in GFSK and BLE packet
 *        types
 */
typedef enum
{
    GFSK_BLE_MOD_IND_0_35                   =  0,
    GFSK_BLE_MOD_IND_0_50                   =  1,
    GFSK_BLE_MOD_IND_0_75                   =  2,
    GFSK_BLE_MOD_IND_1_00                   =  3,
    GFSK_BLE_MOD_IND_1_25                   =  4,
    GFSK_BLE_MOD_IND_1_50                   =  5,
    GFSK_BLE_MOD_IND_1_75                   =  6,
    GFSK_BLE_MOD_IND_2_00                   =  7,
    GFSK_BLE_MOD_IND_2_25                   =  8,
    GFSK_BLE_MOD_IND_2_50                   =  9,
    GFSK_BLE_MOD_IND_2_75                   = 10,
    GFSK_BLE_MOD_IND_3_00                   = 11,
    GFSK_BLE_MOD_IND_3_25                   = 12,
    GFSK_BLE_MOD_IND_3_50                   = 13,
    GFSK_BLE_MOD_IND_3_75                   = 14,
    GFSK_BLE_MOD_IND_4_00                   = 15,
}RadioGfskBleModIndexes_t;

/*!
 * \brief Represents the possible combination of bitrate and bandwidth for FLRC
 *        packet type
 *
 * The bitrate is in Mb/s and the bitrate in MHz
 */
typedef enum
{
    FLRC_BR_1_300_BW_1_2                    = 0x45,
    FLRC_BR_1_040_BW_1_2                    = 0x69,
    FLRC_BR_0_650_BW_0_6                    = 0x86,
    FLRC_BR_0_520_BW_0_6                    = 0xAA,
    FLRC_BR_0_325_BW_0_3                    = 0xC7,
    FLRC_BR_0_260_BW_0_3                    = 0xEB,
}RadioFlrcBitrates_t;

/*!
 * \brief Represents the possible values for coding rate parameter in FLRC
 *        packet type
 */
typedef enum
{
    FLRC_CR_1_2                             = 0x00,
    FLRC_CR_3_4                             = 0x02,
    FLRC_CR_1_0                             = 0x04,
}RadioFlrcCodingRates_t;

/*!
 * \brief Represents the modulation shaping parameter for GFSK, FLRC and BLE
 *        packet types
 */
typedef enum
{
    RADIO_MOD_SHAPING_BT_OFF                = 0x00,         //! No filtering
    RADIO_MOD_SHAPING_BT_1_0                = 0x10,
    RADIO_MOD_SHAPING_BT_0_5                = 0x20,
}RadioModShapings_t;

/*!
 * \brief Represents the possible spreading factor values in LORA packet types
 */
typedef enum
{
    LORA_SF5                                = 0x50,
    LORA_SF6                                = 0x60,
    LORA_SF7                                = 0x70,
    LORA_SF8                                = 0x80,
    LORA_SF9                                = 0x90,
    LORA_SF10                               = 0xA0,
    LORA_SF11                               = 0xB0,
    LORA_SF12                               = 0xC0,
}RadioLoRaSpreadingFactors_t;

/*!
 * \brief Represents the bandwidth values for LORA packet type
 */
typedef enum
{
    LORA_BW_0200                            = 0x34,
    LORA_BW_0400                            = 0x26,
    LORA_BW_0800                            = 0x18,
    LORA_BW_1600                            = 0x0A,
}RadioLoRaBandwidths_t;

/*!
 * \brief Represents the coding rate values for LORA packet type
 */
typedef enum
{
    LORA_CR_4_5                             = 0x01,
    LORA_CR_4_6                             = 0x02,
    LORA_CR_4_7                             = 0x03,
    LORA_CR_4_8                             = 0x04,
    LORA_CR_LI_4_5                          = 0x05,
    LORA_CR_LI_4_6                          = 0x06,
    LORA_CR_LI_4_7                          = 0x07,
}RadioLoRaCodingRates_t;

/*!
 * \brief Represents the preamble length values for GFSK and FLRC packet
 *        types
 */
typedef enum
{
    PREAMBLE_LENGTH_04_BITS                 = 0x00,         //!< Preamble length: 04 bits
    PREAMBLE_LENGTH_08_BITS                 = 0x10,         //!< Preamble length: 08 bits
    PREAMBLE_LENGTH_12_BITS                 = 0x20,         //!< Preamble length: 12 bits
    PREAMBLE_LENGTH_16_BITS                 = 0x30,         //!< Preamble length: 16 bits
    PREAMBLE_LENGTH_20_BITS                 = 0x40,         //!< Preamble length: 20 bits
    PREAMBLE_LENGTH_24_BITS                 = 0x50,         //!< Preamble length: 24 bits
    PREAMBLE_LENGTH_28_BITS                 = 0x60,         //!< Preamble length: 28 bits
    PREAMBLE_LENGTH_32_BITS                 = 0x70,         //!< Preamble length: 32 bits
}RadioPreambleLengths_t;

/*!
 * \brief Represents the SyncWord length for FLRC packet type
 */
typedef enum
{
    FLRC_NO_SYNCWORD                        = 0x00,
    FLRC_SYNCWORD_LENGTH_4_BYTE             = 0x04,
}RadioFlrcSyncWordLengths_t;

/*!
 * \brief The length of sync words for GFSK packet type
 */
typedef enum
{
    GFSK_SYNCWORD_LENGTH_1_BYTE             = 0x00,         //!< Sync word length: 1 byte
    GFSK_SYNCWORD_LENGTH_2_BYTE             = 0x02,         //!< Sync word length: 2 bytes
    GFSK_SYNCWORD_LENGTH_3_BYTE             = 0x04,         //!< Sync word length: 3 bytes
    GFSK_SYNCWORD_LENGTH_4_BYTE             = 0x06,         //!< Sync word length: 4 bytes
    GFSK_SYNCWORD_LENGTH_5_BYTE             = 0x08,         //!< Sync word length: 5 bytes
}RadioSyncWordLengths_t;

/*!
 * \brief Represents the possible combinations of SyncWord correlators
 *        activated for GFSK and FLRC packet types
 */
typedef enum
{
    RADIO_RX_MATCH_SYNCWORD_OFF             = 0x00,         //!< No correlator turned on, i.e. do not search for SyncWord
    RADIO_RX_MATCH_SYNCWORD_1               = 0x10,
    RADIO_RX_MATCH_SYNCWORD_2               = 0x20,
    RADIO_RX_MATCH_SYNCWORD_1_2             = 0x30,
    RADIO_RX_MATCH_SYNCWORD_3               = 0x40,
    RADIO_RX_MATCH_SYNCWORD_1_3             = 0x50,
    RADIO_RX_MATCH_SYNCWORD_2_3             = 0x60,
    RADIO_RX_MATCH_SYNCWORD_1_2_3           = 0x70,
}RadioSyncWordRxMatchs_t;

/*!
 *  \brief Radio packet length mode for GFSK and FLRC packet types
 */
typedef enum
{
    RADIO_PACKET_FIXED_LENGTH               = 0x00,         //!< The packet is known on both sides, no header included in the packet
    RADIO_PACKET_VARIABLE_LENGTH            = 0x20,         //!< The packet is on variable size, header included
}RadioPacketLengthModes_t;

/*!
 * \brief Represents the CRC length for GFSK and FLRC packet types
 *
 * \warning Not all configurations are available for both GFSK and FLRC
 *          packet type. Refer to the datasheet for possible configuration.
 */
typedef enum
{
    RADIO_CRC_OFF                           = 0x00,         //!< No CRC in use
    RADIO_CRC_1_BYTES                       = 0x10,
    RADIO_CRC_2_BYTES                       = 0x20,
    RADIO_CRC_3_BYTES                       = 0x30,
}RadioCrcTypes_t;

/*!
 * \brief Radio whitening mode activated or deactivated for GFSK, FLRC and
 *        BLE packet types
 */
typedef enum
{
    RADIO_WHITENING_ON                      = 0x00,
    RADIO_WHITENING_OFF                     = 0x08,
}RadioWhiteningModes_t;

/*!
 * \brief Holds the packet length mode of a LORA packet type
 */
typedef enum
{
    LORA_PACKET_VARIABLE_LENGTH             = 0x00,         //!< The packet is on variable size, header included
    LORA_PACKET_FIXED_LENGTH                = 0x80,         //!< The packet is known on both sides, no header included in the packet
    LORA_PACKET_EXPLICIT                    = LORA_PACKET_VARIABLE_LENGTH,
    LORA_PACKET_IMPLICIT                    = LORA_PACKET_FIXED_LENGTH,
}RadioLoRaPacketLengthsModes_t;

/*!
 * \brief Represents the CRC mode for LORA packet type
 */
typedef enum
{
    LORA_CRC_ON                             = 0x20,         //!< CRC activated
    LORA_CRC_OFF                            = 0x00,         //!< CRC not used
}RadioLoRaCrcModes_t;

/*!
 * \brief Represents the IQ mode for LORA packet type
 */
typedef enum
{
    LORA_IQ_NORMAL                          = 0x40,
    LORA_IQ_INVERTED                        = 0x00,
}RadioLoRaIQModes_t;

/*!
 * \brief Represents the length of the ID to check in ranging operation
 */
typedef enum
{
    RANGING_IDCHECK_LENGTH_08_BITS          = 0x00,
    RANGING_IDCHECK_LENGTH_16_BITS,
    RANGING_IDCHECK_LENGTH_24_BITS,
    RANGING_IDCHECK_LENGTH_32_BITS,
}RadioRangingIdCheckLengths_t;

/*!
 * \brief Represents the result type to be used in ranging operation
 */
typedef enum
{
    RANGING_RESULT_RAW                      = 0x00,
    RANGING_RESULT_AVERAGED                 = 0x01,
    RANGING_RESULT_DEBIASED                 = 0x02,
    RANGING_RESULT_FILTERED                 = 0x03,
}RadioRangingResultTypes_t;

/*!
 * \brief Represents the connection state for BLE packet type
 */
typedef enum
{
    BLE_PAYLOAD_LENGTH_MAX_31_BYTES         = 0x00,
    BLE_PAYLOAD_LENGTH_MAX_37_BYTES         = 0x20,
    BLE_TX_TEST_MODE                        = 0x40,
    BLE_PAYLOAD_LENGTH_MAX_255_BYTES        = 0x80,
}RadioBleConnectionStates_t;

/*!
 * \brief Represents the CRC field length for BLE packet type
 */
typedef enum
{
    BLE_CRC_OFF                             = 0x00,
    BLE_CRC_3B                              = 0x10,
}RadioBleCrcTypes_t;

/*!
 * \brief Represents the specific packets to use in BLE packet type
 */
typedef enum
{
    BLE_PRBS_9                              = 0x00,         //!< Pseudo Random Binary Sequence based on 9th degree polynomial
    BLE_PRBS_15                             = 0x0C,         //!< Pseudo Random Binary Sequence based on 15th degree polynomial
    BLE_EYELONG_1_0                         = 0x04,         //!< Repeated '11110000' sequence
    BLE_EYELONG_0_1                         = 0x18,         //!< Repeated '00001111' sequence
    BLE_EYESHORT_1_0                        = 0x08,         //!< Repeated '10101010' sequence
    BLE_EYESHORT_0_1                        = 0x1C,         //!< Repeated '01010101' sequence
    BLE_ALL_1                               = 0x10,         //!< Repeated '11111111' sequence
    BLE_ALL_0                               = 0x14,         //!< Repeated '00000000' sequence
}RadioBleTestPayloads_t;

/*!
 * \brief Represents the interruption masks available for the radio
 *
 * \remark Note that not all these interruptions are available for all packet types
 */
typedef enum
{
    IRQ_RADIO_NONE                          = 0x0000,
    IRQ_TX_DONE                             = 0x0001,
    IRQ_RX_DONE                             = 0x0002,
    IRQ_SYNCWORD_VALID                      = 0x0004,
    IRQ_SYNCWORD_ERROR                      = 0x0008,
    IRQ_HEADER_VALID                        = 0x0010,
    IRQ_HEADER_ERROR                        = 0x0020,
    IRQ_CRC_ERROR                           = 0x0040,
    IRQ_RANGING_SLAVE_RESPONSE_DONE         = 0x0080,
    IRQ_RANGING_SLAVE_REQUEST_DISCARDED     = 0x0100,
    IRQ_RANGING_MASTER_RESULT_VALID         = 0x0200,
    IRQ_RANGING_MASTER_TIMEOUT              = 0x0400,
    IRQ_RANGING_SLAVE_REQUEST_VALID         = 0x0800,
    IRQ_CAD_DONE                            = 0x1000,
    IRQ_CAD_DETECTED                        = 0x2000,
    IRQ_RX_TX_TIMEOUT                       = 0x4000,
    IRQ_PREAMBLE_DETECTED                   = 0x8000,
    IRQ_RADIO_ALL                           = 0xFFFF,
}RadioIrqMasks_t;

/*!
 * \brief Represents the digital input/output of the radio
 */
typedef enum
{
    RADIO_DIO1                              = 0x02,
    RADIO_DIO2                              = 0x04,
    RADIO_DIO3                              = 0x08,
}RadioDios_t;

/*!
 * \brief Represents the tick size available for Rx/Tx timeout operations
 */
typedef enum
{
    RADIO_TICK_SIZE_0015_US                 = 0x00,
    RADIO_TICK_SIZE_0062_US                 = 0x01,
    RADIO_TICK_SIZE_1000_US                 = 0x02,
    RADIO_TICK_SIZE_4000_US                 = 0x03,
}RadioTickSizes_t;

/*!
 * \brief Represents the role of the radio during ranging operations
 */
typedef enum
{
    RADIO_RANGING_ROLE_SLAVE                = 0x00,
    RADIO_RANGING_ROLE_MASTER               = 0x01,
}RadioRangingRoles_t;

/*!
 * \brief Represents the possible Mask settings for sensitivity selection
 */
typedef enum
{
    LNA_LOW_POWER_MODE,
    LNA_HIGH_SENSITIVITY_MODE,
}RadioLnaSettings_t;

/*!
 * \brief Represents an amount of time measurable by the radio clock
 *
 * @code
 * Time = PeriodBase * PeriodBaseCount
 * Example:
 * PeriodBase = RADIO_TICK_SIZE_4000_US( 4 ms )
 * PeriodBaseCount = 1000
 * Time = 4e-3 * 1000 = 4 seconds
 * @endcode
 */
typedef struct TickTime_s
{
    RadioTickSizes_t PeriodBase;                            //!< The base time of ticktime
    /*!
     * \brief The number of periodBase for ticktime
     * Special values are:
     *     - 0x0000 for single mode
     *     - 0xFFFF for continuous mode
     */
    uint16_t PeriodBaseCount;
}TickTime_t;

/*!
* \brief RX_TX_CONTINUOUS and RX_TX_SINGLE are two particular values for TickTime.
* The former keep the radio in Rx or Tx mode, even after successfull reception
* or transmission. It should never generate Timeout interrupt.
* The later let the radio enought time to make one reception or transmission.
* No Timeout interrupt is generated, and the radio fall in StandBy mode after
* reception or transmission.
*/
#define RX_TX_CONTINUOUS ( TickTime_t ){ RADIO_TICK_SIZE_0015_US, 0xFFFF }
#define RX_TX_SINGLE     ( TickTime_t ){ RADIO_TICK_SIZE_0015_US, 0 }

/*!
 * \brief The type describing the modulation parameters for every packet types
 */
typedef struct
{
    RadioPacketTypes_t                    PacketType;       //!< Packet to which the modulation parameters are referring to.
    struct
    {
        /*!
         * \brief Holds the GFSK modulation parameters
         *
         * In GFSK modulation, the bit-rate and bandwidth are linked together. In this structure, its values are set using the same token.
         */
        struct
        {
            RadioGfskBleBitrates_t    BitrateBandwidth;     //!< The bandwidth and bit-rate values for BLE and GFSK modulations
            RadioGfskBleModIndexes_t  ModulationIndex;      //!< The coding rate for BLE and GFSK modulations
            RadioModShapings_t        ModulationShaping;    //!< The modulation shaping for BLE and GFSK modulations
        }Gfsk;
        /*!
         * \brief Holds the LORA modulation parameters
         *
         * LORA modulation is defined by Spreading Factor (SF), Bandwidth and Coding Rate
         */
        struct
        {
            RadioLoRaSpreadingFactors_t  SpreadingFactor;   //!< Spreading Factor for the LORA modulation
            RadioLoRaBandwidths_t        Bandwidth;         //!< Bandwidth for the LORA modulation
            RadioLoRaCodingRates_t       CodingRate;        //!< Coding rate for the LORA modulation
        }LoRa;
        /*!
         * \brief Holds the FLRC modulation parameters
         *
         * In FLRC modulation, the bit-rate and bandwidth are linked together. In this structure, its values are set using the same token.
         */
        struct
        {
            RadioFlrcBitrates_t          BitrateBandwidth;  //!< The bandwidth and bit-rate values for FLRC modulation
            RadioFlrcCodingRates_t       CodingRate;        //!< The coding rate for FLRC modulation
            RadioModShapings_t           ModulationShaping; //!< The modulation shaping for FLRC modulation
        }Flrc;
        /*!
         * \brief Holds the BLE modulation parameters
         *
         * In BLE modulation, the bit-rate and bandwidth are linked together. In this structure, its values are set using the same token.
         */
        struct
        {
            RadioGfskBleBitrates_t       BitrateBandwidth;  //!< The bandwidth and bit-rate values for BLE and GFSK modulations
            RadioGfskBleModIndexes_t     ModulationIndex;   //!< The coding rate for BLE and GFSK modulations
            RadioModShapings_t           ModulationShaping; //!< The modulation shaping for BLE and GFSK modulations
        }Ble;
    }Params;                                                //!< Holds the modulation parameters structure
}ModulationParams_t;

/*!
 * \brief The type describing the packet parameters for every packet types
 */
typedef struct
{
    RadioPacketTypes_t                    PacketType;       //!< Packet to which the packet parameters are referring to.
    struct
    {
        /*!
         * \brief Holds the GFSK packet parameters
         */
        struct
        {
            RadioPreambleLengths_t       PreambleLength;    //!< The preamble length for GFSK packet type
            RadioSyncWordLengths_t       SyncWordLength;    //!< The synchronization word length for GFSK packet type
            RadioSyncWordRxMatchs_t      SyncWordMatch;     //!< The synchronization correlator to use to check synchronization word
            RadioPacketLengthModes_t     HeaderType;        //!< If the header is explicit, it will be transmitted in the GFSK packet. If the header is implicit, it will not be transmitted
            uint8_t                      PayloadLength;     //!< Size of the payload in the GFSK packet
            RadioCrcTypes_t              CrcLength;         //!< Size of the CRC block in the GFSK packet
            RadioWhiteningModes_t        Whitening;         //!< Usage of whitening on payload and CRC blocks plus header block if header type is variable
        }Gfsk;
        /*!
         * \brief Holds the LORA packet parameters
         */
        struct
        {
            uint8_t                       PreambleLength;   //!< The preamble length is the number of LORA symbols in the preamble. To set it, use the following formula @code Number of symbols = PreambleLength[3:0] * ( 2^PreambleLength[7:4] ) @endcode
            RadioLoRaPacketLengthsModes_t HeaderType;       //!< If the header is explicit, it will be transmitted in the LORA packet. If the header is implicit, it will not be transmitted
            uint8_t                       PayloadLength;    //!< Size of the payload in the LORA packet
            RadioLoRaCrcModes_t           Crc;              //!< Size of CRC block in LORA packet
            RadioLoRaIQModes_t            InvertIQ;         //!< Allows to swap IQ for LORA packet
        }LoRa;
        /*!
         * \brief Holds the FLRC packet parameters
         */
        struct
        {
            RadioPreambleLengths_t       PreambleLength;    //!< The preamble length for FLRC packet type
            RadioFlrcSyncWordLengths_t   SyncWordLength;    //!< The synchronization word length for FLRC packet type
            RadioSyncWordRxMatchs_t      SyncWordMatch;     //!< The synchronization correlator to use to check synchronization word
            RadioPacketLengthModes_t     HeaderType;        //!< If the header is explicit, it will be transmitted in the FLRC packet. If the header is implicit, it will not be transmitted.
            uint8_t                      PayloadLength;     //!< Size of the payload in the FLRC packet
            RadioCrcTypes_t              CrcLength;         //!< Size of the CRC block in the FLRC packet
            RadioWhiteningModes_t        Whitening;         //!< Usage of whitening on payload and CRC blocks plus header block if header type is variable
        }Flrc;
        /*!
         * \brief Holds the BLE packet parameters
         */
        struct
        {
            RadioBleConnectionStates_t    ConnectionState;   //!< The BLE state
            RadioBleCrcTypes_t            CrcLength;         //!< Size of the CRC block in the BLE packet
            RadioBleTestPayloads_t        BleTestPayload;    //!< Special BLE payload for test purpose
            RadioWhiteningModes_t         Whitening;         //!< Usage of whitening on PDU and CRC blocks of BLE packet
        }Ble;
    }Params;                                                 //!< Holds the packet parameters structure
}PacketParams_t;

/*!
 * \brief Represents the packet status for every packet type
 */
typedef struct
{
    RadioPacketTypes_t                    packetType;        //!< Packet to which the packet status are referring to.
    union
    {
        struct
        {
            int8_t RssiSync;                                //!< The RSSI measured on last packet
            struct
            {
                bool SyncError :1;                          //!< SyncWord error on last packet
                bool LengthError :1;                        //!< Length error on last packet
                bool CrcError :1;                           //!< CRC error on last packet
                bool AbortError :1;                         //!< Abort error on last packet
                bool HeaderReceived :1;                     //!< Header received on last packet
                bool PacketReceived :1;                     //!< Packet received
                bool PacketControlerBusy :1;                //!< Packet controller busy
            }ErrorStatus;                                   //!< The error status Byte
            struct
            {
                bool RxNoAck :1;                            //!< No acknowledgment received for Rx with variable length packets
                bool PacketSent :1;                         //!< Packet sent, only relevant in Tx mode
            }TxRxStatus;                                    //!< The Tx/Rx status Byte
            uint8_t SyncAddrStatus :3;                      //!< The id of the correlator who found the packet
        }Gfsk;
        struct
        {
            int8_t RssiPkt;                                 //!< The RSSI of the last packet
            int8_t SnrPkt;                                  //!< The SNR of the last packet
        }LoRa;
        struct
        {
            int8_t RssiSync;                                //!< The RSSI of the last packet
            struct
            {
                bool SyncError :1;                          //!< SyncWord error on last packet
                bool LengthError :1;                        //!< Length error on last packet
                bool CrcError :1;                           //!< CRC error on last packet
                bool AbortError :1;                         //!< Abort error on last packet
                bool HeaderReceived :1;                     //!< Header received on last packet
                bool PacketReceived :1;                     //!< Packet received
                bool PacketControlerBusy :1;                //!< Packet controller busy
            }ErrorStatus;                                   //!< The error status Byte
            struct
            {
                uint8_t RxPid :2;                           //!< PID of the Rx
                bool RxNoAck :1;                            //!< No acknowledgment received for Rx with variable length packets
                bool RxPidErr :1;                           //!< Received PID error
                bool PacketSent :1;                         //!< Packet sent, only relevant in Tx mode
            }TxRxStatus;                                    //!< The Tx/Rx status Byte
            uint8_t SyncAddrStatus :3;                      //!< The id of the correlator who found the packet
        }Flrc;
        struct
        {
            int8_t RssiSync;                                //!< The RSSI of the last packet
            struct
            {
                bool SyncError :1;                          //!< SyncWord error on last packet
                bool LengthError :1;                        //!< Length error on last packet
                bool CrcError :1;                           //!< CRC error on last packet
                bool AbortError :1;                         //!< Abort error on last packet
                bool HeaderReceived :1;                     //!< Header received on last packet
                bool PacketReceived :1;                     //!< Packet received
                bool PacketControlerBusy :1;                //!< Packet controller busy
            }ErrorStatus;                                   //!< The error status Byte
            struct
            {
                bool PacketSent :1;                         //!< Packet sent, only relevant in Tx mode
            }TxRxStatus;                                    //!< The Tx/Rx status Byte
            uint8_t SyncAddrStatus :3;                      //!< The id of the correlator who found the packet
        }Ble;
    };
}PacketStatus_t;

/*!
 * \brief Represents the Rx internal counters values when GFSK or LORA packet type is used
 */
typedef struct
{
    RadioPacketTypes_t                    packetType;       //!< Packet to which the packet status are referring to.
    union
    {
        struct
        {
            uint16_t PacketReceived;                        //!< Number of received packets
            uint16_t CrcError;                              //!< Number of CRC errors
            uint16_t LengthError;                           //!< Number of length errors
            uint16_t SyncwordError;                         //!< Number of sync-word errors
        }Gfsk;
        struct
        {
            uint16_t PacketReceived;                        //!< Number of received packets
            uint16_t CrcError;                              //!< Number of CRC errors
            uint16_t HeaderValid;                           //!< Number of valid headers
        }LoRa;
    };
}RxCounter_t;

/*!
 * \brief Represents a calibration configuration
 */
typedef struct
{
    uint8_t RC64KEnable    : 1;                             //!< Calibrate RC64K clock
    uint8_t RC13MEnable    : 1;                             //!< Calibrate RC13M clock
    uint8_t PLLEnable      : 1;                             //!< Calibrate PLL
    uint8_t ADCPulseEnable : 1;                             //!< Calibrate ADC Pulse
    uint8_t ADCBulkNEnable : 1;                             //!< Calibrate ADC bulkN
    uint8_t ADCBulkPEnable : 1;                             //!< Calibrate ADC bulkP
}CalibrationParams_t;

/*!
 * \brief Represents a sleep mode configuration
 */
typedef struct
{
    uint8_t WakeUpRTC               : 1;                    //!< Get out of sleep mode if wakeup signal received from RTC
    uint8_t InstructionRamRetention : 1;                    //!< InstructionRam is conserved during sleep
    uint8_t DataBufferRetention     : 1;                    //!< Data buffer is conserved during sleep
    uint8_t DataRamRetention        : 1;                    //!< Data ram is conserved during sleep
}SleepParams_t;

/*!
 * \brief Represents the SX1280 and its features
 *
 * It implements the commands the SX1280 can understands
 */
class SX1280 : public Radio
{
public:
    /*!
     * \brief Instantiates a SX1280 object and provides API functions to communicates with the radio
     *
     * \param [in]  callbacks      Pointer to the callbacks structure defining
     *                             all callbacks function pointers
     */
    SX1280( RadioCallbacks_t *callbacks ):
        // The class members are value-initialiazed in member-initilaizer list
        Radio( callbacks ), OperatingMode( MODE_STDBY_RC ), PacketType( PACKET_TYPE_NONE ),
        LoRaBandwidth( LORA_BW_1600 ), IrqState( false ), PollingMode( false )
    {
        this->dioIrq        = &SX1280::OnDioIrq;

        // Warning: this constructor set the LoRaBandwidth member to a valid
        // value, but it is not related to the actual radio configuration!
    }

    virtual ~SX1280( )
    {
    }

private:
    /*!
     * \brief Holds the internal operating mode of the radio
     */
    RadioOperatingModes_t OperatingMode;

    /*!
     * \brief Stores the current packet type set in the radio
     */
    RadioPacketTypes_t PacketType;

    /*!
     * \brief Stores the current LORA bandwidth set in the radio
     */
    RadioLoRaBandwidths_t LoRaBandwidth;

    /*!
     * \brief Holds a flag raised on radio interrupt
     */
    bool IrqState;

    /*!
     * \brief Hardware DIO IRQ functions
     */
    DioIrqHandler dioIrq;

    /*!
     * \brief Holds the polling state of the driver
     */
    bool PollingMode;

    /*! 
     * \brief Compute the two's complement for a register of size lower than
     *        32bits
     *
     * \param [in]  num           The register to be two's complemented
     * \param [in]  bitCnt        The position of the sign bit
     */
    static int32_t complement2( const uint32_t num, const uint8_t bitCnt );

    /*!
     * \brief Returns the value of LoRa bandwidth from driver's value
     *
     * The value is returned in Hz so that it can be represented as an integer
     * type. Most computation should be done as integer to reduce floating
     * point related errors.
     *
     * \retval loRaBw             The value of the current bandwidth in Hz
     */
    int32_t GetLoRaBandwidth( void );

protected:
    /*!
     * \brief Sets a function to be triggered on radio interrupt
     *
     * \param [in]  irqHandler    A pointer to a function to be run on interrupt
     *                            from the radio
     */
    virtual void IoIrqInit( DioIrqHandler irqHandler ) = 0;

    /*!
     * \brief DIOs interrupt callback
     *
     * \remark Called to handle all 3 DIOs pins
     */
    void OnDioIrq( void );

    /*!
     * \brief Set the role of the radio during ranging operations
     *
     * \param [in]  role          Role of the radio
     */
    void SetRangingRole( RadioRangingRoles_t role );

public:
    /*!
     * \brief Initializes the radio driver
     */
    void Init( void );

    /*!
     * \brief Set the driver in polling mode.
     *
     * In polling mode the application is responsible to call ProcessIrqs( ) to
     * execute callbacks functions.
     * The default mode is Interrupt Mode.
     * @code
     * // Initializations and callbacks declaration/definition
     * radio = SX1280( mosi, miso, sclk, nss, busy, int1, int2, int3, rst, &callbacks );
     * radio.Init( );
     * radio.SetPollingMode( );
     *
     * while( true )
     * {
     *                            //     IRQ processing is automatically done
     *     radio.ProcessIrqs( );  // <-- here, as well as callback functions
     *                            //     calls
     *     // Do some applicative work
     * }
     * @endcode
     *
     * \see SX1280::SetInterruptMode
     */
    void SetPollingMode( void );

    /*!
     * \brief Set the driver in interrupt mode.
     *
     * In interrupt mode, the driver communicate with the radio during the
     * interruption by direct calls to ProcessIrqs( ). The main advantage is
     * the possibility to have low power application architecture.
     * This is the default mode.
     * @code
     * // Initializations and callbacks declaration/definition
     * radio = SX1280( mosi, miso, sclk, nss, busy, int1, int2, int3, rst, &callbacks );
     * radio.Init( );
     * radio.SetInterruptMode( );   // Optionnal. Driver default behavior
     *
     * while( true )
     * {
     *     // Do some applicative work
     * }
     * @endcode
     *
     * \see SX1280::SetPollingMode
     */
    void SetInterruptMode( void );

    /*!
     * \brief Initializes the radio registers to the recommended default values
     */
    void SetRegistersDefault( void );

    /*!
     * \brief Returns the current device firmware version
     *
     * \retval      version       Firmware version
     */
    virtual uint16_t GetFirmwareVersion( void );

    /*!
     * \brief Resets the radio
     */
    virtual void Reset( void ) = 0;

    /*!
     * \brief Wake-ups the radio from Sleep mode
     */
    virtual void Wakeup( void ) = 0;

    /*!
     * \brief Writes the given command to the radio
     *
     * \param [in]  opcode        Command opcode
     * \param [in]  buffer        Command parameters byte array
     * \param [in]  size          Command parameters byte array size
     */
    virtual void WriteCommand( RadioCommands_t opcode, uint8_t *buffer, uint16_t size ) = 0;

    /*!
     * \brief Reads the given command from the radio
     *
     * \param [in]  opcode        Command opcode
     * \param [in]  buffer        Command parameters byte array
     * \param [in]  size          Command parameters byte array size
     */
    virtual void ReadCommand( RadioCommands_t opcode, uint8_t *buffer, uint16_t size ) = 0;

    /*!
     * \brief Writes multiple radio registers starting at address
     *
     * \param [in]  address       First Radio register address
     * \param [in]  buffer        Buffer containing the new register's values
     * \param [in]  size          Number of registers to be written
     */
    virtual void WriteRegister( uint16_t address, uint8_t *buffer, uint16_t size ) = 0;

    /*!
     * \brief Writes the radio register at the specified address
     *
     * \param [in]  address       Register address
     * \param [in]  value         New register value
     */
    virtual void WriteRegister( uint16_t address, uint8_t value ) = 0;

    /*!
     * \brief Reads multiple radio registers starting at address
     *
     * \param [in]  address       First Radio register address
     * \param [out] buffer        Buffer where to copy the registers data
     * \param [in]  size          Number of registers to be read
     */
    virtual void ReadRegister( uint16_t address, uint8_t *buffer, uint16_t size ) = 0;

    /*!
     * \brief Reads the radio register at the specified address
     *
     * \param [in]  address       Register address
     *
     * \retval      data          Register value
     */
    virtual uint8_t ReadRegister( uint16_t address ) = 0;

    /*!
     * \brief Writes Radio Data Buffer with buffer of size starting at offset.
     *
     * \param [in]  offset        Offset where to start writing
     * \param [in]  buffer        Buffer pointer
     * \param [in]  size          Buffer size
     */
    virtual void WriteBuffer( uint8_t offset, uint8_t *buffer, uint8_t size ) = 0;

    /*!
     * \brief Reads Radio Data Buffer at offset to buffer of size
     *
     * \param [in]  offset        Offset where to start reading
     * \param [out] buffer        Buffer pointer
     * \param [in]  size          Buffer size
     */
    virtual void ReadBuffer( uint8_t offset, uint8_t *buffer, uint8_t size ) = 0;

    /*!
     * \brief Gets the current status of the radio DIOs
     *
     * \retval      status        [Bit #3: DIO3, Bit #2: DIO2,
     *                             Bit #1: DIO1, Bit #0: BUSY]
     */
    virtual uint8_t GetDioStatus( void ) = 0;

    /*!
     * \brief Gets the current Operation Mode of the Radio
     *
     * \retval      opMode        Last operating mode
     */
    virtual RadioOperatingModes_t GetOpMode( void );

    /*!
     * \brief Gets the current radio status
     *
     * \retval      status        Radio status
     */
    virtual RadioStatus_t GetStatus( void );

    /*!
     * \brief Sets the radio in sleep mode
     *
     * \param [in]  sleepConfig   The sleep configuration describing data
     *                            retention and RTC wake-up
     */
    void SetSleep( SleepParams_t sleepConfig );

    /*!
     * \brief Sets the radio in configuration mode
     *
     * \param [in]  mode          The standby mode to put the radio into
     */
    void SetStandby( RadioStandbyModes_t mode );

    /*!
     * \brief Sets the radio in FS mode
     */
    void SetFs( void );

    /*!
     * \brief Sets the radio in transmission mode
     *
     * \param [in]  timeout       Structure describing the transmission timeout value
     */
    void SetTx( TickTime_t timeout );

    /*!
     * \brief Sets the radio in reception mode
     *
     * \param [in]  timeout       Structure describing the reception timeout value
     */
    void SetRx( TickTime_t timeout );

    /*!
     * \brief Sets the Rx duty cycle management parameters
     *
     * \param [in]  periodBase             Base time for Rx and Sleep TickTime sequences
     * \param [in]  periodBaseCountRx      Number of base time for Rx TickTime sequence
     * \param [in]  periodBaseCountSleep   Number of base time for Sleep TickTime sequence
     */
    void SetRxDutyCycle( RadioTickSizes_t periodBase, uint16_t periodBaseCountRx, uint16_t periodBaseCountSleep );

    /*!
     * \brief Sets the radio in CAD mode
     *
     * \see SX1280::SetCadParams
     */
    void SetCad( void );

    /*!
     * \brief Sets the radio in continuous wave transmission mode
     */
    void SetTxContinuousWave( void );

    /*!
     * \brief Sets the radio in continuous preamble transmission mode
     */
    void SetTxContinuousPreamble( void );

    /*!
     * \brief Sets the radio for the given protocol
     *
     * \param [in]  packetType    [PACKET_TYPE_GFSK, PACKET_TYPE_LORA,
     *                             PACKET_TYPE_RANGING, PACKET_TYPE_FLRC,
     *                             PACKET_TYPE_BLE]
     *
     * \remark This method has to be called before SetRfFrequency,
     *         SetModulationParams and SetPacketParams
     */
    void SetPacketType( RadioPacketTypes_t packetType );

    /*!
     * \brief Gets the current radio protocol
     *
     * Default behavior return the packet type returned by the radio. To
     * reduce the SPI activity and return the packet type stored by the
     * driver, a second optional argument must be provided evaluating as true
     * boolean
     *
     * \param [in]  returnLocalCopy If true, the packet type returned is the last
     *                              saved in the driver
     *                              If false, the packet type is obtained from
     *                              the chip
     *                              Default: false
     *
     * \retval      packetType    [PACKET_TYPE_GENERIC, PACKET_TYPE_LORA,
     *                             PACKET_TYPE_RANGING, PACKET_TYPE_FLRC,
     *                             PACKET_TYPE_BLE, PACKET_TYPE_NONE]
     */
    RadioPacketTypes_t GetPacketType( bool returnLocalCopy = false );

    /*!
     * \brief Sets the RF frequency
     *
     * \param [in]  rfFrequency   RF frequency [Hz]
     */
    void SetRfFrequency( uint32_t rfFrequency );

    /*!
     * \brief Sets the transmission parameters
     *
     * \param [in]  power         RF output power [-18..13] dBm
     * \param [in]  rampTime      Transmission ramp up time
     */
    void SetTxParams( int8_t power, RadioRampTimes_t rampTime );

    /*!
     * \brief Sets the number of symbols to be used for Channel Activity
     *        Detection operation
     *
     * \param [in]  cadSymbolNum  The number of symbol to use for Channel Activity
     *                            Detection operations [LORA_CAD_01_SYMBOL, LORA_CAD_02_SYMBOLS,
     *                            LORA_CAD_04_SYMBOLS, LORA_CAD_08_SYMBOLS, LORA_CAD_16_SYMBOLS]
     */
    void SetCadParams( RadioLoRaCadSymbols_t cadSymbolNum );

    /*!
     * \brief Sets the data buffer base address for transmission and reception
     *
     * \param [in]  txBaseAddress Transmission base address
     * \param [in]  rxBaseAddress Reception base address
     */
    void SetBufferBaseAddresses( uint8_t txBaseAddress, uint8_t rxBaseAddress );

    /*!
     * \brief Set the modulation parameters
     *
     * \param [in]  modParams     A structure describing the modulation parameters
     */
    void SetModulationParams( ModulationParams_t *modParams );

    /*!
     * \brief Sets the packet parameters
     *
     * \param [in]  packetParams  A structure describing the packet parameters
     */
    void SetPacketParams( PacketParams_t *packetParams );

    /*!
     * \brief Gets the last received packet buffer status
     *
     * \param [out] rxPayloadLength       Last received packet payload length
     * \param [out] rxStartBufferPointer  Last received packet buffer address pointer
     */
    void GetRxBufferStatus( uint8_t *rxPayloadLength, uint8_t *rxStartBufferPointer );

    /*!
     * \brief Gets the last received packet status
     *
     * The packet status structure returned depends on the modem type selected
     *
     * \see PacketStatus_t for the description of availabled informations
     *
     * \param [out] packetStatus  A structure of packet status
     */
    void GetPacketStatus( PacketStatus_t *packetStatus );

    /*!
     * \brief Returns the instantaneous RSSI value for the last packet received
     *
     * \retval      rssiInst      Instantaneous RSSI
     */
    int8_t GetRssiInst( void );

    /*!
     * \brief   Sets the IRQ mask and DIO masks
     *
     * \param [in]  irqMask       General IRQ mask
     * \param [in]  dio1Mask      DIO1 mask
     * \param [in]  dio2Mask      DIO2 mask
     * \param [in]  dio3Mask      DIO3 mask
     */
    void SetDioIrqParams( uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask, uint16_t dio3Mask );

    /*!
     * \brief Returns the current IRQ status
     *
     * \retval      irqStatus     IRQ status
     */
    uint16_t GetIrqStatus( void );

    /*!
     * \brief Clears the IRQs
     *
     * \param [in]  irqMask       IRQ(s) to be cleared
     */
    void ClearIrqStatus( uint16_t irqMask );

    /*!
     * \brief Calibrates the given radio block
     *
     * \param [in]  calibParam    The description of blocks to be calibrated
     */
    void Calibrate( CalibrationParams_t calibParam );

    /*!
     * \brief Sets the power regulators operating mode
     *
     * \param [in]  mode          [0: LDO, 1:DC_DC]
     */
    void SetRegulatorMode( RadioRegulatorModes_t mode );

    /*!
     * \brief Saves the current selected modem configuration into data RAM
     */
    void SetSaveContext( void );

    /*!
     * \brief Sets the chip to automatically send a packet after the end of a packet reception
     *
     * \remark The offset is automatically compensated inside the function
     *
     * \param [in]  time          The delay in us after which a Tx is done
     */
    void SetAutoTx( uint16_t time );

    /*!
     * \brief Stop the chip to automatically send a packet after the end of a packet reception
     *        if previously activated with SX1280::SetAutoTx command
     */
    void StopAutoTx( void );

    /*!
     * \brief Sets the chip to stay in FS mode after sending a packet
     *
     * \param [in]  enableAutoFs  Turn on auto FS
     */
    void SetAutoFs( bool enableAutoFs );

    /*!
     * \brief Enables or disables long preamble detection mode
     *
     * \param [in]  enable        Turn on long preamble mode
     */
    void SetLongPreamble( bool enable );

    /*!
     * \brief Saves the payload to be send in the radio buffer
     *
     * \param [in]  payload       A pointer to the payload
     * \param [in]  size          The size of the payload
     * \param [in]  offset        The address in FIFO where writting first byte (default = 0x00)
     */
    void SetPayload( uint8_t *payload, uint8_t size, uint8_t offset = 0x00 );

    /*!
     * \brief Reads the payload received. If the received payload is longer
     * than maxSize, then the method returns 1 and do not set size and payload.
     *
     * \param [out] payload       A pointer to a buffer into which the payload will be copied
     * \param [out] size          A pointer to the size of the payload received
     * \param [in]  maxSize       The maximal size allowed to copy into the buffer
     */
    uint8_t GetPayload( uint8_t *payload, uint8_t *size, uint8_t maxSize );

    /*!
     * \brief Sends a payload
     *
     * \param [in]  payload       A pointer to the payload to send
     * \param [in]  size          The size of the payload to send
     * \param [in]  timeout       The timeout for Tx operation
     * \param [in]  offset        The address in FIFO where writting first byte (default = 0x00)
     */
    void SendPayload( uint8_t *payload, uint8_t size, TickTime_t timeout, uint8_t offset = 0x00 );

    /*!
     * \brief Sets the Sync Word given by index used in GFSK, FLRC and BLE protocols
     *
     * \remark 5th byte isn't used in FLRC and BLE protocols
     *
     * \param [in]  syncWordIdx   Index of SyncWord to be set [1..3]
     * \param [in]  syncWord      SyncWord bytes ( 5 bytes )
     *
     * \retval      status        [0: OK, 1: NOK]
     */
    uint8_t SetSyncWord( uint8_t syncWordIdx, uint8_t *syncWord );

    /*!
     * \brief Defines how many error bits are tolerated in sync word detection
     *
     * \param [in]  errorBits     Number of error bits supported to validate the Sync word detection
     *                            ( default is 4 bit, minimum is 1 bit )
     */
    void SetSyncWordErrorTolerance( uint8_t errorBits );

    /*!
     * \brief Sets the Initial value for the LFSR used for the CRC calculation
     *
     * \param [in]  seed          Initial LFSR value ( 4 bytes )
     *
     * \retval      updated       [0: failure, 1: success]
     *
     */
    uint8_t SetCrcSeed( uint8_t *seed );

    /*!
     * \brief Set the Access Address field of BLE packet
     *
     * \param [in]  accessAddress The access address to be used for next BLE packet sent
     *
     * \see SX1280::SetBleAdvertizerAccessAddress
     */
    void SetBleAccessAddress( uint32_t accessAddress );

    /*!
     * \brief Set the Access Address for Advertizer BLE packets
     *
     * All advertizer BLE packets must use a particular value for Access
     * Address field. This method sets it.
     *
     * \see SX1280::SetBleAccessAddress
     */
    void SetBleAdvertizerAccessAddress( void );

    /*!
     * \brief Sets the seed used for the CRC calculation
     *
     * \param [in]  polynomial    The seed value
     *
     */
    void SetCrcPolynomial( uint16_t polynomial );

    /*!
     * \brief Sets the Initial value of the LFSR used for the whitening in GFSK, FLRC and BLE protocols
     *
     * \param [in]  seed          Initial LFSR value
     */
    void SetWhiteningSeed( uint8_t seed );
    
    /*!
     * \brief Enable manual gain control and disable AGC
     *
     * \see SX1280::SetManualGainValue, SX1280::DisableManualGain
     */
    void EnableManualGain( void );
    
    /*!
     * \brief Disable the manual gain control and enable AGC
     *
     * \see SX1280::EnableManualGain
     */
    void DisableManualGain( void );

    /*!
     * \brief Set the gain for the AGC
     *
     * SX1280::EnableManualGain must be called before using this method
     *
     * \param [in]  gain          The value of gain to set, refer to datasheet for value meaning
     *
     * \see SX1280::EnableManualGain, SX1280::DisableManualGain
     */
    void SetManualGainValue( uint8_t gain );

    /*!
     * \brief Configure the LNA regime of operation
     *
     * \param [in]  lnaSetting    The LNA setting. Possible values are
     *                            LNA_LOW_POWER_MODE and
     *                            LNA_HIGH_SENSITIVITY_MODE
     */
    void SetLNAGainSetting( const RadioLnaSettings_t lnaSetting );

    /*!
     * \brief Sets the number of bits used to check that ranging request match ranging ID
     *
     * \param [in]  length        [0: 8 bits, 1: 16 bits,
     *                             2: 24 bits, 3: 32 bits]
     */
    void SetRangingIdLength( RadioRangingIdCheckLengths_t length );

    /*!
     * \brief Sets ranging device id
     *
     * \param [in]  address       Device address
     */
    void SetDeviceRangingAddress( uint32_t address );

    /*!
     * \brief Sets the device id to ping in a ranging request
     *
     * \param [in]  address       Address of the device to ping
     */
    void SetRangingRequestAddress( uint32_t address );

    /*!
     * \brief Return the ranging result value
     *
     * \param [in]  resultType    Specifies the type of result.
     *                            [0: RAW, 1: Averaged,
     *                             2: De-biased, 3:Filtered]
     *
     * \retval      ranging       The ranging measure filtered according to resultType [m]
     */
    double GetRangingResult( RadioRangingResultTypes_t resultType );

    /*!
     * \brief Return the last ranging result power indicator
     *
     * The value returned is not an absolute power measurement. It is
     * a relative power measurement.
     *
     * \retval      deltaThreshold  A relative power indicator
     */
    uint8_t GetRangingPowerDeltaThresholdIndicator( void );



    /*!
     * \brief Sets the standard processing delay between Master and Slave
     *
     * \param [in]  cal           RxTx delay offset for correcting ranging bias.
     *
     * The calibration value reflects the group delay of the radio front end and
     * must be re-performed for each new SX1280 PCB design. The value is obtained
     * empirically by either conducted measurement in a known electrical length
     * coaxial RF cable (where the design is connectorised) or by radiated
     * measurement, at a known distance, where an antenna is present.
     * The result of the calibration process is that the SX1280 ranging result
     * accurately reflects the physical range, the calibration procedure therefore
     * removes the average timing error from the time-of-flight measurement for a
     * given design.
     *
     * The values are Spreading Factor dependents, and depend also of the board
     * design.
     */
    void SetRangingCalibration( uint16_t cal );

    /*!
     * \brief Clears the ranging filter
     */
    void RangingClearFilterResult( void );

    /*!
     * \brief Set the number of samples considered in the built-in filter
     *
     * \param [in]  numSample     The number of samples to use built-in filter
     *                            [8..255]
     *
     * \remark Value inferior to 8 will be silently set to 8
     */
    void RangingSetFilterNumSamples( uint8_t numSample );

    /*!
     * \brief Return the Estimated Frequency Error in LORA and RANGING operations
     *
     * \retval efe                The estimated frequency error [Hz]
     */
    double GetFrequencyError( );

    /*!
     * \brief Process the analysis of radio IRQs and calls callback functions
     *        depending on radio state
     */
    void ProcessIrqs( void );

    /*!
     * \brief Force the preamble length in GFSK and BLE mode
     *
     * \param [in]  preambleLength  The desired preamble length
     */
    void ForcePreambleLength( RadioPreambleLengths_t preambleLength );
};

#endif // __SX1280_H__
