#include "cst816s.h"

void CST816S_init(cst816s_t *handle,const struct device *gpio_dev,const struct device *i2c_dev,uint8_t rst,uint8_t irq,uint8_t *data)
{
    handle->rst.pin = rst;
    handle->irq.pin = irq;
    handle->i2c_dev = i2c_dev;
    handle->rst.port = gpio_dev;
    handle->irq.port = gpio_dev;
    gpio_pin_configure(gpio_dev,rst,GPIO_OUTPUT);
    gpio_pin_configure(gpio_dev,irq,GPIO_INPUT);
    handle->data_arr = data;
}

data_struct CST816S_begin(cst816s_t *handle)
{
/*implementation without interrupt*/
/*Have to set irq pin as input for interrupt*/
/*rst pin by default configured as output*/

 gpio_pin_set_dt(&handle->rst, 1);
  k_msleep(50);
  gpio_pin_set_dt(&handle->rst, 0);
  k_msleep(5);
  gpio_pin_set_dt(&handle->rst, 1 );
  k_msleep(50);
  data_struct data;
  CST816S_read(handle,CST816S_ADDRESS, 0x15, &data.version, 1);
  k_msleep(5);
  CST816S_read(handle,CST816S_ADDRESS, 0xA7, data.versionInfo, 3);
  return data;

}
uint8_t CST816S_read(cst816s_t *handle,uint16_t addr, uint8_t reg_addr, uint8_t *reg_data, uint32_t length)
{
    uint8_t msg_arr[1];
	struct i2c_msg msg_obj;
	msg_obj.buf = msg_arr; 
	msg_obj.len = 1;
	for(uint8_t i=0;i<length;i++){
      msg_arr[0] = reg_addr+i;
	  msg_obj.flags = I2C_MSG_WRITE;
	  i2c_transfer(handle->i2c_dev,&msg_obj,1,addr);
      msg_obj.flags = I2C_MSG_READ;
	  i2c_transfer(handle->i2c_dev,&msg_obj,1,addr);
	  reg_data[i] = msg_arr[0];
    }
  return 0;
}
uint8_t CST816S_write(cst816s_t *handle,uint8_t addr, uint8_t reg_addr, const uint8_t *reg_data, uint32_t length)
{
      uint8_t msg_arr[10];
	  struct i2c_msg msg_obj;
	  msg_obj.buf = msg_arr;
	  msg_obj.len = length;
      msg_arr[0] = reg_addr;
      for(uint8_t i=1;i<=length;i++){
        msg_arr[i] = reg_data[i];
      }
	  msg_obj.flags = I2C_MSG_WRITE;
	  i2c_transfer(handle->i2c_dev,&msg_obj,1,addr);
  return 0;
}

data_struct CST816S_read_touch(cst816s_t *handle) {
  uint8_t *data_raw = handle->data_arr;
  CST816S_read(handle,CST816S_ADDRESS, 0x01, data_raw, 6);
  data_struct data;
  data.gestureID = data_raw[0];
  data.points = data_raw[1];
  data.event = data_raw[2] >> 6;
  data.x = ((data_raw[2] & 0xF) << 8) + data_raw[3];
  data.y = ((data_raw[4] & 0xF) << 8) + data_raw[5];
  return data;
}

void CST816S_sleep(cst816s_t *handle) {
  gpio_pin_set_dt(&handle->rst, 0);
  k_msleep(5);
  gpio_pin_set_dt(&handle->rst, 1 );
  k_msleep(50);
  uint8_t standby_value = 0x03;
  CST816S_write(handle->i2c_dev,CST816S_ADDRESS, 0xA5, &standby_value, 1);
}