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
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
#define SPI_STM32_ERR_MSK (LL_SPI_SR_UDR | LL_SPI_SR_CRCE | LL_SPI_SR_MODF | \
			   LL_SPI_SR_OVR | LL_SPI_SR_TIFRE)
#else
#if defined(LL_SPI_SR_UDR)
#define SPI_STM32_ERR_MSK (LL_SPI_SR_UDR | LL_SPI_SR_CRCERR | LL_SPI_SR_MODF | \
			   LL_SPI_SR_OVR | LL_SPI_SR_FRE)
#elif defined(SPI_SR_FRE)
#define SPI_STM32_ERR_MSK (LL_SPI_SR_CRCERR | LL_SPI_SR_MODF | \
			   LL_SPI_SR_OVR | LL_SPI_SR_FRE)
#else
#define SPI_STM32_ERR_MSK (LL_SPI_SR_CRCERR | LL_SPI_SR_MODF | LL_SPI_SR_OVR)
#endif
#endif /* CONFIG_SOC_SERIES_STM32MP1X */

int ll_func_get_err(spi_stm32_t* spi)
{
    SPI_TypeDef* ll_spi = (SPI_TypeDef*)spi;
    uint32_t sr = LL_SPI_ReadReg(ll_spi, SR);

    if (sr & SPI_STM32_ERR_MSK) {
        /* OVR error must be explicitly cleared */
        if (LL_SPI_IsActiveFlag_OVR(ll_spi)) {
            LL_SPI_ClearFlag_OVR(ll_spi);
        }

        return (sr & SPI_STM32_ERR_MSK);
    }

    return 0;
}

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

bool ll_func_is_nss_polarity_low(spi_stm32_t* spi)
{
    SPI_TypeDef* ll_spi = (SPI_TypeDef*)spi;
    return LL_SPI_GetNSSPolarity(ll_spi) == LL_SPI_NSS_POLARITY_LOW;
}

