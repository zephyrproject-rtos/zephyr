/**
 * Project                           : Secure IoT SoC
 * Name of the file                  : spi_shakti.c
 * Brief Description of file         : This is a zephyr rtos SPI Driver file for Mindgrove Silicon's SPI Peripheral.
 * Name of Author                    : Kishore J
 * Email ID                          : kishore@mindgrovetech.in
 * 
 * @file spi_shakti.c
 * @author Kishore J (kishore@mindgrovetech.in)
 * @brief This is a zephyr rtos SPI Driver file for Mindgrove Silicon's SPI Peripheral.
 * @version 0.1
 * @date 2024-04-17
 * 
 * @copyright Copyright (c) Mindgrove Technologies Pvt. Ltd 2023. All rights reserved.
 * 
 * @copyright Copyright (c) 2017 Google LLC.
 * @copyright Copyright (c) 2018 qianfan Zhao.
 * @copyright Copyright (c) 2023 Gerson Fernando Budke.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT shakti_spi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_shakti);

#include <soc.h>
#include <stdbool.h>
#include <zephyr/sys/util.h>
#include<zephyr/kernel.h>
#include "spi_shakti.h"

/*HELPER FUNCTIONS*/

int spi_number;
sspi_struct *sspi_instance[SSPI_MAX_COUNT];

/* configuration from device tree */
// int sclk_config[5] = DT_PROP(DT_NODELABEL(spi0), sclk_configure);
// int comm_config[5] = DT_PROP(DT_NODELABEL(spi0), comm_configure);

/* variables used to configure spi */
int pol;
int pha;
int prescale = 0x10;
int setup_time = 0x0;
int hold_time = 0x0;
int master_mode;
int lsb_first;
int comm_mode;
int spi_size;

/**
 * @fn int sspi_shakti_init(const struct device *dev)
 * @details The function initializes a SPI instance and returns its base address.
 * @param dev in the `sspi_shakti_init` function is a pointer to a structure of type `device`.
 * It is used to access information about the device being initialized.
 * @return the base address of the SPI instance that was initialized. If the initialization fails due
 * to an invalid SPI instance number, it returns -1.
 */

int sspi_shakti_init(const struct device *dev)
{ 
  // printk("init\n");
  int spi_base;
  char *spi_inst; 
  spi_inst = dev->name;
  struct spi_shakti_cfg *confg = (struct spi_shakti_cfg *)dev->config;
  printk("SPI: %s\n", spi_inst);
  spi_number = spi_inst[6] - '0';
  gpio_pin_configure_dt(&(((struct spi_shakti_cfg*)(dev->config))->ncs),1);
  printk("SPI NUMBER: %d\n", spi_number);
  k_mutex_init(&(confg->mutex));
  if (spi_number < SSPI_MAX_COUNT & spi_number >= 0){
    sspi_instance[spi_number] = (sspi_struct*) ( (SSPI0_BASE_ADDRESS + ( spi_number * SSPI_BASE_OFFSET) ) );
    spi_base = sspi_instance[spi_number];
    #ifdef SPI_DEBUG
    printk("\nSPI%d Initialized..", spi_number);
    #endif
    return 0;
  }
  else{
    printk("\nInvalid SPI instance %d. This SoC supports only SPI-0 to SPI-3", spi_number);
    return -1;
  }
  return 0;
}


/**
 * @fn int sspi_shakti_configure(const struct device *dev, const struct spi_config *config)
 * @details This function configures the spi device.
 * @param dev in the `sspi_shakti_configure` function is a pointer to a structure of type `device`.
 * It is used to access information about the device being initialized.
 * @param config in the `sspi_shakti_configure` function is a pointer to a structure of type `spi_config`.
 * It is used to access the information about the configurations of the spi device.
 * @return -EINVAL if the invalid configuration is set.
 */

