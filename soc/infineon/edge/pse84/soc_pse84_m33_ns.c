/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Infineon PSOC EDGE84 SoC (CM33 non-secure).
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include "cy_pdl.h"
#include <system_edge.h>

#if defined(CONFIG_BUILD_WITH_TFM)
#include "cy_syslib.h"
#include "mtb_srf_pool_init.h"
#endif

#if defined(CONFIG_PSOC_EDGE_M55_SRF_SUPPORT)
#include "cyabs_rtos.h"
#include "mtb_ipc.h"
#include "mtb_ipc_config.h"
#include "mtb_srf_ipc.h"
#include "mtb_srf_ipc_init.h"
#endif

#define PSE84_CPU_FREQ_HZ DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)

/* -------------------------------------------------------------------------- */
/* Common IPC semaphores                                                      */
/* -------------------------------------------------------------------------- */

/* cybsp_ipc_sema is part of the Infineon BSP public ABI (cybsp.h). */
CY_SECTION_SHAREDMEM cy_stc_ipc_sema_t cybsp_ipc_sema;
CY_SECTION_SHAREDMEM static uint32_t pse84_ipc_sema_array[CY_IPC_SEMA_COUNT /
							  CY_IPC_SEMA_PER_WORD];

/* -------------------------------------------------------------------------- */
/* TF-M SRF default pool                                                      */
/* -------------------------------------------------------------------------- */

#if defined(CONFIG_BUILD_WITH_TFM)

mtb_srf_pool_t cy_pdl_srf_default_pool;
CY_SECTION_SHAREDMEM _MTB_SRF_DATA_ALIGN
uint32_t srf_default_pool_memory[(MTB_SRF_POOL_ENTRY_SIZE(MTB_SRF_MAX_ARG_IN_SIZE,
								 MTB_SRF_MAX_ARG_OUT_SIZE) *
					 MTB_SRF_POOL_SIZE) / sizeof(uint32_t)];

static void pse84_srf_pool_init(void)
{
	cy_rslt_t result = mtb_srf_pool_init(&cy_pdl_srf_default_pool,
					     &srf_default_pool_memory[0],
					     MTB_SRF_POOL_SIZE,
					     MTB_SRF_MAX_ARG_IN_SIZE,
					     MTB_SRF_MAX_ARG_OUT_SIZE);

	if (result != CY_RSLT_SUCCESS) {
		/*
		 * If SRF pool initialization fails then panic as TF-M relies
		 * on this pool for handling secure requests.
		 */
		k_panic();
	}
}

#endif /* CONFIG_BUILD_WITH_TFM */

/* -------------------------------------------------------------------------- */
/* CM55 SRF IPC relay                                                         */
/* -------------------------------------------------------------------------- */

#if defined(CONFIG_PSOC_EDGE_M55_SRF_SUPPORT)

#define CM55_BOOT_WAIT_TIME_USEC (10U)

#define MTB_SRF_IPC_SEMA_COUNT \
	(MTB_IPC_SEMA_NUM_SRF_REQ_END - MTB_IPC_SEMA_NUM_SRF_REQ_START + 1)
#define MTB_IPC_IRQ_SEMA_SRF       (MTB_IPC_IRQ_SEMA_SRF_RELAY)
#define MTB_IPC_IRQ_QUEUE_SRF      (MTB_IPC_IRQ_QUEUE_SRF_RELAY)

#define THREAD_PRIO                5
#define SRF_THREAD_STACK_SIZE      4096
#define SRF_RELAY_READY_TIMEOUT_MS 500

/*
 * Shared-memory state.
 *
 * cybsp_cm33_ipc_instance and cybsp_mtb_srf_relay_context are part of the
 * Infineon BSP public ABI (cybsp.h) and must keep their names.
 */
mtb_ipc_t cybsp_cm33_ipc_instance;
mtb_srf_ipc_relay_context_t cybsp_mtb_srf_relay_context;

CY_SECTION_SHAREDMEM mtb_ipc_shared_t pse84_srf_ipc_shared_data _MTB_IPC_DATA_ALIGN;
CY_SECTION_SHAREDMEM mtb_ipc_mbox_data_t pse84_srf_ipc_mbox_data _MTB_IPC_DATA_ALIGN;
CY_SECTION_SHAREDMEM mtb_ipc_semaphore_data_t
	pse84_srf_ipc_sem_data[MTB_SRF_IPC_SEMA_COUNT] _MTB_IPC_DATA_ALIGN;

static mtb_ipc_semaphore_t pse84_srf_ipc_semaphores[MTB_SRF_IPC_SEMA_COUNT];
static mtb_ipc_mbox_receiver_t pse84_srf_ipc_receiver;

/* Relay threads */
K_MSGQ_DEFINE(pse84_srf_request_queue, sizeof(void *), MTB_SRF_POOL_SIZE, 4);
K_SEM_DEFINE(pse84_srf_relay_ready_sem, 0, 2);
K_THREAD_STACK_DEFINE(pse84_srf_receive_stack, SRF_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(pse84_srf_process_stack, SRF_THREAD_STACK_SIZE);
static struct k_thread pse84_srf_receive_thread;
static struct k_thread pse84_srf_process_thread;

static void pse84_srf_ipc_sem_isr(const void *arg)
{
	ARG_UNUSED(arg);
	mtb_ipc_semaphore_process_interrupt(&cybsp_cm33_ipc_instance);
}

static void pse84_srf_ipc_queue_isr(const void *arg)
{
	ARG_UNUSED(arg);
	mtb_ipc_queue_process_interrupt(&cybsp_cm33_ipc_instance);
}

static void pse84_srf_receive_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	k_sem_give(&pse84_srf_relay_ready_sem);
	mtb_srf_ipc_receive_thread(p1);
}

