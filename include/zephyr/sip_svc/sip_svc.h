/*
 * Copyright (c) 2022-2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SIP_SVC_H_
#define ZEPHYR_INCLUDE_SIP_SVC_H_

/**
 * @file
 * @brief Public API for ARM SiP services
 *
 * ARM SiP service provides the capability to send the
 * SMC/HVC call from kernel running at EL1 to hypervisor/secure
 * monitor firmware running at EL2/EL3.
 *
 * Only allow one SMC and one HVC per system.
 *
 * The service support multiple clients.
 *
 * The client must open a channel before sending any request and
 * close the channel immediately after complete. The service only
 * allow one channel at one time.
 *
 * The service will return the SMC/HVC return value to the client
 * via callback function.
 *
 * The client state machine
 * - INVALID: Invalid state before registration.
 * - IDLE   : Initial state.
 * - OPEN   : The client will switch from IDLE to OPEN once it
 *            successfully open the channel. On the other hand, it
 *            will switch from OPEN to IDLE state once it successfully
 *            close the channel.
 * - ABORT  : The client has closed the channel, however, there are
 *            incomplete transactions being left over. The service
 *            will only move the client back to IDLE state once all
 *            transactions completed. The client is not allowed to
 *            re-open the channel when in ABORT state/
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/arm64/arm-smccc.h>
#include <zephyr/drivers/sip_svc/sip_svc_proto.h>

#define SIP_SVC_CLIENT_ST_INVALID 0
#define SIP_SVC_CLIENT_ST_IDLE	  1
#define SIP_SVC_CLIENT_ST_OPEN	  2
#define SIP_SVC_CLIENT_ST_ABORT	  3

/** @brief ARM sip service callback function prototype for response after completion
 *
 * On success , response is returned via a callback to the user.
 *
 * @param c_token Client's token
 * @param res pointer to struct sip_svc_response
 */
typedef void (*sip_svc_cb_fn)(uint32_t c_token, struct sip_svc_response *res);

/**
 * @brief Register a client on ARM SiP service
 *
 * On success, the client will be at IDLE state in the service and
 * the service will return a token to the client. The client can then
 * use the token to open the channel on the service and communicate
 * with hypervisor/secure monitor firmware running at EL2/EL3.
 *
 * @param ctrl Pointer to controller instance whose service provides ARM SMC/HVC
 *            SiP services.
 * @param priv_data Pointer to client private data.
 *
 * @retval token_id on success.
 * @retval SIP_SVC_ID_INVALID invalid arguments, failure to allocate a client id and failure to get
 * a lock.
 */
uint32_t sip_svc_register(void *ctrl, void *priv_data);

/**
 * @brief Unregister a client on ARM SiP service
 *
 * On success, detach the client from the service. Unregistration
 * is only allowed when all transactions belong to the client are closed.
 *
 * @param ctrl Pointer to controller instance which provides ARM SiP services.
 * @param c_token Client's token
 *
 * @retval 0 on success.
 * @retval -EINVALinvalid arguments.
 * @retval -ENODATA if client is not registered correctly.
 * @retval -EBUSY if client has pending transactions.
 * @retval -ECANCELED if client is not in IDLE state.
 * @retval -ENOLCK if failure in acquiring mutex.
 */
int sip_svc_unregister(void *ctrl, uint32_t c_token);

/**
 * @brief Client requests to open a channel on ARM SiP service.
 *
 * Client must open a channel before sending any request via
 * SMC/HVC to hypervisor/secure monitor firmware running at EL2/EL3.
 *
 * The service only allows one opened channel at one time and it is protected
 * by mutex.
 *
 * @param ctrl Pointer to controller instance which provides ARM SiP services.
 * @param c_token Client's token
 * @param k_timeout Waiting time if the mutex have been locked.
 *                   When the mutex have been locked:
 *                   - returns non-zero error code immediately if value is K_NO_WAIT
 *                   - wait forever if the value is K_FOREVER
 *                   - otherwise, for the given time
 *
 * @retval 0 on success.
 * @retval -EINVAL invalid arguments.
 * @retval -ETIMEDOUT timeout expiry.
 * @retval -EALREADY client state is already open.
 */
int sip_svc_open(void *ctrl, uint32_t c_token, k_timeout_t k_timeout);

/**
 * @brief Client requests to close the channel on ARM SiP services.
 *
 * Client must close the channel immediately once complete.
 *
 * @param ctrl Pointer to controller instance which provides ARM SiP services.
 * @param c_token Client's token
 * @param pre_close_req pre close request sent to lower layer on channel close.
 *
 * @retval 0 on success, negative errno on failure.
 * @retval -EINVAL invalid arguments.
 * @retval -ENOTSUP error on sending pre_close_request.
 * @retval -EPROTO client is not in OPEN state.
 */
int sip_svc_close(void *ctrl, uint32_t c_token, struct sip_svc_request *pre_close_req);

/**
 * @brief Client requests to send a SMC/HVC call to EL3/EL2
 *
 * Client must open a channel on the device before using this function.
 * This function is non-blocking and can be called from any context. The
 * service will return a Transaction ID to the client if the request
 * is being accepted. Client callback is called when the transaction is
 * completed.
 *
 * @param ctrl Pointer to controller instance which provides ARM SiP services.
 * @param c_token Client's token
 * @param req Address to the user input in struct sip_svc_request format.
 * @param cb Callback. SMC/SVC return value will be passed to client via
 *           context in struct sip_svc_response format in callback.
 *
 * @retval transaction id on success.
 * @retval -EINVAL invalid arguments.
 * @retval -EOPNOTSUPP invalid command id or function id.
 * @retval -ESRCH invalid client state.
 * @retval -ENOMEM failure to allocate memory.
 * @retval -ENOMSG failure to insert into database.
 * @retval -ENOBUF failure to insert into msgq.
 * @retval -ENOLCK failure to get lock.
 * @retval -EHOSTDOWN sip_svc thread not present.
 * @retval -ENOTSUP check for unsupported condition.
 */
int sip_svc_send(void *ctrl, uint32_t c_token, struct sip_svc_request *req, sip_svc_cb_fn cb);

/**
 * @brief Get the address pointer to the client private data.
 *
 * The pointer is provided by client during registration.
 *
 * @param ctrl Pointer to controller instance which provides ARM SiP service.
 * @param c_token Client's token
 *
 * @retval Address pointer to the client private data.
 * @retval NULL invalid arguments and failure to get lock.
 */
void *sip_svc_get_priv_data(void *ctrl, uint32_t c_token);

/**
 * @brief get the ARM SiP service handle
 *
 * @param method Pointer to controller instance which provides ARM SiP service.
 *
 * @retval Valid pointer.
 * @retval NULL invalid arguments and on providing unsupported method name.
 */
void *sip_svc_get_controller(char *method);

#endif /* ZEPHYR_INCLUDE_SIP_SVC_H_ */
