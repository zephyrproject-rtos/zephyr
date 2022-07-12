/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_INTEL_ADSP_ACE_IPC_REGS_H
#define ZEPHYR_SOC_INTEL_ADSP_ACE_IPC_REGS_H

#include <intel_adsp_ipc.h>

/**
 * @file
 *
 * Inter Processor Communication - used for sending interrupts to and receiving
 * them from another device. ACE uses it to talk to the host and the CSME.
 * In general there is one of these blocks instantiated for each endpoint of a
 * connection.
 */

/**
 * @brief IPC registers layout for Intel ADSP ACE1X SoC family.
 */
struct intel_adsp_ipc {
	uint32_t tdr;
	uint32_t tda;
	uint32_t unused0[2];
	uint32_t idr;
	uint32_t ida;
	uint32_t unused1[2];
	uint32_t cst;
	uint32_t csr;
	uint32_t ctl;
	uint32_t cap;
	uint32_t unused2[52];
	uint32_t tdd;
	uint32_t unused3[31];
	uint32_t idd;
};

/**
 * @brief ACE SoC family Intra DSP Communication.
 *
 * ACE SoC platform family provides an array of IPC endpoints to be used for
 * peer-to-peer communication between DSP cores - master to slave and backwards.
 * Given endpoint can be accessed by:
 * @code
 * IDC[slave_core_id].agents[agent_id].ipc;
 * @endcode
 */
struct ace_idc {
	/**
	 * @brief IPC Agent Endpoints.
	 *
	 * Each connection is organized into two "agents" ("A" - master core and "B" - slave core).
	 * Each agent is wired to its own interrupt.
	 * Agents array represents mutually exclusive IPC endpoint access:
	 * (A=1/B=0) - agents[0].
	 * (A=0/B=1) - agents[1].
	 */
	union {
		int8_t unused[512];
		struct intel_adsp_ipc ipc;
	} agents[2];
};

/**
 * @brief Defines register for intra DSP communication.
 */
#define IDC ((volatile struct ace_idc *)INTEL_ADSP_IDC_REG_ADDRESS)

#endif /* ZEPHYR_SOC_INTEL_ADSP_ACE_IPC_REGS_H */
