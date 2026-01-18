/* Copyright 2020-2025 Espressif Systems (Shanghai) CO LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/param.h>
#include <assert.h>
#include "esp_loader_io.h"
#include "esp_loader.h"
#include "common.h"

#define BIN_HEADER_SIZE     0x8
#define BIN_HEADER_EXT_SIZE 0x18
// Maximum block sized for RAM and Flash writes, respectively.
#define ESP_RAM_BLOCK       0x1800

// Only bootloader addresses vary by chip
// DO NOT specify array size - let compiler determine it from initializers
static const uint32_t bootloader_addresses[] = {
    [ESP8266_CHIP] = 0x0,
    [ESP32_CHIP]   = 0x1000,
    [ESP32S2_CHIP] = 0x1000,
    [ESP32C3_CHIP] = 0x0,
    [ESP32S3_CHIP] = 0x0,
    [ESP32C2_CHIP] = 0x0,
    [ESP32C5_CHIP] = 0x2000,
    [ESP32H2_CHIP] = 0x0,
    [ESP32C6_CHIP] = 0x0,
    [ESP32P4_CHIP] = 0x2000
};

// If someone adds a new chip but forgets to update the array, compilation FAILS
_Static_assert(sizeof(bootloader_addresses) / sizeof(bootloader_addresses[0]) == ESP_MAX_CHIP,
               "bootloader_addresses array size mismatch! "
               "If you added a new chip to target_chip_t, you MUST add its address to bootloader_addresses[]");

static const char *get_error_string(const esp_loader_error_t error)
{
    const char *mapping[ESP_LOADER_ERROR_INVALID_RESPONSE + 1] = {
        "NONE", "UNKNOWN", "TIMEOUT", "IMAGE SIZE",
        "INVALID MD5", "INVALID PARAMETER", "INVALID TARGET",
        "UNSUPPORTED CHIP", "UNSUPPORTED FUNCTION", "INVALID RESPONSE"
    };

    assert(error <= ESP_LOADER_ERROR_INVALID_RESPONSE);

    return mapping[error];
}

esp_loader_error_t connect_to_target(uint32_t higher_transmission_rate)
{
    esp_loader_connect_args_t connect_config = ESP_LOADER_CONNECT_DEFAULT();
    esp_loader_error_t err = esp_loader_connect(&connect_config);

    if (err != ESP_LOADER_SUCCESS) {

        printk("Cannot connect to target. Error: %s\n", get_error_string(err));

        if (err == ESP_LOADER_ERROR_TIMEOUT) {
            printk("Check if the host and the target are properly connected.\n");
        } else if (err == ESP_LOADER_ERROR_INVALID_TARGET) {
            printk("You could be using an unsupported chip, or chip revision.\n");
        } else if (err == ESP_LOADER_ERROR_INVALID_RESPONSE) {
            printk("Try lowering the transmission rate or using shorter wires to connect the host and the target.\n");
        }

        return err;
    }
    printk("Connected to target\n");

#if (defined SERIAL_FLASHER_INTERFACE_UART) || (defined SERIAL_FLASHER_INTERFACE_USB)
    if (higher_transmission_rate && esp_loader_get_target() != ESP8266_CHIP) {
        err = esp_loader_change_transmission_rate(higher_transmission_rate);
        if (err == ESP_LOADER_ERROR_UNSUPPORTED_FUNC) {
            printk("ESP8266 does not support change transmission rate command.");
            return err;
        } else if (err != ESP_LOADER_SUCCESS) {
            printk("Unable to change transmission rate on target.");
            return err;
        } else {
            err = loader_port_change_transmission_rate(higher_transmission_rate);
            if (err != ESP_LOADER_SUCCESS) {
                printk("Unable to change transmission rate.");
                return err;
            }
            printk("Transmission rate changed.\n");
        }
    }
#endif /* SERIAL_FLASHER_INTERFACE_UART || SERIAL_FLASHER_INTERFACE_USB */

    return ESP_LOADER_SUCCESS;
}

#if (defined SERIAL_FLASHER_INTERFACE_UART) || (defined SERIAL_FLASHER_INTERFACE_USB)
esp_loader_error_t connect_to_target_with_stub(const uint32_t current_transmission_rate,
        const uint32_t higher_transmission_rate)
{
    esp_loader_connect_args_t connect_config = ESP_LOADER_CONNECT_DEFAULT();

    esp_loader_error_t err = esp_loader_connect_with_stub(&connect_config);
    if (err != ESP_LOADER_SUCCESS) {
        printk("Cannot connect to target. Error: %s\n", get_error_string(err));

        if (err == ESP_LOADER_ERROR_TIMEOUT) {
            printk("Check if the host and the target are properly connected.\n");
        } else if (err == ESP_LOADER_ERROR_INVALID_TARGET) {
            printk("You could be using an unsupported chip, or chip revision.\n");
        } else if (err == ESP_LOADER_ERROR_INVALID_RESPONSE) {
            printk("Try lowering the transmission rate or using shorter wires to connect the host and the target.\n");
        }

        return err;
    }
    printk("Connected to target\n");

    if (higher_transmission_rate != current_transmission_rate) {
        err = esp_loader_change_transmission_rate_stub(current_transmission_rate,
                higher_transmission_rate);

        if (err == ESP_LOADER_ERROR_UNSUPPORTED_FUNC) {
            printk("ESP8266 does not support change transmission rate command.");
            return err;
        } else if (err != ESP_LOADER_SUCCESS) {
            printk("Unable to change transmission rate on target.");
            return err;
        } else {
            err = loader_port_change_transmission_rate(higher_transmission_rate);
            if (err != ESP_LOADER_SUCCESS) {
                printk("Unable to change transmission rate.");
                return err;
            }
            printk("Transmission rate changed.\n");
        }
    }

    return ESP_LOADER_SUCCESS;
}
#endif /* SERIAL_FLASHER_INTERFACE_UART || SERIAL_FLASHER_INTERFACE_USB */

