/*
 * Copyright (c) 2015 - 2019, Nordic Semiconductor ASA
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
#ifndef NRF_PDM_H_
#define NRF_PDM_H_

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_pdm_hal PDM HAL
 * @{
 * @ingroup nrf_pdm
 * @brief   Hardware access layer for managing the Pulse Density Modulation (PDM) peripheral.
 */

/** @brief Minimum value of PDM gain. */
#define NRF_PDM_GAIN_MINIMUM  0x00
/** @brief Default value of PDM gain. */
#define NRF_PDM_GAIN_DEFAULT  0x28
/** @brief Maximum value of PDM gain. */
#define NRF_PDM_GAIN_MAXIMUM  0x50


/** @brief PDM gain type. */
typedef uint8_t nrf_pdm_gain_t;

/** @brief PDM tasks. */
typedef enum
{
    NRF_PDM_TASK_START = offsetof(NRF_PDM_Type, TASKS_START), ///< Starts continuous PDM transfer.
    NRF_PDM_TASK_STOP  = offsetof(NRF_PDM_Type, TASKS_STOP)   ///< Stops PDM transfer.
} nrf_pdm_task_t;

/** @brief PDM events. */
typedef enum
{
    NRF_PDM_EVENT_STARTED = offsetof(NRF_PDM_Type, EVENTS_STARTED), ///< PDM transfer is started.
    NRF_PDM_EVENT_STOPPED = offsetof(NRF_PDM_Type, EVENTS_STOPPED), ///< PDM transfer is finished.
    NRF_PDM_EVENT_END     = offsetof(NRF_PDM_Type, EVENTS_END)      ///< The PDM has written the last sample specified by SAMPLE.MAXCNT (or the last sample after a STOP task has been received) to Data RAM.
} nrf_pdm_event_t;

/** @brief PDM interrupt masks. */
typedef enum
{
    NRF_PDM_INT_STARTED = PDM_INTENSET_STARTED_Msk, ///< Interrupt on EVENTS_STARTED event.
    NRF_PDM_INT_STOPPED = PDM_INTENSET_STOPPED_Msk, ///< Interrupt on EVENTS_STOPPED event.
    NRF_PDM_INT_END     = PDM_INTENSET_END_Msk      ///< Interrupt on EVENTS_END event.
} nrf_pdm_int_mask_t;

/** @brief PDM clock frequency. */
typedef enum
{
    NRF_PDM_FREQ_1000K = PDM_PDMCLKCTRL_FREQ_1000K,   ///< PDM_CLK = 1.000 MHz.
    NRF_PDM_FREQ_1032K = PDM_PDMCLKCTRL_FREQ_Default, ///< PDM_CLK = 1.032 MHz.
    NRF_PDM_FREQ_1067K = PDM_PDMCLKCTRL_FREQ_1067K    ///< PDM_CLK = 1.067 MHz.
} nrf_pdm_freq_t;

/** @brief PDM operation mode. */
typedef enum
{
    NRF_PDM_MODE_STEREO = PDM_MODE_OPERATION_Stereo,  ///< Sample and store one pair (Left + Right) of 16-bit samples per RAM word.
    NRF_PDM_MODE_MONO   = PDM_MODE_OPERATION_Mono     ///< Sample and store two successive Left samples (16 bit each) per RAM word.
} nrf_pdm_mode_t;

/** @brief PDM sampling mode. */
typedef enum
{
    NRF_PDM_EDGE_LEFTFALLING = PDM_MODE_EDGE_LeftFalling,  ///< Left (or mono) is sampled on falling edge of PDM_CLK.
    NRF_PDM_EDGE_LEFTRISING  = PDM_MODE_EDGE_LeftRising    ///< Left (or mono) is sampled on rising edge of PDM_CLK.
} nrf_pdm_edge_t;


/**
 * @brief Function for triggering a PDM task.
 *
 * @param[in] task PDM task.
 */
__STATIC_INLINE void nrf_pdm_task_trigger(nrf_pdm_task_t task);

/**
 * @brief Function for getting the address of a PDM task register.
 *
 * @param[in] task PDM task.
 *
 * @return Address of the specified PDM task.
 */
__STATIC_INLINE uint32_t nrf_pdm_task_address_get(nrf_pdm_task_t task);

/**
 * @brief Function for retrieving the state of the PDM event.
 *
 * @param[in] event Event to be checked.
 *
 * @retval true  The event has been generated.
 * @retval false The event has not been generated.
 */
