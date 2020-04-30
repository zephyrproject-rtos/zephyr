/*
 * Copyright (c) 2019-2020 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <init.h>
#include <logging/log.h>

#include <soc.h>

LOG_MODULE_REGISTER(nrf5340pdk_nrf5340_cpuapp, CONFIG_LOG_DEFAULT_LEVEL);

/* Shared memory definitions */
#if DT_HAS_CHOSEN(zephyr_ipc_shm)
#define SHM_NODE            DT_CHOSEN(zephyr_ipc_shm)
#define SHM_BASE_ADDRESS    DT_REG_ADDR(SHM_NODE)
#define SHM_SIZE            DT_REG_SIZE(SHM_NODE)
#endif

#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)

/* This should come from DTS, possibly an overlay. */
#if defined(CONFIG_BOARD_NRF5340PDK_NRF5340_CPUAPP)
#define CPUNET_UARTE_PIN_TX  25
#define CPUNET_UARTE_PIN_RX  26
#define CPUNET_UARTE_PORT_TRX NRF_P0
#define CPUNET_UARTE_PIN_RTS 10
#define CPUNET_UARTE_PIN_CTS 12
#elif defined(CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP)
#define CPUNET_UARTE_PIN_TX  1
#define CPUNET_UARTE_PIN_RX  0
#define CPUNET_UARTE_PORT_TRX NRF_P1
#define CPUNET_UARTE_PIN_RTS 11
#define CPUNET_UARTE_PIN_CTS 10
#endif

#if defined(CONFIG_BT_CTLR_DEBUG_PINS_CPUAPP)
#include <../subsys/bluetooth/controller/ll_sw/nordic/hal/nrf5/debug.h>
#else
#define DEBUG_SETUP()
#endif

static void remoteproc_mgr_config(void)
{
	/* UARTE */
	/* Assign specific GPIOs that will be used to get UARTE from
	 * nRF5340 Network MCU.
	 */
	CPUNET_UARTE_PORT_TRX->PIN_CNF[CPUNET_UARTE_PIN_TX] =
	GPIO_PIN_CNF_MCUSEL_NetworkMCU << GPIO_PIN_CNF_MCUSEL_Pos;
	CPUNET_UARTE_PORT_TRX->PIN_CNF[CPUNET_UARTE_PIN_RX] =
	GPIO_PIN_CNF_MCUSEL_NetworkMCU << GPIO_PIN_CNF_MCUSEL_Pos;
	NRF_P0->PIN_CNF[CPUNET_UARTE_PIN_RTS] =
	GPIO_PIN_CNF_MCUSEL_NetworkMCU << GPIO_PIN_CNF_MCUSEL_Pos;
	NRF_P0->PIN_CNF[CPUNET_UARTE_PIN_CTS] =
	GPIO_PIN_CNF_MCUSEL_NetworkMCU << GPIO_PIN_CNF_MCUSEL_Pos;

	/* Route Bluetooth Controller Debug Pins */
	DEBUG_SETUP();

	/* Retain nRF5340 Network MCU in Secure domain (bus
	 * accesses by Network MCU will have Secure attribute set).
	 */
	NRF_SPU->EXTDOMAIN[0].PERM = 1 << 4;
}
#endif /* !CONFIG_TRUSTED_EXECUTION_NONSECURE */

static int remoteproc_mgr_boot(const struct device *dev)
{
	ARG_UNUSED(dev);

#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	/* Secure domain may configure permissions for the Network MCU. */
	remoteproc_mgr_config();
#endif /* !CONFIG_TRUSTED_EXECUTION_NONSECURE */

#if !defined(CONFIG_TRUSTED_EXECUTION_SECURE)
	/*
	 * Building Zephyr with CONFIG_TRUSTED_EXECUTION_SECURE=y implies
	 * building also a Non-Secure image. The Non-Secure image will, in
	 * this case do the remainder of actions to properly configure and
	 * boot the Network MCU.
	 */
#if defined(SHM_BASE_ADDRESS) && (SHM_BASE_ADDRESS != 0)

	/* Initialize inter-processor shared memory block to zero. It is
	 * assumed that the application image has access to the shared
	 * memory at this point (see #24147).
	 */
	memset((void *) SHM_BASE_ADDRESS, 0, SHM_SIZE);
#endif

	/* Release the Network MCU, 'Release force off signal' */
	NRF_RESET->NETWORK.FORCEOFF = RESET_NETWORK_FORCEOFF_FORCEOFF_Release;

	LOG_DBG("Network MCU released.");
#endif /* !CONFIG_TRUSTED_EXECUTION_SECURE */

	return 0;
}

SYS_INIT(remoteproc_mgr_boot, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