void ll_func_set_internal_ss_mode_high(spi_stm32_t* spi)
{
    SPI_TypeDef* ll_spi = (SPI_TypeDef*)spi;
    LL_SPI_SetInternalSSLevel(ll_spi, LL_SPI_SS_LEVEL_HIGH);
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

void ll_func_disable_int_tx_empty(spi_stm32_t *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	LL_SPI_DisableIT_TXP(spi);
#else
	LL_SPI_DisableIT_TXE(spi);
#endif /* st_stm32h7_spi */
}

void ll_func_clear_modf_flag(spi_stm32_t* spi)
{
    SPI_TypeDef* ll_spi = (SPI_TypeDef*)spi;
    LL_SPI_ClearFlag_MODF(ll_spi);
}

bool ll_func_is_modf_flag_set(spi_stm32_t* spi)
{
    SPI_TypeDef* ll_spi = (SPI_TypeDef*)spi;
    return LL_SPI_IsActiveFlag_MODF(ll_spi) != 0;
}

static void disable_spi(SPI_TypeDef *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	if (LL_SPI_IsActiveMasterTransfer(spi)) {
		LL_SPI_SuspendMasterTransfer(spi);
		while (LL_SPI_IsActiveMasterTransfer(spi)) {
			/* NOP */
		}
	}

	LL_SPI_Disable(spi);
	while (LL_SPI_IsEnabled(spi)) {
		/* NOP */
	}

	/* Flush RX buffer */
	while (LL_SPI_IsActiveFlag_RXP(spi)) {
		(void)LL_SPI_ReceiveData8(spi);
	}
	LL_SPI_ClearFlag_SUSP(spi);
#else
	LL_SPI_Disable(spi);
#endif /* st_stm32h7_spi */
}

void ll_func_enable_spi(spi_stm32_t* spi, bool enable)
{
    SPI_TypeDef* ll_spi = (SPI_TypeDef*)spi;
    if (enable) {
        LL_SPI_Enable(ll_spi);
    } else {
        disable_spi(ll_spi);
    }
}

void ll_func_disable_int_rx_not_empty(spi_stm32_t *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	LL_SPI_DisableIT_RXP(spi);
#else
	LL_SPI_DisableIT_RXNE(spi);
#endif /* st_stm32h7_spi */
}

void ll_func_disable_int_errors(spi_stm32_t *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	LL_SPI_DisableIT_UDR(spi);
	LL_SPI_DisableIT_OVR(spi);
	LL_SPI_DisableIT_CRCERR(spi);
	LL_SPI_DisableIT_FRE(spi);
	LL_SPI_DisableIT_MODF(spi);
#else
	LL_SPI_DisableIT_ERR(spi);
#endif /* st_stm32h7_spi */
}

uint32_t ll_func_spi_is_busy(spi_stm32_t *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	return LL_SPI_IsActiveFlag_EOT(spi);
#else
	return LL_SPI_IsActiveFlag_BSY(spi);
#endif /* st_stm32h7_spi */
}

static int set_spi_standard_ti(SPI_TypeDef* spi)
{
#ifndef LL_SPI_PROTOCOL_TI
    /* on stm32F1 or some stm32L1 (cat1,2) without SPI_CR2_FRF */
    return -ENOTSUP;
#else
    LL_SPI_SetStandard(spi, LL_SPI_PROTOCOL_TI);
    return 0;
#endif
}

static int set_spi_standard_motorola(SPI_TypeDef* spi)
{
#if defined(LL_SPI_PROTOCOL_MOTOROLA) && defined(SPI_CR2_FRF)
    LL_SPI_SetStandard(spi, LL_SPI_PROTOCOL_MOTOROLA);
#endif
    return 0;
}

int ll_func_set_standard(spi_stm32_t* spi, stm32_standard_t st)
{
    SPI_TypeDef* ll_spi = (SPI_TypeDef*)spi;
    switch (st) {
        case STM32_SPI_STANDARD_TI:
            return set_spi_standard_ti(ll_spi);
        case STM32_SPI_STANDARD_MOTOROLA:
            return set_spi_standard_motorola(ll_spi);
        default:
            assert(0);
            return -EINVAL;
    }
    return 0;
}


int ll_func_set_baudrate_prescaler(spi_stm32_t* spi,
                                   uint32_t spi_periph_clk,
                                   uint32_t target_spi_frequency)
{
    const uint32_t scaler[] = {
        LL_SPI_BAUDRATEPRESCALER_DIV2,
        LL_SPI_BAUDRATEPRESCALER_DIV4,
        LL_SPI_BAUDRATEPRESCALER_DIV8,
        LL_SPI_BAUDRATEPRESCALER_DIV16,
        LL_SPI_BAUDRATEPRESCALER_DIV32,
        LL_SPI_BAUDRATEPRESCALER_DIV64,
        LL_SPI_BAUDRATEPRESCALER_DIV128,
        LL_SPI_BAUDRATEPRESCALER_DIV256
    };

    SPI_TypeDef* ll_spi = (SPI_TypeDef*)spi;

    int br;
    for (br = 1 ; br <= ARRAY_SIZE(scaler) ; ++br) {
        uint32_t clk = spi_periph_clk >> br;

        if (clk <= target_spi_frequency) {
            break;
        }
    }

    if (br > ARRAY_SIZE(scaler)) {
        return -EINVAL;
    }

    LL_SPI_SetBaudRatePrescaler(ll_spi, scaler[br - 1]);

    return 0;
}


void ll_func_set_polarity(spi_stm32_t* spi, stm32_spi_cpol_t cpol)
{
    SPI_TypeDef* ll_spi = (SPI_TypeDef*)spi;
    switch (cpol) {
        case STM32_SPI_CPOL_0:
            LL_SPI_SetClockPolarity(ll_spi, LL_SPI_POLARITY_LOW);
            break;
        case STM32_SPI_CPOL_1:
            LL_SPI_SetClockPolarity(ll_spi, LL_SPI_POLARITY_HIGH);
            break;
        default:
            assert(0);
            break;
    }
}

void ll_func_set_clock_phase(spi_stm32_t* spi, stm32_spi_cpha_t cpha)
{
    SPI_TypeDef* ll_spi = (SPI_TypeDef*)spi;
    switch (cpha) {
        case STM32_SPI_CPHA_0:
            LL_SPI_SetClockPhase(ll_spi, LL_SPI_PHASE_1EDGE);
            break;
        case STM32_SPI_CPHA_1:
            LL_SPI_SetClockPhase(ll_spi, LL_SPI_PHASE_2EDGE);
            break;
        default:
            assert(0);
            break;
    }
}

void ll_func_set_transfer_direction_full_duplex(spi_stm32_t* spi)
{
    SPI_TypeDef* ll_spi = (SPI_TypeDef*)spi;
    LL_SPI_SetTransferDirection(ll_spi, LL_SPI_FULL_DUPLEX);
}

void ll_func_set_bit_order(spi_stm32_t* spi, stm32_spi_bit_order_t bit_order)
{
    SPI_TypeDef* ll_spi = (SPI_TypeDef*)spi;
    switch (bit_order) {
        case STM32_SPI_LSB_FIRST:
            LL_SPI_SetTransferBitOrder(ll_spi, LL_SPI_LSB_FIRST);
            break;
        case STM32_SPI_MSB_FIRST:
            LL_SPI_SetTransferBitOrder(ll_spi, LL_SPI_MSB_FIRST);
            break;
        default:
            assert(0);
            break;
    }
}

void ll_func_disable_crc(spi_stm32_t* spi)
{
    SPI_TypeDef* ll_spi = (SPI_TypeDef*)spi;
    LL_SPI_DisableCRC(ll_spi);
}

void ll_func_set_nss_mode(spi_stm32_t* spi, stm32_spi_nss_mode_t mode)
{
    SPI_TypeDef* ll_spi = (SPI_TypeDef*)spi;
    switch (mode) {
        case STM32_SPI_NSS_SOFT:
            LL_SPI_SetNSSMode(ll_spi, LL_SPI_NSS_SOFT);
            break;
        case STM32_SPI_NSS_HARD_OUTPUT:
            LL_SPI_SetNSSMode(ll_spi, LL_SPI_NSS_HARD_OUTPUT);
            break;
        case STM32_SPI_NSS_HARD_INPUT:
            LL_SPI_SetNSSMode(ll_spi, LL_SPI_NSS_HARD_INPUT);
            break;
        default:
            assert(0);
            break;
    }
}

void ll_func_set_mode(spi_stm32_t* spi, stm32_spi_mode_t mode)
{
    SPI_TypeDef* ll_spi = (SPI_TypeDef*)spi;
    switch (mode) {
        case STM32_SPI_MASTER:
            LL_SPI_SetMode(ll_spi, LL_SPI_MODE_MASTER);
            break;
        case STM32_SPI_SLAVE:
            LL_SPI_SetMode(ll_spi, LL_SPI_MODE_SLAVE);
            break;
        default:
            assert(0);
            break;
    }
}

void ll_func_set_data_width(spi_stm32_t* spi, stm32_spi_data_width_t data_width)
{
    SPI_TypeDef* ll_spi = (SPI_TypeDef*)spi;
    switch (data_width) {
        case STM32_SPI_DATA_WIDTH_8:
            LL_SPI_SetDataWidth(ll_spi, LL_SPI_DATAWIDTH_8BIT);
            break;
        case STM32_SPI_DATA_WIDTH_16:
            LL_SPI_SetDataWidth(ll_spi, LL_SPI_DATAWIDTH_16BIT);
            break;
        default:
            assert(0);
            break;
    }
}


#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_spi_fifo)

void ll_func_set_fifo_threshold_8bit(spi_stm32_t *spi)
{
#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32h7_spi)
	LL_SPI_SetFIFOThreshold(spi, LL_SPI_FIFO_TH_01DATA);
#else
	LL_SPI_SetRxFIFOThreshold(spi, LL_SPI_RX_FIFO_TH_QUARTER);
#endif /* st_stm32h7_spi */
}

#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32_spi_fifo) */
