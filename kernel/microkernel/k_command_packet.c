/*
 * Copyright (c) 2012, 2014 Wind River Systems, Inc.
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
 * @file
 * @brief Microkernel command packet library
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <microkernel/command_packet.h>
#include <micro_private.h>
#include <sections.h>

/**
 * Generate build error by defining a negative-size array if the hard-coded
 * command packet size differs from the actual size; otherwise, define
 * a zero-element array that gets thrown away by linker
 */
uint32_t _k_test_cmd_pkt_size
	[0 - ((CMD_PKT_SIZE_IN_WORDS * sizeof(uint32_t)) != sizeof(struct k_args))];

/**
 *
 * @brief Send command packet to be processed by _k_server
 * @param cmd_packet Arguments
 * @return N/A
 */
void _k_task_call(struct k_args *cmd_packet)
{
	cmd_packet->alloc = false;
	_k_current_task->args = cmd_packet;
	nano_task_stack_push(&_k_command_stack, (uint32_t)cmd_packet);
}