static void pse84_srf_process_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	k_sem_give(&pse84_srf_relay_ready_sem);
	mtb_srf_ipc_process_thread(p1);
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

	memset(&pse84_srf_ipc_shared_data, 0, sizeof(pse84_srf_ipc_shared_data));
	memset(&pse84_srf_ipc_mbox_data, 0, sizeof(pse84_srf_ipc_mbox_data));
	memset(&pse84_srf_ipc_sem_data, 0, sizeof(pse84_srf_ipc_sem_data));
	memset(&pse84_srf_ipc_semaphores, 0, sizeof(pse84_srf_ipc_semaphores));

	for (uint32_t idx = 0; idx < MTB_SRF_IPC_SEMA_COUNT; idx++) {
		srf_ipc_sem_idx_list[idx] = idx + MTB_IPC_SEMA_NUM_SRF_REQ_START;
	}

	/*
	 * The IPC instance and SRF relay are required for any CM55 secure request
	 * to be serviced; if either init fails there is no safe way to continue.
	 */
	if (mtb_ipc_init(&cybsp_cm33_ipc_instance, &pse84_srf_ipc_shared_data,
			 &ipc_config) != CY_RSLT_SUCCESS) {
		k_panic();
	}

	mtb_srf_ipc_relay_init_cfg_t relay_init_cfg = {
		.ipc_instance        = &cybsp_cm33_ipc_instance,
		.mailbox_handle      = &pse84_srf_ipc_receiver,
		.ipc_sem_indexes     = srf_ipc_sem_idx_list,
		.mailbox_data        = &pse84_srf_ipc_mbox_data,
		.semaphore_data      = pse84_srf_ipc_sem_data,
		.semaphore_handles   = pse84_srf_ipc_semaphores,
		.mbox_idx            = MTB_IPC_MBOX_IDX_SRF,
		.mbox_read_sema_idx  = MTB_IPC_SEMA_NUM_SRF_MBOX_READ,
		.mbox_write_sema_idx = MTB_IPC_SEMA_NUM_SRF_MBOX_WRITE,
		.num_semaphores      = MTB_SRF_IPC_SEMA_COUNT,
		.num_requests        = MTB_SRF_POOL_SIZE,
		.ipc_req_queue       = &pse84_srf_request_queue,
	};

	if (mtb_srf_ipc_relay_init(&cybsp_mtb_srf_relay_context,
				   &relay_init_cfg) != CY_RSLT_SUCCESS) {
		k_panic();
	}
}

static void pse84_srf_relay_start(void)
{
	k_thread_create(&pse84_srf_receive_thread, pse84_srf_receive_stack,
			SRF_THREAD_STACK_SIZE, pse84_srf_receive_thread_entry,
			&cybsp_mtb_srf_relay_context, NULL, NULL,
			THREAD_PRIO, 0, K_NO_WAIT);

	k_thread_create(&pse84_srf_process_thread, pse84_srf_process_stack,
			SRF_THREAD_STACK_SIZE, pse84_srf_process_thread_entry,
			&cybsp_mtb_srf_relay_context, NULL, NULL,
			THREAD_PRIO, 0, K_NO_WAIT);

	/* Ensure relay threads have started before CM55 can submit first request. */
	if ((k_sem_take(&pse84_srf_relay_ready_sem,
			K_MSEC(SRF_RELAY_READY_TIMEOUT_MS)) != 0) ||
	    (k_sem_take(&pse84_srf_relay_ready_sem,
			K_MSEC(SRF_RELAY_READY_TIMEOUT_MS)) != 0)) {
		k_panic();
		return;
	}

	uint32_t cm55_start_address = DT_REG_ADDR(DT_NODELABEL(m55_xip));
	#if CONFIG_BOOTLOADER_MCUBOOT
	/* If MCUBoot is present, adjust the start address to the actual
	 * first entry of the vector table.
	 */
	cm55_start_address += CONFIG_ROM_START_OFFSET;
	#endif

	Cy_SysEnableCM55(MXCM55, cm55_start_address, CM55_BOOT_WAIT_TIME_USEC);
}

#endif /* CONFIG_PSOC_EDGE_M55_SRF_SUPPORT */

/* -------------------------------------------------------------------------- */
/* SoC init hooks                                                             */
/* -------------------------------------------------------------------------- */

void soc_early_init_hook(void)
{
	/* Initialize SystemCoreClock variable. */
	SystemCoreClockSetup(PSE84_CPU_FREQ_HZ, PSE84_CPU_FREQ_HZ);

	cybsp_ipc_sema.maxSema     = CY_IPC_SEMA_COUNT;
	cybsp_ipc_sema.arrayPtr    = pse84_ipc_sema_array;
	cybsp_ipc_sema.arrayPtr_sec = NULL;

	Cy_IPC_Sema_InitExt(IPC0_SEMA_CH_NUM, &cybsp_ipc_sema);

#if defined(CONFIG_BUILD_WITH_TFM)
	pse84_srf_pool_init();
#endif

#if defined(CONFIG_PSOC_EDGE_M55_SRF_SUPPORT)
	pse84_srf_ipc_init();
#endif
}

void soc_late_init_hook(void)
{
#if defined(CONFIG_PSOC_EDGE_M55_SRF_SUPPORT)
	pse84_srf_relay_start();
#endif
}
