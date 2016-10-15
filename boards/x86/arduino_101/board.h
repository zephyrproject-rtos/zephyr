/*
 * Copyright (c) 2015 Intel Corporation
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

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

#define LED0_GPIO_PORT  CONFIG_GPIO_QMSI_0_NAME
#define LED0_GPIO_PIN   2

#if defined(CONFIG_USB)
/* GPIO driver name */
#define USB_GPIO_DRV_NAME	CONFIG_GPIO_QMSI_0_NAME
/* GPIO pin for enabling VBUS */
#define USB_VUSB_EN_GPIO	28
#endif

#endif /* __INC_BOARD_H */
