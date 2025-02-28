/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/arch/cpu.h>  // Required for arch_curr_cpu()->id

#include <tacit/tacit.h>

#define NUM_ITERS 60 // 0 to 2pi, 60 steps
#define M_PI 3.14159265358979323846

void FOC_invParkTransform(float *v_alpha, float *v_beta, float v_q, float v_d, float sin_theta, float cos_theta) {
  *v_alpha  = -(sin_theta * v_q) + (cos_theta * v_d);
  *v_beta   =  (cos_theta * v_q) + (sin_theta * v_d);
}

void FOC_invClarkSVPWM(float *v_a, float *v_b, float *v_c, float v_alpha, float v_beta) {
  float v_a_phase = v_alpha;
  float v_b_phase = (-.5f * v_alpha) + ((sqrtf(3.f)/2.f) * v_beta);
  float v_c_phase = (-.5f * v_alpha) - ((sqrtf(3.f)/2.f) * v_beta);

  float v_neutral = .5f * (fmaxf(fmaxf(v_a_phase, v_b_phase), v_c_phase) + fminf(fminf(v_a_phase, v_b_phase), v_c_phase));

  *v_a = v_a_phase - v_neutral;
  *v_b = v_b_phase - v_neutral;
  *v_c = v_c_phase - v_neutral;
}

void FOC_update(float *v_a, float *v_b, float *v_c, float vq, float vd) {
  float v_alpha, v_beta;
  FOC_invParkTransform(&v_alpha, &v_beta, vq, vd, sin(vq), cos(vq));
  FOC_invClarkSVPWM(v_a, v_b, v_c, v_alpha, v_beta);
}


int main(void)
{
	LTraceEncoderType *encoder = l_trace_encoder_get(arch_curr_cpu()->id);
    l_trace_encoder_configure_target(encoder, TARGET_PRINT);
    l_trace_encoder_start(encoder);
	// Print to the host via TSI

	printf("Hello World! %s\n", CONFIG_BOARD_TARGET);


	for (int i = 0; i < NUM_ITERS; i++) {
		float vq = 2 * M_PI * i / NUM_ITERS;
		float vd = 0;
		volatile float v_a, v_b, v_c;
		for (int j = 0; j < 2; j++) {
		FOC_update(&v_a, &v_b, &v_c, vq, vd);
		}
	}


	l_trace_encoder_stop(encoder);

	// spin for a bit, to make sure the trace buffer is flushed
    for (int i = 0; i < 10; i++) {
      __asm__("nop");
    }
	// Send an exit command to the host to terminate the simulation
	sys_reboot(SYS_REBOOT_COLD);
	return 0;
}
