/*
 * Copyright (c) 2017 Google LLC.
 * Copyright (c) 2018 qianfan Zhao.
 * Copyright (c) 2023 Gerson Fernando Budke.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT shakti_spi0

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_shakti);

#include <soc.h>
#include <stdbool.h>

#include <zephyr/sys/util.h>
#include<zephyr/kernel.h>

#include <spi_shakti.h>


/*HELPER FUNCTIONS*/

sspi_struct *sspi_instance[SSPI_MAX_COUNT];

int sclk_config[];
int comm_config[];

int sspi_shakti_init(const struct device *dev, int spi_number)
{ 
  int spi_base;
  if (spi_number < SSPI_MAX_COUNT & spi_number >= 0){
    sspi_instance[spi_number] = (sspi_struct*) ( (SSPI0_BASE_ADDRESS + ( spi_number * SSPI_BASE_OFFSET) ) );
    spi_base = sspi_instance[spi_number];
    #ifdef SPI_DEBUG
    printk("\nSPI%d Initialized..", spi_number);
    #endif
    return spi_base;
  }
  else{
    printk("\nInvalid SPI instance %d. This SoC supports only SPI-0 to SPI-3", spi_number);
    return -1;
  }
}


void sspi_shakti_busy(const struct device *dev, int spi_number)
{
  uint16_t temp; 
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


void sclk_shakti_config(const struct device *dev, int spi_number, int pol, int pha, int prescale, int setup_time, int hold_time)
{

  sspi_shakti_busy(spi_number);

  if ((pol == 0 && pha == 1)||(pol == 1 && pha == 0)){
    printk("\nInvalid Clock Configuration (NO FALLING EDGE). Hence Exiting..!");
    exit(-1);
  }
  else{
    sspi_instance[spi_number] -> clk_control =  SPI_TX2SS_DELAY(hold_time) | SPI_SS2TX_DELAY(setup_time) | SPI_PRESCALE(prescale)| SPI_CLK_POLARITY(pol) | SPI_CLK_PHASE(pha);
  }
}


void sspi_shakti_configure_clock_in_hz(const struct device *dev, int spi_number, uint32_t bit_rate)
{
    uint8_t prescaler = (CLOCK_FREQUENCY / bit_rate) - 1;

    if(bit_rate >= CLOCK_FREQUENCY){
        printk("\n Invalid bit rate value. Bit rate should be less than CLOCK_FREQUENCY");
		    return;
    }

    uint32_t temp = sspi_instance[spi_number] -> clk_control & (~SPI_PRESCALE(0xFF) );
    sspi_instance[spi_number] -> clk_control = temp | SPI_PRESCALE(prescaler);
}


void sspi_shakti_comm_control_config(const struct device *dev, int spi_number, int master_mode, int lsb_first, int comm_mode, int spi_size){

  int out_en=0;

  if (master_mode==MASTER){
    out_en = SPI_OUT_EN_SCLK | SPI_OUT_EN_NCS | SPI_OUT_EN_MOSI ;
    }
  else{
    out_en = SPI_OUT_EN_MISO;
  }
  sspi_shakti_busy(spi_number);
  sspi_instance[spi_number]->comm_control = \
        SPI_MASTER(master_mode) | SPI_LSB_FIRST(lsb_first) | SPI_COMM_MODE(comm_mode) | \
        SPI_TOTAL_BITS_TX(spi_size) | SPI_TOTAL_BITS_RX(spi_size) | \
        out_en;
}


void sspi_shakti_enable(const struct device *dev, int spi_number){

  uint32_t temp = sspi_instance[spi_number]->comm_control;
  sspi_shakti_busy(spi_number);
  sspi_instance[spi_number]->comm_control = temp | SPI_ENABLE(ENABLE);
}


void sspi_shakti_disable(const struct device *dev, int spi_number){

  uint32_t temp = sspi_instance[spi_number]->comm_control;
  uint32_t disable = ~SPI_ENABLE(ENABLE);
  //sspi_busy(spi_number);
  sspi_instance[spi_number]->comm_control = 0;// temp & disable; //| SPI_ENABLE(DISABLE);
}


// int sspi_shakti_transmit_data(const struct device *dev, int spi_number, )

int sspi8_shakti_transmit_data(const struct device *dev, int spi_number, uint8_t *tx_data, uint8_t data){

  for (int i = 0; i<data; i++){
    int temp = sspi_shakti_check_tx_fifo_32(spi_number);
    if(temp == 0){
      sspi_instance[spi_number]->data_tx.data_8 = *(tx_data + i);
      #ifdef SPI_DEBUG
      printk("tx_data[%d] = %d\n", i, tx_data[i]);
      #endif
    }
    else{
      #ifdef SPI_DEBUG
      printk("\nTX FIFO is full");
      #endif
      return -1;
    }
  }
  sspi_shakti_wait_till_tx_complete(spi_number);
  return 0;
}


int sspi16_shakti_transmit_data(const struct device *dev, int spi_number, uint16_t * tx_data, uint8_t data){

  for(int i = 0; i<data; i++){
    int temp = sspi_shakti_check_tx_fifo_30(spi_number);
    if (temp == 0){
      sspi_instance[spi_number]->data_tx.data_16 = *(tx_data + i);
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
  
  sspi_wait_till_tx_complete(spi_number);
  return 0;
}

int sspi32_shakti_transmit_data(const struct device *dev, int spi_number, uint32_t * tx_data, uint8_t data){

  for( int i = 0; i<data; i++){
    int temp = sspi_check_tx_fifo_28(spi_number);
    if (temp == 0){
      sspi_instance[spi_number]->data_tx.data_32 = *(tx_data + i);
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
  sspi_wait_till_tx_complete(spi_number);
  return 0;
}


void sspi_shakti_wait_till_tx_not_en(const struct device *dev, int spi_number){
  uint16_t temp;

  while(1){
    temp = sspi_instance[spi_number]->comm_status;
    temp = temp & SPI_TX_EN;
    if (temp == 0){
      break;
    }
  }
}


void sspi_shakti_wait_till_tx_complete(const struct device *dev, int spi_number){

    uint32_t temp;
    int i = 0;
    #ifdef SPI_DEBUG
    printk("\nChecking TX FIFO is empty.");
    #endif
    while(1){
      sspi_shakti_wait_till_tx_not_en(spi_number); 
      temp = sspi_instance[spi_number]->fifo_status;
      temp = temp & SPI_TX_EMPTY; 
      if (temp == 0){
        sspi_enable(spi_number);
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

int sspi_shakti_check_tx_fifo_32(const struct device *dev, int spi_number){

    uint32_t temp;
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


int sspi_shakti_check_tx_fifo_30(const struct device *dev, int spi_number){

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


int sspi_shakti_check_tx_fifo_28(const struct device *dev, int spi_number){

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


void sspi_shakti_wait_till_rxfifo_not_empty(const struct device *dev, int spi_number){
    uint32_t temp;

    while(1){
      temp = sspi_instance[spi_number]->fifo_status;
      temp = temp & SPI_RX_EMPTY;
      if (temp == 0){
        #ifdef SPI_DEBUG
        printk("\nRX FIFO is not empty.");
        #endif
        break;
      }
    }
}


void sspi_shakti_wait_till_rxfifo_2(const struct device *dev, int spi_number){

    uint32_t temp;
    uint16_t temp1;

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



void sspi_shakti_wait_till_rxfifo_4(const struct device *dev, int spi_number){

    uint32_t temp;
    uint16_t temp1;

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



void sspi8_shakti_receive_data(const struct device *dev, int spi_number, uint8_t rx_data[FIFO_DEPTH_8],/*uint8_t data_size*/){

    // int no_of_itr = data_size;
    uint32_t temp;
    temp = sspi_instance[spi_number]->comm_control;
    temp = temp & SPI_COMM_MODE(3);

    if (temp == SPI_COMM_MODE(1)){
      sspi_enable(spi_number);
    }
    sspi_shakti_wait_till_rxfifo_not_empty(spi_number);
    rx_data= sspi_instance[spi_number]->data_rx.data_8;
}



void sspi16_shakti_receive_data(const struct device *dev, int spi_number, uint16_t rx_data[FIFO_DEPTH_16] /*,uint8_t data_size*/){

    int no_of_itr = data_size;
    uint32_t temp;
    temp = sspi_instance[spi_number]->comm_control;
    temp = temp & SPI_COMM_MODE(3);

    for (int i=0; i < no_of_itr; i++){
      if (temp == SPI_COMM_MODE(1)){
        sspi_enable(spi_number);
      }
      sspi_wait_till_rxfifo_2(spi_number);
      rx_data[i] = sspi_instance[spi_number]->data_rx.data_16;
    }
}



void sspi32_shakti_receive_data(const struct device *dev, int spi_number, uint32_t rx_data[FIFO_DEPTH_32],/*uint8_t data_size*/ ){

    int no_of_itr = data_size;
    uint32_t temp;
    temp = sspi_instance[spi_number]->comm_control;
    temp = temp & SPI_COMM_MODE(3);

    for (int i=0; i < no_of_itr; i++){
      if (temp == SPI_COMM_MODE(1)){
        sspi_enable(spi_number);
      }
      sspi_wait_till_rxfifo_4(spi_number);
      rx_data[i] = sspi_instance[spi_number]->data_rx.data_32;
    }
}


void qualify(const struct device *dev, int spi_number){
  sspi_instance[spi_number]->qual = 3;
}


void inter_enable_config(const struct device *dev, int spi_number, uint32_t value){

  sspi_instance[spi_number]->intr_en = value;
}


/* API Functions */

static int spi_shakti_transceive(const struct device *dev,
			  const struct spi_config *config,
			  const struct spi_buf_set *tx_bufs,
			  const struct spi_buf_set *rx_bufs)
        
{
  spi_context_lock(&SPI_DATA(dev)->ctx, false, NULL, NULL, config);
  
  sclk_config[] = DT_PROP(DT_NODELABEL(spi0), sclk_configure);
  comm_config[] = DT_PROP(DT_NODELABEL(spi0), comm_configure);

  int spi_number;
  int pol = sclk_config[0];
  int pha = sclk_config[1];
  int prescale = sclk_config[2];
  int setup_time = sclk_config[3];
  int hold_time = sclk_config[4];

  int master_mode = comm_config[0];
  int lsb_first = comm_config[1];
  int comm_mode = comm_config[2];
  int spi_size = comm_config[3];

  printk("pol %d\n", pol);
  printk("pha %d\n", pha);
  printk("prescale %d\n", prescale);
  printk("setup_time %d\n", setup_time);
  printk("hold_time %d\n", hold_time);
  printk("master_mode %d\n", master_mode);
  printk("lsb_first %d\n", lsb_first);
  printk("comm_mode %d\n", comm_mode);
  printk("spi_size %d\n", spi_size);

  sspi_shakti_init(*dev, spi_number);
  sclk_shakti_config(*dev, spi_number, pol, pha, prescale, setup_time, hold_time);
  
  if(comm_config[2] == SIMPLEX_TX)
  {
    sspi_shakti_comm_control_config(*dev, spi_number, master_mode, lsb_first, comm_mode, spi_size);
    spi_context_buffers_setup(&SPI_DATA(dev)->ctx, tx_bufs, NULL, 1);

    if (comm_config[3] == DATA_SIZE_8)
    {
      sspi8_shakti_transmit_data(*dev, spi_number, *tx_bufs, sizeof(tx_bufs)/sizeof(tx_bufs[0]));
    }

    else if (comm_config[3] == DATA_SIZE_16)
    {
      sspi16_shakti_transmit_data(*dev, spi_number, *tx_bufs, sizeof(tx_bufs)/sizeof(tx_bufs[0]));
    }

    else if (comm_config[3] == DATA_SIZE_32)
    {
      sspi32_shakti_transmit_data(*dev, spi_number, *tx_bufs, sizeof(tx_bufs)/sizeof(tx_bufs[0]));
    }
  }

  if (comm_config[2] == SIMPLEX_RX)
  {
    sspi_shakti_comm_control_config(*dev, spi_number, master_mode, lsb_first, comm_mode, spi_size);
    spi_context_buffers_setup(&SPI_DATA(dev)->ctx, NULL, rx_bufs, 1);

    if (comm_config == DATA_SIZE_8)
    {
      sspi8_shakti_receive_data(*dev, spi_number, *rx_bufs);
    }

    if (comm_config == DATA_SIZE_16)
    {
      sspi16_shakti_receive_data(*dev, spi_number, *rx_bufs);
    }

    if (comm_config == DATA_SIZE_32)
    {
      sspi32_shakti_receive_data(*dev, spi_number, *rx_bufs);
    }
    

  }
  
}



static int spi_shakti_release(const struct device *dev,
		       const struct spi_config *config)
{
	spi_context_unlock_unconditionally(&SPI_DATA(dev)->ctx);
	return 0;
}


static struct spi_driver_api spi_shakti_api = {
	.transceive = spi_shakti_transceive,
	.release = spi_shakti_release,
};


	static struct spi_shakti_data spi_shakti_data_0 = { \
		SPI_CONTEXT_INIT_LOCK(spi_shakti_data_##n, ctx), \
		SPI_CONTEXT_INIT_SYNC(spi_shakti_data_##n, ctx), \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)	\
	}; \

static const struct spi_shakti_cfg spi_shakti_cfg_0 = {
  .base = SPI0_START , \
  .f_sys = CLOCK_FREQUENCY,/
};


DEVICE_DT_INST_DEFINE(n, \
			spi_shakti_init, \
			NULL, \
			&spi_shakti_data_0, \
			&spi_shakti_cfg_0, \
			POST_KERNEL, \
			CONFIG_SPI_INIT_PRIORITY, \
			&spi_shakti_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_INIT)
