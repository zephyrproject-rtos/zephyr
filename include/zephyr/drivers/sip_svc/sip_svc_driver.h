/*
 * Copyright (c) 2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SIP_SVC_DRIVER_H_
#define ZEPHYR_INCLUDE_SIP_SVC_DRIVER_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/arm64/arm-smccc.h>
#include <zephyr/drivers/sip_svc/sip_svc_proto.h>

#define DEV_API(dev) ((struct svc_driver_api *)(dev)->api)

/**
 * @brief Length of SVC conduit name.
 *
 */
#define SVC_CONDUIT_NAME_LENGTH (4)

/**
 * @brief Callback API for executing the supervisory call
 * See @a sip_supervisory_call() for argument description
 */
typedef void (*sip_supervisory_call_t)(const struct device *dev, unsigned long function_id,
				       unsigned long arg0, unsigned long arg1, unsigned long arg2,
				       unsigned long arg3, unsigned long arg4, unsigned long arg5,
				       unsigned long arg6, struct arm_smccc_res *res);

/**
 * @brief Callback API for validating function id for the supervisory call.
 * See @a sip_svc_plat_func_id_valid() for argument description
 */
typedef bool (*sip_svc_plat_func_id_valid_t)(const struct device *dev, uint32_t command,
					     uint32_t func_id);

/**
 * @brief Callback API for generating the transaction id from client id.
 * See @a sip_svc_plat_format_trans_id() for argument description
 */
typedef uint32_t (*sip_svc_plat_format_trans_id_t)(const struct device *dev, uint32_t client_idx,
						   uint32_t trans_idx);

/**
 * @brief Callback API for retrieving client transaction id from transaction id
 * See @a sip_svc_plat_get_trans_idx() for argument description
 */
typedef uint32_t (*sip_svc_plat_get_trans_idx_t)(const struct device *dev, uint32_t trans_id);

/**
 * @brief Callback API for updating transaction id for request packet for lower layer
 * See @a sip_svc_plat_update_trans_id() for argument description
 */
typedef void (*sip_svc_plat_update_trans_id_t)(const struct device *dev,
					       struct sip_svc_request *request, uint32_t trans_id);

/**
 * @brief Callback API for freeing command buffer in ASYNC packets
 * See @a sip_svc_plat_free_async_memory() for argument description
 */
typedef void (*sip_svc_plat_free_async_memory_t)(const struct device *dev,
						 struct sip_svc_request *request);

/**
 * @brief Callback API to construct Polling packet for ASYNC transaction.
 * See @a sip_svc_plat_async_res_req() for argument description
 */
typedef int (*sip_svc_plat_async_res_req_t)(const struct device *dev, unsigned long *a0,
					    unsigned long *a1, unsigned long *a2, unsigned long *a3,
					    unsigned long *a4, unsigned long *a5, unsigned long *a6,
					    unsigned long *a7, char *buf, size_t size);

/**
 * @brief Callback API to check the response of polling request
 * See @a sip_svc_plat_async_res_res() for argument description
 */
typedef int (*sip_svc_plat_async_res_res_t)(const struct device *dev, struct arm_smccc_res *res,
					    char *buf, size_t *size, uint32_t *trans_id);

/**
 * @brief Callback API for retrieving error code from a supervisory call response.
 * See @a sip_svc_plat_get_error_code() for argument description.
 */
typedef uint32_t (*sip_svc_plat_get_error_code_t)(const struct device *dev,
						  struct arm_smccc_res *res);

/**
 * @brief  API structure for sip_svc driver.
 *
 */
__subsystem struct svc_driver_api {
	sip_supervisory_call_t sip_supervisory_call;
	sip_svc_plat_func_id_valid_t sip_svc_plat_func_id_valid;
	sip_svc_plat_format_trans_id_t sip_svc_plat_format_trans_id;
	sip_svc_plat_get_trans_idx_t sip_svc_plat_get_trans_idx;
	sip_svc_plat_update_trans_id_t sip_svc_plat_update_trans_id;
	sip_svc_plat_free_async_memory_t sip_svc_plat_free_async_memory;
	sip_svc_plat_async_res_req_t sip_svc_plat_async_res_req;
	sip_svc_plat_async_res_res_t sip_svc_plat_async_res_res;
	sip_svc_plat_get_error_code_t sip_svc_plat_get_error_code;
};

/**
 * @brief supervisory call function which will execute the smc/hvc call
 *
 * @param dev  Pointer to the device structure for the driver instance.
 * @param function_id  Function identifier for the supervisory call.
 * @param arg0  Argument 0 for supervisory call.
 * @param arg1  Argument 1 for supervisory call.
 * @param arg2  Argument 2 for supervisory call.
 * @param arg3  Argument 3 for supervisory call.
 * @param arg4  Argument 4 for supervisory call.
 * @param arg5  Argument 5 for supervisory call.
 * @param arg6  Argument 6 for supervisory call.
 * @param res Pointer to response buffer for supervisory call.
 */
__syscall void sip_supervisory_call(const struct device *dev, unsigned long function_id,
				    unsigned long arg0, unsigned long arg1, unsigned long arg2,
				    unsigned long arg3, unsigned long arg4, unsigned long arg5,
				    unsigned long arg6, struct arm_smccc_res *res);
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

/**
 * @brief Validate the function id for the supervisory call.
 *
 * @param dev  Pointer to the device structure for the driver instance.
 * @param command  Command which specify if the call is SYNC or ASYNC.
 * @param func_id  Function identifier
 *
 * @retval true if command and function identifiers are valid.
 * @retval false if command and function identifiers are invalid.
 */