// To know about the bit fields in the operation refer to spi.h
int sspi_shakti_configure(const struct device *dev, const struct spi_config *config)
{   
  // if spi is configured as master 0th bit in the operation is set to 0.
  if ((config->operation & 0x1) == 0){ 
    printk("master is supported\n");
    master_mode = MASTER; 
  }
  else {
    printk("Slave is not supported\n");
    return -EINVAL;
  }
  // spi.h have 3 modes, but secure-iot spi only supports 2 modes pol and pha. 
  if((config->operation & POL_AND_PHA) == POL_AND_PHA){
    printk("pol 1 pha 1 \n");
    pol = 1;
    pha = 1;
  } else if((config -> operation & INV_POLANDPHA) == INV_POLANDPHA){
    printk("pol 0 and pha 0 \n");
    pol = 0;
    pha = 0;
  }
  else{
    printk("Invalid pol and pha combination \n");
    return -EINVAL;
  }

  // If lsb is set, then 4th bit in the operation is set to 1 or if msb is set the 4th bit in the operation is set to 0.
  if((config -> operation & SPI_TRANSFER_LSB) == SPI_TRANSFER_MSB){
    printk("MSB FIRST \n");   
    lsb_first = MSB_FIRST;
  }
  else {
    printk("LSB FIRST\n");    
    lsb_first = LSB_FIRST;
  }
  
  // spi data size fields in operation is from 5 to 10, if the word size is set to 8 then 8th bit is set to 1.
  if( config -> operation & SPI_WORD_SET(8))
  {
    printk("SPI size 8 \n");
    spi_size = DATA_SIZE_8;
  }
  // spi data size fields in operation is from 5 to 10, if the word size is set to 16 then 9th bit is set to 1.
  else if(config -> operation & SPI_WORD_SET(16))
  { 
    printk("SPI size 16 \n");
    spi_size = DATA_SIZE_16;
  }
  // spi data size fields in operation is from 5 to 10, if the word size is set to 32 then 10th bit is set to 1.
  else if(config -> operation & SPI_WORD_SET(32))
  {
    printk("SPI size 32 \n");
    spi_size = DATA_SIZE_32;
  }
  else {
    printk("Invalid data size \n");
    return -EINVAL;
  }

  // If spi mode is set to full duplex then 11th bit in operation is set to 0, else if spi mode is set to half duplex then 11th bit in operation is set to 1.
  if((config -> operation & HALFDUPLEX) ==  HALFDUPLEX){
    printk("Half duplex \n");
    comm_mode = HALF_DUPLEX;
  }
  else if((config -> operation & FULLDUPLEX) == FULLDUPLEX){
    printk("Full duplex \n"); // default spi works in full duplex.
    comm_mode = FULL_DUPLEX;
  }
  else{
    printk("SPI supports only fullduplex or halfduplex mode \n");
    return -EINVAL;
  }
}

/**
 * @fn void sspi_shakti_busy(const struct device *dev)
 * @brief The function waits until the SPI communication is busy.
 * @details It is used to select the specific SPI instance that needs to be checked for busy status.
 * @param dev The function `sspi_shakti_busy` takes a pointer to a structure `device` as a parameter
 * named `dev`. This structure likely contains information about the device that needs to be intereacted* 
 */

void sspi_shakti_busy(const struct device *dev)
{
  // printk("busy\n");
  volatile uint16_t temp; 
  while(1){
    temp = sspi_instance[spi_number]->comm_status;
    temp =  temp & SPI_BUSY;
    #ifdef SPI_DEBUG
    printk("\nchecking SPI%d busy", spi_number);
    #endif
    if (temp == SPI_BUSY){
      printk("\nSPI%d is busy", spi_number);
    }
    else {
      break;
    }
  }
  #ifdef SPI_DEBUG
  printk("\nSPI%d is not busy", spi_number);
  #endif
}


/**
 * @fn void sclk_shakti_config( const struct device *dev, int pol, int pha, int prescale, int setup_time, int hold_time)
 * @details The function configures the clock control settings for a specified SPI instance.
 * @param dev in the `sclk_shakti_config` function is a pointer to a structure of type `device`. It is used to 
 * specify the device for which the SPI clock configuration is being set.
 * @param pol The polarity of the clock signal. It determines the idle state of the clock signal. If it
 * is set to 0, the clock signal is low when idle. If it is set to 1, the clock signal is high when
 * idle.
 * @param pha The pha parameter in the sclk_config function is used to set the clock phase of the SPI
 * (Serial Peripheral Interface) bus. It determines the timing relationship between the data and the
 * clock signal. A value of 0 indicates that data is captured on the leading edge of the clock signal,
 * while value of 1 indicates that data is captured on the trailing edge of the clock signal.
 * @param prescale The prescale parameter is used to set the clock frequency of the SPI bus. It
 * determines the division factor of the system clock frequency to generate the SPI clock frequency.
 * @param setup_time The setup time parameter in the sclk_config function is the delay between the
 * assertion of the chip select signal and the start of the clock signal. It is the time required for
 * the slave device to prepare for data transfer.
 * @param hold_time Hold time refers to the amount of time that the SPI clock signal should be held low
 * after the data has been transmitted. This is important to ensure that the receiving device has
 * enough time to sample the data correctly. The hold time is typically specified in terms of clock
 * cycles.
 * @note if \a pol is 0, then \a pha cannot be 1, as it makes sampling at Falling edge. Likewise, if \a pol is 1 
 * then \a pha cannot be 0 as it makes sampling at the Falling edge.
 */

void sclk_shakti_config(const struct device *dev, int pol, int pha, int prescale, int setup_time, int hold_time)
{
  // printk("configuring \n");
  sspi_shakti_busy(dev);

  if ((pol == 0 && pha == 1)||(pol == 1 && pha == 0)){
    printk("\nInvalid Clock Configuration (NO FALLING EDGE). Hence Exiting..!");
    exit(-1);
  }
  else{
    sspi_instance[spi_number] -> clk_control =  SPI_TX2SS_DELAY(hold_time) | SPI_SS2TX_DELAY(setup_time) | SPI_PRESCALE(prescale)| SPI_CLK_POLARITY(pol) | SPI_CLK_PHASE(pha);
    printk("clk %x\n",  sspi_instance[spi_number] -> clk_control );
  }
}


