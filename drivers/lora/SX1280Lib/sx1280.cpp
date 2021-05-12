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
#include "mbed.h"
#include "sx1280.h"
#include "sx1280-hal.h"

/*!
 * \brief Radio registers definition
 *
 */
typedef struct
{
    uint16_t      Addr;                             //!< The address of the register
    uint8_t       Value;                            //!< The value of the register
}RadioRegisters_t;

/*!
 * \brief Radio hardware registers initialization definition
 */
#define RADIO_INIT_REGISTERS_VALUE  { }

/*!
 * \brief Radio hardware registers initialization
 */
const RadioRegisters_t RadioRegsInit[] = RADIO_INIT_REGISTERS_VALUE;

void SX1280::Init( void )
{
    Reset( );
    IoIrqInit( dioIrq );
    Wakeup( );
    SetRegistersDefault( );
}

void SX1280::SetRegistersDefault( void )
{
    for( int16_t i = 0; i < sizeof( RadioRegsInit ) / sizeof( RadioRegisters_t ); i++ )
    {
        WriteRegister( RadioRegsInit[i].Addr, RadioRegsInit[i].Value );
    }
}

uint16_t SX1280::GetFirmwareVersion( void )
{
    return( ( ( ReadRegister( REG_LR_FIRMWARE_VERSION_MSB ) ) << 8 ) | ( ReadRegister( REG_LR_FIRMWARE_VERSION_MSB + 1 ) ) );
}

RadioStatus_t SX1280::GetStatus( void )
{
    uint8_t stat = 0;
    RadioStatus_t status;

    ReadCommand( RADIO_GET_STATUS, ( uint8_t * )&stat, 1 );
    status.Value = stat;
    return( status );
}

RadioOperatingModes_t SX1280::GetOpMode( void )
{
    return( OperatingMode );
}

void SX1280::SetSleep( SleepParams_t sleepConfig )
{
    uint8_t sleep = ( sleepConfig.WakeUpRTC << 3 ) |
                    ( sleepConfig.InstructionRamRetention << 2 ) |
                    ( sleepConfig.DataBufferRetention << 1 ) |
                    ( sleepConfig.DataRamRetention );

    OperatingMode = MODE_SLEEP;
    WriteCommand( RADIO_SET_SLEEP, &sleep, 1 );
}

void SX1280::SetStandby( RadioStandbyModes_t standbyConfig )
{
    WriteCommand( RADIO_SET_STANDBY, ( uint8_t* )&standbyConfig, 1 );
    if( standbyConfig == STDBY_RC )
    {
        OperatingMode = MODE_STDBY_RC;
    }
    else
    {
        OperatingMode = MODE_STDBY_XOSC;
    }
}

void SX1280::SetFs( void )
{
    WriteCommand( RADIO_SET_FS, 0, 0 );
    OperatingMode = MODE_FS;
}

void SX1280::SetTx( TickTime_t timeout )
{
    uint8_t buf[3];
    buf[0] = timeout.PeriodBase;
    buf[1] = ( uint8_t )( ( timeout.PeriodBaseCount >> 8 ) & 0x00FF );
    buf[2] = ( uint8_t )( timeout.PeriodBaseCount & 0x00FF );

    ClearIrqStatus( IRQ_RADIO_ALL );

    // If the radio is doing ranging operations, then apply the specific calls
    // prior to SetTx
    if( GetPacketType( true ) == PACKET_TYPE_RANGING )
    {
        SetRangingRole( RADIO_RANGING_ROLE_MASTER );
    }
    WriteCommand( RADIO_SET_TX, buf, 3 );
    OperatingMode = MODE_TX;
}

void SX1280::SetRx( TickTime_t timeout )
{
    uint8_t buf[3];
    buf[0] = timeout.PeriodBase;
    buf[1] = ( uint8_t )( ( timeout.PeriodBaseCount >> 8 ) & 0x00FF );
    buf[2] = ( uint8_t )( timeout.PeriodBaseCount & 0x00FF );

    ClearIrqStatus( IRQ_RADIO_ALL );

    // If the radio is doing ranging operations, then apply the specific calls
    // prior to SetRx
    if( GetPacketType( true ) == PACKET_TYPE_RANGING )
    {
        SetRangingRole( RADIO_RANGING_ROLE_SLAVE );
    }
    WriteCommand( RADIO_SET_RX, buf, 3 );
    OperatingMode = MODE_RX;
}

void SX1280::SetRxDutyCycle( RadioTickSizes_t periodBase, uint16_t periodBaseCountRx, uint16_t periodBaseCountSleep )
{
    uint8_t buf[5];

    buf[0] = periodBase;
    buf[1] = ( uint8_t )( ( periodBaseCountRx >> 8 ) & 0x00FF );
    buf[2] = ( uint8_t )( periodBaseCountRx & 0x00FF );
    buf[3] = ( uint8_t )( ( periodBaseCountSleep >> 8 ) & 0x00FF );
    buf[4] = ( uint8_t )( periodBaseCountSleep & 0x00FF );
    WriteCommand( RADIO_SET_RXDUTYCYCLE, buf, 5 );
    OperatingMode = MODE_RX;
}

void SX1280::SetCad( void )
{
    WriteCommand( RADIO_SET_CAD, 0, 0 );
    OperatingMode = MODE_CAD;
}

void SX1280::SetTxContinuousWave( void )
{
    WriteCommand( RADIO_SET_TXCONTINUOUSWAVE, 0, 0 );
}

void SX1280::SetTxContinuousPreamble( void )
{
    WriteCommand( RADIO_SET_TXCONTINUOUSPREAMBLE, 0, 0 );
}

void SX1280::SetPacketType( RadioPacketTypes_t packetType )
{
    // Save packet type internally to avoid questioning the radio
    this->PacketType = packetType;

    WriteCommand( RADIO_SET_PACKETTYPE, ( uint8_t* )&packetType, 1 );
}

