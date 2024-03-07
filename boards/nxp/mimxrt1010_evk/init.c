/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 #include <soc.h>

void SystemInitHook(void)
{
#if DT_SAME_NODE(DT_NODELABEL(flexspi), DT_PARENT(DT_CHOSEN(flash)))
	/* AT25SF128A SPI Flash on the RT1010-EVK requires special alignment
	 * considerations, so set the READADDROPT bit in the FlexSPI so it
	 * will fetch more data than each AHB burst requires to meet alignment
	 * requirements
	 *
	 * Without this, the FlexSPI will return corrupted data during early
	 * boot, causing a hardfault. This can also be resolved by enabling
	 * the instruction cache in very early boot.
	 */
	FLEXSPI->AHBCR |= FLEXSPI_AHBCR_READADDROPT_MASK;
#endif
}
