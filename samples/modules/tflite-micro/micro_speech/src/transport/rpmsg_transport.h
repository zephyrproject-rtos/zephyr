/*
 * Copyright 2025 The TensorFlow Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef RPMSG_TRANSPORT_H
#define RPMSG_TRANSPORT_H

#include <zephyr/kernel.h>
#include <openamp/open_amp.h>

#ifdef __cplusplus
extern "C" {
#endif

struct rpmsg_rcv_msg {
    void *data;
    size_t len;
};

extern struct k_msgq tty_msgq;
extern struct rpmsg_endpoint tty_ept;
extern struct k_sem data_tty_ready_sem;
extern struct rpmsg_device *rpdev;

void rpmsg_transport_start(void);
int rpmsg_recv_tty_callback(struct rpmsg_endpoint *ept, void *data,
                                size_t len, uint32_t src, void *priv);
#ifdef __cplusplus
}
#endif

#endif /* RPMSG_TRANSPORT_H */