RadioPacketTypes_t SX1280::GetPacketType( bool returnLocalCopy )
{
    RadioPacketTypes_t packetType = PACKET_TYPE_NONE;
    if( returnLocalCopy == false )
    {
        ReadCommand( RADIO_GET_PACKETTYPE, ( uint8_t* )&packetType, 1 );
        if( this->PacketType != packetType )
        {
            this->PacketType = packetType;
        }
    }
    else
    {
        packetType = this->PacketType;
    }
    return packetType;
}

void SX1280::SetRfFrequency( uint32_t rfFrequency )
{
    uint8_t buf[3];
    uint32_t freq = 0;

    freq = ( uint32_t )( ( double )rfFrequency / ( double )FREQ_STEP );
    buf[0] = ( uint8_t )( ( freq >> 16 ) & 0xFF );
    buf[1] = ( uint8_t )( ( freq >> 8 ) & 0xFF );
    buf[2] = ( uint8_t )( freq & 0xFF );
    WriteCommand( RADIO_SET_RFFREQUENCY, buf, 3 );
}

void SX1280::SetTxParams( int8_t power, RadioRampTimes_t rampTime )
{
    uint8_t buf[2];

    // The power value to send on SPI/UART is in the range [0..31] and the
    // physical output power is in the range [-18..13]dBm
    buf[0] = power + 18;
    buf[1] = ( uint8_t )rampTime;
    WriteCommand( RADIO_SET_TXPARAMS, buf, 2 );
}

void SX1280::SetCadParams( RadioLoRaCadSymbols_t cadSymbolNum )
{
    WriteCommand( RADIO_SET_CADPARAMS, ( uint8_t* )&cadSymbolNum, 1 );
    OperatingMode = MODE_CAD;
}

void SX1280::SetBufferBaseAddresses( uint8_t txBaseAddress, uint8_t rxBaseAddress )
{
    uint8_t buf[2];

    buf[0] = txBaseAddress;
    buf[1] = rxBaseAddress;
    WriteCommand( RADIO_SET_BUFFERBASEADDRESS, buf, 2 );
}

void SX1280::SetModulationParams( ModulationParams_t *modParams )
{
    uint8_t buf[3];

    // Check if required configuration corresponds to the stored packet type
    // If not, silently update radio packet type
    if( this->PacketType != modParams->PacketType )
    {
        this->SetPacketType( modParams->PacketType );
    }

    switch( modParams->PacketType )
    {
        case PACKET_TYPE_GFSK:
            buf[0] = modParams->Params.Gfsk.BitrateBandwidth;
            buf[1] = modParams->Params.Gfsk.ModulationIndex;
            buf[2] = modParams->Params.Gfsk.ModulationShaping;
            break;
        case PACKET_TYPE_LORA:
        case PACKET_TYPE_RANGING:
            buf[0] = modParams->Params.LoRa.SpreadingFactor;
            buf[1] = modParams->Params.LoRa.Bandwidth;
            buf[2] = modParams->Params.LoRa.CodingRate;
            this->LoRaBandwidth = modParams->Params.LoRa.Bandwidth;
            break;
        case PACKET_TYPE_FLRC:
            buf[0] = modParams->Params.Flrc.BitrateBandwidth;
            buf[1] = modParams->Params.Flrc.CodingRate;
            buf[2] = modParams->Params.Flrc.ModulationShaping;
            break;
        case PACKET_TYPE_BLE:
            buf[0] = modParams->Params.Ble.BitrateBandwidth;
            buf[1] = modParams->Params.Ble.ModulationIndex;
            buf[2] = modParams->Params.Ble.ModulationShaping;
            break;
        case PACKET_TYPE_NONE:
            buf[0] = NULL;
            buf[1] = NULL;
            buf[2] = NULL;
            break;
    }
    WriteCommand( RADIO_SET_MODULATIONPARAMS, buf, 3 );
}

void SX1280::SetPacketParams( PacketParams_t *packetParams )
{
    uint8_t buf[7];
    // Check if required configuration corresponds to the stored packet type
    // If not, silently update radio packet type
    if( this->PacketType != packetParams->PacketType )
    {
        this->SetPacketType( packetParams->PacketType );
    }

    switch( packetParams->PacketType )
    {
        case PACKET_TYPE_GFSK:
            buf[0] = packetParams->Params.Gfsk.PreambleLength;
            buf[1] = packetParams->Params.Gfsk.SyncWordLength;
            buf[2] = packetParams->Params.Gfsk.SyncWordMatch;
            buf[3] = packetParams->Params.Gfsk.HeaderType;
            buf[4] = packetParams->Params.Gfsk.PayloadLength;
            buf[5] = packetParams->Params.Gfsk.CrcLength;
            buf[6] = packetParams->Params.Gfsk.Whitening;
            break;
        case PACKET_TYPE_LORA:
        case PACKET_TYPE_RANGING:
            buf[0] = packetParams->Params.LoRa.PreambleLength;
            buf[1] = packetParams->Params.LoRa.HeaderType;
            buf[2] = packetParams->Params.LoRa.PayloadLength;
            buf[3] = packetParams->Params.LoRa.Crc;
            buf[4] = packetParams->Params.LoRa.InvertIQ;
            buf[5] = NULL;
            buf[6] = NULL;
            break;
        case PACKET_TYPE_FLRC:
            buf[0] = packetParams->Params.Flrc.PreambleLength;
            buf[1] = packetParams->Params.Flrc.SyncWordLength;
            buf[2] = packetParams->Params.Flrc.SyncWordMatch;
            buf[3] = packetParams->Params.Flrc.HeaderType;
            buf[4] = packetParams->Params.Flrc.PayloadLength;
            buf[5] = packetParams->Params.Flrc.CrcLength;
            buf[6] = packetParams->Params.Flrc.Whitening;
            break;
        case PACKET_TYPE_BLE:
            buf[0] = packetParams->Params.Ble.ConnectionState;
            buf[1] = packetParams->Params.Ble.CrcLength;
            buf[2] = packetParams->Params.Ble.BleTestPayload;
            buf[3] = packetParams->Params.Ble.Whitening;
            buf[4] = NULL;
            buf[5] = NULL;
            buf[6] = NULL;
            break;
        case PACKET_TYPE_NONE:
            buf[0] = NULL;
            buf[1] = NULL;
            buf[2] = NULL;
            buf[3] = NULL;
            buf[4] = NULL;
            buf[5] = NULL;
            buf[6] = NULL;
            break;
    }
    WriteCommand( RADIO_SET_PACKETPARAMS, buf, 7 );
}

