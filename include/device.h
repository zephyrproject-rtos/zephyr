
/* Copyright (c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
 * @param driver_api pointer to structure containing the API unctions for
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
