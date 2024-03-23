/*
 * Copyright (c) 2024 Glenn Andrews
 * State Machine example copyright (c) Miro Samek
 *
 * Implementation of the statechart in Figure 2.11 of
 * Practical UML Statecharts in C/C++, 2nd Edition by Miro Samek
 * https://www.state-machine.com/psicc2
 * Used with permission of the author.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#define PSICC2_THREAD_STACK_SIZE       1024
#define PSICC2_THREAD_PRIORITY         7
#define PSICC2_THREAD_EVENT_QUEUE_SIZE 10

/**
 * @brief Event to be sent to an event queue
 */
struct psicc2_event {
	uint32_t event_id;
} __aligned(4);

/**
 * @brief List of events that can be sent to the state machine
 */
enum demo_events {
	EVENT_A,
	EVENT_B,
	EVENT_C,
	EVENT_D,
	EVENT_E,
	EVENT_F,
	EVENT_G,
	EVENT_H,
	EVENT_I,
	EVENT_TERMINATE,
};

/* event queue to post messages to */
extern struct k_msgq psicc2_msgq;

/**
 * @brief Initializes and starts the PSICC2 demo thread
 * @param None
 */
void psicc2_thread_run(void);
