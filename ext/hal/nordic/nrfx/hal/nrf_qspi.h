/*
 * Copyright (c) 2016 - 2018, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NRF_QSPI_H__
#define NRF_QSPI_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_qspi_hal QSPI HAL
 * @{
 * @ingroup nrf_qspi
 * @brief   Hardware access layer for managing the QSPI peripheral.
 */

/**
 * @brief This value can be used as a parameter for the @ref nrf_qspi_pins_set
 *        function to specify that a given QSPI signal (SCK, CSN, IO0, IO1, IO2, or IO3)
 *        will not be connected to a physical pin.
 */
#define NRF_QSPI_PIN_NOT_CONNECTED 0xFF

/**
 * @brief Macro for setting proper values to pin registers.
 */

#define NRF_QSPI_PIN_VAL(pin) (pin) == NRF_QSPI_PIN_NOT_CONNECTED ? 0xFFFFFFFF : (pin)

/**
 * @brief QSPI tasks.
 */
typedef enum
{
    /*lint -save -e30*/
    NRF_QSPI_TASK_ACTIVATE   = offsetof(NRF_QSPI_Type, TASKS_ACTIVATE),   /**< Activate the QSPI interface. */
    NRF_QSPI_TASK_READSTART  = offsetof(NRF_QSPI_Type, TASKS_READSTART),  /**< Start transfer from external flash memory to internal RAM. */
    NRF_QSPI_TASK_WRITESTART = offsetof(NRF_QSPI_Type, TASKS_WRITESTART), /**< Start transfer from internal RAM to external flash memory. */
    NRF_QSPI_TASK_ERASESTART = offsetof(NRF_QSPI_Type, TASKS_ERASESTART), /**< Start external flash memory erase operation. */
    NRF_QSPI_TASK_DEACTIVATE = offsetof(NRF_QSPI_Type, TASKS_DEACTIVATE), /**< Deactivate the QSPI interface. */
    /*lint -restore*/
} nrf_qspi_task_t;

/**
 * @brief QSPI events.
 */
typedef enum
{
    /*lint -save -e30*/
    NRF_QSPI_EVENT_READY = offsetof(NRF_QSPI_Type, EVENTS_READY) /**< QSPI peripheral is ready after it executes any task. */
    /*lint -restore*/
} nrf_qspi_event_t;

/**
 * @brief QSPI interrupts.
 */
typedef enum
{
    NRF_QSPI_INT_READY_MASK = QSPI_INTENSET_READY_Msk /**< Interrupt on READY event. */
} nrf_qspi_int_mask_t;

/**
 * @brief QSPI frequency divider values.
 */
typedef enum
{
    NRF_QSPI_FREQ_32MDIV1,  /**< 32.0 MHz. */
    NRF_QSPI_FREQ_32MDIV2,  /**< 16.0 MHz. */
    NRF_QSPI_FREQ_32MDIV3,  /**< 10.6 MHz. */
    NRF_QSPI_FREQ_32MDIV4,  /**< 8.00 MHz. */
    NRF_QSPI_FREQ_32MDIV5,  /**< 6.40 MHz. */
    NRF_QSPI_FREQ_32MDIV6,  /**< 5.33 MHz. */
    NRF_QSPI_FREQ_32MDIV7,  /**< 4.57 MHz. */
    NRF_QSPI_FREQ_32MDIV8,  /**< 4.00 MHz. */
    NRF_QSPI_FREQ_32MDIV9,  /**< 3.55 MHz. */
    NRF_QSPI_FREQ_32MDIV10, /**< 3.20 MHz. */
    NRF_QSPI_FREQ_32MDIV11, /**< 2.90 MHz. */
    NRF_QSPI_FREQ_32MDIV12, /**< 2.66 MHz. */
    NRF_QSPI_FREQ_32MDIV13, /**< 2.46 MHz. */
    NRF_QSPI_FREQ_32MDIV14, /**< 2.29 MHz. */
    NRF_QSPI_FREQ_32MDIV15, /**< 2.13 MHz. */
    NRF_QSPI_FREQ_32MDIV16, /**< 2.00 MHz. */
} nrf_qspi_frequency_t;

/**
 * @brief Interface configuration for a read operation.
 */
