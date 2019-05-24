/*
 * Copyright (c) 2014 - 2019, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef NRF_QDEC_H__
#define NRF_QDEC_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_qdec_hal QDEC HAL
 * @{
 * @ingroup nrf_qdec
 * @brief   Hardware access layer for managing the Quadrature Decoder (QDEC) peripheral.
 */

/**
 * @brief This value can be provided as a parameter for the @ref nrf_qdec_pio_assign
 *        function call to specify that a LED signal shall not be use by the QDEC and
 *        connected to a physical pin.
 */
#define NRF_QDEC_LED_NOT_CONNECTED  0xFFFFFFFF

/** @brief QDEC tasks. */
typedef enum
{
    NRF_QDEC_TASK_START      = offsetof(NRF_QDEC_Type, TASKS_START),     /**< Starting the quadrature decoder. */
    NRF_QDEC_TASK_STOP       = offsetof(NRF_QDEC_Type, TASKS_STOP),      /**< Stopping the quadrature decoder. */
    NRF_QDEC_TASK_READCLRACC = offsetof(NRF_QDEC_Type, TASKS_READCLRACC) /**< Reading and clearing ACC and ACCDBL registers. */
} nrf_qdec_task_t;

/** @brief QDEC events. */
typedef enum
{
    NRF_QDEC_EVENT_SAMPLERDY = offsetof(NRF_QDEC_Type, EVENTS_SAMPLERDY), /**< Event generated for every new sample.  */
    NRF_QDEC_EVENT_REPORTRDY = offsetof(NRF_QDEC_Type, EVENTS_REPORTRDY), /**< Event generated for every new report.  */
    NRF_QDEC_EVENT_ACCOF     = offsetof(NRF_QDEC_Type, EVENTS_ACCOF)      /**< Event generated for every accumulator overflow. */
} nrf_qdec_event_t;

/** @brief QDEC shortcuts. */
typedef enum
{
    NRF_QDEC_SHORT_REPORTRDY_READCLRACC_MASK = QDEC_SHORTS_REPORTRDY_READCLRACC_Msk, /**< Shortcut between REPORTRDY event and READCLRACC task.  */
    NRF_QDEC_SHORT_SAMPLERDY_STOP_MASK       = QDEC_SHORTS_SAMPLERDY_STOP_Msk        /**< Shortcut between SAMPLERDY event and STOP task.  */
} nrf_qdec_short_mask_t;

/** @brief QDEC interrupts. */
typedef enum
{
    NRF_QDEC_INT_SAMPLERDY_MASK = QDEC_INTENSET_SAMPLERDY_Msk, /**< Mask for enabling or disabling an interrupt on SAMPLERDY event.  */
    NRF_QDEC_INT_REPORTRDY_MASK = QDEC_INTENSET_REPORTRDY_Msk, /**< Mask for enabling or disabling an interrupt on REPORTRDY event.  */
    NRF_QDEC_INT_ACCOF_MASK     = QDEC_INTENSET_ACCOF_Msk      /**< Mask for enabling or disabling an interrupt on ACCOF event.  */
} nrf_qdec_int_mask_t;

/** @brief States of the enable bit. */
typedef enum
{
    NRF_QDEC_DISABLE = QDEC_ENABLE_ENABLE_Disabled, /**< Mask for disabling the QDEC periperal. When disabled, the QDEC decoder pins are not active.  */
    NRF_QDEC_ENABLE  = QDEC_ENABLE_ENABLE_Enabled   /**< Mask for enabling the QDEC periperal. When enabled, the QDEC pins are active. */
} nrf_qdec_enable_t;

/** @brief States of the debounce filter enable bit. */
typedef enum
{
    NRF_QDEC_DBFEN_DISABLE = QDEC_DBFEN_DBFEN_Disabled, /**< Mask for disabling the debounce filter.  */
    NRF_QDEC_DBFEN_ENABLE  = QDEC_DBFEN_DBFEN_Enabled   /**< Mask for enabling the debounce filter.  */
} nrf_qdec_dbfen_t;

