/*
 * Copyright (c) 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef STM32_LL_SPI_H_
#define STM32_LL_SPI_H_

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/kernel.h>

typedef uint32_t SPI_TypeDef;
typedef void *clock_control_subsys_t;
struct pinctrl_dev_config;
struct stm32_pclken {
	uint32_t bus;
};

#define ZEPHYR_INCLUDE_DRIVERS_PINCTRL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_STM32_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_STM32_H_
#define PINCTRL_STATE_DEFAULT                    0U
#define PINCTRL_STATE_SLEEP                      1U
#define PINCTRL_DT_DEFINE(id)
#define PINCTRL_DT_DEV_CONFIG_GET(id)            NULL
#define pinctrl_apply_state(cfg, id)             ((void)(cfg), 0)
#define STM32_DMA_CONFIG_DIRECTION(x)            0U
#define STM32_DMA_CONFIG_PERIPHERAL_DATA_SIZE(x) 1U
#define STM32_DMA_CONFIG_MEMORY_DATA_SIZE(x)     1U
#define STM32_DMA_CONFIG_PRIORITY(x)             0U
#define STM32_DMA_FEATURES_FIFO_THRESHOLD(x)     0
#define LL_SPI_DMA_GetRegAddr(spi)               ((void)(spi), 0U)
#define LL_SPI_TransmitData8(spi, d)             ((void)(spi), (void)(mock_spi_last_status = (d)))
#define LL_SPI_ReceiveData8(spi)                 ((void)(spi))
#define LL_SPI_Disable(spi)                      ((void)(spi))
#define LL_SPI_Enable(spi)                       ((void)(spi))
#define LL_SPI_SetClockPolarity(spi, p)          ((void)(spi))
#define LL_SPI_SetClockPhase(spi, p)             ((void)(spi))
#define LL_SPI_SetTransferDirection(spi, d)      ((void)(spi))
#define LL_SPI_SetTransferBitOrder(spi, b)       ((void)(spi))
#define LL_SPI_DisableCRC(spi)                   ((void)(spi))
#define LL_SPI_SetDataWidth(spi, w)              ((void)(spi))
#define LL_SPI_SetNSSMode(spi, n)                ((void)(spi))
#define LL_SPI_SetMode(spi, m)                   ((void)(spi))
#define LL_SPI_EnableDMAReq_RX(spi)              ((void)(spi))
#define LL_SPI_EnableDMAReq_TX(spi)              ((void)(spi))
#define DT_CHOSEN_zephyr_host_cmd_spi_backend               DT_N_S_spi_200
#define DT_N_S_spi_200_COMPAT_MATCHES_st_stm32_spi_host_cmd 1
#define STM32_CLOCK_CONTROL_NODE                            DT_NODELABEL(gpio0)
#define STM32_DT_CLOCKS(id)                                 {{0}}
#undef DT_NUM_CLOCKS
#define DT_NUM_CLOCKS(id)                                   1
#undef DT_DMAS_CTLR_BY_NAME
#define DT_DMAS_CTLR_BY_NAME(id, dir)                       DT_NODELABEL(gpio0)
#define MOCK_DMA_CH_rx                                      0U
#define MOCK_DMA_CH_tx                                      1U
#undef DT_DMAS_CELL_BY_NAME
#define DT_DMAS_CELL_BY_NAME(id, dir, cell)                 MOCK_DMA_CH_##dir
#undef DT_DMAS_CELL_BY_NAME_OR
#define DT_DMAS_CELL_BY_NAME_OR(id, dir, cell, def)         (def)
#define clock_control_on(dev, sys)                          ((void)(dev), (void)(sys), 0)
#define clock_control_off(dev, sys)                         ((void)(dev), (void)(sys), 0)
#define clock_control_configure(dev, sys, d)                ((void)(dev), (void)(sys), 0)

extern uint8_t mock_spi_last_status;
extern int mock_rx_reload_count;
extern dma_callback_t mock_tx_dma_cb;
extern void *mock_tx_dma_user_data;
extern struct k_sem tx_dma_started_sem;

#define dma_config(d, ch, cfg) ((void)(d), (ch) == MOCK_DMA_CH_tx ? \
	(mock_tx_dma_cb = (cfg)->dma_callback, mock_tx_dma_user_data = (cfg)->user_data, 0) : 0)
#define dma_reload(d, ch, src, dst, sz) ((void)(d), (void)(src), (void)(dst), (void)(sz), \
	(ch) == MOCK_DMA_CH_rx ? (++mock_rx_reload_count, 0) : 0)
#define dma_start(d, ch) ((void)(d), (ch) == MOCK_DMA_CH_tx ? \
	(k_sem_give(&tx_dma_started_sem), 0) : 0)
#define dma_stop(d, ch) ((void)(d), (void)(ch))
#define dma_get_status(d, ch, st) ((void)(d), (void)(ch), (st)->pending_length = 0, 0)

#endif /* STM32_LL_SPI_H_ */
