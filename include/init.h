
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

#ifndef _INIT_H_
#define _INIT_H_

#include <device.h>
#define __used			__attribute__((__used__))

/*! @def __define_initconfig
 *
 *  @brief Define an init object
 *
 *  @details This macro declares an init object to be placed a
 *  given init level section in the image. This macro not should be used
 *  directly.
 *
 *  @param cfg_name Name of the config object created with
 *  DECLARE_DEVICE_INIT_CONFIG() macro that will be referenced by
 *  init object.
 *
 *  @param id The init level id where the init object will be placed
 *  in the image.
 *
 *  @param driver_data The pointer to the driver data for the driver instance.
 *  @sa DECLARE_DEVICE_INIT_CONFIG()
 */
#define __define_initconfig(cfg_name, id, data)			    \
	 static struct device (__initconfig_##cfg_name##id) __used  \
	__attribute__((__section__(".initconfig" #id ".init"))) = { \
		 .config = &(config_##cfg_name),\
		 .driver_data = data};

#define pure_init(cfg, data)		__define_initconfig(cfg, 0, data)
#define nano_early_init(cfg, data)	__define_initconfig(cfg, 1, data)
#define nano_late_init(cfg, data)	__define_initconfig(cfg, 2, data)
#define micro_early_init(cfg, data)	__define_initconfig(cfg, 3, data)
#define micro_late_init(cfg, data)	__define_initconfig(cfg, 4, data)
#define pre_app_init(cfg, data)		__define_initconfig(cfg, 5, data)
#define late_initconfig(cfg, data)	__define_initconfig(cfg, 6, data)


#endif /* _INIT_H_ */
