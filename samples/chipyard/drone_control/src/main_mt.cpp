/*
 * Copyright (c) 2025, Dima Nikiforov <vnikiforov@berkeley.edu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/device.h>
#include <stdio.h>
#include <string.h>
#include <admm.hpp>
#include "problem_data/quadrotor_20hz_params.hpp"
#include "glob_opts.hpp"

#define NUM_DRONES 1
#define STACK_SIZE (8192*16)
#define BUFFER_SIZE 256
#define NUM_STATE_VARS 12

#define DEBUG_UART

TinyCache    caches[NUM_DRONES];
TinyWorkspace works[NUM_DRONES];
TinySettings  settings[NUM_DRONES];
TinySolver    solvers[NUM_DRONES];  // Filled during init

K_THREAD_STACK_ARRAY_DEFINE(drone_stacks, NUM_DRONES, STACK_SIZE);
struct k_thread drone_threads[NUM_DRONES];
k_tid_t drone_thread_ids[NUM_DRONES];

struct DroneTask {
    struct k_sem start_sem;
    struct k_sem done_sem;
    float obs[NSTATES];
    float action[NINPUTS];
};

struct k_mutex uart_mutex;

static struct DroneTask drone_tasks[NUM_DRONES];
const struct device *uart0 = DEVICE_DT_GET(DT_NODELABEL(uart0));


void send_str(const char *str) {
    k_mutex_lock(&uart_mutex, K_FOREVER);
    for (size_t i = 0; str[i] != '\0'; i++) {
        uart_poll_out(uart0, str[i]);
    }
    k_mutex_unlock(&uart_mutex);
}

void recv_str(char *buf, size_t buf_size) {
    size_t i = 0;
    unsigned char c = 0;
    while (i < buf_size - 1) {
        if (uart_poll_in(uart0, &c) == 0) {
            if (c == '\r' || c == '\n') break;
            buf[i++] = c;
        }
    }
    buf[i] = '\0';
}

void drone_worker(void *id_ptr, void *, void *) {
    int id = (int)(uintptr_t)id_ptr;

    enable_vector_operations();

    // Use global TinyMPC structs
    TinyCache    *cache   = &caches[id];
    TinyWorkspace *work   = &works[id];
    TinySettings  *setting = &settings[id];
    TinySolver    *solver  = &solvers[id];

    solver->cache = cache;
    solver->work = work;
    solver->settings = setting;

    tiny_init(solver);

    init_VectorNx(&work->x1);
    init_VectorNx(&work->x2);
    init_VectorNx(&work->x3);
    init_VectorNu(&work->u1);
    init_VectorNu(&work->u2);

    // Set up MPC matrices
    cache->rho = rho_value;
    matsetv(cache->Kinf.data, Kinf_data, cache->Kinf.outer, cache->Kinf.inner);
    transpose(cache->Kinf.data, cache->KinfT.data, NINPUTS, NSTATES);
    matsetv(cache->Pinf.data, Pinf_data, cache->Pinf.outer, cache->Pinf.inner);
    transpose(cache->Pinf.data, cache->PinfT.data, NSTATES, NSTATES);
    matsetv(cache->Quu_inv.data, Quu_inv_data, cache->Quu_inv.outer, cache->Quu_inv.inner);
    matsetv(cache->AmBKt.data, AmBKt_data, cache->AmBKt.outer, cache->AmBKt.inner);
    matsetv(cache->coeff_d2p.data, coeff_d2p_data, cache->coeff_d2p.outer, cache->coeff_d2p.inner);

    matsetv(work->Adyn.data, Adyn_data, work->Adyn.outer, work->Adyn.inner);
    matsetv(work->Bdyn.data, Bdyn_data, work->Bdyn.outer, work->Bdyn.inner);
    transpose(work->Bdyn.data, work->BdynT.data, NSTATES, NINPUTS);
    matsetv(work->Q.data, Q_data, work->Q.outer, work->Q.inner);
    matsetv(work->R.data, R_data, work->R.outer, work->R.inner);

    matset(work->u_min.data, -0.583, work->u_min.outer, work->u_min.inner);
    matset(work->u_max.data, 1 - 0.583, work->u_max.outer, work->u_max.inner);
    matset(work->x_min.data, -5, work->x_min.outer, work->x_min.inner);
    matset(work->x_max.data, 5, work->x_max.outer, work->x_max.inner);

    // Xref zero reference
    tinytype Xref_origin[NSTATES] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    for (int j = 0; j < NHORIZON; j++) {
        matsetv(work->Xref.vector[j], Xref_origin, 1, NSTATES);
    }


    while (1) {
        k_sem_take(&drone_tasks[id].start_sem, K_FOREVER);

        #ifdef DEBUG_UART
        
        char obs_buf[256];
        
        snprintf(obs_buf, sizeof(obs_buf),
                 "Obs[%d] = [%0.3f %0.3f %0.3f %0.3f %0.3f %0.3f %0.3f %0.3f %0.3f %0.3f %0.3f %0.3f]\n",
                 id,
                 drone_tasks[id].obs[0], drone_tasks[id].obs[1], drone_tasks[id].obs[2],
                 drone_tasks[id].obs[3], drone_tasks[id].obs[4], drone_tasks[id].obs[5],
                 drone_tasks[id].obs[6], drone_tasks[id].obs[7], drone_tasks[id].obs[8],
                 drone_tasks[id].obs[9], drone_tasks[id].obs[10], drone_tasks[id].obs[11]);
        // send_str(obs_buf);
        /**/
        #endif

        

        matsetv(work->x.vector[0], drone_tasks[id].obs, 1, NSTATES);
        matset(work->y.data, 0.0, work->y.outer, work->y.inner);
        matset(work->g.data, 0.0, work->g.outer, work->g.inner);

        memset(drone_tasks[id].action, 0, sizeof(float) * NINPUTS);

        int status = tiny_solve(solver);

        memcpy(drone_tasks[id].action, work->u.vector[0], sizeof(float) * NINPUTS);
        

        k_sem_give(&drone_tasks[id].done_sem);

    }
}



