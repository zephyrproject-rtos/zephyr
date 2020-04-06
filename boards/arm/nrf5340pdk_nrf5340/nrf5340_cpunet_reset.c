/*
 * Copyright (c) 2019 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <init.h>
#include <logging/log.h>

#include <soc.h>

LOG_MODULE_REGISTER(nrf5340pdk_nrf5340_cpuapp, CONFIG_LOG_DEFAULT_LEVEL);

#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)

/* This should come from DTS, possibly an overlay. */
#define CPUNET_UARTE_PIN_TX  25
#define CPUNET_UARTE_PIN_RX  26
#define CPUNET_UARTE_PIN_RTS 10
#define CPUNET_UARTE_PIN_CTS 12

static void remoteproc_mgr_config(void)
{
	/* UARTE */
	/* Assign specific GPIOs that will be used to get UARTE from
	 * nRF5340 Network MCU.
	 */
	NRF_P0->PIN_CNF[CPUNET_UARTE_PIN_TX] =
	GPIO_PIN_CNF_MCUSEL_NetworkMCU << GPIO_PIN_CNF_MCUSEL_Pos;
	NRF_P0->PIN_CNF[CPUNET_UARTE_PIN_RX] =
	GPIO_PIN_CNF_MCUSEL_NetworkMCU << GPIO_PIN_CNF_MCUSEL_Pos;
	NRF_P0->PIN_CNF[CPUNET_UARTE_PIN_RTS] =
	GPIO_PIN_CNF_MCUSEL_NetworkMCU << GPIO_PIN_CNF_MCUSEL_Pos;
	NRF_P0->PIN_CNF[CPUNET_UARTE_PIN_CTS] =
	GPIO_PIN_CNF_MCUSEL_NetworkMCU << GPIO_PIN_CNF_MCUSEL_Pos;

	/* Retain nRF5340 Network MCU in Secure domain (bus
	 * accesses by Network MCU will have Secure attribute set).
	 */
	NRF_SPU->EXTDOMAIN[0].PERM = 1 << 4;
}
#endif /* !CONFIG_TRUSTED_EXECUTION_NONSECURE */

static int remoteproc_mgr_boot(struct device *dev)
{
	ARG_UNUSED(dev);

#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	/* Secure domain may configure permissions for the Network MCU. */
	remoteproc_mgr_config();
#endif /* !CONFIG_TRUSTED_EXECUTION_NONSECURE */

#if (DT_IPC_SHM_BASE_ADDRESS != 0)
	/* Initialize inter-processor shared memory block to zero. It is
	 * assumed that the application image has access to the shared
	 * memory at this point (see #24147).
	 */
	memset((void *) DT_IPC_SHM_BASE_ADDRESS, 0, KB(DT_IPC_SHM_SIZE));
#endif

	/* Release the Network MCU, 'Release force off signal' */
	NRF_RESET->NETWORK.FORCEOFF = RESET_NETWORK_FORCEOFF_FORCEOFF_Release;

	LOG_DBG("Network MCU released.");

	return 0;
}

SYS_INIT(remoteproc_mgr_boot, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
