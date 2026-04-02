/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef BUSY_SIM_H__
#define BUSY_SIM_H__

typedef void (*busy_sim_cb_t)(void);

/**
 * @brief Start busy simulator.
 *
 * When started, it is using counter device to generate interrupts at random
 * intervals and busy loop for random period of time in that interrupt. Interrupt
 * source and priority is configured in the devicetree. Default entropy source
 * is used for getting random numbers. System work queue is used to get random
 * values and keep them in a ring buffer.
 *
 * @param active_avg Average time of busy looping in the counter callback (in microseconds).
 *
 * @param active_delta Specifies deviation from average time of busy looping (in microseconds).
 *
 * @param idle_avg Average time of counter alarm timeout (in microseconds).
 *
 * @param idle_delta Specifies deviation from average time of counter alarm (in microseconds).
 *
 * @param cb Callback called from the context of the busy simulator timeout. If ZLI interrupt
 * is used for busy simulator counter then kernel API cannot be used from that callback.
 * Callback is called before busy waiting.
 */
void busy_sim_start(uint32_t active_avg, uint32_t active_delta,
		    uint32_t idle_avg, uint32_t idle_delta, busy_sim_cb_t cb);

/** @brief Stop busy simulator. */
void busy_sim_stop(void);

#endif /* BUSY_SIM_H__ */
