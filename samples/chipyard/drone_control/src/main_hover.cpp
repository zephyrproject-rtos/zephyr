/*
 * Copyright (c) 2025, Dima Nikiforov <vnikiforov@berkeley.edu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <stdio.h>
#include <zephyr/kernel.h>

#include <admm.hpp>
#include <zephyr/sys/reboot.h>
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
/* 
*/



#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#include <stdio.h>
#include <string.h>

/*
const struct device *uart0 = DEVICE_DT_GET(DT_NODELABEL(uart0));




void send_str(const struct device *uart, char *str)
{
	int msg_len = strlen(str);

	for (int i = 0; i < msg_len; i++) {
		uart_poll_out(uart, str[i]);
	}

	printk("Device %s sent: \"%s\"\n", uart->name, str);
}*/



int main(void)
{
    printf("Testing drone control on %s\n", CONFIG_BOARD);

    enable_vector_operations();
    tiny_init(&solver);  // Initializes solver, workspace, cache

    printf("Entered MPC simulation main!\n");

    // Temporary vectors for simulation
    tiny_VectorNx v1, v2;
    init_VectorNx(&v1);
    init_VectorNx(&v2);

    // Setup problem data
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

    // Valid range for inputs and states
    matset(work.u_min.data, -0.583, work.u_min.outer, work.u_min.inner);
    matset(work.u_max.data, 1 - 0.583, work.u_max.outer, work.u_max.inner);
    matset(work.x_min.data, -5, work.x_min.outer, work.x_min.inner);
    matset(work.x_max.data, 5, work.x_max.outer, work.x_max.inner);

    // Initialize optimization residuals and settings (redundant with tiny_init but safe)
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
    init_VectorNx(&Xref_origin);
    tinytype Xref_origin_data[NSTATES] = {
        0, 0, 1.5, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    matsetv(Xref_origin.data, Xref_origin_data, Xref_origin.outer, Xref_origin.inner);
    for (int j = 0; j < NHORIZON; j++) {
        matsetv(work.Xref.vector[j], Xref_origin_data, 1, NSTATES);
    }

    // Setup initial state x0
    tiny_VectorNx x0, x1;
    init_VectorNx(&x0);
    init_VectorNx(&x1);
    tinytype x0_data[NSTATES] = {
        -3.64893626e-02,  3.70428882e-02,  2.25366379e-01, -1.92755080e-01,
        -1.91678221e-01, -2.21354598e-03,  9.62340916e-01, -4.09749891e-01,
        -3.78764621e-01,  7.50158432e-02, -6.63581815e-01,  6.71744046e-01
    };
    matsetv(x0.data, x0_data, x0.outer, x0.inner);
    matset(x1.data, 0, x1.outer, x1.inner);

    // Print CSV header
    printf("Step,TrackingError,x,y,z,phi,theta,psi,dx,dy,dz,dphi,dtheta,dpsi,u1,u2,u3,u4\n");

    for (int k = 0; k < 70; ++k) {
        // 1. Update measurement: copy current state into work.x (column 0)
        matsetv(work.x.vector[0], x0.data, 1, NSTATES);

        // 2. Reset dual variables
        matset(work.y.data, 0.0, work.y.outer, work.y.inner);
        matset(work.g.data, 0.0, work.g.outer, work.g.inner);

        // 3. Solve the MPC problem using ADMM
        tiny_solve(&solver);

        // 4. Simulate forward step: x0 = A*x0 + B*u
#ifdef USE_MATVEC
        matvec(work.Adyn.data, x0.data, v1.data, NSTATES, NSTATES);
        matvec(work.Bdyn.data, work.u.vector[0], v2.data, NSTATES, NINPUTS);
#else
        matmul(x0.data, work.Adyn.data, v1.data, 1, NSTATES, NSTATES);
        matmul(work.u.vector[0], work.Bdyn.data, v2.data, 1, NSTATES, NINPUTS);
#endif
        matadd(v1.data, v2.data, x0.data, 1, NSTATES);

        // 5. Print simulation results
        printf("%d,", k);
        matsub(x0.data, work.Xref.vector[1], v1.data, 1, NSTATES);
        float norm = matnorm(v1.data, 1, NSTATES);
        printf("%0.7f,\n", norm);

        // Optional: print full state/input vectors here if desired
    }

    sys_reboot(SYS_REBOOT_COLD);
    return 0;
}


#ifdef __cplusplus
}
#endif
