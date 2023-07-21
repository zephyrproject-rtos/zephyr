/*
 * Copyright (c) 2023 Graphcore Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <soc.h>
#include <stm32_ll_spi.h>

#include "stm32_spi_iface.h"

/*
 * Check for SPI_SR_FRE to determine support for TI mode frame format
 * error flag, because STM32F1 SoCs do not support it and  STM32CUBE
 * for F1 family defines an unused LL_SPI_SR_FRE.
 */
#define SPI_STM32_ERR_MSK (LL_SPI_SR_UDR | LL_SPI_SR_CRCE | LL_SPI_SR_MODF | \
            LL_SPI_SR_OVR | LL_SPI_SR_TIFRE)

void ll_func_transmit_data_8(spi_stm32_t* spi, uint32_t val)
{
    SPI_TypeDef* ll_spi = (SPI_TypeDef*)spi;
    LL_SPI_TransmitData8(ll_spi, val);
}

void ll_func_transmit_data_16(spi_stm32_t* spi, uint32_t val)
{
    SPI_TypeDef* ll_spi = (SPI_TypeDef*)spi;
    LL_SPI_TransmitData16(ll_spi, val);
}

uint32_t ll_func_receive_data_8(spi_stm32_t* spi)
{
    SPI_TypeDef* ll_spi = (SPI_TypeDef*)spi;
    return LL_SPI_ReceiveData8(ll_spi);
}

uint32_t ll_func_receive_data_16(spi_stm32_t* spi)
{
    SPI_TypeDef* ll_spi = (SPI_TypeDef*)spi;
    return LL_SPI_ReceiveData16(ll_spi);
}

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)

bool ll_func_is_active_master_transfer(spi_stm32_t* spi)
{
    SPI_TypeDef* ll_spi = (SPI_TypeDef*)spi;
    return LL_SPI_IsActiveMasterTransfer(ll_spi) != 0;
}

void ll_func_start_master_transfer(spi_stm32_t* spi)
{
    SPI_TypeDef* ll_spi = (SPI_TypeDef*)spi;
    LL_SPI_StartMasterTransfer(ll_spi);
}

#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi) */

stm32_spi_mode_t ll_func_get_mode(spi_stm32_t* spi)
{
    SPI_TypeDef* ll_spi = (SPI_TypeDef*)spi;
    uint32_t mode = LL_SPI_GetMode(ll_spi);
    switch (mode) {
        case LL_SPI_MODE_MASTER:
            return STM32_SPI_MASTER;
        case LL_SPI_MODE_SLAVE:
            return STM32_SPI_SLAVE;
        default:
            assert(0);
            break;
    }
    return 0;
}

uint32_t ll_func_tx_is_empty(spi_stm32_t *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	return LL_SPI_IsActiveFlag_TXP(spi);
#else
	return LL_SPI_IsActiveFlag_TXE(spi);
#endif /* st_stm32h7_spi */
}

uint32_t ll_func_rx_is_not_empty(spi_stm32_t *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	return LL_SPI_IsActiveFlag_RXP(spi);
#else
	return LL_SPI_IsActiveFlag_RXNE(spi);
#endif /* st_stm32h7_spi */
}
