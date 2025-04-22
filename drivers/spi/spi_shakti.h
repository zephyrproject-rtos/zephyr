/**
 * Project                           : Secure IoT SoC
 * Name of the file                  : spi_shakti.h
 * Brief Description of file         : Header to zephyr spi driver
 * Name of Author                    : Kishore. J
 * Email ID                          : kishore@mindgrovetech.in
 * 
 * @file spi_shakti.h
 * @author Kishore. J (kishore@mindgrovetech.in)
 * @brief This is a zephyr SSPI Driver's Header file for Mindgrove Silicon's SPI Peripheral
 * @version 0.1
 * @date 2024-04-17
 * 
 * @copyright Copyright (c) Mindgrove Technologies Pvt. Ltd 2023. All rights reserved.
 * 
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _SPI_SHAKTI__H
#define __SPI_SHAKTI__H

#include "spi_context.h"
#include <stdint.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/device.h>
#include <stdlib.h>
#include <zephyr/drivers/spi.h>
// #include <./zephyr/drivers/pinctrl.h>

#define SPI_CFG(dev) ((struct spi_shakti_cfg *) ((dev)->config))
#define SPI_DATA(dev) ((struct spi_shakti_data *) ((dev)->data))
#define SPI_REG(dev, offset) ((mem_addr_t) (SPI_CFG(dev)->base + (offset)))

/*clock frequency*/
#define CLOCK_FREQUENCY 40000000

/*!Serial Peripheral Interface Offsets */
#define SPI_START_0 0x00020000 /* Serial Peripheral Interface 0 */
#define SPI_START_1 0x00020100 /* Serial Peripheral Interface 1 */
#define SPI_START_2 0x00020200 /* Serial Peripheral Interface 2 */
#define SPI_START_3 0x00020300 /* Serial Peripheral Interface 3 */

/* Struct to access SSPI registers as 32 bit registers */

#define SSPI_MAX_COUNT  4 /*! Number of Standard SSPI used in the SOC */

#define SSPI0_BASE_ADDRESS  0x00020000 /*! Standard Serial Peripheral Interface 0 Base address*/
#define SSPI0_END_ADDRESS  0x000200FF /*! Standard Serial Peripheral Interface 0 Base address*/

#define SSPI1_BASE_ADDRESS  0x00020100 /*! Standard Serial Peripheral Interface 1 Base address*/
#define SSPI1_END_ADDRESS  0x000201FF /*! Standard Serial Peripheral Interface 1 Base address*/

#define SSPI2_BASE_ADDRESS  0x00020200 /*! Standard Serial Peripheral Interface 2 Base address*/
#define SSPI2_END_ADDRESS  0x000202FF /*! Standard Serial Peripheral Interface 2 Base address*/

#define SSPI3_BASE_ADDRESS  0x00020300 /*! Standard Serial Peripheral Interface 3 Base address*/
#define SSPI3_END_ADDRESS  0x000203FF /*! Standard Serial Peripheral Interface 3 Base address*/

#define SSPI_BASE_OFFSET 0x100

#define COMMCTRL    0x00
#define CLKCTRL     0x04
#define TXREG       0x08
#define RXREG       0x0C
#define INTR_EN     0x10
#define FIFOSTS     0x14
#define COMMSTS     0x18
#define INQUAL      0x1C

//SSPIx Clock Control Register
#define SPI_CLK_POLARITY(x)   (x<<0)
#define SPI_CLK_PHASE(x)      (x<<1)
#define SPI_PRESCALE(x)       (x<<2)
#define SPI_SS2TX_DELAY(x)    (x<<10)
#define SPI_TX2SS_DELAY(x)    (x<<18)

//SSPIx Communication Control Register
#define SPI_MASTER(x)          (x<<0)  
#define SPI_ENABLE(x)          (x<<1)
#define SPI_LSB_FIRST(x)       (x<<2)
#define SPI_COMM_MODE(x)       (x<<4)
#define SPI_TOTAL_BITS_TX(x)   (x<<6)
#define SPI_TOTAL_BITS_RX(x)   (x<<14)
#define SPI_OUT_EN_SCLK        (1<<22)
#define SPI_OUT_EN_NCS         (1<<23)
#define SPI_OUT_EN_MISO        (1<<24)
#define SPI_OUT_EN_MOSI        (1<<25)

