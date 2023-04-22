

#ifndef _BH1749_H__
#define _BH1749_H__

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#define I2C_ADDRESS         0x38

#define SYS_CONTROL         0x40

#define MODE_CNTL1          0x41
#define MODE_CTRL2          0x42

#define RED_DATAL           0x50
#define RED_DATAH           0x51

#define GREEN_DATAL         0x52
#define GREEN_DATAH         0x53

#define BLUE_DATAL          0x54
#define BLUE_DATAH          0x55

#define IR_DATAL            0x58
#define IR_DATAH            0x59

#define GREEN2_DATAL        0x5A
#define GREEN2_DATAH        0x5B

#define INTERRUPT            0x60
#define PERSISTANCE         0x61


#define TH_HIGHL            0x62
#define TH_HIGHH            0x63

#define TH_LOWL             0x64
#define TH_LOWH             0x65

#define CHIP_ID_VAL         0x92


struct bh1749_config {
    struct i2c_dt_spec i2c;

};
struct bh1749_data {

    uint16_t red;
    uint16_t green;
    uint16_t blue;

    uint16_t ir;

    uint16_t green2;

    uint16_t th_high;
    uint16_t th_low;

};

#endif
