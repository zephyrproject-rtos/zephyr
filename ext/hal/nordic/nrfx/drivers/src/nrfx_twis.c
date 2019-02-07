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

#include <nrfx.h>

#if NRFX_CHECK(NRFX_TWIS_ENABLED)

#if !(NRFX_CHECK(NRFX_TWIS0_ENABLED) || \
      NRFX_CHECK(NRFX_TWIS1_ENABLED) || \
      NRFX_CHECK(NRFX_TWIS2_ENABLED) || \
      NRFX_CHECK(NRFX_TWIS3_ENABLED))
#error "No enabled TWIS instances. Check <nrfx_config.h>."
#endif

#include <nrfx_twis.h>
#include "prs/nrfx_prs.h"

#define NRFX_LOG_MODULE TWIS
#include <nrfx_log.h>

#define EVT_TO_STR(event)                                             \
    (event == NRF_TWIS_EVENT_STOPPED   ? "NRF_TWIS_EVENT_STOPPED"   : \
    (event == NRF_TWIS_EVENT_ERROR     ? "NRF_TWIS_EVENT_ERROR"     : \
    (event == NRF_TWIS_EVENT_RXSTARTED ? "NRF_TWIS_EVENT_RXSTARTED" : \
    (event == NRF_TWIS_EVENT_TXSTARTED ? "NRF_TWIS_EVENT_TXSTARTED" : \
    (event == NRF_TWIS_EVENT_WRITE     ? "NRF_TWIS_EVENT_WRITE"     : \
    (event == NRF_TWIS_EVENT_READ      ? "NRF_TWIS_EVENT_READ"      : \
                                         "UNKNOWN EVENT"))))))


/**
 * @brief Actual state of internal state machine
 *
 * Current substate of powered on state.
 */
typedef enum
{
    NRFX_TWIS_SUBSTATE_IDLE,          ///< No ongoing transmission
    NRFX_TWIS_SUBSTATE_READ_WAITING,  ///< Read request received, waiting for data
    NRFX_TWIS_SUBSTATE_READ_PENDING,  ///< Reading is actually pending (data sending)
    NRFX_TWIS_SUBSTATE_WRITE_WAITING, ///< Write request received, waiting for data buffer
    NRFX_TWIS_SUBSTATE_WRITE_PENDING, ///< Writing is actually pending (data receiving)
} nrfx_twis_substate_t;

// Control block - driver instance local data.
typedef struct
{
    nrfx_twis_event_handler_t       ev_handler;
    // Internal copy of hardware errors flags merged with specific internal
    // driver errors flags.
    // This value can be changed in the interrupt and cleared in the main program.
    // Always use Atomic load-store when updating this value in main loop.
    volatile uint32_t               error;
    nrfx_drv_state_t                state;
    volatile nrfx_twis_substate_t   substate;

    volatile bool                   semaphore;
} twis_control_block_t;
static twis_control_block_t m_cb[NRFX_TWIS_ENABLED_COUNT];

/**
 * @brief Used interrupts mask
 *
 * Mask for all interrupts used by this library
 */
static const uint32_t m_used_ints_mask = NRF_TWIS_INT_STOPPED_MASK   |
                                         NRF_TWIS_INT_ERROR_MASK     |
                                         NRF_TWIS_INT_RXSTARTED_MASK |
                                         NRF_TWIS_INT_TXSTARTED_MASK |
                                         NRF_TWIS_INT_WRITE_MASK     |
                                         NRF_TWIS_INT_READ_MASK;

/**
 * @brief Clear all  events
 *
 * Function clears all actually pending events
 */
static void nrfx_twis_clear_all_events(NRF_TWIS_Type * const p_reg)
{
    /* Clear all events */
    nrf_twis_event_clear(p_reg, NRF_TWIS_EVENT_STOPPED);
    nrf_twis_event_clear(p_reg, NRF_TWIS_EVENT_ERROR);
    nrf_twis_event_clear(p_reg, NRF_TWIS_EVENT_RXSTARTED);
    nrf_twis_event_clear(p_reg, NRF_TWIS_EVENT_TXSTARTED);
    nrf_twis_event_clear(p_reg, NRF_TWIS_EVENT_WRITE);
    nrf_twis_event_clear(p_reg, NRF_TWIS_EVENT_READ);
}