/**
 * @fn void sspi_shakti_configure_clock_in_hz(const struct device *dev, uint32_t bit_rate)
 * @brief The function configures the clock frequency for a specified SPI interface based on a given bit rate.
 * @details Calculates the prescaler value from the passed bit_rate and updates the prescaler value in clock control register.
 * @param dev is a pointer to a structure of type `device`. It is used to specify the device for which the 
 * SPI clock configuration in hz is set.
 * @param bit_rate The desired clock frequency in Hz for the SPI communication.
 * @return nothing (void).  
 */

void sspi_shakti_configure_clock_in_hz(const struct device *dev, uint32_t bit_rate)
{
    uint8_t prescaler = (CLOCK_FREQUENCY / bit_rate) - 1;

    if(bit_rate >= CLOCK_FREQUENCY){
        printk("\n Invalid bit rate value. Bit rate should be less than CLOCK_FREQUENCY");
		    return;
    }

    uint32_t temp = sspi_instance[spi_number] -> clk_control & (~SPI_PRESCALE(0xFF) );
    sspi_instance[spi_number] -> clk_control = temp | SPI_PRESCALE(prescaler);
}


/**
 * @fn void sspi_shakti_comm_control_config(int spi_number, int master_mode, int lsb_first, int comm_mode, int spi_size)
 * @details The function configures the communication control for a specified SPI Instance based on the given
 * parameters.
 * @param dev is a pointer to a structure of type `device`. It is used to access information about the device that needs 
 * to be configured.
 * @param master_mode Specifies whether the SPI interface is operating in master or slave mode. It can
 * take the values MASTER or SLAVE which is defined in the \a sspi.h file.
 * @param lsb_first The lsb_first parameter is a flag that determines whether the least significant bit
 * (LSB) or the most significant bit (MSB) is transmitted first in the SPI communication. If lsb_first
 * is set to 1, the LSB is transmitted first, and if it is set to 0, MSB will be transmitted first.
 * @param comm_mode The communication mode of the SPI interface, which can be set to either 
 * \arg SPI_MODE_0 = SIMPLEX_TX,
 * \arg SPI_MODE_1 = SIMPLEX_RX,
 * \arg SPI_MODE_2 = HALF_DUPLEX, or
 * \arg SPI_MODE_3 = FULL_DUPLEX. 
 * These defines are included in the \a sspi.h .
 * @param spi_size spi_size refers to the total number of bits to be transmitted and received in each
 * SPI transaction. It is used to configure the SPI communication control register. Mostly, the values for 
 * the \a spi_size will be 8, 16. and 32.
 * @see sspi_busy function.
 */

void sspi_shakti_comm_control_config(const struct device *dev, int master_mode, int lsb_first, int comm_mode, int spi_size)
{
  // printk("comm_config \n");
  int out_en=0;

  if (master_mode==MASTER){
    out_en = SPI_OUT_EN_SCLK | SPI_OUT_EN_NCS | SPI_OUT_EN_MOSI ;
    }
  else{
    out_en = SPI_OUT_EN_MISO;
  }
  sspi_shakti_busy(dev);
  sspi_instance[spi_number]->comm_control = \
        SPI_MASTER(master_mode) | SPI_LSB_FIRST(lsb_first) | SPI_COMM_MODE(comm_mode) | \
        SPI_TOTAL_BITS_TX(spi_size) | SPI_TOTAL_BITS_RX(spi_size) | \
        out_en;
}


/**
 * @fn void sspi_shakti_enable(const struct device *dev)
 * @brief The function enables a specified SPI instance.
 * @param dev is a pointer to a structure of type `device`. It contains the information about the device 
 * that needs to be enabled.
 */

void sspi_shakti_enable(const struct device *dev)
{
  uint32_t temp = sspi_instance[spi_number]->comm_control;
  sspi_shakti_busy(dev);
  sspi_instance[spi_number]->comm_control = temp | SPI_ENABLE(ENABLE);
}

/**
 * @fn void sspi_shakti_disable(const struct device *dev)
 * @brief This function disables a specified SPI instance.
 * @param dev is a pointer to a structure of type 'device'. It contains the information about the device that
 * needs to be disabled.
 *  */

void sspi_shakti_disable(const struct device *dev)
{
  uint32_t temp = sspi_instance[spi_number]->comm_control;
  uint32_t disable = ~SPI_ENABLE(ENABLE);
  //sspi_busy(spi_number);
  sspi_instance[spi_number]->comm_control = 0;// temp & disable; //| SPI_ENABLE(DISABLE);
}

/**
 * @fn int sspi8_shakti_transmit_data(const struct device *dev, struct spi_buf *tx_data)
 * @brief The function transmits data over SPI in 8-bit format.
 * @details This function checks if the TX FIFO is not full, and it writes the data to the tx register.
 * @param dev The `dev` parameter in the `sspi8_shakti_transmit_data` function is a pointer to a
 * structure of type `device`. It is used to specify the device on which the SPI communication is being
 * performed.
 * @param tx_data The `tx_data` parameter is a pointer to a `spi_buf` structure, which likely contains
 * information about the data to be transmitted over SPI. The structure includes a buffer
 * (`buf`) that holds the actual data to be sent and a length (`len`) indicating the size of the data
 * buffer
 * @see sspi_check_tx_fifo_32 function and sspi_wait_till_tx_complete function.
 */

