
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

#ifndef _DEVICE_H_
#define _DEVICE_H_

/** @def DECLARE_DEVICE_INIT_CONFIG
 *
 *  @brief Define an config object
 *
 *  @details This macro declares an config object to be placed in the
 *  image by the linker in the ROM region.
 *
 *  @param cfg_name Name of the config object to be created. This name
 *  must be used in the *_init() macro(s) defined in init.h so the
 *  linker can associate the config object with the correct init
 *  object.
 *
 *  @param drv_name The name this instance of the driver exposes to
 *  the system.
 *  @param init_fn Address to the init function of the driver.
 *  @param config The address to the structure containing the
 *  configuration information for this instance of the driver.
 *
 *  @sa __define_initconfig()
 */
#define DECLARE_DEVICE_INIT_CONFIG(cfg_name, drv_name, init_fn, config) \
	static struct device_config config_##cfg_name	__used		\
	__attribute__((__section__(".devconfig.init"))) = { \
		.name = drv_name, .init = (init_fn), \
		.config_info = (config)				   \
	}

/* Common Error Codes devices can provide */
#define DEV_OK			0  /* No error */
#define DEV_FAIL		1 /* General operation failure */
#define DEV_INVALID_OP		2 /* Invalid operation */
#define DEV_INVALID_CONF	3 /* Invalid configuration */
#define DEV_USED		4 /* Device controller in use */
#define DEV_NO_ACCESS		5 /* Controller not accessible */
#define DEV_NO_SUPPORT		6 /* Device type not supported */
#define DEV_NOT_CONFIG		7 /* Device not configured */

struct device;

/**
 * @brief Static device information (In ROM) Per driver instance
 * @param name name of the device
 * @param init init function for the driver
 * @param config_info address of driver instance config information
 */
struct device_config {
	char	*name;
	int (*init)(struct device *device);
	void *config_info;
};

/**
 * @brief Runtime device structure (In memory) Per driver instance
 * @param device_config Build time config information
 * @param driver_api pointer to structure containing the API functions for
 * the device type. This pointer is filled in by the driver at init time.
 * @param driver_data river instance data. For driver use only
 */
struct device {
	struct device_config *config;
	void *driver_api;
	void *driver_data;
};

void _sys_device_do_config_level(int level);
struct device* device_get_binding(char *name);

#endif /* _DEVICE_H_ */
