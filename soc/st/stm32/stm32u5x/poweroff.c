/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>
#include <zephyr/drivers/misc/stm32_wkup_pins/stm32_wkup_pins.h>

#include <stm32_ll_cortex.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_usart.h>
#include <stm32_ll_dma.h>

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32u5_dma)
#define DT_DRV_COMPAT st_stm32_uart
#define TURN_OFF_UART_DMA(inst)									\
	{											\
		USART_TypeDef *usart = (USART_TypeDef *)DT_INST_REG_ADDR(inst);			\
												\
		if (DT_INST_DMAS_HAS_NAME(inst, tx)) {						\
			while (LL_DMA_IsEnabledChannel(						\
				(DMA_TypeDef *)DT_INST_DMAS_CTLR_BY_NAME(inst, tx),		\
				 DT_INST_DMAS_CHANNEL_BY_NAME(inst, tx))) {			\
					/* wait for DMA to be disabled */			\
			}									\
			LL_USART_DisableDMAReq_TX(usart);					\
		}										\
		if (DT_INST_DMAS_HAS_NAME(inst, rx)) {						\
			while (LL_DMA_IsEnabledChannel(						\
				(DMA_TypeDef *)DT_INST_DMAS_CTLR_BY_NAME(inst, rx),		\
				DT_INST_DMAS_CHANNEL_BY_NAME(inst, rx))) {			\
					/* wait for DMA to be disabled */			\
			}									\
			LL_USART_DisableDMAReq_RX(usart);					\
		}										\
	}
#endif

void z_sys_poweroff(void)
{
#ifdef CONFIG_STM32_WKUP_PINS
	stm32_pwr_wkup_pin_cfg_pupd();
#endif /* CONFIG_STM32_WKUP_PINS */

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32u5_dma)
/*
 * Workaround for STM32U5 Errata ES0499:
 * uart_stm32 driver leaves DMA enabled after transfer due to hardware erratum.
 * Explicitly disable DMA here to allow proper shutdown.
 */
	DT_INST_FOREACH_STATUS_OKAY(TURN_OFF_UART_DMA)
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32u5_dma) */

	LL_PWR_ClearFlag_WU();

	LL_PWR_SetPowerMode(LL_PWR_SHUTDOWN_MODE);
	LL_LPM_EnableDeepSleep();

	k_cpu_idle();

	CODE_UNREACHABLE;
}