int sspi8_shakti_transmit_data(const struct device *dev, uint8_t data)
{
    printk("transferring");
    volatile int temp = sspi_shakti_check_tx_fifo_32();
    if(temp == 0){
      printk("tx_data : %d\n",data);
      sspi_instance[spi_number]->data_tx.data_8 = data;
      #ifdef SPI_DEBUG
      printk("tx_data[%d] = %d\n", i, tx_data[i]);
      #endif
    }
    else{
      #ifdef SPI_DEBUG
      printk("\nTX FIFO is full");
      #endif
      printk("TX FIFO is full \n");
      return -1;
    }

  sspi_shakti_wait_till_tx_complete();
  printk("transferring1 \n");
  return 0;
}

/**
 * @fn int sspi16_shakti_transmit_data(const struct device *dev, struct spi_buf *tx_data)
 * @brief The function transmits 16-bit data over SPI in chunks of a specified size.
 * @details This function checks if the TX FIFO contains atleast 2-bytes space, and it writes the data to the tx register.
 * @param dev The `dev` parameter in the `sspi16_shakti_transmit_data` function is a pointer to a
 * structure of type `device`. It is used to specify the device on which the SPI communication is being
 * performed.
 * @param tx_data The `tx_data` parameter is a pointer to a `spi_buf` structure, which likely contains
 * information about the data to be transmitted over SPI. The structure includes a buffer
 * (`buf`) that holds the actual data to be sent and a length (`len`) indicating the size of the data
 * buffer.
 * @see sspi_check_tx_fifo_30 function and sspi_wait_till_tx_complete function.
 * @return an integer value. If the transmission is successful, it returns 0. If the TX FIFO does not
 * have enough space, it returns -1.
 */

int sspi16_shakti_transmit_data(const struct device *dev, struct spi_buf *tx_data)
{
  int data = tx_data->len;
  printk("DATA_SIZE : %d\n", data);

  for(int i = 0; i<data; i++){
    volatile int temp = sspi_shakti_check_tx_fifo_30();
    uint16_t *buffer = tx_data->buf;
    if (temp == 0){
      printk("tx_data[%d] : %x\n", i, buffer[i]);
      printk("Addr :%#x",&(sspi_instance[spi_number]->data_tx.data_16));
      sspi_instance[spi_number]->data_tx.data_16 = buffer[i];

      #ifdef SPI_DEBUG
      printk("tx_data[%d] = %d\n", i, tx_data[i]);
      #endif
    }
    else{
      #ifdef SPI_DEBUG
      printk("\nTX FIFO has not enough space.");
      #endif
      return -1;
    }
  }
  
  sspi_shakti_wait_till_tx_complete(dev);
  return 0;
}

/**
 * @fn int sspi32_shakti_transmit_data(const struct device *dev, struct spi_buf *tx_data)
 * @brief The function transmits 32-bit data over SPI in chunks of a specified size.
 * @details This function checks if the TX FIFO contains atleast 4-bytes space, and it writes the data to the tx register. 
 * @param dev The `dev` parameter in the `sspi32_shakti_transmit_data` function is a pointer to a
 * structure of type `device`. It is used to specify the device on which the SPI communication is being
 * performed.
 * @param tx_data The `tx_data` parameter is a pointer to a `spi_buf` structure, which likely contains
 * information about the data to be transmitted over SPI. The structure includes a buffer
 * (`buf`) that holds the actual data to be sent and a length (`len`) indicating the size of the data
 * buffer.
 * @see sspi_check_tx_fifo_28 function and sspi_wait_till_tx_complete function.
 * @return an integer value. If the transmission is successful, it returns 0. If the TX FIFO does not
 * have enough space, it returns -1.
 */

int sspi32_shakti_transmit_data(const struct device *dev, struct spi_buf *tx_data)
{
  int data = tx_data->len;
  for( int i = 0; i<data; i++){
    volatile int temp = sspi_shakti_check_tx_fifo_28();
    uint32_t *buffer = tx_data->buf;
    if (temp == 0){
      printk("tx_data[%d] : %x\n", i, buffer[i]);
      printk("Addr :%#x",&(sspi_instance[spi_number]->data_tx.data_32));
      sspi_instance[spi_number]->data_tx.data_32 = buffer[i];
      #ifdef SPI_DEBUG
      printk("tx_data[%d] = %d\n", i, tx_data[i]);
      #endif
      }
    else{
      #ifdef SPI_DEBUG
      printk("\nTX FIFO has not enough space.");
      #endif
      return -1;
    }
  }
  sspi_shakti_wait_till_tx_complete(dev);
  return 0;
}

