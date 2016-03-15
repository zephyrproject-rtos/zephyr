/* pinmux_galileo.h */

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

#ifndef __PINMUX_GALILEO_PRIV_H
#define __PINMUX_GALILEO_PRIV_H

struct galileo_data {
	struct device *exp0;
	struct device *exp1;
	struct device *exp2;
	struct device *pwm0;

	/* GPIO<0>..GPIO<7> */
	struct device *gpio_dw;

	/* GPIO<8>..GPIO<9>, which means to pin 0 and 1 on core well. */
	struct device *gpio_core;

	/* GPIO_SUS<0>..GPIO_SUS<5> */
	struct device *gpio_resume;

	struct pin_config *mux_config;
};

struct galileo_data galileo_pinmux_driver;

int _galileo_pinmux_set_pin(struct device *port, uint8_t pin, uint32_t func);

int _galileo_pinmux_get_pin(struct device *port, uint32_t pin, uint32_t *func);

#endif /* __PINMUX_GALILEO_PRIV_H */
