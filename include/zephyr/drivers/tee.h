/*
 * Copyright (c) 2023 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup tee_interface
 * @brief Main header file for TEE (Trusted Execution Environment) driver API.
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
#define ZEPHYR_INCLUDE_DRIVERS_TEE_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/**
 * @brief Interfaces to work with Trusted Execution Environment (TEE).
 * @defgroup tee_interface TEE
 * @ingroup io_interfaces
 * @{
 *
 * The generic interface to work with Trusted Execution Environment (TEE).
 * TEE is Trusted OS, running in the Secure Space, such as TrustZone in ARM cpus.
 * It also can be presented as the separate secure co-processors. It allows system
 * to implement logic, separated from the Normal World.
 *
 * Using TEE syscalls:
 * - tee_get_version() to get current TEE capabilities
 * - tee_open_session() to open new session to the TA
 * - tee_close_session() to close session to the TA
 * - tee_cancel() to cancel session or invoke function
 * - tee_invoke_func() to invoke function to the TA
 * - tee_shm_register() to register shared memory region
 * - tee_shm_unregister() to unregister shared memory region
 * - tee_shm_alloc() to allocate shared memory region
 * - tee_shm_free() to free shared memory region
 */

#ifdef __cplusplus
extern "C" {
#endif
#define TEE_UUID_LEN 16

#define TEE_GEN_CAP_GP		BIT(0) /* GlobalPlatform compliant TEE */
#define TEE_GEN_CAP_PRIVILEGED	BIT(1) /* Privileged device (for supplicant) */
#define TEE_GEN_CAP_REG_MEM	BIT(2) /* Supports registering shared memory */
#define TEE_GEN_CAP_MEMREF_NULL BIT(3) /* Support NULL MemRef */

#define TEE_SHM_REGISTER	BIT(0)
#define TEE_SHM_ALLOC		BIT(1)

#define TEE_PARAM_ATTR_TYPE_NONE		0 /* parameter not used */
#define TEE_PARAM_ATTR_TYPE_VALUE_INPUT		1
#define TEE_PARAM_ATTR_TYPE_VALUE_OUTPUT	2
#define TEE_PARAM_ATTR_TYPE_VALUE_INOUT		3 /* input and output */
#define TEE_PARAM_ATTR_TYPE_MEMREF_INPUT	5
#define TEE_PARAM_ATTR_TYPE_MEMREF_OUTPUT	6
#define TEE_PARAM_ATTR_TYPE_MEMREF_INOUT	7 /* input and output */
#define TEE_PARAM_ATTR_TYPE_MASK		0xff
#define TEE_PARAM_ATTR_META			0x100
#define TEE_PARAM_ATTR_MASK			(TEE_PARAM_ATTR_TYPE_MASK | TEE_PARAM_ATTR_META)

/**
 * @brief Function error origins, of type TEEC_ErrorOrigin. These indicate where in
 * the software stack a particular return value originates from.
 *
 * TEEC_ORIGIN_API         The error originated within the TEE Client API
 *                         implementation.
 * TEEC_ORIGIN_COMMS       The error originated within the underlying
 *                         communications stack linking the rich OS with
 *                         the TEE.
 * TEEC_ORIGIN_TEE         The error originated within the common TEE code.
 * TEEC_ORIGIN_TRUSTED_APP The error originated within the Trusted Application
 *                         code.
 */
#define TEEC_ORIGIN_API		0x00000001
#define TEEC_ORIGIN_COMMS	0x00000002
#define TEEC_ORIGIN_TEE		0x00000003
#define TEEC_ORIGIN_TRUSTED_APP 0x00000004

/**
 * Return values. Type is TEEC_Result
 *
 * TEEC_SUCCESS                 The operation was successful.
 * TEEC_ERROR_GENERIC           Non-specific cause.
 * TEEC_ERROR_ACCESS_DENIED     Access privileges are not sufficient.
 * TEEC_ERROR_CANCEL            The operation was canceled.
 * TEEC_ERROR_ACCESS_CONFLICT   Concurrent accesses caused conflict.
 * TEEC_ERROR_EXCESS_DATA       Too much data for the requested operation was
 *                              passed.
 * TEEC_ERROR_BAD_FORMAT        Input data was of invalid format.
 * TEEC_ERROR_BAD_PARAMETERS    Input parameters were invalid.
 * TEEC_ERROR_BAD_STATE         Operation is not valid in the current state.
 * TEEC_ERROR_ITEM_NOT_FOUND    The requested data item is not found.
 * TEEC_ERROR_NOT_IMPLEMENTED   The requested operation should exist but is not
 *                              yet implemented.
 * TEEC_ERROR_NOT_SUPPORTED     The requested operation is valid but is not
 *                              supported in this implementation.
 * TEEC_ERROR_NO_DATA           Expected data was missing.
 * TEEC_ERROR_OUT_OF_MEMORY     System ran out of resources.
 * TEEC_ERROR_BUSY              The system is busy working on something else.
 * TEEC_ERROR_COMMUNICATION     Communication with a remote party failed.
 * TEEC_ERROR_SECURITY          A security fault was detected.
 * TEEC_ERROR_SHORT_BUFFER      The supplied buffer is too short for the
 *                              generated output.
 * TEEC_ERROR_TARGET_DEAD       Trusted Application has panicked
 *                              during the operation.
 */

/**
 *  Standard defined error codes.
 */
#define TEEC_SUCCESS				0x00000000
#define TEEC_ERROR_STORAGE_NOT_AVAILABLE	0xF0100003
#define TEEC_ERROR_GENERIC			0xFFFF0000
#define TEEC_ERROR_ACCESS_DENIED		0xFFFF0001
#define TEEC_ERROR_CANCEL			0xFFFF0002
#define TEEC_ERROR_ACCESS_CONFLICT		0xFFFF0003
#define TEEC_ERROR_EXCESS_DATA			0xFFFF0004
#define TEEC_ERROR_BAD_FORMAT			0xFFFF0005
#define TEEC_ERROR_BAD_PARAMETERS		0xFFFF0006
#define TEEC_ERROR_BAD_STATE			0xFFFF0007
#define TEEC_ERROR_ITEM_NOT_FOUND		0xFFFF0008
#define TEEC_ERROR_NOT_IMPLEMENTED		0xFFFF0009
#define TEEC_ERROR_NOT_SUPPORTED		0xFFFF000A
#define TEEC_ERROR_NO_DATA			0xFFFF000B
#define TEEC_ERROR_OUT_OF_MEMORY		0xFFFF000C
#define TEEC_ERROR_BUSY				0xFFFF000D
#define TEEC_ERROR_COMMUNICATION		0xFFFF000E
#define TEEC_ERROR_SECURITY			0xFFFF000F
#define TEEC_ERROR_SHORT_BUFFER			0xFFFF0010
#define TEEC_ERROR_EXTERNAL_CANCEL		0xFFFF0011
#define TEEC_ERROR_TARGET_DEAD			0xFFFF3024
#define TEEC_ERROR_STORAGE_NO_SPACE		0xFFFF3041

/**
 * Session login methods, for use in tee_open_session() as parameter
 * connectionMethod. Type is uint32_t.
 *
 * TEEC_LOGIN_PUBLIC            No login data is provided.
 * TEEC_LOGIN_USER              Login data about the user running the Client
 *                              Application process is provided.
 * TEEC_LOGIN_GROUP             Login data about the group running the Client
 *                              Application process is provided.
 * TEEC_LOGIN_APPLICATION       Login data about the running Client Application
 *                              itself is provided.
 * TEEC_LOGIN_USER_APPLICATION  Login data about the user and the running
 *                              Client Application itself is provided.
 * TEEC_LOGIN_GROUP_APPLICATION Login data about the group and the running
 *                              Client Application itself is provided.
 */
#define TEEC_LOGIN_PUBLIC		0x00000000
#define TEEC_LOGIN_USER			0x00000001
#define TEEC_LOGIN_GROUP		0x00000002
#define TEEC_LOGIN_APPLICATION		0x00000004
#define TEEC_LOGIN_USER_APPLICATION	0x00000005
#define TEEC_LOGIN_GROUP_APPLICATION	0x00000006

/**
 * @brief TEE version
 *
 * Identifies the TEE implementation,@ref impl_id is one of TEE_IMPL_ID_* above.
 * @ref impl_caps is implementation specific, for example TEE_OPTEE_CAP_*
 * is valid when @ref impl_id == TEE_IMPL_ID_OPTEE.
 */
struct tee_version_info {
	uint32_t impl_id; /**< [out] TEE implementation id */
	uint32_t impl_caps; /**< [out] implementation specific capabilities */
	uint32_t gen_caps;  /**< Generic capabilities, defined by TEE_GEN_CAPS_* above */
};

 /**
  * @brief - Open session argument
  */
struct tee_open_session_arg {
	uint8_t uuid[TEE_UUID_LEN]; /**< [in] UUID of the Trusted Application */
	uint8_t clnt_uuid[TEE_UUID_LEN]; /**< [in] UUID of client */
	uint32_t clnt_login; /**< login class of client, TEE_IOCTL_LOGIN_* above */
	uint32_t cancel_id; /**< [in] cancellation id, a unique value to identify this request */
	uint32_t session; /**< [out] session id */
	uint32_t ret; /**< [out] return value */
	uint32_t ret_origin; /**< [out] origin of the return value */
};

/**
 * @brief Tee parameter
 *
 * @ref attr & TEE_PARAM_ATTR_TYPE_MASK indicates if memref or value is used in
 * the union. TEE_PARAM_ATTR_TYPE_VALUE_* indicates value and
 * TEE_PARAM_ATTR_TYPE_MEMREF_* indicates memref. TEE_PARAM_ATTR_TYPE_NONE
 * indicates that none of the members are used.
 *
 * Shared memory is allocated with TEE_IOC_SHM_ALLOC which returns an
 * identifier representing the shared memory object. A memref can reference
 * a part of a shared memory by specifying an offset (@ref a) and size (@ref b) of
 * the object. To supply the entire shared memory object set the offset
 * (@ref a) to 0 and size (@ref b) to the previously returned size of the object.
 */
struct tee_param {
	uint64_t attr; /**< attributes */
	uint64_t a; /**< if a memref, offset into the shared memory object, else a value*/
	uint64_t b; /**< if a memref, size of the buffer, else a value parameter */
	uint64_t c; /**< if a memref, shared memory identifier, else a value parameter */
};

/**
 * @brief Invokes a function in a Trusted Application
 */
struct tee_invoke_func_arg {
	uint32_t func;       /**< [in] Trusted Application function, specific to the TA */
	uint32_t session;    /**< [in] session id */
	uint32_t cancel_id;  /**< [in] cancellation id, a unique value to identify this request */
	uint32_t ret;        /**< [out] return value */
	uint32_t ret_origin; /**< [out] origin of the return value */
};

/**
 * @brief Tee shared memory structure
 */
struct tee_shm {
	const struct device *dev; /**< [out] pointer to the device driver structure */
	void *addr;               /**< [out] shared buffer pointer */
	uint64_t size;            /**< [out] shared buffer size */
	uint32_t flags;           /**< [out] shared buffer flags */
};

/**
 * @typedef tee_get_version_t
 *
 * @brief Callback API to get current tee version
 *
 * See @a tee_version_get() for argument definitions.
 */
typedef int (*tee_get_version_t)(const struct device *dev, struct tee_version_info *info);

/**
 * @typedef tee_open_session_t
 *
 * @brief Callback API to open session to Trusted Application
 *
 * See @a tee_open_session() for argument definitions.
 */
typedef int (*tee_open_session_t)(const struct device *dev, struct tee_open_session_arg *arg,
				  unsigned int num_param, struct tee_param *param,
				  uint32_t *session_id);
/**
 * @typedef tee_close_session_t
 *
 * @brief Callback API to close session to TA
 *
 * See @a tee_close_session() for argument definitions.
 */
typedef int (*tee_close_session_t)(const struct device *dev, uint32_t session_id);

/**
 * @typedef tee_cancel_t
 *
 * @brief Callback API to cancel open session of invoke function to TA
 *
 * See @a tee_cancel() for argument definitions.
 */
typedef int (*tee_cancel_t)(const struct device *dev, uint32_t session_id, uint32_t cancel_id);

/**
 * @typedef tee_invoke_func_t
 *
 * @brief Callback API to invoke function to TA
 *
 * See @a tee_invoke_func() for argument definitions.
 */
typedef int (*tee_invoke_func_t)(const struct device *dev, struct tee_invoke_func_arg *arg,
				 unsigned int num_param, struct tee_param *param);
/**
 * @typedef tee_shm_register_t
 *
 * @brief Callback API to register shared memory
 *
 * See @a tee_shm_register() for argument definitions.
 */
typedef int (*tee_shm_register_t)(const struct device *dev, struct tee_shm *shm);

/**
 * @typedef tee_shm_unregister_t
 *
 * @brief Callback API to unregister shared memory
 *
 * See @a tee_shm_unregister() for argument definitions.
 */
typedef int (*tee_shm_unregister_t)(const struct device *dev, struct tee_shm *shm);

/**
 * @typedef tee_suppl_recv_t
 *
 * @brief Callback API to receive a request for TEE supplicant
 *
 * See @a tee_suppl_recv() for argument definitions.
 */
typedef int (*tee_suppl_recv_t)(const struct device *dev, uint32_t *func, unsigned int *num_params,
				struct tee_param *param);

/**
 * @typedef tee_suppl_send_t
 *
 * @brief Callback API to send a request for TEE supplicant
 *
 * See @a tee_suppl_send() for argument definitions.
 */
typedef int (*tee_suppl_send_t)(const struct device *dev, unsigned int ret, unsigned int num_params,
				struct tee_param *param);

__subsystem struct tee_driver_api {
	tee_get_version_t get_version;
	tee_open_session_t open_session;
	tee_close_session_t close_session;
	tee_cancel_t cancel;
	tee_invoke_func_t invoke_func;
	tee_shm_register_t shm_register;
	tee_shm_unregister_t shm_unregister;
	tee_suppl_recv_t suppl_recv;
	tee_suppl_send_t suppl_send;
};

/**
 * @brief Get the current TEE version info
 *
 * Returns info as tee version info which includes capabilities description
 *
 * @param dev TEE device
 * @param info Structure to return the capabilities
 *
 * @retval -ENOSYS If callback was not implemented
 *
 * @retval 0       On success, negative on error
 */
__syscall int tee_get_version(const struct device *dev, struct tee_version_info *info);

static inline int z_impl_tee_get_version(const struct device *dev, struct tee_version_info *info)
{
	const struct tee_driver_api *api = (const struct tee_driver_api *)dev->api;

	if (!api->get_version) {
		return -ENOSYS;
	}

	return api->get_version(dev, info);
}

/**
 * @brief Open session for Trusted Environment
 *
 * Opens the new session to the Trusted Environment
 *
 * @param dev TEE device
 * @param arg Structure with the session arguments
 * @param num_param Number of the additional params to be passed
 * @param param List of the params to pass to open_session call
 * @param session_id Returns id of the created session
 *
 * @retval -ENOSYS If callback was not implemented
 *
 * @retval 0       On success, negative on error
 */
__syscall int tee_open_session(const struct device *dev, struct tee_open_session_arg *arg,
			       unsigned int num_param, struct tee_param *param,
			       uint32_t *session_id);

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

/**
 * @brief Close session for Trusted Environment
 *
 * Closes session to the Trusted Environment
 *
 * @param dev TEE device
 * @param session_id session to close
 *
 * @retval -ENOSYS If callback was not implemented
 *
 * @retval 0       On success, negative on error
 */
__syscall int tee_close_session(const struct device *dev, uint32_t session_id);

static inline int z_impl_tee_close_session(const struct device *dev, uint32_t session_id)
{
	const struct tee_driver_api *api = (const struct tee_driver_api *)dev->api;

	if (!api->close_session) {
		return -ENOSYS;
	}

	return api->close_session(dev, session_id);
}

/**
 * @brief Cancel session or invoke function for Trusted Environment
 *
 * Cancels session or invoke function for TA
 *
 * @param dev TEE device
 * @param session_id session to close
 * @param cancel_id cancel reason
 *
 * @retval -ENOSYS If callback was not implemented
 *
 * @retval 0       On success, negative on error
 */
__syscall int tee_cancel(const struct device *dev, uint32_t session_id, uint32_t cancel_id);

static inline int z_impl_tee_cancel(const struct device *dev, uint32_t session_id,
				    uint32_t cancel_id)
{
	const struct tee_driver_api *api = (const struct tee_driver_api *)dev->api;

	if (!api->cancel) {
		return -ENOSYS;
	}

	return api->cancel(dev, session_id, cancel_id);
}

/**
 * @brief Invoke function for Trusted Environment Application
 *
 * Invokes function to the TA
 *
 * @param dev TEE device
 * @param arg Structure with the invoke function arguments
 * @param num_param Number of the additional params to be passed
 * @param param List of the params to pass to open_session call
 *
 * @retval -ENOSYS If callback was not implemented
 *
 * @retval 0       On success, negative on error
 */
__syscall int tee_invoke_func(const struct device *dev, struct tee_invoke_func_arg *arg,
			      unsigned int num_param, struct tee_param *param);

static inline int z_impl_tee_invoke_func(const struct device *dev, struct tee_invoke_func_arg *arg,
					 unsigned int num_param, struct tee_param *param)
{
	const struct tee_driver_api *api = (const struct tee_driver_api *)dev->api;

	if (!api->invoke_func) {
		return -ENOSYS;
	}

	return api->invoke_func(dev, arg, num_param, param);
}

/**
 * @brief Helper function to allocate and register shared memory
 *
 * Allocates and registers shared memory for TEE
 *
 * @param dev TEE device
 * @param addr Address of the shared memory
 * @param align Region alignment
 * @param size Size of the shared memory region
 * @param flags Flags to set registering parameters
 * @param shmp Return shared memory structure
 *
 * @retval 0       On success, negative on error
 */
int tee_add_shm(const struct device *dev, void *addr, size_t align, size_t size, uint32_t flags,
		struct tee_shm **shmp);

/**
 * @brief Helper function to remove and unregister shared memory
 *
 * Removes and unregisters shared memory for TEE
 *
 * @param dev TEE device
 * @param shm Pointer to tee_shm structure
 *
 * @retval 0       On success, negative on error
 */
int tee_rm_shm(const struct device *dev, struct tee_shm *shm);

/**
 * @brief Register shared memory for Trusted Environment
 *
 * Registers shared memory for TEE
 *
 * @param dev TEE device
 * @param addr Address of the shared memory
 * @param size Size of the shared memory region
 * @param flags Flags to set registering parameters
 * @param shm Return shared memory structure
 *
 * @retval -ENOSYS If callback was not implemented
 *
 * @retval 0       On success, negative on error
 */
__syscall int tee_shm_register(const struct device *dev, void *addr, size_t size,
			       uint32_t flags, struct tee_shm **shm);

static inline int z_impl_tee_shm_register(const struct device *dev, void *addr, size_t size,
					  uint32_t flags, struct tee_shm **shm)
{
	flags &= ~TEE_SHM_ALLOC;
	return tee_add_shm(dev, addr, 0, size, flags | TEE_SHM_REGISTER, shm);
}

/**
 * @brief Unregister shared memory for Trusted Environment
 *
 * Unregisters shared memory for TEE
 *
 * @param dev TEE device
 * @param shm Shared memory structure
 *
 * @retval -ENOSYS If callback was not implemented
 *
 * @retval 0       On success, negative on error
 */
__syscall int tee_shm_unregister(const struct device *dev, struct tee_shm *shm);

static inline int z_impl_tee_shm_unregister(const struct device *dev, struct tee_shm *shm)
{
	return tee_rm_shm(dev, shm);
}

/**
 * @brief Allocate shared memory region for Trusted Environment
 *
 * Allocate shared memory for TEE
 *
 * @param dev TEE device
 * @param size Region size
 * @param flags to allocate region
 * @param shm Return shared memory structure
 *
 * @retval -ENOSYS If callback was not implemented
 *
 * @retval 0       On success, negative on error
 */
__syscall int tee_shm_alloc(const struct device *dev, size_t size, uint32_t flags,
			    struct tee_shm **shm);

static inline int z_impl_tee_shm_alloc(const struct device *dev, size_t size, uint32_t flags,
				       struct tee_shm **shm)
{
	return tee_add_shm(dev, NULL, 0, size, flags | TEE_SHM_ALLOC | TEE_SHM_REGISTER, shm);
}

/**
 * @brief Free shared memory region for Trusted Environment
 *
 * Frees shared memory for TEE
 *
 * @param dev TEE device
 * @param shm Shared memory structure
 *
 * @retval -ENOSYS If callback was not implemented
 *
 * @retval 0       On success, negative on error
 */
__syscall int tee_shm_free(const struct device *dev, struct tee_shm *shm);

static inline int z_impl_tee_shm_free(const struct device *dev, struct tee_shm *shm)
{
	return tee_rm_shm(dev, shm);
}

/**
 * @brief Receive a request for TEE Supplicant
 *
 * @param dev TEE device
 * @param func Supplicant function
 * @param num_params Number of parameters to be passed
 * @param param List of the params for send/receive
 *
 * @retval -ENOSYS If callback was not implemented
 *
 * @retval 0       On success, negative on error
 */
__syscall int tee_suppl_recv(const struct device *dev, uint32_t *func, unsigned int *num_params,
			     struct tee_param *param);

static inline int z_impl_tee_suppl_recv(const struct device *dev, uint32_t *func,
					unsigned int *num_params, struct tee_param *param)
{
	const struct tee_driver_api *api = (const struct tee_driver_api *)dev->api;

	if (!api->suppl_recv) {
		return -ENOSYS;
	}

	return api->suppl_recv(dev, func, num_params, param);
}

/**
 * @brief Send a request for TEE Supplicant function
 *
 * @param dev TEE device
 * @param ret supplicant return code
 * @param num_params Number of parameters to be passed
 * @param param List of the params for send/receive
 *
 * @retval -ENOSYS If callback was not implemented
 * @retval         Return value for sent request
 *
 * @retval 0       On success, negative on error
 */
__syscall int tee_suppl_send(const struct device *dev, unsigned int ret, unsigned int num_params,
			     struct tee_param *param);

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

/**
 * @}
 */

#include <zephyr/syscalls/tee.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_TEE_H_ */