/**
 * @fn sspi_shakti_wait_till_tx_not_en(const struct device *dev)
 * @brief The function waits until the transmit enable flag of a specified SPI instance is cleared.
 * @param dev is a pointer to the structure of type 'device'. It contains the information about the 
 * SPI peripheral that needs to be configured and operated on.
 */

void sspi_shakti_wait_till_tx_not_en(const struct device *dev)
{
  uint16_t temp;
  while(1){
    temp = sspi_instance[spi_number]->comm_status;
    temp = temp & SPI_TX_EN;
    if (temp == 0){
      break;
    }
  }
}

/**
 * @fn void sspi_shakti_wait_till_tx_complete(int spi_number) 
 * @brief The function when called, waits till the TX transfer completes.
 * @details This function waits until the transmit FIFO of a specified SPI instance is empty.
 * @param dev The `dev` parameter in the `sspi_shakti_wait_till_tx_complete` function is a pointer to a
 * structure of type `device`. This structure likely contains information or configurations related to
 * the device being used for SPI communication.
 * @see sspi_shakti_wait_till_tx_not_en function.
 */

void sspi_shakti_wait_till_tx_complete(const struct device *dev)
{
  volatile uint32_t temp;
  int i = 0;
  #ifdef SPI_DEBUG
  printk("\nChecking TX FIFO is empty.");
  #endif
  while(1){
    // sspi_shakti_wait_till_tx_not_en(dev); 
    temp = sspi_instance[spi_number]->fifo_status;
    temp = temp & SPI_TX_EMPTY; 
    if (temp == 0){
      sspi_shakti_enable(dev);
      if (i == 0){
        #ifdef SPI_DEBUG
        printk("\nTX FIFO is not empty, waiting till the FIFO gets empty.");
        #endif
      }
      else{
        #ifdef SPI_DEBUG
        printk("\nTX FIFO is not empty");
        #endif
      }
      i += 1;
    }
    else{
      break;
    }
  }
  #ifdef SPI_DEBUG
  printk("\nTX FIFO is empty");
  #endif
}

/**
 * @fn int sspi_shakti_check_tx_fifo_32(const struct device *dev)
 * @brief This function when called will check if TX Buffer is Full.
 * @param dev is a pointer to a structure of type 'device'. It constains the information about the
 * SPI peripheral to be configured and operated on.
 * @return an integer value. If the TX FIFO is full, it returns 1. If the TX FIFO is not full, it
 * returns 0.
 */

int sspi_shakti_check_tx_fifo_32(const struct device *dev)
{
  volatile uint32_t temp;
  #ifdef SPI_DEBUG
  printk("\nChecking if TX FIFO is full.");
  #endif

  temp = sspi_instance[spi_number]->fifo_status;
  // printk("\ntemp : %x", temp);
  temp = temp & SPI_TX_FULL;

  if (temp == SPI_TX_FULL){
    #ifdef SPI_DEBUG
    printk("\nTX FIFO is full, waiting till fifo gets not full.");
    #endif
    return 1;
  }
  else{
    #ifdef SPI_DEBUG
    printk("\nTX FIFO is not full.");
    #endif
    return 0;
  }
}

/**
 * @fn int sspi_shakti_check_tx_fifo_30(const struct device *dev)
 * @brief This function when called will check if TX Buffer contains space for atleast 2 bytes 
 * and also transmit FIFO of a specified SPI instance has atmost 30 bytes.
 * @param dev is a pointer to a structure of type 'device'. It constains the information about the
 * SPI peripheral to be configured and operated on.
 * @return an integer value. If the TX FIFO contains less than or equal to 30 bytes, it returns 0.
 * Otherwise, it returns 1.
 */

int sspi_shakti_check_tx_fifo_30(const struct device *dev)
{
  uint32_t temp;
  uint16_t temp1;
  #ifdef SPI_DEBUG
  printk("\nChecking if TX FIFO is less than 30.");
  #endif

  temp = sspi_instance[spi_number]->fifo_status;
  temp = temp & SPI_TX_30;
  temp1 = sspi_instance[spi_number]->comm_status;
  temp1 = temp1 & SPI_TX_FIFO(7);

  if ((temp == SPI_TX_30 && temp1 == SPI_TX_FIFO(7)) || (temp1 < SPI_TX_FIFO(7))){
    #ifdef SPI_DEBUG
    printk("\nTX FIFO is not 30.");
    #endif
    return 0;
  }
  else{
    #ifdef SPI_DEBUG
    printk("\nTX FIFO contains less than or equal to 30.");
    #endif
    return 1;
  }
}

/**
 * @fn int sspi_shakti_check_tx_fifo_28(const struct device *dev)
 * @brief This function when called will check if TX Buffer contains space for atleast 4 bytes.
 * and also transmit FIFO of a specified SPI instance has atmost 28 bytes.
 * @param dev is a pointer to a structure of type 'device'. It constains the information about the
 * SPI peripheral to be configured and operated on.
 * @return an integer value. If the TX FIFO contains less than or equal to 28 bytes, it returns 0.
 * Otherwise, it returns 1.
 */

