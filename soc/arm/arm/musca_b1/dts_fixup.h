/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* SoC level DTS fixup file */

#define DT_NUM_IRQ_PRIO_BITS	DT_ARM_V8M_NVIC_E000E100_ARM_NUM_IRQ_PRIORITY_BITS

#define DT_NUM_MPU_REGIONS		DT_ARM_ARMV8M_MPU_E000ED90_ARM_NUM_MPU_REGIONS

#if defined (CONFIG_ARM_NONSECURE_FIRMWARE)

/* CMSDK APB Universal Asynchronous Receiver-Transmitter (UART) */
#define DT_PL011_PORT0_BASE_ADDRESS		DT_ARM_PL011_40105000_BASE_ADDRESS
#define DT_PL011_PORT0_IRQ_TX			DT_ARM_PL011_40105000_IRQ_TX
#define DT_PL011_PORT0_IRQ_RX			DT_ARM_PL011_40105000_IRQ_RX
#define DT_PL011_PORT0_IRQ_RXTIM		DT_ARM_PL011_40105000_IRQ_RXTIM
#define DT_PL011_PORT0_IRQ_ERR			DT_ARM_PL011_40105000_IRQ_ERR
#define DT_PL011_PORT0_IRQ_PRI			DT_ARM_PL011_40105000_IRQ_0_PRIORITY
#define DT_PL011_PORT0_CLOCK_FREQUENCY		DT_ARM_PL011_40105000_CLOCKS_CLOCK_FREQUENCY
#define DT_PL011_PORT0_BAUD_RATE		DT_ARM_PL011_40105000_CURRENT_SPEED
#define DT_PL011_PORT0_NAME			DT_ARM_PL011_40105000_LABEL

#define DT_PL011_PORT1_BASE_ADDRESS		DT_ARM_PL011_40106000_BASE_ADDRESS
#define DT_PL011_PORT1_IRQ_TX			DT_ARM_PL011_40106000_IRQ_TX
#define DT_PL011_PORT1_IRQ_RX			DT_ARM_PL011_40106000_IRQ_RX
#define DT_PL011_PORT1_IRQ_RXTIM		DT_ARM_PL011_40106000_IRQ_RXTIM
#define DT_PL011_PORT1_IRQ_ERR			DT_ARM_PL011_40106000_IRQ_ERR
#define DT_PL011_PORT1_IRQ_PRI			DT_ARM_PL011_40106000_IRQ_0_PRIORITY
#define DT_PL011_PORT1_CLOCK_FREQUENCY		DT_ARM_PL011_40106000_CLOCKS_CLOCK_FREQUENCY
#define DT_PL011_PORT1_BAUD_RATE		DT_ARM_PL011_40106000_CURRENT_SPEED
#define DT_PL011_PORT1_NAME			DT_ARM_PL011_40106000_LABEL

/* SCC */
#define DT_ARM_SCC_BASE_ADDRESS			DT_ARM_SCC_4010B000_BASE_ADDRESS

#else

/* CMSDK APB Universal Asynchronous Receiver-Transmitter (UART) */
#define DT_PL011_PORT0_BASE_ADDRESS		DT_ARM_PL011_50105000_BASE_ADDRESS
#define DT_PL011_PORT0_IRQ_TX			DT_ARM_PL011_50105000_IRQ_TX
#define DT_PL011_PORT0_IRQ_RX			DT_ARM_PL011_50105000_IRQ_RX
#define DT_PL011_PORT0_IRQ_RXTIM		DT_ARM_PL011_50105000_IRQ_RXTIM
#define DT_PL011_PORT0_IRQ_ERR			DT_ARM_PL011_50105000_IRQ_ERR
#define DT_PL011_PORT0_IRQ_PRI			DT_ARM_PL011_50105000_IRQ_0_PRIORITY
#define DT_PL011_PORT0_CLOCK_FREQUENCY		DT_ARM_PL011_50105000_CLOCKS_CLOCK_FREQUENCY
#define DT_PL011_PORT0_BAUD_RATE		DT_ARM_PL011_50105000_CURRENT_SPEED
#define DT_PL011_PORT0_NAME			DT_ARM_PL011_50105000_LABEL

#define DT_PL011_PORT1_BASE_ADDRESS		DT_ARM_PL011_50106000_BASE_ADDRESS
#define DT_PL011_PORT1_IRQ_TX			DT_ARM_PL011_50106000_IRQ_TX
#define DT_PL011_PORT1_IRQ_RX			DT_ARM_PL011_50106000_IRQ_RX
#define DT_PL011_PORT1_IRQ_RXTIM		DT_ARM_PL011_50106000_IRQ_RXTIM
#define DT_PL011_PORT1_IRQ_ERR			DT_ARM_PL011_50106000_IRQ_ERR
#define DT_PL011_PORT1_IRQ_PRI			DT_ARM_PL011_50106000_IRQ_0_PRIORITY
#define DT_PL011_PORT1_CLOCK_FREQUENCY		DT_ARM_PL011_50106000_CLOCKS_CLOCK_FREQUENCY
#define DT_PL011_PORT1_BAUD_RATE		DT_ARM_PL011_50106000_CURRENT_SPEED
#define DT_PL011_PORT1_NAME			DT_ARM_PL011_50106000_LABEL

/* SCC */
#define DT_ARM_SCC_BASE_ADDRESS			DT_ARM_SCC_5010B000_BASE_ADDRESS

#endif

/* End of SoC Level DTS fixup file */
