/*
 * Copyright (c) 2016 - 2019, Nordic Semiconductor ASA
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

#if NRFX_CHECK(NRFX_USBD_ENABLED)

#include <nrfx_usbd.h>
#include "nrfx_usbd_errata.h"
#include <nrfx_systick.h> /* Marker to delete when not required anymore: >> NRFX_USBD_ERRATA_ENABLE << */
#include <string.h>

#define NRFX_LOG_MODULE USBD
#include <nrfx_log.h>

#ifndef NRFX_USBD_EARLY_DMA_PROCESS
/* Try to process DMA request when endpoint transmission has been detected
 * and just after last EasyDMA has been processed.
 * It speeds up the transmission a little (about 10% measured)
 * with a cost of more CPU power used.
 */
#define NRFX_USBD_EARLY_DMA_PROCESS 1
#endif

#ifndef NRFX_USBD_PROTO1_FIX_DEBUG
/* Debug information when events are fixed*/
#define NRFX_USBD_PROTO1_FIX_DEBUG 1
#endif

#define NRFX_USBD_LOG_PROTO1_FIX_PRINTF(...)                           \
    do{                                                                \
        if (NRFX_USBD_PROTO1_FIX_DEBUG){ NRFX_LOG_DEBUG(__VA_ARGS__); }\
    } while (0)

#ifndef NRFX_USBD_STARTED_EV_ENABLE
#define NRFX_USBD_STARTED_EV_ENABLE    0
#endif

#ifndef NRFX_USBD_CONFIG_ISO_IN_ZLP
/*
 * Respond to an IN token on ISO IN endpoint with ZLP when no data is ready.
 * NOTE: This option does not work on Engineering A chip.
 */
#define NRFX_USBD_CONFIG_ISO_IN_ZLP  0
#endif

#ifndef NRFX_USBD_ISO_DEBUG
/* Also generate information about ISOCHRONOUS events and transfers.
 * Turn this off if no ISOCHRONOUS transfers are going to be debugged and this
 * option generates a lot of useless messages. */
#define NRFX_USBD_ISO_DEBUG 1
#endif

#ifndef NRFX_USBD_FAILED_TRANSFERS_DEBUG
/* Also generate debug information for failed transfers.
 * It might be useful but may generate a lot of useless debug messages
 * in some library usages (for example when transfer is generated and the
 * result is used to check whatever endpoint was busy. */
#define NRFX_USBD_FAILED_TRANSFERS_DEBUG 1
#endif

#ifndef NRFX_USBD_DMAREQ_PROCESS_DEBUG
/* Generate additional messages that mark the status inside
 * @ref usbd_dmareq_process.
 * It is useful to debug library internals but may generate a lot of
 * useless debug messages. */
#define NRFX_USBD_DMAREQ_PROCESS_DEBUG 1
#endif


/**
 * @defgroup nrfx_usbd_int USB Device driver internal part
 * @internal
 * @ingroup nrfx_usbd
 *
 * This part contains auxiliary internal macros, variables and functions.
 * @{
 */

/**
 * @brief Assert endpoint number validity.
 *
 * Internal macro to be used during program creation in debug mode.
 * Generates assertion if endpoint number is not valid.
 *
 * @param ep Endpoint number to validity check.
 */
#define NRFX_USBD_ASSERT_EP_VALID(ep) NRFX_ASSERT(                               \
    ((NRF_USBD_EPIN_CHECK(ep) && (NRF_USBD_EP_NR_GET(ep) < NRF_USBD_EPIN_CNT ))  \
    ||                                                                           \
    (NRF_USBD_EPOUT_CHECK(ep) && (NRF_USBD_EP_NR_GET(ep) < NRF_USBD_EPOUT_CNT))) \
);

/**
 * @brief Lowest position of bit for IN endpoint.
 *
 * The first bit position corresponding to IN endpoint.
 * @sa ep2bit bit2ep
 */
#define NRFX_USBD_EPIN_BITPOS_0   0

/**
 * @brief Lowest position of bit for OUT endpoint.
 *
 * The first bit position corresponding to OUT endpoint
 * @sa ep2bit bit2ep
 */
#define NRFX_USBD_EPOUT_BITPOS_0  16

/**
 * @brief Input endpoint bits mask.
 */
#define NRFX_USBD_EPIN_BIT_MASK (0xFFFFU << NRFX_USBD_EPIN_BITPOS_0)

/**
 * @brief Output endpoint bits mask.
 */
#define NRFX_USBD_EPOUT_BIT_MASK (0xFFFFU << NRFX_USBD_EPOUT_BITPOS_0)

/**
 * @brief Isochronous endpoint bit mask
 */
#define USBD_EPISO_BIT_MASK \
    ((1U << NRFX_USBD_EP_BITPOS(NRFX_USBD_EPOUT8)) | \
     (1U << NRFX_USBD_EP_BITPOS(NRFX_USBD_EPIN8)))

/**
 * @brief Auxiliary macro to change EP number into bit position.
 *
 * This macro is used by @ref ep2bit function but also for statically check
 * the bitpos values integrity during compilation.
 *
 * @param[in] ep Endpoint number.
 * @return Endpoint bit position.
 */
#define NRFX_USBD_EP_BITPOS(ep) \
    ((NRF_USBD_EPIN_CHECK(ep) ? NRFX_USBD_EPIN_BITPOS_0 : NRFX_USBD_EPOUT_BITPOS_0) \
     + NRF_USBD_EP_NR_GET(ep))

/**
 * @brief Helper macro for creating an endpoint transfer event.
 *
 * @param[in] name     Name of the created transfer event variable.
 * @param[in] endpoint Endpoint number.
 * @param[in] ep_stat  Endpoint state to report.
 *
 * @return Initialized event constant variable.
 */
#define NRFX_USBD_EP_TRANSFER_EVENT(name, endpont, ep_stat) \
    const nrfx_usbd_evt_t name = {                          \
        NRFX_USBD_EVT_EPTRANSFER,                           \
        .data = {                                           \
            .eptransfer = {                                 \
                    .ep = endpont,                          \
                    .status = ep_stat                       \
            }                                               \
        }                                                   \
    }

/* Check it the bit positions values match defined DATAEPSTATUS bit positions */
NRFX_STATIC_ASSERT(NRFX_USBD_EP_BITPOS(NRFX_USBD_EPIN1)  == USBD_EPDATASTATUS_EPIN1_Pos );
NRFX_STATIC_ASSERT(NRFX_USBD_EP_BITPOS(NRFX_USBD_EPIN2)  == USBD_EPDATASTATUS_EPIN2_Pos );
NRFX_STATIC_ASSERT(NRFX_USBD_EP_BITPOS(NRFX_USBD_EPIN3)  == USBD_EPDATASTATUS_EPIN3_Pos );
NRFX_STATIC_ASSERT(NRFX_USBD_EP_BITPOS(NRFX_USBD_EPIN4)  == USBD_EPDATASTATUS_EPIN4_Pos );
NRFX_STATIC_ASSERT(NRFX_USBD_EP_BITPOS(NRFX_USBD_EPIN5)  == USBD_EPDATASTATUS_EPIN5_Pos );
NRFX_STATIC_ASSERT(NRFX_USBD_EP_BITPOS(NRFX_USBD_EPIN6)  == USBD_EPDATASTATUS_EPIN6_Pos );
NRFX_STATIC_ASSERT(NRFX_USBD_EP_BITPOS(NRFX_USBD_EPIN7)  == USBD_EPDATASTATUS_EPIN7_Pos );
NRFX_STATIC_ASSERT(NRFX_USBD_EP_BITPOS(NRFX_USBD_EPOUT1) == USBD_EPDATASTATUS_EPOUT1_Pos);
NRFX_STATIC_ASSERT(NRFX_USBD_EP_BITPOS(NRFX_USBD_EPOUT2) == USBD_EPDATASTATUS_EPOUT2_Pos);
NRFX_STATIC_ASSERT(NRFX_USBD_EP_BITPOS(NRFX_USBD_EPOUT3) == USBD_EPDATASTATUS_EPOUT3_Pos);
NRFX_STATIC_ASSERT(NRFX_USBD_EP_BITPOS(NRFX_USBD_EPOUT4) == USBD_EPDATASTATUS_EPOUT4_Pos);
NRFX_STATIC_ASSERT(NRFX_USBD_EP_BITPOS(NRFX_USBD_EPOUT5) == USBD_EPDATASTATUS_EPOUT5_Pos);
NRFX_STATIC_ASSERT(NRFX_USBD_EP_BITPOS(NRFX_USBD_EPOUT6) == USBD_EPDATASTATUS_EPOUT6_Pos);
NRFX_STATIC_ASSERT(NRFX_USBD_EP_BITPOS(NRFX_USBD_EPOUT7) == USBD_EPDATASTATUS_EPOUT7_Pos);


/**
 * @brief Current driver state.
 */
static nrfx_drv_state_t m_drv_state = NRFX_DRV_STATE_UNINITIALIZED;

/**
 * @brief Event handler for the driver.
 *
 * Event handler that would be called on events.
 *
 * @note Currently it cannot be null if any interrupt is activated.
 */
static nrfx_usbd_event_handler_t m_event_handler;

/**
 * @brief Detected state of the bus.
 *
 * Internal state changed in interrupts handling when
 * RESUME or SUSPEND event is processed.
 *
 * Values:
 * - true  - bus suspended
 * - false - ongoing normal communication on the bus
 *
 * @note This is only the bus state and does not mean that the peripheral is in suspend state.
 */
static volatile bool m_bus_suspend;

/**
 * @brief Internal constant that contains interrupts disabled in suspend state.
 *
 * Internal constant used in @ref nrfx_usbd_suspend_irq_config and @ref nrfx_usbd_active_irq_config
 * functions.
 */
static const uint32_t m_irq_disabled_in_suspend =
    NRF_USBD_INT_ENDEPIN0_MASK    |
    NRF_USBD_INT_EP0DATADONE_MASK |
    NRF_USBD_INT_ENDEPOUT0_MASK   |
    NRF_USBD_INT_EP0SETUP_MASK    |
    NRF_USBD_INT_DATAEP_MASK;

/**
 * @brief Direction of last received Setup transfer.
 *
 * This variable is used to redirect internal setup data event
 * into selected endpoint (IN or OUT).
 */
static nrfx_usbd_ep_t m_last_setup_dir;

/**
 * @brief Mark endpoint readiness for DMA transfer.
 *
 * Bits in this variable are cleared and set in interrupts.
 * 1 means that endpoint is ready for DMA transfer.
 * 0 means that DMA transfer cannot be performed on selected endpoint.
 */
static uint32_t m_ep_ready;

/**
 * @brief Mark endpoint with prepared data to transfer by DMA.
 *
 * This variable can be from any place in the code (interrupt or main thread).
 * It would be cleared only from USBD interrupt.
 *
 * Mask prepared USBD data for transmission.
 * It is cleared when no more data to transmit left.
 */
static uint32_t m_ep_dma_waiting;

/**
 * @brief Current EasyDMA state.
 *
 * Single flag, updated only inside interrupts, that marks current EasyDMA state.
 * In USBD there is only one DMA channel working in background, and new transfer
 * cannot be started when there is ongoing transfer on any other channel.
 */
static bool m_dma_pending;