typedef enum
{
    NRF_QSPI_READOC_FASTREAD = QSPI_IFCONFIG0_READOC_FASTREAD, /**< Single data line SPI. FAST_READ (opcode 0x0B). */
    NRF_QSPI_READOC_READ2O   = QSPI_IFCONFIG0_READOC_READ2O,   /**< Dual data line SPI. READ2O (opcode 0x3B). */
    NRF_QSPI_READOC_READ2IO  = QSPI_IFCONFIG0_READOC_READ2IO,  /**< Dual data line SPI. READ2IO (opcode 0xBB). */
    NRF_QSPI_READOC_READ4O   = QSPI_IFCONFIG0_READOC_READ4O,   /**< Quad data line SPI. READ4O (opcode 0x6B). */
    NRF_QSPI_READOC_READ4IO  = QSPI_IFCONFIG0_READOC_READ4IO   /**< Quad data line SPI. READ4IO (opcode 0xEB). */
} nrf_qspi_readoc_t;

/**
 * @brief Interface configuration for a write operation.
 */
typedef enum
{
    NRF_QSPI_WRITEOC_PP    = QSPI_IFCONFIG0_WRITEOC_PP,    /**< Single data line SPI. PP (opcode 0x02). */
    NRF_QSPI_WRITEOC_PP2O  = QSPI_IFCONFIG0_WRITEOC_PP2O,  /**< Dual data line SPI. PP2O (opcode 0xA2). */
    NRF_QSPI_WRITEOC_PP4O  = QSPI_IFCONFIG0_WRITEOC_PP4O,  /**< Quad data line SPI. PP4O (opcode 0x32). */
    NRF_QSPI_WRITEOC_PP4IO = QSPI_IFCONFIG0_WRITEOC_PP4IO, /**< Quad data line SPI. READ4O (opcode 0x38). */
} nrf_qspi_writeoc_t;

/**
 * @brief Interface configuration for addressing mode.
 */
typedef enum
{
    NRF_QSPI_ADDRMODE_24BIT = QSPI_IFCONFIG0_ADDRMODE_24BIT, /**< 24-bit addressing. */
    NRF_QSPI_ADDRMODE_32BIT = QSPI_IFCONFIG0_ADDRMODE_32BIT  /**< 32-bit addressing. */
} nrf_qspi_addrmode_t;

/**
 * @brief QSPI SPI mode. Polarization and phase configuration.
 */
typedef enum
{
    NRF_QSPI_MODE_0 = QSPI_IFCONFIG1_SPIMODE_MODE0, /**< Mode 0 (CPOL=0, CPHA=0). */
    NRF_QSPI_MODE_1 = QSPI_IFCONFIG1_SPIMODE_MODE3  /**< Mode 1 (CPOL=1, CPHA=1). */
} nrf_qspi_spi_mode_t;

/**
 * @brief Addressing configuration mode.
 */
typedef enum
{
    NRF_QSPI_ADDRCONF_MODE_NOINSTR = QSPI_ADDRCONF_MODE_NoInstr, /**< Do not send any instruction. */
    NRF_QSPI_ADDRCONF_MODE_OPCODE  = QSPI_ADDRCONF_MODE_Opcode,  /**< Send opcode. */
    NRF_QSPI_ADDRCONF_MODE_OPBYTE0 = QSPI_ADDRCONF_MODE_OpByte0, /**< Send opcode, byte0. */
    NRF_QSPI_ADDRCONF_MODE_ALL     = QSPI_ADDRCONF_MODE_All      /**< Send opcode, byte0, byte1. */
} nrf_qspi_addrconfig_mode_t;

/**
 * @brief Erasing data length.
 */
typedef enum
{
    NRF_QSPI_ERASE_LEN_4KB  = QSPI_ERASE_LEN_LEN_4KB,  /**< Erase 4 kB block (flash command 0x20). */
    NRF_QSPI_ERASE_LEN_64KB = QSPI_ERASE_LEN_LEN_64KB, /**< Erase 64 kB block (flash command 0xD8). */
    NRF_QSPI_ERASE_LEN_ALL  = QSPI_ERASE_LEN_LEN_All   /**< Erase all (flash command 0xC7). */
} nrf_qspi_erase_len_t;

/**
 * @brief Custom instruction length.
 */