void SX1280::ForcePreambleLength( RadioPreambleLengths_t preambleLength )
{
    this->WriteRegister( REG_LR_PREAMBLELENGTH, ( this->ReadRegister( REG_LR_PREAMBLELENGTH ) & MASK_FORCE_PREAMBLELENGTH ) | preambleLength );
}

void SX1280::GetRxBufferStatus( uint8_t *rxPayloadLength, uint8_t *rxStartBufferPointer )
{
    uint8_t status[2];

    ReadCommand( RADIO_GET_RXBUFFERSTATUS, status, 2 );

    // In case of LORA fixed header, the rxPayloadLength is obtained by reading
    // the register REG_LR_PAYLOADLENGTH
    if( ( this -> GetPacketType( true ) == PACKET_TYPE_LORA ) && ( ReadRegister( REG_LR_PACKETPARAMS ) >> 7 == 1 ) )
    {
        *rxPayloadLength = ReadRegister( REG_LR_PAYLOADLENGTH );
    }
    else if( this -> GetPacketType( true ) == PACKET_TYPE_BLE )
    {
        // In the case of BLE, the size returned in status[0] do not include the 2-byte length PDU header
        // so it is added there
        *rxPayloadLength = status[0] + 2;
    }
    else
    {
        *rxPayloadLength = status[0];
    }

    *rxStartBufferPointer = status[1];
}

void SX1280::GetPacketStatus( PacketStatus_t *packetStatus )
{
    uint8_t status[5];

    ReadCommand( RADIO_GET_PACKETSTATUS, status, 5 );

    packetStatus->packetType = this -> GetPacketType( true );
    switch( packetStatus->packetType )
    {
        case PACKET_TYPE_GFSK:
            packetStatus->Gfsk.RssiSync = -( status[1] / 2 );

            packetStatus->Gfsk.ErrorStatus.SyncError = ( status[2] >> 6 ) & 0x01;
            packetStatus->Gfsk.ErrorStatus.LengthError = ( status[2] >> 5 ) & 0x01;
            packetStatus->Gfsk.ErrorStatus.CrcError = ( status[2] >> 4 ) & 0x01;
            packetStatus->Gfsk.ErrorStatus.AbortError = ( status[2] >> 3 ) & 0x01;
            packetStatus->Gfsk.ErrorStatus.HeaderReceived = ( status[2] >> 2 ) & 0x01;
            packetStatus->Gfsk.ErrorStatus.PacketReceived = ( status[2] >> 1 ) & 0x01;
            packetStatus->Gfsk.ErrorStatus.PacketControlerBusy = status[2] & 0x01;

            packetStatus->Gfsk.TxRxStatus.RxNoAck = ( status[3] >> 5 ) & 0x01;
            packetStatus->Gfsk.TxRxStatus.PacketSent = status[3] & 0x01;

            packetStatus->Gfsk.SyncAddrStatus = status[4] & 0x07;
            break;

        case PACKET_TYPE_LORA:
        case PACKET_TYPE_RANGING:
            packetStatus->LoRa.RssiPkt = -( status[0] / 2 );
            ( status[1] < 128 ) ? ( packetStatus->LoRa.SnrPkt = status[1] / 4 ) : ( packetStatus->LoRa.SnrPkt = ( ( status[1] - 256 ) /4 ) );
            break;

        case PACKET_TYPE_FLRC:
            packetStatus->Flrc.RssiSync = -( status[1] / 2 );

            packetStatus->Flrc.ErrorStatus.SyncError = ( status[2] >> 6 ) & 0x01;
            packetStatus->Flrc.ErrorStatus.LengthError = ( status[2] >> 5 ) & 0x01;
            packetStatus->Flrc.ErrorStatus.CrcError = ( status[2] >> 4 ) & 0x01;
            packetStatus->Flrc.ErrorStatus.AbortError = ( status[2] >> 3 ) & 0x01;
            packetStatus->Flrc.ErrorStatus.HeaderReceived = ( status[2] >> 2 ) & 0x01;
            packetStatus->Flrc.ErrorStatus.PacketReceived = ( status[2] >> 1 ) & 0x01;
            packetStatus->Flrc.ErrorStatus.PacketControlerBusy = status[2] & 0x01;

            packetStatus->Flrc.TxRxStatus.RxPid = ( status[3] >> 6 ) & 0x03;
            packetStatus->Flrc.TxRxStatus.RxNoAck = ( status[3] >> 5 ) & 0x01;
            packetStatus->Flrc.TxRxStatus.RxPidErr = ( status[3] >> 4 ) & 0x01;
            packetStatus->Flrc.TxRxStatus.PacketSent = status[3] & 0x01;

            packetStatus->Flrc.SyncAddrStatus = status[4] & 0x07;
            break;

        case PACKET_TYPE_BLE:
            packetStatus->Ble.RssiSync =  -( status[1] / 2 );

            packetStatus->Ble.ErrorStatus.SyncError = ( status[2] >> 6 ) & 0x01;
            packetStatus->Ble.ErrorStatus.LengthError = ( status[2] >> 5 ) & 0x01;
            packetStatus->Ble.ErrorStatus.CrcError = ( status[2] >> 4 ) & 0x01;
            packetStatus->Ble.ErrorStatus.AbortError = ( status[2] >> 3 ) & 0x01;
            packetStatus->Ble.ErrorStatus.HeaderReceived = ( status[2] >> 2 ) & 0x01;
            packetStatus->Ble.ErrorStatus.PacketReceived = ( status[2] >> 1 ) & 0x01;
            packetStatus->Ble.ErrorStatus.PacketControlerBusy = status[2] & 0x01;

            packetStatus->Ble.TxRxStatus.PacketSent = status[3] & 0x01;

            packetStatus->Ble.SyncAddrStatus = status[4] & 0x07;
            break;

        case PACKET_TYPE_NONE:
            // In that specific case, we set everything in the packetStatus to zeros
            // and reset the packet type accordingly
            memset( packetStatus, 0, sizeof( PacketStatus_t ) );
            packetStatus->packetType = PACKET_TYPE_NONE;
            break;
    }
}

