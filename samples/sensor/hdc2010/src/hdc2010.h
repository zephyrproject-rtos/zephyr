// config reg
// config reg
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#define CONFIG_REG 0X0E
#define MEASUREMENT_CONFIG 0x0F
// who i am reg
#define DEVICE_ID_L 0xFE
#define DEVICE_ID_H 0xFF

// menu id
#define MENU_ID_L 0xFC
#define MENU_ID_H 0xFD

// reading temp reg
#define TEMP_L 0X00
#define TEMP_H 0X01

// config interrupt pin
#define INTERRUPT_CONFIG 0x07
int readDeviceId(const struct i2c_dt_spec *hdc2010, uint16_t *id, uint16_t *id1);
int hdc2010Init(const struct i2c_dt_spec *hdc2010);
int hdc2010DeInit();
int Hdc2010ReadingTemperature(const struct i2c_dt_spec *hdc2010, int *val);
int hdc_enable_interrupt(const struct i2c_dt_spec *hdc2010);
int hdc2010_setInterruptPolarity(const struct i2c_dt_spec *hdc2010, int polarity);
int triggerMeasurement(const struct i2c_dt_spec *hdc2010);
int hdc2010_enableDRDYInterrupt(const struct i2c_dt_spec *hdc2010);
int HDC2010_Enable_DRDY_Interrupt(const struct i2c_dt_spec *hdc2010, int polarity);
int hdc2010_enableThresMaxInterrupt(const struct i2c_dt_spec *hdc2010);
int hdc2010_enableThresMinInterrupt(const struct i2c_dt_spec *hdc2010);
int setThresholdTempMax(const struct i2c_dt_spec *hdc2010, uint8_t data);
int setThresholdTempMin(const struct i2c_dt_spec *hdc2010, uint8_t data);
