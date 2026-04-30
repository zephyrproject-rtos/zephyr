/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Raspberry Pi VideoCore firmware mailbox property interface.
 *
 * Tag reference:
 *   https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
 */

/**
 * @file
 * @brief Raspberry Pi VideoCore firmware property channel API
 */

#ifndef ZEPHYR_DRIVERS_FIRMWARE_RPI_FW_RPI_FW_H_
#define ZEPHYR_DRIVERS_FIRMWARE_RPI_FW_RPI_FW_H_

#include <stdint.h>

#include <zephyr/device.h>

/**
 * @defgroup rpi_fw Raspberry Pi VideoCore Firmware
 * @{
 */

/** @brief Request is being processed by the firmware */
#define RPI_FW_REQUEST_PROCESS 0x00000000U
/** @brief Request completed successfully */
#define RPI_FW_REQUEST_SUCCESS 0x80000000U
/** @brief Request failed with an error */
#define RPI_FW_REQUEST_ERROR   0x80000001U

/** @name Firmware info tags */
/** @{ */
/** @brief End of property tag list */
#define RPI_FW_TAG_END                  0x00000000U
/** @brief Get VideoCore firmware build timestamp */
#define RPI_FW_TAG_GET_FIRMWARE_VERSION 0x00000001U
/** @} */

/** @name Board info tags */
/** @{ */
/** @brief Get board model number */
#define RPI_FW_TAG_GET_BOARD_MODEL    0x00010001U
/** @brief Get board hardware revision code */
#define RPI_FW_TAG_GET_BOARD_REVISION 0x00010002U
/** @brief Get board MAC address */
#define RPI_FW_TAG_GET_BOARD_MAC_ADDR 0x00010003U
/** @brief Get board unique serial number */
#define RPI_FW_TAG_GET_BOARD_SERIAL   0x00010004U
/** @brief Get ARM core memory base and size */
#define RPI_FW_TAG_GET_ARM_MEMORY     0x00010005U
/** @brief Get VideoCore memory base and size */
#define RPI_FW_TAG_GET_VC_MEMORY      0x00010006U
/** @brief Get list of available clocks */
#define RPI_FW_TAG_GET_CLOCKS         0x00010007U
/** @} */

/** @name Clock tags */
/** @{ */
/** @brief Get clock enable/disable state */
#define RPI_FW_TAG_GET_CLOCK_STATE    0x00030001U
/** @brief Get current clock rate in Hz */
#define RPI_FW_TAG_GET_CLOCK_RATE     0x00030002U
/** @brief Get maximum supported clock rate in Hz */
#define RPI_FW_TAG_GET_MAX_CLOCK_RATE 0x00030004U
/** @brief Get minimum supported clock rate in Hz */
#define RPI_FW_TAG_GET_MIN_CLOCK_RATE 0x00030007U
/** @brief Get turbo mode state */
#define RPI_FW_TAG_GET_TURBO          0x00030009U
/** @brief Get measured clock rate in Hz */
#define RPI_FW_TAG_GET_CLOCK_MEASURED 0x00030047U
/** @brief Set clock enable/disable state */
#define RPI_FW_TAG_SET_CLOCK_STATE    0x00038001U
/** @brief Set clock rate in Hz */
#define RPI_FW_TAG_SET_CLOCK_RATE     0x00038002U
/** @brief Set turbo mode state */
#define RPI_FW_TAG_SET_TURBO          0x00038009U
/** @brief Set SD host clock rate */
#define RPI_FW_TAG_SET_SDHOST_CLOCK   0x00038042U
/** @} */

/** @name Voltage tags */
/** @{ */
/** @brief Get current voltage in micro-volts */
#define RPI_FW_TAG_GET_VOLTAGE     0x00030003U
/** @brief Get maximum supported voltage in micro-volts */
#define RPI_FW_TAG_GET_MAX_VOLTAGE 0x00030005U
/** @brief Get minimum supported voltage in micro-volts */
#define RPI_FW_TAG_GET_MIN_VOLTAGE 0x00030008U
/** @brief Set voltage in micro-volts */
#define RPI_FW_TAG_SET_VOLTAGE     0x00038003U
/** @} */

