/*
 * Copyright (c) 2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SIP_SVC_DRIVER_H_
#error "Should only be included by zephyr/drivers/sip_svc/sip_svc_driver.h"
#endif

#ifndef ZEPHYR_INCLUDE_DRIVERS_SIP_SVC_DRIVER_INTERNAL_IMPL_H_
#define ZEPHYR_INCLUDE_DRIVERS_SIP_SVC_DRIVER_INTERNAL_IMPL_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/arm64/arm-smccc.h>
#include <zephyr/drivers/sip_svc/sip_svc_proto.h>
#include <zephyr/sip_svc/sip_svc_controller.h>

static inline void z_impl_sip_supervisory_call(const struct device *dev, unsigned long function_id,
					       unsigned long arg0, unsigned long arg1,
					       unsigned long arg2, unsigned long arg3,
					       unsigned long arg4, unsigned long arg5,
					       unsigned long arg6, struct arm_smccc_res *res)
{
	__ASSERT(dev, "dev shouldn't be NULL");
	const struct svc_driver_api *api = DEV_API(dev);

	__ASSERT(api->sip_supervisory_call, "sip_supervisory_call shouldn't be NULL");
	__ASSERT(res, "response pointer shouldn't be NULL");

	api->sip_supervisory_call(dev, function_id, arg0, arg1, arg2, arg3, arg4, arg5, arg6, res);
}

static inline bool z_impl_sip_svc_plat_func_id_valid(const struct device *dev, uint32_t command,
						     uint32_t func_id)
{
	__ASSERT(dev, "dev shouldn't be NULL");
	const struct svc_driver_api *api = DEV_API(dev);

	__ASSERT(api->sip_svc_plat_func_id_valid,
		 "sip_svc_plat_func_id_valid func shouldn't be NULL");

	return api->sip_svc_plat_func_id_valid(dev, command, func_id);
}

static inline uint32_t z_impl_sip_svc_plat_format_trans_id(const struct device *dev,
							   uint32_t client_idx, uint32_t trans_idx)
{
	__ASSERT(dev, "dev shouldn't be NULL");
	const struct svc_driver_api *api = DEV_API(dev);

	__ASSERT(api->sip_svc_plat_format_trans_id,
		 "sip_svc_plat_format_trans_id func shouldn't be NULL");

	return api->sip_svc_plat_format_trans_id(dev, client_idx, trans_idx);
}

static inline uint32_t z_impl_sip_svc_plat_get_trans_idx(const struct device *dev,
							 uint32_t trans_id)
{
	__ASSERT(dev, "dev shouldn't be NULL");
	const struct svc_driver_api *api = DEV_API(dev);

	__ASSERT(api->sip_svc_plat_get_trans_idx,
		 "sip_svc_plat_get_trans_idx func shouldn't be NULL");

	return api->sip_svc_plat_get_trans_idx(dev, trans_id);
}

static inline void z_impl_sip_svc_plat_update_trans_id(const struct device *dev,
						       struct sip_svc_request *request,
						       uint32_t trans_id)
{
	__ASSERT(dev, "dev shouldn't be NULL");
	const struct svc_driver_api *api = DEV_API(dev);

	__ASSERT(api->sip_svc_plat_update_trans_id,
		 "sip_svc_plat_update_trans_id func shouldn't be NULL");
	__ASSERT(request, "request shouldn't be NULL");

	return api->sip_svc_plat_update_trans_id(dev, request, trans_id);
}

static inline uint32_t z_impl_sip_svc_plat_get_error_code(const struct device *dev,
							  struct arm_smccc_res *res)
{
	__ASSERT(dev, "dev shouldn't be NULL");
	const struct svc_driver_api *api = DEV_API(dev);

	__ASSERT(api->sip_svc_plat_get_error_code,
		 "sip_svc_plat_get_error_code func shouldn't be NULL");
	__ASSERT(res, "res shouldn't be NULL");

	return api->sip_svc_plat_get_error_code(dev, res);
}

static inline int z_impl_sip_svc_plat_async_res_req(const struct device *dev, unsigned long *a0,
						    unsigned long *a1, unsigned long *a2,
						    unsigned long *a3, unsigned long *a4,
						    unsigned long *a5, unsigned long *a6,
						    unsigned long *a7, char *buf, size_t size)
{
	__ASSERT(dev, "dev shouldn't be NULL");
	const struct svc_driver_api *api = DEV_API(dev);

	__ASSERT(api->sip_svc_plat_async_res_req,
		 "sip_svc_plat_async_res_req func shouldn't be NULL");
	__ASSERT(a0, "a0 shouldn't be NULL");
	__ASSERT(a1, "a1 shouldn't be NULL");
	__ASSERT(a2, "a2 shouldn't be NULL");
	__ASSERT(a3, "a3 shouldn't be NULL");
	__ASSERT(a4, "a4 shouldn't be NULL");
	__ASSERT(a5, "a5 shouldn't be NULL");
	__ASSERT(a6, "a6 shouldn't be NULL");
	__ASSERT(a7, "a7 shouldn't be NULL");
	__ASSERT(((buf == NULL && size == 0) || (buf != NULL && size != 0)),
		 "buf and size should represent a buffer");
	return api->sip_svc_plat_async_res_req(dev, a0, a1, a2, a3, a4, a5, a6, a7, buf, size);
}

static inline int z_impl_sip_svc_plat_async_res_res(const struct device *dev,
						    struct arm_smccc_res *res, char *buf,
						    size_t *size, uint32_t *trans_id)
{
	__ASSERT(dev, "dev shouldn't be NULL");
	const struct svc_driver_api *api = DEV_API(dev);

	__ASSERT(api->sip_svc_plat_async_res_res,
		 "sip_svc_plat_async_res_res func shouldn't be NULL");
	__ASSERT(res, "res shouldn't be NULL");
	__ASSERT(buf, "buf shouldn't be NULL");
	__ASSERT(size, "size shouldn't be NULL");
	__ASSERT(trans_id, "buf shouldn't be NULL");

	return api->sip_svc_plat_async_res_res(dev, res, buf, size, trans_id);
}

static inline void z_impl_sip_svc_plat_free_async_memory(const struct device *dev,
							 struct sip_svc_request *request)
{
	__ASSERT(dev, "dev shouldn't be NULL");
	const struct svc_driver_api *api = DEV_API(dev);

	__ASSERT(api->sip_svc_plat_free_async_memory,
		 "sip_svc_plat_free_async_memory func shouldn't be NULL");
	__ASSERT(request, "request shouldn't be NULL");

	api->sip_svc_plat_free_async_memory(dev, request);
}

#endif /* ZEPHYR_INCLUDE_DRIVERS_SIP_SVC_DRIVER_INTERNAL_IMPL_H_ */