/**
 * @brief Reset all the registers to known state
 *
 * This function clears all registers that requires it to known state.
 * TWIS is left disabled after this function.
 * All events are cleared.
 * @param[out] p_reg TWIS to reset register address
 */
static inline void nrfx_twis_swreset(NRF_TWIS_Type * p_reg)
{
    /* Disable TWIS */
    nrf_twis_disable(p_reg);

    /* Disconnect pins */
    nrf_twis_pins_set(p_reg, ~0U, ~0U);

    /* Disable interrupt global for the instance */
    NRFX_IRQ_DISABLE(nrfx_get_irq_number(p_reg));

    /* Disable interrupts */
    nrf_twis_int_disable(p_reg, ~0U);
}

/**
 * @brief Configure pin
 *
 * Function configures selected for work as SDA or SCL.
 * @param pin Pin number to configure
 */
static inline void nrfx_twis_config_pin(uint32_t pin, nrf_gpio_pin_pull_t pull)
{
    nrf_gpio_cfg(pin,
                 NRF_GPIO_PIN_DIR_INPUT,
                 NRF_GPIO_PIN_INPUT_DISCONNECT,
                 pull,
                 NRF_GPIO_PIN_S0D1,
                 NRF_GPIO_PIN_NOSENSE);
}

/**
 * @brief Auxiliary function for getting event state on right bit possition
 *
 * This function calls @ref nrf_twis_event_get function but the the result
 * is shifted to match INTEN register scheme.
 *
 * @param[in,out] p_reg TWIS to read  event from
 * @param ev  Event code
 *
 * @return Selected event state shifted by @ref nrfx_event_to_bitpos
 *
 * @sa nrf_twis_event_get
 * @sa nrfx_event_to_bitpos
 */
static inline uint32_t nrfx_twis_event_bit_get(NRF_TWIS_Type *  p_reg,
                                               nrf_twis_event_t ev)
{
    return (uint32_t)nrf_twis_event_get_and_clear(p_reg, ev) << nrfx_event_to_bitpos(ev);
}

/**
 * @brief Auxiliary function for checking event bit inside given flags value
 *
 * Function used here to check presence of the event inside given flags value.
 * It transforms given event to bit possition and then checks if in given variable it is cleared.
 *
 * @param flags Flags to test
 * @param ev Event code
 *
 * @retval true Flag for selected event is set
 * @retval false Flag for selected event is cleared
 */
static inline bool nrfx_twis_check_bit(uint32_t         flags,
                                       nrf_twis_event_t ev)
{
    return 0 != (flags & (1U << nrfx_event_to_bitpos(ev)));
}

/**
 * @brief Auxiliary function for clearing event bit in given flags value
 *
 * Function used to clear selected event bit.
 *
 * @param flags Flags to process
 * @param ev    Event code to clear
 *
 * @return Value @em flags with cleared event bit that matches given @em ev
 */
static inline uint32_t nrfx_twis_clear_bit(uint32_t         flags,
                                           nrf_twis_event_t ev)
{
    return flags & ~(1U << nrfx_event_to_bitpos(ev));
}

static void call_event_handler(twis_control_block_t const * p_cb,
                               nrfx_twis_evt_t const *      p_evt)
{
    nrfx_twis_event_handler_t handler = p_cb->ev_handler;
    if (handler != NULL)
    {
        handler(p_evt);
    }
}

/**
 * @brief Auxiliary function for error processing
 *
 * Function called when in current substate the event apears and it cannot be processed.
 * It should be called also on ERROR event.
 * If given @em error parameter has zero value the @ref NRFX_TWIS_ERROR_UNEXPECTED_EVENT
 * would be set.
 *
 * @param p_cb   Pointer to the driver instance control block.
 * @param evt    What error event raport to event handler
 * @param error  Error flags
 */