int8_t SX1280::GetRssiInst( void )
{
    uint8_t raw = 0;

    ReadCommand( RADIO_GET_RSSIINST, &raw, 1 );

    return ( int8_t ) ( -raw / 2 );
}

void SX1280::SetDioIrqParams( uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask, uint16_t dio3Mask )
{
    uint8_t buf[8];

    buf[0] = ( uint8_t )( ( irqMask >> 8 ) & 0x00FF );
    buf[1] = ( uint8_t )( irqMask & 0x00FF );
    buf[2] = ( uint8_t )( ( dio1Mask >> 8 ) & 0x00FF );
    buf[3] = ( uint8_t )( dio1Mask & 0x00FF );
    buf[4] = ( uint8_t )( ( dio2Mask >> 8 ) & 0x00FF );
    buf[5] = ( uint8_t )( dio2Mask & 0x00FF );
    buf[6] = ( uint8_t )( ( dio3Mask >> 8 ) & 0x00FF );
    buf[7] = ( uint8_t )( dio3Mask & 0x00FF );
    WriteCommand( RADIO_SET_DIOIRQPARAMS, buf, 8 );
}

uint16_t SX1280::GetIrqStatus( void )
{
    uint8_t irqStatus[2];
    ReadCommand( RADIO_GET_IRQSTATUS, irqStatus, 2 );
    return ( irqStatus[0] << 8 ) | irqStatus[1];
}

void SX1280::ClearIrqStatus( uint16_t irqMask )
{
    uint8_t buf[2];

    buf[0] = ( uint8_t )( ( ( uint16_t )irqMask >> 8 ) & 0x00FF );
    buf[1] = ( uint8_t )( ( uint16_t )irqMask & 0x00FF );
    WriteCommand( RADIO_CLR_IRQSTATUS, buf, 2 );
}

void SX1280::Calibrate( CalibrationParams_t calibParam )
{
    uint8_t cal = ( calibParam.ADCBulkPEnable << 5 ) |
                  ( calibParam.ADCBulkNEnable << 4 ) |
                  ( calibParam.ADCPulseEnable << 3 ) |
                  ( calibParam.PLLEnable << 2 ) |
                  ( calibParam.RC13MEnable << 1 ) |
                  ( calibParam.RC64KEnable );
    WriteCommand( RADIO_CALIBRATE, &cal, 1 );
}

void SX1280::SetRegulatorMode( RadioRegulatorModes_t mode )
{
    WriteCommand( RADIO_SET_REGULATORMODE, ( uint8_t* )&mode, 1 );
}

void SX1280::SetSaveContext( void )
{
    WriteCommand( RADIO_SET_SAVECONTEXT, 0, 0 );
}

void SX1280::SetAutoTx( uint16_t time )
{
    uint16_t compensatedTime = time - ( uint16_t )AUTO_TX_OFFSET;
    uint8_t buf[2];

    buf[0] = ( uint8_t )( ( compensatedTime >> 8 ) & 0x00FF );
    buf[1] = ( uint8_t )( compensatedTime & 0x00FF );
    WriteCommand( RADIO_SET_AUTOTX, buf, 2 );
}

void SX1280::StopAutoTx( void )
{
    uint8_t buf[2] = {0x00, 0x00};
    WriteCommand( RADIO_SET_AUTOTX, buf, 2 );
}

void SX1280::SetAutoFs( bool enableAutoFs )
{
    WriteCommand( RADIO_SET_AUTOFS, ( uint8_t * )&enableAutoFs, 1 );
}

void SX1280::SetLongPreamble( bool enable )
{
    WriteCommand( RADIO_SET_LONGPREAMBLE, ( uint8_t * )&enable, 1 );
}

void SX1280::SetPayload( uint8_t *buffer, uint8_t size, uint8_t offset )
{
    WriteBuffer( offset, buffer, size );
}

uint8_t SX1280::GetPayload( uint8_t *buffer, uint8_t *size , uint8_t maxSize )
{
    uint8_t offset;

    GetRxBufferStatus( size, &offset );
    if( *size > maxSize )
    {
        return 1;
    }
    ReadBuffer( offset, buffer, *size );
    return 0;
}

void SX1280::SendPayload( uint8_t *payload, uint8_t size, TickTime_t timeout, uint8_t offset )
{
    SetPayload( payload, size, offset );
    SetTx( timeout );
}

