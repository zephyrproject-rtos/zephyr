#ifndef CST816S_H
#define CST816S_H
#include<stdint.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#define CST816S_ADDRESS     0x15

enum GESTURE {
  NONE = 0x00,
  SWIPE_UP = 0x01,
  SWIPE_DOWN = 0x02,
  SWIPE_LEFT = 0x03,
  SWIPE_RIGHT = 0x04,
  SINGLE_CLICK = 0x05,
  DOUBLE_CLICK = 0x0B,
  LONG_PRESS = 0x0C

};

typedef struct {
  uint8_t gestureID; // Gesture ID
  uint8_t points;  // Number of touch points
  uint8_t event; // Event (0 = Down, 1 = Up, 2 = Contact)
  int x;
  int y;
  uint8_t version;
  uint8_t versionInfo[3];
}data_struct;

typedef struct{
    volatile struct device *i2c_dev;
    struct gpio_dt_spec rst;
	struct gpio_dt_spec irq;
    volatile uint8_t* data_arr;
}cst816s_t;


void CST816S_init(cst816s_t *handle,const struct device *gpio_dev,const struct device *i2c_dev,uint8_t rst,uint8_t irq,uint8_t *data);

data_struct CST816S_begin(cst816s_t *handle);

uint8_t CST816S_read(cst816s_t *handle,uint16_t addr, uint8_t reg_addr, uint8_t *reg_data, uint32_t length);

uint8_t CST816S_write(cst816s_t *handle,uint8_t addr, uint8_t reg_addr, const uint8_t *reg_data, uint32_t length);

data_struct CST816S_read_touch(cst816s_t *handle);

void CST816S_sleep(cst816s_t *handle);

#endif
