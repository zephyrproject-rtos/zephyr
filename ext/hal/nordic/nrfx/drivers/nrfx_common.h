/*
 * Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
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

#ifndef NRFX_COMMON_H__
#define NRFX_COMMON_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <nrf.h>
#include <nrf_peripherals.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_common Common module
 * @{
 * @ingroup nrfx
 * @brief Common module.
 */

/**
 * @brief Macro for checking if the specified identifier is defined and it has
 *        a non-zero value.
 *
 * Normally, preprocessors treat all undefined identifiers as having the value
 * zero. However, some tools, like static code analyzers, may issue a warning
 * when such identifier is evaluated. This macro gives the possibility to suppress
 * such warnings only in places where this macro is used for evaluation, not in
 * the whole analyzed code.
 */
#define NRFX_CHECK(module_enabled)  (module_enabled)

/**
 * @brief Macro for concatenating two tokens in macro expansion.
 *
 * @note This macro is expanded in two steps so that tokens given as macros
 *       themselves are fully expanded before they are merged.
 *
 * @param p1  First token.
 * @param p2  Second token.
 *
 * @return The two tokens merged into one, unless they cannot together form
 *         a valid token (in such case, the preprocessor issues a warning and
 *         does not perform the concatenation).
 *
 * @sa NRFX_CONCAT_3
 */
#define NRFX_CONCAT_2(p1, p2)       NRFX_CONCAT_2_(p1, p2)
/**
 * @brief Internal macro used by @ref NRFX_CONCAT_2 to perform the expansion
 *        in two steps.
 */
#define NRFX_CONCAT_2_(p1, p2)      p1 ## p2

/**
 * @brief Macro for concatenating three tokens in macro expansion.
 *
 * @note This macro is expanded in two steps so that tokens given as macros
 *       themselves are fully expanded before they are merged.
 *
 * @param p1  First token.
 * @param p2  Second token.
 * @param p3  Third token.
 *
 * @return The three tokens merged into one, unless they cannot together form
 *         a valid token (in such case, the preprocessor issues a warning and
 *         does not perform the concatenation).
 *
 * @sa NRFX_CONCAT_2
 */
#define NRFX_CONCAT_3(p1, p2, p3)   NRFX_CONCAT_3_(p1, p2, p3)
/**
 * @brief Internal macro used by @ref NRFX_CONCAT_3 to perform the expansion
 *        in two steps.
 */
#define NRFX_CONCAT_3_(p1, p2, p3)  p1 ## p2 ## p3

/**@brief Macro for performing rounded integer division (as opposed to
 *        truncating the result).
 *
 * @param a  Numerator.
 * @param b  Denominator.
 *
 * @return Rounded (integer) result of dividing @c a by @c b.
 */
#define NRFX_ROUNDED_DIV(a, b)  (((a) + ((b) / 2)) / (b))

/**@brief Macro for checking if given lengths of EasyDMA transfers do not exceed
 *        the limit of the specified peripheral.
 *
 * @param peripheral Peripheral to check the lengths against.
 * @param length1    First length to be checked.
 * @param length2    Second length to be checked (pass 0 if not needed).
 *
 * @return
 */
#define NRFX_EASYDMA_LENGTH_VALIDATE(peripheral, length1, length2)            \
    (((length1) < (1U << NRFX_CONCAT_2(peripheral, _EASYDMA_MAXCNT_SIZE))) && \
     ((length2) < (1U << NRFX_CONCAT_2(peripheral, _EASYDMA_MAXCNT_SIZE))))

/**@brief Macro for waiting until condition is met.
 *
 * @param[in]  condition Condition to meet.
 * @param[in]  attempts  Maximum number of condition checks. Must not be 0.
 * @param[in]  delay_us  Delay between consecutive checks, in microseconds.
 * @param[out] result    Boolean variable to store the result of the wait process.
 *                       Set to true if the condition is met or false otherwise.
 */
#define NRFX_WAIT_FOR(condition, attempts, delay_us, result) \
do {                                                         \
    result =  false;                                         \
    uint32_t remaining_attempts = (attempts);                \
    do {                                                     \
           if (condition)                                    \
           {                                                 \
               result =  true;                               \
               break;                                        \
           }                                                 \
           NRFX_DELAY_US(delay_us);                          \
    } while (--remaining_attempts);                          \
} while(0)