__syscall bool sip_svc_plat_func_id_valid(const struct device *dev, uint32_t command,
					  uint32_t func_id);
static inline bool z_impl_sip_svc_plat_func_id_valid(const struct device *dev, uint32_t command,
						     uint32_t func_id)
{
	__ASSERT(dev, "dev shouldn't be NULL");
	const struct svc_driver_api *api = DEV_API(dev);

	__ASSERT(api->sip_svc_plat_func_id_valid,
		 "sip_svc_plat_func_id_valid func shouldn't be NULL");

	return api->sip_svc_plat_func_id_valid(dev, command, func_id);
}

/**
 * @brief Formats and generates the transaction id from client id.
 *
 * @param dev  Pointer to the device structure for the driver instance.
 * @param client_idx client index.
 * @param trans_idx transaction index.
 *
 * @retval transaction id, which is used for tracking each transaction.
 */
__syscall uint32_t sip_svc_plat_format_trans_id(const struct device *dev, uint32_t client_idx,
						uint32_t trans_idx);
static inline uint32_t z_impl_sip_svc_plat_format_trans_id(const struct device *dev,
							   uint32_t client_idx, uint32_t trans_idx)
{
	__ASSERT(dev, "dev shouldn't be NULL");
	const struct svc_driver_api *api = DEV_API(dev);

	__ASSERT(api->sip_svc_plat_format_trans_id,
		 "sip_svc_plat_format_trans_id func shouldn't be NULL");

	return api->sip_svc_plat_format_trans_id(dev, client_idx, trans_idx);
}

/**
 * @brief Retrieve client transaction id from packet transaction id.
 *
 * @param dev  Pointer to the device structure for the driver instance.
 * @param trans_id transaction identifier if for a transaction.
 *
 * @retval client transaction id form Transaction id.
 */
__syscall uint32_t sip_svc_plat_get_trans_idx(const struct device *dev, uint32_t trans_id);
static inline uint32_t z_impl_sip_svc_plat_get_trans_idx(const struct device *dev,
							 uint32_t trans_id)
{
	__ASSERT(dev, "dev shouldn't be NULL");
	const struct svc_driver_api *api = DEV_API(dev);

	__ASSERT(api->sip_svc_plat_get_trans_idx,
		 "sip_svc_plat_get_trans_idx func shouldn't be NULL");

	return api->sip_svc_plat_get_trans_idx(dev, trans_id);
}

/**
 * @brief Update transaction id for sip_svc_request for lower layer.
 *
 * @param dev  Pointer to the device structure for the driver instance.
 * @param request  Pointer to sip_svc_request structure.
 * @param trans_id Transaction id.
 */
__syscall void sip_svc_plat_update_trans_id(const struct device *dev,
					    struct sip_svc_request *request, uint32_t trans_id);
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

/**
 * @brief Retrieve the error code from arm_smccc_res response.
 *
 * @param dev  Pointer to the device structure for the driver instance.
 * @param res  Pointer to struct arm_smccc_res response.
 *
 * @retval 0 on success.
 * @retval SIP_SVC_ID_INVALID on failure
 */
__syscall uint32_t sip_svc_plat_get_error_code(const struct device *dev, struct arm_smccc_res *res);
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

/**
 * @brief Set arguments for polling supervisory call. For ASYNC polling of response.
 *
 * @param dev  Pointer to the device structure for the driver instance.
 * @param a0  Argument 0 for supervisory call.
 * @param a1  Argument 1 for supervisory call.
 * @param a2  Argument 2 for supervisory call.
 * @param a3  Argument 3 for supervisory call.
 * @param a4  Argument 4 for supervisory call.
 * @param a5  Argument 5 for supervisory call.
 * @param a6  Argument 6 for supervisory call.
 * @param a7  Argument 7 for supervisory call.
 * @param buf Pointer for response buffer.
 * @param size Size of response buffer.
 *
 * @retval  0 on success
 */
__syscall int sip_svc_plat_async_res_req(const struct device *dev, unsigned long *a0,
					 unsigned long *a1, unsigned long *a2, unsigned long *a3,
					 unsigned long *a4, unsigned long *a5, unsigned long *a6,
					 unsigned long *a7, char *buf, size_t size);
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

/**
 * @brief Check the response of polling supervisory call and retrieve the response
 *        size and transaction id.
 *
 * @param dev  Pointer to the device structure for the driver instance.
 * @param res  Pointer to struct arm_smccc_res response.
 * @param buf  Pointer to response buffer.
 * @param size  Size of response in response buffer
 * @param trans_id  Transaction id of the response.
 *
 * @retval 0 on getting a valid polling response from supervisory call.
 * @retval -EINPROGRESS on no valid polling response from supervisory call.
 */
__syscall int sip_svc_plat_async_res_res(const struct device *dev, struct arm_smccc_res *res,
					 char *buf, size_t *size, uint32_t *trans_id);
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

/**
 * @brief Free the command buffer used for ASYNC packet after sending it to lower layers.
 *
 * @param dev  Pointer to the device structure for the driver instance.
 * @param request Pointer to sip_svc_request packet.
 */
__syscall void sip_svc_plat_free_async_memory(const struct device *dev,
					      struct sip_svc_request *request);
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


#include <syscalls/sip_svc_driver.h>

#endif /* ZEPHYR_INCLUDE_SIP_SVC_DRIVER_H_ */
