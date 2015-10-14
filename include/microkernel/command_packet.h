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
 * exposing its innards. It allows a subsystem, like a driver, that needs its
 * fibers or ISRs to communicate with tasks to define a set of such packets to
 * be passed to the isr_xxx routines, such as isr_sem_give(), that, in turn,
 * needs to obtain command packets to interact with the microkernel.
 *
 * Each command packet set is created in global memory using ...
 *   CMD_PKT_SET_INSTANCE(set variable name, # of command packets in the set);
 *
 * Once created, the command packet set is referenced as an ordinary structured
 * variable.

 * A command packet set is a simple ring buffer.  No error checking is performed
 * when a command packet is retrieved from the set.  Each obtained command packet
 * is implicitly released once the command packet has been processed.  Thus, it is
 * important that each command packet be processed in a near-FIFO order to prevent
 * corruption of command packets that are already in use.  To this end, drivers
 * that have an ISR component should use their own command packet set.
 */

#ifndef _COMMAND_PACKET_H
#define _COMMAND_PACKET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <microkernel/base_api.h>

/* define size of command packet (without exposing its internal structure) */

#define CMD_PKT_SIZE_IN_WORDS (19)

/* define command packet set types */

typedef uint32_t cmdPkt_t[CMD_PKT_SIZE_IN_WORDS];

struct cmd_pkt_set {
	uint32_t num_packets;      /* number of command packets in set */
	uint32_t index;            /* index into command packet array */
	cmdPkt_t *command_packet;  /* pointer to array of command packets */
};

/**
 * @brief Define a command packet set
 *
 * This macro is used to create a command packet set of the specified size.
 *
 * @param name Name of command packet set
 * @param num Number of packets in the set
 *
 * @warning The command packet set exists in the global namespace,
 * and cannot be hidden by prefixing the macro with "static".
 */
#define CMD_PKT_SET_INSTANCE(name, num) \
	uint32_t __noinit (_k_cmd_pkts_ ## name)[CMD_PKT_SIZE_IN_WORDS * (num)]; \
	struct cmd_pkt_set name = {(num), 0, (cmdPkt_t *)(_k_cmd_pkts_ ## name)}

/* externs */

extern struct k_args *_cmd_pkt_get(struct cmd_pkt_set *pSet);

#ifdef __cplusplus
}
#endif

#endif /* _COMMAND_PACKET_H */