__STATIC_INLINE bool nrf_pdm_event_check(nrf_pdm_event_t event);

/**
 * @brief Function for clearing a PDM event.
 *
 * @param[in] event PDM event.
 */
__STATIC_INLINE void nrf_pdm_event_clear(nrf_pdm_event_t event);

/**
 * @brief Function for getting the address of a PDM event register.
 *
 * @param[in] event PDM event.
 *
 * @return Address of the specified PDM event.
 */
__STATIC_INLINE volatile uint32_t * nrf_pdm_event_address_get(nrf_pdm_event_t event);

/**
 * @brief Function for enabling PDM interrupts.
 *
 * @param[in] int_mask Mask of interrupts to be enabled.
 */
__STATIC_INLINE void nrf_pdm_int_enable(uint32_t int_mask);

/**
 * @brief Function for retrieving the state of PDM interrupts.
 *
 * @param[in] int_mask Mask of interrupts to be checked.
 *
 * @retval true  All specified interrupts are enabled.
 * @retval false At least one of the given interrupts is not enabled.
 */
__STATIC_INLINE bool nrf_pdm_int_enable_check(uint32_t int_mask);

/**
 * @brief Function for disabling interrupts.
 *
 * @param[in] int_mask Mask of interrupts to be disabled.
 */
__STATIC_INLINE void nrf_pdm_int_disable(uint32_t int_mask);

#if defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for setting the subscribe configuration for a given
 *        PDM task.
 *
 * @param[in] task    Task for which to set the configuration.
 * @param[in] channel Channel through which to subscribe events.
 */
__STATIC_INLINE void nrf_pdm_subscribe_set(nrf_pdm_task_t task,
                                           uint8_t        channel);

/**
 * @brief Function for clearing the subscribe configuration for a given
 *        PDM task.
 *
 * @param[in] task Task for which to clear the configuration.
 */
__STATIC_INLINE void nrf_pdm_subscribe_clear(nrf_pdm_task_t task);

/**
 * @brief Function for setting the publish configuration for a given
 *        PDM event.
 *
 * @param[in] event   Event for which to set the configuration.
 * @param[in] channel Channel through which to publish the event.
 */
__STATIC_INLINE void nrf_pdm_publish_set(nrf_pdm_event_t event,
                                         uint8_t         channel);

/**
 * @brief Function for clearing the publish configuration for a given
 *        PDM event.
 *
 * @param[in] event Event for which to clear the configuration.
 */
__STATIC_INLINE void nrf_pdm_publish_clear(nrf_pdm_event_t event);
#endif // defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for enabling the PDM peripheral.
 *
 * The PDM peripheral must be enabled before use.
 */
__STATIC_INLINE void nrf_pdm_enable(void);

/** @brief Function for disabling the PDM peripheral. */
__STATIC_INLINE void nrf_pdm_disable(void);

/**
 * @brief Function for checking if the PDM peripheral is enabled.
 *
 * @retval true  The PDM peripheral is enabled.
 * @retval false The PDM peripheral is not enabled.
 */
__STATIC_INLINE bool nrf_pdm_enable_check(void);

/**
 * @brief Function for setting the PDM operation mode.
 *
 * @param[in] pdm_mode PDM operation mode.
 * @param[in] pdm_edge PDM sampling mode.
 */
__STATIC_INLINE void nrf_pdm_mode_set(nrf_pdm_mode_t pdm_mode, nrf_pdm_edge_t pdm_edge);

/**
 * @brief Function for getting the PDM operation mode.
 *
 * @param[out] p_pdm_mode PDM operation mode.
 * @param[out] p_pdm_edge PDM sampling mode.
 */
__STATIC_INLINE void nrf_pdm_mode_get(nrf_pdm_mode_t * p_pdm_mode, nrf_pdm_edge_t * p_pdm_edge);

/**
 * @brief Function for setting the PDM clock frequency.
 *
 * @param[in] pdm_freq PDM clock frequency.
 */
__STATIC_INLINE void nrf_pdm_clock_set(nrf_pdm_freq_t pdm_freq);

/**
 * @brief Function for getting the PDM clock frequency.
 *
 * @return PDM clock frequency.
 */
__STATIC_INLINE nrf_pdm_freq_t nrf_pdm_clock_get(void);

/**
 * @brief Function for setting up the PDM pins.
 *
 * @param[in] psel_clk CLK pin number.
 * @param[in] psel_din DIN pin number.
 */
