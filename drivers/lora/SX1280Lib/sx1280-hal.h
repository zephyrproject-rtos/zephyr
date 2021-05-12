/*
  ______                              _
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2015 Semtech

Description: Handling of the node configuration protocol

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#ifndef __SX1280_HAL_H__
#define __SX1280_HAL_H__

#include "sx1280.h"

/*!
 * \brief Actual implementation of a SX1280 radio
 */
class SX1280Hal : public SX1280
{
public:
    /*!
     * \brief Constructor for SX1280Hal with SPI support
     *
     * Represents the physical connectivity with the radio and set callback functions on radio interrupts
     */
    SX1280Hal( PinName mosi, PinName miso, PinName sclk, PinName nss,
               PinName busy, PinName dio1, PinName dio2, PinName dio3, PinName rst,
               RadioCallbacks_t *callbacks );

    /*!
     * \brief Constructor for SX1280Hal with UART support
     *
     * Represents the physical connectivity with the radio and set callback functions on radio interrupts
     */
    SX1280Hal( PinName tx, PinName rx, PinName ctsn,
               PinName busy, PinName dio1, PinName dio2, PinName dio3, PinName rst,
               RadioCallbacks_t *callbacks );

    /*!
     * \brief Destructor for SX1280Hal with UART support
     *
     * Take care of the correct destruction of the communication objects
     */
    virtual ~SX1280Hal( void );

    /*!
     * \brief Soft resets the radio
     */
    virtual void Reset( void );

    /*!
     * \brief Wakes up the radio
     */
    virtual void Wakeup( void );

    /*!
     * \brief Set the SPI Speed
     *
     * \param [in]  spiSpeed      Speed of the SPI in Hz
     */
    void SetSpiSpeed( uint32_t spiSpeed );

    /*!
     * \brief Send a command that write data to the radio
     *
     * \param [in]  opcode        Opcode of the command
     * \param [in]  buffer        Buffer to be send to the radio
     * \param [in]  size          Size of the buffer to send
     */
    virtual void WriteCommand( RadioCommands_t opcode, uint8_t *buffer, uint16_t size );

    /*!
     * \brief Send a command that read data from the radio
     *
     * \param [in]  opcode        Opcode of the command
     * \param [out] buffer        Buffer holding data from the radio
     * \param [in]  size          Size of the buffer
     */
    virtual void ReadCommand( RadioCommands_t opcode, uint8_t *buffer, uint16_t size );

    /*!
     * \brief Write data to the radio memory
     *
     * \param [in]  address       The address of the first byte to write in the radio
     * \param [in]  buffer        The data to be written in radio's memory
     * \param [in]  size          The number of bytes to write in radio's memory
     */
    virtual void WriteRegister( uint16_t address, uint8_t *buffer, uint16_t size );

    /*!
     * \brief Write a single byte of data to the radio memory
     *
     * \param [in]  address       The address of the first byte to write in the radio
     * \param [in]  value         The data to be written in radio's memory
     */
    virtual void WriteRegister( uint16_t address, uint8_t value );

    /*!
     * \brief Read data from the radio memory
     *
     * \param [in]  address       The address of the first byte to read from the radio
     * \param [out] buffer        The buffer that holds data read from radio
     * \param [in]  size          The number of bytes to read from radio's memory
     */
    virtual void ReadRegister( uint16_t address, uint8_t *buffer, uint16_t size );

    /*!
     * \brief Read a single byte of data from the radio memory
     *
     * \param [in]  address       The address of the first byte to write in the
     *                            radio
     *
     * \retval      value         The value of the byte at the given address in
     *                            radio's memory
     */
    virtual uint8_t ReadRegister( uint16_t address );

    /*!
     * \brief Write data to the buffer holding the payload in the radio
     *
     * \param [in]  offset        The offset to start writing the payload
     * \param [in]  buffer        The data to be written (the payload)
     * \param [in]  size          The number of byte to be written
     */
    virtual void WriteBuffer( uint8_t offset, uint8_t *buffer, uint8_t size );

    /*!
     * \brief Read data from the buffer holding the payload in the radio
     *
     * \param [in]  offset        The offset to start reading the payload
     * \param [out] buffer        A pointer to a buffer holding the data from the radio
     * \param [in]  size          The number of byte to be read
     */
    virtual void ReadBuffer( uint8_t offset, uint8_t *buffer, uint8_t size );

    /*!
     * \brief Returns the status of DIOs pins
     *
     * \retval      dioStatus     A byte where each bit represents a DIO state:
     *                            [ DIO3 | DIO2 | DIO1 | BUSY ]
     */
    virtual uint8_t GetDioStatus( void );

protected:

    SPI *RadioSpi;                                  //!< The SPI object used to communicate with the radio
    Serial *RadioUart;                              //!< The UART object used to communicate with the radio
    DigitalOut RadioNss;                            //!< The pin connected to Radio chip select (active low)
    DigitalInOut RadioReset;                        //!< The reset pin connected to the radio
    DigitalOut RadioCtsn;                           //!< The Clear To Send radio pin (active low)

    DigitalIn    BUSY;                              //!< The pin connected to BUSY
    InterruptIn *DIO1;                              //!< The pin connected to DIO1
    InterruptIn *DIO2;                              //!< The pin connected to DIO2
    InterruptIn *DIO3;                              //!< The pin connected to DIO3

    /*!
     * \brief Initializes SPI object used to communicate with the radio
     */
    virtual void SpiInit( void );

    /*!
     * \brief Initializes UART object used to communicate with the radio
     */
    virtual void UartInit( void );

    /*!
     * \brief Sets the callback functions to be run on DIO1..3 interrupt
     *
     * \param [in]  irqHandler    A function pointer of the function to be run on every DIO interrupt
     */
    virtual void IoIrqInit( DioIrqHandler irqHandler );
};

#endif // __SX1280_HAL_H__
