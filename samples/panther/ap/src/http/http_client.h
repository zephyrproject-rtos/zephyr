/**
 * \file
 *
 * \brief HTTP client service.
 *
 * Copyright (c) 2016 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

/**
 * \defgroup sam0_httpc_group HTTP client service
 *
 * This module provides implementation of HTTP client 1.1 for WINC1500 board. 
 *
 * Detailed description of HTTP, please refer to the following documents.<br>
 * http://tools.ietf.org/html/rfc2616
 *
 * Revision history
 * 2014/10/07 : Initial draft. (v1.0.0)
 * 2014/12/17 : Add extension header and fix some issues. (v1.0.1)
 * 2015/01/07 : Optimize the WINC driver 18.0 (v1.0.2)
 * @{
 */

#ifndef HTTP_CLEINT_H_INCLUDED
#define HTTP_CLEINT_H_INCLUDED

#include "socket/include/socket.h"
#include "common/include/nm_common.h"
#include "iot/sw_timer.h"
#include "http_entity.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Protocol version string of HTTP client. */
#define HTTP_PROTO_NAME               "HTTP/1.1"
/** Max size of URI. */
#define HTTP_MAX_URI_LENGTH           64

/**
 * \brief A type of HTTP method.
 */
enum http_method {
	/* Method type of GET. */
	HTTP_METHOD_GET = 1,
	/* Method type of POST. */
	HTTP_METHOD_POST,
	/* Method type of DELETE. */
	HTTP_METHOD_DELETE,
	/* Method type of PUT. */
	HTTP_METHOD_PUT,
	/* Method type of OPTIONS. */
	HTTP_METHOD_OPTIONS,
	/* Method type of HEAD. */
	HTTP_METHOD_HEAD,
};

/**
 * \brief A type of HTTP client callback.
 */
enum http_client_callback_type {
	/** 
	 * Socket was connected. 
	 * After received this event, try send request message to the server. 
	 */
	HTTP_CLIENT_CALLBACK_SOCK_CONNECTED,
	/** The request operation is completed. */
	HTTP_CLIENT_CALLBACK_REQUESTED,
	/** Received HTTP response message. */
	HTTP_CLIENT_CALLBACK_RECV_RESPONSE,
	/** Received Chunked data. */
	HTTP_CLIENT_CALLBACK_RECV_CHUNKED_DATA,
	/** The session was closed. */
	HTTP_CLIENT_CALLBACK_DISCONNECTED,
};

/**
 * \brief Structure of the HTTP_CLIENT_CALLBACK_SOCK_CONNECTED callback.
 */
struct http_client_data_sock_connected {
	/** 
	 * Result of operation. 
	 *
	 * \return     -ENOENT         No such address.
	 * \return     -EINVAL         Invalid argument.
	 * \return     -ENOSPC         No space left on device.
	 * \return     -EIO            Device was occurred error due to unknown exception.
	 * \return     -EDESTADDRREQ   Destination address required.
	 * \return     -ECONNRESET     Connection reset by peer.
	 * \return     -EAGAIN         Try again.
	 * \return     -EBUSY          Device or resource busy.
	 * \return     -EADDRINUSE     Address already in use.
	 * \return     -EALREADY       Socket already connected.
	 * \return     -ENOTCONN       Service in bad state.
	 * \return     -ECONNREFUSED   Connection refused.
	 * \return     -EOVERFLOW      Value too large for defined data type.
	 * \return     -EBADMSG        Not a data message.
	 */
	int result;
};

/**
 * \brief Structure of the HTTP_CLIENT_CALLBACK_REQUESTED callback.
 */
struct http_client_data_requested {
	int dummy;
};

/**
 * \brief Structure of the HTTP_CLIENT_CALLBACK_RECV_RESPONSE callback.
 */
struct http_client_data_recv_response {
	/** 
	 * Response code of HTTP request. 
	 * Refer to following link.
	 * http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
	 */
	uint16_t response_code;
	/** If this flag is set to zero, This data is used chunked encoding. */
	uint8_t is_chunked;
	/** Length of entity. */
	uint32_t content_length;
	/** 
	 * Content buffer. 
	 * If this value is equal to zero, it means This data is too big compared with the receive buffer.
	 * In this situation, Data will be transmitted through HTTP_CLIENT_CALLBACK_RECV_CHUNKED_DATA callback.
	 */
	char *content;
};

/**
 * \brief Structure of the HTTP_CLIENT_CALLBACK_RECV_CHUNKED_DATA callback.
 */
