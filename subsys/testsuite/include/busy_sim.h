/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef BUSY_SIM_H__
#define BUSY_SIM_H__

/**
 * @brief Start busy simulator.
 *
 * When started, it is using counter device to generate interrupts at random
 * intervals and busy loop for random period of time in that interrupt. Interrupt
 * source and priority is configured in the devicetree. Default entropy source
 * is used for getting random numbers. System work queue is used to get random
 * values and keep them in a ring buffer.
 *
 * @param active_avg Avarage time of busy looping in the counter callback (in microseconds).
 * @param active_delta Specifies deviation from avarage time of busy looping (in microseconds).
 * @param idle_avg Avarage time of counter alarm timeout (in microseconds).
 * @param idle_delta Specifies deviation from avarage time of counter alarm (in microseconds).
 */
void busy_sim_start(uint32_t active_avg, uint32_t active_delta,
			  uint32_t idle_avg, uint32_t idle_delta);

/** @brief Stop busy simulator. */
void busy_sim_stop(void);

#endif /* BUSY_SIM_H__ */