/**
 * @brief Simulated data EP status bits required for errata 104.
 *
 * Marker to delete when not required anymore: >> NRFX_USBD_ERRATA_ENABLE <<.
 */
static uint32_t m_simulated_dataepstatus;

/**
 * @brief The structure that would hold transfer configuration to every endpoint
 *
 * The structure that holds all the data required by the endpoint to proceed
 * with LIST functionality and generate quick callback directly when data
 * buffer is ready.
 */
typedef struct
{
    nrfx_usbd_handler_t   handler;         //!< Handler for current transfer, function pointer.
    void *                p_context;       //!< Context for transfer handler.
    size_t                transfer_cnt;    //!< Number of transferred bytes in the current transfer.
    uint16_t              max_packet_size; //!< Configured endpoint size.
    nrfx_usbd_ep_status_t status;          //!< NRFX_SUCCESS or error code, never NRFX_ERROR_BUSY - this one is calculated.
} usbd_ep_state_t;

/**
 * @brief The array of transfer configurations for the endpoints.
 *
 * The status of the transfer on each endpoint.
 */
static struct
{
    usbd_ep_state_t ep_out[NRF_USBD_EPOUT_CNT]; //!< Status for OUT endpoints.
    usbd_ep_state_t ep_in [NRF_USBD_EPIN_CNT ]; //!< Status for IN endpoints.
} m_ep_state;

/**
 * @brief Status variables for integrated feeders.
 *
 * Current status for integrated feeders (IN transfers).
 * Integrated feeders are used for default transfers:
 * 1. Simple RAM transfer.
 * 2. Simple flash transfer.
 * 3. RAM transfer with automatic ZLP.
 * 4. Flash transfer with automatic ZLP.
 */
nrfx_usbd_transfer_t m_ep_feeder_state[NRF_USBD_EPIN_CNT];

/**
 * @brief Status variables for integrated consumers.
 *
 * Current status for integrated consumers.
 * Currently one type of transfer is supported:
 * 1. Transfer to RAM.
 *
 * Transfer is finished automatically when received data block is smaller
 * than the endpoint buffer or all the required data is received.
 */
nrfx_usbd_transfer_t m_ep_consumer_state[NRF_USBD_EPOUT_CNT];


/**
 * @brief Buffer used to send data directly from FLASH.
 *
 * This is internal buffer that would be used to emulate the possibility
 * to transfer data directly from FLASH.
 * We do not have to care about the source of data when calling transfer functions.
 *
 * We do not need more buffers that one, because only one transfer can be pending
 * at once.
 */
static uint32_t m_tx_buffer[NRFX_CEIL_DIV(
    NRFX_USBD_FEEDER_BUFFER_SIZE, sizeof(uint32_t))];

/* Early declaration. Documentation above definition. */
static void usbd_dmareq_process(void);


/**
 * @brief Change endpoint number to endpoint event code.
 *
 * @param ep Endpoint number.
 *
 * @return Connected endpoint event code.
 *
 * Marker to delete when not required anymore: >> NRFX_USBD_ERRATA_ENABLE <<.
 */
static inline nrf_usbd_event_t nrfx_usbd_ep_to_endevent(nrfx_usbd_ep_t ep)
{
    NRFX_USBD_ASSERT_EP_VALID(ep);

    static const nrf_usbd_event_t epin_endev[] =
    {
        NRF_USBD_EVENT_ENDEPIN0,
        NRF_USBD_EVENT_ENDEPIN1,
        NRF_USBD_EVENT_ENDEPIN2,
        NRF_USBD_EVENT_ENDEPIN3,
        NRF_USBD_EVENT_ENDEPIN4,
        NRF_USBD_EVENT_ENDEPIN5,
        NRF_USBD_EVENT_ENDEPIN6,
        NRF_USBD_EVENT_ENDEPIN7,
        NRF_USBD_EVENT_ENDISOIN0
    };
    static const nrf_usbd_event_t epout_endev[] =
    {
        NRF_USBD_EVENT_ENDEPOUT0,
        NRF_USBD_EVENT_ENDEPOUT1,
        NRF_USBD_EVENT_ENDEPOUT2,
        NRF_USBD_EVENT_ENDEPOUT3,
        NRF_USBD_EVENT_ENDEPOUT4,
        NRF_USBD_EVENT_ENDEPOUT5,
        NRF_USBD_EVENT_ENDEPOUT6,
        NRF_USBD_EVENT_ENDEPOUT7,
        NRF_USBD_EVENT_ENDISOOUT0
    };

    return (NRF_USBD_EPIN_CHECK(ep) ? epin_endev : epout_endev)[NRF_USBD_EP_NR_GET(ep)];
}


/**
 * @brief Get interrupt mask for selected endpoint.
 *
 * @param[in] ep Endpoint number.
 *
 * @return Interrupt mask related to the EasyDMA transfer end for the
 *         chosen endpoint.
 */
static inline uint32_t nrfx_usbd_ep_to_int(nrfx_usbd_ep_t ep)
{
    NRFX_USBD_ASSERT_EP_VALID(ep);

    static const uint8_t epin_bitpos[] =
    {
        USBD_INTEN_ENDEPIN0_Pos,
        USBD_INTEN_ENDEPIN1_Pos,
        USBD_INTEN_ENDEPIN2_Pos,
        USBD_INTEN_ENDEPIN3_Pos,
        USBD_INTEN_ENDEPIN4_Pos,
        USBD_INTEN_ENDEPIN5_Pos,
        USBD_INTEN_ENDEPIN6_Pos,
        USBD_INTEN_ENDEPIN7_Pos,
        USBD_INTEN_ENDISOIN_Pos
    };
    static const uint8_t epout_bitpos[] =
    {
        USBD_INTEN_ENDEPOUT0_Pos,
        USBD_INTEN_ENDEPOUT1_Pos,
        USBD_INTEN_ENDEPOUT2_Pos,
        USBD_INTEN_ENDEPOUT3_Pos,
        USBD_INTEN_ENDEPOUT4_Pos,
        USBD_INTEN_ENDEPOUT5_Pos,
        USBD_INTEN_ENDEPOUT6_Pos,
        USBD_INTEN_ENDEPOUT7_Pos,
        USBD_INTEN_ENDISOOUT_Pos
    };

    return 1UL << (NRF_USBD_EPIN_CHECK(ep) ? epin_bitpos : epout_bitpos)[NRF_USBD_EP_NR_GET(ep)];
}

/**
 * @name Integrated feeders and consumers
 *
 * Internal, default functions for transfer processing.
 * @{
 */

/**
 * @brief Integrated consumer to RAM buffer.
 *
 * @param p_next    See @ref nrfx_usbd_consumer_t documentation.
 * @param p_context See @ref nrfx_usbd_consumer_t documentation.
 * @param ep_size   See @ref nrfx_usbd_consumer_t documentation.
 * @param data_size See @ref nrfx_usbd_consumer_t documentation.
 *
 * @retval true  Continue transfer.
 * @retval false This was the last transfer.
 */
bool nrfx_usbd_consumer(
    nrfx_usbd_ep_transfer_t * p_next,
    void * p_context,
    size_t ep_size,
    size_t data_size)
{
    nrfx_usbd_transfer_t * p_transfer = p_context;
    NRFX_ASSERT(ep_size >= data_size);
    NRFX_ASSERT((p_transfer->p_data.rx == NULL) ||
        nrfx_is_in_ram(p_transfer->p_data.rx));

    size_t size = p_transfer->size;
    if (size < data_size)
    {
        NRFX_LOG_DEBUG("consumer: buffer too small: r: %u, l: %u", data_size, size);
        /* Buffer size to small */
        p_next->size = 0;
        p_next->p_data = p_transfer->p_data;
    }
    else
    {
        p_next->size = data_size;
        p_next->p_data = p_transfer->p_data;
        size -= data_size;
        p_transfer->size = size;
        p_transfer->p_data.addr += data_size;
    }
    return (ep_size == data_size) && (size != 0);
}

/**
 * @brief Integrated feeder from RAM source.
 *
 * @param[out]    p_next    See @ref nrfx_usbd_feeder_t documentation.
 * @param[in,out] p_context See @ref nrfx_usbd_feeder_t documentation.
 * @param[in]     ep_size   See @ref nrfx_usbd_feeder_t documentation.
 *
 * @retval true  Continue transfer.
 * @retval false This was the last transfer.
 */
bool nrfx_usbd_feeder_ram(
    nrfx_usbd_ep_transfer_t * p_next,
    void * p_context,
    size_t ep_size)
{
    nrfx_usbd_transfer_t * p_transfer = p_context;
    NRFX_ASSERT(nrfx_is_in_ram(p_transfer->p_data.tx));

    size_t tx_size = p_transfer->size;
    if (tx_size > ep_size)
    {
        tx_size = ep_size;
    }

    p_next->p_data = p_transfer->p_data;
    p_next->size = tx_size;

    p_transfer->size -= tx_size;
    p_transfer->p_data.addr += tx_size;

    return (p_transfer->size != 0);
}

/**
 * @brief Integrated feeder from RAM source with ZLP.
 *
 * @param[out]    p_next    See @ref nrfx_usbd_feeder_t documentation.
 * @param[in,out] p_context See @ref nrfx_usbd_feeder_t documentation.
 * @param[in]     ep_size   See @ref nrfx_usbd_feeder_t documentation.
 *
 * @retval true  Continue transfer.
 * @retval false This was the last transfer.
 */
bool nrfx_usbd_feeder_ram_zlp(
    nrfx_usbd_ep_transfer_t * p_next,
    void * p_context,
    size_t ep_size)
{
    nrfx_usbd_transfer_t * p_transfer = p_context;
    NRFX_ASSERT(nrfx_is_in_ram(p_transfer->p_data.tx));

    size_t tx_size = p_transfer->size;
    if (tx_size > ep_size)
    {
        tx_size = ep_size;
    }

    p_next->p_data.tx = (tx_size == 0) ? NULL : p_transfer->p_data.tx;
    p_next->size = tx_size;

    p_transfer->size -= tx_size;
    p_transfer->p_data.addr += tx_size;

    return (tx_size != 0);
}

/**
 * @brief Integrated feeder from a flash source.
 *
 * @param[out]    p_next    See @ref nrfx_usbd_feeder_t documentation.
 * @param[in,out] p_context See @ref nrfx_usbd_feeder_t documentation.
 * @param[in]     ep_size   See @ref nrfx_usbd_feeder_t documentation.
 *
 * @retval true  Continue transfer.
 * @retval false This was the last transfer.
 */
bool nrfx_usbd_feeder_flash(nrfx_usbd_ep_transfer_t * p_next, void * p_context, size_t ep_size)
{
    nrfx_usbd_transfer_t * p_transfer = p_context;
    NRFX_ASSERT(!nrfx_is_in_ram(p_transfer->p_data.tx));

    size_t tx_size  = p_transfer->size;
    void * p_buffer = nrfx_usbd_feeder_buffer_get();

    if (tx_size > ep_size)
    {
        tx_size = ep_size;
    }

    NRFX_ASSERT(tx_size <= NRFX_USBD_FEEDER_BUFFER_SIZE);
    memcpy(p_buffer, (p_transfer->p_data.tx), tx_size);

    p_next->p_data.tx = p_buffer;
    p_next->size = tx_size;

    p_transfer->size -= tx_size;
    p_transfer->p_data.addr += tx_size;

    return (p_transfer->size != 0);
}

