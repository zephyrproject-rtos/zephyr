
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

/*! @def DECLARE_DEVICE_INIT_CONFIG
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
	};\

struct device;

/* Static device infomation (In ROM) Per driver instance */
struct device_config {
	/*! name of the device */
	char	*name;
	/*! init function for the driver */
	int (*init)(struct device *device);
	/*! address of driver instance config information */
	void *config_info;
};

/* Runtime device structure (In memory) Per driver instance */
struct device {
	/*! Build time config information */
	struct device_config *config;
	/*! pointer to structure containing the API functions for the
	 * device type. This pointer is filled in by the driver at
	 * init time.
	 */
	void *driver_api;
	/*! Driver instance data. For driver use only*/
	void *driver_data;
};

void device_do_config_level(int level);
struct device* device_get_binding(char *name);

#endif /* _DEVICE_H_ */