/** @name Temperature tags */
/** @{ */
/** @brief Get current SoC temperature in milli-degrees Celsius */
#define RPI_FW_TAG_GET_TEMPERATURE     0x00030006U
/** @brief Get maximum safe SoC temperature in milli-degrees Celsius */
#define RPI_FW_TAG_GET_MAX_TEMPERATURE 0x0003000AU
/** @} */

/** @name GPU memory tags */
/** @{ */
/** @brief Allocate a block of GPU memory */
#define RPI_FW_TAG_ALLOCATE_MEMORY      0x0003000CU
/** @brief Lock a GPU memory block and return its bus address */
#define RPI_FW_TAG_LOCK_MEMORY          0x0003000DU
/** @brief Unlock a GPU memory block */
#define RPI_FW_TAG_UNLOCK_MEMORY        0x0003000EU
/** @brief Release a GPU memory block */
#define RPI_FW_TAG_RELEASE_MEMORY       0x0003000FU
/** @brief Execute code on the VideoCore */
#define RPI_FW_TAG_EXECUTE_CODE         0x00030010U
/** @brief Get dispmanx resource memory handle */
#define RPI_FW_TAG_GET_DISPMANX_MEM_HDL 0x00030014U
/** @brief Get a 128-byte EDID block from attached display */
#define RPI_FW_TAG_GET_EDID_BLOCK       0x00030020U
/** @} */

/** @name GPIO / peripheral register tags */
/** @{ */
/** @brief Get firmware-controlled GPIO pin state */
#define RPI_FW_TAG_GET_GPIO_STATE  0x00030041U
/** @brief Set firmware-controlled GPIO pin state */
#define RPI_FW_TAG_SET_GPIO_STATE  0x00038041U
/** @brief Get firmware-controlled GPIO pin configuration */
#define RPI_FW_TAG_GET_GPIO_CONFIG 0x00030043U
/** @brief Set firmware-controlled GPIO pin configuration */
#define RPI_FW_TAG_SET_GPIO_CONFIG 0x00038043U
/** @brief Read a peripheral register via the firmware */
#define RPI_FW_TAG_GET_PERIPH_REG  0x00030045U
/** @brief Write a peripheral register via the firmware */
#define RPI_FW_TAG_SET_PERIPH_REG  0x00038045U
/** @brief Get PoE HAT fan value */
#define RPI_FW_TAG_GET_POE_HAT_VAL 0x00030049U
/** @brief Set PoE HAT fan value */
#define RPI_FW_TAG_SET_POE_HAT_VAL 0x00038049U
/** @brief Get system timer counter (STC) value */
#define RPI_FW_TAG_GET_STC         0x0003000BU
/** @} */

