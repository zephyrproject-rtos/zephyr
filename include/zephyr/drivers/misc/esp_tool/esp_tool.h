/** Copyright 2025 Espressif Systems (Shanghai) CO LTD
 *
 * @file esp_tool.h
 * @brief ESP Tool Driver Public API for Zephyr RTOS
 *
 * This driver provides an esptool-inspired, high-level API for programming,
 * debugging and interacting with Espressif targets over serial.
 * It is a thin Zephyr-native wrapper around the esp-serial-flasher library.
 *
 */

#ifndef ZEPHYR_DRIVERS_MISC_ESP_TOOL_H_
#define ZEPHYR_DRIVERS_MISC_ESP_TOOL_H_

#include <stddef.h>
#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ESP Tool driver API
 * @defgroup esp_tool_interface ESP Tool Interface
 * @{
 */

/**
 * @brief Connect to the target ESP device in normal (download) mode.
 *
 * Establishes a connection with the target ESP SoC using the ROM bootloader.
 * This function synchronizes with the target and prepares it for further
 * flash or memory operations.
 *
 * @note This function acquires and releases the driver's internal mutex.
 *
 * @param dev Pointer to the device structure.
 * @param hs Switch to higher speed after connection
 *
 * @return 0 on success, or negative errno code on failure
 *         (typically -ETIMEDOUT on synchronization failure,
 *          -EIO on communication error, etc.).
 */
int esp_tool_connect(const struct device *dev, bool hs);

/**
 * @brief Connect to the target ESP device using a flasher stub (if available).
 *
 * Similar to @ref esp_tool_connect(), but attempts to load and use a
 * performance-optimized flasher stub (stub loader) instead of relying solely
 * on the ROM bootloader. This can significantly improve flashing speed and
 * reliability on supported targets.
 *
 * @note Stub support depends on the target chip and firmware configuration.
 * @note This function acquires and releases the driver's internal mutex.
 *
 * @param dev Pointer to the device structure.
 * @param hs Switch to higher speed after connection
 *
 * @return 0 on success, or negative errno code on failure.
 */
int esp_tool_connect_stub(const struct device *dev, bool hs);

/**
 * @brief Read the target chip ID / model identifier.
 *
 * Retrieves the unique chip identification value from the target.
 * This can be used to identify the exact ESP chip variant (ESP32, ESP32-S3, etc.).
 *
 * @param dev    Pointer to the device structure.
 * @param[out] id Pointer to variable where the chip ID will be stored.
 *
 * @return 0 on success, or negative errno code on failure.
 */
int esp_tool_get_target(const struct device *dev, uint32_t *id);

/**
 * @brief Reset the target ESP device.
 *
 * Performs a software or hardware reset of the connected ESP SoC.
 * Usually required after flashing or when switching between modes.
 *
 * @param dev Pointer to the device structure.
 *
 * @return 0 on success, or negative errno code on failure.
 */
int esp_tool_reset_target(const struct device *dev);

/**
 * @brief Begin a streaming flash write operation.
 *
 * Prepares the target to receive flash data starting at the given offset.
 * Must be followed by one or more calls to @ref esp_tool_flash_write() and
 * finished with @ref esp_tool_flash_finish().
 *
 * @param dev       Pointer to the device structure.
 * @param offset    Starting byte address in flash where writing begins.
 * @param total_size Total size of the data that will be written (used for progress tracking).
 *
 * @return 0 on success, or negative errno code on failure.
 */
int esp_tool_flash_start(const struct device *dev, uint32_t offset,
			size_t total_size, size_t block_size);

/**
 * @brief Write a block of data to flash (streaming mode).
 *
 * Sends the next portion of data to the target during an active flash write session.
 * Must be called only between successful @ref esp_tool_flash_start() and
 * @ref esp_tool_flash_finish() calls.
 *
 * @param dev   Pointer to the device structure.
 * @param data  Buffer containing data to write.
 * @param len   Number of bytes to write.
 *
 * @return 0 on success, or negative errno code on failure.
 */
int esp_tool_flash_write(const struct device *dev, const void *data, size_t len);

/**
 * @brief Finalize a streaming flash write operation.
 *
 * Completes the flash programming sequence started by @ref esp_tool_flash_start().
 * Usually includes verification (if enabled) and final handshake.
 *
 * @param dev Pointer to the device structure.
 * @param reboot Reboot target after flash write.
 *
 * @return 0 on success, or negative errno code on failure.
 */
int esp_tool_flash_finish(const struct device *dev, bool reboot);

/**
 * @brief Detect the size of the attached SPI flash chip.
 *
 * Queries the flash chip (via JEDEC ID or similar mechanism) and returns its total size.
 *
 * @param dev    Pointer to the device structure.
 * @param[out] size Pointer to variable that will receive the detected flash size in bytes.
 *
 * @return 0 on success, or negative errno code on failure.
 */
int esp_tool_flash_detect_size(const struct device *dev, uint32_t *size);

/**
 * @brief Erase a specific region of the flash memory.
 *
 * Erases the flash memory in the given range. The region must be aligned to
 * the flash sector/block size (typically 4 KiB).
 *
 * @param dev    Pointer to the device structure.
 * @param offset Starting address of the region to erase.
 * @param size   Size of the region to erase (in bytes).
 *
 * @return 0 on success, or negative errno code on failure.
 */
int esp_tool_flash_erase_region(const struct device *dev, uint32_t offset, size_t size);

/**
 * @brief Erase the entire flash memory (chip erase).
 *
 * Performs a full chip erase operation. This can take a long time.
 *
 * @warning Use with caution — all data will be lost.
 *
 * @param dev Pointer to the device structure.
 *
 * @return 0 on success, or negative errno code on failure.
 */
int esp_tool_flash_erase(const struct device *dev);

/**
 * @brief Read data from flash memory.
 *
 * Reads raw data directly from the target's flash memory.
 *
 * @param dev    Pointer to the device structure.
 * @param offset Starting address in flash to read from.
 * @param[out] data Buffer where read data will be stored.
 * @param len    Number of bytes to read.
 *
 * @return 0 on success, or negative errno code on failure.
 */
int esp_tool_flash_read(const struct device *dev, uint32_t offset,
      	uint8_t *data, size_t len);

/**
 * @brief Verify that flash contents match the provided data.
 *
 * Compares the flash memory at the given address against the supplied buffer.
 * Useful for post-write verification.
 *
 * @param dev     Pointer to the device structure.
 * @param address Starting address in flash to verify.
 * @param data    Buffer containing expected data.
 * @param len     Number of bytes to compare.
 *
 * @return 0 if contents match, or negative errno code on mismatch / error.
 */
int esp_tool_verify_image(const struct device *dev, uint32_t address,
                                        const uint8_t *data, size_t len);

/**
 * @brief Begin a streaming write to target RAM.
 *
 * Prepares the target to receive binary data into RAM (usually for running
 * a second-stage loader or test application).
 *
 * @param dev       Pointer to the device structure.
 * @param offset    Starting address in RAM where data will be loaded.
 * @param total_size Total size of the data that will be written.
 *
 * @return 0 on success, or negative errno code on failure.
 */
int esp_tool_mem_start(const struct device *dev, uint32_t offset, size_t total_size);

/**
 * @brief Write a block of data to target RAM (streaming mode).
 *
 * Sends the next portion of data during an active RAM load session.
 *
 * @param dev   Pointer to the device structure.
 * @param data  Buffer containing data to write.
 * @param len   Number of bytes to write.
 *
 * @return 0 on success, or negative errno code on failure.
 */
int esp_tool_mem_write(const struct device *dev, const uint8_t *data, size_t len);

/**
 * @brief Finalize a streaming RAM load operation.
 *
 * Completes the RAM loading sequence and usually jumps to the loaded code.
 *
 * @param dev Pointer to the device structure.
 *
 * @return 0 on success, or negative errno code on failure.
 */
int esp_tool_mem_finish(const struct device *dev);

/**
 * @brief Write a 32-bit value to a memory-mapped register on the target.
 *
 * Direct register write (only works when target is in a state that allows it).
 *
 * @param dev   Pointer to the device structure.
 * @param addr  Register address.
 * @param value Value to write.
 *
 * @return 0 on success, or negative errno code on failure.
 */
int esp_tool_register_write(const struct device *dev, uint32_t addr, uint32_t value);

/**
 * @brief Read a 32-bit value from a memory-mapped register on the target.
 *
 * Direct register read.
 *
 * @param dev    Pointer to the device structure.
 * @param addr   Register address.
 * @param[out] value Pointer to variable that will receive the read value.
 *
 * @return 0 on success, or negative errno code on failure.
 */
int esp_tool_register_read(const struct device *dev, uint32_t addr, uint32_t *value);

/**
 * @brief Read the target's MAC address.
 *
 * Retrieves the factory-programmed MAC address of the ESP device.
 *
 * @param dev  Pointer to the device structure.
 * @param[out] ptr Buffer (usually 6 bytes) where the MAC address will be stored.
 *
 * @return 0 on success, or negative errno code on failure.
 */
int esp_tool_mac_read(const struct device *dev, void *ptr);

/**
 * @brief Change the UART baud rate used for communication with the target.
 *
 * Negotiates a higher baud rate with the target (e.g. 921600) for faster transfers.
 * Works only in normal download mode (not secure download mode).
 *
 * @param dev      Pointer to the device structure.
 * @param baudrate New baud rate to set (must be supported by both sides).
 *
 * @return 0 on success, or negative errno code on failure.
 */
int esp_tool_change_tr_rate(const struct device *dev, uint32_t baudrate);

/**
 * @brief Change the UART baud rate while using the flasher stub.
 *
 * Similar to @ref esp_tool_change_tr_rate(), but intended to be used after
 * a stub has been loaded.
 *
 * @param dev      Pointer to the device structure.
 * @param baudrate New baud rate to set.
 *
 * @return 0 on success, or negative errno code on failure.
 */
int esp_tool_change_tr_rate_stub(const struct device *dev, uint32_t baudrate);

/**
 * @brief Retrieve security configuration information from the target.
 *
 * Reads efuse/security related flags (flash encryption, secure boot status, etc.).
 *
 * @param dev Pointer to the device structure.
 *
 * @return 0 on success, or negative errno code on failure.
 */
int esp_tool_get_security_info(const struct device *dev);

/**
 * @brief Connect to the target running in secure download mode.
 *
 * Establishes connection to an ESP device that has Secure UART ROM Download Mode enabled.
 * Only a limited subset of commands is available in this mode.
 *
 * @param dev Pointer to the device structure.
 *
 * @return 0 on success, or negative errno code on failure.
 */
int esp_tool_connect_secure_download_mode(const struct device *dev);

// quick add list
int esp_tool_get_boot_offset(const struct device *dev, uint32_t *off);
uint32_t esp_tool_get_current_baudrate(const struct device *dev);
bool esp_tool_is_connected(const struct device *dev);
int esp_tool_flash_binary(const struct device *dev, const void *image,
			  size_t size, uint32_t offset);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MISC_ESP_TOOL_H_ */