struct http_client_data_recv_chunked_data {
	/** Length of chunked data. */
	uint32_t length;
	/** Buffer of data. */
	char *data;
	/** A flag for the indicating whether the last data. */
	char is_complete;
};

/**
 * \brief Structure of the HTTP_CLIENT_CALLBACK_DISCONNECTED callback.
 */
struct http_client_data_disconnected {
	/** 
	 * Reason of disconnecting.
	 *
	 * \return     -ENOENT         No such address.
	 * \return     -EINVAL         Invalid argument.
	 * \return     -ENOSPC         No space left on device.
	 * \return     -EIO            Device was occurred error due to unknown exception.
	 * \return     -EDESTADDRREQ   Destination address required.
	 * \return     -ECONNRESET     Connection reset by peer.
	 * \return     -EAGAIN         Try again.
	 * \return     -EBUSY          Device or resource busy.
	 * \return     -EADDRINUSE     Address already in use.
	 * \return     -EALREADY       Socket already connected.
	 * \return     -ENOTCONN       Service in bad state.
	 * \return     -ECONNREFUSED   Connection refused.
	 * \return     -EOVERFLOW      Value too large for defined data type.
	 * \return     -EBADMSG        Not a data message.
	 * \return     -ENOTSUP        Unsupported operation.
	 */
	int reason;
};

/**
 * \brief Structure of the HTTP client callback.
 */
union http_client_data {
	struct http_client_data_sock_connected sock_connected;
	struct http_client_data_requested requested;
	struct http_client_data_recv_response recv_response;
	struct http_client_data_recv_chunked_data recv_chunked_data;
	struct http_client_data_disconnected disconnected;
};

/* Before declaring for the callback type. */
struct http_client_module;
/**
 * \brief Callback interface of HTTP client service.
 *
 * \param[in]  module_inst     Module instance of HTTP client module.
 * \param[in]  type            Type of event.
 * \param[in]  data            Data structure of the event. \refer http_client_data
 */
typedef void (*http_client_callback_t)(struct http_client_module *module_inst, int type, union http_client_data *data);

/**
 * \brief HTTP client configuration structure
 *
 * Configuration struct for a HTTP client instance. This structure should be
 * initialized by the \ref http_client_get_config_defaults function before being
 * modified by the user application.
 */
struct http_client_config {
	/** 
	 * TCP port number of HTTP. 
	 * Default value is 80.
	 */
	uint16_t port;
	/**
	 * A flag for the whether using the TLS socket or not. 
	 * Default value is 0.
	 */
	uint8_t tls;
	/** 
	 * Timer module for the request timeout
	 * Default value is NULL.
	 */
	struct sw_timer_module *timer_inst;
	/** 
	 * Time value for the request time out. 
	 * Unit is milliseconds.
	 * Default value is 20000. (20 seconds)
	 */
	uint16_t timeout;
	/** 
	 * Rx buffer. 
	 * Default value is NULL.
	 */
	char *recv_buffer;
	/** 
	 * Maximum size of the receive buffer. 
	 * Default value is 256.
	 */
	uint32_t recv_buffer_size;
	/** 
	 * Send buffer size in the HTTP client service. 
	 * This buffer is located in the stack.
	 * Therefore, The size of the buffer increases the speed will increase, but it may cause a stack overflow.
	 * Apache server is not supported that packet header is divided in the multiple packets.
	 * So, it MUST bigger than 82.
	 * Default value is 82.
	 */
	uint32_t send_buffer_size;
	/** 
	 * User agent of this client. 
	 * This value is must located in the Heap or code region.
	 * Default value is Atmel/{version}
	 */
	const char *user_agent;
};


/**
 * \brief HTTP client request instance.
 */
struct http_client_req {
	/** Status of request. */
	uint32_t state;
	/** URI of this request. */
	char uri[HTTP_MAX_URI_LENGTH];
	/** Entity of this request. */
	struct http_entity entity;
	/** Method of this request. */
	enum http_method method;
	/** Content-Length of this request. */
	int content_length;
	/** The size of the data sent. */
	int sent_length;
	/** 
	 * Extension header of the HTTP request. It is located in the heap memory. 
	 * Use of a little size of the extension header can be caused memory fragmentation.
	 */
	char *ext_header;
};

/**
 * \brief HTTP client response instance.
 */
struct http_client_resp {
	/** Status of response. */
	uint32_t state;
	/** Content-Length of this response. */
	int content_length;
	/** The size of the data received. */
	int read_length;
	/** Response code of this response. */
	uint16_t response_code;
};