/** @name Framebuffer tags */
/** @{ */
/** @brief Allocate framebuffer, returns bus address and data_size */
#define RPI_FW_TAG_FB_ALLOCATE_BUFFER     0x00040001U
/** @brief Blank or unblank the display */
#define RPI_FW_TAG_FB_BLANK_SCREEN        0x00040002U
/** @brief Get physical display width and height in pixels */
#define RPI_FW_TAG_FB_GET_PHYSICAL_SIZE   0x00040003U
/** @brief Get virtual framebuffer width and height in pixels */
#define RPI_FW_TAG_FB_GET_VIRTUAL_SIZE    0x00040004U
/** @brief Get framebuffer colour depth in bits per pixel */
#define RPI_FW_TAG_FB_GET_DEPTH           0x00040005U
/** @brief Get pixel colour byte order (RGB or BGR) */
#define RPI_FW_TAG_FB_GET_PIXEL_ORDER     0x00040006U
/** @brief Get alpha channel mode */
#define RPI_FW_TAG_FB_GET_ALPHA_MODE      0x00040007U
/** @brief Get framebuffer pitch in bytes per line */
#define RPI_FW_TAG_FB_GET_PITCH           0x00040008U
/** @brief Get virtual framebuffer X/Y pixel offset */
#define RPI_FW_TAG_FB_GET_VIRTUAL_OFFSET  0x00040009U
/** @brief Get overscan compensation values in pixels */
#define RPI_FW_TAG_FB_GET_OVERSCAN        0x0004000AU
/** @brief Get 256-entry palette */
#define RPI_FW_TAG_FB_GET_PALETTE         0x0004000BU
/** @brief Test physical size without applying it */
#define RPI_FW_TAG_FB_TEST_PHYSICAL_SIZE  0x00044003U
/** @brief Test virtual size without applying it */
#define RPI_FW_TAG_FB_TEST_VIRTUAL_SIZE   0x00044004U
/** @brief Test colour depth without applying it */
#define RPI_FW_TAG_FB_TEST_DEPTH          0x00044005U
/** @brief Test pixel order without applying it */
#define RPI_FW_TAG_FB_TEST_PIXEL_ORDER    0x00044006U
/** @brief Test alpha mode without applying it */
#define RPI_FW_TAG_FB_TEST_ALPHA_MODE     0x00044007U
/** @brief Test virtual offset without applying it */
#define RPI_FW_TAG_FB_TEST_VIRTUAL_OFFSET 0x00044009U
/** @brief Test overscan without applying it */
#define RPI_FW_TAG_FB_TEST_OVERSCAN       0x0004400AU
/** @brief Test palette without applying it */
#define RPI_FW_TAG_FB_TEST_PALETTE        0x0004400BU
/** @brief Release and free the framebuffer */
#define RPI_FW_TAG_FB_RELEASE_BUFFER      0x00048001U
/** @brief Set physical display width and height in pixels */
#define RPI_FW_TAG_FB_SET_PHYSICAL_SIZE   0x00048003U
/** @brief Set virtual framebuffer width and height in pixels */
#define RPI_FW_TAG_FB_SET_VIRTUAL_SIZE    0x00048004U
/** @brief Set framebuffer colour depth in bits per pixel */
#define RPI_FW_TAG_FB_SET_DEPTH           0x00048005U
/** @brief Set pixel colour byte order (RGB or BGR) */
#define RPI_FW_TAG_FB_SET_PIXEL_ORDER     0x00048006U
/** @brief Set alpha channel mode */
#define RPI_FW_TAG_FB_SET_ALPHA_MODE      0x00048007U
/** @brief Set virtual framebuffer X/Y pixel offset */
#define RPI_FW_TAG_FB_SET_VIRTUAL_OFFSET  0x00048009U
/** @brief Set overscan compensation values in pixels */
#define RPI_FW_TAG_FB_SET_OVERSCAN        0x0004800AU
/** @brief Set 256-entry palette */
#define RPI_FW_TAG_FB_SET_PALETTE         0x0004800BU
/** @} */

/**
 * @brief Send a property tag to the VideoCore firmware.
 *
 * Wraps @p data in a ViedoCore mailbox envelope and submits
 * it to the VPU over mailbox channel. Blocks until the VPU responds
 * or a 100 miliseconds timeout elapses.
 *
 * @param dev       Firmware device from DEVICE_DT_GET_ANY(raspberrypi_bcm2711_videocore).
 * @param tag       One of the RPI_FW_TAG_* defines.
 * @param data      Value data (in/out). Updated with firmware response.
 * @param data_size Byte data_size of @p data.
 *
 * @retval 0          Success.
 * @retval -EINVAL    Firmware returned an error status.
 * @retval -ETIMEDOUT Firmware did not respond within 100 miliseconds.
 */
int rpi_fw_transfer(const struct device *dev, uint32_t tag, void *data, uint32_t data_size);

#endif /* ZEPHYR_DRIVERS_FIRMWARE_RPI_FW_RPI_FW_H_ */