typedef enum
{
    NRF_QSPI_CINSTR_LEN_1B = QSPI_CINSTRCONF_LENGTH_1B, /**< Send opcode only. */
    NRF_QSPI_CINSTR_LEN_2B = QSPI_CINSTRCONF_LENGTH_2B, /**< Send opcode, CINSTRDAT0.BYTE0. */
    NRF_QSPI_CINSTR_LEN_3B = QSPI_CINSTRCONF_LENGTH_3B, /**< Send opcode, CINSTRDAT0.BYTE0 -> CINSTRDAT0.BYTE1. */
    NRF_QSPI_CINSTR_LEN_4B = QSPI_CINSTRCONF_LENGTH_4B, /**< Send opcode, CINSTRDAT0.BYTE0 -> CINSTRDAT0.BYTE2. */
    NRF_QSPI_CINSTR_LEN_5B = QSPI_CINSTRCONF_LENGTH_5B, /**< Send opcode, CINSTRDAT0.BYTE0 -> CINSTRDAT0.BYTE3. */
    NRF_QSPI_CINSTR_LEN_6B = QSPI_CINSTRCONF_LENGTH_6B, /**< Send opcode, CINSTRDAT0.BYTE0 -> CINSTRDAT1.BYTE4. */
    NRF_QSPI_CINSTR_LEN_7B = QSPI_CINSTRCONF_LENGTH_7B, /**< Send opcode, CINSTRDAT0.BYTE0 -> CINSTRDAT1.BYTE5. */
    NRF_QSPI_CINSTR_LEN_8B = QSPI_CINSTRCONF_LENGTH_8B, /**< Send opcode, CINSTRDAT0.BYTE0 -> CINSTRDAT1.BYTE6. */
    NRF_QSPI_CINSTR_LEN_9B = QSPI_CINSTRCONF_LENGTH_9B  /**< Send opcode, CINSTRDAT0.BYTE0 -> CINSTRDAT1.BYTE7. */
} nrf_qspi_cinstr_len_t;

/**
 * @brief Pins configuration.
 */
typedef struct
{
    uint8_t sck_pin; /**< SCK pin number. */
    uint8_t csn_pin; /**< Chip select pin number. */
    uint8_t io0_pin; /**< IO0/MOSI pin number. */
    uint8_t io1_pin; /**< IO1/MISO pin number. */
    uint8_t io2_pin; /**< IO2 pin number (optional).
                      * Set to @ref NRF_QSPI_PIN_NOT_CONNECTED if this signal is not needed.
                      */
    uint8_t io3_pin; /**< IO3 pin number (optional).
                      * Set to @ref NRF_QSPI_PIN_NOT_CONNECTED if this signal is not needed.
                      */
} nrf_qspi_pins_t;

/**
 * @brief Custom instruction configuration.
 */
typedef struct
{
    uint8_t               opcode;    /**< Opcode used in custom instruction transmission. */
    nrf_qspi_cinstr_len_t length;    /**< Length of the custom instruction data. */
    bool                  io2_level; /**< I/O line level during transmission. */
    bool                  io3_level; /**< I/O line level during transmission. */
    bool                  wipwait;   /**< Wait if a Wait in Progress bit is set in the memory status byte. */
    bool                  wren;      /**< Send write enable before instruction. */
} nrf_qspi_cinstr_conf_t;

/**
 * @brief Addressing mode register configuration. See @ref nrf_qspi_addrconfig_set
 */
typedef struct
{
    uint8_t                    opcode;  /**< Opcode used to enter proper addressing mode. */
    uint8_t                    byte0;   /**< Byte following the opcode. */
    uint8_t                    byte1;   /**< Byte following byte0. */
    nrf_qspi_addrconfig_mode_t mode;    /**< Extended addresing mode. */
    bool                       wipwait; /**< Enable/disable waiting for complete operation execution. */
    bool                       wren;    /**< Send write enable before instruction. */
} nrf_qspi_addrconfig_conf_t;

/**
 * @brief Structure with QSPI protocol interface configuration.
 */
typedef struct
{
    nrf_qspi_readoc_t   readoc;    /**< Read operation code. */
    nrf_qspi_writeoc_t  writeoc;   /**< Write operation code. */
    nrf_qspi_addrmode_t addrmode;  /**< Addresing mode (24-bit or 32-bit). */
    bool                dpmconfig; /**< Enable the Deep Power-down Mode (DPM) feature. */
} nrf_qspi_prot_conf_t;

/**
 * @brief QSPI physical interface configuration.
 */
typedef struct
{
    uint8_t              sck_delay; /**< tSHSL, tWHSL, and tSHWL in number of 16 MHz periods (62.5ns). */
    bool                 dpmen;     /**< Enable the DPM feature. */
    nrf_qspi_spi_mode_t  spi_mode;  /**< SPI phase and polarization. */
    nrf_qspi_frequency_t sck_freq;  /**< SCK frequency given as enum @ref nrf_qspi_frequency_t. */
} nrf_qspi_phy_conf_t;