/**
 * \brief Structure of HTTP client connection instance.
 */
struct http_client_module {
	/** Socket instance of HTTP session. */
	SOCKET sock;
	/** Destination host address of the session. */
	char host[HOSTNAME_MAX_SIZE];
	
	/** A flag for the socket is sending. */
	uint8_t sending	        : 1;
	/** A flag that whether using the persistent connection or not. */
	uint8_t permanent       : 1;
	/** A flag for the receive buffer located in the heap. */
	uint8_t alloc_buffer    : 1;

	/** Size that received. */
	uint32_t recved_size;

	/** SW Timer ID for the request time out. */
	int timer_id;
	
	/** Callback interface entry. */
	http_client_callback_t cb;
	
	/** Configuration instance of HTTP client module. That was registered from the \ref http_client_init*/
	struct http_client_config config;
	
	/** Data relating the request. */
	struct http_client_req req;
	
	/** Data relating the response. */
	struct http_client_resp resp;
};

/**
 * \brief Get default configuration of HTTP client module.
 *
 * \param[in]  config          Pointer of configuration structure which will be used in the module.
 */
void http_client_get_config_defaults(struct http_client_config *const config);

/**
 * \brief Initialize HTTP client service.
 *
 * \param[in]  module          Module instance of HTTP client module.
 * \param[in]  config          Pointer of configuration structure which will be used in the module.
 *
 * \return     0               Function succeeded
 * \return     -ENOENT         No such address.
 * \return     -EINVAL         Invalid argument.
 * \return     -ENOSPC         No space left on device.
 * \return     -EIO            Device was occurred error due to unknown exception.
 * \return     -EDESTADDRREQ   Destination address required.
 * \return     -ECONNRESET     Connection reset by peer.
 * \return     -EAGAIN         Try again.
 * \return     -EBUSY          Device or resource busy.
 * \return     -EADDRINUSE     Address already in use.
 * \return     -EALREADY       Socket already connected.
 * \return     -ENOTCONN       Service in bad state.
 * \return     -ECONNREFUSED   Connection refused.
 * \return     -EOVERFLOW      Value too large for defined data type.
 * \return     -EBADMSG        Not a data message.
 * \return     -ENOMEM         Out of memory.
 */
int http_client_init(struct http_client_module *const module, struct http_client_config *config);

/**
 * \brief Terminate HTTP client service.
 *
 * \param[in]  module          Module instance of HTTP client.
 *
 * \return     0               Function succeeded
 * \return     -ENOENT         No such address.
 * \return     -EINVAL         Invalid argument.
 * \return     -ENOSPC         No space left on device.
 * \return     -EIO            Device was occurred error due to unknown exception.
 * \return     -EDESTADDRREQ   Destination address required.
 * \return     -ECONNRESET     Connection reset by peer.
 * \return     -EAGAIN         Try again.
 * \return     -EBUSY          Device or resource busy.
 * \return     -EADDRINUSE     Address already in use.
 * \return     -EALREADY       Socket already connected.
 * \return     -ENOTCONN       Service in bad state.
 * \return     -ECONNREFUSED   Connection refused.
 * \return     -EOVERFLOW      Value too large for defined data type.
 * \return     -EBADMSG        Not a data message.
 * \return     -ENOMEM         Out of memory.
 * \return     -ENOTSUP        Unsupported operation.
 */
int http_client_deinit(struct http_client_module *const module);

/**
 * \brief Register and enable the callback.
 *
 * \param[in]  module_inst     Instance of HTTP client module.
 * \param[in]  callback        Callback entry for the HTTP client module.
 *
 * \return     0               Function succeeded
 * \return     -ENOENT         No such address.
 * \return     -EINVAL         Invalid argument.
 * \return     -ENOSPC         No space left on device.
 * \return     -EIO            Device was occurred error due to unknown exception.
 * \return     -EDESTADDRREQ   Destination address required.
 * \return     -ECONNRESET     Connection reset by peer.
 * \return     -EAGAIN         Try again.
 * \return     -EBUSY          Device or resource busy.
 * \return     -EADDRINUSE     Address already in use.
 * \return     -EALREADY       Socket already connected.
 * \return     -ENOTCONN       Service in bad state.
 * \return     -ECONNREFUSED   Connection refused.
 * \return     -EOVERFLOW      Value too large for defined data type.
 * \return     -EBADMSG        Not a data message.
 * \return     -ENOMEM         Out of memory.
 * \return     -ENOTSUP        Unsupported operation.
 */
