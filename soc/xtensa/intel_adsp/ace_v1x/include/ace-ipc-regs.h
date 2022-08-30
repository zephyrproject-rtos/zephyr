/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SOC_INTEL_ADSP_ACE_IPC_REGS_H
#define ZEPHYR_SOC_INTEL_ADSP_ACE_IPC_REGS_H

/**
 * @remark Inter Processor Communication:
 * Used for sending interrupts to and receiving them from another
 * device. ACE uses it to talk to the host and the CSME. In general
 * there is one of these blocks instantiated for each endpoint of a
 * connection.
 */
struct cavs_ipc {
#ifdef CONFIG_SOC_SERIES_INTEL_ACE1X
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
#endif
};

#define CAVS_IPC_BUSY BIT(31)
#define CAVS_IPC_DONE BIT(31)

#define CAVS_IPC_CTL_TBIE BIT(0)
#define CAVS_IPC_CTL_IDIE BIT(1)

/**
 * @remark MTL provides an array of six IPC endpoints to be used for
 * peer-to-peer communication between DSP cores. This is organized as
 * two "agents" (A and B) each wired to its own interrupt. Each agent,
 * has three numbered endpoints (0,1,2), each of which is paired with
 * the equivalent endpoint on the other agent.
 * So a given endpoint on an agent (A=0/B=1) can be found via:
 *     MTL_P2P_IPC[endpoint].agents[agent].ipc
 */
struct mtl_p2p_ipc {
	union {
		int8_t unused[512];
		struct cavs_ipc ipc;
	} agents[2];
};

/**
 * @brief This register is for intra DSP communication, among multiple Tensilica Cores.
 * @todo Move to Devicetree.
 */
#define IDCC_REG	0x70400
#define MTL_P2P_IPC ((volatile struct mtl_p2p_ipc *)IDCC_REG)

#endif /* ZEPHYR_SOC_INTEL_ADSP_ACE_IPC_REGS_H */
