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
#include <stdio.h>

/*HELPER FUNCTIONS*/

int spi_number;
sspi_struct *sspi_instance[SSPI_MAX_COUNT];

#define CLEAR_MASK                         0x0000

/* configuration from device tree */
// int sclk_config[5] = DT_PROP(DT_NODELABEL(spi0), sclk_configure);
// int comm_config[5] = DT_PROP(DT_NODELABEL(spi0), comm_configure);

/* variables used to configure spi */
int pol;
int pha;
int prescale = 0x01;
int setup_time = 0x0;
int hold_time = 0x0;
int master_mode;
int lsb_first;
int comm_mode;
int spi_size;

typedef struct 
{
    uint8_t spi_number      :2;
    uint8_t pol             :1;
    uint8_t pha             :1;
    uint8_t prescale;
    uint8_t setup_time;
    uint8_t hold_time;
    uint8_t spi_mode        :1;
    uint8_t lsb_first       :1;
    uint8_t comm_mode       :2;
    uint8_t spi_size;
    uint8_t bits;
    uint8_t configure       :1;
}SPI_Config_t;

static int spi_shakti_transceive(const struct device *dev,
			  const struct spi_config *config,
			  const struct spi_buf_set *tx_bufs,
			  const struct spi_buf_set *rx_bufs)
{
  uint32_t len;
  volatile uint32_t temp = 0;

  // sspi_shakti_init(dev);

  while (1){
    temp =  sspi_instance[spi_number]->comm_status;
    temp = temp & SPI_BUSY;
    if (temp != SPI_BUSY){
        temp =0;
        break;
    }
  }

  if ((config->operation & 0x1) == 0)
  {
    printk("master is supported\n");
    master_mode = MASTER;
  }
  else
  {
    printk("Slave is not supported\n");
    return 1;
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
    return 0;
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
    return 0;
  }

  
  if((config -> operation & HALFDUPLEX) ==  HALFDUPLEX){
    printk("Half duplex \n");
    comm_mode = HALF_DUPLEX;
  }
  else if((config -> operation & FULLDUPLEX) == FULLDUPLEX){
    printk("Full duplex \n"); // default spi works in full duplex.
    comm_mode = FULL_DUPLEX;
  }
  else if((config -> operation & SIMPLEX_TX) == SIMPLEX_TX)
  {
    comm_mode = SIMPLEX_TX;
    printk("Simplex tx \n");
  }
  else if((config -> operation & SIMPLEX_TX) == SIMPLEX_TX)
  {
    comm_mode = SIMPLEX_RX;
    printk("Simplex rx \n");
  }
  else{
    printk("Mode is not supported \n");
    return 0;
  } 

  uint8_t rx_buff[16];
  struct spi_buf rx_loc_bufs = { .buf = rx_buff, .len = tx_bufs->buffers->len};
  struct spi_buf_set rx_loc_buffs = { .buffers = &rx_loc_bufs, .count = 1};
  if(rx_bufs == NULL){
    rx_bufs = &rx_loc_buffs;
  }

  if ((pol == 0 && pha == 1)||(pol == 1 && pha == 0)) return 1;

  k_mutex_lock(&(((struct spi_shakti_cfg*)(dev->config))->mutex),K_FOREVER);
  sspi_instance[spi_number]->clk_control = CLEAR_MASK;
  sspi_instance[spi_number]->clk_control = SPI_CLK_POLARITY(pol) | SPI_CLK_PHASE(pha) | SPI_PRESCALE(prescale) | SPI_SS2TX_DELAY(setup_time) | SPI_TX2SS_DELAY(hold_time);

  sspi_instance[spi_number]->comm_control = CLEAR_MASK;
  if (master_mode == MASTER)
  {
    sspi_instance[spi_number]->comm_control = SPI_MASTER(master_mode) | SPI_LSB_FIRST(lsb_first) | SPI_COMM_MODE(comm_mode) | SPI_TOTAL_BITS_TX(spi_size) | SPI_TOTAL_BITS_RX(spi_size) | SPI_OUT_EN_SCLK(1) | SPI_OUT_EN_NCS(1) | SPI_OUT_EN_MOSI(1) | SPI_OUT_EN_MISO(0);
  }
  else
  {
    sspi_instance[spi_number]->comm_control = SPI_MASTER(master_mode) | SPI_LSB_FIRST(lsb_first) | SPI_COMM_MODE(comm_mode) | SPI_TOTAL_BITS_TX(spi_size) | SPI_TOTAL_BITS_RX(spi_size) | SPI_OUT_EN_SCLK(0) | SPI_OUT_EN_NCS(0) | SPI_OUT_EN_MOSI(0) | SPI_OUT_EN_MISO(1);
  }
  
  if (tx_bufs == NULL && rx_bufs == NULL) return 1;
  spi_context_buffers_setup(&SPI_DATA(dev)->ctx, tx_bufs, rx_bufs, 1);
  len = tx_bufs ? tx_bufs->buffers->len : rx_bufs->buffers->len;

  if (comm_mode == SIMPLEX_TX)
  {
    for (int i = 0; i < len; i++)
    {
      if ((spi_size == DATA_SIZE_8) && ((sspi_instance[spi_number]->fifo_status & SPI_TX_FULL) != SPI_TX_FULL))
      {
        sspi_instance[spi_number]->data_tx.data_8 = ((uint8_t*)(tx_bufs->buffers->buf))[i];
        printk("tx_data= %d\n", sspi_instance[spi_number]->data_tx.data_8);
      }
      else if ((spi_size == DATA_SIZE_16) && (((sspi_instance[spi_number]->fifo_status & SPI_TX_30 == SPI_TX_30) && ((sspi_instance[spi_number]->comm_status & SPI_TX_FIFO(7)) == SPI_TX_FIFO(7))) || ((sspi_instance[spi_number]->comm_status & SPI_TX_FIFO(7)) < SPI_TX_FIFO(7))))
      {
        sspi_instance[spi_number]->data_tx.data_16 = ((uint16_t*)(tx_bufs->buffers->buf))[i];
        printk("tx_data= %d\n", sspi_instance[spi_number]->data_tx.data_16);
      }
      else if ((spi_size == DATA_SIZE_32) && (((sspi_instance[spi_number]->fifo_status & SPI_TX_28 == SPI_TX_28) && ((sspi_instance[spi_number]->comm_status & SPI_TX_FIFO(7)) == SPI_TX_FIFO(6))) || ((sspi_instance[spi_number]->comm_status & SPI_TX_FIFO(7)) < SPI_TX_FIFO(6))))
      {
        sspi_instance[spi_number]->data_tx.data_32 = ((uint32_t*)(tx_bufs->buffers->buf))[i];
        printk("tx_data= %d\n", sspi_instance[spi_number]->data_tx.data_32);
      }
      while (1)
      {
        temp = sspi_instance[spi_number]->comm_status & SPI_TX_EN;
        if (temp != SPI_TX_EN)
        {
          temp = 0;
          break;
        }        
      }
      if ((sspi_instance[spi_number]->fifo_status & SPI_TX_EMPTY) != SPI_TX_EMPTY)
      {
        while (1)
        {
          temp = sspi_instance[spi_number]->comm_status & SPI_BUSY;
          if (temp != SPI_BUSY)
          {
            temp = 0;
            break;
          }          
        }
        sspi_instance[spi_number]->comm_control |= SPI_ENABLE(ENABLE);
      }      
    }    
  }

  else if (comm_mode == SIMPLEX_RX)
  {
    for (int i = 0; i < len; i++)
    {
      if ((sspi_instance[spi_number]->comm_control & SPI_COMM_MODE(3)) == SPI_COMM_MODE(1))
      {
        while (1)
        {
          temp = sspi_instance[spi_number]->comm_status & SPI_BUSY;
          if (temp != SPI_BUSY)
          {
            temp = 0;
            break;
          }          
        }
        sspi_instance[spi_number]->comm_control |= SPI_ENABLE(ENABLE);
      }
      if (spi_size == DATA_SIZE_8)
      {
        while (1)
        {
          temp = sspi_instance[spi_number]->fifo_status & SPI_RX_EMPTY;
          if (temp != SPI_RX_EMPTY)
          {
            temp = 0;
            break;
          }
        }
        ((uint8_t*)(rx_bufs->buffers->buf))[i] = sspi_instance[spi_number]->data_rx.data_8;
        printk("rx_data= %d\n", ((uint8_t*)(rx_bufs->buffers->buf))[i]);
      }
      else if (spi_size == DATA_SIZE_16)
      {
        while (1)
        {
          temp = sspi_instance[spi_number]->comm_status & SPI_RX_FIFO(7);
          if (temp >= SPI_RX_FIFO(1))
          {
            temp = 0;
            break;
          }          
        }
        ((uint16_t*)(rx_bufs->buffers->buf))[i] = sspi_instance[spi_number]->data_rx.data_16;
        printk("rx_data= %d\n", ((uint16_t*)(rx_bufs->buffers->buf))[i]);
      }
      else if (spi_size == DATA_SIZE_32)
      {
        while (1)
        {
          temp = sspi_instance[spi_number]->comm_status & SPI_RX_FIFO(7);
          if (temp >= SPI_RX_FIFO(2))
          {
            temp = 0;
            break;
          }          
        }
        ((uint32_t*)(rx_bufs->buffers->buf))[i] = sspi_instance[spi_number]->data_rx.data_32;
        printk("rx_data= %d\n", ((uint32_t*)(rx_bufs->buffers->buf))[i]);
      }
    } 
  }
  
  else if (comm_mode == FULL_DUPLEX || comm_mode == HALF_DUPLEX)
  {
    printk("mode = %d\n", comm_mode);
    for (int i = 0; i < len; i++)
    {
      if ((spi_size == DATA_SIZE_8) && ((sspi_instance[spi_number]->fifo_status & SPI_TX_FULL) != SPI_TX_FULL))
      {
        // k_busy_wait(1000);
        // printk("tx_data= %d\n", sspi_instance[spi_number]->data_tx.data_8);
        sspi_instance[spi_number]->data_tx.data_8 = ((uint8_t*)(tx_bufs->buffers->buf))[i];
        // k_busy_wait(1000);
      }
      else if ((spi_size == DATA_SIZE_16) && (((sspi_instance[spi_number]->fifo_status & SPI_TX_30 == SPI_TX_30) && ((sspi_instance[spi_number]->comm_status & SPI_TX_FIFO(7)) == SPI_TX_FIFO(7))) || ((sspi_instance[spi_number]->comm_status & SPI_TX_FIFO(7)) < SPI_TX_FIFO(7))))
      {
        // k_busy_wait(1000);
        printk("tx_data= %d\n", sspi_instance[spi_number]->data_tx.data_16);
        sspi_instance[spi_number]->data_tx.data_16 = ((uint16_t*)(tx_bufs->buffers->buf))[i];
        // k_busy_wait(1000);
      }
      else if ((spi_size == DATA_SIZE_32) && (((sspi_instance[spi_number]->fifo_status & SPI_TX_28 == SPI_TX_28) && ((sspi_instance[spi_number]->comm_status & SPI_TX_FIFO(7)) == SPI_TX_FIFO(6))) || ((sspi_instance[spi_number]->comm_status & SPI_TX_FIFO(7)) < SPI_TX_FIFO(6))))
      {
        // k_busy_wait(1000);
        printk("tx_data= %d\n", sspi_instance[spi_number]->data_tx.data_32);
        sspi_instance[spi_number]->data_tx.data_16 = ((uint16_t*)(tx_bufs->buffers->buf))[i];
        // k_busy_wait(1000);
      }
      while (1)
      {
        temp = sspi_instance[spi_number]->comm_status & SPI_TX_EN;
        if (temp != SPI_TX_EN)
        {
          temp = 0;
          break;
        }        
      }
      if(((sspi_instance[spi_number]->fifo_status & SPI_TX_EMPTY) != SPI_TX_EMPTY) || ((sspi_instance[spi_number]->comm_control & SPI_COMM_MODE(3)) == SPI_COMM_MODE(1)))
      {
        while (1)
        {
          temp = sspi_instance[spi_number]->comm_status & SPI_BUSY;
          if (temp != SPI_BUSY)
          {
            temp = 0;
            break;
          }          
        }
        sspi_instance[spi_number]->comm_control |= SPI_ENABLE(ENABLE);
      }
      if (spi_size == DATA_SIZE_8)
      {
        while (1)
        {
          temp = sspi_instance[spi_number]->fifo_status & SPI_RX_EMPTY;
          if (temp != SPI_RX_EMPTY)
          {
            temp = 0;
            break;
          }
        }        
        // k_busy_wait(1000);
        // printk("rx_data= %d\n", ((uint8_t*)(rx_bufs->buffers->buf))[i]);
        ((uint8_t*)(rx_bufs->buffers->buf))[i] = sspi_instance[spi_number]->data_rx.data_8;
        // k_busy_wait(1000);
      }
      else if (spi_size == DATA_SIZE_16)
      {
        while (1)
        {
          temp = sspi_instance[spi_number]-> comm_status & SPI_RX_FIFO(7);
          if (temp >= SPI_RX_FIFO(1))
          {
            temp = 0;
            break;
          }          
        }
        // k_busy_wait(1000);
        printk("rx_data= %d\n", ((uint8_t*)(rx_bufs->buffers->buf))[i]);
        ((uint16_t*)(rx_bufs->buffers->buf))[i] = sspi_instance[spi_number]->data_rx.data_16;
        // k_busy_wait(1000);
      }
      else if (spi_size == DATA_SIZE_32)
      {
        while (1)
        {
          temp = sspi_instance[spi_number]-> comm_status & SPI_RX_FIFO(7);
          if (temp >= SPI_RX_FIFO(2))
          {
            temp = 0;
            break;
          }          
        }
        // k_busy_wait(1000);
        printk("rx_data= %d\n", ((uint8_t*)(rx_bufs->buffers->buf))[i]);
        ((uint32_t*)(rx_bufs->buffers->buf))[i] = sspi_instance[spi_number]->data_rx.data_32;
        // k_busy_wait(1000);
      }
    }    
  }
  k_mutex_unlock(&(((struct spi_shakti_cfg*)(dev->config))->mutex));
  return 0;  
}

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