int sspi_shakti_check_tx_fifo_28(const struct device *dev)
{
  uint32_t temp;
  uint16_t temp1;

  #ifdef SPI_DEBUG
  printk("\nChecking if TX FIFO is less than 28.");
  #endif

  temp = sspi_instance[spi_number]->fifo_status;
  temp = temp & SPI_TX_28;
  temp1 = sspi_instance[spi_number]->comm_status;
  temp1 = temp1 & SPI_TX_FIFO(7);

  if ((temp == SPI_TX_28 && temp1 == SPI_TX_FIFO(6)) || (temp1 < SPI_TX_FIFO(6))){
    #ifdef SPI_DEBUG
    printk("\nTX FIFO is not 28.");
    #endif
    return 0;
  }
  else{
    #ifdef SPI_DEBUG
    printk("\nTX FIFO contains less than or equal to 28.");
    #endif
    return 1;
  }
}

/**
 * @fn void sspi_wait_till_rxfifo_not_empty(int spi_number)
 * @brief The function waits until the receive FIFO of a specified SPI instance is not empty.
 * @param dev is a pointer to a structure of type 'device'. It constains the information about the
 * SPI peripheral to be configured and operated on.
 */

void sspi_shakti_wait_till_rxfifo_not_empty(const struct device *dev)
{
  volatile uint32_t temp;
  while(1){
    temp = sspi_instance[1]->fifo_status;
    temp = temp & SPI_RX_EMPTY;
    if (temp == 0){
      #ifdef SPI_DEBUG
      printk("\nRX FIFO is not empty.");
      #endif
      break;
    }
  }
}

/**
 * @fn void sspi_shakti_wait_till_rxfifo_2(const struct device *dev)
 * @brief The function waits until the receive FIFO of a specified SPI instance has atleast 2 bytes.
 * @param dev is a pointer to a structure of type 'device'. It constains the information about the
 * SPI peripheral to be configured and operated on.
 */

void sspi_shakti_wait_till_rxfifo_2(const struct device *dev)
{
  volatile uint32_t temp;
  volatile uint16_t temp1;

  while(1){
    temp = sspi_instance[spi_number]->fifo_status;
    temp = temp & SPI_RX_DUAL;

    temp1 = sspi_instance[spi_number]->comm_status;
    temp1 = temp1 & SPI_RX_FIFO(7);

    // if ((temp == SPI_RX_DUAL && temp1 == SPI_RX_FIFO(1)) || (temp1 > SPI_RX_FIFO(0))){
    //   #ifdef SPI_DEBUG
    //   printk("\nRX FIFO is not empty.");
    //   #endif
    //   break;
    // }

    if((temp1 >= SPI_RX_FIFO(1))){
      #ifdef SPI_DEBUG
      printk("\nRX FIFO is more than or equal to 2 bytes.");
      #endif
      break;
    }
  }
}

/**
 * @fn void sspi_shakti_wait_till_rxfifo_2(const struct device *dev)
 * @brief The function waits until the receive FIFO of a specified SPI instance has atleast 4 bytes.
 * @param dev is a pointer to a structure of type 'device'. It constains the information about the
 * SPI peripheral to be configured and operated on.
 */

void sspi_shakti_wait_till_rxfifo_4(const struct device *dev)
{
  volatile uint32_t temp;
  volatile uint16_t temp1;
  while(1){
    temp = sspi_instance[spi_number]->fifo_status;
    temp = temp & SPI_RX_QUAD;

    temp1 = sspi_instance[spi_number]->comm_status;
    temp1 = temp1 & SPI_RX_FIFO(7);

    // if ((temp == SPI_RX_QUAD && temp1 == SPI_RX_FIFO(2)) || (temp1 > SPI_RX_FIFO(1))){
    //   #ifdef SPI_DEBUG
    //   printk("\nRX FIFO is not empty.");
    //   #endif
    //   break;
    // }
    if (temp1 >= SPI_RX_FIFO(2)){
      #ifdef SPI_DEBUG
      printk("\nRX FIFO is more than or equal to 4 bytes.");
      #endif
      break;
    }
  }
}

/**
 * @fn void sspi8_shakti_receive_data(const struct device *dev, struct spi_buf *rx_data))
 * @brief The function receives 8-bit data from an SPI interface.
 * @details This function checks if the RX FIFO is not empty, and then it reads the data from the rx register.
 * @param dev is a pointer to the structure of type 'device'. It contains the information about the SPI device 
 * thas is used for receiving data.
 * @param rx_data The `rx_data` parameter is a pointer to a `spi_buf` structure, which typically
 * contains information about the buffer used for receiving data over SPI communication. The structure
 * includes fields such as `buf` (pointer to the data buffer), `len` (length of the data
 * buffer)
 */

uint8_t sspi8_shakti_receive_data(const struct device *dev)
{
  volatile uint32_t temp;
  uint8_t data;
  temp = sspi_instance[spi_number]->comm_control;
  temp = temp & SPI_COMM_MODE(3);
    if (temp == SPI_COMM_MODE(1)){
      sspi_shakti_enable(dev);
    }
    sspi_shakti_wait_till_rxfifo_not_empty(dev);
    data = sspi_instance[spi_number]->data_rx.data_8;
    printk("rx_data : %d\n",data);
    return data;
}

