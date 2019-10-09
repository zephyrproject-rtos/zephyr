/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __PIN_CONFIG_JLF_H__
#define __PIN_CONFIG_JLF_H__

#define DT_ILITEK_ILI9340_0_BUS_NAME "SPI_2"
#define DT_ILITEK_ILI9340_0_SPI_MAX_FREQUENCY 10*1000

#define DT_ILITEK_ILI9340_0_BASE_ADDRESS      1
#define DT_ILITEK_ILI9340_0_RESET_GPIOS_CONTROLLER "GPIO_0"
#define DT_ILITEK_ILI9340_0_RESET_GPIOS_PIN 5
#define DT_ILITEK_ILI9340_0_CMD_DATA_GPIOS_CONTROLLER "GPIO_0"
#define DT_ILITEK_ILI9340_0_CMD_DATA_GPIOS_PIN 4

#define XPT2046_SPI_DEVICE_NAME "SPI_2"
#define XPT2046_SPI_MAX_FREQUENCY 10*1000
#define XPT2046_CS_GPIO_CONTROLLER "GPIO_0"
#define XPT2046_CS_GPIO_PIN         6

#define XPT2046_PEN_GPIO_CONTROLLER "GPIO_0"
#define XPT2046_PEN_GPIO_PIN         7

#define HOST_DEVICE_COMM_UART_NAME "UART_1"
#endif /* __PIN_CONFIG_JLF_H__ */
