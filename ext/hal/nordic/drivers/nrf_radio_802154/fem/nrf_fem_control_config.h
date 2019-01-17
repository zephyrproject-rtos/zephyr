/* Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef NRF_FEM_CONTROL_CONFIG_H_
#define NRF_FEM_CONTROL_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @section Timings.
 */

/** Time in us when PA GPIO is activated before radio is ready for transmission. */
#define NRF_FEM_PA_TIME_IN_ADVANCE  23

/** Time in us when LNA GPIO is activated before radio is ready for reception. */
#define NRF_FEM_LNA_TIME_IN_ADVANCE 5

#ifdef NRF52840_XXAA

/** Radio ramp-up time in TX mode, in us. */
#define NRF_FEM_RADIO_TX_STARTUP_LATENCY_US 40

/** Radio ramp-up time in RX mode, in us. */
#define NRF_FEM_RADIO_RX_STARTUP_LATENCY_US 40

#else

#error "Device not supported."

#endif

#ifdef __cplusplus
}
#endif

#endif /* NRF_FEM_CONTROL_CONFIG_H_ */
