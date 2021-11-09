/*
 * Copyright (c) 2019-2021 Nordic Semiconductor ASA.
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <init.h>
#include <logging/log.h>

#include <soc.h>

LOG_MODULE_REGISTER(bl5340_dvk_cpuapp, CONFIG_LOG_DEFAULT_LEVEL);

#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)

/* This should come from DTS, possibly an overlay. */
#if defined(CONFIG_BOARD_BL5340_DVK_CPUAPP) || \
defined(CONFIG_BOARD_BL5340PA_DVK_CPUAPP)
#define CPUNET_UARTE_PIN_TX  8
#define CPUNET_UARTE_PIN_RX  10
#define CPUNET_UARTE_PORT_TRX NRF_P1
#define CPUNET_UARTE_PIN_RTS 7
#define CPUNET_UARTE_PIN_CTS 9
#define CPUNET_UARTE_PORT_RCTS NRF_P1

#if defined(CONFIG_BOARD_BL5340PA_DVK_CPUAPP)
#define CPUNET_FEM_PIN_TX 30
#define CPUNET_FEM_PIN_RX 31
#define CPUNET_FEM_PIN_PDN 29
#define CPUNET_FEM_PIN_ANT_SEL 4
#define CPUNET_FEM_PIN_SPI_SCK 0
#define CPUNET_FEM_PIN_SPI_MISO 4
#define CPUNET_FEM_PIN_SPI_MOSI 1
#define CPUNET_FEM_PIN_SPI_CS 5
#define CPUNET_FEM_PORT_TX NRF_P0
#define CPUNET_FEM_PORT_RX NRF_P0
#define CPUNET_FEM_PORT_PDN NRF_P0
#define CPUNET_FEM_PORT_ANT_SEL NRF_P0
#define CPUNET_FEM_PORT_SPI_SCK NRF_P1
#define CPUNET_FEM_PORT_SPI_MISO NRF_P1
#define CPUNET_FEM_PORT_SPI_MOSI NRF_P1
#define CPUNET_FEM_PORT_SPI_CS NRF_P1
#endif
#endif

static void remoteproc_mgr_config(void)
{
#if defined(CONFIG_BOARD_ENABLE_CPUNET_UART_PASSTHROUGH)
	/* UARTE */
	/* Assign specific GPIOs that will be used to get UARTE from
	 * nRF5340 Network MCU.
	 */
	CPUNET_UARTE_PORT_TRX->PIN_CNF[CPUNET_UARTE_PIN_TX] =
		GPIO_PIN_CNF_MCUSEL_NetworkMCU << GPIO_PIN_CNF_MCUSEL_Pos;
	CPUNET_UARTE_PORT_TRX->PIN_CNF[CPUNET_UARTE_PIN_RX] =
		GPIO_PIN_CNF_MCUSEL_NetworkMCU << GPIO_PIN_CNF_MCUSEL_Pos;
	CPUNET_UARTE_PORT_RCTS->PIN_CNF[CPUNET_UARTE_PIN_RTS] =
		GPIO_PIN_CNF_MCUSEL_NetworkMCU << GPIO_PIN_CNF_MCUSEL_Pos;
	CPUNET_UARTE_PORT_RCTS->PIN_CNF[CPUNET_UARTE_PIN_CTS] =
		GPIO_PIN_CNF_MCUSEL_NetworkMCU << GPIO_PIN_CNF_MCUSEL_Pos;
#endif

#if defined(CONFIG_BOARD_BL5340PA_DVK_CPUAPP) && defined(CONFIG_BOARD_ENABLE_CPUNET_FEM_PASSTHROUGH)
	/* FEM (GPIO and SPI) */
	/* Assign specific GPIOs that will be used to access FEM from
	 * nRF5340 Network MCU.
	 */
	CPUNET_FEM_PORT_TX->PIN_CNF[CPUNET_FEM_PIN_TX] =
		GPIO_PIN_CNF_MCUSEL_NetworkMCU << GPIO_PIN_CNF_MCUSEL_Pos;
	CPUNET_FEM_PORT_RX->PIN_CNF[CPUNET_FEM_PIN_RX] =
		GPIO_PIN_CNF_MCUSEL_NetworkMCU << GPIO_PIN_CNF_MCUSEL_Pos;
	CPUNET_FEM_PORT_PDN->PIN_CNF[CPUNET_FEM_PIN_PDN] =
		GPIO_PIN_CNF_MCUSEL_NetworkMCU << GPIO_PIN_CNF_MCUSEL_Pos;
	CPUNET_FEM_PORT_ANT_SEL->PIN_CNF[CPUNET_FEM_PIN_ANT_SEL] =
		GPIO_PIN_CNF_MCUSEL_NetworkMCU << GPIO_PIN_CNF_MCUSEL_Pos;
	CPUNET_FEM_PORT_SPI_SCK->PIN_CNF[CPUNET_FEM_PIN_SPI_SCK] =
		GPIO_PIN_CNF_MCUSEL_NetworkMCU << GPIO_PIN_CNF_MCUSEL_Pos;
	CPUNET_FEM_PORT_SPI_MISO->PIN_CNF[CPUNET_FEM_PIN_SPI_MISO] =
		GPIO_PIN_CNF_MCUSEL_NetworkMCU << GPIO_PIN_CNF_MCUSEL_Pos;
	CPUNET_FEM_PORT_SPI_MOSI->PIN_CNF[CPUNET_FEM_PIN_SPI_MOSI] =
		GPIO_PIN_CNF_MCUSEL_NetworkMCU << GPIO_PIN_CNF_MCUSEL_Pos;
	CPUNET_FEM_PORT_SPI_CS->PIN_CNF[CPUNET_FEM_PIN_SPI_CS] =
		GPIO_PIN_CNF_MCUSEL_NetworkMCU << GPIO_PIN_CNF_MCUSEL_Pos;
#endif

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

	/* Release the Network MCU, 'Release force off signal' */
	NRF_RESET->NETWORK.FORCEOFF = RESET_NETWORK_FORCEOFF_FORCEOFF_Release;

	LOG_DBG("Network MCU released.");
#endif /* !CONFIG_TRUSTED_EXECUTION_SECURE */

	return 0;
}

SYS_INIT(remoteproc_mgr_boot, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