static inline void nrfx_twis_process_error(twis_control_block_t * p_cb,
                                           nrfx_twis_evt_type_t   evt,
                                           uint32_t               error)
{
    if (0 == error)
    {
        error = NRFX_TWIS_ERROR_UNEXPECTED_EVENT;
    }
    nrfx_twis_evt_t evdata;
    evdata.type       = evt;
    evdata.data.error = error;

    p_cb->error |= error;

    call_event_handler(p_cb, &evdata);
}

static void nrfx_twis_state_machine(NRF_TWIS_Type *        p_reg,
                                    twis_control_block_t * p_cb)
{
    if (!NRFX_TWIS_NO_SYNC_MODE)
    {
        /* Exclude parallel processing of this function */
        if (p_cb->semaphore)
        {
            return;
        }
        p_cb->semaphore = 1;
    }

    /* Event data structure to be passed into event handler */
    nrfx_twis_evt_t evdata;
    /* Current substate copy  */
    nrfx_twis_substate_t substate = p_cb->substate;
    /* Event flags */
    uint32_t ev = 0;

    /* Get all events */
    ev |= nrfx_twis_event_bit_get(p_reg, NRF_TWIS_EVENT_STOPPED);
    ev |= nrfx_twis_event_bit_get(p_reg, NRF_TWIS_EVENT_ERROR);
    ev |= nrfx_twis_event_bit_get(p_reg, NRF_TWIS_EVENT_RXSTARTED);
    ev |= nrfx_twis_event_bit_get(p_reg, NRF_TWIS_EVENT_TXSTARTED);
    ev |= nrfx_twis_event_bit_get(p_reg, NRF_TWIS_EVENT_WRITE);
    ev |= nrfx_twis_event_bit_get(p_reg, NRF_TWIS_EVENT_READ);

    /* State machine */
    while (0 != ev)
    {
        switch (substate)
        {
        case NRFX_TWIS_SUBSTATE_IDLE:
            if (nrfx_twis_check_bit(ev, NRF_TWIS_EVENT_STOPPED))
            {
                /* Stopped event is always allowed in IDLE state - just ignore */
                ev = nrfx_twis_clear_bit(ev, NRF_TWIS_EVENT_STOPPED);
            }
            else if (nrfx_twis_check_bit(ev, NRF_TWIS_EVENT_READ))
            {
                evdata.type = NRFX_TWIS_EVT_READ_REQ;
                if (nrfx_twis_check_bit(ev, NRF_TWIS_EVENT_TXSTARTED))
                {
                    substate = NRFX_TWIS_SUBSTATE_READ_PENDING;
                    evdata.data.buf_req = false;
                }
                else
                {
                    substate = NRFX_TWIS_SUBSTATE_READ_WAITING;
                    evdata.data.buf_req = true;
                }
                call_event_handler(p_cb, &evdata);
                ev = nrfx_twis_clear_bit(ev, NRF_TWIS_EVENT_READ);
                ev = nrfx_twis_clear_bit(ev, NRF_TWIS_EVENT_TXSTARTED);
                ev = nrfx_twis_clear_bit(ev, NRF_TWIS_EVENT_WRITE);
                ev = nrfx_twis_clear_bit(ev, NRF_TWIS_EVENT_RXSTARTED);
            }
            else if (nrfx_twis_check_bit(ev, NRF_TWIS_EVENT_WRITE))
            {
                evdata.type = NRFX_TWIS_EVT_WRITE_REQ;
                if (nrfx_twis_check_bit(ev, NRF_TWIS_EVENT_RXSTARTED))
                {
                    substate = NRFX_TWIS_SUBSTATE_WRITE_PENDING;
                    evdata.data.buf_req = false;
                }
                else
                {
                    substate = NRFX_TWIS_SUBSTATE_WRITE_WAITING;
                    evdata.data.buf_req = true;
                }
                call_event_handler(p_cb, &evdata);
                ev = nrfx_twis_clear_bit(ev, NRF_TWIS_EVENT_READ);
                ev = nrfx_twis_clear_bit(ev, NRF_TWIS_EVENT_TXSTARTED);
                ev = nrfx_twis_clear_bit(ev, NRF_TWIS_EVENT_WRITE);
                ev = nrfx_twis_clear_bit(ev, NRF_TWIS_EVENT_RXSTARTED);
            }
            else
            {
                nrfx_twis_process_error(p_cb,
                                        NRFX_TWIS_EVT_GENERAL_ERROR,
                                        nrf_twis_error_source_get_and_clear(p_reg));
                ev = 0;
            }
            break;
        case NRFX_TWIS_SUBSTATE_READ_WAITING:
            if (nrfx_twis_check_bit(ev, NRF_TWIS_EVENT_TXSTARTED) ||
                nrfx_twis_check_bit(ev, NRF_TWIS_EVENT_WRITE)     ||
                nrfx_twis_check_bit(ev, NRF_TWIS_EVENT_READ)      ||
                nrfx_twis_check_bit(ev, NRF_TWIS_EVENT_STOPPED))
            {
                substate = NRFX_TWIS_SUBSTATE_READ_PENDING;
                /* Any other bits requires further processing in PENDING substate */
                ev = nrfx_twis_clear_bit(ev, NRF_TWIS_EVENT_TXSTARTED);
            }
            else
            {
                nrfx_twis_process_error(p_cb,
                                        NRFX_TWIS_EVT_READ_ERROR,
                                        nrf_twis_error_source_get_and_clear(p_reg));
                substate = NRFX_TWIS_SUBSTATE_IDLE;
                ev = 0;
            }
            break;
        case NRFX_TWIS_SUBSTATE_READ_PENDING:
            if (nrfx_twis_check_bit(ev, NRF_TWIS_EVENT_WRITE) ||
                nrfx_twis_check_bit(ev, NRF_TWIS_EVENT_READ)  ||
                nrfx_twis_check_bit(ev, NRF_TWIS_EVENT_STOPPED))
            {
                evdata.type = NRFX_TWIS_EVT_READ_DONE;
                evdata.data.tx_amount = nrf_twis_tx_amount_get(p_reg);
                NRFX_LOG_INFO("Transfer tx_len:%d", evdata.data.tx_amount);
                NRFX_LOG_DEBUG("Tx data:");
                NRFX_LOG_HEXDUMP_DEBUG((uint8_t const *)p_reg->TXD.PTR,
                                       evdata.data.tx_amount * sizeof(uint8_t));
                call_event_handler(p_cb, &evdata);
                /* Go to idle and repeat the state machine if READ or WRITE events detected.
                 * This time READ or WRITE would be started */
                substate = NRFX_TWIS_SUBSTATE_IDLE;
                ev = nrfx_twis_clear_bit(ev, NRF_TWIS_EVENT_STOPPED);
            }
            else
            {
                nrfx_twis_process_error(p_cb,
                                        NRFX_TWIS_EVT_READ_ERROR,
                                        nrf_twis_error_source_get_and_clear(p_reg));
                substate = NRFX_TWIS_SUBSTATE_IDLE;
                ev = 0;
            }
            break;
        case NRFX_TWIS_SUBSTATE_WRITE_WAITING:
            if (nrfx_twis_check_bit(ev, NRF_TWIS_EVENT_RXSTARTED) ||
                nrfx_twis_check_bit(ev, NRF_TWIS_EVENT_WRITE)     ||
                nrfx_twis_check_bit(ev, NRF_TWIS_EVENT_READ)      ||
                nrfx_twis_check_bit(ev, NRF_TWIS_EVENT_STOPPED))
            {
                substate = NRFX_TWIS_SUBSTATE_WRITE_PENDING;
                /* Any other bits requires further processing in PENDING substate */
                ev = nrfx_twis_clear_bit(ev, NRF_TWIS_EVENT_RXSTARTED);
            }
            else
            {
                nrfx_twis_process_error(p_cb,
                                        NRFX_TWIS_EVT_WRITE_ERROR,
                                        nrf_twis_error_source_get_and_clear(p_reg));
                substate = NRFX_TWIS_SUBSTATE_IDLE;
                ev = 0;
            }
            break;
        case NRFX_TWIS_SUBSTATE_WRITE_PENDING:
            if (nrfx_twis_check_bit(ev, NRF_TWIS_EVENT_WRITE) ||
                nrfx_twis_check_bit(ev, NRF_TWIS_EVENT_READ)  ||
                nrfx_twis_check_bit(ev, NRF_TWIS_EVENT_STOPPED))
            {
                evdata.type = NRFX_TWIS_EVT_WRITE_DONE;
                evdata.data.rx_amount = nrf_twis_rx_amount_get(p_reg);
                call_event_handler(p_cb, &evdata);
                /* Go to idle and repeat the state machine if READ or WRITE events detected.
                 * This time READ or WRITE would be started */
                substate = NRFX_TWIS_SUBSTATE_IDLE;
                ev = nrfx_twis_clear_bit(ev, NRF_TWIS_EVENT_STOPPED);
            }
            else
            {
                nrfx_twis_process_error(p_cb,
                                        NRFX_TWIS_EVT_WRITE_ERROR,
                                        nrf_twis_error_source_get_and_clear(p_reg));
                substate = NRFX_TWIS_SUBSTATE_IDLE;
                ev = 0;
            }
            break;
        default:
            substate = NRFX_TWIS_SUBSTATE_IDLE;
            /* Do not clear any events and repeat the machine */
            break;
        }
    }

    p_cb->substate = substate;
    if (!NRFX_TWIS_NO_SYNC_MODE)
    {
        p_cb->semaphore = 0;
    }
}