/** @brief Active LED polarity. */
typedef enum
{
    NRF_QDEC_LEPOL_ACTIVE_LOW  = QDEC_LEDPOL_LEDPOL_ActiveLow, /**< QDEC LED active on output pin low.  */
    NRF_QDEC_LEPOL_ACTIVE_HIGH = QDEC_LEDPOL_LEDPOL_ActiveHigh /**< QDEC LED active on output pin high.  */
} nrf_qdec_ledpol_t;

/** @brief Available sampling periods. */
typedef enum
{
    NRF_QDEC_SAMPLEPER_128us   = QDEC_SAMPLEPER_SAMPLEPER_128us,  /**< QDEC sampling period 128 microseconds.  */
    NRF_QDEC_SAMPLEPER_256us   = QDEC_SAMPLEPER_SAMPLEPER_256us,  /**< QDEC sampling period 256 microseconds.  */
    NRF_QDEC_SAMPLEPER_512us   = QDEC_SAMPLEPER_SAMPLEPER_512us,  /**< QDEC sampling period 512 microseconds.  */
    NRF_QDEC_SAMPLEPER_1024us  = QDEC_SAMPLEPER_SAMPLEPER_1024us, /**< QDEC sampling period 1024 microseconds.  */
    NRF_QDEC_SAMPLEPER_2048us  = QDEC_SAMPLEPER_SAMPLEPER_2048us, /**< QDEC sampling period 2048 microseconds.  */
    NRF_QDEC_SAMPLEPER_4096us  = QDEC_SAMPLEPER_SAMPLEPER_4096us, /**< QDEC sampling period 4096 microseconds.  */
    NRF_QDEC_SAMPLEPER_8192us  = QDEC_SAMPLEPER_SAMPLEPER_8192us, /**< QDEC sampling period 8192 microseconds.  */
    NRF_QDEC_SAMPLEPER_16384us = QDEC_SAMPLEPER_SAMPLEPER_16384us /**< QDEC sampling period 16384 microseconds.  */
} nrf_qdec_sampleper_t;

/** @brief Available report periods. */
typedef enum
{
    NRF_QDEC_REPORTPER_10  = QDEC_REPORTPER_REPORTPER_10Smpl,  /**< QDEC report period 10 samples.  */
    NRF_QDEC_REPORTPER_40  = QDEC_REPORTPER_REPORTPER_40Smpl,  /**< QDEC report period 40 samples.  */
    NRF_QDEC_REPORTPER_80  = QDEC_REPORTPER_REPORTPER_80Smpl,  /**< QDEC report period 80 samples.  */
    NRF_QDEC_REPORTPER_120 = QDEC_REPORTPER_REPORTPER_120Smpl, /**< QDEC report period 120 samples. */
    NRF_QDEC_REPORTPER_160 = QDEC_REPORTPER_REPORTPER_160Smpl, /**< QDEC report period 160 samples. */
    NRF_QDEC_REPORTPER_200 = QDEC_REPORTPER_REPORTPER_200Smpl, /**< QDEC report period 200 samples. */
    NRF_QDEC_REPORTPER_240 = QDEC_REPORTPER_REPORTPER_240Smpl, /**< QDEC report period 240 samples. */
    NRF_QDEC_REPORTPER_280 = QDEC_REPORTPER_REPORTPER_280Smpl, /**< QDEC report period 280 samples. */
    NRF_QDEC_REPORTPER_DISABLED                                /**< QDEC reporting disabled.        */
} nrf_qdec_reportper_t;

/** @brief Function for enabling QDEC. */
__STATIC_INLINE void nrf_qdec_enable(void);

/** @brief Function for disabling QDEC. */
__STATIC_INLINE void nrf_qdec_disable(void);

/**
 * @brief Function for returning the enable state of QDEC.
 *
 * @return State of the register.
 */
__STATIC_INLINE uint32_t nrf_qdec_enable_get(void);

/**
 * @brief Function for enabling QDEC interrupts by mask.
 *
 * @param[in] mask Mask of interrupts to be enabled.
 */
__STATIC_INLINE void nrf_qdec_int_enable(uint32_t mask);

/**
 * @brief Function for disabling QDEC interrupts by mask.
 *
 * @param[in] mask Mask of interrupts to be disabled.
 */
__STATIC_INLINE void nrf_qdec_int_disable(uint32_t mask);

