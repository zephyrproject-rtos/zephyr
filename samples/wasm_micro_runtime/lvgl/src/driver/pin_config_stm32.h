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
#ifndef __PIN_CONFIG_STM32_H__
#define __PIN_CONFIG_STM32_H__

#define DT_ILITEK_ILI9340_0_BUS_NAME "SPI_1"
#define DT_ILITEK_ILI9340_0_SPI_MAX_FREQUENCY 24*1000*1000

#define DT_ILITEK_ILI9340_0_BASE_ADDRESS      1
#define DT_ILITEK_ILI9340_0_RESET_GPIOS_CONTROLLER "GPIOC"
#define DT_ILITEK_ILI9340_0_RESET_GPIOS_PIN    12
#define DT_ILITEK_ILI9340_0_CMD_DATA_GPIOS_CONTROLLER "GPIOC"
#define DT_ILITEK_ILI9340_0_CMD_DATA_GPIOS_PIN 11

#define DT_ILITEK_ILI9340_0_CS_GPIO_CONTROLLER  "GPIOC"
#define DT_ILITEK_ILI9340_0_CS_GPIO_PIN            10

#define XPT2046_SPI_DEVICE_NAME "SPI_1"
#define XPT2046_SPI_MAX_FREQUENCY 12*1000*1000
#define XPT2046_CS_GPIO_CONTROLLER "GPIOD"
#define XPT2046_CS_GPIO_PIN         0

#define XPT2046_PEN_GPIO_CONTROLLER "GPIOD"
#define XPT2046_PEN_GPIO_PIN         1

#define HOST_DEVICE_COMM_UART_NAME "UART_6"

#endif /* __PIN_CONFIG_STM32_H__ */
