#define DT_DRV_COMPAT shakti_spi0

#include <soc.h>
#include <stdbool.h>

#include "spi_shakti.h"

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

sspi_struct *sspi_instance[SSPI_MAX_COUNT];

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


void sspi_busy(const struct device *dev, int spi_number){
  uint16_t temp; 

  while(1){
    temp = sspi_instance[spi_number]->comm_status;
    temp =  temp & SPI_BUSY;
    #ifdef SPI_DEBUG
    printf("\nchecking SPI%d busy", spi_number);
    #endif
    if (temp == SPI_BUSY){
      printf("\nSPI%d is busy", spi_number);
    }
    else {
      break;
    }
  }
  #ifdef SPI_DEBUG
  printf("\nSPI%d is not busy", spi_number);
  #endif

}


int sspi_init(const struct device *dev, int spi_number)
{ 
  int spi_base;
  if (spi_number < SSPI_MAX_COUNT & spi_number >= 0){
    sspi_instance[spi_number] = (sspi_struct*) ( (SSPI0_BASE_ADDRESS + ( spi_number * SSPI_BASE_OFFSET) ) );
    spi_base = sspi_instance[spi_number];
    #ifdef SPI_DEBUG
    printf("\nSPI%d Initialized..", spi_number);
    #endif
    return spi_base;
  }
  else{
    printf("\nInvalid SPI instance %d. This SoC supports only SPI-0 to SPI-3", spi_number);
    return -1;
  }
}


void sclk_config(const struct device *dev, int spi_number, int pol, int pha, int prescale, int setup_time, int hold_time)
{

  sspi_busy(spi_number);

  if ((pol == 0 && pha == 1)||(pol == 1 && pha == 0)){
    printf("\nInvalid Clock Configuration (NO FALLING EDGE). Hence Exiting..!");
    exit(-1);
  }
  else{
    sspi_instance[spi_number] -> clk_control =  SPI_TX2SS_DELAY(hold_time) | SPI_SS2TX_DELAY(setup_time) | SPI_PRESCALE(prescale)| SPI_CLK_POLARITY(pol) | SPI_CLK_PHASE(pha);
  }
}


void sspi_configure_clock_in_hz(const struct device *dev, int spi_number, uint32_t bit_rate)
{
    uint8_t prescaler = (CLOCK_FREQUENCY / bit_rate) - 1;

    if(bit_rate >= CLOCK_FREQUENCY){
        printf("\n Invalid bit rate value. Bit rate should be less than CLOCK_FREQUENCY");
		    return;
    }

    uint32_t temp = sspi_instance[spi_number] -> clk_control & (~SPI_PRESCALE(0xFF) );
    sspi_instance[spi_number] -> clk_control = temp | SPI_PRESCALE(prescaler);
}


void sspi_comm_control_config(const struct device *dev, int spi_number, int master_mode, int lsb_first, int comm_mode, int spi_size){

  int out_en=0;

  if (master_mode==MASTER){
    out_en = SPI_OUT_EN_SCLK | SPI_OUT_EN_NCS | SPI_OUT_EN_MOSI ;
    }
  else{
    out_en = SPI_OUT_EN_MISO;
  }
  sspi_busy(spi_number);
  sspi_instance[spi_number]->comm_control = \
        SPI_MASTER(master_mode) | SPI_LSB_FIRST(lsb_first) | SPI_COMM_MODE(comm_mode) | \
        SPI_TOTAL_BITS_TX(spi_size) | SPI_TOTAL_BITS_RX(spi_size) | \
        out_en;
}


void sspi_enable(const struct device *dev, int spi_number){

  uint32_t temp = sspi_instance[spi_number]->comm_control;
  sspi_busy(spi_number);
  sspi_instance[spi_number]->comm_control = temp | SPI_ENABLE(ENABLE);
}


void sspi_disable(const struct device *dev, int spi_number){

  uint32_t temp = sspi_instance[spi_number]->comm_control;
  uint32_t disable = ~SPI_ENABLE(ENABLE);
  //sspi_busy(spi_number);
  sspi_instance[spi_number]->comm_control = 0;// temp & disable; //| SPI_ENABLE(DISABLE);
}


