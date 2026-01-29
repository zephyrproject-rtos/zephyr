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

#pragma once

#include "esp_loader.h"

// Standard flash addresses that are the same for all chips
#define PARTITION_TABLE_ADDRESS  0x8000
#define APPLICATION_ADDRESS      0x10000

esp_loader_error_t connect_to_target(uint32_t higher_transmission_rate);
esp_loader_error_t connect_to_target_with_stub(uint32_t current_transmission_rate,
        uint32_t higher_transmission_rate);
esp_loader_error_t flash_binary(const uint8_t *bin, size_t size, size_t address);
esp_loader_error_t load_ram_binary(const uint8_t *bin);

// Address helper functions
uint32_t get_bootloader_address(target_chip_t chip);