uint8_t SX1280::SetSyncWord( uint8_t syncWordIdx, uint8_t *syncWord )
{
    uint16_t addr;
    uint8_t syncwordSize = 0;

    switch( GetPacketType( true ) )
    {
        case PACKET_TYPE_GFSK:
            syncwordSize = 5;
            switch( syncWordIdx )
            {
                case 1:
                    addr = REG_LR_SYNCWORDBASEADDRESS1;
                    break;
                case 2:
                    addr = REG_LR_SYNCWORDBASEADDRESS2;
                    break;
                case 3:
                    addr = REG_LR_SYNCWORDBASEADDRESS3;
                    break;
                default:
                    return 1;
            }
            break;
        case PACKET_TYPE_FLRC:
            // For FLRC packet type, the SyncWord is one byte shorter and
            // the base address is shifted by one byte
            syncwordSize = 4;
            switch( syncWordIdx )
            {
                case 1:
                    addr = REG_LR_SYNCWORDBASEADDRESS1 + 1;
                    break;
                case 2:
                    addr = REG_LR_SYNCWORDBASEADDRESS2 + 1;
                    break;
                case 3:
                    addr = REG_LR_SYNCWORDBASEADDRESS3 + 1;
                    break;
                default:
                    return 1;
            }
            break;
        case PACKET_TYPE_BLE:
            // For Ble packet type, only the first SyncWord is used and its
            // address is shifted by one byte
            syncwordSize = 4;
            switch( syncWordIdx )
            {
                case 1:
                    addr = REG_LR_SYNCWORDBASEADDRESS1 + 1;
                    break;
                default:
                    return 1;
            }
            break;
        default:
            return 1;
    }
    WriteRegister( addr, syncWord, syncwordSize );
    return 0;
}

void SX1280::SetSyncWordErrorTolerance( uint8_t ErrorBits )
{
    ErrorBits = ( ReadRegister( REG_LR_SYNCWORDTOLERANCE ) & 0xF0 ) | ( ErrorBits & 0x0F );
    WriteRegister( REG_LR_SYNCWORDTOLERANCE, ErrorBits );
}

uint8_t SX1280::SetCrcSeed( uint8_t *seed )
{
    uint8_t updated = 0;
    switch( GetPacketType( true ) )
    {
        case PACKET_TYPE_GFSK:
        case PACKET_TYPE_FLRC:
            WriteRegister( REG_LR_CRCSEEDBASEADDR, seed, 2 );
            updated = 1;
            break;
        case PACKET_TYPE_BLE:
            this->WriteRegister(0x9c7, seed[2] );
            this->WriteRegister(0x9c8, seed[1] );
            this->WriteRegister(0x9c9, seed[0] );
            updated = 1;
            break;
        default:
            break;
    }
    return updated;
}

void SX1280::SetBleAccessAddress( uint32_t accessAddress )
{
    this->WriteRegister( REG_LR_BLE_ACCESS_ADDRESS, ( accessAddress >> 24 ) & 0x000000FF );
    this->WriteRegister( REG_LR_BLE_ACCESS_ADDRESS + 1, ( accessAddress >> 16 ) & 0x000000FF );
    this->WriteRegister( REG_LR_BLE_ACCESS_ADDRESS + 2, ( accessAddress >> 8 ) & 0x000000FF );
    this->WriteRegister( REG_LR_BLE_ACCESS_ADDRESS + 3, accessAddress & 0x000000FF );
}

void SX1280::SetBleAdvertizerAccessAddress( void )
{
    this->SetBleAccessAddress( BLE_ADVERTIZER_ACCESS_ADDRESS );
}

void SX1280::SetCrcPolynomial( uint16_t polynomial )
{
    uint8_t val[2];

    val[0] = ( uint8_t )( polynomial >> 8 ) & 0xFF;
    val[1] = ( uint8_t )( polynomial  & 0xFF );

    switch( GetPacketType( true ) )
    {
        case PACKET_TYPE_GFSK:
        case PACKET_TYPE_FLRC:
            WriteRegister( REG_LR_CRCPOLYBASEADDR, val, 2 );
            break;
        default:
            break;
    }
}

void SX1280::SetWhiteningSeed( uint8_t seed )
{
    switch( GetPacketType( true ) )
    {
        case PACKET_TYPE_GFSK:
        case PACKET_TYPE_FLRC:
        case PACKET_TYPE_BLE:
            WriteRegister( REG_LR_WHITSEEDBASEADDR, seed );
            break;
        default:
            break;
    }
}

void SX1280::EnableManualGain( void )
{
    this->WriteRegister( REG_ENABLE_MANUAL_GAIN_CONTROL, this->ReadRegister( REG_ENABLE_MANUAL_GAIN_CONTROL ) | MASK_MANUAL_GAIN_CONTROL );
    this->WriteRegister( REG_DEMOD_DETECTION, this->ReadRegister( REG_DEMOD_DETECTION ) & MASK_DEMOD_DETECTION );
}

void SX1280::DisableManualGain( void )
{
    this->WriteRegister( REG_ENABLE_MANUAL_GAIN_CONTROL, this->ReadRegister( REG_ENABLE_MANUAL_GAIN_CONTROL ) & (~MASK_MANUAL_GAIN_CONTROL) );
    this->WriteRegister( REG_DEMOD_DETECTION, this->ReadRegister( REG_DEMOD_DETECTION ) | (~MASK_DEMOD_DETECTION) );
}

void SX1280::SetManualGainValue( uint8_t gain )
{
    this->WriteRegister( REG_MANUAL_GAIN_VALUE, ( this->ReadRegister( REG_MANUAL_GAIN_VALUE ) & MASK_MANUAL_GAIN_VALUE ) | gain );
}

void SX1280::SetLNAGainSetting( const RadioLnaSettings_t lnaSetting )
{
    switch(lnaSetting)
    {
        case LNA_HIGH_SENSITIVITY_MODE:
        {
            this->WriteRegister( REG_LNA_REGIME, this->ReadRegister( REG_LNA_REGIME ) | MASK_LNA_REGIME );
            break;
        }
        case LNA_LOW_POWER_MODE:
        {
            this->WriteRegister( REG_LNA_REGIME, this->ReadRegister( REG_LNA_REGIME ) & ~MASK_LNA_REGIME );
            break;
        }
    }
}