/**
 * @brief Function for activating a specific QSPI task.
 *
 * @param[in] p_reg Pointer to the peripheral register structure.
 * @param[in] task  Task to activate.
 */
__STATIC_INLINE void nrf_qspi_task_trigger(NRF_QSPI_Type * p_reg, nrf_qspi_task_t task);

/**
 * @brief Function for getting the address of a specific QSPI task register.
 *
 * @param[in] p_reg Pointer to the peripheral register structure.
 * @param[in] task  Requested task.
 *
 * @return Address of the specified task register.
 */
__STATIC_INLINE uint32_t nrf_qspi_task_address_get(NRF_QSPI_Type const * p_reg,
                                                   nrf_qspi_task_t       task);

/**
 * @brief Function for clearing a specific QSPI event.
 *
 * @param[in] p_reg      Pointer to the peripheral register structure.
 * @param[in] qspi_event Event to clear.
 */
__STATIC_INLINE void nrf_qspi_event_clear(NRF_QSPI_Type *  p_reg, nrf_qspi_event_t qspi_event);

/**
 * @brief Function for checking the state of a specific SPI event.
 *
 * @param[in] p_reg      Pointer to the peripheral register structure.
 * @param[in] qspi_event Event to check.
 *
 * @retval true  If the event is set.
 * @retval false If the event is not set.
 */
__STATIC_INLINE bool nrf_qspi_event_check(NRF_QSPI_Type const * p_reg, nrf_qspi_event_t qspi_event);

/**
 * @brief Function for getting the address of a specific QSPI event register.
 *
 * @param[in] p_reg      Pointer to the peripheral register structure.
 * @param[in] qspi_event Requested event.
 *
 * @return Address of the specified event register.
 */
__STATIC_INLINE uint32_t * nrf_qspi_event_address_get(NRF_QSPI_Type const * p_reg,
                                                      nrf_qspi_event_t      qspi_event);

/**
 * @brief Function for enabling specified interrupts.
 *
 * @param[in] p_reg          Pointer to the peripheral register structure.
 * @param[in] qspi_int_mask  Interrupts to enable.
 */
__STATIC_INLINE void nrf_qspi_int_enable(NRF_QSPI_Type * p_reg, uint32_t qspi_int_mask);

/**
 * @brief Function for disabling specified interrupts.
 *
 * @param[in] p_reg          Pointer to the peripheral register structure.
 * @param[in] qspi_int_mask  Interrupts to disable.
 */
__STATIC_INLINE void nrf_qspi_int_disable(NRF_QSPI_Type * p_reg, uint32_t qspi_int_mask);

/**
 * @brief Function for retrieving the state of a given interrupt.
 *
 * @param[in] p_reg    Pointer to the peripheral register structure.
 * @param[in] qspi_int Interrupt to check.
 *
 * @retval true  If the interrupt is enabled.
 * @retval false If the interrupt is not enabled.
 */
__STATIC_INLINE bool nrf_qspi_int_enable_check(NRF_QSPI_Type const * p_reg,
                                               nrf_qspi_int_mask_t   qspi_int);

/**
 * @brief Function for enabling the QSPI peripheral.
 *
 * @param[in] p_reg Pointer to the peripheral register structure.
 */
__STATIC_INLINE void nrf_qspi_enable(NRF_QSPI_Type * p_reg);

/**
 * @brief Function for disabling the QSPI peripheral.
 *
 * @param[in] p_reg Pointer to the peripheral register structure.
 */
__STATIC_INLINE void nrf_qspi_disable(NRF_QSPI_Type * p_reg);

/**
 * @brief Function for configuring QSPI pins.
 *
 * If a given signal is not needed, pass the @ref NRF_QSPI_PIN_NOT_CONNECTED
 * value instead of its pin number.
 *
 * @param[in] p_reg  Pointer to the peripheral register structure.
 * @param[in] p_pins Pointer to the pins configuration structure. See @ref nrf_qspi_pins_t.
 */
__STATIC_INLINE void nrf_qspi_pins_set(NRF_QSPI_Type *         p_reg,
                                       const nrf_qspi_pins_t * p_pins);

/**
 * @brief Function for setting the QSPI XIPOFFSET register.
 *
 * @param[in] p_reg      Pointer to the peripheral register structure.
 * @param[in] xip_offset Address offset in the external memory for Execute in Place operation.
 */
__STATIC_INLINE void nrf_qspi_xip_offset_set(NRF_QSPI_Type * p_reg,
                                             uint32_t        xip_offset);

