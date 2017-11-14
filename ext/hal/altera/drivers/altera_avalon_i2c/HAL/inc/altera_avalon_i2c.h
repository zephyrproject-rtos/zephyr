/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2016 Altera Corporation, San Jose, California, USA.           *
* All rights reserved.                                                        *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
*                                                                             *
******************************************************************************/

/*! \file
 *  Altera Avalon I2C Controller API
 */
 
#ifndef __ALT_AVALON_I2C_H__
#define __ALT_AVALON_I2C_H__

#include <stddef.h>
#include <errno.h>

#include "sys/alt_dev.h"
#include "alt_types.h"
#include "altera_avalon_i2c_regs.h"
#include "os/alt_sem.h"
#include "os/alt_flag.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * The function alt_find_dev() is used to search the device list "list" to
 * locate a device named "name". If a match is found, then a pointer to the
 * device is returned, otherwise NULL is returned.
 */
extern alt_dev* alt_find_dev (const char* name, alt_llist* list);

/* Callback routine type definition */
typedef void (*alt_avalon_i2c_callback)(void *context);

/******************************************************************************/
/*! \addtogroup ALT_AVALON_I2C I2C Controller API
 *
 * This document defines the API for configuring and managing the Avalon I2C controller.
 *
 * The I2C controller provides support for a communication link between integrated
 * circuits on a board. It is a simple two-wire bus which consists of a serial
 * data line (SDA) and a serial clock (SCL) for use in applications such as
 * temperature sensors and voltage level translators to EEPROMs, A/D and D/A
 * converters, CODECs, and many types of microprocessors.
 *
 * The Avalon (FPGA) I2C controller IP enables system
 * software to communicate serially with I2C buses. Each I2C controller can
 * operate in master mode only, and supports standard mode of up to 100
 * kilobits per second (Kbps) or fast mode of up to 400 Kbps. 
 * 
 * 
 * Features of the I2C Controller:
 *  * Supports both 100 KBps and 400 KBps modes
 *  * Supports I2C master mode
 *  * Support both 7-bit and 10-bit addressing modes
 *  * Mixed read and write combined-format transactions
 *  * Bulk transmit and receive
 *  * Interrupt support
 *  * CMD and RX Fifos  
 *
 * For complete details on the configuration and operation of the I2C controller,
 * consult the following references:
 *  * <em>Altera Avalon I2C Core Functional Description</em>
 *
 * @{
 */



/* Status Codes */
typedef alt_u32 ALT_AVALON_I2C_STATUS_CODE;
#define ALT_AVALON_I2C_TRUE (1)
#define ALT_AVALON_I2C_FALSE (0)
#define ALT_AVALON_I2C_SUCCESS (0)
#define ALT_AVALON_I2C_ERROR (-1)
#define ALT_AVALON_I2C_TIMEOUT (-2)
#define ALT_AVALON_I2C_BAD_ARG (-3)
#define ALT_AVALON_I2C_RANGE (-4)
#define ALT_AVALON_I2C_NACK_ERR (-5)
#define ALT_AVALON_I2C_ARB_LOST_ERR (-6)
#define ALT_AVALON_I2C_BUSY (-7)

/******************************************************************************/
/*! \addtogroup INSTANCEFUNCTIONS I2C Instance Init Functions
 *
 * These functions are used by the system structure when initializing
 * the I2C instance.  These functions are not typically called by a 
 * user of the I2C IP except for the alt_avalon_i2c_open()
 * function which is required to obtain a pointer to the desired I2C
 * instance.
 * 
 * @{
 */
 
 /* constant used in the alt_avalon_i2c_send_address routine */
 #define TARGET_ADDR_MASK_10BIT 0x7800
 
 /*!
 * This type enumerates the two addressing modes formats supported by the I2C
 * controller.
 *
 * The I2C controller does not support mixed address format - that is, a 7-bit
 * address transaction followed by a 10-bit address transaction or vice versa -
 * combined format transactions.
 */
typedef enum ALT_AVALON_I2C_ADDR_MODE_e
{
    ALT_AVALON_I2C_ADDR_MODE_7_BIT     = 0,
                                    /*!< 7-Bit Address Format  */
    ALT_AVALON_I2C_ADDR_MODE_10_BIT    = 1
                                    /*!< 10-Bit Address Format */
} ALT_AVALON_I2C_ADDR_MODE_t;
 
 /*!
 * A pointer or handle to the I2C controller device instance. The ALT_AVALON_I2C_DEV_t is
 * initialized by a call to alt_avalon_i2c_init() and subsequently used by the other I2C
 * controller API functions as a reference to a specific device.
 *
 */
typedef struct ALT_AVALON_I2C_DEV_s
{
    
    alt_llist                            llist; 
        /*!< Device linked-list entry */
    const char                           *name;
        /*!< Name of i2c in Qsys system */
    alt_u32                              *i2c_base;
        /*!< Base address of regs */
     alt_u32                              irq_controller_ID;
        /*!< device IRQ controller ID */ 
    alt_u32                              irq_ID;
        /*!< device IRQ ID */    
    alt_avalon_i2c_callback              callback; 
        /*!< Callback routine pointer */
    void                                 *callback_context; 
        /*!< Callback context pointer */ 
    alt_u32                              control; 
        /*!< user define control setting during interrupt registering*/
    alt_u32                              master_target_address;
        /*!< master target address */
    alt_u32                              cmd_fifo_size;
        /*!< cmd fifo */
    alt_u32                              rx_fifo_size;
        /*!< rx fifo */    
    alt_u32                              ip_freq_in_hz;
        /*!< freq of clock to ip */    
    ALT_AVALON_I2C_ADDR_MODE_t           address_mode;
        /*!< address mode 7 or 10 bit */
    ALT_SEM                              (regs_lock);
        /*!< Semaphore used to control access registers
             in multi-threaded mode */ 
} ALT_AVALON_I2C_DEV_t;
 
/*!
 * The allocation of the ALT_AVALON_I2C_DEV_t structure memory
 * and initial settings.
 *
 */ 
