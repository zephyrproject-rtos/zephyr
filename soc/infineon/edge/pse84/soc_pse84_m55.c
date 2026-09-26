/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Infineon PSOC EDGE84 SoC (CM55).
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <cy_pdl.h>
#include "soc.h"
#include <cy_sysint.h>
#include <system_edge.h>

#if defined(CONFIG_PSOC_EDGE_M55_SRF_SUPPORT)
#include <cy_syslib.h>
#include "mtb_ipc_config.h"
#include <mtb_srf.h>
#include <mtb_srf_ipc_init.h>
#include <mtb_srf_pool_init.h>
#else
#include <ifx_cycfg_init.h>
#endif

#define CY_IPC_MAX_ENDPOINTS (8UL)
#define PSE84_CPU_FREQ_HZ    DT_PROP(DT_PATH(cpus, cpu_1), clock_frequency)
#define CM55_STARTUP_WAIT_MS 50U

/*
 * IPC pipe endpoint table consumed by Cy_IPC_Pipe_Config() during SoC
 * bring-up.  Declared outside the SRF guard because both the SRF
 * (pse84_srf_bringup()) and the non-SRF soc_early_init_hook() paths configure
 * the IPC pipe.  The type comes from cy_pdl.h, which is included in every
 * build.
 */
static cy_stc_ipc_pipe_ep_t system_ipc_pipe_ep_array[CY_IPC_MAX_ENDPOINTS];

/* -------------------------------------------------------------------------- */
/* CM55 SRF client                                                            */
/* -------------------------------------------------------------------------- */

#if defined(CONFIG_PSOC_EDGE_M55_SRF_SUPPORT)

#define MTB_IPC_IRQ_SEMA_SRF  (MTB_IPC_IRQ_SEMA_SRF_CLIENT)
#define MTB_IPC_IRQ_QUEUE_SRF (MTB_IPC_IRQ_QUEUE_SRF_CLIENT)

#define MTB_SRF_IPC_SEMA_COUNT \
	(MTB_IPC_SEMA_NUM_SRF_REQ_END - MTB_IPC_SEMA_NUM_SRF_REQ_START + 1)

#define MTB_IPC_GET_HANDLE_TIMEOUT_MS 1000UL

/*
 * Public ABI (cybsp.h):
 *   cybsp_mtb_srf_client_context, cybsp_cm55_ipc_instance
 *
 * Public ABI (referenced by PDL drivers and system_edge.c):
 *   cy_pdl_srf_default_pool.
 */
mtb_srf_ipc_client_context_t cybsp_mtb_srf_client_context;
mtb_ipc_t cybsp_cm55_ipc_instance;

mtb_srf_pool_t cy_pdl_srf_default_pool;
CY_SECTION_SHAREDMEM _MTB_SRF_DATA_ALIGN
uint32_t srf_default_pool_memory[(MTB_SRF_POOL_ENTRY_SIZE(MTB_SRF_MAX_ARG_IN_SIZE,
								 MTB_SRF_MAX_ARG_OUT_SIZE) *
					 MTB_SRF_POOL_SIZE) / sizeof(uint32_t)];

/* File-local SRF client state */
CY_SECTION_SHAREDMEM _MTB_SRF_DATA_ALIGN static mtb_srf_ipc_packet_t
	pse84_srf_ipc_requests[MTB_SRF_POOL_SIZE];
static mtb_ipc_semaphore_t pse84_srf_ipc_semaphores[MTB_SRF_IPC_SEMA_COUNT];
static mtb_ipc_mbox_sender_t pse84_srf_ipc_sender;
static mtb_srf_ipc_pool_t pse84_srf_ipc_pool;

static void pse84_srf_ipc_sem_isr(const void *arg)
{
	ARG_UNUSED(arg);
	mtb_ipc_semaphore_process_interrupt(&cybsp_cm55_ipc_instance);
}

static void pse84_srf_ipc_queue_isr(const void *arg)
{
	ARG_UNUSED(arg);
	mtb_ipc_queue_process_interrupt(&cybsp_cm55_ipc_instance);
}

static void pse84_srf_pool_init(void)
{
	cy_rslt_t result = mtb_srf_pool_init(&cy_pdl_srf_default_pool,
					     &srf_default_pool_memory[0],
					     MTB_SRF_POOL_SIZE,
					     MTB_SRF_MAX_ARG_IN_SIZE,
					     MTB_SRF_MAX_ARG_OUT_SIZE);

	if (result != CY_RSLT_SUCCESS) {
		/*
		 * The SRF pool is required for every secure request issued by
		 * the PDL drivers; without it nothing on the CM55 side can run.
		 */
		k_panic();
	}
}

