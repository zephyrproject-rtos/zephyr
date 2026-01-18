/** Copyright 2025 Espressif Systems (Shanghai) CO LTD
 *
 * @file esp_tool.h
 * @brief ESP Tool Driver Public API for Zephyr RTOS
 *
 * This driver provides a Zephyr API layer on top of the esp-serial-flasher
 * library for flashing ESP targets over various serial interface.
 *
 * OR:
 *
 * This driver provides an esptool-inspired, high-level API for programming,
 * debugging and interacting with Espressif targets over serial.
 * It is a thin Zephyr-native wrapper around the esp-serial-flasher library
 * (using its modern esp_loader_* API internally).
 *
 * The API is designed to be used from Zephyr applications and subsystems
 * (e.g., DFU, west flash runner, console tools).
 */

#ifndef ZEPHYR_DRIVERS_MISC_ESP_TOOL_H_
#define ZEPHYR_DRIVERS_MISC_ESP_TOOL_H_

#include <zephyr/types.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if 0 //??????????  syscall or not, subsys or n ot ?

/**
 * @brief ESP Tool driver API structure
 */
struct esp_tool_driver_api {
    /* Connection */
    int (*connect)(const struct device *dev);
    int (*connect_with_stub)(const struct device *dev);

    /* Target control and information */
    int (*get_target)(const struct device *dev, uint32_t *chip_type);
    int (*reset_target)(const struct device *dev);

    /* Flash operations - streaming style */
    int (*flash_start)(const struct device *dev, uint32_t offset, size_t total_size);
    int (*flash_write)(const struct device *dev, const uint8_t *data, size_t len);
    int (*flash_finish)(const struct device *dev);

    /* Flash operations - direct / utility */
    int (*flash_erase_region)(const struct device *dev, uint32_t offset, size_t size);
    int (*flash_erase)(const struct device *dev);
    int (*flash_detect_size)(const struct device *dev, uint32_t *size);
    int (*flash_read)(const struct device *dev, uint32_t offset, uint8_t *data, size_t len);
    int (*verify_image)(const struct device *dev, uint32_t address, const uint8_t *data, size_t len);

    /* RAM (memory) load operations */
    int (*mem_start)(const struct device *dev, uint32_t offset, size_t total_size);
    int (*mem_write)(const struct device *dev, const uint8_t *data, size_t len);
    int (*mem_finish)(const struct device *dev);

    /* Register access */
    int (*register_write)(const struct device *dev, uint32_t addr, uint32_t value);
    int (*register_read)(const struct device *dev, uint32_t addr, uint32_t *value);

    /* Miscellaneous */
    int (*mac_read)(const struct device *dev, uint8_t mac[6]);
    int (*change_tr_rate)(const struct device *dev, uint32_t baudrate);
    int (*change_tr_rate_stub)(const struct device *dev, uint32_t baudrate);

    /* Advanced / security */
    int (*get_security_info)(const struct device *dev);
    int (*connect_secure_download_mode)(const struct device *dev);
};
#endif

/**
 * @brief ESP Tool driver API
 * @defgroup esp_tool_interface ESP Tool Interface
 * @{
 */

/* Connection */
int esp_tool_connect(const struct device *dev);

int esp_tool_connect_with_stub(const struct device *dev);

/* Target control and information */
int esp_tool_get_target(const struct device *dev, uint32_t *id);

int esp_tool_reset_target(const struct device *dev);

/* Flash operations - streaming style */
int esp_tool_flash_start(const struct device *dev, uint32_t offset, size_t total_size);

int esp_tool_flash_write(const struct device *dev, const void *data, size_t len);

int esp_tool_flash_finish(const struct device *dev);

/* Flash operations - direct / utility */
int esp_tool_flash_detect_size(const struct device *dev, uint32_t *size);

int esp_tool_flash_erase_region(const struct device *dev, uint32_t offset, size_t size);

int esp_tool_flash_erase(const struct device *dev);


int esp_tool_flash_read(const struct device *dev, uint32_t offset,
      	uint8_t *data, size_t len);
int esp_tool_verify_image(const struct device *dev, uint32_t address,
                                        const uint8_t *data, size_t len);

/* RAM (memory) load operations */
int esp_tool_mem_start(const struct device *dev, uint32_t offset, size_t total_size);

int esp_tool_mem_write(const struct device *dev, const uint8_t *data, size_t len);

int esp_tool_mem_finish(const struct device *dev);

/* Register access */
int esp_tool_register_write(const struct device *dev, uint32_t addr, uint32_t value);

int esp_tool_register_read(const struct device *dev, uint32_t addr, uint32_t *value);

/* Miscellaneous */
int esp_tool_mac_read(const struct device *dev, void *ptr);

int esp_tool_change_tr_rate(const struct device *dev, uint32_t baudrate);

int esp_tool_change_tr_rate_stub(const struct device *dev, uint32_t baudrate);

/* Advanced / security */
int esp_tool_get_security_info(const struct device *dev);

int esp_tool_connect_secure_download_mode(const struct device *dev);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MISC_ESP_TOOL_H_ */
