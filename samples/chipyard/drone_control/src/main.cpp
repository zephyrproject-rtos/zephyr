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
#include "problem_data/quadrotor_50hz_params_unconstrained.hpp"  // TinyMPC problem data
#include "glob_opts.hpp"  // Contains NSTATES, NINPUTS, NHORIZON, etc.

#ifdef __cplusplus
extern "C" {
#endif

// Global TinyMPC objects (they are defined in admm.hpp or related headers)
TinyCache cache;
TinyWorkspace work;
TinySettings settings;
TinySolver solver{&settings, &cache, &work};

int main(void)
{
    // Print initial greeting.
    printf("Testing drone control on %s\n", CONFIG_BOARD);

    // Get the console UART device from Zephyr's configuration
    // const struct device *uart_dev = device_get_binding(CONFIG_UART_CONSOLE_ON_DEV_NAME);
    const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
    if (!uart_dev) {
        printf("Error: Console UART device not found.\n");
        return -1;
    }


    #define BUFFER_SIZE 256
    char input_buf[BUFFER_SIZE];
    size_t pos = 0;
    unsigned char c;


    // Enable vector operations (provided by TinyMPC/ADMM)
    enable_vector_operations();

    printf("Entered MPC simulation main!\n");

    // Temporary vectors for MPC computations
    tiny_VectorNx v1, v2;

    // Setup problem data
    cache.rho = rho_value;
    cache.Kinf.set(Kinf_data);
    transpose(cache.Kinf.data, cache.KinfT.data, NINPUTS, NSTATES);
    cache.Pinf.set(Pinf_data);
    transpose(cache.Pinf.data, cache.PinfT.data, NSTATES, NSTATES);
    cache.Quu_inv.set(Quu_inv_data);
    cache.AmBKt.set(AmBKt_data);
    cache.coeff_d2p.set(coeff_d2p_data);
    work.Adyn.set(Adyn_data);
    work.Bdyn.set(Bdyn_data);
    transpose(work.Bdyn.data, work.BdynT.data, NSTATES, NINPUTS);
    work.Q.set(Q_data);
    work.R.set(R_data);

    // Valid range for inputs and states
    work.u_min = -0.583;
    work.u_max = 1 - 0.583;
    work.x_min = -5;
    work.x_max = 5;

    // Initialize optimization residuals and settings
    work.primal_residual_state = 0;
    work.primal_residual_input = 0;
    work.dual_residual_state = 0;
    work.dual_residual_input = 0;
    work.status = 0;
    work.iter = 0;
    settings.abs_pri_tol = 0.001;
    settings.abs_dua_tol = 0.001;
    settings.max_iter = 10;
    settings.check_termination = 1;
    settings.en_input_bound = 1;
    settings.en_state_bound = 1;

    // Setup hovering setpoint (desired state)
    tiny_VectorNx Xref_origin;
    tinytype Xref_origin_data[NSTATES] = {
        0, 0, 1.5, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    Xref_origin.set(Xref_origin_data);
    // For each step in the horizon, set the reference to hover at the same point.
    for (int j = 0; j < NHORIZON; j++) {
        tinytype *target = work.Xref.col(j);
        matsetv(target, Xref_origin_data, 1, NSTATES);
    }

    // Setup an initial state x0 (this will be overwritten by input)
    tiny_VectorNx x0;
    tinytype x0_data[NSTATES] = {
        -3.64893626e-02,  3.70428882e-02,  2.25366379e-01, -1.92755080e-01,
        -1.91678221e-01, -2.21354598e-03,  9.62340916e-01, -4.09749891e-01,
        -3.78764621e-01,  7.50158432e-02, -6.63581815e-01,  6.71744046e-01
    };
    x0.set(x0_data);

    while (true)
    {

        printf("Enter new state (12 floats separated by spaces):\n");

        // Poll UART until a newline or carriage return is received.
        while (true) {
            int ret = uart_poll_in(uart_dev, &c);
            if (ret == 0) {
                if (c == '\n' || c == '\r') {
                    input_buf[pos] = '\0';
                    break;
                }
                if (pos < (BUFFER_SIZE - 1)) {
                    input_buf[pos++] = c;
                }
            } else {
                k_sleep(K_MSEC(10));
            }
        }

        // Parse the input string for 12 floats.
        float new_state[12];
        int num_parsed = sscanf(input_buf,
                                "%f %f %f %f %f %f %f %f %f %f %f %f",
                                &new_state[0],  &new_state[1],  &new_state[2],
                                &new_state[3],  &new_state[4],  &new_state[5],
                                &new_state[6],  &new_state[7],  &new_state[8],
                                &new_state[9],  &new_state[10], &new_state[11]);

        if (num_parsed != 12) {
            printf("Error: expected 12 floats but got %d\n", num_parsed);
        } else {
            // Update the current state x0 using the new values.
            x0.set(new_state);
        }
        // 1. Update measurement: copy current state into work.x (column 0)
        matsetv(work.x.col(0), x0.data, 1, NSTATES);

        // 2. Reset dual variables (if needed)
        work.y = 0.0;
        work.g = 0.0;

        // 3. Solve the MPC problem using ADMM
        tiny_solve(&solver);

        // 4. Print the computed control actions.
        printf("Computed actions: %f %f %f %f\n", 
               work.u.col(0)[0], work.u.col(0)[1], work.u.col(0)[2], work.u.col(0)[3]);
    }

    // Optionally, you could reboot at the end:
    sys_reboot(SYS_REBOOT_COLD);
    return 0;
}

#ifdef __cplusplus
}
#endif
