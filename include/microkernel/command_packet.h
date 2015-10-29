/* command_packet.h - command packet header file */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
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

/**
 * @brief Microkernel command packet library
 *
 * A command packet is an opaque data structure that maps to a k_args without
 * exposing its innards. This library allows a subsystem that needs to define
 * a command packet the ability to do so.
 */

#ifndef _COMMAND_PACKET_H
#define _COMMAND_PACKET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <microkernel/base_api.h>

/* define size of command packet (without exposing its internal structure) */

#define CMD_PKT_SIZE_IN_WORDS (19)

/* define command packet type */

typedef uint32_t cmdPkt_t[CMD_PKT_SIZE_IN_WORDS];

#ifdef __cplusplus
}
#endif

#endif /* _COMMAND_PACKET_H */