void spawn_drone_workers() {
    for (int i = 0; i < NUM_DRONES; i++) {
        k_sem_init(&drone_tasks[i].start_sem, 0, 1);
        k_sem_init(&drone_tasks[i].done_sem, 0, 1);

        drone_thread_ids[i] = k_thread_create(&drone_threads[i], drone_stacks[i], STACK_SIZE,
            drone_worker, (void *)(uintptr_t)i, NULL, NULL,
            K_PRIO_PREEMPT(1), 0, K_FOREVER);
        k_thread_cpu_pin(drone_thread_ids[i], (i+1) % CONFIG_MP_MAX_NUM_CPUS);
    }
    for (int i = 0; i < NUM_DRONES; i++) {
        
        k_thread_start(drone_thread_ids[i]);

    }
}

int main(void) {
    if (!device_is_ready(uart0)) return -1;
    k_mutex_init(&uart_mutex);
    
    spawn_drone_workers();

    send_str("Ready To Receive UART Commands\n");

    char input_buf[BUFFER_SIZE];
    float parsed[NUM_STATE_VARS];
    int id;

    while (1) {
        recv_str(input_buf, BUFFER_SIZE);
        int n = sscanf(input_buf, "%d %f %f %f %f %f %f %f %f %f %f %f %f",
                       &id, &parsed[0], &parsed[1], &parsed[2], &parsed[3],
                       &parsed[4], &parsed[5], &parsed[6], &parsed[7],
                       &parsed[8], &parsed[9], &parsed[10], &parsed[11]);

        if (n != 13 || id >= NUM_DRONES) {
            send_str("Invalid input. Expected ID + 12 floats.\n");
            continue;
        }


        memcpy(drone_tasks[id].obs, parsed, sizeof(parsed));
        k_sem_give(&drone_tasks[id].start_sem);


        k_sem_take(&drone_tasks[id].done_sem, K_FOREVER);


        char outbuf[128];
        snprintf(outbuf, sizeof(outbuf),
                 "u = [%0.4f %0.4f %0.4f %0.4f]\n",
                 drone_tasks[id].action[0], drone_tasks[id].action[1],
                 drone_tasks[id].action[2], drone_tasks[id].action[3]);
        send_str(outbuf);
    }

    return 0;
}
