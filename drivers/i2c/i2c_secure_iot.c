/**
 * Project                           : Secure IoT SoC
 * Name of the file                  : i2c_v2_driver.c
 * Brief Description of file         : This is a Baremetal I2C Driver file for Mindgrove Silicon's I2C Peripheral.
 * Name of Author                    : Vishwajith.N.S 
 * Email ID                          : vishwajithns6@gmail.com
 * 
 * @file i2c_v2_driver.c
 * @author Vishwajith .N.S (vishwajithns6@gmail.com)
 * @brief This is a Baremetal I2C Driver file for Mindgrove Silicon's I2C Peripheral.
 * @version 0.2
 * @date 2023-07-08
 * 
 * @copyright Copyright (c) Mindgrove Technologies Pvt. Ltd 2023. All rights reserved.
 * 
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#define DT_DRV_COMPAT shakti_i2c0
#include <stdint.h>
#include <stdlib.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/sys_io.h>
/**baremetal driver defines*/
#define I2C_PIN 0x80
#define I2C_ESO 0x40
#define I2C_ES1 0x20
#define I2C_ES2 0x10
#define I2C_ENI 0x08
#define I2C_STA 0x04
#define I2C_STO 0x02
#define I2C_ACK 0x01

#define I2C_INI 0x40   
#define I2C_STS 0x20
#define I2C_BER 0x10
#define I2C_AD0 0x08
#define I2C_LRB 0x08
#define I2C_AAS 0x04
#define I2C_LAB 0x02
#define I2C_BB  0x01

#define I2C_START         (I2C_PIN | I2C_ESO | I2C_STA | I2C_ACK)
#define I2C_STOP          (I2C_PIN | I2C_ESO | I2C_STO | I2C_ACK)
#define I2C_REPSTART      (                 I2C_ESO | I2C_STA | I2C_ACK)
#define I2C_IDLE          (I2C_ESO                  | I2C_ACK)
#define I2C_NACK          (I2C_ESO)
#define I2C_DISABLE       (I2C_PIN|I2C_ACK)


#define I2C_READ 1
#define I2C_WRITE 0
#define MAX_I2C_COUNT 2
#define DELAY 17
#define DELAY_FREQ_BASE 40000000

/**baremetal driver defines*/


/*******Zephyr defines*********/
#define I2C_STANDARD_MODE 100000
#define I2C_FAST_MODE 400000
#define I2C_PRESCALE 0x00
#define I2C_CONTROL  0x08
#define I2C_DATA     0x10
#define I2C_STATUS   0x18
#define I2C_SCL_DIV  0x38
#define WRITE_TO_REG(confg,offset,value)   *((uint32_t*)((((struct i2c_seciot_cfg*)(confg))->base)+offset)) = value
#define READ_REG(confg,offset) *((uint32_t*)((((struct i2c_seciot_cfg*)(confg))->base)+offset))
#define READ_REG_8BIT(confg,offset) *((uint8_t*)((((struct i2c_seciot_cfg*)(confg))->base)+offset))
/* Struct to access I2C registers as 32 bit registers */

typedef struct
{
/* 0x00 */
    uint8_t  prescale;          //prescalar register 8 bit
    uint8_t prescale_rsvd1;
    uint16_t prescale_rsvd2;
    uint32_t   prescale_rsvd3;//reserved 

/* 0x08 */
    union{
        struct{
            uint8_t ack :1;
            uint8_t sto :1;
            uint8_t sta :1;
            uint8_t eni :1;
            uint8_t es2 :1;
            uint8_t es1 :1;
            uint8_t eso :1;
            uint8_t pin_control :1;
        } __attribute__((__packed__));
        uint8_t control;
    };          //control register 8 bit
    uint8_t control_rsvd1;
    uint16_t control_rsvd2;
    uint32_t   control_rsvd3;//reserved

/* 0x10 */
    uint8_t  data;              //data register 8 bit
    uint8_t data_rsvd1;
    uint16_t data_rsvd2;
    uint32_t   data_rsvd3;   //reserved

/* 0x18 */
    union{


        struct{
            uint8_t nBB :1;
            uint8_t LAB:1;
            uint8_t aas :1;
            uint8_t lrb :1;
            uint8_t ber :1;
            uint8_t sts :1;
            uint8_t zero :1;
            uint8_t pin_status :1;
        } __attribute__((__packed__));
        uint8_t  status;//status register 8 bit
        };         
        uint8_t   status_rsvd1; //reserved
        uint16_t   status_rsvd2; //reserved
        uint32_t   status_rsvd3; //reserved

/* 0x20 */
    uint32_t  s01;           //reserved
    uint32_t   s01_rsvd;     //reserved

/* 0x28 */
    uint32_t  s3;            //reserved
    uint32_t   s3_rsvd;      //reserved

/* 0x30 */
    uint32_t  time;         //reserved
    uint32_t   time_rsvd;   //reserved

/* 0x38 */
    uint32_t  scl;          //scl divider register 32 bit
    uint32_t   scl_rsvd;    //reserved
} i2c_struct __attribute__((__packed__));


