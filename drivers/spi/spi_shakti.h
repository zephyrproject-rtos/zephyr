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

// typedef union{
//     uint8_t tx_data8;
//     uint16_t tx_data16;
//     uint32_t tx_data32;
// }size;

typedef union{
    uint32_t data_32;
    uint16_t data_16;
    uint8_t data_8;
} Data;


typedef struct{
    uint32_t    comm_control;
    uint32_t    clk_control;
    Data        data_tx; 
    Data        data_rx;
    uint32_t    intr_en;
    uint32_t    fifo_status;
    uint16_t    comm_status;
    uint16_t    reserve0;
    uint8_t     qual;
    uint8_t     reserve1;
    uint16_t    reserve2;
}sspi_struct;


struct spi_shakti_data {
	struct spi_context ctx;
};

struct spi_shakti_cfg {
	uint32_t base;
	uint32_t f_sys;
    const struct pinctrl_dev_config *pcfg;
};


#endif

#ifdef __cplusplus
}
#endif