int sspi8_transmit_data(const struct device *dev, int spi_number, uint8_t tx_data[FIFO_DEPTH_8], uint8_t data_size){

  int no_of_itr = data_size;
  for (int i=0; i < no_of_itr; i++){
    int temp = sspi_check_tx_fifo_32(spi_number);
    if(temp == 0){
      sspi_instance[spi_number]->data_tx.data_8 = tx_data[i];
      #ifdef SPI_DEBUG
      printf("tx_data[%d] = %d\n", i, tx_data[i]);
      #endif
    }
    else{
      #ifdef SPI_DEBUG
      printf("\nTX FIFO is full");
      #endif
      return -1;
    }
  }
  sspi_wait_till_tx_complete(spi_number);
  return 0;
}


int sspi16_transmit_data(const struct device *dev, int spi_number, uint16_t tx_data[FIFO_DEPTH_16], uint8_t data_size){

  int no_of_itr = data_size;
  for (int i=0; i < no_of_itr; i++){
    int temp = sspi_check_tx_fifo_30(spi_number);
    if (temp == 0){
      sspi_instance[spi_number]->data_tx.data_16 = tx_data[i];
      #ifdef SPI_DEBUG
      printf("tx_data[%d] = %d\n", i, tx_data[i]);
      #endif
    }
    else{
      #ifdef SPI_DEBUG
      printf("\nTX FIFO has not enough space.");
      #endif
      return -1;
    }
  }
  sspi_wait_till_tx_complete(spi_number);
  return 0;
}

int sspi32_transmit_data(const struct device *dev, int spi_number, uint32_t tx_data[FIFO_DEPTH_32], uint8_t data_size){

  int no_of_itr = data_size;
  for (int i=0; i < no_of_itr; i++){
    int temp = sspi_check_tx_fifo_28(spi_number);
    if (temp == 0){
      sspi_instance[spi_number]->data_tx.data_32 = tx_data[i];
      #ifdef SPI_DEBUG
      printf("tx_data[%d] = %d\n", i, tx_data[i]);
      #endif
      }
    else{
      #ifdef SPI_DEBUG
      printf("\nTX FIFO has not enough space.");
      #endif
      return -1;
      }
  }
  sspi_wait_till_tx_complete(spi_number);
  return 0;
}


void sspi_wait_till_tx_not_en(const struct device *dev, int spi_number){
  uint16_t temp;

  while(1){
    temp = sspi_instance[spi_number]->comm_status;
    temp = temp & SPI_TX_EN;
    if (temp == 0){
      break;
    }
  }
}


void sspi_wait_till_tx_complete(const struct device *dev, int spi_number){

    uint32_t temp;
    int i = 0;
    #ifdef SPI_DEBUG
    printf("\nChecking TX FIFO is empty.");
    #endif
    while(1){
      sspi_wait_till_tx_not_en(spi_number); 
      temp = sspi_instance[spi_number]->fifo_status;
      temp = temp & SPI_TX_EMPTY; 
      if (temp == 0){
        sspi_enable(spi_number);
        if (i == 0){
          #ifdef SPI_DEBUG
          printf("\nTX FIFO is not empty, waiting till the FIFO gets empty.");
          #endif
        }
        else{
          #ifdef SPI_DEBUG
          printf("\nTX FIFO is not empty");
          #endif
        }
        i += 1;
      }
      else{
        break;
      }
    }
    #ifdef SPI_DEBUG
    printf("\nTX FIFO is empty");
    #endif
}

int sspi_check_tx_fifo_32(const struct device *dev, int spi_number){

    uint32_t temp;
    #ifdef SPI_DEBUG
    printf("\nChecking if TX FIFO is full.");
    #endif

    temp = sspi_instance[spi_number]->fifo_status;
    // printf("\ntemp : %x", temp);
    temp = temp & SPI_TX_FULL;

    if (temp == SPI_TX_FULL){
      #ifdef SPI_DEBUG
      printf("\nTX FIFO is full, waiting till fifo gets not full.");
      #endif
      return 1;
    }
    else{
      #ifdef SPI_DEBUG
      printf("\nTX FIFO is not full.");
      #endif
      return 0;
    }
}


int sspi_check_tx_fifo_30(const struct device *dev, int spi_number){

    uint32_t temp;
    uint16_t temp1;
    #ifdef SPI_DEBUG
    printf("\nChecking if TX FIFO is less than 30.");
    #endif

    temp = sspi_instance[spi_number]->fifo_status;
    temp = temp & SPI_TX_30;
    temp1 = sspi_instance[spi_number]->comm_status;
    temp1 = temp1 & SPI_TX_FIFO(7);

    if ((temp == SPI_TX_30 && temp1 == SPI_TX_FIFO(7)) || (temp1 < SPI_TX_FIFO(7))){
      #ifdef SPI_DEBUG
      printf("\nTX FIFO is not 30.");
      #endif
      return 0;
    }
    else{
      #ifdef SPI_DEBUG
      printf("\nTX FIFO contains less than or equal to 30.");
      #endif
      return 1;
    }
}