/**
 * @brief Function for getting the enabled interrupts of the QDEC.
 *
 * @param[in] mask Mask of interrupts to be checked.
 *
 * @return Bitfield with enabled interrupts.
 */
__STATIC_INLINE uint32_t nrf_qdec_int_enable_check(nrf_qdec_int_mask_t mask);

/** @brief Function for enabling the QDEC debouncing filter. */
__STATIC_INLINE void nrf_qdec_dbfen_enable(void);

/** @brief Function for disabling the QDEC debouncing filter. */
__STATIC_INLINE void nrf_qdec_dbfen_disable(void);

/**
 * @brief Function for getting the state of the QDEC debouncing filter.
 *
 * @retval NRF_QDEC_DBFEN_DISABLE The debouncing filter is disabled.
 * @retval NRF_QDEC_DBFEN_ENABLE  The debouncing filter is enabled.
 */
__STATIC_INLINE uint32_t nrf_qdec_dbfen_get(void);

/**
 * @brief Function for assigning QDEC pins.
 *
 * @param[in] psela   Pin number.
 * @param[in] pselb   Pin number.
 * @param[in] pselled Pin number.
 */
__STATIC_INLINE void nrf_qdec_pio_assign( uint32_t psela, uint32_t pselb, uint32_t pselled);

/**
 * @brief Function for setting the specified QDEC task.
 *
 * @param[in] task QDEC task to be triggered.
 */
__STATIC_INLINE void nrf_qdec_task_trigger(nrf_qdec_task_t task);

/**
 * @brief Function for retrieving the address of a QDEC task register.
 *
 * @param[in] task QDEC task to get its address.
 *
 * @return Address of the specified QDEC task.
 */
__STATIC_INLINE uint32_t * nrf_qdec_task_address_get(nrf_qdec_task_t task);

/**
 * @brief Function for clearing the specified QDEC event.
 *
 * @param[in] event QDEC event to be cleared.
 */
__STATIC_INLINE void nrf_qdec_event_clear(nrf_qdec_event_t event);

/**
 * @brief Function for getting the state of the specified QDEC event.
 *
 * @param[in] event QDEC event to be checked.
 *
 * @return State of the specified QDEC event.
 */
__STATIC_INLINE uint32_t nrf_qdec_event_check(nrf_qdec_event_t event);

/**
 * @brief Function for retrieving the address of the specified QDEC event register.
 *
 * @param[in] event QDEC event to get its address.
 *
 * @return Address of the specified QDEC event.
 */
__STATIC_INLINE uint32_t * nrf_qdec_event_address_get(nrf_qdec_event_t event);

/**
 * @brief Function for setting QDEC shortcuts.
 *
 * @param[in] mask Mask of QDEC shortcuts to be set.
 */
__STATIC_INLINE void nrf_qdec_shorts_enable(uint32_t mask);

/**
 * @brief Function for clearing shortcuts of the QDEC by mask.
 *
 * @param[in] mask Mask of QDEC shortcuts to be cleared.
 */
__STATIC_INLINE void nrf_qdec_shorts_disable(uint32_t mask);

/**
 * @brief Function for retrieving value of the sampling period.
 *
 * @return Sample period @ref nrf_qdec_sampleper_t.
 */
__STATIC_INLINE int32_t nrf_qdec_sampleper_reg_get(void);

/**
 * @brief Function for converting return value of the @p nrf_qdec_sampleper_reg_get() function
 *        to microseconds.
 *
 * @param[in] sampleper The sampling period.
 *
 * @return Period in microseconds.
 */
__STATIC_INLINE uint32_t nrf_qdec_sampleper_to_value(uint32_t sampleper);

/**
 * @brief Function for setting value of the QDEC sampling period.
 *
 * @param[in] sample_per The sampling period.
 */
__STATIC_INLINE void nrf_qdec_sampleper_set(nrf_qdec_sampleper_t sample_per);

/**
 * @brief Function for retrieving value of the QDEC SAMPLE register.
 *
 * @return Value of the SAMPLE register.
 */
__STATIC_INLINE int32_t nrf_qdec_sample_get(void);

/**
 * @brief Function for retrieving value of the QDEC ACC register.
 *
 * @return Value of the ACC register.
 */
__STATIC_INLINE int32_t nrf_qdec_acc_get(void);

