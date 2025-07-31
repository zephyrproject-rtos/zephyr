/*
 * I2C Scanner Sample for LPC54S018
 *
 * This sample scans the I2C bus for connected devices and
 * demonstrates basic I2C communication.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(i2c_scanner, LOG_LEVEL_INF);

/* Status LED */
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* Get I2C device from device tree (FLEXCOMM4 configured as I2C) */
#define I2C_NODE DT_ALIAS(arduino_i2c)
static const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);

/* Common I2C device addresses to check */
static const uint8_t common_addresses[] = {
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,  /* PCF8574 I/O expander */
    0x3C, 0x3D,                                        /* OLED displays */
    0x48, 0x49, 0x4A, 0x4B,                           /* Temperature sensors */
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,  /* EEPROM */
    0x68, 0x69,                                        /* RTC (DS3231, etc) */
    0x76, 0x77,                                        /* BME280/BMP280 */
};

static void scan_i2c_bus(void)
{
    uint8_t data;
    int ret;
    int found = 0;

    LOG_INF("Scanning I2C bus for devices...");
    LOG_INF("Address range: 0x08 - 0x77");

    /* Scan standard I2C address range */
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        /* Try to read one byte from the device */
        ret = i2c_read(i2c_dev, &data, 1, addr);
        
        if (ret == 0) {
            LOG_INF("Found device at address 0x%02X", addr);
            found++;
            
            /* Check if it's a known device type */
            for (int i = 0; i < ARRAY_SIZE(common_addresses); i++) {
                if (addr == common_addresses[i]) {
                    switch (addr) {
                        case 0x3C:
                        case 0x3D:
                            LOG_INF("  -> Possibly OLED display");
                            break;
                        case 0x68:
                        case 0x69:
                            LOG_INF("  -> Possibly RTC (DS3231/DS1307)");
                            break;
                        case 0x76:
                        case 0x77:
                            LOG_INF("  -> Possibly BME280/BMP280 sensor");
                            break;
                        default:
                            if (addr >= 0x50 && addr <= 0x57) {
                                LOG_INF("  -> Possibly EEPROM");
                            } else if (addr >= 0x20 && addr <= 0x27) {
                                LOG_INF("  -> Possibly I/O expander");
                            }
                            break;
                    }
                    break;
                }
            }
        }
    }

    if (found == 0) {
        LOG_INF("No I2C devices found");
    } else {
        LOG_INF("Scan complete. Found %d device(s)", found);
    }
}

/* Example: Read from 24C02 EEPROM if present */
static void eeprom_example(void)
{
    uint8_t eeprom_addr = 0x50;
    uint8_t reg_addr = 0x00;
    uint8_t data[16];
    int ret;

    /* Check if EEPROM is present */
    ret = i2c_read(i2c_dev, data, 1, eeprom_addr);
    if (ret != 0) {
        LOG_DBG("No EEPROM found at 0x%02X", eeprom_addr);
        return;
    }

    LOG_INF("EEPROM example - reading first 16 bytes:");

    /* Read 16 bytes from EEPROM starting at address 0 */
    ret = i2c_write_read(i2c_dev, eeprom_addr, &reg_addr, 1, data, sizeof(data));
    if (ret == 0) {
        LOG_HEXDUMP_INF(data, sizeof(data), "EEPROM data:");
    } else {
        LOG_ERR("Failed to read EEPROM: %d", ret);
    }
}

/* Example: Configure PCF8574 I/O expander if present */
static void io_expander_example(void)
{
    uint8_t pcf8574_addr = 0x20;
    uint8_t data = 0x00;
    int ret;

    /* Check if PCF8574 is present */
    ret = i2c_read(i2c_dev, &data, 1, pcf8574_addr);
    if (ret != 0) {
        LOG_DBG("No PCF8574 found at 0x%02X", pcf8574_addr);
        return;
    }

    LOG_INF("PCF8574 I/O expander example:");

    /* Toggle all outputs */
    for (int i = 0; i < 4; i++) {
        data = (i % 2) ? 0xFF : 0x00;
        ret = i2c_write(i2c_dev, &data, 1, pcf8574_addr);
        if (ret == 0) {
            LOG_INF("Set outputs to 0x%02X", data);
        } else {
            LOG_ERR("Failed to write to PCF8574: %d", ret);
            break;
        }
        k_sleep(K_MSEC(500));
    }
}

int main(void)
{
    int ret;

    LOG_INF("I2C Scanner Sample for LPC54S018");
    LOG_INF("I2C bus: FLEXCOMM4");
    LOG_INF("SCL: P0.26 (Arduino D15)");
    LOG_INF("SDA: P0.25 (Arduino D14)");

    /* Configure LED */
    if (device_is_ready(led.port)) {
        ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
        if (ret < 0) {
            LOG_ERR("Failed to configure LED: %d", ret);
        }
    }

    /* Check if I2C device is ready */
    if (!device_is_ready(i2c_dev)) {
        LOG_ERR("I2C device not ready");
        return -1;
    }

    /* Configure I2C */
    uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_FAST) | I2C_MODE_CONTROLLER;
    ret = i2c_configure(i2c_dev, i2c_cfg);
    if (ret < 0) {
        LOG_ERR("Failed to configure I2C: %d", ret);
        return -1;
    }

    LOG_INF("I2C configured at 400 kHz (Fast mode)");

    /* Main loop */
    while (1) {
        /* Toggle LED to show activity */
        if (device_is_ready(led.port)) {
            gpio_pin_toggle_dt(&led);
        }

        /* Scan I2C bus */
        scan_i2c_bus();

        /* Run examples if devices are found */
        eeprom_example();
        io_expander_example();

        /* Wait before next scan */
        LOG_INF("Waiting 10 seconds before next scan...");
        LOG_INF("Use shell command 'i2c scan flexcomm4' for manual scan");
        k_sleep(K_SECONDS(10));
    }

    return 0;
}