/**
 * @brief Function for setting the QSPI IFCONFIG0 register.
 *
 * @param[in] p_reg    Pointer to the peripheral register structure.
 * @param[in] p_config Pointer to the QSPI protocol interface configuration structure. See @ref nrf_qspi_prot_conf_t.
 */
__STATIC_INLINE void nrf_qspi_ifconfig0_set(NRF_QSPI_Type *              p_reg,
                                            const nrf_qspi_prot_conf_t * p_config);

/**
 * @brief Function for setting the QSPI IFCONFIG1 register.
 *
 * @param[in] p_reg    Pointer to the peripheral register structure.
 * @param[in] p_config Pointer to the QSPI physical interface configuration structure. See @ref nrf_qspi_phy_conf_t.
 */
__STATIC_INLINE void nrf_qspi_ifconfig1_set(NRF_QSPI_Type *             p_reg,
                                            const nrf_qspi_phy_conf_t * p_config);

/**
 * @brief Function for setting the QSPI ADDRCONF register.
 *
 * Function must be executed before sending task NRF_QSPI_TASK_ACTIVATE. Data stored in the structure
 * is sent during the start of the peripheral. Remember that the reset instruction can set
 * addressing mode to default in the memory device. If memory reset is necessary before configuring
 * the addressing mode, use custom instruction feature instead of this function.
 * Case with reset: Enable the peripheral without setting ADDRCONF register, send reset instructions
 * using a custom instruction feature (reset enable and then reset), set proper addressing mode
 * using the custom instruction feature.
 *
 * @param[in] p_reg    Pointer to the peripheral register structure.
 * @param[in] p_config Pointer to the addressing mode configuration structure. See @ref nrf_qspi_addrconfig_conf_t.
*/
__STATIC_INLINE void nrf_qspi_addrconfig_set(NRF_QSPI_Type *                    p_reg,
                                             const nrf_qspi_addrconfig_conf_t * p_config);

/**
 * @brief Function for setting write data into the peripheral register (without starting the process).
 *
 * @param[in] p_reg     Pointer to the peripheral register structure.
 * @param[in] p_buffer  Pointer to the writing buffer.
 * @param[in] length    Lenght of the writing data.
 * @param[in] dest_addr Address in memory to write to.
 */
__STATIC_INLINE void nrf_qspi_write_buffer_set(NRF_QSPI_Type * p_reg,
                                               void const *    p_buffer,
                                               uint32_t        length,
                                               uint32_t        dest_addr);

/**
 * @brief Function for setting read data into the peripheral register (without starting the process).
 *
 * @param[in]  p_reg    Pointer to the peripheral register structure.
 * @param[out] p_buffer Pointer to the reading buffer.
 * @param[in]  length   Length of the read data.
 * @param[in]  src_addr Address in memory to read from.
 */
__STATIC_INLINE void nrf_qspi_read_buffer_set(NRF_QSPI_Type * p_reg,
                                              void *          p_buffer,
                                              uint32_t        length,
                                              uint32_t        src_addr);

/**
 * @brief Function for setting erase data into the peripheral register (without starting the process).
 *
 * @param[in] p_reg      Pointer to the peripheral register structure.
 * @param[in] erase_addr Start address to erase. Address must have padding set to 4 bytes.
 * @param[in] len        Size of erasing area.
 */
__STATIC_INLINE void nrf_qspi_erase_ptr_set(NRF_QSPI_Type *      p_reg,
                                            uint32_t             erase_addr,
                                            nrf_qspi_erase_len_t len);

/**
 * @brief Function for getting the peripheral status register.
 *
 * @param[in] p_reg Pointer to the peripheral register structure.
 *
 * @return Peripheral status register.
 */
__STATIC_INLINE uint32_t nrf_qspi_status_reg_get(NRF_QSPI_Type const * p_reg);

/**
 * @brief Function for getting the device status register stored in the peripheral status register.
 *
 * @param[in] p_reg Pointer to the peripheral register structure.
 *
 * @return Device status register (lower byte).
 */
__STATIC_INLINE uint8_t nrf_qspi_sreg_get(NRF_QSPI_Type const * p_reg);

/**
 * @brief Function for checking if the peripheral is busy or not.
 *
 * @param[in] p_reg Pointer to the peripheral register structure.
 *
 * @retval true  If QSPI is busy.
 * @retval false If QSPI is ready.
 */
__STATIC_INLINE bool nrf_qspi_busy_check(NRF_QSPI_Type const * p_reg);