__STATIC_INLINE void nrf_pdm_psel_connect(uint32_t psel_clk, uint32_t psel_din);

/** @brief Function for disconnecting the PDM pins. */
__STATIC_INLINE void nrf_pdm_psel_disconnect(void);

/**
 * @brief Function for setting the PDM gain.
 *
 * @param[in] gain_l Left channel gain.
 * @param[in] gain_r Right channel gain.
 */
__STATIC_INLINE void nrf_pdm_gain_set(nrf_pdm_gain_t gain_l, nrf_pdm_gain_t gain_r);

/**
 * @brief Function for getting the PDM gain.
 *
 * @param[out] p_gain_l Left channel gain.
 * @param[out] p_gain_r Right channel gain.
 */
__STATIC_INLINE void nrf_pdm_gain_get(nrf_pdm_gain_t * p_gain_l, nrf_pdm_gain_t * p_gain_r);

/**
 * @brief Function for setting the PDM sample buffer.
 *
 * The amount of allocated RAM depends on the operation mode.
 * - For stereo mode: N 32-bit words.
 * - For mono mode: Ceil(N/2) 32-bit words.
 *
 * @param[in] p_buffer Pointer to the RAM address where samples are to be written with EasyDMA.
 * @param[in] num      Number of samples to allocate memory for in EasyDMA mode.
 */
__STATIC_INLINE void nrf_pdm_buffer_set(uint32_t * p_buffer, uint32_t num);

/**
 * @brief Function for getting the current PDM sample buffer address.
 *
 * @return Pointer to the current sample buffer.
 */
__STATIC_INLINE uint32_t * nrf_pdm_buffer_get(void);

#ifndef SUPPRESS_INLINE_IMPLEMENTATION
__STATIC_INLINE void nrf_pdm_task_trigger(nrf_pdm_task_t task)
{
    *((volatile uint32_t *)((uint8_t *)NRF_PDM + (uint32_t)task)) = 0x1UL;
}

__STATIC_INLINE uint32_t nrf_pdm_task_address_get(nrf_pdm_task_t task)
{
    return (uint32_t)((uint8_t *)NRF_PDM + (uint32_t)task);
}

__STATIC_INLINE bool nrf_pdm_event_check(nrf_pdm_event_t event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)NRF_PDM + (uint32_t)event);
}

__STATIC_INLINE void nrf_pdm_event_clear(nrf_pdm_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)NRF_PDM + (uint32_t)event)) = 0x0UL;
#if __CORTEX_M == 0x04
    volatile uint32_t dummy = *((volatile uint32_t *)((uint8_t *)NRF_PDM + (uint32_t)event));
    (void)dummy;
#endif
}

__STATIC_INLINE volatile uint32_t * nrf_pdm_event_address_get(nrf_pdm_event_t event)
{
    return (volatile uint32_t *)((uint8_t *)NRF_PDM + (uint32_t)event);
}

__STATIC_INLINE void nrf_pdm_int_enable(uint32_t int_mask)
{
    NRF_PDM->INTENSET = int_mask;
}

__STATIC_INLINE bool nrf_pdm_int_enable_check(uint32_t int_mask)
{
    return (bool)(NRF_PDM->INTENSET & int_mask);
}

__STATIC_INLINE void nrf_pdm_int_disable(uint32_t int_mask)
{
    NRF_PDM->INTENCLR = int_mask;
}

#if defined(DPPI_PRESENT)
__STATIC_INLINE void nrf_pdm_subscribe_set(nrf_pdm_task_t task,
                                           uint8_t        channel)
{
    *((volatile uint32_t *) ((uint8_t *) NRF_PDM + (uint32_t) task + 0x80uL)) =
            ((uint32_t)channel | PDM_SUBSCRIBE_START_EN_Msk);
}

__STATIC_INLINE void nrf_pdm_subscribe_clear(nrf_pdm_task_t task)
{
    *((volatile uint32_t *) ((uint8_t *) NRF_PDM + (uint32_t) task + 0x80uL)) = 0;
}

__STATIC_INLINE void nrf_pdm_publish_set(nrf_pdm_event_t event,
                                         uint8_t         channel)
{
    *((volatile uint32_t *) ((uint8_t *) NRF_PDM + (uint32_t) event + 0x80uL)) =
            ((uint32_t)channel | PDM_PUBLISH_STARTED_EN_Msk);
}