static void pse84_srf_ipc_init(void)
{
	mtb_ipc_config_t ipc_config = {
		.internal_channel_index = MTB_IPC_CHANNEL_SRF,
		.semaphore_irq          = MTB_IPC_IRQ_SEMA_SRF,
		.queue_irq              = MTB_IPC_IRQ_QUEUE_SRF,
		.semaphore_num          = MTB_IPC_SEMA_NUM_SRF,
	};
	uint32_t srf_ipc_sem_idx_list[MTB_SRF_IPC_SEMA_COUNT];

	IRQ_CONNECT(CY_IPC_INTR_MUX(MTB_IPC_IRQ_SEMA_SRF), IRQ_PRIO_LOWEST,
		    pse84_srf_ipc_sem_isr, NULL, 0);
	IRQ_CONNECT(CY_IPC_INTR_MUX(MTB_IPC_IRQ_QUEUE_SRF), IRQ_PRIO_LOWEST,
		    pse84_srf_ipc_queue_isr, NULL, 0);
	irq_enable(CY_IPC_INTR_MUX(MTB_IPC_IRQ_SEMA_SRF));
	irq_enable(CY_IPC_INTR_MUX(MTB_IPC_IRQ_QUEUE_SRF));

	for (uint32_t idx = 0; idx < MTB_SRF_IPC_SEMA_COUNT; idx++) {
		srf_ipc_sem_idx_list[idx] = idx + MTB_IPC_SEMA_NUM_SRF_REQ_START;
	}

	/*
	 * The IPC handle and SRF client are required to submit any secure
	 * request to the CM33 relay; if either fails there is no safe way
	 * to continue.
	 */
	if (mtb_ipc_get_handle(&cybsp_cm55_ipc_instance, &ipc_config,
			       MTB_IPC_GET_HANDLE_TIMEOUT_MS) != CY_RSLT_SUCCESS) {
		k_panic();
	}

	mtb_srf_ipc_client_init_cfg_t client_init_cfg = {
		.ipc_instance      = &cybsp_cm55_ipc_instance,
		.mailbox_handle    = &pse84_srf_ipc_sender,
		.ipc_sem_indexes   = srf_ipc_sem_idx_list,
		.srf_ipc_requests  = pse84_srf_ipc_requests,
		.srf_ipc_pool      = &pse84_srf_ipc_pool,
		.semaphore_handles = pse84_srf_ipc_semaphores,
		.mbox_idx          = MTB_IPC_MBOX_IDX_SRF,
		.num_semaphores    = MTB_SRF_IPC_SEMA_COUNT,
		.num_requests      = MTB_SRF_POOL_SIZE,
	};

	if (mtb_srf_ipc_client_init(&cybsp_mtb_srf_client_context,
				    &client_init_cfg) != CY_RSLT_SUCCESS) {
		k_panic();
	}
}

/*
 * Bring up the CM55 SRF/IPC client: initialize the IPC semaphore, the SRF
 * request pool and the IPC/SRF client, publish the system clock and configure
 * the IPC pipe.  Shared verbatim by the cold-boot path (soc_early_init_hook())
 * and the DS-RAM warm-boot re-init (ifx_soc_srf_client_reinit()).  The caller
 * owns any wait needed for the CM33 to publish its endpoint first.
 */
static void pse84_srf_bringup(void)
{
	Cy_IPC_Sema_Init(IPC0_SEMA_CH_NUM, 0, NULL);
	pse84_srf_pool_init();
	pse84_srf_ipc_init();

	SystemCoreClockSetup(PSE84_CPU_FREQ_HZ, PSE84_CPU_FREQ_HZ);

	Cy_IPC_Pipe_Config(system_ipc_pipe_ep_array);
}

#if defined(CONFIG_PM_S2RAM)
void ifx_soc_srf_client_reinit(void)
{
	/*
	 * DeepSleep-RAM warm-boot re-establishment of the CM55 SRF/IPC client.
	 *
	 * On DS-RAM the CM33 secure image cold-boots from scratch at the same time
	 * this core resumes from retained RAM.  soc_early_init_hook() is not re-run
	 * on the warm-boot path, so the SRF client, IPC handle and shared memory are
	 * never re-synchronized with the freshly published CM33 endpoint and the
	 * first secure request faults.  Re-run the same SRF bring-up the cold-boot
	 * path uses to rebuild the client.  Runs in thread context, where the
	 * blocking IPC handle handshake (mtb_ipc_get_handle) is allowed.
	 *
	 * No CM33 startup delay is needed here (unlike cold boot): by the time the
	 * application reaches its post-wake resume the CM33 has long finished
	 * booting, and mtb_ipc_get_handle() blocks until the CM33 endpoint is up.
	 */
	pse84_srf_bringup();
}
#endif /* CONFIG_PM_S2RAM */

#endif /* CONFIG_PSOC_EDGE_M55_SRF_SUPPORT */

/* -------------------------------------------------------------------------- */
/* SoC init hooks                                                             */
/* -------------------------------------------------------------------------- */

void soc_early_init_hook(void)
{
	/* Enable Loop and branch info cache */
	__DMB();
	__ISB();
	SCB_EnableICache();
	SCB_EnableDCache();

#if defined(CONFIG_PSOC_EDGE_M55_SRF_SUPPORT)
	/*
	 * Wait for the CM33 to finish configuring peripherals before we touch
	 * any shared state (IPC semaphores, SRF pool, ...).
	 */
	Cy_SysLib_Delay(CM55_STARTUP_WAIT_MS);

	/* IPC semaphore, SRF pool/client, system clock and IPC pipe. */
	pse84_srf_bringup();
#else
	/* Initializes the system */
	ifx_cycfg_init();

	/* Initialize SystemCoreClock variable. */
	SystemCoreClockSetup(PSE84_CPU_FREQ_HZ, PSE84_CPU_FREQ_HZ);

	Cy_IPC_Pipe_Config(system_ipc_pipe_ep_array);

	/*
	 * Wait for the CM33 to finish configuring peripherals before the CM55
	 * application proceeds.
	 */
	Cy_SysLib_Delay(CM55_STARTUP_WAIT_MS);
#endif
}

cy_israddress Cy_SysInt_SetVector(IRQn_Type IRQn, cy_israddress userIsr)
{
	ARG_UNUSED(IRQn);
	ARG_UNUSED(userIsr);
	return 0;
}
