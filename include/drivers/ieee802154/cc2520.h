/*
 * Copyright (c) 2016 Intel Corporation
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

#include <device.h>

enum cc2520_gpio_index {
	CC2520_GPIO_IDX_VREG_EN	= 0,
	CC2520_GPIO_IDX_RESET,
	CC2520_GPIO_IDX_FIFO,
	CC2520_GPIO_IDX_CCA,
	CC2520_GPIO_IDX_SFD,
	CC2520_GPIO_IDX_FIFOP,

	CC2520_GPIO_IDX_MAX,
};

struct cc2520_gpio_configuration {
	struct device *dev;
	uint32_t pin;
};

struct cc2520_gpio_configuration *cc2520_configure_gpios(void);
