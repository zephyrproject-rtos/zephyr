/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __WEBSOCKET_INTERNAL_H__
#define __WEBSOCKET_INTERNAL_H__

#include <net/http.h>
#include <net/http_parser.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Strip websocket header from the packet.
 *
 * @details The function will remove websocket header from the network packet.
 *
 * @param pkt Received network packet
 * @param masked The mask status of the message is returned.
 * @param mask_value The mask value of the message is returned.
 * @param message_length Total length of the message from websocket header.
 * @param message_type_flag Type of the websocket message (WS_FLAG_xxx value)
 * @param header_len Length of the websocket header is returned to caller.
 *
 * @return 0 if ok, <0 if error
 */
int ws_strip_header(struct net_pkt *pkt, bool *masked, u32_t *mask_value,
		    u32_t *message_length, u32_t *message_type_flag,
		    u32_t *header_len);

/**
 * @brief Mask or unmask a websocket message if needed
 *
 * @details The function will either add or remove the masking from the data.
 *
 * @param pkt Network packet to process
 * @param masking_value The mask value to use.
 * @param data_read How many bytes we have read. This is modified by this
 * function.
 */
void ws_mask_pkt(struct net_pkt *pkt, u32_t masking_value, u32_t *data_read);

/**
 * @brief This is called by HTTP server after all the HTTP headers have been
 * received.
 *
 * @details The function will check if this is a valid websocket connection
 * or not.
 *
 * @param parser HTTP parser instance
 *
 * @return 0 if ok, 1 if there is no body, 2 if HTTP connection is to be
 * upgraded to websocket one
 */
int ws_headers_complete(struct http_parser *parser);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __WS_H__ */
