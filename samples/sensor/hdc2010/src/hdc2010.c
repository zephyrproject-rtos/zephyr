#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include "hdc2010.h"
// init flag
int initFlag = 0;
// init temp flag
int tempFlag = 0;
// init hum flag
int humFlag = 0;

uint8_t config[2] = {CONFIG_REG, 0x80};

int readDeviceId(const struct i2c_dt_spec *hdc2010, uint16_t *id, uint16_t *id1)
{
    int ret;
    // first reading device id
    uint8_t reg[2] = {DEVICE_ID_L, DEVICE_ID_H};
    uint8_t device_id[2] = {DEVICE_ID_L, DEVICE_ID_H};
    ret = i2c_write_read_dt(hdc2010, &reg[0], 1, &device_id[0], 1);
    if (ret)
    {
        printk("device id error\n");
        return -1;
    }
    ret = i2c_write_read_dt(hdc2010, &reg[1], 1, &device_id[1], 1);
    if (ret)
    {
        printk("device id error\n");
        return -1;
    }
    *id = (device_id[1] << 8) | device_id[0];
    // second reading menu id
    reg[0] = MENU_ID_L;
    reg[1] = MENU_ID_H;
    ret = i2c_write_read_dt(hdc2010, &reg[0], 1, &device_id[0], 1);
    if (ret)
    {
        printk("menu id error\n");
        return -1;
    }
    ret = i2c_write_read_dt(hdc2010, &reg[1], 1, &device_id[1], 1);
    if (ret)
    {
        printk("menu id error");
        return -1;
    }
    *id1 = (device_id[1] << 8) | device_id[0];
    printk("%x %x device id and menu id\n", *id, *id1);
    return 0;
}

int hdc2010Init(const struct i2c_dt_spec *hdc2010)
{
    uint8_t oldConfig;
    int ret;
    if (initFlag)
    {
        return 0;
    }

    uint8_t tempReg[2] = {CONFIG_REG, 0};

    ret = i2c_write_read_dt(hdc2010, &tempReg[0], 1, &oldConfig, 1);
    if (ret)
    {
        printk("writing in reg problem\n");
        return -1;
    }
    uint8_t newConfig = oldConfig | 7 << 4;
    config[1] = newConfig;
    ret = i2c_write_dt(hdc2010, config, sizeof(config));
    if (ret != 0)
    {
        printk("soft reseting is fail\n");
        return -1;
    }

    /*
    Bits 2 and 1 of the MEASUREMENT_CONFIG register controls
    the measurement mode
    */
    // Upper two bits of the MEASUREMENT_CONFIG register controls the temperature resolution
    // default value is 14
    // Bit 0 of the MEASUREMENT_CONFIG register can be used to trigger measurements

    config[0] = MEASUREMENT_CONFIG;
    config[1] = 0X4b;
    ret = i2c_write_dt(hdc2010, config, sizeof(config));
    if (ret != 0)
    {
        printk("setup measurement config failed \n");
        return -1;
    }
    initFlag = 1;
    tempFlag = 1;
    return 0;
}

int hdc2010DeInit()
{
    tempFlag = 0;
    humFlag = 0;
    initFlag = 0;
    return 0;
}

int Hdc2010ReadingTemperature(const struct i2c_dt_spec *hdc2010, int *val)
{
    int ret;
    volatile uint16_t temp;
    float intermediate, value;
    hdc2010Init(hdc2010);
    uint8_t tempReg[2] = {TEMP_L, TEMP_H};
    uint8_t reading[2];
    ret = i2c_write_read_dt(hdc2010, &tempReg[0], 1, &reading[0], 1);
    if (ret)
    {
        printk("failed at reading temperature\n");
        return -1;
    }
    ret = i2c_write_read_dt(hdc2010, &tempReg[1], 1, &reading[1], 1);
    if (ret)
    {
        printk("failed at reading temperature\n");
        return -1;
    }
    temp = (reading[1] << 8) | reading[0];
    intermediate = temp / 65536.0;
    intermediate = intermediate * 165;
    value = intermediate - 40.00;
    *val = value;
    printk("%0.2f C\n", value);
    hdc2010DeInit();
    return 0;
}

int hdc_enable_interrupt(const struct i2c_dt_spec *hdc2010)
{
    uint8_t oldConfig;
    int ret;
    uint8_t tempReg[2] = {CONFIG_REG, 0};

    ret = i2c_write_read_dt(hdc2010, &tempReg[0], 1, &oldConfig, 1);
    if (ret)
    {
        printk("reading in reg problem int\n");
        return -1;
    }
    uint8_t newConfig = oldConfig | 1 << 2 | 1 << 0; // setting intEn pin in config reg
    tempReg[1] = newConfig;

    ret = i2c_write_dt(hdc2010, tempReg, sizeof(tempReg));
    if (ret)
    {
        printk("setting int pin is not set\n");
        return -1;
    }
    return ret;
}