int sspi_check_tx_fifo_28(const struct device *dev, int spi_number){

    uint32_t temp;
    uint16_t temp1;

    #ifdef SPI_DEBUG
    printf("\nChecking if TX FIFO is less than 28.");
    #endif

    temp = sspi_instance[spi_number]->fifo_status;
    temp = temp & SPI_TX_28;
    temp1 = sspi_instance[spi_number]->comm_status;
    temp1 = temp1 & SPI_TX_FIFO(7);

    if ((temp == SPI_TX_28 && temp1 == SPI_TX_FIFO(6)) || (temp1 < SPI_TX_FIFO(6))){
      #ifdef SPI_DEBUG
      printf("\nTX FIFO is not 28.");
      #endif
      return 0;
    }
    else{
      #ifdef SPI_DEBUG
      printf("\nTX FIFO contains less than or equal to 28.");
      #endif
      return 1;
    }
}


void sspi_wait_till_rxfifo_not_empty(const struct device *dev, int spi_number){
    uint32_t temp;

    while(1){
      temp = sspi_instance[spi_number]->fifo_status;
      temp = temp & SPI_RX_EMPTY;
      if (temp == 0){
        #ifdef SPI_DEBUG
        printf("\nRX FIFO is not empty.");
        #endif
        break;
      }
    }
}


void sspi_wait_till_rxfifo_2(const struct device *dev, int spi_number){

    uint32_t temp;
    uint16_t temp1;

    while(1){
      temp = sspi_instance[spi_number]->fifo_status;
      temp = temp & SPI_RX_DUAL;

      temp1 = sspi_instance[spi_number]->comm_status;
      temp1 = temp1 & SPI_RX_FIFO(7);

      // if ((temp == SPI_RX_DUAL && temp1 == SPI_RX_FIFO(1)) || (temp1 > SPI_RX_FIFO(0))){
      //   #ifdef SPI_DEBUG
      //   printf("\nRX FIFO is not empty.");
      //   #endif
      //   break;
      // }

      if((temp1 >= SPI_RX_FIFO(1))){
        #ifdef SPI_DEBUG
        printf("\nRX FIFO is more than or equal to 2 bytes.");
        #endif
        break;
      }
    }
}



void sspi_wait_till_rxfifo_4(const struct device *dev, int spi_number){

    uint32_t temp;
    uint16_t temp1;

    while(1){
      temp = sspi_instance[spi_number]->fifo_status;
      temp = temp & SPI_RX_QUAD;

      temp1 = sspi_instance[spi_number]->comm_status;
      temp1 = temp1 & SPI_RX_FIFO(7);

      // if ((temp == SPI_RX_QUAD && temp1 == SPI_RX_FIFO(2)) || (temp1 > SPI_RX_FIFO(1))){
      //   #ifdef SPI_DEBUG
      //   printf("\nRX FIFO is not empty.");
      //   #endif
      //   break;
      // }
      if (temp1 >= SPI_RX_FIFO(2)){
        #ifdef SPI_DEBUG
        printf("\nRX FIFO is more than or equal to 4 bytes.");
        #endif
        break;
      }
    }
}



void sspi8_receive_data(const struct device *dev, int spi_number, uint8_t rx_data[FIFO_DEPTH_8], uint8_t data_size){

    int no_of_itr = data_size;
    uint32_t temp;
    temp = sspi_instance[spi_number]->comm_control;
    temp = temp & SPI_COMM_MODE(3);

    for (int i=0; i < no_of_itr; i++){
      if (temp == SPI_COMM_MODE(1)){
        sspi_enable(spi_number);
      }
      sspi_wait_till_rxfifo_not_empty(spi_number);
      rx_data[i] = sspi_instance[spi_number]->data_rx.data_8;
    }
}



void sspi16_receive_data(const struct device *dev, int spi_number, uint16_t rx_data[FIFO_DEPTH_16], uint8_t data_size){

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



void sspi32_receive_data(const struct device *dev, int spi_number, uint32_t rx_data[FIFO_DEPTH_32], uint8_t data_size){

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