/**
 * @brief Integrated feeder from a flash source with ZLP.
 *
 * @param[out]    p_next    See @ref nrfx_usbd_feeder_t documentation.
 * @param[in,out] p_context See @ref nrfx_usbd_feeder_t documentation.
 * @param[in]     ep_size   See @ref nrfx_usbd_feeder_t documentation.
 *
 * @retval true  Continue transfer.
 * @retval false This was the last transfer.
 */
bool nrfx_usbd_feeder_flash_zlp(nrfx_usbd_ep_transfer_t * p_next, void * p_context, size_t ep_size)
{
    nrfx_usbd_transfer_t * p_transfer = p_context;
    NRFX_ASSERT(!nrfx_is_in_ram(p_transfer->p_data.tx));

    size_t tx_size  = p_transfer->size;
    void * p_buffer = nrfx_usbd_feeder_buffer_get();

    if (tx_size > ep_size)
    {
        tx_size = ep_size;
    }

    NRFX_ASSERT(tx_size <= NRFX_USBD_FEEDER_BUFFER_SIZE);

    if (tx_size != 0)
    {
        memcpy(p_buffer, (p_transfer->p_data.tx), tx_size);
        p_next->p_data.tx = p_buffer;
    }
    else
    {
        p_next->p_data.tx = NULL;
    }
    p_next->size = tx_size;

    p_transfer->size -= tx_size;
    p_transfer->p_data.addr += tx_size;

    return (tx_size != 0);
}

/** @} */

/**
 * @brief Change Driver endpoint number to HAL endpoint number.
 *
 * @param ep Driver endpoint identifier.
 *
 * @return Endpoint identifier in HAL.
 *
 * @sa nrfx_usbd_ep_from_hal
 */
static inline uint8_t ep_to_hal(nrfx_usbd_ep_t ep)
{
    NRFX_USBD_ASSERT_EP_VALID(ep);
    return (uint8_t)ep;
}

/**
 * @brief Generate start task number for selected endpoint index.
 *
 * @param ep Endpoint number.
 *
 * @return Task for starting EasyDMA transfer on selected endpoint.
 */
static inline nrf_usbd_task_t task_start_ep(nrfx_usbd_ep_t ep)
{
    NRFX_USBD_ASSERT_EP_VALID(ep);
    return (nrf_usbd_task_t)(
        (NRF_USBD_EPIN_CHECK(ep) ? NRF_USBD_TASK_STARTEPIN0 : NRF_USBD_TASK_STARTEPOUT0) +
        (NRF_USBD_EP_NR_GET(ep) * sizeof(uint32_t)));
}

/**
 * @brief Access selected endpoint state structure.
 *
 * Function used to change or just read the state of selected endpoint.
 * It is used for internal transmission state.
 *
 * @param ep Endpoint number.
 */
static inline usbd_ep_state_t* ep_state_access(nrfx_usbd_ep_t ep)
{
    NRFX_USBD_ASSERT_EP_VALID(ep);
    return ((NRF_USBD_EPIN_CHECK(ep) ? m_ep_state.ep_in : m_ep_state.ep_out) +
        NRF_USBD_EP_NR_GET(ep));
}

/**
 * @brief Change endpoint number to bit position.
 *
 * Bit positions are defined the same way as they are placed in DATAEPSTATUS register,
 * but bits for endpoint 0 are included.
 *
 * @param ep Endpoint number.
 *
 * @return Bit position related to the given endpoint number.
 *
 * @sa bit2ep
 */
static inline uint8_t ep2bit(nrfx_usbd_ep_t ep)
{
    NRFX_USBD_ASSERT_EP_VALID(ep);
    return NRFX_USBD_EP_BITPOS(ep);
}

/**
 * @brief Change bit position to endpoint number.
 *
 * @param bitpos Bit position.
 *
 * @return Endpoint number corresponding to given bit position.
 *
 * @sa ep2bit
 */
static inline nrfx_usbd_ep_t bit2ep(uint8_t bitpos)
{
    NRFX_STATIC_ASSERT(NRFX_USBD_EPOUT_BITPOS_0 > NRFX_USBD_EPIN_BITPOS_0);
    return (nrfx_usbd_ep_t)((bitpos >= NRFX_USBD_EPOUT_BITPOS_0) ?
        NRF_USBD_EPOUT(bitpos - NRFX_USBD_EPOUT_BITPOS_0) : NRF_USBD_EPIN(bitpos));
}

/**
 * @brief Mark that EasyDMA is working.
 *
 * Internal function to set the flag informing about EasyDMA transfer pending.
 * This function is called always just after the EasyDMA transfer is started.
 */
static inline void usbd_dma_pending_set(void)
{
    if (nrfx_usbd_errata_199())
    {
        *((volatile uint32_t *)0x40027C1C) = 0x00000082;
    }
    m_dma_pending = true;
}

/**
 * @brief Mark that EasyDMA is free.
 *
 * Internal function to clear the flag informing about EasyDMA transfer pending.
 * This function is called always just after the finished EasyDMA transfer is detected.
 */
static inline void usbd_dma_pending_clear(void)
{
    if (nrfx_usbd_errata_199())
    {
        *((volatile uint32_t *)0x40027C1C) = 0x00000000;
    }
    m_dma_pending = false;
}

/**
 * @brief Start selected EasyDMA transmission.
 *
 * This is internal auxiliary function.
 * No checking is made if EasyDMA is ready for new transmission.
 *
 * @param[in] ep Number of endpoint for transmission.
 *               If it is OUT endpoint transmission would be directed from endpoint to RAM.
 *               If it is in endpoint transmission would be directed from RAM to endpoint.
 */
static inline void usbd_dma_start(nrfx_usbd_ep_t ep)
{
    nrf_usbd_task_trigger(task_start_ep(ep));
}

void nrfx_usbd_isoinconfig_set(nrf_usbd_isoinconfig_t config)
{
    nrf_usbd_isoinconfig_set(config);
}

nrf_usbd_isoinconfig_t nrfx_usbd_isoinconfig_get(void)
{
    return nrf_usbd_isoinconfig_get();
}

/**
 * @brief Abort pending transfer on selected endpoint.
 *
 * @param ep Endpoint number.
 *
 * @note
 * This function locks interrupts that may be costly.
 * It is good idea to test if the endpoint is still busy before calling this function:
 * @code
   (m_ep_dma_waiting & (1U << ep2bit(ep)))
 * @endcode
 * This function would check it again, but it makes it inside critical section.
 */
