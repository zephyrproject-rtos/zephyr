/*
 * Copyright (c) 2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SIP_SVC_PROTO_H_
#define ZEPHYR_INCLUDE_SIP_SVC_PROTO_H_

/**
 * @file
 * @brief Arm SiP services communication protocol
 *        between service provider and client.
 *
 * Client to fill in the input data in struct sip_svc_request format
 * when requesting SMC/HVC service via 'send' function.
 *
 * Service to fill in the SMC/HVC return value in struct sip_svc_response
 * format and pass to client via Callback.
 */

/**
 *  @brief Invalid id value
 */
#define SIP_SVC_ID_INVALID 0xFFFFFFFF

/** @brief Header format
 */

#define SIP_SVC_PROTO_VER 0x0

#define SIP_SVC_PROTO_HEADER_CODE_OFFSET 0
#define SIP_SVC_PROTO_HEADER_CODE_MASK	 0xFFFF

#define SIP_SVC_PROTO_HEADER_TRANS_ID_OFFSET 16
#define SIP_SVC_PROTO_HEADER_TRANS_ID_MASK   0xFF

#define SIP_SVC_PROTO_HEADER_VER_OFFSET 30
#define SIP_SVC_PROTO_HEADER_VER_MASK	0x3

#define SIP_SVC_PROTO_HEADER(code, trans_id)                                                       \
	((((code)&SIP_SVC_PROTO_HEADER_CODE_MASK) << SIP_SVC_PROTO_HEADER_CODE_OFFSET) |           \
	 (((trans_id)&SIP_SVC_PROTO_HEADER_TRANS_ID_MASK)                                          \
	  << SIP_SVC_PROTO_HEADER_TRANS_ID_OFFSET) |                                               \
	 ((SIP_SVC_PROTO_VER & SIP_SVC_PROTO_HEADER_VER_MASK) << SIP_SVC_PROTO_HEADER_VER_OFFSET))

#define SIP_SVC_PROTO_HEADER_GET_CODE(header)                                                      \
	(((header) >> SIP_SVC_PROTO_HEADER_CODE_OFFSET) & SIP_SVC_PROTO_HEADER_CODE_MASK)

#define SIP_SVC_PROTO_HEADER_GET_TRANS_ID(header)                                                  \
	(((header) >> SIP_SVC_PROTO_HEADER_TRANS_ID_OFFSET) & SIP_SVC_PROTO_HEADER_TRANS_ID_MASK)

#define SIP_SVC_PROTO_HEADER_SET_TRANS_ID(header, trans_id)                                        \
	(header) &= ~(SIP_SVC_PROTO_HEADER_TRANS_ID_MASK << SIP_SVC_PROTO_HEADER_TRANS_ID_OFFSET); \
	(header) |= (((trans_id)&SIP_SVC_PROTO_HEADER_TRANS_ID_MASK)                               \
		     << SIP_SVC_PROTO_HEADER_TRANS_ID_OFFSET);

/** @brief Arm SiP services command code in request header
 *
 * SIP_SVC_PROTO_CMD_SYNC
 *     - Typical flow, synchronous request. Service expects EL3/EL2 firmware to
 *       return the result immediately during SMC/HVC call.
 *
 * SIP_SVC_PROTO_CMD_ASYNC
 *     - Asynchronous request. Service is required to poll the response via a
 *       separate SMC/HVC call. Use this method if the request requires longer
 *       processing in EL3/EL2.
 */

#define SIP_SVC_PROTO_CMD_SYNC	0x0
#define SIP_SVC_PROTO_CMD_ASYNC 0x1
#define SIP_SVC_PROTO_CMD_MAX	SIP_SVC_PROTO_CMD_ASYNC

/** @brief Error code in response header
 *
 * SIP_SVC_PROTO_STATUS_OK
 *     - Successfully execute the request.
 *
 * SIP_SVC_PROTO_STATUS_UNKNOWN
 *     - Unrecognized SMC/HVC Function ID.
 *
 * SIP_SVC_PROTO_STATUS_BUSY
 *     - The request is still in progress. Please try again.
 *
 * SIP_SVC_PROTO_STATUS_REJECT
 *     - The request have been rejected due to improper input data.
 *
 * SIP_SVC_PROTO_STATUS_NO_RESPONSE
 *     - No response from target hardware yet.
 *
 * SIP_SVC_PROTO_STATUS_ERROR
 *     - Error occurred when executing the request.
 *
 * SIP_SVC_PROTO_STATUS_NOT_SUPPORT
 *     - Unsupported Arm SiP services command code
 */

#define SIP_SVC_PROTO_STATUS_OK		 0x0
#define SIP_SVC_PROTO_STATUS_UNKNOWN	 0xFFFF
#define SIP_SVC_PROTO_STATUS_BUSY	 0x1
#define SIP_SVC_PROTO_STATUS_REJECT	 0x2
#define SIP_SVC_PROTO_STATUS_NO_RESPONSE 0x3
#define SIP_SVC_PROTO_STATUS_ERROR	 0x4

/** @brief SiP Service communication protocol
 *         request format.
 *
 * request header
 *     - bits [15: 0] Arm SiP services command code
 *     - bits [23:16] Transaction ID (Filled in by sip_svc service)
 *     - bits [29:24] Unused. Reserved.
 *     - bits [31:30] Arm SiP services communication protocol version
 *
 * a0 - a7
 *     - User input data to be filled into a0-a7 registers when trigger
 *       SMC/HVC
 *
 * resp_data_addr
 *     - This parameter only used by asynchronous command.
 *     - Dynamic memory address for service to put the asynchronous response
 *       data. The service will free this memory space if the client has
 *       cancelled the transaction.
 *
 * resp_data_size
 *     - This parameter only used by asynchronous command.
 *     - Maximum memory size in bytes of resp_data_addr
 *
 * priv_data
 *     - Memory address to client context. Service will pass this address back
 *       to client in response format via callback.
 */

struct sip_svc_request {
	uint32_t header;
	unsigned long a0;
	unsigned long a1;
	unsigned long a2;
	unsigned long a3;
	unsigned long a4;
	unsigned long a5;
	unsigned long a6;
	unsigned long a7;
	uint64_t resp_data_addr;
	uint32_t resp_data_size;
	void *priv_data;
};

/** @brief SiP Services service communication protocol
 *         response format.
 *
 * response header
 *     - bits [15: 0] Error code
 *     - bits [23:16] Transaction ID
 *     - bits [29:24] Unused. Reserved.
 *     - bits [31:30] Arm SiP services communication protocol version
 *
 * a0 - a3
 *     - SMC/HVC return value
 *
 * resp_data_addr
 *     - This parameter only used by asynchronous command.
 *     - Dynamic memory address that put the asynchronous response data.
 *       This address is provided by client during request. Client is responsible
 *       to free the memory space when receive the callback of a asynchronous
 *       command transaction.The memory needs to be dynamically allocated,
 *       the framework will free the allocated memory if the channel is in ABORT
 *       state.
 *
 * resp_data_size
 *     - This parameter only used by asynchronous command.
 *     - Valid data size in bytes of resp_data_addr
 *
 * priv_data
 *     - Memory address to client context which given during request.
 */

struct sip_svc_response {
	uint32_t header;
	unsigned long a0;
	unsigned long a1;
	unsigned long a2;
	unsigned long a3;
	uint64_t resp_data_addr;
	uint32_t resp_data_size;
	void *priv_data;
};

#endif /* ZEPHYR_INCLUDE_SIP_SVC_PROTO_H_ */
