/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018-2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Copyright (c) 2016, SICS Swedish ICT AB.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LWM2M_RD_CLIENT_H
#define LWM2M_RD_CLIENT_H

#include <zephyr/net/lwm2m.h> /* struct lwm2m_ctx */

int lwm2m_rd_client_init(void);
void engine_trigger_update(bool update_objects);
int engine_trigger_bootstrap(void);
int lwm2m_rd_client_pause(void);
int lwm2m_rd_client_resume(void);

int lwm2m_rd_client_timeout(struct lwm2m_ctx *client_ctx);
bool lwm2m_rd_client_is_registred(struct lwm2m_ctx *client_ctx);
bool lwm2m_rd_client_is_suspended(struct lwm2m_ctx *client_ctx);
#if defined(CONFIG_LWM2M_RD_CLIENT_SUPPORT_BOOTSTRAP)
void engine_bootstrap_finish(void);
#endif
int lwm2m_rd_client_connection_resume(struct lwm2m_ctx *client_ctx);
void engine_update_tx_time(void);
struct lwm2m_message *lwm2m_get_ongoing_rd_msg(void);

/**
 * @brief Notify RD client that this server is disabled.
 *
 * This may return error -EPERM, if RD client is not registered on that server.
 *
 * @param inst_id server instance id
 * @return int 0 on success, negative errno on failure.
 */
int lwm2m_rd_client_server_disabled(uint16_t inst_id);

/**
 * @brief Set client context for the RD client.
 *
 * For testing purposes, it might be required to set the client context
 * without starting the RD client.
 *
 * @param ctx context
 */
void lwm2m_rd_client_set_ctx(struct lwm2m_ctx *ctx);

#endif /* LWM2M_RD_CLIENT_H */