static inline void usbd_ep_abort(nrfx_usbd_ep_t ep)
{
    NRFX_CRITICAL_SECTION_ENTER();

    usbd_ep_state_t * p_state = ep_state_access(ep);

    if (NRF_USBD_EPOUT_CHECK(ep))
    {
        /* Host -> Device */
        if ((~m_ep_dma_waiting) & (1U << ep2bit(ep)))
        {
            /* If the bit in m_ep_dma_waiting in cleared - nothing would be
             * processed inside transfer processing */
            nrfx_usbd_transfer_out_drop(ep);
        }
        else
        {
            p_state->handler.consumer = NULL;
            m_ep_dma_waiting &= ~(1U << ep2bit(ep));
            m_ep_ready &= ~(1U << ep2bit(ep));
        }
        /* Aborted */
        p_state->status = NRFX_USBD_EP_ABORTED;
    }
    else
    {
        if(!NRF_USBD_EPISO_CHECK(ep))
        {
            /* Workaround: Disarm the endpoint if there is any data buffered. */
            if(ep != NRFX_USBD_EPIN0)
            {
                *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7B6 + (2u * (NRF_USBD_EP_NR_GET(ep) - 1));
                uint8_t temp = *((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
                temp |= (1U << 1);
                *((volatile uint32_t *)(NRF_USBD_BASE + 0x804)) |= temp;
                (void)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
            }
            else
            {
                *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7B4;
                uint8_t temp = *((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
                temp |= (1U << 2);
                *((volatile uint32_t *)(NRF_USBD_BASE + 0x804)) |= temp;
                (void)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
            }
        }
        if ((m_ep_dma_waiting | (~m_ep_ready)) & (1U << ep2bit(ep)))
        {
            /* Device -> Host */
            m_ep_dma_waiting &= ~(1U << ep2bit(ep));
            m_ep_ready       |=   1U << ep2bit(ep) ;

            p_state->handler.feeder = NULL;
            p_state->status = NRFX_USBD_EP_ABORTED;
            NRFX_USBD_EP_TRANSFER_EVENT(evt, ep, NRFX_USBD_EP_ABORTED);
            m_event_handler(&evt);
        }
    }
    NRFX_CRITICAL_SECTION_EXIT();
}

void nrfx_usbd_ep_abort(nrfx_usbd_ep_t ep)
{
    usbd_ep_abort(ep);
}


/**
 * @brief Abort all pending endpoints.
 *
 * Function aborts all pending endpoint transfers.
 */
static void usbd_ep_abort_all(void)
{
    uint32_t ep_waiting = m_ep_dma_waiting | (m_ep_ready & NRFX_USBD_EPOUT_BIT_MASK);
    while (0 != ep_waiting)
    {
        uint8_t bitpos = __CLZ(__RBIT(ep_waiting));
        if (!NRF_USBD_EPISO_CHECK(bit2ep(bitpos)))
        {
            usbd_ep_abort(bit2ep(bitpos));
        }
        ep_waiting &= ~(1U << bitpos);
    }

    m_ep_ready = (((1U << NRF_USBD_EPIN_CNT) - 1U) << NRFX_USBD_EPIN_BITPOS_0);
}

/**
 * @brief Force the USBD interrupt into pending state.
 *
 * This function is used to force USBD interrupt to be processed right now.
 * It makes it possible to process all EasyDMA access on one thread priority level.
 */
static inline void usbd_int_rise(void)
{
    NRFX_IRQ_PENDING_SET(USBD_IRQn);
}

/**
 * @name USBD interrupt runtimes.
 *
 * Interrupt runtimes that would be vectorized using @ref m_isr.
 * @{
 */

static void ev_usbreset_handler(void)
{
    m_bus_suspend = false;
    m_last_setup_dir = NRFX_USBD_EPOUT0;

    const nrfx_usbd_evt_t evt = {
            .type = NRFX_USBD_EVT_RESET
    };

    m_event_handler(&evt);
}

static void ev_started_handler(void)
{
#if NRFX_USBD_STARTED_EV_ENABLE
    // Handler not used by the stack.
    // May be used for debugging.
#endif
}

/**
 * @brief Handler for EasyDMA event without endpoint clearing.
 *
 * This handler would be called when EasyDMA transfer for endpoints that does not require clearing.
 * All in endpoints are cleared automatically when new EasyDMA transfer is initialized.
 * For endpoint 0 see @ref nrf_usbd_ep0out_dma_handler.
 *
 * @param[in] ep Endpoint number.
 */
static inline void nrf_usbd_ep0in_dma_handler(void)
{
    const nrfx_usbd_ep_t ep = NRFX_USBD_EPIN0;
    NRFX_LOG_DEBUG("USB event: DMA ready IN0");
    usbd_dma_pending_clear();

    usbd_ep_state_t * p_state = ep_state_access(ep);
    if (NRFX_USBD_EP_ABORTED == p_state->status)
    {
        /* Clear transfer information just in case */
        (void)(NRFX_ATOMIC_FETCH_AND(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
    }
    else if (p_state->handler.feeder == NULL)
    {
        (void)(NRFX_ATOMIC_FETCH_AND(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
    }
    else
    {
        /* Nothing to do */
    }
}

/**
 * @brief Handler for EasyDMA event without endpoint clearing.
 *
 * This handler would be called when EasyDMA transfer for endpoints that does not require clearing.
 * All in endpoints are cleared automatically when new EasyDMA transfer is initialized.
 * For endpoint 0 see @ref nrf_usbd_ep0out_dma_handler.
 *
 * @param[in] ep Endpoint number.
 */
static inline void nrf_usbd_epin_dma_handler(nrfx_usbd_ep_t ep)
{
    NRFX_LOG_DEBUG("USB event: DMA ready IN: %x", ep);
    NRFX_ASSERT(NRF_USBD_EPIN_CHECK(ep));
    NRFX_ASSERT(!NRF_USBD_EPISO_CHECK(ep));
    NRFX_ASSERT(NRF_USBD_EP_NR_GET(ep) > 0);
    usbd_dma_pending_clear();

    usbd_ep_state_t * p_state = ep_state_access(ep);
    if (NRFX_USBD_EP_ABORTED == p_state->status)
    {
        /* Clear transfer information just in case */
        (void)(NRFX_ATOMIC_FETCH_AND(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
    }
    else if (p_state->handler.feeder == NULL)
    {
        (void)(NRFX_ATOMIC_FETCH_AND(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
    }
    else
    {
        /* Nothing to do */
    }
}

/**
 * @brief Handler for EasyDMA event from in isochronous endpoint.
 */
static inline void nrf_usbd_epiniso_dma_handler(nrfx_usbd_ep_t ep)
{
    if (NRFX_USBD_ISO_DEBUG)
    {
        NRFX_LOG_DEBUG("USB event: DMA ready ISOIN: %x", ep);
    }
    NRFX_ASSERT(NRF_USBD_EPIN_CHECK(ep));
    NRFX_ASSERT(NRF_USBD_EPISO_CHECK(ep));
    usbd_dma_pending_clear();

    usbd_ep_state_t * p_state = ep_state_access(ep);
    if (NRFX_USBD_EP_ABORTED == p_state->status)
    {
        /* Clear transfer information just in case */
        (void)(NRFX_ATOMIC_FETCH_AND(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
    }
    else if (p_state->handler.feeder == NULL)
    {
        (void)(NRFX_ATOMIC_FETCH_AND(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
        /* Send event to the user - for an ISO IN endpoint, the whole transfer is finished in this moment */
        NRFX_USBD_EP_TRANSFER_EVENT(evt, ep, NRFX_USBD_EP_OK);
        m_event_handler(&evt);
    }
    else
    {
        /* Nothing to do */
    }
}

/**
 * @brief Handler for EasyDMA event for OUT endpoint 0.
 *
 * EP0 OUT have to be cleared automatically in special way - only in the middle of the transfer.
 * It cannot be cleared when required transfer is finished because it means the same that accepting the comment.
 */
static inline void nrf_usbd_ep0out_dma_handler(void)
{
    const nrfx_usbd_ep_t ep = NRFX_USBD_EPOUT0;
    NRFX_LOG_DEBUG("USB event: DMA ready OUT0");
    usbd_dma_pending_clear();

    usbd_ep_state_t * p_state = ep_state_access(ep);
    if (NRFX_USBD_EP_ABORTED == p_state->status)
    {
        /* Clear transfer information just in case */
        (void)(NRFX_ATOMIC_FETCH_AND(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
    }
    else if (p_state->handler.consumer == NULL)
    {
        (void)(NRFX_ATOMIC_FETCH_AND(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
        /* Send event to the user - for an OUT endpoint, the whole transfer is finished in this moment */
        NRFX_USBD_EP_TRANSFER_EVENT(evt, ep, NRFX_USBD_EP_OK);
        m_event_handler(&evt);
    }
    else
    {
        nrfx_usbd_setup_data_clear();
    }
}

/**
 * @brief Handler for EasyDMA event from endpoinpoint that requires clearing.
 *
 * This handler would be called when EasyDMA transfer for OUT endpoint has been finished.
 *
 * @param[in] ep Endpoint number.
 */
static inline void nrf_usbd_epout_dma_handler(nrfx_usbd_ep_t ep)
{
    NRFX_LOG_DEBUG("DMA ready OUT: %x", ep);
    NRFX_ASSERT(NRF_USBD_EPOUT_CHECK(ep));
    NRFX_ASSERT(!NRF_USBD_EPISO_CHECK(ep));
    NRFX_ASSERT(NRF_USBD_EP_NR_GET(ep) > 0);
    usbd_dma_pending_clear();

    usbd_ep_state_t * p_state = ep_state_access(ep);
    if (NRFX_USBD_EP_ABORTED == p_state->status)
    {
        /* Clear transfer information just in case */
        (void)(NRFX_ATOMIC_FETCH_AND(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
    }
    else if (p_state->handler.consumer == NULL)
    {
        (void)(NRFX_ATOMIC_FETCH_AND(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
        /* Send event to the user - for an OUT endpoint, the whole transfer is finished in this moment */
        NRFX_USBD_EP_TRANSFER_EVENT(evt, ep, NRFX_USBD_EP_OK);
        m_event_handler(&evt);
    }
    else
    {
        /* Nothing to do */
    }

#if NRFX_USBD_EARLY_DMA_PROCESS
    /* Speed up */
    usbd_dmareq_process();
#endif
}

/**
 * @brief Handler for EasyDMA event from out isochronous endpoint.
 */
static inline void nrf_usbd_epoutiso_dma_handler(nrfx_usbd_ep_t ep)
{
    if (NRFX_USBD_ISO_DEBUG)
    {
        NRFX_LOG_DEBUG("DMA ready ISOOUT: %x", ep);
    }
    NRFX_ASSERT(NRF_USBD_EPISO_CHECK(ep));
    usbd_dma_pending_clear();

    usbd_ep_state_t * p_state = ep_state_access(ep);
    if (NRFX_USBD_EP_ABORTED == p_state->status)
    {
        /* Nothing to do - just ignore */
    }
    else if (p_state->handler.consumer == NULL)
    {
        (void)(NRFX_ATOMIC_FETCH_AND(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
        /* Send event to the user - for an OUT endpoint, the whole transfer is finished in this moment */
        NRFX_USBD_EP_TRANSFER_EVENT(evt, ep, NRFX_USBD_EP_OK);
        m_event_handler(&evt);
    }
    else
    {
        /* Nothing to do */
    }
}


static void ev_dma_epin0_handler(void)  { nrf_usbd_ep0in_dma_handler(); }
static void ev_dma_epin1_handler(void)  { nrf_usbd_epin_dma_handler(NRFX_USBD_EPIN1 ); }
static void ev_dma_epin2_handler(void)  { nrf_usbd_epin_dma_handler(NRFX_USBD_EPIN2 ); }
static void ev_dma_epin3_handler(void)  { nrf_usbd_epin_dma_handler(NRFX_USBD_EPIN3 ); }
static void ev_dma_epin4_handler(void)  { nrf_usbd_epin_dma_handler(NRFX_USBD_EPIN4 ); }
static void ev_dma_epin5_handler(void)  { nrf_usbd_epin_dma_handler(NRFX_USBD_EPIN5 ); }
static void ev_dma_epin6_handler(void)  { nrf_usbd_epin_dma_handler(NRFX_USBD_EPIN6 ); }
static void ev_dma_epin7_handler(void)  { nrf_usbd_epin_dma_handler(NRFX_USBD_EPIN7 ); }
static void ev_dma_epin8_handler(void)  { nrf_usbd_epiniso_dma_handler(NRFX_USBD_EPIN8 ); }

static void ev_dma_epout0_handler(void) { nrf_usbd_ep0out_dma_handler(); }
static void ev_dma_epout1_handler(void) { nrf_usbd_epout_dma_handler(NRFX_USBD_EPOUT1); }
static void ev_dma_epout2_handler(void) { nrf_usbd_epout_dma_handler(NRFX_USBD_EPOUT2); }
static void ev_dma_epout3_handler(void) { nrf_usbd_epout_dma_handler(NRFX_USBD_EPOUT3); }
static void ev_dma_epout4_handler(void) { nrf_usbd_epout_dma_handler(NRFX_USBD_EPOUT4); }
static void ev_dma_epout5_handler(void) { nrf_usbd_epout_dma_handler(NRFX_USBD_EPOUT5); }
static void ev_dma_epout6_handler(void) { nrf_usbd_epout_dma_handler(NRFX_USBD_EPOUT6); }
static void ev_dma_epout7_handler(void) { nrf_usbd_epout_dma_handler(NRFX_USBD_EPOUT7); }
static void ev_dma_epout8_handler(void) { nrf_usbd_epoutiso_dma_handler(NRFX_USBD_EPOUT8); }

static void ev_sof_handler(void)
{
    nrfx_usbd_evt_t evt =  {
            NRFX_USBD_EVT_SOF,
            .data = { .sof = { .framecnt = nrf_usbd_framecntr_get() }}
    };

    /* Process isochronous endpoints */
    uint32_t iso_ready_mask = (1U << ep2bit(NRFX_USBD_EPIN8));
    if (nrf_usbd_episoout_size_get(NRFX_USBD_EPOUT8) != NRF_USBD_EPISOOUT_NO_DATA)
    {
        iso_ready_mask |= (1U << ep2bit(NRFX_USBD_EPOUT8));
    }
    m_ep_ready |= iso_ready_mask;

    m_event_handler(&evt);
}

/**
 * @brief React on data transfer finished.
 *
 * Auxiliary internal function.
 * @param ep     Endpoint number.
 * @param bitpos Bit position for selected endpoint number.
 */
static void usbd_ep_data_handler(nrfx_usbd_ep_t ep, uint8_t bitpos)
{
    NRFX_LOG_DEBUG("USBD event: EndpointData: %x", ep);
    /* Mark endpoint ready for next DMA access */
    m_ep_ready |= (1U << bitpos);

    if (NRF_USBD_EPIN_CHECK(ep))
    {
        /* IN endpoint (Device -> Host) */

        /* Secure against the race condition that occurs when an IN transfer is interrupted
         * by an OUT transaction, which in turn is interrupted by a process with higher priority.
         * If the IN events ENDEPIN and EPDATA arrive during that high priority process,
         * the OUT handler might call usbd_ep_data_handler without calling 
         * nrf_usbd_epin_dma_handler (or nrf_usbd_ep0in_dma_handler) for the IN transaction.
         */
        if (nrf_usbd_event_get_and_clear(nrfx_usbd_ep_to_endevent(ep)))
        {
            if (ep != NRFX_USBD_EPIN0)
            {
                nrf_usbd_epin_dma_handler(ep);
            }
            else
            {
                nrf_usbd_ep0in_dma_handler();
            }
        }

        if (0 == (m_ep_dma_waiting & (1U << bitpos)))
        {
            NRFX_LOG_DEBUG("USBD event: EndpointData: In finished");
            /* No more data to be send - transmission finished */
            NRFX_USBD_EP_TRANSFER_EVENT(evt, ep, NRFX_USBD_EP_OK);
            m_event_handler(&evt);
        }
    }
    else
    {
        /* OUT endpoint (Host -> Device) */
        if (0 == (m_ep_dma_waiting & (1U << bitpos)))
        {
            NRFX_LOG_DEBUG("USBD event: EndpointData: Out waiting");
            /* No buffer prepared - send event to the application */
            NRFX_USBD_EP_TRANSFER_EVENT(evt, ep, NRFX_USBD_EP_WAITING);
            m_event_handler(&evt);
        }
    }
}

static void ev_setup_data_handler(void)
{
    usbd_ep_data_handler(m_last_setup_dir, ep2bit(m_last_setup_dir));
}

static void ev_setup_handler(void)
{
    NRFX_LOG_DEBUG("USBD event: Setup (rt:%.2x r:%.2x v:%.4x i:%.4x l:%u )",
        nrf_usbd_setup_bmrequesttype_get(),
        nrf_usbd_setup_brequest_get(),
        nrf_usbd_setup_wvalue_get(),
        nrf_usbd_setup_windex_get(),
        nrf_usbd_setup_wlength_get());
    uint8_t bmRequestType = nrf_usbd_setup_bmrequesttype_get();

    if ((m_ep_dma_waiting | ((~m_ep_ready) & NRFX_USBD_EPIN_BIT_MASK))
        & (1U <<ep2bit(m_last_setup_dir)))
    {
        NRFX_LOG_DEBUG("USBD drv: Trying to abort last transfer on EP0");
        usbd_ep_abort(m_last_setup_dir);
    }

    m_last_setup_dir =
        ((bmRequestType & USBD_BMREQUESTTYPE_DIRECTION_Msk) == 
         (USBD_BMREQUESTTYPE_DIRECTION_HostToDevice << USBD_BMREQUESTTYPE_DIRECTION_Pos)) ?
        NRFX_USBD_EPOUT0 : NRFX_USBD_EPIN0;

    (void)(NRFX_ATOMIC_FETCH_AND(
        &m_ep_dma_waiting,
        ~((1U << ep2bit(NRFX_USBD_EPOUT0)) | (1U << ep2bit(NRFX_USBD_EPIN0)))));
    m_ep_ready |= 1U << ep2bit(NRFX_USBD_EPIN0);


    const nrfx_usbd_evt_t evt = {
            .type = NRFX_USBD_EVT_SETUP
    };
    m_event_handler(&evt);
}

static void ev_usbevent_handler(void)
{
    uint32_t event = nrf_usbd_eventcause_get_and_clear();

    if (event & NRF_USBD_EVENTCAUSE_ISOOUTCRC_MASK)
    {
        NRFX_LOG_DEBUG("USBD event: ISOOUTCRC");
        /* Currently no support */
    }
    if (event & NRF_USBD_EVENTCAUSE_SUSPEND_MASK)
    {
        NRFX_LOG_DEBUG("USBD event: SUSPEND");
        m_bus_suspend = true;
        const nrfx_usbd_evt_t evt = {
                .type = NRFX_USBD_EVT_SUSPEND
        };
        m_event_handler(&evt);
    }
    if (event & NRF_USBD_EVENTCAUSE_RESUME_MASK)
    {
        NRFX_LOG_DEBUG("USBD event: RESUME");
        m_bus_suspend = false;
        const nrfx_usbd_evt_t evt = {
                .type = NRFX_USBD_EVT_RESUME
        };
        m_event_handler(&evt);
    }
    if (event & NRF_USBD_EVENTCAUSE_WUREQ_MASK)
    {
        NRFX_LOG_DEBUG("USBD event: WUREQ (%s)", m_bus_suspend ? "In Suspend" : "Active");
        if (m_bus_suspend)
        {
            NRFX_ASSERT(!nrf_usbd_lowpower_check());
            m_bus_suspend = false;

            nrf_usbd_dpdmvalue_set(NRF_USBD_DPDMVALUE_RESUME);
            nrf_usbd_task_trigger(NRF_USBD_TASK_DRIVEDPDM);

            const nrfx_usbd_evt_t evt = {
                    .type = NRFX_USBD_EVT_WUREQ
            };
            m_event_handler(&evt);
        }
    }
}

static void ev_epdata_handler(void)
{
    /* Get all endpoints that have acknowledged transfer */
    uint32_t dataepstatus = nrf_usbd_epdatastatus_get_and_clear();
    if (nrfx_usbd_errata_104())
    {
        dataepstatus |= (m_simulated_dataepstatus &
            ~((1U << NRFX_USBD_EPOUT_BITPOS_0) | (1U << NRFX_USBD_EPIN_BITPOS_0)));
        m_simulated_dataepstatus &=
             ((1U << NRFX_USBD_EPOUT_BITPOS_0) | (1U << NRFX_USBD_EPIN_BITPOS_0));
    }
    NRFX_LOG_DEBUG("USBD event: EndpointEPStatus: %x", dataepstatus);

    /* All finished endpoint have to be marked as busy */
    while (dataepstatus)
    {
        uint8_t bitpos    = __CLZ(__RBIT(dataepstatus));
        nrfx_usbd_ep_t ep = bit2ep(bitpos);
        dataepstatus &= ~(1UL << bitpos);

        (void)(usbd_ep_data_handler(ep, bitpos));
    }
    if (NRFX_USBD_EARLY_DMA_PROCESS)
    {
        /* Speed up */
        usbd_dmareq_process();
    }
}

/**
 * @brief Function to select the endpoint to start.
 *
 * Function that realizes algorithm to schedule right channel for EasyDMA transfer.
 * It gets a variable with flags for the endpoints currently requiring transfer.
 *
 * @param[in] req Bit flags for channels currently requiring transfer.
 *                Bits 0...8 used for IN endpoints.
 *                Bits 16...24 used for OUT endpoints.
 * @note
 * This function would be never called with 0 as a @c req argument.
 * @return The bit number of the endpoint that should be processed now.
 */
static uint8_t usbd_dma_scheduler_algorithm(uint32_t req)
{
    /* Only prioritized scheduling mode is supported. */
    return __CLZ(__RBIT(req));
}

/**
 * @brief Get the size of isochronous endpoint.
 *
 * The size of isochronous endpoint is configurable.
 * This function returns the size of isochronous buffer taking into account
 * current configuration.
 *
 * @param[in] ep Endpoint number.
 *
 * @return The size of endpoint buffer.
 */
static inline size_t usbd_ep_iso_capacity(nrfx_usbd_ep_t ep)
{
    (void)ep;
    nrf_usbd_isosplit_t split = nrf_usbd_isosplit_get();
    if (NRF_USBD_ISOSPLIT_HALF == split)
    {
        return NRFX_USBD_ISOSIZE / 2;
    }
    return NRFX_USBD_ISOSIZE;
}

/**
 * @brief Process all DMA requests.
 *
 * Function that have to be called from USBD interrupt handler.
 * It have to be called when all the interrupts connected with endpoints transfer
 * and DMA transfer are already handled.
 */
static void usbd_dmareq_process(void)
{
    if (!m_dma_pending)
    {
        uint32_t req;
        while (0 != (req = m_ep_dma_waiting & m_ep_ready))
        {
            uint8_t pos;
            if (NRFX_USBD_CONFIG_DMASCHEDULER_ISO_BOOST && ((req & USBD_EPISO_BIT_MASK) != 0))
            {
                pos = usbd_dma_scheduler_algorithm(req & USBD_EPISO_BIT_MASK);
            }
            else
            {
                pos = usbd_dma_scheduler_algorithm(req);
            }
            nrfx_usbd_ep_t ep = bit2ep(pos);
            usbd_ep_state_t * p_state = ep_state_access(ep);

            nrfx_usbd_ep_transfer_t transfer;
            bool continue_transfer;

            NRFX_STATIC_ASSERT(offsetof(usbd_ep_state_t, handler.feeder) ==
                offsetof(usbd_ep_state_t, handler.consumer));
            NRFX_ASSERT((p_state->handler.feeder) != NULL);

            if (NRF_USBD_EPIN_CHECK(ep))
            {
                /* Device -> Host */
                continue_transfer = p_state->handler.feeder(
                    &transfer,
                    p_state->p_context,
                    p_state->max_packet_size);

                if (!continue_transfer)
                {
                    p_state->handler.feeder = NULL;
                }
            }
            else
            {
                /* Host -> Device */
                const size_t rx_size = nrfx_usbd_epout_size_get(ep);
                continue_transfer = p_state->handler.consumer(
                    &transfer,
                    p_state->p_context,
                    p_state->max_packet_size,
                    rx_size);

                if (transfer.p_data.rx == NULL)
                {
                    /* Dropping transfer - allow processing */
                    NRFX_ASSERT(transfer.size == 0);
                }
                else if (transfer.size < rx_size)
                {
                    NRFX_LOG_DEBUG("Endpoint %x overload (r: %u, e: %u)", ep, rx_size, transfer.size);
                    p_state->status = NRFX_USBD_EP_OVERLOAD;
                    (void)(NRFX_ATOMIC_FETCH_AND(&m_ep_dma_waiting, ~(1U << pos)));
                    NRFX_USBD_EP_TRANSFER_EVENT(evt, ep, NRFX_USBD_EP_OVERLOAD);
                    m_event_handler(&evt);
                    /* This endpoint will not be transmitted now, repeat the loop */
                    continue;
                }
                else
                {
                    /* Nothing to do - only check integrity if assertions are enabled */
                    NRFX_ASSERT(transfer.size == rx_size);
                }

                if (!continue_transfer)
                {
                    p_state->handler.consumer = NULL;
                }
            }

            usbd_dma_pending_set();
            m_ep_ready &= ~(1U << pos);
            if (NRFX_USBD_ISO_DEBUG || (!NRF_USBD_EPISO_CHECK(ep)))
            {
                NRFX_LOG_DEBUG(
                    "USB DMA process: Starting transfer on EP: %x, size: %u",
                    ep,
                    transfer.size);
            }
            /* Update number of currently transferred bytes */
            p_state->transfer_cnt += transfer.size;
            /* Start transfer to the endpoint buffer */
            nrf_usbd_ep_easydma_set(ep, transfer.p_data.addr, (uint32_t)transfer.size);

            if (nrfx_usbd_errata_104())
            {
                uint32_t cnt_end = (uint32_t)(-1);
                do
                {
                    uint32_t cnt = (uint32_t)(-1);
                    do
                    {
                        nrf_usbd_event_clear(NRF_USBD_EVENT_STARTED);
                        usbd_dma_start(ep);
                        nrfx_systick_delay_us(2);
                        ++cnt;
                    }while (!nrf_usbd_event_check(NRF_USBD_EVENT_STARTED));
                    if (cnt)
                    {
                        NRFX_USBD_LOG_PROTO1_FIX_PRINTF("   DMA restarted: %u times", cnt);
                    }

                    nrfx_systick_delay_us(30);
                    while (0 == (0x20 & *((volatile uint32_t *)(NRF_USBD_BASE + 0x474))))
                    {
                        nrfx_systick_delay_us(2);
                    }
                    nrfx_systick_delay_us(1);

                    ++cnt_end;
                } while (!nrf_usbd_event_check(nrfx_usbd_ep_to_endevent(ep)));
                if (cnt_end)
                {
                    NRFX_USBD_LOG_PROTO1_FIX_PRINTF("   DMA fully restarted: %u times", cnt_end);
                }
            }
            else
            {
                usbd_dma_start(ep);
                /* There is a lot of USBD registers that cannot be accessed during EasyDMA transfer.
                 * This is quick fix to maintain stability of the stack.
                 * It cost some performance but makes stack stable. */
                while (!nrf_usbd_event_check(nrfx_usbd_ep_to_endevent(ep)))
                {
                    /* Empty */
                }
            }

            if (NRFX_USBD_DMAREQ_PROCESS_DEBUG)
            {
                NRFX_LOG_DEBUG("USB DMA process - finishing");
            }
            /* Transfer started - exit the loop */
            break;
        }
    }
    else
    {
        if (NRFX_USBD_DMAREQ_PROCESS_DEBUG)
        {
            NRFX_LOG_DEBUG("USB DMA process - EasyDMA busy");
        }
    }
}
/** @} */

/**
 * @brief USBD interrupt service routines.
 *
 */
static const nrfx_irq_handler_t m_isr[] =
{
    [USBD_INTEN_USBRESET_Pos   ] = ev_usbreset_handler,
    [USBD_INTEN_STARTED_Pos    ] = ev_started_handler,
    [USBD_INTEN_ENDEPIN0_Pos   ] = ev_dma_epin0_handler,
    [USBD_INTEN_ENDEPIN1_Pos   ] = ev_dma_epin1_handler,
    [USBD_INTEN_ENDEPIN2_Pos   ] = ev_dma_epin2_handler,
    [USBD_INTEN_ENDEPIN3_Pos   ] = ev_dma_epin3_handler,
    [USBD_INTEN_ENDEPIN4_Pos   ] = ev_dma_epin4_handler,
    [USBD_INTEN_ENDEPIN5_Pos   ] = ev_dma_epin5_handler,
    [USBD_INTEN_ENDEPIN6_Pos   ] = ev_dma_epin6_handler,
    [USBD_INTEN_ENDEPIN7_Pos   ] = ev_dma_epin7_handler,
    [USBD_INTEN_EP0DATADONE_Pos] = ev_setup_data_handler,
    [USBD_INTEN_ENDISOIN_Pos   ] = ev_dma_epin8_handler,
    [USBD_INTEN_ENDEPOUT0_Pos  ] = ev_dma_epout0_handler,
    [USBD_INTEN_ENDEPOUT1_Pos  ] = ev_dma_epout1_handler,
    [USBD_INTEN_ENDEPOUT2_Pos  ] = ev_dma_epout2_handler,
    [USBD_INTEN_ENDEPOUT3_Pos  ] = ev_dma_epout3_handler,
    [USBD_INTEN_ENDEPOUT4_Pos  ] = ev_dma_epout4_handler,
    [USBD_INTEN_ENDEPOUT5_Pos  ] = ev_dma_epout5_handler,
    [USBD_INTEN_ENDEPOUT6_Pos  ] = ev_dma_epout6_handler,
    [USBD_INTEN_ENDEPOUT7_Pos  ] = ev_dma_epout7_handler,
    [USBD_INTEN_ENDISOOUT_Pos  ] = ev_dma_epout8_handler,
    [USBD_INTEN_SOF_Pos        ] = ev_sof_handler,
    [USBD_INTEN_USBEVENT_Pos   ] = ev_usbevent_handler,
    [USBD_INTEN_EP0SETUP_Pos   ] = ev_setup_handler,
    [USBD_INTEN_EPDATA_Pos     ] = ev_epdata_handler
};

/**
 * @name Interrupt handlers
 *
 * @{
 */
void nrfx_usbd_irq_handler(void)
{
    const uint32_t enabled = nrf_usbd_int_enable_get();
    uint32_t to_process = enabled;
    uint32_t active = 0;

    /* Check all enabled interrupts */
    while (to_process)
    {
        uint8_t event_nr = __CLZ(__RBIT(to_process));
        if (nrf_usbd_event_get_and_clear((nrf_usbd_event_t)nrfx_bitpos_to_event(event_nr)))
        {
            active |= 1UL << event_nr;
        }
        to_process &= ~(1UL << event_nr);
    }

    if (nrfx_usbd_errata_104())
    {
        /* Event correcting */
        if ((!m_dma_pending) && (0 != (active & (USBD_INTEN_SOF_Msk))))
        {
            uint8_t usbi, uoi, uii;
            /* Testing */
            *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7A9;
            uii = (uint8_t)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
            if (0 != uii)
            {
                uii &= (uint8_t)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
            }

            *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AA;
            uoi = (uint8_t)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
            if (0 != uoi)
            {
                uoi &= (uint8_t)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
            }
            *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AB;
            usbi = (uint8_t)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
            if (0 != usbi)
            {
                usbi &= (uint8_t)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
            }
            /* Processing */
            *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AC;
            uii &= (uint8_t)*((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
            if (0 != uii)
            {
                uint8_t rb;
                m_simulated_dataepstatus |= ((uint32_t)uii) << NRFX_USBD_EPIN_BITPOS_0;
                *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7A9;
                *((volatile uint32_t *)(NRF_USBD_BASE + 0x804)) = uii;
                rb = (uint8_t)*((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
                NRFX_USBD_LOG_PROTO1_FIX_PRINTF("   uii: 0x%.2x (0x%.2x)", uii, rb);
                (void)rb;
            }

            *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AD;
            uoi &= (uint8_t)*((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
            if (0 != uoi)
            {
                uint8_t rb;
                m_simulated_dataepstatus |= ((uint32_t)uoi) << NRFX_USBD_EPOUT_BITPOS_0;
                *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AA;
                *((volatile uint32_t *)(NRF_USBD_BASE + 0x804)) = uoi;
                rb = (uint8_t)*((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
                NRFX_USBD_LOG_PROTO1_FIX_PRINTF("   uoi: 0x%.2u (0x%.2x)", uoi, rb);
                (void)rb;
            }

            *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AE;
            usbi &= (uint8_t)*((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
            if (0 != usbi)
            {
                uint8_t rb;
                if (usbi & 0x01)
                {
                    active |= USBD_INTEN_EP0SETUP_Msk;
                }
                if (usbi & 0x10)
                {
                    active |= USBD_INTEN_USBRESET_Msk;
                }
                *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7AB;
                *((volatile uint32_t *)(NRF_USBD_BASE + 0x804)) = usbi;
                rb = (uint8_t)*((volatile uint32_t *)(NRF_USBD_BASE + 0x804));
                NRFX_USBD_LOG_PROTO1_FIX_PRINTF("   usbi: 0x%.2u (0x%.2x)", usbi, rb);
                (void)rb;
            }

            if (0 != (m_simulated_dataepstatus &
                ~((1U << NRFX_USBD_EPOUT_BITPOS_0) | (1U << NRFX_USBD_EPIN_BITPOS_0))))
            {
                active |= enabled & NRF_USBD_INT_DATAEP_MASK;
            }
            if (0 != (m_simulated_dataepstatus &
                ((1U << NRFX_USBD_EPOUT_BITPOS_0) | (1U << NRFX_USBD_EPIN_BITPOS_0))))
            {
                if (0 != (enabled & NRF_USBD_INT_EP0DATADONE_MASK))
                {
                    m_simulated_dataepstatus &=
                        ~((1U << NRFX_USBD_EPOUT_BITPOS_0) | (1U << NRFX_USBD_EPIN_BITPOS_0));
                    active |= NRF_USBD_INT_EP0DATADONE_MASK;
                }
            }
        }
    }

    /* Process the active interrupts */
    bool setup_active = 0 != (active & NRF_USBD_INT_EP0SETUP_MASK);
    active &= ~NRF_USBD_INT_EP0SETUP_MASK;

    while (active)
    {
        uint8_t event_nr = __CLZ(__RBIT(active));
        m_isr[event_nr]();
        active &= ~(1UL << event_nr);
    }
    usbd_dmareq_process();

    if (setup_active)
    {
        m_isr[USBD_INTEN_EP0SETUP_Pos]();
    }
}

/** @} */
/** @} */

nrfx_err_t nrfx_usbd_init(nrfx_usbd_event_handler_t event_handler)
{
    NRFX_ASSERT(event_handler);

    if (m_drv_state != NRFX_DRV_STATE_UNINITIALIZED)
    {
        return NRFX_ERROR_INVALID_STATE;
    }

    m_event_handler = event_handler;
    m_drv_state = NRFX_DRV_STATE_INITIALIZED;

    uint8_t n;
    for (n = 0; n < NRF_USBD_EPIN_CNT; ++n)
    {
        nrfx_usbd_ep_t ep = NRFX_USBD_EPIN(n);
        nrfx_usbd_ep_max_packet_size_set(ep, NRF_USBD_EPISO_CHECK(ep) ?
            (NRFX_USBD_ISOSIZE / 2) : NRFX_USBD_EPSIZE);
        usbd_ep_state_t * p_state = ep_state_access(ep);
        p_state->status = NRFX_USBD_EP_OK;
        p_state->handler.feeder = NULL;
        p_state->transfer_cnt = 0;
    }
    for (n = 0; n < NRF_USBD_EPOUT_CNT; ++n)
    {
        nrfx_usbd_ep_t ep = NRFX_USBD_EPOUT(n);
        nrfx_usbd_ep_max_packet_size_set(ep, NRF_USBD_EPISO_CHECK(ep) ?
            (NRFX_USBD_ISOSIZE / 2) : NRFX_USBD_EPSIZE);
        usbd_ep_state_t * p_state = ep_state_access(ep);
        p_state->status = NRFX_USBD_EP_OK;
        p_state->handler.consumer = NULL;
        p_state->transfer_cnt = 0;
    }

    return NRFX_SUCCESS;
}

void nrfx_usbd_uninit(void)
{
    NRFX_ASSERT(m_drv_state == NRFX_DRV_STATE_INITIALIZED);

    m_event_handler = NULL;
    m_drv_state = NRFX_DRV_STATE_UNINITIALIZED;
    return;
}

void nrfx_usbd_enable(void)
{
    NRFX_ASSERT(m_drv_state == NRFX_DRV_STATE_INITIALIZED);

    /* Prepare for READY event receiving */
    nrf_usbd_eventcause_clear(NRF_USBD_EVENTCAUSE_READY_MASK);

    if (nrfx_usbd_errata_187())
    {
        NRFX_CRITICAL_SECTION_ENTER();
        if (*((volatile uint32_t *)(0x4006EC00)) == 0x00000000)
        {
            *((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
            *((volatile uint32_t *)(0x4006ED14)) = 0x00000003;
            *((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
        }
        else
        {
            *((volatile uint32_t *)(0x4006ED14)) = 0x00000003;
        }
        NRFX_CRITICAL_SECTION_EXIT();
    }
    
    if (nrfx_usbd_errata_171())
    {
        NRFX_CRITICAL_SECTION_ENTER();
        if (*((volatile uint32_t *)(0x4006EC00)) == 0x00000000)
        {
            *((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
            *((volatile uint32_t *)(0x4006EC14)) = 0x000000C0;
            *((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
        }
        else
        {
            *((volatile uint32_t *)(0x4006EC14)) = 0x000000C0;
        }
        NRFX_CRITICAL_SECTION_EXIT();
    }

    /* Enable the peripheral */
    nrf_usbd_enable();
    /* Waiting for peripheral to enable, this should take a few us */
    while (0 == (NRF_USBD_EVENTCAUSE_READY_MASK & nrf_usbd_eventcause_get()))
    {
        /* Empty loop */
    }
    nrf_usbd_eventcause_clear(NRF_USBD_EVENTCAUSE_READY_MASK);
    
    if (nrfx_usbd_errata_171())
    {
        NRFX_CRITICAL_SECTION_ENTER();
        if (*((volatile uint32_t *)(0x4006EC00)) == 0x00000000)
        {
            *((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
            *((volatile uint32_t *)(0x4006EC14)) = 0x00000000;
            *((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
        }
        else
        {
            *((volatile uint32_t *)(0x4006EC14)) = 0x00000000;
        }

        NRFX_CRITICAL_SECTION_EXIT();
    }

    if (nrfx_usbd_errata_166())
    {
        *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7E3;
        *((volatile uint32_t *)(NRF_USBD_BASE + 0x804)) = 0x40;
        __ISB();
        __DSB();
    }

    nrf_usbd_isosplit_set(NRF_USBD_ISOSPLIT_HALF);

    if (NRFX_USBD_CONFIG_ISO_IN_ZLP)
    {
        nrfx_usbd_isoinconfig_set(NRF_USBD_ISOINCONFIG_ZERODATA);
    }
    else
    {
        nrfx_usbd_isoinconfig_set(NRF_USBD_ISOINCONFIG_NORESP);
    }

    m_ep_ready = (((1U << NRF_USBD_EPIN_CNT) - 1U) << NRFX_USBD_EPIN_BITPOS_0);
    m_ep_dma_waiting = 0;
    usbd_dma_pending_clear();
    m_last_setup_dir = NRFX_USBD_EPOUT0;

    m_drv_state = NRFX_DRV_STATE_POWERED_ON;

    if (nrfx_usbd_errata_187())
    {
        NRFX_CRITICAL_SECTION_ENTER();
        if (*((volatile uint32_t *)(0x4006EC00)) == 0x00000000)
        {
            *((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
            *((volatile uint32_t *)(0x4006ED14)) = 0x00000000;
            *((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
        }
        else
        {
            *((volatile uint32_t *)(0x4006ED14)) = 0x00000000;
        }
        NRFX_CRITICAL_SECTION_EXIT();
    }
}

void nrfx_usbd_disable(void)
{
    NRFX_ASSERT(m_drv_state != NRFX_DRV_STATE_UNINITIALIZED);

    /* Stop just in case */
    nrfx_usbd_stop();

    /* Disable all parts */
    nrf_usbd_int_disable(nrf_usbd_int_enable_get());
    nrf_usbd_disable();
    usbd_dma_pending_clear();
    m_drv_state = NRFX_DRV_STATE_INITIALIZED;
}

void nrfx_usbd_start(bool enable_sof)
{
    NRFX_ASSERT(m_drv_state == NRFX_DRV_STATE_POWERED_ON);
    m_bus_suspend = false;

    uint32_t ints_to_enable =
       NRF_USBD_INT_USBRESET_MASK     |
       NRF_USBD_INT_STARTED_MASK      |
       NRF_USBD_INT_ENDEPIN0_MASK     |
       NRF_USBD_INT_EP0DATADONE_MASK  |
       NRF_USBD_INT_ENDEPOUT0_MASK    |
       NRF_USBD_INT_USBEVENT_MASK     |
       NRF_USBD_INT_EP0SETUP_MASK     |
       NRF_USBD_INT_DATAEP_MASK;

   if (enable_sof || nrfx_usbd_errata_104())
   {
       ints_to_enable |= NRF_USBD_INT_SOF_MASK;
   }

   /* Enable all required interrupts */
   nrf_usbd_int_enable(ints_to_enable);

   /* Enable interrupt globally */
   NRFX_IRQ_PRIORITY_SET(USBD_IRQn, NRFX_USBD_CONFIG_IRQ_PRIORITY);
   NRFX_IRQ_ENABLE(USBD_IRQn);

   /* Enable pullups */
   nrf_usbd_pullup_enable();
}

void nrfx_usbd_stop(void)
{
    NRFX_ASSERT(m_drv_state == NRFX_DRV_STATE_POWERED_ON);

    /* Clear interrupt */
    NRFX_IRQ_PENDING_CLEAR(USBD_IRQn);

    if (NRFX_IRQ_IS_ENABLED(USBD_IRQn))
    {
        /* Abort transfers */
        usbd_ep_abort_all();

        /* Disable pullups */
        nrf_usbd_pullup_disable();

        /* Disable interrupt globally */
        NRFX_IRQ_DISABLE(USBD_IRQn);

        /* Disable all interrupts */
        nrf_usbd_int_disable(~0U);
    }
}

bool nrfx_usbd_is_initialized(void)
{
    return (m_drv_state >= NRFX_DRV_STATE_INITIALIZED);
}

bool nrfx_usbd_is_enabled(void)
{
    return (m_drv_state >= NRFX_DRV_STATE_POWERED_ON);
}

bool nrfx_usbd_is_started(void)
{
    return (nrfx_usbd_is_enabled() && NRFX_IRQ_IS_ENABLED(USBD_IRQn));
}

bool nrfx_usbd_suspend(void)
{
    bool suspended = false;

    NRFX_CRITICAL_SECTION_ENTER();
    if (m_bus_suspend)
    {
        usbd_ep_abort_all();

        if (!(nrf_usbd_eventcause_get() & NRF_USBD_EVENTCAUSE_RESUME_MASK))
        {
            nrf_usbd_lowpower_enable();
            if (nrf_usbd_eventcause_get() & NRF_USBD_EVENTCAUSE_RESUME_MASK)
            {
                nrf_usbd_lowpower_disable();
            }
            else
            {
                suspended = true;
            }
        }
    }
    NRFX_CRITICAL_SECTION_EXIT();

    return suspended;
}

bool nrfx_usbd_wakeup_req(void)
{
    bool started = false;

    NRFX_CRITICAL_SECTION_ENTER();
    if (m_bus_suspend && nrf_usbd_lowpower_check())
    {
        nrf_usbd_lowpower_disable();
        started = true;

        if (nrfx_usbd_errata_171())
        {
            if (*((volatile uint32_t *)(0x4006EC00)) == 0x00000000)
            {
                *((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
                *((volatile uint32_t *)(0x4006EC14)) = 0x000000C0;
                *((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
            }
            else
            {
                *((volatile uint32_t *)(0x4006EC14)) = 0x000000C0;
            }

        }
    }
    NRFX_CRITICAL_SECTION_EXIT();

    return started;
}

bool nrfx_usbd_suspend_check(void)
{
    return nrf_usbd_lowpower_check();
}

void nrfx_usbd_suspend_irq_config(void)
{
    nrf_usbd_int_disable(m_irq_disabled_in_suspend);
}

void nrfx_usbd_active_irq_config(void)
{
    nrf_usbd_int_enable(m_irq_disabled_in_suspend);
}

bool nrfx_usbd_bus_suspend_check(void)
{
    return m_bus_suspend;
}

void nrfx_usbd_force_bus_wakeup(void)
{
    m_bus_suspend = false;
}

void nrfx_usbd_ep_max_packet_size_set(nrfx_usbd_ep_t ep, uint16_t size)
{
    /* Only power of 2 size allowed */
    NRFX_ASSERT((size != 0) && (size & (size - 1)) == 0);
    /* Packet size cannot be higher than maximum buffer size */
    NRFX_ASSERT((NRF_USBD_EPISO_CHECK(ep) && (size <= usbd_ep_iso_capacity(ep))) ||
                (!NRF_USBD_EPISO_CHECK(ep) && (size <= NRFX_USBD_EPSIZE)));

    usbd_ep_state_t * p_state = ep_state_access(ep);
    p_state->max_packet_size = size;
}

uint16_t nrfx_usbd_ep_max_packet_size_get(nrfx_usbd_ep_t ep)
{
    usbd_ep_state_t const * p_state = ep_state_access(ep);
    return p_state->max_packet_size;
}

bool nrfx_usbd_ep_enable_check(nrfx_usbd_ep_t ep)
{
    return nrf_usbd_ep_enable_check(ep_to_hal(ep));
}

void nrfx_usbd_ep_enable(nrfx_usbd_ep_t ep)
{
    nrf_usbd_int_enable(nrfx_usbd_ep_to_int(ep));

    if (nrf_usbd_ep_enable_check(ep))
    {
        return;
    }
    nrf_usbd_ep_enable(ep_to_hal(ep));
    if ((NRF_USBD_EP_NR_GET(ep) != 0) &&
        NRF_USBD_EPOUT_CHECK(ep) &&
        !NRF_USBD_EPISO_CHECK(ep))
    {
        NRFX_CRITICAL_SECTION_ENTER();
        nrfx_usbd_transfer_out_drop(ep);
        m_ep_dma_waiting &= ~(1U << ep2bit(ep));
        NRFX_CRITICAL_SECTION_EXIT();
    }
}

void nrfx_usbd_ep_disable(nrfx_usbd_ep_t ep)
{
    usbd_ep_abort(ep);
    nrf_usbd_ep_disable(ep_to_hal(ep));
    nrf_usbd_int_disable(nrfx_usbd_ep_to_int(ep));
}

void nrfx_usbd_ep_default_config(void)
{
    nrf_usbd_int_disable(
        NRF_USBD_INT_ENDEPIN1_MASK  |
        NRF_USBD_INT_ENDEPIN2_MASK  |
        NRF_USBD_INT_ENDEPIN3_MASK  |
        NRF_USBD_INT_ENDEPIN4_MASK  |
        NRF_USBD_INT_ENDEPIN5_MASK  |
        NRF_USBD_INT_ENDEPIN6_MASK  |
        NRF_USBD_INT_ENDEPIN7_MASK  |
        NRF_USBD_INT_ENDISOIN0_MASK |
        NRF_USBD_INT_ENDEPOUT1_MASK |
        NRF_USBD_INT_ENDEPOUT2_MASK |
        NRF_USBD_INT_ENDEPOUT3_MASK |
        NRF_USBD_INT_ENDEPOUT4_MASK |
        NRF_USBD_INT_ENDEPOUT5_MASK |
        NRF_USBD_INT_ENDEPOUT6_MASK |
        NRF_USBD_INT_ENDEPOUT7_MASK |
        NRF_USBD_INT_ENDISOOUT0_MASK
    );
    nrf_usbd_int_enable(NRF_USBD_INT_ENDEPIN0_MASK | NRF_USBD_INT_ENDEPOUT0_MASK);
    nrf_usbd_ep_all_disable();
}

nrfx_err_t nrfx_usbd_ep_transfer(
    nrfx_usbd_ep_t               ep,
    nrfx_usbd_transfer_t const * p_transfer)
{
    nrfx_err_t ret;
    const uint8_t ep_bitpos = ep2bit(ep);
    NRFX_ASSERT(NULL != p_transfer);

    NRFX_CRITICAL_SECTION_ENTER();
    /* Setup data transaction can go only in one direction at a time */
    if ((NRF_USBD_EP_NR_GET(ep) == 0) && (ep != m_last_setup_dir))
    {
        ret = NRFX_ERROR_INVALID_ADDR;
        if (NRFX_USBD_FAILED_TRANSFERS_DEBUG &&
            (NRFX_USBD_ISO_DEBUG || (!NRF_USBD_EPISO_CHECK(ep))))
        {
            NRFX_LOG_DEBUG("Transfer failed: Invalid EPr\n");
        }
    }
    else if ((m_ep_dma_waiting | ((~m_ep_ready) & NRFX_USBD_EPIN_BIT_MASK)) & (1U << ep_bitpos))
    {
        /* IN (Device -> Host) transfer has to be transmitted out to allow new transmission */
        ret = NRFX_ERROR_BUSY;
        if (NRFX_USBD_FAILED_TRANSFERS_DEBUG)
        {
            NRFX_LOG_DEBUG("Transfer failed: EP is busy");
        }
    }
    else
    {
        usbd_ep_state_t * p_state =  ep_state_access(ep);
        /* Prepare transfer context and handler description */
        nrfx_usbd_transfer_t * p_context;
        if (NRF_USBD_EPIN_CHECK(ep))
        {
            p_context = m_ep_feeder_state + NRF_USBD_EP_NR_GET(ep);
            if (nrfx_is_in_ram(p_transfer->p_data.tx))
            {
                /* RAM */
                if (0 == (p_transfer->flags & NRFX_USBD_TRANSFER_ZLP_FLAG))
                {
                    p_state->handler.feeder = nrfx_usbd_feeder_ram;
                    if (NRFX_USBD_ISO_DEBUG || (!NRF_USBD_EPISO_CHECK(ep)))
                    {
                        NRFX_LOG_DEBUG(
                            "Transfer called on endpoint %x, size: %u, mode: "
                            "RAM",
                            ep,
                            p_transfer->size);
                    }
                }
                else
                {
                    p_state->handler.feeder = nrfx_usbd_feeder_ram_zlp;
                    if (NRFX_USBD_ISO_DEBUG || (!NRF_USBD_EPISO_CHECK(ep)))
                    {
                        NRFX_LOG_DEBUG(
                            "Transfer called on endpoint %x, size: %u, mode: "
                            "RAM_ZLP",
                            ep,
                            p_transfer->size);
                    }
                }
            }
            else
            {
                /* Flash */
                if (0 == (p_transfer->flags & NRFX_USBD_TRANSFER_ZLP_FLAG))
                {
                    p_state->handler.feeder = nrfx_usbd_feeder_flash;
                    if (NRFX_USBD_ISO_DEBUG || (!NRF_USBD_EPISO_CHECK(ep)))
                    {
                        NRFX_LOG_DEBUG(
                            "Transfer called on endpoint %x, size: %u, mode: "
                            "FLASH",
                            ep,
                            p_transfer->size);
                    }
                }
                else
                {
                    p_state->handler.feeder = nrfx_usbd_feeder_flash_zlp;
                    if (NRFX_USBD_ISO_DEBUG || (!NRF_USBD_EPISO_CHECK(ep)))
                    {
                        NRFX_LOG_DEBUG(
                            "Transfer called on endpoint %x, size: %u, mode: "
                            "FLASH_ZLP",
                            ep,
                            p_transfer->size);
                    }
                }
            }
        }
        else
        {
            p_context = m_ep_consumer_state + NRF_USBD_EP_NR_GET(ep);
            NRFX_ASSERT((p_transfer->p_data.rx == NULL) || (nrfx_is_in_ram(p_transfer->p_data.rx)));
            p_state->handler.consumer = nrfx_usbd_consumer;
        }
        *p_context = *p_transfer;
        p_state->p_context = p_context;

        p_state->transfer_cnt = 0;
        p_state->status    =  NRFX_USBD_EP_OK;
        m_ep_dma_waiting   |= 1U << ep_bitpos;
        ret = NRFX_SUCCESS;
        usbd_int_rise();
    }
    NRFX_CRITICAL_SECTION_EXIT();
    return ret;
}

nrfx_err_t nrfx_usbd_ep_handled_transfer(
    nrfx_usbd_ep_t                   ep,
    nrfx_usbd_handler_desc_t const * p_handler)
{
    nrfx_err_t ret;
    const uint8_t ep_bitpos = ep2bit(ep);
    NRFX_ASSERT(NULL != p_handler);

    NRFX_CRITICAL_SECTION_ENTER();
    /* Setup data transaction can go only in one direction at a time */
    if ((NRF_USBD_EP_NR_GET(ep) == 0) && (ep != m_last_setup_dir))
    {
        ret = NRFX_ERROR_INVALID_ADDR;
        if (NRFX_USBD_FAILED_TRANSFERS_DEBUG && (NRFX_USBD_ISO_DEBUG || (!NRF_USBD_EPISO_CHECK(ep))))
        {
            NRFX_LOG_DEBUG("Transfer failed: Invalid EP");
        }
    }
    else if ((m_ep_dma_waiting | ((~m_ep_ready) & NRFX_USBD_EPIN_BIT_MASK)) & (1U << ep_bitpos))
    {
        /* IN (Device -> Host) transfer has to be transmitted out to allow a new transmission */
        ret = NRFX_ERROR_BUSY;
        if (NRFX_USBD_FAILED_TRANSFERS_DEBUG && (NRFX_USBD_ISO_DEBUG || (!NRF_USBD_EPISO_CHECK(ep))))
        {
            NRFX_LOG_DEBUG("Transfer failed: EP is busy");
        }
    }
    else
    {
        /* Transfer can be configured now */
        usbd_ep_state_t * p_state =  ep_state_access(ep);

        p_state->transfer_cnt = 0;
        p_state->handler   = p_handler->handler;
        p_state->p_context = p_handler->p_context;
        p_state->status    =  NRFX_USBD_EP_OK;
        m_ep_dma_waiting   |= 1U << ep_bitpos;

        ret = NRFX_SUCCESS;
        if (NRFX_USBD_ISO_DEBUG || (!NRF_USBD_EPISO_CHECK(ep)))
        {
            NRFX_LOG_DEBUG("Transfer called on endpoint %x, mode: Handler", ep);
        }
        usbd_int_rise();
    }
    NRFX_CRITICAL_SECTION_EXIT();
    return ret;
}

void * nrfx_usbd_feeder_buffer_get(void)
{
    return m_tx_buffer;
}

nrfx_usbd_ep_status_t nrfx_usbd_ep_status_get(nrfx_usbd_ep_t ep, size_t * p_size)
{
    nrfx_usbd_ep_status_t ret;

    usbd_ep_state_t const * p_state = ep_state_access(ep);
    NRFX_CRITICAL_SECTION_ENTER();
    *p_size = p_state->transfer_cnt;
    ret = (p_state->handler.consumer == NULL) ? p_state->status : NRFX_USBD_EP_BUSY;
    NRFX_CRITICAL_SECTION_EXIT();
    return ret;
}

size_t nrfx_usbd_epout_size_get(nrfx_usbd_ep_t ep)
{
    return nrf_usbd_epout_size_get(ep_to_hal(ep));
}

bool nrfx_usbd_ep_is_busy(nrfx_usbd_ep_t ep)
{
    return (0 != ((m_ep_dma_waiting | ((~m_ep_ready) & NRFX_USBD_EPIN_BIT_MASK)) & (1U << ep2bit(ep))));
}

void nrfx_usbd_ep_stall(nrfx_usbd_ep_t ep)
{
    NRFX_LOG_DEBUG("USB: EP %x stalled.", ep);
    nrf_usbd_ep_stall(ep_to_hal(ep));
}

void nrfx_usbd_ep_stall_clear(nrfx_usbd_ep_t ep)
{
    if (NRF_USBD_EPOUT_CHECK(ep) && nrfx_usbd_ep_stall_check(ep))
    {
        nrfx_usbd_transfer_out_drop(ep);
    }
    nrf_usbd_ep_unstall(ep_to_hal(ep));
}

bool nrfx_usbd_ep_stall_check(nrfx_usbd_ep_t ep)
{
    return nrf_usbd_ep_is_stall(ep_to_hal(ep));
}

void nrfx_usbd_ep_dtoggle_clear(nrfx_usbd_ep_t ep)
{
    nrf_usbd_dtoggle_set(ep, NRF_USBD_DTOGGLE_DATA0);
}

void nrfx_usbd_setup_get(nrfx_usbd_setup_t * p_setup)
{
    memset(p_setup, 0, sizeof(nrfx_usbd_setup_t));
    p_setup->bmRequestType = nrf_usbd_setup_bmrequesttype_get();
    p_setup->bRequest      = nrf_usbd_setup_brequest_get();
    p_setup->wValue        = nrf_usbd_setup_wvalue_get();
    p_setup->wIndex        = nrf_usbd_setup_windex_get();
    p_setup->wLength       = nrf_usbd_setup_wlength_get();
}

void nrfx_usbd_setup_data_clear(void)
{
    if (nrfx_usbd_errata_104())
    {
        /* For this fix to work properly, it must be ensured that the task is
         * executed twice one after another - blocking ISR. This is however a temporary
         * solution to be used only before production version of the chip. */
        uint32_t primask_copy = __get_PRIMASK();
        __disable_irq();
        nrf_usbd_task_trigger(NRF_USBD_TASK_EP0RCVOUT);
        nrf_usbd_task_trigger(NRF_USBD_TASK_EP0RCVOUT);
        __set_PRIMASK(primask_copy);
    }
    else
    {
        nrf_usbd_task_trigger(NRF_USBD_TASK_EP0RCVOUT);
    }
}

void nrfx_usbd_setup_clear(void)
{
    NRFX_LOG_DEBUG(">> ep0status >>");
    nrf_usbd_task_trigger(NRF_USBD_TASK_EP0STATUS);
}

void nrfx_usbd_setup_stall(void)
{
    NRFX_LOG_DEBUG("Setup stalled.");
    nrf_usbd_task_trigger(NRF_USBD_TASK_EP0STALL);
}

nrfx_usbd_ep_t nrfx_usbd_last_setup_dir_get(void)
{
    return m_last_setup_dir;
}

void nrfx_usbd_transfer_out_drop(nrfx_usbd_ep_t ep)
{
    NRFX_ASSERT(NRF_USBD_EPOUT_CHECK(ep));

    if (nrfx_usbd_errata_200())
    {
        NRFX_CRITICAL_SECTION_ENTER();
        m_ep_ready &= ~(1U << ep2bit(ep));
        *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7C5 + (2u * NRF_USBD_EP_NR_GET(ep));
        *((volatile uint32_t *)(NRF_USBD_BASE + 0x804)) = 0;
        (void)(*((volatile uint32_t *)(NRF_USBD_BASE + 0x804)));
        NRFX_CRITICAL_SECTION_EXIT();
    }
    else
    {
        NRFX_CRITICAL_SECTION_ENTER();
        m_ep_ready &= ~(1U << ep2bit(ep));
        if (!NRF_USBD_EPISO_CHECK(ep))
        {
            nrf_usbd_epout_clear(ep);
        }
        NRFX_CRITICAL_SECTION_EXIT();
    }
}

#endif // NRFX_CHECK(NRFX_USBD_ENABLED)