/**
 * @fn void sspi16_shakti_receive_data(const struct device *dev, struct spi_buf *rx_data))
 * @brief The function receives 16-bit data from an SPI interface.
 * @details This function checks if the RX FIFO contains atleast 2-bytes, and then it reads the data from the rx register.
 * @param dev is a pointer to the structure of type 'device'. It contains the information about the SPI device 
 * thas is used for receiving data.
 * @param rx_data The `rx_data` parameter is a pointer to a `spi_buf` structure, which typically
 * contains information about the buffer used for receiving data over SPI communication. The structure
 * includes fields such as `buf` (pointer to the data buffer), `len` (length of the data
 * buffer)
*/

void sspi16_shakti_receive_data(const struct device *dev, struct spi_buf *rx_data)
{
  int no_of_itr = rx_data->len;;
  volatile uint32_t temp;
  temp = sspi_instance[spi_number]->comm_control;
  temp = temp & SPI_COMM_MODE(3);

  for (int i=0; i < no_of_itr; i++){
    
    uint16_t *buffer = rx_data->buf;
    if (temp == SPI_COMM_MODE(1)){
      sspi_shakti_enable(dev);
    }
    sspi_shakti_wait_till_rxfifo_2(dev);
    buffer[i] = sspi_instance[spi_number]->data_rx.data_16;
    printk("rx_data[%d] : %x\n", i, buffer[i]);
  }
}

/**
 * @fn void sspi32_shakti_receive_data(const struct device *dev, struct spi_buf *rx_data))
 * @brief The function receives 16-bit data from an SPI interface.
 * @details This function checks if the RX FIFO contains atleast 2-bytes, and then it reads the data from the rx register.
 * @param dev is a pointer to the structure of type 'device'. It contains the information about the SPI device 
 * thas is used for receiving data.
 * @param rx_data The `rx_data` parameter is a pointer to a `spi_buf` structure, which typically
 * contains information about the buffer used for receiving data over SPI communication. The structure
 * includes fields such as `buf` (pointer to the data buffer), `len` (length of the data
 * buffer)
*/

void sspi32_shakti_receive_data(const struct device *dev, struct spi_buf *rx_data)
{
  int no_of_itr = rx_data->len;;
  volatile uint32_t temp;
  temp = sspi_instance[spi_number]->comm_control;
  temp = temp & SPI_COMM_MODE(3);

  for (int i=0; i < no_of_itr; i++){

    uint32_t *buffer = rx_data->buf;
    if (temp == SPI_COMM_MODE(1)){
      sspi_shakti_enable(dev);
    }
    sspi_shakti_wait_till_rxfifo_4(dev);
    buffer[i] = sspi_instance[spi_number]->data_rx.data_32;
    printk("rx_data[%d] : %x\n", i, buffer[i]);
  }
}


void qualify(const struct device *dev)
{
  sspi_instance[spi_number]->qual = 3;
}


void inter_enable_config(const struct device *dev, uint32_t value)
{

  sspi_instance[spi_number]->intr_en = value;
}


/* API Functions */

/**
 * @fn int spi_shakti_transceive(const struct device *dev, const struct spi_config *config, sonst struct spi_buf_set *tx_bufs,
 *                                const struct spi_buf_set *rx_bufs)
 * @brief The function `spi_shakti_transceive` handles SPI communication based on the specified configuration
 * parameters.
 * @param dev The `dev` parameter is a pointer to a structure of type `device`. It is used to reference
 * the device for which the SPI communication is being performed.
 * @param config The `config` parameter in the `spi_shakti_transceive` function is of type `struct
 * spi_config` and contains configuration settings for the SPI communication such as clock, polarity, phase, 
 * prescale value, setup time, and hold time.
 * @param tx_bufs The `tx_bufs` parameter in the `spi_shakti_transceive` function represents the
 * transmit buffer set for SPI communication. It contains the data that needs to be transmitted over
 * the SPI bus. The function uses this parameter to send data based on the specified communication mode such as 
 * simplex tx, simplex rx. full duplex and half duplex.
 * @param rx_bufs The `rx_bufs` parameter in the `spi_shakti_transceive` function represents the
 * receive buffer set for SPI communication. It contains the data buffers where received data will be
 * stored after a SPI transaction. The function uses this parameter to set up the receive buffers for
 * different communication modes such as simplex tx, simplex rx. full duplex and half duplex.
 */

static int spi_shakti_transceive(const struct device *dev,
			  const struct spi_config *config,
			  const struct spi_buf_set *tx_bufs,
			  const struct spi_buf_set *rx_bufs)
        