/**
 * @brief Macro for getting the interrupt number assigned to a specific
 *        peripheral.
 *
 * In Nordic SoCs the IRQ number assigned to a peripheral is equal to the ID
 * of this peripheral, and there is a direct relationship between this ID and
 * the peripheral base address, i.e. the address of a fixed block of 0x1000
 * bytes of address space assigned to this peripheral.
 * See the chapter "Peripheral interface" (sections "Peripheral ID" and
 * "Interrupts") in the product specification of a given SoC.
 *
 * @param[in] base_addr  Peripheral base address or pointer.
 *
 * @return Interrupt number associated with the specified peripheral.
 */
#define NRFX_IRQ_NUMBER_GET(base_addr)  (uint8_t)((uint32_t)(base_addr) >> 12)

/**
 * @brief IRQ handler type.
 */
typedef void (* nrfx_irq_handler_t)(void);

/**
 * @brief Driver state.
 */
typedef enum
{
    NRFX_DRV_STATE_UNINITIALIZED, ///< Uninitialized.
    NRFX_DRV_STATE_INITIALIZED,   ///< Initialized but powered off.
    NRFX_DRV_STATE_POWERED_ON,    ///< Initialized and powered on.
} nrfx_drv_state_t;


/**
 * @brief Function for checking if an object is placed in the Data RAM region.
 *
 * Several peripherals (the ones using EasyDMA) require the transfer buffers
 * to be placed in the Data RAM region. This function can be used to check if
 * this condition is met.
 *
 * @param[in] p_object  Pointer to an object whose location is to be checked.
 *
 * @retval true  If the pointed object is located in the Data RAM region.
 * @retval false Otherwise.
 */
__STATIC_INLINE bool nrfx_is_in_ram(void const * p_object);

/**
 * @brief Function for getting the interrupt number for a specific peripheral.
 *
 * @param[in] p_reg  Peripheral base pointer.
 *
 * @return Interrupt number associated with the pointed peripheral.
 */
__STATIC_INLINE IRQn_Type nrfx_get_irq_number(void const * p_reg);

/**
 * @brief Function for converting an INTEN register bit position to the
 *        corresponding event identifier.
 *
 * The event identifier is the offset between the event register address and
 * the peripheral base address, and is equal (thus, can be directly cast) to
 * the corresponding value of the enumerated type from HAL (nrf_*_event_t).

 * @param bit  INTEN register bit position.
 *
 * @return Event identifier.
 *
 * @sa nrfx_event_to_bitpos
 */
__STATIC_INLINE uint32_t nrfx_bitpos_to_event(uint32_t bit);

/**
 * @brief Function for converting an event identifier to the corresponding
 *        INTEN register bit position.
 *
 * The event identifier is the offset between the event register address and
 * the peripheral base address, and is equal (thus, can be directly cast) to
 * the corresponding value of the enumerated type from HAL (nrf_*_event_t).
 *
 * @param event  Event identifier.
 *
 * @return INTEN register bit position.
 *
 * @sa nrfx_bitpos_to_event
 */
__STATIC_INLINE uint32_t nrfx_event_to_bitpos(uint32_t event);


#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE bool nrfx_is_in_ram(void const * p_object)
{
    return ((((uint32_t)p_object) & 0xE0000000u) == 0x20000000u);
}

__STATIC_INLINE IRQn_Type nrfx_get_irq_number(void const * p_reg)
{
    return (IRQn_Type)NRFX_IRQ_NUMBER_GET(p_reg);
}

__STATIC_INLINE uint32_t nrfx_bitpos_to_event(uint32_t bit)
{
    static const uint32_t event_reg_offset = 0x100u;
    return event_reg_offset + (bit * sizeof(uint32_t));
}

__STATIC_INLINE uint32_t nrfx_event_to_bitpos(uint32_t event)
{
    static const uint32_t event_reg_offset = 0x100u;
    return (event - event_reg_offset) / sizeof(uint32_t);
}

#endif

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRFX_COMMON_H__