/**
 * @brief Function for setting registers sending with custom instruction transmission.
 *
 * This function can be ommited when using NRF_QSPI_CINSTR_LEN_1B as the length argument
 * (sending only opcode without data).
 *
 * @param[in] p_reg     Pointer to the peripheral register structure.
 * @param[in] length    Length of the custom instruction data.
 * @param[in] p_tx_data Pointer to the data to send with the custom instruction.
 */
__STATIC_INLINE void nrf_qspi_cinstrdata_set(NRF_QSPI_Type *       p_reg,
                                             nrf_qspi_cinstr_len_t length,
                                             void const *          p_tx_data);

/**
 * @brief Function for getting data from register after custom instruction transmission.
 * @param[in] p_reg     Pointer to the peripheral register structure.
 * @param[in] length    Length of the custom instruction data.
 * @param[in] p_rx_data Pointer to the reading buffer.
 */
__STATIC_INLINE void nrf_qspi_cinstrdata_get(NRF_QSPI_Type const * p_reg,
                                             nrf_qspi_cinstr_len_t length,
                                             void *                p_rx_data);

/**
 * @brief Function for sending custom instruction to external memory.
 *
 * @param[in] p_reg    Pointer to the peripheral register structure.
 * @param[in] p_config Pointer to the custom instruction configuration structure. See @ref nrf_qspi_cinstr_conf_t.
 */

__STATIC_INLINE void nrf_qspi_cinstr_transfer_start(NRF_QSPI_Type *                p_reg,
                                                    const nrf_qspi_cinstr_conf_t * p_config);

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrf_qspi_task_trigger(NRF_QSPI_Type * p_reg, nrf_qspi_task_t task)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)task)) = 0x1UL;
}

__STATIC_INLINE uint32_t nrf_qspi_task_address_get(NRF_QSPI_Type const * p_reg,
                                                   nrf_qspi_task_t       task)
{
    return ((uint32_t)p_reg + (uint32_t)task);
}

__STATIC_INLINE void nrf_qspi_event_clear(NRF_QSPI_Type * p_reg, nrf_qspi_event_t qspi_event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)qspi_event)) = 0x0UL;
}

__STATIC_INLINE bool nrf_qspi_event_check(NRF_QSPI_Type const * p_reg, nrf_qspi_event_t qspi_event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)qspi_event);
}

__STATIC_INLINE uint32_t * nrf_qspi_event_address_get(NRF_QSPI_Type const * p_reg,
                                                      nrf_qspi_event_t      qspi_event)
{
    return (uint32_t *)((uint8_t *)p_reg + (uint32_t)qspi_event);
}

__STATIC_INLINE void nrf_qspi_int_enable(NRF_QSPI_Type * p_reg, uint32_t qspi_int_mask)
{
    p_reg->INTENSET = qspi_int_mask;
}

__STATIC_INLINE void nrf_qspi_int_disable(NRF_QSPI_Type * p_reg, uint32_t qspi_int_mask)
{
    p_reg->INTENCLR = qspi_int_mask;
}

__STATIC_INLINE bool nrf_qspi_int_enable_check(NRF_QSPI_Type const * p_reg,
                                               nrf_qspi_int_mask_t   qspi_int)
{
    return (bool)(p_reg->INTENSET & qspi_int);
}

__STATIC_INLINE void nrf_qspi_enable(NRF_QSPI_Type * p_reg)
{
    p_reg->ENABLE = (QSPI_ENABLE_ENABLE_Enabled << QSPI_ENABLE_ENABLE_Pos);
}

__STATIC_INLINE void nrf_qspi_disable(NRF_QSPI_Type * p_reg)
{
    p_reg->ENABLE = (QSPI_ENABLE_ENABLE_Disabled << QSPI_ENABLE_ENABLE_Pos);
}

__STATIC_INLINE void nrf_qspi_pins_set(NRF_QSPI_Type * p_reg, const nrf_qspi_pins_t * p_pins)
{
    p_reg->PSEL.SCK = NRF_QSPI_PIN_VAL(p_pins->sck_pin);
    p_reg->PSEL.CSN = NRF_QSPI_PIN_VAL(p_pins->csn_pin);
    p_reg->PSEL.IO0 = NRF_QSPI_PIN_VAL(p_pins->io0_pin);
    p_reg->PSEL.IO1 = NRF_QSPI_PIN_VAL(p_pins->io1_pin);
    p_reg->PSEL.IO2 = NRF_QSPI_PIN_VAL(p_pins->io2_pin);
    p_reg->PSEL.IO3 = NRF_QSPI_PIN_VAL(p_pins->io3_pin);
}

