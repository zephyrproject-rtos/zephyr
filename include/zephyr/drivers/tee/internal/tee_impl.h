/*
 * Copyright (c) 2023 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Inline syscall implementation for tee driver APIs.
 */

/*
 * Copyright (c) 2015-2016, Linaro Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
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

#ifndef ZEPHYR_INCLUDE_DRIVERS_TEE_H_
#error "Should only be included by zephyr/drivers/tee.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_TEE_INTERNAL_TEE_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_TEE_INTERNAL_TEE_IMPL_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

static inline int z_impl_tee_get_version(const struct device *dev, struct tee_version_info *info)
{
	const struct tee_driver_api *api = (const struct tee_driver_api *)dev->api;

	if (!api->get_version) {
		return -ENOSYS;
	}

	return api->get_version(dev, info);
}

static inline int z_impl_tee_open_session(const struct device *dev,
					  struct tee_open_session_arg *arg,
					  unsigned int num_param, struct tee_param *param,
					  uint32_t *session_id)
{
	const struct tee_driver_api *api = (const struct tee_driver_api *)dev->api;

	if (!api->open_session) {
		return -ENOSYS;
	}

	return api->open_session(dev, arg, num_param, param, session_id);
}

static inline int z_impl_tee_close_session(const struct device *dev, uint32_t session_id)
{
	const struct tee_driver_api *api = (const struct tee_driver_api *)dev->api;

	if (!api->close_session) {
		return -ENOSYS;
	}

	return api->close_session(dev, session_id);
}

static inline int z_impl_tee_cancel(const struct device *dev, uint32_t session_id,
				    uint32_t cancel_id)
{
	const struct tee_driver_api *api = (const struct tee_driver_api *)dev->api;

	if (!api->cancel) {
		return -ENOSYS;
	}

	return api->cancel(dev, session_id, cancel_id);
}

static inline int z_impl_tee_invoke_func(const struct device *dev, struct tee_invoke_func_arg *arg,
					 unsigned int num_param, struct tee_param *param)
{
	const struct tee_driver_api *api = (const struct tee_driver_api *)dev->api;

	if (!api->invoke_func) {
		return -ENOSYS;
	}

	return api->invoke_func(dev, arg, num_param, param);
}

static inline int z_impl_tee_shm_register(const struct device *dev, void *addr, size_t size,
					  uint32_t flags, struct tee_shm **shm)
{
	flags &= ~TEE_SHM_ALLOC;
	return tee_add_shm(dev, addr, 0, size, flags | TEE_SHM_REGISTER, shm);
}

static inline int z_impl_tee_shm_unregister(const struct device *dev, struct tee_shm *shm)
{
	return tee_rm_shm(dev, shm);
}

static inline int z_impl_tee_shm_alloc(const struct device *dev, size_t size, uint32_t flags,
				       struct tee_shm **shm)
{
	return tee_add_shm(dev, NULL, 0, size, flags | TEE_SHM_ALLOC | TEE_SHM_REGISTER, shm);
}

static inline int z_impl_tee_shm_free(const struct device *dev, struct tee_shm *shm)
{
	return tee_rm_shm(dev, shm);
}

static inline int z_impl_tee_suppl_recv(const struct device *dev, uint32_t *func,
					unsigned int *num_params, struct tee_param *param)
{
	const struct tee_driver_api *api = (const struct tee_driver_api *)dev->api;

	if (!api->suppl_recv) {
		return -ENOSYS;
	}

	return api->suppl_recv(dev, func, num_params, param);
}

static inline int z_impl_tee_suppl_send(const struct device *dev, unsigned int ret,
					unsigned int num_params, struct tee_param *param)
{
	const struct tee_driver_api *api = (const struct tee_driver_api *)dev->api;

	if (!api->suppl_send) {
		return -ENOSYS;
	}

	return api->suppl_send(dev, ret, num_params, param);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_TEE_INTERNAL_TEE_IMPL_H_ */