int hdc2010_setInterruptPolarity(const struct i2c_dt_spec *hdc2010, int polarity)
{
    uint8_t oldConfig;
    int ret;
    uint8_t tempReg[2] = {CONFIG_REG, 0};

    ret = i2c_write_read_dt(hdc2010, &tempReg[0], 1, &oldConfig, 1);
    if (ret)
    {
        printk("writing in reg problem\n");
        return -1;
    }
    uint16_t newConfig;
    if (polarity)
    {
        newConfig = oldConfig | 1 << 1; // setting polarity of pin active high
    }
    else
    {
        newConfig = oldConfig & ~(1 << 1); // setting polarity of pin active low
    }
    tempReg[1] = newConfig;
    newConfig = newConfig | 1 << 0;
    ret = i2c_write_dt(hdc2010, tempReg, sizeof(tempReg));
    if (ret)
    {
        printk("setting int pin is not set\n");
        return -1;
    }
    return ret;
}
// trigger reading
int triggerMeasurement(const struct i2c_dt_spec *hdc2010)
{
    uint8_t oldConfig;
    int ret;
    uint8_t tempReg[2] = {MEASUREMENT_CONFIG, 0};

    ret = i2c_write_read_dt(hdc2010, &tempReg[0], 1, &oldConfig, 1);
    if (ret)
    {
        printk("writing in reg problem\n");
        return -1;
    }
    uint8_t newConfig = oldConfig | 1 << 0; // setting start measurement
    tempReg[1] = newConfig;

    ret = i2c_write_dt(hdc2010, tempReg, sizeof(tempReg));
    if (ret)
    {
        printk("setting int pin is not set\n");
        return -1;
    }
    return 0;
}
// setting up drdy bit
int hdc2010_enableDRDYInterrupt(const struct i2c_dt_spec *hdc2010)
{
    uint8_t config;
    int ret;
    uint8_t tempReg[2] = {INTERRUPT_CONFIG, 0};

    ret = i2c_write_read_dt(hdc2010, &tempReg[0], 1, &config, 1);
    if (ret)
    {
        printk("writing in reg problem\n");
        return -1;
    }
    uint8_t newConfig = config | 1 << 7; // setting intEn pin in config reg
    tempReg[1] = newConfig;

    ret = i2c_write_dt(hdc2010, tempReg, sizeof(tempReg));
    if (ret)
    {
        printk("setting int pin is not set\n");
        return -1;
    }
    return 0;
}

// setting up interrupt drdy
int HDC2010_Enable_DRDY_Interrupt(const struct i2c_dt_spec *hdc2010, int polarity)
{

    int ret = hdc2010_enableDRDYInterrupt(hdc2010);
    ret = hdc_enable_interrupt(hdc2010);
    if (ret)
    {
        printk("hdc2010 enable interrupt error\n");
        return -1;
    }
    ret = hdc2010_setInterruptPolarity(hdc2010, polarity);
    if (ret)
    {
        printk("hdc2010 enabling polarity error\n");
    }
    ret = triggerMeasurement(hdc2010);

    return 0;
}
int hdc2010_enableThresMaxInterrupt(const struct i2c_dt_spec *hdc2010)
{
    uint8_t config;
    int ret;
    uint8_t tempReg[2] = {INTERRUPT_CONFIG, 0};

    ret = i2c_write_read_dt(hdc2010, &tempReg[0], 1, &config, 1);
    if (ret)
    {
        printk("writing in reg problem\n");
        return -1;
    }
    uint8_t newConfig = config | 1 << 6; // setting intEn pin in config reg
    tempReg[1] = newConfig;

    ret = i2c_write_dt(hdc2010, tempReg, sizeof(tempReg));
    if (ret)
    {
        printk("setting int pin is not set\n");
        return -1;
    }
    return 0;
}

int hdc2010_enableThresMinInterrupt(const struct i2c_dt_spec *hdc2010)
{
    uint8_t config;
    int ret;
    uint8_t tempReg[2] = {INTERRUPT_CONFIG, 0};

    ret = i2c_write_read_dt(hdc2010, &tempReg[0], 1, &config, 1);
    if (ret)
    {
        printk("writing in reg problem\n");
        return -1;
    }
    uint8_t newConfig = config | 1 << 5; // setting intEn pin in config reg
    tempReg[1] = newConfig;

    ret = i2c_write_dt(hdc2010, tempReg, sizeof(tempReg));
    if (ret)
    {
        printk("setting int pin is not set\n");
        return -1;
    }
    return 0;
}

int setThresholdTempMax(const struct i2c_dt_spec *hdc2010, uint8_t data)
{

    uint8_t temp[2] = {0x0b, data};
    int ret = i2c_write_dt(hdc2010, temp, sizeof(temp));
    return 0;
}
int setThresholdTempMin(const struct i2c_dt_spec *hdc2010, uint8_t data)
{
    uint8_t temp[2] = {
        0x0a, data};
    int ret = i2c_write_dt(hdc2010, temp, sizeof(temp));
    return 0;
}