int http_client_register_callback(struct http_client_module *const module, http_client_callback_t callback);
	
/**
 * \brief Unregister callback.
 *
 * \param[in]  module_inst     Instance of HTTP client module.
 *
 * \return     0               Function succeeded
 * \return     -ENOENT         No such address.
 * \return     -EINVAL         Invalid argument.
 * \return     -ENOSPC         No space left on device.
 * \return     -EIO            Device was occurred error due to unknown exception.
 * \return     -EDESTADDRREQ   Destination address required.
 * \return     -ECONNRESET     Connection reset by peer.
 * \return     -EAGAIN         Try again.
 * \return     -EBUSY          Device or resource busy.
 * \return     -EADDRINUSE     Address already in use.
 * \return     -EALREADY       Socket already connected.
 * \return     -ENOTCONN       Service in bad state.
 * \return     -ECONNREFUSED   Connection refused.
 * \return     -EOVERFLOW      Value too large for defined data type.
 * \return     -EBADMSG        Not a data message.
 * \return     -ENOMEM         Out of memory.
 * \return     -ENOTSUP        Unsupported operation.
 */
int http_client_unregister_callback(struct http_client_module *const module);

/**
 * \brief Event handler of socket event.
 *
 * \param[in]  sock            Socket descriptor.
 * \param[in]  msg_type        Event type.
 * \param[in]  msg_data        Structure of socket event.
 */
void http_client_socket_event_handler(SOCKET sock, uint8_t msg_type, void *msg_data);

/**
 * \brief Event handler of gethostbyname.
 *
 * \param[in]  doamin_name     Domain name.
 * \param[in]  server_ip       Server IP.
 */
void http_client_socket_resolve_handler(uint8_t *doamin_name, uint32_t server_ip);

/**
 * \brief Event handler of gethostbyname.
 *
 * \param[in]  module_inst     Instance of HTTP client module.
 * \param[in]  url             URL of request.
 * \param[in]  method          Method of request.
 * \param[in]  entity          Entity of request. Entity is consist of Entity header and Entity body Please refer to \ref http_entity.
 * \param[in]  ext_header      Extension header of the request.It must ends with new line character(\r\n).
 *
 * \return     0               Function succeeded
 * \return     -ENOENT         No such address.
 * \return     -EINVAL         Invalid argument.
 * \return     -ENOSPC         No space left on device.
 * \return     -EIO            Device was occurred error due to unknown exception.
 * \return     -EDESTADDRREQ   Destination address required.
 * \return     -ECONNRESET     Connection reset by peer.
 * \return     -EAGAIN         Try again.
 * \return     -EBUSY          Device or resource busy.
 * \return     -EADDRINUSE     Address already in use.
 * \return     -EALREADY       Socket already connected.
 * \return     -ENOTCONN       Service in bad state.
 * \return     -ECONNREFUSED   Connection refused.
 * \return     -EOVERFLOW      Value too large for defined data type.
 * \return     -EBADMSG        Not a data message.
 * \return     -ENOMEM         Out of memory.
 * \return     -ENOTSUP        Unsupported operation.
 */
int http_client_send_request(struct http_client_module *const module, const char *url,
	enum http_method method, struct http_entity *const entity, const char *ext_header);

/**
 * \brief Force close HTTP connection.
 *
 * \param[in]  module_inst     Instance of HTTP client module.
 *
 * \return     0               Function succeeded
 * \return     -ENOENT         No such address.
 * \return     -EINVAL         Invalid argument.
 * \return     -ENOSPC         No space left on device.
 * \return     -EIO            Device was occurred error due to unknown exception.
 * \return     -EDESTADDRREQ   Destination address required.
 * \return     -ECONNRESET     Connection reset by peer.
 * \return     -EAGAIN         Try again.
 * \return     -EBUSY          Device or resource busy.
 * \return     -EADDRINUSE     Address already in use.
 * \return     -EALREADY       Socket already connected.
 * \return     -ENOTCONN       Service in bad state.
 * \return     -ECONNREFUSED   Connection refused.
 * \return     -EOVERFLOW      Value too large for defined data type.
 * \return     -EBADMSG        Not a data message.
 * \return     -ENOMEM         Out of memory.
 * \return     -ENOTSUP        Unsupported operation.
 */
int http_client_close(struct http_client_module *const module);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* HTTP_CLEINT_H_INCLUDED */
