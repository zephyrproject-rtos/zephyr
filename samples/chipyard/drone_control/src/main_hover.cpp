/*
 * Copyright (c) 2025, Dima Nikiforov <vnikiforov@berkeley.edu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/reboot.h>
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

// #include <new>
// #include <stdint.h>
// #include <stddef.h>

// // Define the scratchpad base address.
// // Make sure this address is valid and that the region is large enough.
// #define SCRATCHPAD_ADDR 0x08000000

// // Helper macro to align addresses.
// #define ALIGN_UP(addr, alignment) (((uintptr_t)(addr) + ((alignment)-1)) & ~((uintptr_t)(alignment)-1))

// // Global pointer variables for our TinyMPC objects.
// static TinyCache* cache_ptr;
// static TinyWorkspace* work_ptr;
// static TinySettings* settings_ptr;
// static TinySolver* solver_ptr;

// // Define macros to alias the original names.
// #define cache    (*cache_ptr)
// #define work     (*work_ptr)
// #define settings (*settings_ptr)
// #define solver   (*solver_ptr)

// void init_tinympc_globals() {
//     // Start at the scratchpad base address.
//     uint8_t *base = reinterpret_cast<uint8_t*>(SCRATCHPAD_ADDR);

//     // Place TinyCache in scratchpad memory.
//     base = reinterpret_cast<uint8_t*>(ALIGN_UP(base, alignof(TinyCache)));
//     cache_ptr = new (base) TinyCache;
//     base += sizeof(TinyCache);

//     // Place TinyWorkspace in scratchpad memory.
//     base = reinterpret_cast<uint8_t*>(ALIGN_UP(base, alignof(TinyWorkspace)));
//     work_ptr = new (base) TinyWorkspace;
//     base += sizeof(TinyWorkspace);

//     // Place TinySettings in scratchpad memory.
//     base = reinterpret_cast<uint8_t*>(ALIGN_UP(base, alignof(TinySettings)));
//     settings_ptr = new (base) TinySettings;
//     base += sizeof(TinySettings);

//     // Place TinySolver in scratchpad memory.
//     // Note: TinySolver's constructor takes pointers to the other objects.
//     base = reinterpret_cast<uint8_t*>(ALIGN_UP(base, alignof(TinySolver)));
//     solver_ptr = new (base) TinySolver(settings_ptr, cache_ptr, work_ptr);
//     base += sizeof(TinySolver);
// }


int main(void)
{

    // init_tinympc_globals();

    // Print initial greeting.
    printf("Testing drone control on %s\n", CONFIG_BOARD);

    // Enable vector operations (provided by TinyMPC/ADMM)
    enable_vector_operations();

    printf("Entered MPC simulation main!\n");

    // Temporary vectors for simulation
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

    // Setup initial state x0
    tiny_VectorNx x0, x1;
    tinytype x0_data[NSTATES] = {
        -3.64893626e-02,  3.70428882e-02,  2.25366379e-01, -1.92755080e-01,
        -1.91678221e-01, -2.21354598e-03,  9.62340916e-01, -4.09749891e-01,
        -3.78764621e-01,  7.50158432e-02, -6.63581815e-01,  6.71744046e-01
    };
    x0.set(x0_data);

    // Print CSV header
    printf("Step,TrackingError,x,y,z,phi,theta,psi,dx,dy,dz,dphi,dtheta,dpsi,u1,u2,u3,u4\n");

    // Simulation loop (e.g., 70 steps)
    for (int k = 0; k < 70; ++k) {

        // printf("Starting step %d\n", k);

        // 1. Update measurement: copy current state into work.x (column 0)
        matsetv(work.x.col(0), x0.data, 1, NSTATES);

        // 2. Reset dual variables (if needed)
        work.y = 0.0;
        work.g = 0.0;

        // 3. Solve the MPC problem using ADMM
        // printf("Solving MPC problem...\n");
        tiny_solve(&solver);
        // printf("MPC problem solved, status: %d, iterations: %d\n", work.status, work.iter);


        // 4. Simulate forward
        // Compute: x0 = Adyn*x0 + Bdyn*u.col(0)
#ifdef USE_MATVEC
        matvec(work.Adyn.data, x0.data, v1.data, NSTATES, NSTATES);
        matvec(work.Bdyn.data, work.u.col(0), v2.data, NSTATES, NINPUTS);
#else
        matmul(x0.data, work.Adyn.data, v1.data, 1, NSTATES, NSTATES);
        matmul(work.u.col(0), work.Bdyn.data, v2.data, 1, NSTATES, NINPUTS);
#endif
        matadd(v1.data, v2.data, x0.data, 1, NSTATES);

        // 5. Print simulation results
        printf("%d,", k);
        // Calculate tracking error: norm(x0 - Xref)
        matsub(x0.data, work.Xref.col(1), v1.data, 1, NSTATES);
        float norm = matnorm(v1.data, 1, NSTATES);
        printf("%0.7f,", norm);

        // Print state values
        // for (int i = 0; i < NSTATES; ++i) {
        //     printf("%0.7f", work.x(0, i));
        //     if (i < NSTATES - 1) {
        //         printf(",");
        //     }
        // }
        // printf(",");

        // // Print input values
        // for (int i = 0; i < NINPUTS; ++i) {
        //     printf("%0.7f", work.u(i, 0));
        //     if (i < NINPUTS - 1) {
        //         printf(",");
        //     }
        // }
        // printf("\n");
    }

    // Optionally, you could reboot at the end:
    sys_reboot(SYS_REBOOT_COLD);
    return 0;
}

#ifdef __cplusplus
}
#endif