__STATIC_INLINE void nrf_qspi_xip_offset_set(NRF_QSPI_Type * p_reg,
                                             uint32_t        xip_offset)
{
    p_reg->XIPOFFSET = xip_offset;
}

__STATIC_INLINE void nrf_qspi_ifconfig0_set(NRF_QSPI_Type *              p_reg,
                                            const nrf_qspi_prot_conf_t * p_config)
{
    uint32_t config = p_config->readoc;
    config |= ((uint32_t)p_config->writeoc)    << QSPI_IFCONFIG0_WRITEOC_Pos;
    config |= ((uint32_t)p_config->addrmode)   << QSPI_IFCONFIG0_ADDRMODE_Pos;
    config |= (p_config->dpmconfig ? 1U : 0U ) << QSPI_IFCONFIG0_DPMENABLE_Pos;

    p_reg->IFCONFIG0 = config;
}

__STATIC_INLINE void nrf_qspi_ifconfig1_set(NRF_QSPI_Type *             p_reg,
                                            const nrf_qspi_phy_conf_t * p_config)
{
    // IFCONFIG1 mask for reserved fields in the register.
    uint32_t config = p_reg->IFCONFIG1 & 0x00FFFF00;
    config |= p_config->sck_delay;
    config |= (p_config->dpmen ? 1U : 0U)      << QSPI_IFCONFIG1_DPMEN_Pos;
    config |= ((uint32_t)(p_config->spi_mode)) << QSPI_IFCONFIG1_SPIMODE_Pos;
    config |= ((uint32_t)(p_config->sck_freq)) << QSPI_IFCONFIG1_SCKFREQ_Pos;

    p_reg->IFCONFIG1 = config;
}

__STATIC_INLINE void nrf_qspi_addrconfig_set(NRF_QSPI_Type *                    p_reg,
                                             const nrf_qspi_addrconfig_conf_t * p_config)
{
    uint32_t config = p_config->opcode;
    config |= ((uint32_t)p_config->byte0)   << QSPI_ADDRCONF_BYTE0_Pos;
    config |= ((uint32_t)p_config->byte1)   << QSPI_ADDRCONF_BYTE1_Pos;
    config |= ((uint32_t)(p_config->mode))  << QSPI_ADDRCONF_MODE_Pos;
    config |= (p_config->wipwait ? 1U : 0U) << QSPI_ADDRCONF_WIPWAIT_Pos;
    config |= (p_config->wren    ? 1U : 0U) << QSPI_ADDRCONF_WREN_Pos;

    p_reg->ADDRCONF = config;
}

__STATIC_INLINE void nrf_qspi_write_buffer_set(NRF_QSPI_Type * p_reg,
                                               void const    * p_buffer,
                                               uint32_t        length,
                                               uint32_t        dest_addr)
{
    p_reg->WRITE.DST = dest_addr;
    p_reg->WRITE.SRC = (uint32_t) p_buffer;
    p_reg->WRITE.CNT = length;
}

__STATIC_INLINE void nrf_qspi_read_buffer_set(NRF_QSPI_Type * p_reg,
                                              void          * p_buffer,
                                              uint32_t        length,
                                              uint32_t        src_addr)
{
    p_reg->READ.SRC = src_addr;
    p_reg->READ.DST = (uint32_t) p_buffer;
    p_reg->READ.CNT = length;
}

__STATIC_INLINE void nrf_qspi_erase_ptr_set(NRF_QSPI_Type *      p_reg,
                                            uint32_t             erase_addr,
                                            nrf_qspi_erase_len_t len)
{
    p_reg->ERASE.PTR = erase_addr;
    p_reg->ERASE.LEN = len;
}

__STATIC_INLINE uint32_t nrf_qspi_status_reg_get(NRF_QSPI_Type const * p_reg)
{
    return p_reg->STATUS;
}

__STATIC_INLINE uint8_t nrf_qspi_sreg_get(NRF_QSPI_Type const * p_reg)
{
    return (uint8_t)(p_reg->STATUS & QSPI_STATUS_SREG_Msk) >> QSPI_STATUS_SREG_Pos;
}

__STATIC_INLINE bool nrf_qspi_busy_check(NRF_QSPI_Type const * p_reg)
{
    return ((p_reg->STATUS & QSPI_STATUS_READY_Msk) >>
            QSPI_STATUS_READY_Pos) == QSPI_STATUS_READY_BUSY;
}