/**
 * @brief Function for retrieving value of the QDEC ACCREAD register.
 *
 * @return Value of the ACCREAD register.
 */
__STATIC_INLINE int32_t nrf_qdec_accread_get(void);

/**
 * @brief Function for retrieving value of the QDEC ACCDBL register.
 *
 * @return Value of the ACCDBL register.
 */
__STATIC_INLINE uint32_t nrf_qdec_accdbl_get(void);

/**
 * @brief Function for retrieving value of the QDEC ACCDBLREAD register.
 *
 * @return Value of the ACCDBLREAD register.
 */
__STATIC_INLINE uint32_t nrf_qdec_accdblread_get(void);

/**
 * @brief Function for setting delay time between setting LED active state and start sampling.
 *
 * @param[in] time_us Delay time (in microseconds) between setting LED active state
 *                    and start sampling.
 */
__STATIC_INLINE void nrf_qdec_ledpre_set(uint32_t time_us);

/**
 * @brief Function for retrieving how long the LED is switched on before sampling.
 *
 * @return The gap in time in microseconds between switched LED to active state and start sampling.
 */
__STATIC_INLINE uint32_t nrf_qdec_ledpre_get(void);

/**
 * @brief Function for setting the report period (in samples).
 *
 * @param[in] reportper The number of samples.
 */
__STATIC_INLINE void nrf_qdec_reportper_set(nrf_qdec_reportper_t reportper);

/**
 * @brief Function for retrieving the report period.
 *
 * @return The report period.
 */
__STATIC_INLINE uint32_t nrf_qdec_reportper_reg_get(void);

/**
 * @brief Function for retrieving the value of QDEC SAMPLEPER register.
 *
 * @param [in] reportper Reportper to be converted to amount of samples per report.
 *
 * @return Number of samples per report.
 */
__STATIC_INLINE uint32_t nrf_qdec_reportper_to_value(uint32_t reportper);

/**
 * @brief Function for setting the active level for the LED.
 *
 * @param[in] pol Level of the active signal of the LED.
 */
__STATIC_INLINE void nrf_qdec_ledpol_set(nrf_qdec_ledpol_t pol);

/**
 * @brief Function for retrieving the active level for the LED.
 *
 * @return Level of the active signal of the LED.
 */
__STATIC_INLINE uint32_t nrf_qdec_ledpol_get(void);


#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrf_qdec_enable(void)
{
    NRF_QDEC->ENABLE = NRF_QDEC_ENABLE;
}

__STATIC_INLINE void nrf_qdec_disable(void)
{
    NRF_QDEC->ENABLE = NRF_QDEC_DISABLE;
}

__STATIC_INLINE uint32_t nrf_qdec_enable_get(void)
{
    return NRF_QDEC->ENABLE;
}

__STATIC_INLINE void nrf_qdec_int_enable(uint32_t mask)
{
    NRF_QDEC->INTENSET = mask; // writing 0 has no effect
}

__STATIC_INLINE void nrf_qdec_int_disable(uint32_t mask)
{
    NRF_QDEC->INTENCLR = mask; // writing 0 has no effect
}

__STATIC_INLINE uint32_t nrf_qdec_int_enable_check(nrf_qdec_int_mask_t mask)
{
    return NRF_QDEC->INTENSET & mask; // when read this register will return the value of INTEN.
}

__STATIC_INLINE void nrf_qdec_dbfen_enable(void)
{
    NRF_QDEC->DBFEN = NRF_QDEC_DBFEN_ENABLE;
}

__STATIC_INLINE void nrf_qdec_dbfen_disable(void)
{
    NRF_QDEC->DBFEN = NRF_QDEC_DBFEN_DISABLE;
}

__STATIC_INLINE uint32_t nrf_qdec_dbfen_get(void)
{
    return NRF_QDEC->DBFEN;
}

__STATIC_INLINE void nrf_qdec_pio_assign( uint32_t psela, uint32_t pselb, uint32_t pselled)
{
#if defined(QDEC_PSEL_A_CONNECT_Pos)
    NRF_QDEC->PSEL.A = psela;
#else
    NRF_QDEC->PSELA = psela;
#endif

#if defined(QDEC_PSEL_B_CONNECT_Pos)
    NRF_QDEC->PSEL.B = pselb;
#else
    NRF_QDEC->PSELB = pselb;
#endif

#if defined(QDEC_PSEL_LED_CONNECT_Pos)
    NRF_QDEC->PSEL.LED = pselled;
#else
    NRF_QDEC->PSELLED = pselled;
#endif
}

