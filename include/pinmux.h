/* pinmux.h - the private pinmux driver header */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __INCLUDE_PINMUX_H
#define __INCLUDE_PINMUX_H

#include <stdint.h>
#include <device.h>

#define PINMUX_NAME		"pinmux"

#define PINMUX_FUNC_A		0
#define PINMUX_FUNC_B		1
#define PINMUX_FUNC_C		2
#define PINMUX_FUNC_D		3

typedef uint32_t (*pmux_set)(struct device *dev, uint32_t pin, uint8_t func);
typedef uint32_t (*pmux_get)(struct device *dev, uint32_t pin, uint8_t *func);

struct pinmux_driver_api {
	pmux_set set;
	pmux_get get;
};


static inline uint32_t pinmux_set_pin(struct device *dev,
				      uint32_t pin,
				      uint8_t func)
{
	struct pinmux_driver_api *api;

	api = (struct pinmux_driver_api *) dev->driver_api;
	return api->set(dev, pin, func);
}

static inline uint32_t pinmux_get_pin(struct device *dev,
				      uint32_t pin,
				      uint8_t *func)
{
	struct pinmux_driver_api *api;

	api = (struct pinmux_driver_api *) dev->driver_api;
	return api->get(dev, pin, func);
}

#endif /* __INCLUDE_PINMUX_H */
