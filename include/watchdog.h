/*
 * Copyright (c) 2015 Intel Corporation.
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

#ifndef _WDT_H_
#define _WDT_H_
#include <stdint.h>

typedef enum { WDT_MODE_RESET = 0, WDT_MODE_INTERRUPT_RESET } wdt_mode_t;

/**
 * WDT configuration struct.
 */
struct wdt_config {
	uint32_t timeout;
	wdt_mode_t mode;
	void (*interrupt_fn)(void);
};

typedef void (*wdt_api_enable)(void);
typedef void (*wdt_api_disable)(void);
typedef int (*wdt_api_set_config)(struct wdt_config *config);
typedef void (*wdt_api_get_config)(struct wdt_config *config);
typedef void (*wdt_api_reload)(void);

struct wdt_driver_api {
	wdt_api_enable enable;
	wdt_api_disable disable;
	wdt_api_get_config get_config;
	wdt_api_set_config set_config;
	wdt_api_reload reload;
};

static inline void wdt_enable(struct device *dev)
{
	struct wdt_driver_api *api;

	api = (struct wdt_driver_api *)dev->driver_api;
	api->enable();
}

static inline void wdt_disable(struct device *dev)
{
	struct wdt_driver_api *api;

	api = (struct wdt_driver_api *)dev->driver_api;
	api->disable();
}

static inline void wdt_get_config(struct device *dev, struct wdt_config *config)
{
	struct wdt_driver_api *api;

	api = (struct wdt_driver_api *)dev->driver_api;
	api->get_config(config);
}

static inline int wdt_set_config(struct device *dev, struct wdt_config *config)
{
	struct wdt_driver_api *api;

	api = (struct wdt_driver_api *)dev->driver_api;
	return api->set_config(config);
}

static inline void wdt_reload(struct device *dev)
{
	struct wdt_driver_api *api;

	api = (struct wdt_driver_api *)dev->driver_api;
	api->reload();
}

#endif