#define ALTERA_AVALON_I2C_INSTANCE(name, dev)           \
static ALT_AVALON_I2C_DEV_t dev =                       \
{                                                       \
    ALT_LLIST_ENTRY,                                    \
    name##_NAME,                                        \
    ((alt_u32 *)(name##_BASE)),                         \
    ((alt_u32)(name##_IRQ_INTERRUPT_CONTROLLER_ID)),    \
    ((alt_u32)(name##_IRQ)),                            \
    ((void *) 0x0),                                     \
    ((void *) 0x0),                                     \
    ((alt_u32)(0)),                                     \
    ((alt_u32)(0)),                                     \
    ((alt_u32)(name##_FIFO_DEPTH)),                     \
    ((alt_u32)(name##_FIFO_DEPTH)),                     \
    ((alt_u32)(name##_FREQ)),                           \
    ((ALT_AVALON_I2C_ADDR_MODE_t)(0)),                  \
};

/*!
 * Initialize the specified I2C controller instance. This routine is called from the 
 * ALTERA_AVALON_I2C_INIT macro and is called automatically by alt_sys_init.c
 *
 * This routine disables interrupts, 
 * registers a specific instance of the device with the HAL,
 * and installs an interrupt handler for the device.
 *
 * \param       i2c_dev
 *              The Avalon FPGA I2C controller instance to initialize.
 *
 *
 */
void alt_avalon_i2c_init(ALT_AVALON_I2C_DEV_t *i2c_dev);

/*
 * The macro ALTERA_AVALON_I2C_INIT is called by the auto-generated function
 * alt_sys_init() to initialize a given device instance.
 */
#define ALTERA_AVALON_I2C_INIT(name, dev)       alt_avalon_i2c_init(&dev);

/*!
 * Retrieve a pointer to the i2c block instance
 * Search the list of registered i2c instances for one with the supplied name.
 *
 * \retval       NULL       The return value will be NULL on failure
 * \retval       non-NULL   Success, ptr to i2c device instance returned.
 *
 * \param        *name      Character pointer to name of i2c peripheral as registered
 *                          with the HAL. For example, an i2c controller named "i2c_0"
 *                          in Qsys would be opened by asking for "/dev/i2c_0".
 *
 */
ALT_AVALON_I2C_DEV_t* alt_avalon_i2c_open(const char* name);

/*! @} */
 
/******************************************************************************/
/*! \addtogroup SPEEDANDTIMINGFUNCTIONS Speed and Timing Configuration Functions
 *
 * These functions are used to set the Speed and Timing parameters. 
 * The ALT_AVALON_I2C_MASTER_CONFIG_t structure
 * is used to set or get the speed, address mode, and scl/sda timing counts of the instance. 
 * The timing parameters can be set or retrieved in terms of Hz by using the 
 * alt_avalon_i2c_master_config_speed_get and alt_avalon_i2c_master_config_speed_set functions.
 * 
 * @{
 */

/*!
 * This type enumerates the I2C controller operational speed modes.
 *
 * The I2C controller can operate in standard mode (with data rates 0 to 100 Kbps)
 * or fast mode (with data rates less than or equal to 400 Kbps). Additionally,
 * fast mode devices are downward compatible. For instance, fast mode devices can
 * communicate with standard mode devices in 0 to 100 Kbps I2C bus
 * system. However, standard mode devices are not upward compatible and should not
 * be incorporated in a fast-mode I2C bus system as they cannot follow the higher
 * transfer rate and therefore unpredictable states would occur.
 *
 */
typedef enum ALT_AVALON_I2C_SPEED_e
{
    ALT_AVALON_I2C_SPEED_STANDARD   = 0,
                                    /*!< Standard mode (0 to 100 Kbps) */
    ALT_AVALON_I2C_SPEED_FAST       = 1
                                    /*!< Fast mode (<= 400 Kbps)       */
} ALT_AVALON_I2C_SPEED_t;

/*!
 * This type enumerates the I2C controller max min speeds in hz.
 *
 * The I2C controller can operate in standard mode (with data rates 0 to 100 Kbps)
 * or fast mode (with data rates less than or equal to 400 Kbps). 
 *
 */
typedef enum ALT_AVALON_I2C_MAX_MIN_HZ_e
{
    ALT_AVALON_I2C_FS_MAX_HZ   = 400000,
                                    /*!< Max speed is 400Khz */
    ALT_AVALON_I2C_FS_MIN_HZ   = 100000,
                                    /*!< Min speed is 100Khz  */
    ALT_AVALON_I2C_SS_MAX_HZ   = 100000,
                                    /*!< Max speed is 100Khz */
    ALT_AVALON_I2C_SS_MIN_HZ   = 1
                                    /*!< Min speed is 1hz  */                                    
} ALT_AVALON_I2C_MAX_MIN_HZ_t;

 
#define ALT_AVALON_I2C_DIFF_LCNT_HCNT 60

/*!
 * This type defines a structure for configuration of the SCL high and low counts
 * to ensure proper I/O timing with the device interface.
 *      
 * See: Clock Frequency Configuration section of <em>Chapter 20. I2C
 *      Controller</em> in the <em>Cyclone V Device Handbook Volume 3: Hard
 *      Processor System Technical Reference Manual</em> for a complete discussion
 *      of calculation of the proper SCL clock high and low times.
 */
typedef struct ALT_AVALON_I2C_MASTER_CONFIG_s
{
    ALT_AVALON_I2C_ADDR_MODE_t addr_mode;
                                    /*!< The address mode (7 or 10 bit) when
                                    *   acting as a master.
                                    */
    ALT_AVALON_I2C_SPEED_t     speed_mode;
                                    /*!< The speed mode of the I2C operation.
                                    */
    alt_u16            scl_hcnt;
                                    /*!< The SCL clock high-period count.
                                    */
    alt_u16            scl_lcnt;    
                                    /*!< The SCL clock low-period count.
                                    */
    alt_u16            sda_cnt;    
                                    /*!< The SDA clock hold count.
                                    */
} ALT_AVALON_I2C_MASTER_CONFIG_t;

/*!
 * Populates the master mode configuration structure (type ALT_AVALON_I2C_ADDR_MODE_t) from registers.
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       cfg
 *              [out] Pointer to a ALT_AVALON_I2C_MASTER_CONFIG_t structure for holding
 *              the returned I2C master mode configuration parameters.
 *
 *
 */
void alt_avalon_i2c_master_config_get(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                          ALT_AVALON_I2C_MASTER_CONFIG_t* cfg);

/*!
 * Sets the registers based on the master mode configuration structure (type ALT_AVALON_I2C_ADDR_MODE_t).
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       cfg
 *              Pointer to a ALT_AVALON_I2C_MASTER_CONFIG_t structure holding the desired
 *              I2C master mode operational parameters.
 *
 *
 */
void alt_avalon_i2c_master_config_set(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                          const ALT_AVALON_I2C_MASTER_CONFIG_t* cfg);
                                          
/*!
 * This utility function returns the speed in hz based on the contents of the passed in configuration structure.
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       cfg
 *              A pointer to a master configuration structure.
 *
 * \param       speed_in_hz
 *              [out] Speed (Hz) the I2C bus is currently configured at based on
 *              the cfg structure (not necessarily on the hardware settings).
 *              To get the hardware speed first populate the cfg structure
 *              with the alt_avalon_i2c_master_config_get() function.
 *
 * \retval      ALT_AVALON_I2C_STATUS_CODE       
 *              ALT_AVALON_I2C_SUCCESS indicates successful status
 *              otherwise one of the ALT_AVALON_I2C_* status codes
 *              is returned.  All failing return values are < 0. 
 */
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_master_config_speed_get(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                                const ALT_AVALON_I2C_MASTER_CONFIG_t* cfg,
                                                alt_u32 * speed_in_hz);

/*!
 * This is a utility function that computes parameters for the I2C master
 * configuration that best matches the speed requested. Only the cfg
 * structure passed in is modified.
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       cfg
 *              A pointer to a master configuration structure.
 *
 * \param       speed_in_hz
 *              Desired Speed (Hz) of the I2C bus.
 *
 * \retval      ALT_AVALON_I2C_STATUS_CODE       
 *              ALT_AVALON_I2C_SUCCESS indicates successful status
 *              otherwise one of the ALT_AVALON_I2C_* status codes
 *              is returned.  All failing return values are < 0. 
 *
 */
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_master_config_speed_set(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                                ALT_AVALON_I2C_MASTER_CONFIG_t * cfg,
                                                alt_u32 speed_in_hz);

/*! @} */
                                                
/******************************************************************************/
/*! \addtogroup HELPERFUNCTIONS Helper Functions
 *
 * These helper functions are used by the I2C transaction functions.
 * 
 * @{
 */
 
 /* Descriptive Constants */
#define ALT_AVALON_I2C_READ (1)   
#define ALT_AVALON_I2C_WRITE (0)  
#define ALT_AVALON_I2C_NO_RESTART (0)
#define ALT_AVALON_I2C_NO_STOP (0)
#define ALT_AVALON_I2C_RESTART (1)
#define ALT_AVALON_I2C_STOP (1)
#define ALT_AVALON_I2C_USE_INTERRUPTS (1)
#define ALT_AVALON_I2C_NO_INTERRUPTS (0)
 
/*!
 * Disables the I2C controller.
 *
 * When the I2C controller is disabled, the TX_READY_EN interrupt is cleared.  
 * However, the IP is not designed to be able to disable the IP in the middle
 * of a transaction so this should be avoided.    This disable function must
 * be done at the end of every START -> STOP sequence.  
 * Before doing this function, wait for the IP to not be busy using the 
 * alt_avalon_i2c_is_busy function.
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 *
 */
 void alt_avalon_i2c_disable(ALT_AVALON_I2C_DEV_t *i2c_dev);

/*!
 * Enables the I2C controller.  This function is done at the beginning of
 * every START->STOP sequence.  
 * This function returns  ALT_AVALON_I2C_BUSY status if the ip is already
 * enabled.  In that case nothing is done.  
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \retval      ALT_AVALON_I2C_BUSY
 *              Indicates the I2C controller is already enabled.
 * \retval      ALT_AVALON_I2C_SUCCESS
 *              Indicates the I2C controller has been successfully enabled. 
 * 
 */
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_enable(ALT_AVALON_I2C_DEV_t *i2c_dev); 
 
/*!
 * Returns ALT_AVALON_I2C_TRUE if the I2C controller is busy. The I2C controller is busy if
 * not in the IDLE state 
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \retval      ALT_AVALON_I2C_TRUE
 *              Indicates the I2C controller is busy.
 * \retval      ALT_AVALON_I2C_FALSE
 *              Indicates the I2C controller is not busy.
 *
 */
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_is_busy(ALT_AVALON_I2C_DEV_t *i2c_dev);

/*!
 * This function reads all available bytes from the receive FIFO, up to max_bytes_to_read.
 * If max_bytes_to_read is 0, then all available bytes are read.
 *
 * This function is used to perform low level access to the data bytes
 * received by the I2C controller and buffered in the receive FIFO. It
 * may be used by master-receivers or slave receivers.
 *
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       buffer
 *              [out]The buffer to receive the read data.
 *
 * \param       max_bytes_to_read      
 *              The maximum number of bytes to be read. If 0 then all available bytes are read.
 *
 * \param       bytes_read       
 *              [out] Pointer to variable indicating the number of bytes read.
 */
void alt_avalon_i2c_rx_read_available(ALT_AVALON_I2C_DEV_t *i2c_dev, alt_u8 *buffer, alt_u32 max_bytes_to_read, alt_u32 *bytes_read);

/*!
 * This function reads a single data byte from the receive FIFO.
 *
 * This function is used to perform low level access to the data bytes
 * received by the I2C controller and buffered in the receive FIFO. It
 * may be used by master-receivers or slave receivers.
 *
 * This function checks for valid data in the receive FIFO
 * before reading the byte. 
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       val
 *              [out] The single data byte read from the receive FIFO.
 *
 * \retval      ALT_AVALON_I2C_STATUS_CODE       
 *              ALT_AVALON_I2C_SUCCESS indicates successful status
 *              otherwise one of the ALT_AVALON_I2C_* status codes
 *              is returned.  All failing return values are < 0. 
 *
 */
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_rx_read(ALT_AVALON_I2C_DEV_t *i2c_dev, alt_u8 *val);

/*!
 * This function writes the Transfer Command FIFO.
 *
 * This function is used to perform low level writes of data to the
 * Transfer Command FIFO for transmission by the I2C controller. 
 * The value written includes the Stop and Start bits.
 *
 * This function checks for there to be space in Transfer Command FIFO 
 * before writing the register.  
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       val
 *              The data to write to the Transfer Command FIFO.
 *
 * \retval      ALT_AVALON_I2C_STATUS_CODE       
 *              ALT_AVALON_I2C_SUCCESS indicates successful status
 *              otherwise one of the ALT_AVALON_I2C_* status codes
 *              is returned.  All failing return values are < 0. 
 *
 */
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_cmd_write(ALT_AVALON_I2C_DEV_t *i2c_dev, 
                                                      alt_u8 val,
                                                      alt_u8 issue_restart,
                                                      alt_u8 issue_stop);

                                                      
/*! @} */

/******************************************************************************/
/*! \addtogroup TARGETADDRESSFUNCTIONS Target Address Functions
 *
 * The I2C target address is the address the I2C device being accessed will respond to.
 * This is a 7 or 10 bit value.   alt_avalon_i2c_send_address() will update the 
 * transfer command register with a value containing the target, direction, and restart
 * bits. 
 *
 * The alt_avalon_i2c_master_target_set() and alt_avalon_i2c_master_target_get() functions 
 * will set or get the target address that will be used in future I2C transactions.
 *
 * The target address must be set before doing any I2C functions.  Once set it does not have
 * to be set again unless changing the device being accessed.
 * 
 * @{
 */
 
/*!
 * This function sends the address and write or read command with optional
 * restart bit to the Transfer Command Fifo.  If the command 
 * fifo is full the command will wait until space is available.  The command
 * sequence will be 7 or 10 bit depending on the address mode setting.
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       rw_bit
 *              The I2C Read (1) or Write(0) bit.
 *
 * \param       issue_restart
 *              If issue_restart is 1, a restart is issued.
 *
 * \retval      ALT_AVALON_I2C_STATUS_CODE       
 *              ALT_AVALON_I2C_SUCCESS indicates successful status
 *              otherwise one of the ALT_AVALON_I2C_* status codes
 *              is returned.  All failing return values are < 0. 
 */
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_send_address(ALT_AVALON_I2C_DEV_t *i2c_dev,const alt_u32 rw_bit,const alt_u8 issue_restart);

/*!
 * This function returns the current target address.
 *
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       target_addr
 *              [out] The 7 or 10 bit slave target address.
 *
 */
void alt_avalon_i2c_master_target_get(ALT_AVALON_I2C_DEV_t * i2c_dev, alt_u32 * target_addr);

/*!
 * This function updates the target address for any future I2C bus IO.
 *
 * This function does not wait for a bus idle state before changing the address.
 * So is not suitable for being called from an interrupt routine.
 *
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       target_addr
 *              The 7 or 10 bit slave target address.
 *
 */
void alt_avalon_i2c_master_target_set(ALT_AVALON_I2C_DEV_t * i2c_dev, alt_u32 target_addr);
/*! @} */

/******************************************************************************/
/*! \addtogroup  Avalon I2C Transmit and Receive Functions
 *
 * The functions alt_avalon_i2c_master_transmit() and alt_avalon_i2c_master_receive()
 * are the main functions for performing i2c transactions.  These functions can be
 * combined to create any combination of read-write operations within an I2C
 * START to STOP sequence.   
 * There are also interrupt versions of the above functions,
 * alt_avalon_i2c_master_transmit_using_interrupts() and alt_avalon_i2c_master_receive_using_interrupts().
 * Though the provided interrupt irq callback is functional and tested, it is mainly provided as an example 
 * starting point for a user to develop their own irq callback routine and to show how the 
 * user callback is registered and used.
 * 
 * The above functions will return an ALT_AVALON_I2C_BUSY status if the ip is already enabled,
 * and the function is being called with a START request (ie, restart parameter is not set). 
 * 
 * The functions alt_avalon_i2c_master_tx(), alt_avalon_i2c_master_rx(), and alt_avalon_i2c_master_tx_rx()
 * are complete START to STOP sequence commands to simplify typical write and read operations and are
 * basically wrappers for the alt_avalon_i2c_master_transmit() and alt_avalon_i2c_master_receive() functions.
 * These wrapper functions will do automatic retry if the device is busy.
 * 
 * @{
 */
 
 
/*!
 * This function transmits START followed by the I2C command
 * byte(s).  Then write requests are sent to fulfill the write request.  The final
 * transaction will issue a STOP. 
 *
 * This API is not suitable for being called in an interrupt context as it may
 * wait for certain controller states before completing.
 *
 * The target address must have been set before using this function.
 *
 * If an ALT_AVALON_I2C_ARB_LOST_ERR, ALT_AVALON_I2C_NACK_ERR, or ALT_AVALON_I2C_BUSY occurs the
 * command will be retried.  ALT_AVALON_I2C_NACK_ERR will occur when the slave
 * device is not yet ready to accept more data.
 *
 * If the use_interrupts parameter is 1, then as soon as all bytes have been 
 * written to the command fifo this function will return.  The interrupt handler
 * will then handle waiting for the device to accept the data and will then complete the
 * i2c transaction.  To use this option the provided optional user interrupt handler
 * callback must have been registered by calling the alt_avalon_i2c_register_optional_irq_handler
 * function.
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       buffer
 *              The data buffer to be sent.
 *
 * \param       size
 *              The size of the data buffer to write to the I2C bus.
 *
 * \param       use_interrupts
 *              The optional user interrupt handler callback will be used to handle
 *              sending the data.
 *
 *
 * \retval      ALT_AVALON_I2C_STATUS_CODE       
 *              ALT_AVALON_I2C_SUCCESS indicates successful status
 *              otherwise one of the ALT_AVALON_I2C_* status codes
 *              is returned.  All failing return values are < 0. 
 *
 */
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_master_tx(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                       const alt_u8 * buffer,
                                       const alt_u32 size,
                                       const alt_u8 use_interrupts);        
                                       
                                       
/*!
 * This function transmits START followed by the I2C command
 * byte(s).  Then read requests are sent to fulfill the read request.  The final
 * transaction will issue a STOP. 
 *
 * This API is not suitable for being called in an interrupt context as it may
 * wait for certain controller states before completing.
 *
 * The target address must have been set before using this function.
 *
 * If an ALT_AVALON_I2C_ARB_LOST_ERR, ALT_AVALON_I2C_NACK_ERR, ALT_AVALON_I2C_BUSY occurs the
 * command will be retried.  ALT_AVALON_I2C_NACK_ERR will occur when the slave
 * device is not yet ready to accept more data.
 *
 * If the use_interrupts parameter is 1, then as soon as all cmd bytes have been 
 * written to the command fifo this function will return.  The interrupt handler
 * will then handle waiting for the device to send the rx data and will then complete the
 * i2c transaction.  To use this option the provided optional user interrupt handler
 * callback must have been registered by calling the alt_avalon_i2c_register_optional_irq_handler
 * function.
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       buffer
 *              [out] The data buffer to be receive the i2c data.
 *
 * \param       size
 *              The size of the data buffer to write to the I2C bus.
 *
 * \param       use_interrupts
 *              The optional user interrupt handler callback will be used to handle
 *              receiving the data.
 *
 * \retval      ALT_AVALON_I2C_STATUS_CODE       
 *              ALT_AVALON_I2C_SUCCESS indicates successful status
 *              otherwise one of the ALT_AVALON_I2C_* status codes
 *              is returned.  All failing return values are < 0. 
 *
 */
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_master_rx(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                       alt_u8 * buffer,
                                       const alt_u32 size,
                                       const alt_u8 use_interrupts);    
                                       
                                       
/*!
 * This function transmits START followed by the I2C command
 * byte(s).  Then write requests are sent to fulfill the write request. 
 * Then a RESTART is issued and read requests are sent until the read
 * request is fulfilled. The final transaction will issue a STOP. 
 *
 * This API is not suitable for being called in an interrupt context as it may
 * wait for certain controller states before completing.
 *
 * The target address must have been set before using this function.
 *
 * If an ALT_AVALON_I2C_ARB_LOST_ERR, ALT_AVALON_I2C_NACK_ERR, or ALT_AVALON_I2C_BUSY occurs the
 * command will be retried.  ALT_AVALON_I2C_NACK_ERR will occur when the slave
 * device is not yet ready to accept or send more data.
 *
 * This command will allow easy access of a device requiring an internal register
 * address to be set before doing a read, for example an eeprom device.
 *
 * Example: If an eeprom requires a 2 byte address to be sent before doing a
 * memory read, the tx buffer would contain the 2 byte address and the txsize
 * would be set to 2.  Then the rxbuffer will recieve the rxsize number of
 * bytes to read from the eeprom as follows:
 *     To Read 0x10 bytes from eeprom at i2c address 0x51 into buffer:
 *         buffer[0]=2;buffer[1]=0; //set eeprom address 0x200
 *         alt_avalon_i2c_master_tx_rx(i2c_ptr,buffer,2,buffer,0x10,0);
 *
 * Notice the tx and rx buffer can be the same buffer if desired.  
 *
 * If the use_interrupts parameter is 1, then as soon as all cmd bytes have been 
 * written to the command fifo this function will return.  The interrupt handler
 * will then handle waiting for the device to send the rx data and will then complete the
 * i2c transaction.  To use this option the provided optional user interrupt handler
 * callback must have been registered by calling the alt_avalon_i2c_register_optional_irq_handler
 * function.
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       txdata
 *              The send data buffer.
 *
 * \param       txsize
 *              The size of the send data buffer to write to the I2C bus.
 *
 * \param       rxdata
 *              [out] The receive data buffer.
 *
 * \param       rxsize
 *              The size of the receive data buffer.
 *
 * \param       use_interrupts
 *              The optional user interrupt handler callback will be used to handle
 *              sending and receiving the data.
 *
 * \retval      ALT_AVALON_I2C_STATUS_CODE       
 *              ALT_AVALON_I2C_SUCCESS indicates successful status
 *              otherwise one of the ALT_AVALON_I2C_* status codes
 *              is returned.  All failing return values are < 0. 
 *
 */                                       
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_master_tx_rx(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                       const alt_u8 * txbuffer,
                                       const alt_u32 txsize,
                                       alt_u8 * rxbuffer,
                                       const alt_u32 rxsize,
                                       const alt_u8 use_interrupts);                                                           

/*!
 * This function transmits either a START or RESTART followed by the I2C command
 * byte(s).  Then write requests are sent to fulfill the write request.  The final
 * transaction may issue a STOP depending on the issue_stop parameter. 
 *
 * This API is not suitable for being called in an interrupt context as it may
 * wait for certain controller states before completing.
 *
 * The target address must have been set before using this function.
 *
 * If the command returns ALT_AVALON_I2C_ARB_LOST_ERR, ALT_AVALON_I2C_NACK_ERR, or
 * ALT_AVALON_I2C_BUSY,
 * the caller is responsible to do a retry (if desired) by calling the function again.
 * To automatically do a complete write sequence with retry use the 
 * alt_avalon_i2c_master_tx() function.
 *
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       data
 *              The data buffer to be sent.
 *
 * \param       size
 *              The size of the data buffer to write to the I2C bus.
 *
 * \param       issue_restart
 *              This parameter controls whether a RESTART or START is issued before
 *              the command byte is sent. If:
 *              * \b true \e  a RESTART is issued before the command byte is sent.
 *              * \b false \e  a START command is issued before the command byte(s) is sent.
 *              
 * \param       issue_stop
 *              This parameter controls whether a STOP is issued after the cmd data is
 *              sent. If:
 *              * \b true - STOP is issued after the final cmd byte is transmitted.
 *              * \b false - STOP is not issued.
 *
 * \retval      ALT_AVALON_I2C_STATUS_CODE       
 *              ALT_AVALON_I2C_SUCCESS indicates successful status
 *              otherwise one of the ALT_AVALON_I2C_* status codes
 *              is returned.  All failing return values are < 0. 
 *
 */
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_master_transmit(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                        const alt_u8 * data,
                                        alt_u32 size,
                                        const alt_u8 issue_restart,
                                        const alt_u8 issue_stop);

                                        
/*!
 * This function transmits either a START or RESTART followed by the I2C command
 * byte(s).  Then write requests are sent to fulfill the write request.  The final
 * transaction may issue a STOP depending on the issue_stop parameter. 
 *
 * This API is not suitable for being called in an interrupt context as it may
 * wait for certain controller states before completing.
 *
 * The target address must have been set before using this function.
 *
 * If the command returns ALT_AVALON_I2C_ARB_LOST_ERR or ALT_AVALON_I2C_NACK_ERR or ALT_AVALON_I2C_BUSY,
 * the caller is responsible to do a retry (if desired) by calling the function again.
 * To automatically do a complete write sequence with retry use the 
 * alt_avalon_i2c_master_tx() function with the retry option set.
 *
 * As soon as all cmd bytes have been 
 * written to the command fifo this function will return.  The interrupt handler
 * will then handle waiting for the device to accept the data completing the
 * i2c transaction.  To use this option the provided optional user interrupt handler
 * callback must have been registered by calling the alt_avalon_i2c_register_optional_irq_handler
 * function.
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       data
 *              The data buffer to be sent.
 *
 * \param       size
 *              The size of the data buffer to write to the I2C bus.
 *
 * \param       issue_restart
 *              This parameter controls whether a RESTART or START is issued before
 *              the command byte is sent. If:
 *              * \b true \e  a RESTART is issued before the command byte is sent.
 *              * \b false \e  a START command is issued before the command byte(s) is sent.
 *              
 * \param       issue_stop
 *              This parameter controls whether a STOP is issued after the cmd data is
 *              sent. If:
 *              * \b true - STOP is issued after the final cmd byte is transmitted.
 *              * \b false - STOP is not issued.
 *
 * \retval      ALT_AVALON_I2C_STATUS_CODE       
 *              ALT_AVALON_I2C_SUCCESS indicates successful status
 *              otherwise one of the ALT_AVALON_I2C_* status codes
 *              is returned.  All failing return values are < 0. 
 *
 */
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_master_transmit_using_interrupts(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                        const alt_u8 * buffer,
                                        alt_u32 size,
                                        const alt_u8 issue_restart,
                                        const alt_u8 issue_stop);                                        
                                        
/*!
 * This function transmits either a START or RESTART followed by the I2C command
 * byte(s).  Then read requests are sent to fulfill the read request.  The final
 * transaction may issue a STOP depending on the issue_stop parameter. 
 *
 * This API is not suitable for being called in an interrupt context as it may
 * wait for certain controller states before completing.
 *
 * The target address must have been set before using this function.
 *
 * If the command returns ALT_AVALON_I2C_ARB_LOST_ERR, ALT_AVALON_I2C_NACK_ERR,
 * or ALT_AVALON_I2C_BUSY
 * the caller is responsible to do a retry (if desired) by repeating the i2c
 * transaction from the call that issued the START, which may not be this one.
 * To automatically do a complete write-read sequence with retry use the 
 * alt_avalon_i2c_master_tx_rx() function with the retry option set.
 *
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       data
 *              [out] The data buffer to receive the requested \e size bytes.
 *
 * \param       size
 *              The size of the data buffer to read from the RX FIFO.
 *
 * \param       issue_restart
 *              This parameter controls whether a RESTART or START is issued before
 *              the command byte is sent. If:
 *              * \b true \e  a RESTART is issued before the command byte is sent.
 *              * \b false \e  a START command is issued before the command byte(s) is sent.
 *              
 * \param       issue_stop
 *              This parameter controls whether a STOP is issued after the cmd data is
 *              sent. If:
 *              * \b true - STOP is issued after the final cmd byte is transmitted.
 *              * \b false - STOP is not issued.
 *
 * \retval      ALT_AVALON_I2C_STATUS_CODE       
 *              ALT_AVALON_I2C_SUCCESS indicates successful status
 *              otherwise one of the ALT_AVALON_I2C_* status codes
 *              is returned.  All failing return values are < 0. 
 *
 */
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_master_receive(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                       alt_u8 * data,
                                       const alt_u32 size,
                                       const alt_u8 issue_restart,
                                       const alt_u8 issue_stop);
                                       
                                       
/*!
 * This function transmits either a START or RESTART followed by the I2C command
 * byte(s).  Then read requests are sent to fulfill the read request.  The final
 * transaction may issue a STOP depending on the issue_stop parameter. 
 *
 * This API is not suitable for being called in an interrupt context as it may
 * wait for certain controller states before completing.
 *
 * The target address must have been set before using this function.
 *
 * If the command returns ALT_AVALON_I2C_ARB_LOST_ERR or ALT_AVALON_I2C_NACK_ERR or ALT_AVALON_I2C_BUSY
 * the caller is responsible to do a retry (if desired) by repeating the i2c
 * transaction from the call that issued the START, which may not be this one.
 * To automatically do a complete write-read sequence with retry use the 
 * alt_avalon_i2c_master_tx_rx() function with the retry option set.
 *
 * As soon as all cmd bytes have been 
 * written to the command fifo this function will return.  The interrupt handler
 * will then handle waiting for the device to send the rx data and will then complete the
 * i2c transaction.  To use this option the provided optional user interrupt handler
 * callback must have been registered by calling the alt_avalon_i2c_register_optional_irq_handler
 * function.
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       data
 *              [out] The data buffer to receive the requested \e size bytes.
 *
 * \param       size
 *              The size of the data buffer to read from the RX FIFO.
 *
 * \param       issue_restart
 *              This parameter controls whether a RESTART or START is issued before
 *              the command byte is sent. If:
 *              * \b true \e  a RESTART is issued before the command byte is sent.
 *              * \b false \e  a START command is issued before the command byte(s) is sent.
 *              
 * \param       issue_stop
 *              This parameter controls whether a STOP is issued after the cmd data is
 *              sent. If:
 *              * \b true - STOP is issued after the final cmd byte is transmitted.
 *              * \b false - STOP is not issued.
 *
 * \retval      ALT_AVALON_I2C_STATUS_CODE       
 *              ALT_AVALON_I2C_SUCCESS indicates successful status
 *              otherwise one of the ALT_AVALON_I2C_* status codes
 *              is returned.  All failing return values are < 0. 
 *
 */
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_master_receive_using_interrupts(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                       alt_u8 * buffer,
                                       const alt_u32 size,
                                       const alt_u8 issue_restart,
                                       const alt_u8 issue_stop);                                       

/*! @} */
                                       
/******************************************************************************/
/*! \addtogroup ALT_AVALON_I2C_INT Interrupt and Status Conditions
 *
 * The functions in this group provide management for the I2C controller status
 * conditions and interrupts.
 *
 * Each I2C controller has a single combined interrupt output. 
 * The following events can generate an interrupt:
 * * Arbitation lost
 * * NACK Detected
 * * Transmit Buffer Empty
 * * Receive Buffer Full
 * * Receive Overflow
 *
 * These interrupt status conditions may be monitored either by polling their
 * status or by configuring interrupt handlers.
 *
 * Functions to get the current status, enable or disable (i.e. mass or unmask),
 * and clear interrupt status conditions for the I2C controller are defined in
 * this section.
 *
 * @{
 */
 
/*!
 * This type enumerates interrupt status conditions for the I2C controller.
 */
typedef enum ALT_AVALON_I2C_ISR_e
{
    ALT_AVALON_I2C_STATUS_RX_OVER      = 1UL << 4,
                                /*!< Set if the receive buffer is completely
                                *   filled to capacity and an additional byte is
                                *   received from an external I2C device. The I2C
                                *   controller acknowledges this, but any data
                                *   bytes received after the FIFO is full are
                                *   discarded.  Writing 1 to this field clears the
                                *   contents to 0.
                                */
    ALT_AVALON_I2C_STATUS_ARBLOST_DET  = 1UL << 3,
                                /*!< This bit is set to 1 when the i2c controller 
                                 *   has lost the bus arbitration.  Writing 1 to
                                 *   this field clears the contents to 0.
                                 */
    ALT_AVALON_I2C_NACK_DET     = 1UL << 2,
                                /*!< This bit is set to 1 when no acknowledgement 
                                 *   (NACK) is recieved by the i2c controller.  The
                                 *   MACK can be during the address or data transmission
                                 *   phase. Writing 1 to
                                 *   this field clears the contents to 0.
                                 */
    ALT_AVALON_I2C_RX_READY    = 1UL << 1,
                                /*!< This bit indicates the RX_DATA fifo level is 
                                 *   equal or more than the RX_DATA fifo threshold.
                                 *   It is automatically cleared when the RX_DATA fifo
                                 *   level is less than the RX_DATA fifo threshold.
                                 */            
    ALT_AVALON_I2C_TX_READY    = 1UL << 0,
                                /*!< This bit indicates the Transfer Command FIFO is ready 
                                 *   for data transmission.   This bit is asserted when the
                                 *   Transfer Command FIFO level is equal to or less than the
                                 *   Transfer Command FIFO threshold.
                                 */                                 
} ALT_AVALON_I2C_STATUS_t;

/* A structure to be used with the optional provided irq callback function*/
typedef struct IRQ_DATA_s
{
   alt_u8 * buffer;
   alt_u32 size;
   alt_u32 irq_busy;
} IRQ_DATA_t;
 
 /*!
 * Associate a user-specific routine with the i2c interrupt handler.
 * If a callback is registered, all enabled ISR's will cause the callback to be executed.
 * The callback runs as part of the interrupt service routine. 
 *
 * An optional user callback routine is provided in this code and, if used, enables use
 * of the alt_avalon_i2c_master_receive_using_interrupts and alt_avalon_i2c_master_transmit_using_interrupts
 * functions.  To register the optionally provided user callback use the alt_avalon_i2c_register_optional_irq_handler
 * function.
 *
 * \param        dev        Pointer to i2c device (instance) structure.
 * \param        callback   Pointer to callback routine to execute at interrupt level
 * \param        control    For masking the source interruption and setting configuration
 * \param        context    callback context
 *
 */  
void alt_avalon_i2c_register_callback(
    ALT_AVALON_I2C_DEV_t *i2c_dev,
    alt_avalon_i2c_callback callback,
    alt_u32 control,
    void *context); 
    
/*!
 * Associate the optional provided user interrupt callback routine with the i2c handler.
 * This is a simple IRQ callback which allows I2C transaction functions to immedietly return
 * while the optional callback handles receiving or transmitting the data to the device
 * and completing the transaction.  This optional callback uses a IRQ_DATA_t structure for
 * irq data.  The function alt_avalon_i2c_interrupt_transaction_status can be used to check 
 * for irq transaction complete, or for an transaction error.  
 * These optionally provided interrupt routines are functional, but are provided
 * mainly for the purpose as working examples of using interrupts with the avalon i2c ip.
 * A user may want to develop a more detailed irq callback routine tailored for specific 
 * device hardware.  In that case, the user callback would be registered with the 
 * alt_avalon_i2c_register_callback function.
 * 
 * Using this optionally provided user callback routine enables use
 * of the alt_avalon_i2c_master_receive_using_interrupts and alt_avalon_i2c_master_transmit_using_interrupts
 * functions. 
 *
 * \param        dev        Pointer to i2c device (instance) structure.
 *
 * \param        irq_data   A structure used for interrupt handler data.
 *                          Nothing needs to be set in this structure by the
 *                          user.  The storage merely has to be provided.
 *                          The irq_busy variable can be used to check if the
 *                          transaction is complete.  In that case it will be 0.
 */  
void alt_avalon_i2c_register_optional_irq_handler(ALT_AVALON_I2C_DEV_t *i2c_dev,IRQ_DATA_t * irq_data); 

/*!
 * When an interrupt transaction has been initiated using the alt_avalon_i2c_master_tx, 
 * alt_avalon_i2c_master_rx, or alt_avalon_i2c_master_tx_rx function with the interrupt
 * option set,  or if using the alt_avalon_i2c_master_transmit_using_interrupts or
 * alt_avalon_i2c_master_receive_using_interrupts functions, then this function can be
 * used to check the status of that transaction.   The only way to ensure error free
 * back to back transactions is to use this function after every interrupt transaction
 * to ensure the transaction had no errors and is complete, before starting the next transaction.
 * Also, if an error is returned from this function, then the user must retry the i2c transaction.
 * One reason an error may be returned is if the device is busy, which is likely to occur
 * occasionally if doing back to back transactions.
 *
 * \param        dev        Pointer to i2c device (instance) structure.
 *
 * \retval      ALT_AVALON_I2C_STATUS_CODE       
 *              ALT_AVALON_I2C_SUCCESS indicates interrupt transaction is
 *              successfully completed.  Another transaction can now be
 *              started.
 *
 *              ALT_AVALON_I2C_BUSY indicates the interrupt transaction is still
 *              busy.
 *
 *              ALT_AVALON_NACK_ERROR indicates the device did not ack.  This is
 *              most likely because the device is busy with the previous transaction.
 *              The transaction must be retried.
 *
 *              otherwise one of the other ALT_AVALON_I2C_* status codes
 *              is returned.  The transaction must be retried.
 *
 *              All failing return values are < 0. 
 *
 */  
ALT_AVALON_I2C_STATUS_CODE alt_avalon_i2c_interrupt_transaction_status(ALT_AVALON_I2C_DEV_t *i2c_dev);
 
 /*!
 * This reads the raw NACK status bit regardless of whether or not the corresponding
 * interrupt is enabled.  The function will update status if the NACK bit is set in the ISR reg/
 * Otherwise nothing is done.
 *
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       status
 *              [in][out] Set to ALT_AVALON_I2C_NACK_ERR if the NACK bit
 *              in the ISR is set. Otherwise status is not modified.
 *
 */
void alt_avalon_i2c_check_nack(ALT_AVALON_I2C_DEV_t *i2c_dev,ALT_AVALON_I2C_STATUS_CODE * status);

/*!
 * This reads the raw ARBLOST status bit regardless of whether or not the corresponding
 * interrupt is enabled. This function will update status if the ARBLOST bit is set in the ISR reg
 * Otherwise nothing is done.
 *
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       status
 *              [in][out] Set to ALT_AVALON_I2C_ARB_LOST_ERR if the ARBLOST bit
 *              in the ISR is set. Otherwise, status is not modified.
 *
 */
void alt_avalon_i2c_check_arblost(ALT_AVALON_I2C_DEV_t *i2c_dev,ALT_AVALON_I2C_STATUS_CODE * status);

/*!
 * Returns the current I2C controller interrupt status conditions.
 *
 * This function returns the current value of the I2C controller interrupt status
 * register value which reflects the current I2C controller status conditions that
 * are not disabled (i.e. masked).
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       status
 *              [out] A pointer to a bit mask of the active \ref ALT_AVALON_I2C_STATUS_t
 *              interrupt and status conditions.
 *
 */
void alt_avalon_i2c_int_status_get(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                       alt_u32 *status);

/*!
 * Returns the I2C controller raw interrupt status conditions irrespective of
 * the interrupt status condition enablement state.
 *
 * This function returns the current value of the I2C controller raw interrupt
 * status register value which reflects the current I2C controller status
 * conditions regardless of whether they are disabled (i.e. masked) or not.
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       status
 *              [out] A pointer to a bit mask of the active \ref ALT_AVALON_I2C_STATUS_t
 *              interrupt and status conditions.
 *
 *
 */
void alt_avalon_i2c_int_raw_status_get(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                           alt_u32 *status);

/*!
 * Clears the specified I2C controller interrupt status conditions identified
 * in the mask.
 *
 * This function clears one or more of the status conditions as contributors to
 * the IRQ interrupt signal state.
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       mask
 *              Specifies the interrupt status conditions to clear. \e mask
 *              is a mask of logically OR'ed \ref ALT_AVALON_I2C_STATUS_t values that
 *              designate the status conditions to clear.
 *
 */
void alt_avalon_i2c_int_clear(ALT_AVALON_I2C_DEV_t *i2c_dev, const alt_u32 mask);

/*!
 * Disable the specified I2C controller interrupt status conditions identified in
 * the mask.
 *
 * This function disables one or more of the status conditions as contributors to
 * the \b ALT_INT_INTERRUPT_I2C<em>n</em>_IRQ interrupt signal state.
 *
 * NOTE: A cleared bit for any status condition in the mask value does not have
 *       the effect of enabling it as a contributor to the IRQ interrupt signal state. 
 *       The function alt_avalon_i2c_int_enable() is used to enable status source conditions.
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       mask
 *              Specifies the status conditions to disable as interrupt source
 *              contributors. \e mask is a mask of logically OR'ed \ref
 *              ALT_AVALON_I2C_STATUS_t values that designate the status conditions to
 *              disable.
 *
 *
 */
void alt_avalon_i2c_int_disable(ALT_AVALON_I2C_DEV_t *i2c_dev, const alt_u32 mask);

/*!
 * Enable the specified I2C controller interrupt status conditions identified in
 * the mask.
 *
 * This function enables one or more of the status conditions as contributors to
 * the IRQ interrupt signal state.
 *
 * NOTE: A cleared bit for any status condition in the mask value does not have
 *       the effect of disabling it as a contributor to the IRQ interrupt signal state.
 *       The function alt_avalon_i2c_int_disable() is used to disable status source conditions.
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       mask
 *              Specifies the status conditions to enable as interrupt source
 *              contributors. \e mask is a mask of logically OR'ed \ref
 *              ALT_AVALON_I2C_STATUS_t values that designate the status conditions to
 *              enable.
 *
 *
 */
void alt_avalon_i2c_int_enable(ALT_AVALON_I2C_DEV_t *i2c_dev, const alt_u32 mask);

/*!
 * Gets the Enabled Interrupts  (reads the ISER register)
 *
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       enabled_ints
 *             [out] A pointer to the variable to recieve the data.
 *
 *
 */
void alt_avalon_i2c_enabled_ints_get(ALT_AVALON_I2C_DEV_t *i2c_dev, alt_u32 * enabled_ints);

/*! @} */

/******************************************************************************/
/*! \addtogroup ALT_AVALON_I2C_RX_FIFO RX FIFO Management
 *
 * The receive FIFO has a configurable threshold value that controls the level of
 * entries (or above) that sets the RX_READY status condition and triggers an
 * interrupt. The valid ranges are 1, 1/4 full, 1/2 full, or full.
 *
 * @{
 */
 
 /*!
 * This type enumerates the RX_DATA_FIFO_THRESHOLD for the I2C controller.
 */
typedef enum ALT_AVALON_I2C_RX_DATA_FIFO_THRESHOLD_e
{
    ALT_AVALON_I2C_RX_DATA_FIFO_1_ENTRY    = 0,
    ALT_AVALON_I2C_RX_DATA_FIFO_1_4_FULL   = 1,
    ALT_AVALON_I2C_RX_DATA_FIFO_1_2_FULL   = 2,
    ALT_AVALON_I2C_RX_DATA_FIFO_FULL       = 3,
} ALT_AVALON_I2C_RX_DATA_FIFO_THRESHOLD_t;

/*!
 * Gets the current receive FIFO threshold level value.
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       threshold
 *              [out] The current threshold value.
 *
 *
 */
void alt_avalon_i2c_rx_fifo_threshold_get(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                              ALT_AVALON_I2C_RX_DATA_FIFO_THRESHOLD_t *threshold);

/*!
 * Sets the current receive FIFO threshold level value.
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       threshold
 *              The threshold value.
 *
 *
 */
void alt_avalon_i2c_rx_fifo_threshold_set(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                              const ALT_AVALON_I2C_RX_DATA_FIFO_THRESHOLD_t threshold);

/*! @} */

/******************************************************************************/
/*! \addtogroup ALT_AVALON_I2C_TFR_CMD Transfer Command FIFO Management
 *
 * The Transfer Command FIFO has a configurable threshold value that controls the level of
 * entries (or below) that sets the TX_READY status condition and triggers an
 * interrupt. The valid range is empty, 1/4 full, 1/2 full, and not full (at least 1 empty entry)
 * 
 * @{
 */

 /*!
 * This type enumerates the TFR_CMD_FIFO_THRESHOLD for the I2C controller.
 */
typedef enum ALT_AVALON_I2C_TFR_CMD_FIFO_THRESHOLD_e
{
    ALT_AVALON_I2C_TFR_CMD_FIFO_EMPTY      = 0,
    ALT_AVALON_I2C_TFR_CMD_FIFO_1_4_FULL   = 1,
    ALT_AVALON_I2C_TFR_CMD_FIFO_1_2_FULL   = 2,
    ALT_AVALON_I2C_TFR_CMD_FIFO_NOT_FULL   = 3,
} ALT_AVALON_I2C_TFR_CMD_FIFO_THRESHOLD_t;

/*!
 * Gets the current Transfer Command FIFO threshold level value.
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       threshold
 *              [out] The current threshold value.
 *
 *
 */
void alt_avalon_i2c_tfr_cmd_fifo_threshold_get(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                              ALT_AVALON_I2C_TFR_CMD_FIFO_THRESHOLD_t *threshold);

  
/*!
 * Sets the current Transfer Command FIFO threshold level value.
 *
 * \param       i2c_dev
 *              A pointer to the I2C controller device block instance.
 *
 * \param       threshold
 *              The threshold value.
 *
 *
 */
void alt_avalon_i2c_tfr_cmd_fifo_threshold_set(ALT_AVALON_I2C_DEV_t *i2c_dev,
                                              const ALT_AVALON_I2C_TFR_CMD_FIFO_THRESHOLD_t threshold);
                                              

/*! @} */

/*! @} */
                                    

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ALTERA_AVALON_I2C_H__ */