i2c_struct* i2c_instance[MAX_I2C_COUNT];



struct i2c_seciot_cfg{
    uint32_t base;
    uint32_t scl_clk;
    uint32_t sys_clk;
   struct k_mutex mutex;
};


static void i2c_start_bit_(const struct device *dev);
static void i2c_end(const struct device *dev);
static void wait_till_txrx_operation_Completes_(const struct device *dev);
static void wait_till_I2c_bus_free_(const struct device *dev);
static void i2c_target_address(const struct device *dev,uint8_t slave_address,uint8_t mode);
static void i2c_write_page(const struct device *dev,uint8_t *data,uint32_t length);
static void i2c_write_byte(const struct device *dev,uint8_t data);
static uint8_t i2c_read_byte(const struct device *dev);
static int i2c_seciot_init(const struct device *dev);
static int i2c_seciot_configure(const struct device *dev,uint32_t dev_config);
static int i2c_seciot_write_msg(const struct device *dev,struct i2c_msg *msg,uint16_t addr);
static int i2c_seciot_read_msg(const struct device *dev,struct i2c_msg *msg,uint16_t addr);
static int i2c_seciot_transfer(const struct device *dev,struct i2c_msg *msgs,uint8_t num_msgs,uint16_t addr);		       



static void waitfor(unsigned int secs)
{
	unsigned int time = 0;
	while (time++ < secs);
	
}
static void delayms(long delay)
{
  for(int i=0;i<(3334*delay*(DELAY_FREQ_BASE/40000000));i++){
    __asm__ volatile("NOP");
  }
}
static void i2c_start_bit_(const struct device *dev){
    struct i2c_seciot_cfg *confg = (struct i2c_seciot_cfg *)dev->config;
 WRITE_TO_REG(confg,I2C_CONTROL,I2C_START);
#ifdef I2C_DEBUG
 printf("\nStart bit is transmitted!!!");
#endif
}

static void i2c_end(const struct device *dev){
    struct i2c_seciot_cfg *confg = (struct i2c_seciot_cfg *)dev->config;
  WRITE_TO_REG(confg,I2C_CONTROL,I2C_STOP);
  //waitfor(1000);
#ifdef I2C_DEBUG
  printf("\nStop bit is transmitted!!!");
#endif
}

static void wait_till_txrx_operation_Completes_(const struct device *dev)
{
    struct i2c_seciot_cfg *confg = (struct i2c_seciot_cfg *)dev->config;
    uint8_t timeout = 4;
    while ((READ_REG(confg,I2C_STATUS) & 0x01) && (--timeout)){
	    delayms(DELAY);
        //waitfor(100000);
    }
    // printf("\nTransmission in progress");
    #ifdef I2C_DEBUG
    if(timeout)
    printf("\nTransmission Completed");
    else
    printf("\nTransmission timeout!!");
#endif
}




static void wait_till_I2c_bus_free_(const struct device *dev)
{
    struct i2c_seciot_cfg *confg = (struct i2c_seciot_cfg *)dev->config;
     volatile uint8_t temp =(READ_REG(confg,I2C_STATUS));
while (!((READ_REG(confg,I2C_STATUS))& 0x01)){
#ifdef I2C_DEBUG
        printf("\nBus is busy...\n");
#endif
        }
#ifdef I2C_DEBUG
    printf("\nBus is free now\n");
#endif
}





static void i2c_target_address(const struct device *dev,uint8_t slave_address,uint8_t mode)
{
  uint8_t dummy;
   struct i2c_seciot_cfg *confg = (struct i2c_seciot_cfg *)dev->config;
  wait_till_I2c_bus_free_(dev);//wait till bus is free
  //printf("slave addr:%#x mode:%d ",slave_address,mode);
  WRITE_TO_REG(confg,I2C_DATA,(slave_address<<1)|(mode));//write data in data register
  i2c_start_bit_(dev);// as soon as start is initiated after start bit is given slave address along with r/~w is transmitted
  wait_till_txrx_operation_Completes_(dev);// wait till the eight bits completely get transmitted
  delayms(DELAY);
  //waitfor(2000);
  if(mode==I2C_READ){
  WRITE_TO_REG(confg,I2C_CONTROL,I2C_NACK);
  //printf("NACK is set\n");
  volatile uint8_t dummy=READ_REG_8BIT(confg,I2C_DATA);
  //printf("dummy read complete:%#x\n",dummy);
  wait_till_txrx_operation_Completes_(dev);
  delayms(DELAY);
  //waitfor(1000);
  }
 #ifdef I2C_DEBUG 
  if(!(i2c_instance[i2c_number]->lrb))//check whether ack is receieved from slave if LRB is cleared ack is recieved
    printf("\nAck received after writing slave address\n");
  else
    printf("\nAck not received after writing slave address!Slave is not detected\n");
#endif
}