void SX1280::SetRangingIdLength( RadioRangingIdCheckLengths_t length )
{
    switch( GetPacketType( true ) )
    {
        case PACKET_TYPE_RANGING:
            WriteRegister( REG_LR_RANGINGIDCHECKLENGTH, ( ( ( ( uint8_t )length ) & 0x03 ) << 6 ) | ( ReadRegister( REG_LR_RANGINGIDCHECKLENGTH ) & 0x3F ) );
            break;
        default:
            break;
    }
}

void SX1280::SetDeviceRangingAddress( uint32_t address )
{
    uint8_t addrArray[] = { address >> 24, address >> 16, address >> 8, address };

    switch( GetPacketType( true ) )
    {
        case PACKET_TYPE_RANGING:
            WriteRegister( REG_LR_DEVICERANGINGADDR, addrArray, 4 );
            break;
        default:
            break;
    }
}

void SX1280::SetRangingRequestAddress( uint32_t address )
{
    uint8_t addrArray[] = { address >> 24, address >> 16, address >> 8, address };

    switch( GetPacketType( true ) )
    {
        case PACKET_TYPE_RANGING:
            WriteRegister( REG_LR_REQUESTRANGINGADDR, addrArray, 4 );
            break;
        default:
            break;
    }
}

double SX1280::GetRangingResult( RadioRangingResultTypes_t resultType )
{
    uint32_t valLsb = 0;
    double val = 0.0;

    switch( GetPacketType( true ) )
    {
        case PACKET_TYPE_RANGING:
            this->SetStandby( STDBY_XOSC );
            this->WriteRegister( 0x97F, this->ReadRegister( 0x97F ) | ( 1 << 1 ) ); // enable LORA modem clock
            WriteRegister( REG_LR_RANGINGRESULTCONFIG, ( ReadRegister( REG_LR_RANGINGRESULTCONFIG ) & MASK_RANGINGMUXSEL ) | ( ( ( ( uint8_t )resultType ) & 0x03 ) << 4 ) );
            valLsb = ( ( ReadRegister( REG_LR_RANGINGRESULTBASEADDR ) << 16 ) | ( ReadRegister( REG_LR_RANGINGRESULTBASEADDR + 1 ) << 8 ) | ( ReadRegister( REG_LR_RANGINGRESULTBASEADDR + 2 ) ) );
            this->SetStandby( STDBY_RC );

            // Convertion from LSB to distance. For explanation on the formula, refer to Datasheet of SX1280
            switch( resultType )
            {
                case RANGING_RESULT_RAW:
                    // Convert the ranging LSB to distance in meter
                    // The theoretical conversion from register value to distance [m] is given by:
                    // distance [m] = ( complement2( register ) * 150 ) / ( 2^12 * bandwidth[MHz] ) )
                    // The API provide BW in [Hz] so the implemented formula is complement2( register ) / bandwidth[Hz] * A,
                    // where A = 150 / (2^12 / 1e6) = 36621.09
                    val = ( double )complement2( valLsb, 24 ) / ( double )this->GetLoRaBandwidth( ) * 36621.09375;
                    break;

                case RANGING_RESULT_AVERAGED:
                case RANGING_RESULT_DEBIASED:
                case RANGING_RESULT_FILTERED:
                    val = ( double )valLsb * 20.0 / 100.0;
                    break;
                default:
                    val = 0.0;
            }
            break;
        default:
            break;
    }
    return val;
}

uint8_t SX1280::GetRangingPowerDeltaThresholdIndicator( void )
{
    SetStandby( STDBY_XOSC );
    WriteRegister( 0x97F, ReadRegister( 0x97F ) | ( 1 << 1 ) ); // enable LoRa modem clock
    WriteRegister( REG_LR_RANGINGRESULTCONFIG, ( ReadRegister( REG_LR_RANGINGRESULTCONFIG ) & MASK_RANGINGMUXSEL ) | ( ( ( ( uint8_t )RANGING_RESULT_RAW ) & 0x03 ) << 4 ) ); // Select raw results
    return ReadRegister( REG_RANGING_RSSI );
}

void SX1280::SetRangingCalibration( uint16_t cal )
{
    switch( GetPacketType( true ) )
    {
        case PACKET_TYPE_RANGING:
            WriteRegister( REG_LR_RANGINGRERXTXDELAYCAL, ( uint8_t )( ( cal >> 8 ) & 0xFF ) );
            WriteRegister( REG_LR_RANGINGRERXTXDELAYCAL + 1, ( uint8_t )( ( cal ) & 0xFF ) );
            break;
        default:
            break;
    }
}

void SX1280::RangingClearFilterResult( void )
{
    uint8_t regVal = ReadRegister( REG_LR_RANGINGRESULTCLEARREG );

    // To clear result, set bit 5 to 1 then to 0
    WriteRegister( REG_LR_RANGINGRESULTCLEARREG, regVal | ( 1 << 5 ) );
    WriteRegister( REG_LR_RANGINGRESULTCLEARREG, regVal & ( ~( 1 << 5 ) ) );
}

void SX1280::RangingSetFilterNumSamples( uint8_t num )
{
    // Silently set 8 as minimum value
    WriteRegister( REG_LR_RANGINGFILTERWINDOWSIZE, ( num < DEFAULT_RANGING_FILTER_SIZE ) ? DEFAULT_RANGING_FILTER_SIZE : num );
}

void SX1280::SetRangingRole( RadioRangingRoles_t role )
{
    uint8_t buf[1];

    buf[0] = role;
    WriteCommand( RADIO_SET_RANGING_ROLE, &buf[0], 1 );
}

