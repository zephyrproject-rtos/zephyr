/*
 * Copyright (c) 2016 Intel Corporation
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

#ifndef _MQTT_H_
#define _MQTT_H_

#include <net/net_core.h>

int mqtt_connect(struct net_context *ctx, char *client_name);
int mqtt_disconnect(struct net_context *ctx);
int mqtt_publish(struct net_context *ctx, char *topic, char *msg);
int mqtt_publish_read(struct net_context *ctx, char *topic_str, char *msg_str);
int mqtt_subscribe(struct net_context *ctx, char *topic);
int mqtt_pingreq(struct net_context *ctx);

#endif
