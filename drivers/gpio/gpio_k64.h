/*
 * Copyright (c) 2016, Wind River Systems, Inc.
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

/**
 * @file Header file for the Freescale K64 GPIO module.
 */

#ifndef _GPIO_K64_H_
#define _GPIO_K64_H_

#include <nanokernel.h>

#include <gpio.h>

/* GPIO Port Register offsets */
#define GPIO_K64_DATA_OUT_OFFSET	0x00  /* Port Data Output */
#define GPIO_K64_SET_OUT_OFFSET		0x04  /* Port Set Output */
#define GPIO_K64_CLR_OUT_OFFSET		0x08  /* Port Clear Output */
#define GPIO_K64_TOGGLE_OUT_OFFSET	0x0C  /* Port Toggle Output */
#define GPIO_K64_DATA_IN_OFFSET		0x10  /* Port Data Input */
#define GPIO_K64_DIR_OFFSET		0x14  /* Port Data Direction */


/** Configuration data */
struct gpio_k64_config {
	/* GPIO module base address */
	uint32_t gpio_base_addr;
	/* Port Control module base address */
	uint32_t port_base_addr;
};

struct gpio_k64_data {
	/* port ISR callback routine address */
	sys_slist_t callbacks;
	/* pin callback routine enable flags, by pin number */
	uint32_t pin_callback_enables;
};
#endif /* _GPIO_K64_H_ */