static inline void nrfx_twis_preprocess_status(nrfx_twis_t const * p_instance)
{
    if (!NRFX_TWIS_NO_SYNC_MODE)
    {
        NRF_TWIS_Type *        p_reg = p_instance->p_reg;
        twis_control_block_t * p_cb  = &m_cb[p_instance->drv_inst_idx];
        if (NULL == p_cb->ev_handler)
        {
            nrfx_twis_state_machine(p_reg, p_cb);
        }
    }
}


/* -------------------------------------------------------------------------
 * Implementation of interface functions
 *
 */


nrfx_err_t nrfx_twis_init(nrfx_twis_t const *        p_instance,
                          nrfx_twis_config_t const * p_config,
                          nrfx_twis_event_handler_t  event_handler)
{
    NRFX_ASSERT(p_config);
    NRFX_ASSERT(p_config->scl != p_config->sda);
    nrfx_err_t err_code;

    NRF_TWIS_Type *        p_reg = p_instance->p_reg;
    twis_control_block_t * p_cb  = &m_cb[p_instance->drv_inst_idx];

    if (p_cb->state != NRFX_DRV_STATE_UNINITIALIZED)
    {
        err_code = NRFX_ERROR_INVALID_STATE;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

#if NRFX_CHECK(NRFX_PRS_ENABLED)
    static nrfx_irq_handler_t const irq_handlers[NRFX_TWIS_ENABLED_COUNT] = {
        #if NRFX_CHECK(NRFX_TWIS0_ENABLED)
        nrfx_twis_0_irq_handler,
        #endif
        #if NRFX_CHECK(NRFX_TWIS1_ENABLED)
        nrfx_twis_1_irq_handler,
        #endif
        #if NRFX_CHECK(NRFX_TWIS2_ENABLED)
        nrfx_twis_2_irq_handler,
        #endif
        #if NRFX_CHECK(NRFX_TWIS3_ENABLED)
        nrfx_twis_3_irq_handler,
        #endif
    };
    if (nrfx_prs_acquire(p_reg,
            irq_handlers[p_instance->drv_inst_idx]) != NRFX_SUCCESS)
    {
        err_code = NRFX_ERROR_BUSY;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }
#endif // NRFX_CHECK(NRFX_PRS_ENABLED)

    if (!NRFX_TWIS_ASSUME_INIT_AFTER_RESET_ONLY)
    {
        nrfx_twis_swreset(p_reg);
    }

    nrfx_twis_config_pin(p_config->scl, p_config->scl_pull);
    nrfx_twis_config_pin(p_config->sda, p_config->sda_pull);

    nrf_twis_config_addr_mask_t addr_mask = (nrf_twis_config_addr_mask_t)0;
    if (0 == (p_config->addr[0] | p_config->addr[1]))
    {
        addr_mask = NRF_TWIS_CONFIG_ADDRESS0_MASK;
    }
    else
    {
        if (0 != p_config->addr[0])
        {
            addr_mask |= NRF_TWIS_CONFIG_ADDRESS0_MASK;
        }
        if (0 != p_config->addr[1])
        {
            addr_mask |= NRF_TWIS_CONFIG_ADDRESS1_MASK;
        }
    }

    /* Peripheral interrupt configure
     * (note - interrupts still needs to be configured in INTEN register.
     * This is done in enable function) */
    NRFX_IRQ_PRIORITY_SET(nrfx_get_irq_number(p_reg),
                          p_config->interrupt_priority);
    NRFX_IRQ_ENABLE(nrfx_get_irq_number(p_reg));

    /* Configure */
    nrf_twis_pins_set          (p_reg, p_config->scl, p_config->sda);
    nrf_twis_address_set       (p_reg, 0, p_config->addr[0]);
    nrf_twis_address_set       (p_reg, 1, p_config->addr[1]);
    nrf_twis_config_address_set(p_reg, addr_mask);

    /* Clear semaphore */
    if (!NRFX_TWIS_NO_SYNC_MODE)
    {
        p_cb->semaphore = 0;
    }
    /* Set internal instance variables */
    p_cb->substate   = NRFX_TWIS_SUBSTATE_IDLE;
    p_cb->ev_handler = event_handler;
    p_cb->state      = NRFX_DRV_STATE_INITIALIZED;
    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}


void nrfx_twis_uninit(nrfx_twis_t const * p_instance)
{
    NRF_TWIS_Type *        p_reg = p_instance->p_reg;
    twis_control_block_t * p_cb  = &m_cb[p_instance->drv_inst_idx];
    NRFX_ASSERT(p_cb->state != NRFX_DRV_STATE_UNINITIALIZED);

    TWIS_PSEL_Type psel = p_reg->PSEL;

    nrfx_twis_swreset(p_reg);

    /* Clear pins state if */
    if (!(TWIS_PSEL_SCL_CONNECT_Msk & psel.SCL))
    {
        nrf_gpio_cfg_default(psel.SCL);
    }
    if (!(TWIS_PSEL_SDA_CONNECT_Msk & psel.SDA))
    {
        nrf_gpio_cfg_default(psel.SDA);
    }

#if NRFX_CHECK(NRFX_PRS_ENABLED)
    nrfx_prs_release(p_reg);
#endif

    /* Clear variables */
    p_cb->ev_handler = NULL;
    p_cb->state      = NRFX_DRV_STATE_UNINITIALIZED;
}


void nrfx_twis_enable(nrfx_twis_t const * p_instance)
{
    NRF_TWIS_Type *        p_reg = p_instance->p_reg;
    twis_control_block_t * p_cb  = &m_cb[p_instance->drv_inst_idx];
    NRFX_ASSERT(p_cb->state == NRFX_DRV_STATE_INITIALIZED);

    nrfx_twis_clear_all_events(p_reg);

    /* Enable interrupts */
    if (NULL != p_cb->ev_handler)
    {
        nrf_twis_int_enable(p_reg, m_used_ints_mask);
    }

    nrf_twis_enable(p_reg);
    p_cb->error    = 0;
    p_cb->state    = NRFX_DRV_STATE_POWERED_ON;
    p_cb->substate = NRFX_TWIS_SUBSTATE_IDLE;
}


void nrfx_twis_disable(nrfx_twis_t const * p_instance)
{
    NRF_TWIS_Type *        p_reg = p_instance->p_reg;
    twis_control_block_t * p_cb  = &m_cb[p_instance->drv_inst_idx];
    NRFX_ASSERT(p_cb->state != NRFX_DRV_STATE_UNINITIALIZED);

    nrf_twis_int_disable(p_reg, m_used_ints_mask);

    nrf_twis_disable(p_reg);
    p_cb->state = NRFX_DRV_STATE_INITIALIZED;
}

/* ARM recommends not using the LDREX and STREX instructions in C code.
 * This is because the compiler might generate loads and stores between
 * LDREX and STREX, potentially clearing the exclusive monitor set by LDREX.
 * This recommendation also applies to the byte, halfword, and doubleword
 * variants LDREXB, STREXB, LDREXH, STREXH, LDREXD, and STREXD.
 *
 * This is the reason for the function below to be implemented in assembly.
 */
//lint -save -e578
#if defined (__CC_ARM )
static __ASM uint32_t nrfx_twis_error_get_and_clear_internal(uint32_t volatile * perror)
{
    mov   r3, r0
    mov   r1, #0
nrfx_twis_error_get_and_clear_internal_try
    ldrex r0, [r3]
    strex r2, r1, [r3]
    cmp   r2, r1                                     /* did this succeed?       */
    bne   nrfx_twis_error_get_and_clear_internal_try /* no - try again          */
    bx    lr
}
#elif defined ( __GNUC__ )
static uint32_t nrfx_twis_error_get_and_clear_internal(uint32_t volatile * perror)
{
    uint32_t ret;
    uint32_t temp;
    __ASM volatile(
        "   .syntax unified           \n"
        "nrfx_twis_error_get_and_clear_internal_try:         \n"
        "   ldrex %[ret], [%[perror]]                        \n"
        "   strex %[temp], %[zero], [%[perror]]              \n"
        "   cmp   %[temp], %[zero]                           \n"
        "   bne   nrfx_twis_error_get_and_clear_internal_try \n"
    : /* Output */
        [ret]"=&l"(ret),
        [temp]"=&l"(temp)
    : /* Input */
        [zero]"l"(0),
        [perror]"l"(perror)
    );
    (void)temp;
    return ret;
}
#elif defined ( __ICCARM__ )
static uint32_t nrfx_twis_error_get_and_clear_internal(uint32_t volatile * perror)
{
    uint32_t ret;
    uint32_t temp;
    __ASM volatile(
        "1:         \n"
        "   ldrex %[ret], [%[perror]]                           \n"
        "   strex %[temp], %[zero], [%[perror]]                 \n"
        "   cmp   %[temp], %[zero]                              \n"
        "   bne.n 1b \n"
    : /* Output */
        [ret]"=&l"(ret),
        [temp]"=&l"(temp)
    : /* Input */
        [zero]"l"(0),
        [perror]"l"(perror)
    );
    (void)temp;
    return ret;
}
#else
    #error Unknown compiler
#endif
//lint -restore

uint32_t nrfx_twis_error_get_and_clear(nrfx_twis_t const * p_instance)
{
    nrfx_twis_preprocess_status(p_instance);
    /* Make sure that access to error member is atomic
     * so there is no bit that is cleared if it is not copied to local variable already. */
    twis_control_block_t * p_cb  = &m_cb[p_instance->drv_inst_idx];
    return nrfx_twis_error_get_and_clear_internal(&p_cb->error);
}


nrfx_err_t nrfx_twis_tx_prepare(nrfx_twis_t const * p_instance,
                                void const *        p_buf,
                                size_t              size)
{
    nrfx_err_t err_code;
    twis_control_block_t const * p_cb = &m_cb[p_instance->drv_inst_idx];

    /* Check power state*/
    if (p_cb->state != NRFX_DRV_STATE_POWERED_ON)
    {
        err_code = NRFX_ERROR_INVALID_STATE;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }
    /* Check data address */
    if (!nrfx_is_in_ram(p_buf))
    {
        err_code = NRFX_ERROR_INVALID_ADDR;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }
    /* Check data size */
    if ((size & TWIS_TXD_MAXCNT_MAXCNT_Msk) != size)
    {
        err_code = NRFX_ERROR_INVALID_LENGTH;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    nrf_twis_tx_prepare(p_instance->p_reg,
                        (uint8_t const *)p_buf,
                        (nrf_twis_amount_t)size);
    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}


nrfx_err_t nrfx_twis_rx_prepare(nrfx_twis_t const * p_instance,
                                void *              p_buf,
                                size_t              size)
{
    nrfx_err_t err_code;
    twis_control_block_t const * p_cb = &m_cb[p_instance->drv_inst_idx];

    /* Check power state*/
    if (p_cb->state != NRFX_DRV_STATE_POWERED_ON)
    {
        err_code = NRFX_ERROR_INVALID_STATE;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }
    /* Check data address */
    if (!nrfx_is_in_ram(p_buf))
    {
        err_code = NRFX_ERROR_INVALID_ADDR;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }
    /* Check data size */
    if ((size & TWIS_RXD_MAXCNT_MAXCNT_Msk) != size)
    {
        err_code = NRFX_ERROR_INVALID_LENGTH;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    nrf_twis_rx_prepare(p_instance->p_reg,
                        (uint8_t *)p_buf,
                        (nrf_twis_amount_t)size);
    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}


bool nrfx_twis_is_busy(nrfx_twis_t const * p_instance)
{
    nrfx_twis_preprocess_status(p_instance);
    twis_control_block_t const * p_cb = &m_cb[p_instance->drv_inst_idx];
    return NRFX_TWIS_SUBSTATE_IDLE != p_cb->substate;
}

bool nrfx_twis_is_waiting_tx_buff(nrfx_twis_t const * p_instance)
{
    nrfx_twis_preprocess_status(p_instance);
    twis_control_block_t const * p_cb = &m_cb[p_instance->drv_inst_idx];
    return NRFX_TWIS_SUBSTATE_READ_WAITING == p_cb->substate;
}

bool nrfx_twis_is_waiting_rx_buff(nrfx_twis_t const * p_instance)
{
    nrfx_twis_preprocess_status(p_instance);
    twis_control_block_t const * p_cb = &m_cb[p_instance->drv_inst_idx];
    return NRFX_TWIS_SUBSTATE_WRITE_WAITING == p_cb->substate;
}

bool nrfx_twis_is_pending_tx(nrfx_twis_t const * p_instance)
{
    nrfx_twis_preprocess_status(p_instance);
    twis_control_block_t const * p_cb = &m_cb[p_instance->drv_inst_idx];
    return NRFX_TWIS_SUBSTATE_READ_PENDING == p_cb->substate;
}

bool nrfx_twis_is_pending_rx(nrfx_twis_t const * p_instance)
{
    nrfx_twis_preprocess_status(p_instance);
    twis_control_block_t const * p_cb = &m_cb[p_instance->drv_inst_idx];
    return NRFX_TWIS_SUBSTATE_WRITE_PENDING == p_cb->substate;
}


#if NRFX_CHECK(NRFX_TWIS0_ENABLED)
void nrfx_twis_0_irq_handler(void)
{
    nrfx_twis_state_machine(NRF_TWIS0, &m_cb[NRFX_TWIS0_INST_IDX]);
}
#endif

#if NRFX_CHECK(NRFX_TWIS1_ENABLED)
void nrfx_twis_1_irq_handler(void)
{
    nrfx_twis_state_machine(NRF_TWIS1, &m_cb[NRFX_TWIS1_INST_IDX]);
}
#endif

#if NRFX_CHECK(NRFX_TWIS2_ENABLED)
void nrfx_twis_2_irq_handler(void)
{
    nrfx_twis_state_machine(NRF_TWIS2, &m_cb[NRFX_TWIS2_INST_IDX]);
}
#endif

#if NRFX_CHECK(NRFX_TWIS3_ENABLED)
void nrfx_twis_3_irq_handler(void)
{
    nrfx_twis_state_machine(NRF_TWIS3, &m_cb[NRFX_TWIS3_INST_IDX]);
}
#endif

#endif // NRFX_CHECK(NRFX_TWIS_ENABLED)