//SSPIx Communication Status Register
#define SPI_BUSY             (1<<0)
#define SPI_TX_EN            (1<<1)
#define SPI_RX_NOT_EN        (1<<2)
#define SPI_TX_FIFO(x)       (x<<3)
#define SPI_RX_FIFO(x)       (x<<6)
#define SPI_OVR              (1<<9)

//SSPIx FIFO Status Register
#define SPI_TX_EMPTY          (1<<0)
#define SPI_TX_DUAL           (1<<1)
#define SPI_TX_QUAD           (1<<2)
#define SPI_TX_OCTAL          (1<<3)
#define SPI_TX_HALF           (1<<4)       
#define SPI_TX_24             (1<<5)
#define SPI_TX_28             (1<<6)
#define SPI_TX_30             (1<<7)
#define SPI_TX_FULL           (1<<8)

#define SPI_RX_EMPTY          (1<<9)
#define SPI_RX_DUAL           (1<<10)
#define SPI_RX_QUAD           (1<<11)
#define SPI_RX_OCTAL          (1<<12)
#define SPI_RX_HALF           (1<<13)
#define SPI_RX_24             (1<<14)
#define SPI_RX_28             (1<<15)
#define SPI_RX_30             (1<<16)
#define SPI_RX_FULL           (1<<17)

//SSPIx Interrupt Enable Register
#define SPI_TX_EMPTY_INTR_EN    (1<<0)
#define SPI_TX_DUAL_INTR_EN     (1<<1)
#define SPI_TX_QUAD_INTR_EN     (1<<2)
#define SPI_TX_OCTAL_INTR_EN    (1<<3)
#define SPI_TX_HALF_INTR_EN     (1<<4)
#define SPI_TX_24_INTR_EN       (1<<5)
#define SPI_TX_28_INTR_EN       (1<<6)
#define SPI_TX_30_INTR_EN       (1<<7)
#define SPI_TX_FULL_INTR_EN     (1<<8)

#define SPI_RX_EMPTY_INTR_EN    (1<<9)
#define SPI_RX_DUAL_INTR_EN     (1<<10)
#define SPI_RX_QUAD_INTR_EN     (1<<11)
#define SPI_RX_OCTAL_INTR_EN    (1<<12)
#define SPI_RX_HALF_INTR_EN     (1<<13)
#define SPI_RX_24_INTR_EN       (1<<14)
#define SPI_RX_28_INTR_EN       (1<<15)
#define SPI_RX_30_INTR_EN       (1<<16)
#define SPI_RX_FULL_INTR_EN     (1<<17)
#define SPI_RX_OVERRUN_INTR_EN  (1<<18)

#define FIFO_DEPTH_8  32
#define FIFO_DEPTH_16 FIFO_DEPTH_8/2
#define FIFO_DEPTH_32 FIFO_DEPTH_8/4

#define MASTER 1
#define SLAVE 0

#define DISABLE 0
#define ENABLE 1

#define LSB_FIRST 1
#define MSB_FIRST 0

#define SIMPLEX_TX 0
#define SIMPLEX_RX 1
#define HALF_DUPLEX 2
#define FULL_DUPLEX 3

#define SUCCESS 0
#define FAILURE -1
#define TIMEOUT -2

#define SPI0 0
#define SPI1 1
#define SPI2 2
#define SPI3 3

#define DATA_SIZE_8 8
#define DATA_SIZE_16 16
#define DATA_SIZE_32 32

#define POL_AND_PHA     0b0000000000000110      // polarity and phase is 1
#define INV_POLANDPHA   0b0000000000000000      // polarity and phase is 0      
#define HALFDUPLEX      0b0000100000000000      // if mode bit is 1 then spi is halfduplex      
#define FULLDUPLEX      0b0000000000000000      // if mode bit is 0 then spi is fullduplex

/**
 * The below code defines a union type named "Data" that can hold a 32-bit integer, a 16-bit integer,
 * or an 8-bit integer.
 */

typedef union{
    uint32_t data_32;
    uint16_t data_16;
    uint8_t data_8;
} Data;