double SX1280::GetFrequencyError( )
{
    uint8_t efeRaw[3] = {0};
    uint32_t efe = 0;
    double efeHz = 0.0;

    switch( this->GetPacketType( true ) )
    {
        case PACKET_TYPE_LORA:
        case PACKET_TYPE_RANGING:
            efeRaw[0] = this->ReadRegister( REG_LR_ESTIMATED_FREQUENCY_ERROR_MSB );
            efeRaw[1] = this->ReadRegister( REG_LR_ESTIMATED_FREQUENCY_ERROR_MSB + 1 );
            efeRaw[2] = this->ReadRegister( REG_LR_ESTIMATED_FREQUENCY_ERROR_MSB + 2 );
            efe = ( efeRaw[0]<<16 ) | ( efeRaw[1]<<8 ) | efeRaw[2];
            efe &= REG_LR_ESTIMATED_FREQUENCY_ERROR_MASK;

            efeHz = 1.55 * ( double )complement2( efe, 20 ) / ( 1600.0 / ( double )this->GetLoRaBandwidth( ) * 1000.0 );
            break;

        case PACKET_TYPE_NONE:
        case PACKET_TYPE_BLE:
        case PACKET_TYPE_FLRC:
        case PACKET_TYPE_GFSK:
            break;
    }

    return efeHz;
}

void SX1280::SetPollingMode( void )
{
    this->PollingMode = true;
}

int32_t SX1280::complement2( const uint32_t num, const uint8_t bitCnt )
{
    int32_t retVal = ( int32_t )num;
    if( num >= 2<<( bitCnt - 2 ) )
    {
        retVal -= 2<<( bitCnt - 1 );
    }
    return retVal;
}

int32_t SX1280::GetLoRaBandwidth( )
{
    int32_t bwValue = 0;

    switch( this->LoRaBandwidth )
    {
        case LORA_BW_0200:
            bwValue = 203125;
            break;
        case LORA_BW_0400:
            bwValue = 406250;
            break;
        case LORA_BW_0800:
            bwValue = 812500;
            break;
        case LORA_BW_1600:
            bwValue = 1625000;
            break;
        default:
            bwValue = 0;
    }
    return bwValue;
}

void SX1280::SetInterruptMode( void )
{
    this->PollingMode = false;
}

void SX1280::OnDioIrq( void )
{
    /*
     * When polling mode is activated, it is up to the application to call
     * ProcessIrqs( ). Otherwise, the driver automatically calls ProcessIrqs( )
     * on radio interrupt.
     */
    if( this->PollingMode == true )
    {
        this->IrqState = true;
    }
    else
    {
        this->ProcessIrqs( );
    }
}