static void i2c_write_page(const struct device *dev,uint8_t *data,uint32_t length){
    delayms(DELAY);
    for (uint32_t i=0;i<length;i++){
        i2c_write_byte(dev,*(data++));
        delayms(DELAY);
    }
}

static void i2c_write_byte(const struct device *dev,uint8_t data){
  struct i2c_seciot_cfg *confg = (struct i2c_seciot_cfg *)dev->config;
  WRITE_TO_REG(confg,I2C_DATA,data);// write the data in data register
  wait_till_txrx_operation_Completes_(dev);// wait till the eight bits completely get transmitted
  delayms(DELAY);
  //waitfor(1000);
  #ifdef I2C_DEBUG
  if(!(i2c_instance[i2c_number]->lrb))//check whether ack is receieved from slave
    printf("\nAck received after writing data\n");
  else
        printf("\nAck not received after writing data\n");
  #endif
}

static uint8_t i2c_read_byte(const struct device *dev){
  struct i2c_seciot_cfg *confg = (struct i2c_seciot_cfg *)dev->config;
  uint8_t data = READ_REG_8BIT(confg,I2C_DATA);// write the data in data register
 // printf("Data recieved: %#x",data);
  delayms(DELAY);
  //waitfor(1000);
  return data;
}
static int i2c_seciot_init(const struct device *dev)
{
    const struct i2c_seciot_cfg *confg = dev->config;
    uint32_t dev_config = confg->scl_clk;
    i2c_seciot_configure(dev,dev_config);

}
static int i2c_seciot_configure(const struct device *dev,uint32_t dev_config)
{
   //printk("Configuring I2C0!!!");
   struct i2c_seciot_cfg *confg = (struct i2c_seciot_cfg *)dev->config;
   uint32_t prescale = 1;
   uint32_t scl_div = (confg->sys_clk/((prescale+1)*confg->scl_clk))-1;
   WRITE_TO_REG(confg,I2C_CONTROL,I2C_PIN);
   WRITE_TO_REG(confg,I2C_PRESCALE,prescale);
   WRITE_TO_REG(confg,I2C_SCL_DIV,scl_div);
   WRITE_TO_REG(confg,I2C_CONTROL,I2C_IDLE);
   k_mutex_init(&(confg->mutex));
}


static int i2c_seciot_write_msg(const struct device *dev,struct i2c_msg *msg,uint16_t addr)
{
    i2c_target_address(dev,(uint8_t)addr,I2C_WRITE);
    uint8_t *data =msg->buf;
    uint8_t length=msg->len;
    i2c_write_page(dev,data,length);
    i2c_end(dev);
    return 0;
}
static int i2c_seciot_read_msg(const struct device *dev,struct i2c_msg *msg,uint16_t addr)
{
    uint8_t length = msg->len;
    if(length == 1)
    {
        i2c_target_address(dev,addr,I2C_READ);
        (msg->buf)[0] = i2c_read_byte(dev);
        i2c_end(dev);
        return 0;
    }
    else 
    {
        return 1;
    }
}
static int i2c_seciot_transfer(const struct device *dev,struct i2c_msg *msgs,uint8_t num_msgs,uint16_t addr)		       
{

	/* Check for NULL pointers */
	if (dev == NULL) {
		//LOG_ERR("Device handle is NULL");
		return -EINVAL;
	}
	if (dev->config == NULL) {
		//LOG_ERR("Device config is NULL");
		return -EINVAL;
	}
	if (msgs == NULL) {
		return -EINVAL;
	}
k_mutex_lock(&(((struct i2c_seciot_cfg*)(dev->config))->mutex),K_FOREVER);
    for (int i = 0; i < num_msgs; i++) {
        delayms(10);
        if (msgs[i].flags & I2C_MSG_READ) {
            
	    	i2c_seciot_read_msg(dev, &(msgs[i]), addr);
	    } else {
	    	i2c_seciot_write_msg(dev, &(msgs[i]), addr);
	    }
  k_mutex_unlock(&(((struct i2c_seciot_cfg*)(dev->config))->mutex));
    }
}

static struct i2c_driver_api i2c_seciot_api = {
	.configure = i2c_seciot_configure,
	.transfer = i2c_seciot_transfer,
};

#define I2C_SECIOT_INIT(n) \
    static struct i2c_seciot_cfg i2c_seciot_cfg_##n = { \
		.base = DT_INST_PROP(n, base), \
		.sys_clk = DT_INST_PROP(n, clock_frequency), \
		.scl_clk = DT_INST_PROP(n, scl_frequency), \
	}; \
	I2C_DEVICE_DT_INST_DEFINE(n, \
			    i2c_seciot_init, \
			    NULL, \
			    NULL, \
			    &i2c_seciot_cfg_##n, \
			    POST_KERNEL, \
			    CONFIG_I2C_INIT_PRIORITY, \
			    &i2c_seciot_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_SECIOT_INIT)