/**
 * @struct sspi_struct
 * @brief This defines a structure for controlling and communicating with an SSPI device.
 * @details This structure contains all the registers used in the Serial Peripheral Interface(SPI).
 * The registers used are: communication_control, clock_control register, TX and RX Data registers,
 * Interrupt Enable register, Communication and FIFO status registers, and an Input Qualification register.
 */

typedef struct{
/**
 * @var uint32_t comm_control
 * This is a 32-bit register that controls the communication
 * settings of the SSPI (Synchronous Serial Peripheral Interface) module. It may include settings such
 * as clock polarity, clock phase, data order, and master/slave mode.
 */
    uint32_t    comm_control;
/**
 * @var uint32_t clk_control
 * The clk_control property is a 32-bit register that controls the
 * clock signal used by the SSPI (Synchronous Serial Peripheral Interface) module. It can be used to
 * set the clock frequency, phase, and polarity.
 */
    uint32_t    clk_control;
/** 
* @var Data data_tx
* The above code is declaring a structure named "Data" and a variable named "data_tx" of that
* structure type. The structure has a member named "data_tx" which is of type "unsigned int" and is
* used to hold data for transmission in a Serial Peripheral Interface (SPI) communication protocol.
* The comment indicates that the data size can be 8, 16, or 32 bits. 
*/
    Data        data_tx; 
/**
* @var Data     data_rx;
* The above code is declaring a structure named "Data" and a variable named "data_rx" of that
* structure type. The structure has a member named "data_rx" which is of type "unsigned int" and is
* used to receive the data from a Serial Peripheral Interface (SPI) communication protocol. This register is used
* to read the data received from the SSPI device during data transfer operations.
* The comment indicates that the data size can be 8, 16, or 32 bits. " 
*/
    Data        data_rx;
/**
 * @var uint32_t intr_en
 * The "intr_en" property is a 32-bit register that controls the
 * interrupt enable status for various events in the SSPI (Synchronous Serial Peripheral Interface)
 * module. These events include transmit buffer empty, receive buffer full, and various error
 * conditions. By setting the appropriate bits in this register, the interrupt will be enbled or disabled.
 */
    uint32_t    intr_en;
/**
 * @var uint32_t fifo_status
 * The fifo_status property is an 32-bit register that indicates the
 * status of the FIFO (First-In-First-Out) buffer in the SSPI (Synchronous Serial Peripheral Interface)
 * module. It can be used to determine if the FIFO is full, empty, or partially full.
 */
    uint32_t    fifo_status;
/**
 * @var uint16_t comm_status
 * The SSPI Communication Status Register is an 16-bit register that
 * provides information about the current status of the SSPI communication. It may contain information
 * such as whether the communication is currently active, whether there are any errors in the
 * communication, or whether the communication has been completed successfully.
 */
    uint16_t    comm_status;
/**
 * @var uint16_t reserve0
 * This is a 16-bit field that is reserved for future use and currently
 * has no defined purpose or functionality. It is included in the structure for potential future
 * expansion or compatibility with other systems.
 */
    uint16_t    reserve0;
/**
 * @var uint8_t qual
 * The "qual" property is the SSPI Input Qualification Control Register,
 * which is an 8-bit register used to set the input qualification level for the SSPI receiver. This
 * register determines the minimum pulse width required for the SSPI receiver to recognize a valid
 * input signal. 
 */
    uint8_t     qual;
/**
 * @var uint8_t reserve5
 * The reserve5 is an 8-bit reserved field in the sspi_struct. It is not
 * used for any specific purpose and is left unused for future modifications or updates to the
 * structure.
 */
    uint8_t     reserve1;
/**
 * @var uint16_t reserve6
 * This is a 16-bit field that is reserved for future use and currently
 * has no defined purpose or functionality. It is included in the structure for potential future
 * expansion or compatibility with other systems.
 */
    uint16_t    reserve2;
}sspi_struct;

struct spi_shakti_data {
	struct spi_context ctx;
};

struct spi_shakti_cfg {
    struct gpio_dt_spec ncs;
	uint32_t base;
	uint32_t f_sys;
    const struct pinctrl_dev_config *pcfg;
    struct k_mutex mutex;
};

#endif

#ifdef __cplusplus
}
#endif