void SX1280::ProcessIrqs( void )
{
    RadioPacketTypes_t packetType = PACKET_TYPE_NONE;

    if( this->PollingMode == true )
    {
        if( this->IrqState == true )
        {
            __disable_irq( );
            this->IrqState = false;
            __enable_irq( );
        }
        else
        {
            return;
        }
    }

    packetType = GetPacketType( true );
    uint16_t irqRegs = GetIrqStatus( );
    ClearIrqStatus( IRQ_RADIO_ALL );

#if( SX1280_DEBUG == 1 )
    DigitalOut TEST_PIN_1( D14 );
    DigitalOut TEST_PIN_2( D15 );
    for( int i = 0x8000; i != 0; i >>= 1 )
    {
        TEST_PIN_2 = 0;
        TEST_PIN_1 = ( ( irqRegs & i ) != 0 ) ? 1 : 0;
        TEST_PIN_2 = 1;
    }
    TEST_PIN_1 = 0;
    TEST_PIN_2 = 0;
#endif

    switch( packetType )
    {
        case PACKET_TYPE_GFSK:
        case PACKET_TYPE_FLRC:
        case PACKET_TYPE_BLE:
            switch( OperatingMode )
            {
                case MODE_RX:
                    if( ( irqRegs & IRQ_RX_DONE ) == IRQ_RX_DONE )
                    {
                        if( ( irqRegs & IRQ_CRC_ERROR ) == IRQ_CRC_ERROR )
                        {
                            if( rxError != NULL )
                            {
                                rxError( IRQ_CRC_ERROR_CODE );
                            }
                        }
                        else if( ( irqRegs & IRQ_SYNCWORD_ERROR ) == IRQ_SYNCWORD_ERROR )
                        {
                            if( rxError != NULL )
                            {
                                rxError( IRQ_SYNCWORD_ERROR_CODE );
                            }
                        }
                        else
                        {
                            if( rxDone != NULL )
                            {
                                rxDone( );
                            }
                        }
                    }
                    if( ( irqRegs & IRQ_SYNCWORD_VALID ) == IRQ_SYNCWORD_VALID )
                    {
                        if( rxSyncWordDone != NULL )
                        {
                            rxSyncWordDone( );
                        }
                    }
                    if( ( irqRegs & IRQ_SYNCWORD_ERROR ) == IRQ_SYNCWORD_ERROR )
                    {
                        if( rxError != NULL )
                        {
                            rxError( IRQ_SYNCWORD_ERROR_CODE );
                        }
                    }
                    if( ( irqRegs & IRQ_RX_TX_TIMEOUT ) == IRQ_RX_TX_TIMEOUT )
                    {
                        if( rxTimeout != NULL )
                        {
                            rxTimeout( );
                        }
                    }
                    if( ( irqRegs & IRQ_TX_DONE ) == IRQ_TX_DONE )
                    {
                        if( txDone != NULL )
                        {
                            txDone( );
                        }
                    }
                    break;
                case MODE_TX:
                    if( ( irqRegs & IRQ_TX_DONE ) == IRQ_TX_DONE )
                    {
                        if( txDone != NULL )
                        {
                            txDone( );
                        }
                    }
                    if( ( irqRegs & IRQ_RX_TX_TIMEOUT ) == IRQ_RX_TX_TIMEOUT )
                    {
                        if( txTimeout != NULL )
                        {
                            txTimeout( );
                        }
                    }
                    break;
                default:
                    // Unexpected IRQ: silently returns
                    break;
            }
            break;
        case PACKET_TYPE_LORA:
            switch( OperatingMode )
            {
                case MODE_RX:
                    if( ( irqRegs & IRQ_RX_DONE ) == IRQ_RX_DONE )
                    {
                        if( ( irqRegs & IRQ_CRC_ERROR ) == IRQ_CRC_ERROR )
                        {
                            if( rxError != NULL )
                            {
                                rxError( IRQ_CRC_ERROR_CODE );
                            }
                        }
                        else
                        {
                            if( rxDone != NULL )
                            {
                                rxDone( );
                            }
                        }
                    }
                    if( ( irqRegs & IRQ_HEADER_VALID ) == IRQ_HEADER_VALID )
                    {
                        if( rxHeaderDone != NULL )
                        {
                            rxHeaderDone( );
                        }
                    }
                    if( ( irqRegs & IRQ_HEADER_ERROR ) == IRQ_HEADER_ERROR )
                    {
                        if( rxError != NULL )
                        {
                            rxError( IRQ_HEADER_ERROR_CODE );
                        }
                    }
                    if( ( irqRegs & IRQ_RX_TX_TIMEOUT ) == IRQ_RX_TX_TIMEOUT )
                    {
                        if( rxTimeout != NULL )
                        {
                            rxTimeout( );
                        }
                    }
                    if( ( irqRegs & IRQ_RANGING_SLAVE_REQUEST_DISCARDED ) == IRQ_RANGING_SLAVE_REQUEST_DISCARDED )
                    {
                        if( rxError != NULL )
                        {
                            rxError( IRQ_RANGING_ON_LORA_ERROR_CODE );
                        }
                    }
                    break;
                case MODE_TX:
                    if( ( irqRegs & IRQ_TX_DONE ) == IRQ_TX_DONE )
                    {
                        if( txDone != NULL )
                        {
                            txDone( );
                        }
                    }
                    if( ( irqRegs & IRQ_RX_TX_TIMEOUT ) == IRQ_RX_TX_TIMEOUT )
                    {
                        if( txTimeout != NULL )
                        {
                            txTimeout( );
                        }
                    }
                    break;
                case MODE_CAD:
                    if( ( irqRegs & IRQ_CAD_DONE ) == IRQ_CAD_DONE )
                    {
                        if( ( irqRegs & IRQ_CAD_DETECTED ) == IRQ_CAD_DETECTED )
                        {
                            if( cadDone != NULL )
                            {
                                cadDone( true );
                            }
                        }
                        else
                        {
                            if( cadDone != NULL )
                            {
                                cadDone( false );
                            }
                        }
                    }
                    else if( ( irqRegs & IRQ_RX_TX_TIMEOUT ) == IRQ_RX_TX_TIMEOUT )
                    {
                        if( rxTimeout != NULL )
                        {
                            rxTimeout( );
                        }
                    }
                    break;
                default:
                    // Unexpected IRQ: silently returns
                    break;
            }
            break;
        case PACKET_TYPE_RANGING:
            switch( OperatingMode )
            {
                // MODE_RX indicates an IRQ on the Slave side
                case MODE_RX:
                    if( ( irqRegs & IRQ_RANGING_SLAVE_REQUEST_DISCARDED ) == IRQ_RANGING_SLAVE_REQUEST_DISCARDED )
                    {
                        if( rangingDone != NULL )
                        {
                            rangingDone( IRQ_RANGING_SLAVE_ERROR_CODE );
                        }
                    }
                    if( ( irqRegs & IRQ_RANGING_SLAVE_REQUEST_VALID ) == IRQ_RANGING_SLAVE_REQUEST_VALID )
                    {
                        if( rangingDone != NULL )
                        {
                            rangingDone( IRQ_RANGING_SLAVE_VALID_CODE );
                        }
                    }
                    if( ( irqRegs & IRQ_RANGING_SLAVE_RESPONSE_DONE ) == IRQ_RANGING_SLAVE_RESPONSE_DONE )
                    {
                        if( rangingDone != NULL )
                        {
                            rangingDone( IRQ_RANGING_SLAVE_VALID_CODE );
                        }
                    }
                    if( ( irqRegs & IRQ_RX_TX_TIMEOUT ) == IRQ_RX_TX_TIMEOUT )
                    {
                        if( rangingDone != NULL )
                        {
                            rangingDone( IRQ_RANGING_SLAVE_ERROR_CODE );
                        }
                    }
                    if( ( irqRegs & IRQ_HEADER_VALID ) == IRQ_HEADER_VALID )
                    {
                        if( rxHeaderDone != NULL )
                        {
                            rxHeaderDone( );
                        }
                    }
                    if( ( irqRegs & IRQ_HEADER_ERROR ) == IRQ_HEADER_ERROR )
                    {
                        if( rxError != NULL )
                        {
                            rxError( IRQ_HEADER_ERROR_CODE );
                        }
                    }
                    break;
                // MODE_TX indicates an IRQ on the Master side
                case MODE_TX:
                    if( ( irqRegs & IRQ_RANGING_MASTER_TIMEOUT ) == IRQ_RANGING_MASTER_TIMEOUT )
                    {
                        if( rangingDone != NULL )
                        {
                            rangingDone( IRQ_RANGING_MASTER_ERROR_CODE );
                        }
                    }
                    if( ( irqRegs & IRQ_RANGING_MASTER_RESULT_VALID ) == IRQ_RANGING_MASTER_RESULT_VALID )
                    {
                        if( rangingDone != NULL )
                        {
                            rangingDone( IRQ_RANGING_MASTER_VALID_CODE );
                        }
                    }
                    break;
                default:
                    // Unexpected IRQ: silently returns
                    break;
            }
            break;
        default:
            // Unexpected IRQ: silently returns
            break;
    }
}