{
  // spi_context_lock(&SPI_DATA(dev)->ctx, false, NULL, NULL, config);
  sspi_shakti_configure(dev, config);
  #ifdef SPI_DEBUG
    printk("pol %d\n", pol);
    printk("pha %d\n", pha);
    printk("prescale %d\n", prescale);
    printk("setup_time %d\n", setup_time);
    printk("hold_time %d\n", hold_time);
    printk("master_mode %d\n", master_mode);
    printk("lsb_first %d\n", lsb_first);
    printk("comm_mode %d\n", comm_mode);
    printk("spi_size %d\n", spi_size);
  #endif
  uint8_t rx_buff[16];
  struct spi_buf rx_loc_bufs = { .buf = rx_buff, .len = tx_bufs->buffers->len};
  struct spi_buf_set rx_loc_buffs = { .buffers = &rx_loc_bufs, .count = 1};
  if(rx_bufs == NULL){
    rx_bufs = &rx_loc_buffs;
  }
  sspi_shakti_init(dev);
  sclk_shakti_config(dev, pol, pha, prescale, setup_time, hold_time);
  k_mutex_lock(&(((struct spi_shakti_cfg*)(dev->config))->mutex),K_FOREVER);
  printf("Pin :%d\n",((struct spi_shakti_cfg*)(dev->config))->ncs.pin);
  //gpio_pin_set_dt(&(((struct spi_shakti_cfg*)(dev->config))->ncs), 1);
  uint32_t len;
  if (comm_mode == FULL_DUPLEX)
  {
    sspi_shakti_comm_control_config(dev, master_mode, lsb_first, comm_mode, spi_size);
    spi_context_buffers_setup(&SPI_DATA(dev)->ctx, tx_bufs, rx_bufs, 1);

    if (spi_size == DATA_SIZE_8)
    {
      len = tx_bufs->buffers->len;
      for(int i = 0;i<len;i++){
      k_sched_lock();
      sspi8_shakti_transmit_data(dev, ((uint8_t*)(tx_bufs->buffers->buf))[i]);
      ((uint8_t*)(tx_bufs->buffers->buf))[i] = sspi8_shakti_receive_data(dev);  
      k_sched_unlock();
    }
    }
    if (spi_size == DATA_SIZE_16)
    {
      sspi16_shakti_transmit_data(dev, tx_bufs->buffers);
      sspi16_shakti_receive_data(dev, rx_bufs->buffers);  
    }

    if (spi_size == DATA_SIZE_32)
    {
      sspi32_shakti_transmit_data(dev, tx_bufs->buffers);
      sspi32_shakti_receive_data(dev, rx_bufs->buffers);  
    }
  }

  if (comm_mode == HALF_DUPLEX)
  {
    sspi_shakti_comm_control_config(dev, master_mode, lsb_first, comm_mode, spi_size);
    spi_context_buffers_setup(&SPI_DATA(dev)->ctx, tx_bufs, rx_bufs, 1);

    if (spi_size == DATA_SIZE_8)
    {
      len = tx_bufs->buffers->len;
      for(int i = 0;i<len;i++){
      sspi8_shakti_transmit_data(dev, ((uint8_t*)(tx_bufs->buffers->buf))[i]);
      ((uint8_t*)(tx_bufs->buffers->buf))[i] = sspi8_shakti_receive_data(dev);
    }

    if (spi_size == DATA_SIZE_16)
    {
      sspi16_shakti_transmit_data(dev, tx_bufs->buffers);
      sspi16_shakti_receive_data(dev, rx_bufs->buffers);  
    }

    if (spi_size == DATA_SIZE_32)
    {
      sspi32_shakti_transmit_data(dev, tx_bufs->buffers);
      sspi32_shakti_receive_data(dev, rx_bufs->buffers);  
    }
  }
  //gpio_pin_set_dt(&(((struct spi_shakti_cfg*)(dev->config))->ncs), 0);
  k_mutex_unlock(&(((struct spi_shakti_cfg*)(dev->config))->mutex));

  return 0;
}
}
static int spi_shakti_release(const struct device *dev,
		       const struct spi_config *config)
{
	// spi_context_unlock_unconditionally(&SPI_DATA(dev)->ctx);
	return 0;
}

static struct spi_driver_api spi_shakti_api = {
	.transceive = spi_shakti_transceive,
	.release = spi_shakti_release,
};

#define SPI_INIT(n)	\
  static struct spi_shakti_data spi_shakti_data_##n = { \
    SPI_CONTEXT_INIT_LOCK(spi_shakti_data_##n, ctx), \
    SPI_CONTEXT_INIT_SYNC(spi_shakti_data_##n, ctx), \
    SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)	\
  }; \
  static struct spi_shakti_cfg spi_shakti_cfg_##n = { \
    .ncs = GPIO_DT_SPEC_INST_GET(n, cs_gpios),\
    .base = SPI_START_##n , \ 
    .f_sys = CLOCK_FREQUENCY, \
  }; \
  DEVICE_DT_INST_DEFINE(n, \
        sspi_shakti_init, \
        NULL, \
        &spi_shakti_data_##n, \
        &spi_shakti_cfg_##n, \
        POST_KERNEL, \
        CONFIG_SPI_INIT_PRIORITY, \
        &spi_shakti_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_INIT)