#ifndef SERIAL_FLASHER_INTERFACE_SPI
esp_loader_error_t flash_binary(const uint8_t *bin, size_t size, size_t address)
{
    esp_loader_error_t err;
    static uint8_t payload[1024];
    const uint8_t *bin_addr = bin;

    printk("Erasing flash (this may take a while)...\n");
    err = esp_loader_flash_start(address, size, sizeof(payload));
    if (err != ESP_LOADER_SUCCESS) {
        printk("Erasing flash failed with error: %s.\n", get_error_string(err));

        if (err == ESP_LOADER_ERROR_INVALID_PARAM) {
            printk("If using Secure Download Mode, double check that the specified "
                   "target flash size is correct.\n");
        }
        return err;
    }
    printk("Start programming\n");

    size_t binary_size = size;
    size_t written = 0;

    while (size > 0) {
        size_t to_read = MIN(size, sizeof(payload));
        memcpy(payload, bin_addr, to_read);

        err = esp_loader_flash_write(payload, to_read);
        if (err != ESP_LOADER_SUCCESS) {
            printk("\nPacket could not be written! Error %s.\n", get_error_string(err));
            return err;
        }

        size -= to_read;
        bin_addr += to_read;
        written += to_read;

        int progress = (int)(((float)written / binary_size) * 100);
        printk("\rProgress: %d %%", progress);
    };

    printk("\nFinished programming\n");

#if MD5_ENABLED
    err = esp_loader_flash_verify();
    if (err == ESP_LOADER_ERROR_UNSUPPORTED_FUNC) {
        printk("ESP8266 does not support flash verify command.");
        return err;
    } else if (err != ESP_LOADER_SUCCESS) {
        printk("MD5 does not match. Error: %s\n", get_error_string(err));
        return err;
    }
    printk("Flash verified\n");
#endif

    return ESP_LOADER_SUCCESS;
}
#endif /* SERIAL_FLASHER_INTERFACE_SPI */

uint32_t get_bootloader_address(target_chip_t chip)
{
    return bootloader_addresses[chip];
}


esp_loader_error_t load_ram_binary(const uint8_t *bin)
{
    printk("Start loading\n");
    esp_loader_error_t err;
    const esp_loader_bin_header_t *header = (const esp_loader_bin_header_t *)bin;
    esp_loader_bin_segment_t segments[header->segments];

    // Parse segments
    uint32_t seg;
    uint32_t *cur_seg_pos;
    // ESP8266 does not have extended header
    uint32_t offset = esp_loader_get_target() == ESP8266_CHIP ? BIN_HEADER_SIZE : BIN_HEADER_EXT_SIZE;
    for (seg = 0, cur_seg_pos = (uint32_t *)(&bin[offset]);
            seg < header->segments;
            seg++) {
        segments[seg].addr = *cur_seg_pos++;
        segments[seg].size = *cur_seg_pos++;
        segments[seg].data = (uint8_t *)cur_seg_pos;
        cur_seg_pos += (segments[seg].size) / 4;
    }

    // Download segments
    for (seg = 0; seg < header->segments; seg++) {
        printk("Downloading %"PRIu32" bytes at 0x%08"PRIx32"...\n", segments[seg].size, segments[seg].addr);

        err = esp_loader_mem_start(segments[seg].addr, segments[seg].size, ESP_RAM_BLOCK);
        if (err != ESP_LOADER_SUCCESS) {
            printk("Loading to RAM could not be started. Error: %s.\n", get_error_string(err));

            if (err == ESP_LOADER_ERROR_INVALID_PARAM) {
                printk("Check if the chip has Secure Download Mode enabled.\n");
            }
            return err;
        }

        size_t remain_size = segments[seg].size;
        const uint8_t *data_pos = segments[seg].data;
        while (remain_size > 0) {
            size_t data_size = MIN(ESP_RAM_BLOCK, remain_size);
            err = esp_loader_mem_write(data_pos, data_size);
            if (err != ESP_LOADER_SUCCESS) {
                printk("\nPacket could not be written! Error: %s.\n", get_error_string(err));
                return err;
            }
            data_pos += data_size;
            remain_size -= data_size;
        }
    }

    err = esp_loader_mem_finish(header->entrypoint);
    if (err != ESP_LOADER_SUCCESS) {
        printk("\nLoading to RAM finished with error: %s.\n", get_error_string(err));
        return err;
    }
    printk("\nFinished loading\n");

    return ESP_LOADER_SUCCESS;
}
