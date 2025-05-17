/*
 * Copyright (c) 2025, Dima Nikiforov <vnikiforov@berkeley.edu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/reboot.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stdio.h>
#include <stdio.h>
#include <admm.hpp>
//#include "problem_data/quadrotor_50hz_params_constrained.hpp"  // TinyMPC problem data
#include "problem_data/quadrotor_20hz_params.hpp"
#include "glob_opts.hpp"  // Contains NSTATES, NINPUTS, NHORIZON, etc.

#ifdef __cplusplus
extern "C" {
#endif

// Global TinyMPC objects
TinyCache cache;
TinyWorkspace work;
TinySettings settings;
TinySolver solver = {&settings, &cache, &work};

#define BUFFER_SIZE 256
#define NUM_STATE_VARS 12

const struct device *uart0 = DEVICE_DT_GET(DT_NODELABEL(uart0));

void send_str(const char *str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        uart_poll_out(uart0, str[i]);
    }
}


void recv_str(char *buf, size_t buf_size)
{
    size_t i = 0;
    unsigned char c = 0;

    // Read until newline or buffer full
    while (i < buf_size - 1) {
        if (uart_poll_in(uart0, &c) == 0) {
            if (c == '\r' || c == '\n') {
                break;
            }
            buf[i++] = c;
        } else {
        }
    }

    buf[i] = '\0';
}


void init_all() {
    tiny_init(&solver);

    init_VectorNx(&work.x1);
    init_VectorNx(&work.x2);
    init_VectorNx(&work.x3);
    init_VectorNu(&work.u1);
    init_VectorNu(&work.u2);
}

int main(void)
{
    if (!device_is_ready(uart0)) {
        return -1;
    }

    // send_str("Testing drone control via UART\n");

    enable_vector_operations();
    init_all();

    // Local temp vectors
    tiny_VectorNx v1, v2, x0;
    init_VectorNx(&v1);
    init_VectorNx(&v2);
    init_VectorNx(&x0);

    // Set up problem data
    cache.rho = rho_value;
    matsetv(cache.Kinf.data, Kinf_data, cache.Kinf.outer, cache.Kinf.inner);
    transpose(cache.Kinf.data, cache.KinfT.data, NINPUTS, NSTATES);
    matsetv(cache.Pinf.data, Pinf_data, cache.Pinf.outer, cache.Pinf.inner);
    transpose(cache.Pinf.data, cache.PinfT.data, NSTATES, NSTATES);
    matsetv(cache.Quu_inv.data, Quu_inv_data, cache.Quu_inv.outer, cache.Quu_inv.inner);
    matsetv(cache.AmBKt.data, AmBKt_data, cache.AmBKt.outer, cache.AmBKt.inner);
    matsetv(cache.coeff_d2p.data, coeff_d2p_data, cache.coeff_d2p.outer, cache.coeff_d2p.inner);
    matsetv(work.Adyn.data, Adyn_data, work.Adyn.outer, work.Adyn.inner);
    matsetv(work.Bdyn.data, Bdyn_data, work.Bdyn.outer, work.Bdyn.inner);
    transpose(work.Bdyn.data, work.BdynT.data, NSTATES, NINPUTS);
    matsetv(work.Q.data, Q_data, work.Q.outer, work.Q.inner);
    matsetv(work.R.data, R_data, work.R.outer, work.R.inner);
    matset(work.u_min.data, -0.583, work.u_min.outer, work.u_min.inner);
    matset(work.u_max.data, 1 - 0.583, work.u_max.outer, work.u_max.inner);
    matset(work.x_min.data, -5, work.x_min.outer, work.x_min.inner);
    matset(work.x_max.data, 5, work.x_max.outer, work.x_max.inner);

    // Hovering reference
    tiny_VectorNx Xref_origin;
    init_VectorNx(&Xref_origin);
    tinytype Xref_origin_data[NSTATES] = {0, 0, 1.0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    matsetv(Xref_origin.data, Xref_origin_data, Xref_origin.outer, Xref_origin.inner);
    for (int j = 0; j < NHORIZON; j++) {
        matsetv(work.Xref.vector[j], Xref_origin_data, 1, NSTATES);
    }

    // Default initial state
    tinytype x0_data[NSTATES] = {
        -0.036, 0.037, 0.225, -0.193,
        -0.192, -0.002, 0.962, -0.409,
        -0.378, 0.075, -0.663, 0.672
    };
    matsetv(x0.data, x0_data, x0.outer, x0.inner);

    char input_buf[BUFFER_SIZE];
    unsigned char c;
    int pos = 0;

    while (1) {
        //send_str("Enter new state (12 floats):\n");
        pos = 0;


        recv_str(input_buf, BUFFER_SIZE);

        float parsed[NUM_STATE_VARS];
        int n = sscanf(input_buf,
                       "%f %f %f %f %f %f %f %f %f %f %f %f",
                       &parsed[0], &parsed[1], &parsed[2], &parsed[3],
                       &parsed[4], &parsed[5], &parsed[6], &parsed[7],
                       &parsed[8], &parsed[9], &parsed[10], &parsed[11]);

        if (n == NUM_STATE_VARS) {
            matsetv(x0.data, parsed, x0.outer, x0.inner);
        } else {
            send_str("Invalid input. Expected 12 floats.\n");
            continue;
        }

        // Load into work.x
        matsetv(work.x.vector[0], x0.data, 1, NSTATES);
        matset(work.y.data, 0.0, work.y.outer, work.y.inner);
        matset(work.g.data, 0.0, work.g.outer, work.g.inner);

        tiny_solve(&solver);

        // Output control results
        char outbuf[128];
        snprintf(outbuf, sizeof(outbuf),
                 "u = [%0.4f %0.4f %0.4f %0.4f]\n",
                 work.u.vector[0][0], work.u.vector[0][1],
                 work.u.vector[0][2], work.u.vector[0][3]);
        send_str(outbuf);
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