__STATIC_INLINE void nrf_qdec_task_trigger(nrf_qdec_task_t task)
{
    *( (volatile uint32_t *)( (uint8_t *)NRF_QDEC + (uint32_t)task) ) = 1;
}

__STATIC_INLINE uint32_t * nrf_qdec_task_address_get(nrf_qdec_task_t task)
{
    return (uint32_t *)( (uint8_t *)NRF_QDEC + (uint32_t)task);
}

__STATIC_INLINE void nrf_qdec_event_clear(nrf_qdec_event_t event)
{
    *( (volatile uint32_t *)( (uint8_t *)NRF_QDEC + (uint32_t)event) ) = 0;
#if __CORTEX_M == 0x04
    volatile uint32_t dummy = *((volatile uint32_t *)((uint8_t *)NRF_QDEC + event));
    (void)dummy;
#endif
}

__STATIC_INLINE uint32_t nrf_qdec_event_check(nrf_qdec_event_t event)
{
    return *(volatile uint32_t *)( (uint8_t *)NRF_QDEC + (uint32_t)event);
}

__STATIC_INLINE uint32_t * nrf_qdec_event_address_get(nrf_qdec_event_t event)
{
    return (uint32_t *)( (uint8_t *)NRF_QDEC + (uint32_t)event);
}

__STATIC_INLINE void nrf_qdec_shorts_enable(uint32_t mask)
{
    NRF_QDEC->SHORTS |= mask;
}

__STATIC_INLINE void nrf_qdec_shorts_disable(uint32_t mask)
{
    NRF_QDEC->SHORTS &= ~mask;
}

__STATIC_INLINE int32_t nrf_qdec_sampleper_reg_get(void)
{
    return NRF_QDEC->SAMPLEPER;
}

__STATIC_INLINE uint32_t nrf_qdec_sampleper_to_value(uint32_t sampleper)
{
    return (1 << (7 + sampleper));
}

__STATIC_INLINE void nrf_qdec_sampleper_set(nrf_qdec_sampleper_t sample_per)
{
    NRF_QDEC->SAMPLEPER = sample_per;
}

__STATIC_INLINE int32_t nrf_qdec_sample_get(void)
{
    return NRF_QDEC->SAMPLE;
}

__STATIC_INLINE int32_t nrf_qdec_acc_get(void)
{
    return NRF_QDEC->ACC;
}

__STATIC_INLINE int32_t nrf_qdec_accread_get(void)
{
    return NRF_QDEC->ACCREAD;
}

__STATIC_INLINE uint32_t nrf_qdec_accdbl_get(void)
{
    return NRF_QDEC->ACCDBL;
}

__STATIC_INLINE uint32_t nrf_qdec_accdblread_get(void)
{
    return NRF_QDEC->ACCDBLREAD;
}

__STATIC_INLINE void nrf_qdec_ledpre_set(uint32_t time_us)
{
    NRF_QDEC->LEDPRE = time_us;
}

__STATIC_INLINE uint32_t nrf_qdec_ledpre_get(void)
{
    return NRF_QDEC->LEDPRE;
}

__STATIC_INLINE void nrf_qdec_reportper_set(nrf_qdec_reportper_t reportper)
{
    NRF_QDEC->REPORTPER = reportper;
}

__STATIC_INLINE uint32_t nrf_qdec_reportper_reg_get(void)
{
    return NRF_QDEC->REPORTPER;
}

__STATIC_INLINE uint32_t nrf_qdec_reportper_to_value(uint32_t reportper)
{
    return (reportper == NRF_QDEC_REPORTPER_10) ? 10 : reportper * 40;
}

__STATIC_INLINE void nrf_qdec_ledpol_set(nrf_qdec_ledpol_t pol)
{
    NRF_QDEC->LEDPOL = pol;
}

__STATIC_INLINE uint32_t nrf_qdec_ledpol_get(void)
{
    return NRF_QDEC->LEDPOL;
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_QDEC_H__