__STATIC_INLINE void nrf_qspi_cinstrdata_set(NRF_QSPI_Type *       p_reg,
                                             nrf_qspi_cinstr_len_t length,
                                             void const *          p_tx_data)
{
    uint32_t reg = 0;
    uint8_t const *p_tx_data_8 = (uint8_t const *) p_tx_data;

    // Load custom instruction.
    switch (length)
    {
        case NRF_QSPI_CINSTR_LEN_9B:
            reg |= ((uint32_t)p_tx_data_8[7]) << QSPI_CINSTRDAT1_BYTE7_Pos;
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_8B:
            reg |= ((uint32_t)p_tx_data_8[6]) << QSPI_CINSTRDAT1_BYTE6_Pos;
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_7B:
            reg |= ((uint32_t)p_tx_data_8[5]) << QSPI_CINSTRDAT1_BYTE5_Pos;
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_6B:
            reg |= ((uint32_t)p_tx_data_8[4]);
            p_reg->CINSTRDAT1 = reg;
            reg = 0;
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_5B:
            reg |= ((uint32_t)p_tx_data_8[3]) << QSPI_CINSTRDAT0_BYTE3_Pos;
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_4B:
            reg |= ((uint32_t)p_tx_data_8[2]) << QSPI_CINSTRDAT0_BYTE2_Pos;
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_3B:
            reg |= ((uint32_t)p_tx_data_8[1]) << QSPI_CINSTRDAT0_BYTE1_Pos;
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_2B:
            reg |= ((uint32_t)p_tx_data_8[0]);
            p_reg->CINSTRDAT0 = reg;
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_1B:
            /* Send only opcode. Case to avoid compiler warnings. */
            break;
        default:
            break;
    }
}

__STATIC_INLINE void nrf_qspi_cinstrdata_get(NRF_QSPI_Type const * p_reg,
                                             nrf_qspi_cinstr_len_t length,
                                             void *                p_rx_data)
{
    uint8_t *p_rx_data_8 = (uint8_t *) p_rx_data;

    uint32_t reg = p_reg->CINSTRDAT1;
    switch (length)
    {
        case NRF_QSPI_CINSTR_LEN_9B:
            p_rx_data_8[7] = (uint8_t)(reg >> QSPI_CINSTRDAT1_BYTE7_Pos);
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_8B:
            p_rx_data_8[6] = (uint8_t)(reg >> QSPI_CINSTRDAT1_BYTE6_Pos);
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_7B:
            p_rx_data_8[5] = (uint8_t)(reg >> QSPI_CINSTRDAT1_BYTE5_Pos);
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_6B:
            p_rx_data_8[4] = (uint8_t)(reg);
            /* fall-through */
        default:
            break;
    }

    reg = p_reg->CINSTRDAT0;
    switch (length)
    {
        case NRF_QSPI_CINSTR_LEN_5B:
            p_rx_data_8[3] = (uint8_t)(reg >> QSPI_CINSTRDAT0_BYTE3_Pos);
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_4B:
            p_rx_data_8[2] = (uint8_t)(reg >> QSPI_CINSTRDAT0_BYTE2_Pos);
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_3B:
            p_rx_data_8[1] = (uint8_t)(reg >> QSPI_CINSTRDAT0_BYTE1_Pos);
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_2B:
            p_rx_data_8[0] = (uint8_t)(reg);
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_1B:
            /* Send only opcode. Case to avoid compiler warnings. */
            break;
        default:
            break;
    }
}

__STATIC_INLINE void nrf_qspi_cinstr_transfer_start(NRF_QSPI_Type *                p_reg,
                                                    const nrf_qspi_cinstr_conf_t * p_config)
{
    p_reg->CINSTRCONF = (((uint32_t)p_config->opcode    << QSPI_CINSTRCONF_OPCODE_Pos) |
                         ((uint32_t)p_config->length    << QSPI_CINSTRCONF_LENGTH_Pos) |
                         ((uint32_t)p_config->io2_level << QSPI_CINSTRCONF_LIO2_Pos) |
                         ((uint32_t)p_config->io3_level << QSPI_CINSTRCONF_LIO3_Pos) |
                         ((uint32_t)p_config->wipwait   << QSPI_CINSTRCONF_WIPWAIT_Pos) |
                         ((uint32_t)p_config->wren      << QSPI_CINSTRCONF_WREN_Pos));
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_QSPI_H__
