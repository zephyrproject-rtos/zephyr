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

/*!
 * \brief Provides the frequency of the chip running on the radio and the frequency step
 *
 * \remark These defines are used for computing the frequency divider to set the RF frequency
 */
#define XTAL_FREQ                                   52000000
#define FREQ_STEP                                   ( ( double )( XTAL_FREQ / ( 1 << 18 ) ) )

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

#endif // __SX1280_H__
