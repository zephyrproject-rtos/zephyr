

#define DT_DRV_COMPAT rohm_bh1749

#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor.h>


#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <stdio.h>

#include "bh1749.h"


LOG_MODULE_REGISTER(BH1749, CONFIG_SENSOR_LOG_LEVEL);


static int bh1749_sample_fetch(const struct device *dev, enum sensor_channel chan)

{    
    struct bh1749_data *data = dev->data;
    const struct bh1749_config *cfg = dev->config;

    uint16_t buf[3];
    uint16_t buff[2];

    uint16_t th[2];

    if(i2c_burst_read_dt(&cfg->i2c, RED_DATAL, (uint8_t *)buf, 6) < 0){
        LOG_ERR("Failed to read data sample");
        return -EIO;
     }

    data->red = sys_be16_to_cpu(buf[0]);
    data->green = sys_be16_to_cpu(buf[1]);
    data->blue = sys_be16_to_cpu(buf[2]);  

    if(i2c_burst_read_dt(&cfg->i2c, IR_DATAL, (uint8_t *)buff, 4) < 0){
        LOG_ERR("Failed to read data sample");
        return -EIO;
    }

    if(i2c_burst_read_dt(&cfg->i2c, TH_HIGHL, (uint8_t *)th, 4) < 0){
        LOG_ERR("Failed to read data sample");
        return -EIO;
    }


    data->ir = sys_be16_to_cpu(buff[0]);
    data->green2 = sys_be16_to_cpu(buff[1]);

    data->th_high = sys_be16_to_cpu(th[0]);
    data->th_low = sys_be16_to_cpu(th[1]);

    return 0;
}

static int bh1749_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val)
{
    struct bh1749_data *data = dev->data;
    
    switch(chan) {
    
    case SENSOR_CHAN_RED: 
        val->val1 = (int)data->red * 0.0125;
        val->val2 = (int)data->red % 80;        
        break;
        
    case SENSOR_CHAN_BLUE:
        val->val1 = (int)data->blue * 0.0125;
        val->val2 = (int)data->blue % 80;
        break;

    case SENSOR_CHAN_GREEN:
        val->val1 = (int)data->green * 0.0125;
        val->val2 = (int)data->green % 80;
        break;

    case SENSOR_CHAN_IR:
        val->val1 = (int)data->ir * 0.0125;
        val->val2 = (int)data->ir % 80;
        break;
    default:
        break;
    }
    return 0;
}

static const struct sensor_driver_api bh1749_driver_api = {

    .sample_fetch = bh1749_sample_fetch,
    .channel_get = bh1749_channel_get,

};


uint8_t bh1749_read_chip_id(const struct device *dev){

    const struct bh1749_config *cfg = dev->config;
    //struct bh1749_data *data = dev->data;
    uint8_t chip_id;

    if(i2c_reg_read_byte_dt(&cfg->i2c, CHIP_ID_VAL, &chip_id) < 0){
        LOG_ERR("Wrong chip detected");
        
        return -EIO;
    }
    return chip_id;
}


int bh1749_init(const struct device *dev){

    const struct bh1749_config *cfg = dev->config;
    //struct bh1749_data *data = dev->data;
    uint8_t id;

    if(!device_is_ready(cfg->i2c.bus)){
        LOG_ERR("Device is not ready!");
        return -ENODEV;
    }
    /*check the chip-id */
    if(i2c_reg_read_byte_dt(&cfg->i2c, CHIP_ID_VAL, &id) < 0){
        LOG_ERR("Wrong chip detected");       
        return -EIO;
    }
    
    if(i2c_reg_write_byte_dt(&cfg->i2c, SYS_CONTROL, 0x0D) < 0){
        LOG_ERR("Error writing SYS control");       
        return -EIO;
    }
    /*keep sensor in highest gain and measurement time*/
    if(i2c_reg_write_byte_dt(&cfg->i2c, MODE_CNTL1, 0x2A) < 0){
        LOG_ERR("Error wtinting control reg1");       
        return -EIO;
    }

    if(i2c_reg_write_byte_dt(&cfg->i2c, MODE_CTRL2, 0x10) < 0){
        LOG_ERR("Error wtinting control reg2");       
        return -EIO;
    }
    
    if(i2c_reg_write_byte_dt(&cfg->i2c, INTERRUPT, 0x00) < 0){
        LOG_ERR("Error wtinting control Interrupt reg");       
        return -EIO;
    }

    if(i2c_reg_write_byte_dt(&cfg->i2c, PERSISTANCE, 0x01) < 0){
        LOG_ERR("Error wtinting Persistance reg");       
        return -EIO;
    }

    /*   Write thresold values  */

    return 0;
}

#define BH1749_DEFINE(inst)									\
	static struct bh1749_data bh1749_data_##inst;						\
												\
	static const struct bh1749_config bh1749_config_##inst = {				\
		.i2c = I2C_DT_SPEC_INST_GET(inst),						\
		};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, bh1749_init, NULL,					\
			      &bh1749_data_##inst, &bh1749_config_##inst,			\
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,				\
			      &bh1749_driver_api);						\

DT_INST_FOREACH_STATUS_OKAY(BH1749_DEFINE)