__STATIC_INLINE void nrf_pdm_publish_clear(nrf_pdm_event_t event)
{
    *((volatile uint32_t *) ((uint8_t *) NRF_PDM + (uint32_t) event + 0x80uL)) = 0;
}
#endif // defined(DPPI_PRESENT)

__STATIC_INLINE void nrf_pdm_enable(void)
{
    NRF_PDM->ENABLE = (PDM_ENABLE_ENABLE_Enabled << PDM_ENABLE_ENABLE_Pos);
}

__STATIC_INLINE void nrf_pdm_disable(void)
{
    NRF_PDM->ENABLE = (PDM_ENABLE_ENABLE_Disabled << PDM_ENABLE_ENABLE_Pos);
}

__STATIC_INLINE bool nrf_pdm_enable_check(void)
{
    return (NRF_PDM->ENABLE == (PDM_ENABLE_ENABLE_Enabled << PDM_ENABLE_ENABLE_Pos));
}

__STATIC_INLINE void nrf_pdm_mode_set(nrf_pdm_mode_t pdm_mode, nrf_pdm_edge_t pdm_edge)
{
    NRF_PDM->MODE = ((pdm_mode << PDM_MODE_OPERATION_Pos) & PDM_MODE_OPERATION_Msk)
                    | ((pdm_edge << PDM_MODE_EDGE_Pos) & PDM_MODE_EDGE_Msk);
}

__STATIC_INLINE void nrf_pdm_mode_get(nrf_pdm_mode_t * p_pdm_mode, nrf_pdm_edge_t * p_pdm_edge)
{
    uint32_t mode = NRF_PDM->MODE;
    *p_pdm_mode = (nrf_pdm_mode_t)((mode & PDM_MODE_OPERATION_Msk ) >> PDM_MODE_OPERATION_Pos);
    *p_pdm_edge = (nrf_pdm_edge_t)((mode & PDM_MODE_EDGE_Msk ) >> PDM_MODE_EDGE_Pos);
}

__STATIC_INLINE void nrf_pdm_clock_set(nrf_pdm_freq_t pdm_freq)
{
    NRF_PDM->PDMCLKCTRL = ((pdm_freq << PDM_PDMCLKCTRL_FREQ_Pos) & PDM_PDMCLKCTRL_FREQ_Msk);
}

__STATIC_INLINE nrf_pdm_freq_t nrf_pdm_clock_get(void)
{
     return (nrf_pdm_freq_t) ((NRF_PDM->PDMCLKCTRL << PDM_PDMCLKCTRL_FREQ_Pos) & PDM_PDMCLKCTRL_FREQ_Msk);
}

__STATIC_INLINE void nrf_pdm_psel_connect(uint32_t psel_clk, uint32_t psel_din)
{
    NRF_PDM->PSEL.CLK = psel_clk;
    NRF_PDM->PSEL.DIN = psel_din;
}

__STATIC_INLINE void nrf_pdm_psel_disconnect(void)
{
    NRF_PDM->PSEL.CLK = ((PDM_PSEL_CLK_CONNECT_Disconnected << PDM_PSEL_CLK_CONNECT_Pos)
                         & PDM_PSEL_CLK_CONNECT_Msk);
    NRF_PDM->PSEL.DIN = ((PDM_PSEL_DIN_CONNECT_Disconnected << PDM_PSEL_DIN_CONNECT_Pos)
                         & PDM_PSEL_DIN_CONNECT_Msk);
}

__STATIC_INLINE void nrf_pdm_gain_set(nrf_pdm_gain_t gain_l, nrf_pdm_gain_t gain_r)
{
    NRF_PDM->GAINL = gain_l;
    NRF_PDM->GAINR = gain_r;
}

__STATIC_INLINE void nrf_pdm_gain_get(nrf_pdm_gain_t * p_gain_l, nrf_pdm_gain_t * p_gain_r)
{
    *p_gain_l = NRF_PDM->GAINL;
    *p_gain_r = NRF_PDM->GAINR;
}

__STATIC_INLINE void nrf_pdm_buffer_set(uint32_t * p_buffer, uint32_t num)
{
    NRF_PDM->SAMPLE.PTR = (uint32_t)p_buffer;
    NRF_PDM->SAMPLE.MAXCNT = num;
}

__STATIC_INLINE uint32_t * nrf_pdm_buffer_get(void)
{
    return (uint32_t *)NRF_PDM->SAMPLE.PTR;
}

#endif // SUPPRESS_INLINE_IMPLEMENTATION
/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_PDM_H_
