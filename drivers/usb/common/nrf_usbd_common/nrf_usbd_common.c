/*
 * Copyright (c) 2016 - 2023, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* This file is undergoing transition towards native Zephyr nrf USB driver. */

/** @cond INTERNAL_HIDDEN */

#include <nrfx.h>

#include "nrf_usbd_common.h"
#include "nrf_usbd_common_errata.h"
#include <string.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nrf_usbd_common, CONFIG_NRF_USBD_COMMON_LOG_LEVEL);

#ifndef NRF_USBD_COMMON_STARTED_EV_ENABLE
#define NRF_USBD_COMMON_STARTED_EV_ENABLE 0
#endif

#ifndef NRF_USBD_COMMON_ISO_DEBUG
/* Also generate information about ISOCHRONOUS events and transfers.
 * Turn this off if no ISOCHRONOUS transfers are going to be debugged and this
 * option generates a lot of useless messages.
 */
#define NRF_USBD_COMMON_ISO_DEBUG 1
#endif

#ifndef NRF_USBD_COMMON_FAILED_TRANSFERS_DEBUG
/* Also generate debug information for failed transfers.
 * It might be useful but may generate a lot of useless debug messages
 * in some library usages (for example when transfer is generated and the
 * result is used to check whatever endpoint was busy.
 */
#define NRF_USBD_COMMON_FAILED_TRANSFERS_DEBUG 1
#endif

#ifndef NRF_USBD_COMMON_DMAREQ_PROCESS_DEBUG
/* Generate additional messages that mark the status inside
 * @ref usbd_dmareq_process.
 * It is useful to debug library internals but may generate a lot of
 * useless debug messages.
 */
#define NRF_USBD_COMMON_DMAREQ_PROCESS_DEBUG 1
#endif

#ifndef NRF_USBD_COMMON_USE_WORKAROUND_FOR_ANOMALY_211
/* Anomaly 211 - Device remains in SUSPEND too long when host resumes
 * a bus activity (sending SOF packets) without a RESUME condition.
 */
#define NRF_USBD_COMMON_USE_WORKAROUND_FOR_ANOMALY_211 0
#endif

/**
 * @defgroup nrf_usbd_common_int USB Device driver internal part
 * @internal
 * @ingroup nrf_usbd_common
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
#define NRF_USBD_COMMON_ASSERT_EP_VALID(ep) __ASSERT_NO_MSG(                             \
	((NRF_USBD_EPIN_CHECK(ep) && (NRF_USBD_EP_NR_GET(ep) < NRF_USBD_EPIN_CNT)) ||    \
	 (NRF_USBD_EPOUT_CHECK(ep) && (NRF_USBD_EP_NR_GET(ep) < NRF_USBD_EPOUT_CNT))));

/**
 * @brief Lowest position of bit for IN endpoint.
 *
 * The first bit position corresponding to IN endpoint.
 * @sa ep2bit bit2ep
 */
#define NRF_USBD_COMMON_EPIN_BITPOS_0 0

/**
 * @brief Lowest position of bit for OUT endpoint.
 *
 * The first bit position corresponding to OUT endpoint
 * @sa ep2bit bit2ep
 */
#define NRF_USBD_COMMON_EPOUT_BITPOS_0 16

/**
 * @brief Input endpoint bits mask.
 */
#define NRF_USBD_COMMON_EPIN_BIT_MASK (0xFFFFU << NRF_USBD_COMMON_EPIN_BITPOS_0)

/**
 * @brief Output endpoint bits mask.
 */
#define NRF_USBD_COMMON_EPOUT_BIT_MASK (0xFFFFU << NRF_USBD_COMMON_EPOUT_BITPOS_0)

/**
 * @brief Isochronous endpoint bit mask
 */
#define USBD_EPISO_BIT_MASK                                                    \
	((1U << NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPOUT8)) |           \
	 (1U << NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPIN8)))

/**
 * @brief Auxiliary macro to change EP number into bit position.
 *
 * This macro is used by @ref ep2bit function but also for statically check
 * the bitpos values integrity during compilation.
 *
 * @param[in] ep Endpoint number.
 * @return Endpoint bit position.
 */
#define NRF_USBD_COMMON_EP_BITPOS(ep)                                                              \
	((NRF_USBD_EPIN_CHECK(ep) ? NRF_USBD_COMMON_EPIN_BITPOS_0 : NRF_USBD_COMMON_EPOUT_BITPOS_0)\
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
#define NRF_USBD_COMMON_EP_TRANSFER_EVENT(name, endpont, ep_stat)                                  \
	const nrf_usbd_common_evt_t name = {NRF_USBD_COMMON_EVT_EPTRANSFER,                        \
				      .data = {.eptransfer = {.ep = endpont, .status = ep_stat}}}

/* Check it the bit positions values match defined DATAEPSTATUS bit positions */
BUILD_ASSERT(
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPIN1) == USBD_EPDATASTATUS_EPIN1_Pos) &&
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPIN2) == USBD_EPDATASTATUS_EPIN2_Pos) &&
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPIN3) == USBD_EPDATASTATUS_EPIN3_Pos) &&
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPIN4) == USBD_EPDATASTATUS_EPIN4_Pos) &&
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPIN5) == USBD_EPDATASTATUS_EPIN5_Pos) &&
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPIN6) == USBD_EPDATASTATUS_EPIN6_Pos) &&
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPIN7) == USBD_EPDATASTATUS_EPIN7_Pos) &&
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPOUT1) == USBD_EPDATASTATUS_EPOUT1_Pos) &&
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPOUT2) == USBD_EPDATASTATUS_EPOUT2_Pos) &&
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPOUT3) == USBD_EPDATASTATUS_EPOUT3_Pos) &&
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPOUT4) == USBD_EPDATASTATUS_EPOUT4_Pos) &&
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPOUT5) == USBD_EPDATASTATUS_EPOUT5_Pos) &&
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPOUT6) == USBD_EPDATASTATUS_EPOUT6_Pos) &&
	(NRF_USBD_COMMON_EP_BITPOS(NRF_USBD_COMMON_EPOUT7) == USBD_EPDATASTATUS_EPOUT7_Pos),
	"NRF_USBD_COMMON bit positions do not match hardware"
);

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
static nrf_usbd_common_event_handler_t m_event_handler;

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
 * Internal constant used in @ref nrf_usbd_common_suspend_irq_config and
 * @ref nrf_usbd_common_active_irq_config functions.
 */
static const uint32_t m_irq_disabled_in_suspend =
	NRF_USBD_INT_ENDEPIN0_MASK | NRF_USBD_INT_EP0DATADONE_MASK | NRF_USBD_INT_ENDEPOUT0_MASK |
	NRF_USBD_INT_EP0SETUP_MASK | NRF_USBD_INT_DATAEP_MASK;

/**
 * @brief Direction of last received Setup transfer.
 *
 * This variable is used to redirect internal setup data event
 * into selected endpoint (IN or OUT).
 */
static nrf_usbd_common_ep_t m_last_setup_dir;

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
static atomic_t m_ep_dma_waiting;

/* Semaphore to guard EasyDMA access.
 * In USBD there is only one DMA channel working in background, and new transfer
 * cannot be started when there is ongoing transfer on any other channel.
 */
static K_SEM_DEFINE(dma_available, 1, 1);

/**
 * @brief Tracks whether total bytes transferred by DMA is even or odd.
 */
static uint8_t m_dma_odd;

/**
 * @brief First time enabling after reset. Used in nRF52 errata 223.
 */
static bool m_first_enable = true;

/**
 * @brief The structure that would hold transfer configuration to every endpoint
 *
 * The structure that holds all the data required by the endpoint to proceed
 * with LIST functionality and generate quick callback directly when data
 * buffer is ready.
 */
typedef struct {
	/** Handler for current transfer, function pointer. */
	nrf_usbd_common_handler_t handler;
	/** Context for transfer handler. */
	void *p_context;
	/** Number of transferred bytes in the current transfer. */
	size_t transfer_cnt;
	/** Configured endpoint size. */
	uint16_t max_packet_size;
	/** NRFX_SUCCESS or error code, never NRFX_ERROR_BUSY - this one is calculated. */
	nrf_usbd_common_ep_status_t status;
} usbd_ep_state_t;

/**
 * @brief The array of transfer configurations for the endpoints.
 *
 * The status of the transfer on each endpoint.
 */
static struct {
	usbd_ep_state_t ep_out[NRF_USBD_EPOUT_CNT]; /*!< Status for OUT endpoints. */
	usbd_ep_state_t ep_in[NRF_USBD_EPIN_CNT];   /*!< Status for IN endpoints. */
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
nrf_usbd_common_transfer_t m_ep_feeder_state[NRF_USBD_EPIN_CNT];

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
nrf_usbd_common_transfer_t m_ep_consumer_state[NRF_USBD_EPOUT_CNT];

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
static uint32_t m_tx_buffer[NRFX_CEIL_DIV(NRF_USBD_COMMON_FEEDER_BUFFER_SIZE, sizeof(uint32_t))];

/* Early declaration. Documentation above definition. */
static void usbd_dmareq_process(void);
static inline void usbd_int_rise(void);
static void nrf_usbd_common_stop(void);

/**
 * @brief Change endpoint number to endpoint event code.
 *
 * @param ep Endpoint number.
 *
 * @return Connected endpoint event code.
 *
 * Marker to delete when not required anymore: >> NRF_USBD_COMMON_ERRATA_ENABLE <<.
 */
static inline nrf_usbd_event_t nrf_usbd_common_ep_to_endevent(nrf_usbd_common_ep_t ep)
{
	NRF_USBD_COMMON_ASSERT_EP_VALID(ep);

	static const nrf_usbd_event_t epin_endev[] = {
		NRF_USBD_EVENT_ENDEPIN0, NRF_USBD_EVENT_ENDEPIN1, NRF_USBD_EVENT_ENDEPIN2,
		NRF_USBD_EVENT_ENDEPIN3, NRF_USBD_EVENT_ENDEPIN4, NRF_USBD_EVENT_ENDEPIN5,
		NRF_USBD_EVENT_ENDEPIN6, NRF_USBD_EVENT_ENDEPIN7, NRF_USBD_EVENT_ENDISOIN0};
	static const nrf_usbd_event_t epout_endev[] = {
		NRF_USBD_EVENT_ENDEPOUT0, NRF_USBD_EVENT_ENDEPOUT1, NRF_USBD_EVENT_ENDEPOUT2,
		NRF_USBD_EVENT_ENDEPOUT3, NRF_USBD_EVENT_ENDEPOUT4, NRF_USBD_EVENT_ENDEPOUT5,
		NRF_USBD_EVENT_ENDEPOUT6, NRF_USBD_EVENT_ENDEPOUT7, NRF_USBD_EVENT_ENDISOOUT0};

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
static inline uint32_t nrf_usbd_common_ep_to_int(nrf_usbd_common_ep_t ep)
{
	NRF_USBD_COMMON_ASSERT_EP_VALID(ep);

	static const uint8_t epin_bitpos[] = {
		USBD_INTEN_ENDEPIN0_Pos, USBD_INTEN_ENDEPIN1_Pos, USBD_INTEN_ENDEPIN2_Pos,
		USBD_INTEN_ENDEPIN3_Pos, USBD_INTEN_ENDEPIN4_Pos, USBD_INTEN_ENDEPIN5_Pos,
		USBD_INTEN_ENDEPIN6_Pos, USBD_INTEN_ENDEPIN7_Pos, USBD_INTEN_ENDISOIN_Pos};
	static const uint8_t epout_bitpos[] = {
		USBD_INTEN_ENDEPOUT0_Pos, USBD_INTEN_ENDEPOUT1_Pos, USBD_INTEN_ENDEPOUT2_Pos,
		USBD_INTEN_ENDEPOUT3_Pos, USBD_INTEN_ENDEPOUT4_Pos, USBD_INTEN_ENDEPOUT5_Pos,
		USBD_INTEN_ENDEPOUT6_Pos, USBD_INTEN_ENDEPOUT7_Pos, USBD_INTEN_ENDISOOUT_Pos};

	return 1UL << (NRF_USBD_EPIN_CHECK(ep) ? epin_bitpos
					       : epout_bitpos)[NRF_USBD_EP_NR_GET(ep)];
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
 * @param p_next    See @ref nrf_usbd_common_consumer_t documentation.
 * @param p_context See @ref nrf_usbd_common_consumer_t documentation.
 * @param ep_size   See @ref nrf_usbd_common_consumer_t documentation.
 * @param data_size See @ref nrf_usbd_common_consumer_t documentation.
 *
 * @retval true  Continue transfer.
 * @retval false This was the last transfer.
 */
bool nrf_usbd_common_consumer(nrf_usbd_common_ep_transfer_t *p_next, void *p_context,
			      size_t ep_size, size_t data_size)
{
	nrf_usbd_common_transfer_t *p_transfer = (nrf_usbd_common_transfer_t *)p_context;

	__ASSERT_NO_MSG(ep_size >= data_size);
	__ASSERT_NO_MSG((p_transfer->p_data.rx == NULL) || nrfx_is_in_ram(p_transfer->p_data.rx));

	size_t size = p_transfer->size;

	if (size < data_size) {
		LOG_DBG("consumer: buffer too small: r: %u, l: %u", data_size, size);
		/* Buffer size to small */
		p_next->size = 0;
		p_next->p_data = p_transfer->p_data;
	} else {
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
 * @param[out]    p_next    See @ref nrf_usbd_common_feeder_t documentation.
 * @param[in,out] p_context See @ref nrf_usbd_common_feeder_t documentation.
 * @param[in]     ep_size   See @ref nrf_usbd_common_feeder_t documentation.
 *
 * @retval true  Continue transfer.
 * @retval false This was the last transfer.
 */
bool nrf_usbd_common_feeder_ram(nrf_usbd_common_ep_transfer_t *p_next,
				void *p_context, size_t ep_size)
{
	nrf_usbd_common_transfer_t *p_transfer = (nrf_usbd_common_transfer_t *)p_context;

	__ASSERT_NO_MSG(nrfx_is_in_ram(p_transfer->p_data.tx));

	size_t tx_size = p_transfer->size;

	if (tx_size > ep_size) {
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
 * @param[out]    p_next    See @ref nrf_usbd_common_feeder_t documentation.
 * @param[in,out] p_context See @ref nrf_usbd_common_feeder_t documentation.
 * @param[in]     ep_size   See @ref nrf_usbd_common_feeder_t documentation.
 *
 * @retval true  Continue transfer.
 * @retval false This was the last transfer.
 */
bool nrf_usbd_common_feeder_ram_zlp(nrf_usbd_common_ep_transfer_t *p_next,
				    void *p_context, size_t ep_size)
{
	nrf_usbd_common_transfer_t *p_transfer = (nrf_usbd_common_transfer_t *)p_context;

	__ASSERT_NO_MSG(nrfx_is_in_ram(p_transfer->p_data.tx));

	size_t tx_size = p_transfer->size;

	if (tx_size > ep_size) {
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
 * @param[out]    p_next    See @ref nrf_usbd_common_feeder_t documentation.
 * @param[in,out] p_context See @ref nrf_usbd_common_feeder_t documentation.
 * @param[in]     ep_size   See @ref nrf_usbd_common_feeder_t documentation.
 *
 * @retval true  Continue transfer.
 * @retval false This was the last transfer.
 */
bool nrf_usbd_common_feeder_flash(nrf_usbd_common_ep_transfer_t *p_next,
				  void *p_context, size_t ep_size)
{
	nrf_usbd_common_transfer_t *p_transfer = (nrf_usbd_common_transfer_t *)p_context;

	__ASSERT_NO_MSG(!nrfx_is_in_ram(p_transfer->p_data.tx));

	size_t tx_size = p_transfer->size;
	void *p_buffer = nrf_usbd_common_feeder_buffer_get();

	if (tx_size > ep_size) {
		tx_size = ep_size;
	}

	__ASSERT_NO_MSG(tx_size <= NRF_USBD_COMMON_FEEDER_BUFFER_SIZE);
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
 * @param[out]    p_next    See @ref nrf_usbd_common_feeder_t documentation.
 * @param[in,out] p_context See @ref nrf_usbd_common_feeder_t documentation.
 * @param[in]     ep_size   See @ref nrf_usbd_common_feeder_t documentation.
 *
 * @retval true  Continue transfer.
 * @retval false This was the last transfer.
 */
bool nrf_usbd_common_feeder_flash_zlp(nrf_usbd_common_ep_transfer_t *p_next,
				      void *p_context, size_t ep_size)
{
	nrf_usbd_common_transfer_t *p_transfer = (nrf_usbd_common_transfer_t *)p_context;

	__ASSERT_NO_MSG(!nrfx_is_in_ram(p_transfer->p_data.tx));

	size_t tx_size = p_transfer->size;
	void *p_buffer = nrf_usbd_common_feeder_buffer_get();

	if (tx_size > ep_size) {
		tx_size = ep_size;
	}

	__ASSERT_NO_MSG(tx_size <= NRF_USBD_COMMON_FEEDER_BUFFER_SIZE);

	if (tx_size != 0) {
		memcpy(p_buffer, (p_transfer->p_data.tx), tx_size);
		p_next->p_data.tx = p_buffer;
	} else {
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
 * @sa nrf_usbd_common_ep_from_hal
 */
static inline uint8_t ep_to_hal(nrf_usbd_common_ep_t ep)
{
	NRF_USBD_COMMON_ASSERT_EP_VALID(ep);
	return (uint8_t)ep;
}

/**
 * @brief Generate start task number for selected endpoint index.
 *
 * @param ep Endpoint number.
 *
 * @return Task for starting EasyDMA transfer on selected endpoint.
 */
static inline nrf_usbd_task_t task_start_ep(nrf_usbd_common_ep_t ep)
{
	NRF_USBD_COMMON_ASSERT_EP_VALID(ep);
	return (nrf_usbd_task_t)((NRF_USBD_EPIN_CHECK(ep) ? NRF_USBD_TASK_STARTEPIN0
							  : NRF_USBD_TASK_STARTEPOUT0) +
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
static inline usbd_ep_state_t *ep_state_access(nrf_usbd_common_ep_t ep)
{
	NRF_USBD_COMMON_ASSERT_EP_VALID(ep);
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
static inline uint8_t ep2bit(nrf_usbd_common_ep_t ep)
{
	NRF_USBD_COMMON_ASSERT_EP_VALID(ep);
	return NRF_USBD_COMMON_EP_BITPOS(ep);
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
static inline nrf_usbd_common_ep_t bit2ep(uint8_t bitpos)
{
	BUILD_ASSERT(NRF_USBD_COMMON_EPOUT_BITPOS_0 > NRF_USBD_COMMON_EPIN_BITPOS_0,
		     "OUT endpoint bits should be higher than IN endpoint bits");
	return (nrf_usbd_common_ep_t)((bitpos >= NRF_USBD_COMMON_EPOUT_BITPOS_0)
					? NRF_USBD_EPOUT(bitpos - NRF_USBD_COMMON_EPOUT_BITPOS_0)
					: NRF_USBD_EPIN(bitpos));
}

/**
 * @brief Mark that EasyDMA is working.
 *
 * Internal function to set the flag informing about EasyDMA transfer pending.
 * This function is called always just after the EasyDMA transfer is started.
 */
static inline void usbd_dma_pending_set(void)
{
	if (nrf_usbd_common_errata_199()) {
		*((volatile uint32_t *)0x40027C1C) = 0x00000082;
	}
}

/**
 * @brief Mark that EasyDMA is free.
 *
 * Internal function to clear the flag informing about EasyDMA transfer pending.
 * This function is called always just after the finished EasyDMA transfer is detected.
 */
static inline void usbd_dma_pending_clear(void)
{
	if (nrf_usbd_common_errata_199()) {
		*((volatile uint32_t *)0x40027C1C) = 0x00000000;
	}
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
static inline void usbd_dma_start(nrf_usbd_common_ep_t ep)
{
	nrf_usbd_task_trigger(NRF_USBD, task_start_ep(ep));
}

void nrf_usbd_common_isoinconfig_set(nrf_usbd_isoinconfig_t config)
{
	nrf_usbd_isoinconfig_set(NRF_USBD, config);
}

nrf_usbd_isoinconfig_t nrf_usbd_common_isoinconfig_get(void)
{
	return nrf_usbd_isoinconfig_get(NRF_USBD);
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
static inline void usbd_ep_abort(nrf_usbd_common_ep_t ep)
{
	unsigned int irq_lock_key = irq_lock();
	usbd_ep_state_t *p_state = ep_state_access(ep);

	if (NRF_USBD_EPOUT_CHECK(ep)) {
		/* Host -> Device */
		if ((~m_ep_dma_waiting) & (1U << ep2bit(ep))) {
			/* If the bit in m_ep_dma_waiting in cleared - nothing would be
			 * processed inside transfer processing
			 */
			nrf_usbd_common_transfer_out_drop(ep);
		} else {
			p_state->handler.consumer = NULL;
			m_ep_dma_waiting &= ~(1U << ep2bit(ep));
			m_ep_ready &= ~(1U << ep2bit(ep));
		}
		/* Aborted */
		p_state->status = NRF_USBD_COMMON_EP_ABORTED;
	} else {
		if (!NRF_USBD_EPISO_CHECK(ep)) {
			/* Workaround: Disarm the endpoint if there is any data buffered. */
			if (ep != NRF_USBD_COMMON_EPIN0) {
				*((volatile uint32_t *)((uint32_t)(NRF_USBD) + 0x800)) =
					0x7B6 + (2u * (NRF_USBD_EP_NR_GET(ep) - 1));
				uint8_t temp =
					*((volatile uint32_t *)((uint32_t)(NRF_USBD) + 0x804));
				temp |= (1U << 1);
				*((volatile uint32_t *)((uint32_t)(NRF_USBD) + 0x804)) |= temp;
				(void)(*((volatile uint32_t *)((uint32_t)(NRF_USBD) + 0x804)));
			} else {
				*((volatile uint32_t *)((uint32_t)(NRF_USBD) + 0x800)) = 0x7B4;
				uint8_t temp =
					*((volatile uint32_t *)((uint32_t)(NRF_USBD) + 0x804));
				temp |= (1U << 2);
				*((volatile uint32_t *)((uint32_t)(NRF_USBD) + 0x804)) |= temp;
				(void)(*((volatile uint32_t *)((uint32_t)(NRF_USBD) + 0x804)));
			}
		}
		if ((m_ep_dma_waiting | (~m_ep_ready)) & (1U << ep2bit(ep))) {
			/* Device -> Host */
			m_ep_dma_waiting &= ~(1U << ep2bit(ep));
			m_ep_ready |= 1U << ep2bit(ep);

			p_state->handler.feeder = NULL;
			p_state->status = NRF_USBD_COMMON_EP_ABORTED;
			NRF_USBD_COMMON_EP_TRANSFER_EVENT(evt, ep, NRF_USBD_COMMON_EP_ABORTED);
			m_event_handler(&evt);
		}
	}

	irq_unlock(irq_lock_key);
}

void nrf_usbd_common_ep_abort(nrf_usbd_common_ep_t ep)
{
	/* Only abort if there is no active DMA */
	k_sem_take(&dma_available, K_FOREVER);
	usbd_ep_abort(ep);
	k_sem_give(&dma_available);

	/* This function was holding DMA semaphore and could potentially prevent
	 * next DMA from executing. Fire IRQ handler to check if any DMA needs
	 * to be started.
	 */
	usbd_int_rise();
}

/**
 * @brief Abort all pending endpoints.
 *
 * Function aborts all pending endpoint transfers.
 */
static void usbd_ep_abort_all(void)
{
	uint32_t ep_waiting = m_ep_dma_waiting | (m_ep_ready & NRF_USBD_COMMON_EPOUT_BIT_MASK);

	while (ep_waiting != 0) {
		uint8_t bitpos = NRF_CTZ(ep_waiting);

		if (!NRF_USBD_EPISO_CHECK(bit2ep(bitpos))) {
			usbd_ep_abort(bit2ep(bitpos));
		}
		ep_waiting &= ~(1U << bitpos);
	}

	m_ep_ready = (((1U << NRF_USBD_EPIN_CNT) - 1U) << NRF_USBD_COMMON_EPIN_BITPOS_0);
}

/**
 * @brief Force the USBD interrupt into pending state.
 *
 * This function is used to force USBD interrupt to be processed right now.
 * It makes it possible to process all EasyDMA access on one thread priority level.
 */
static inline void usbd_int_rise(void)
{
	NVIC_SetPendingIRQ(USBD_IRQn);
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
	m_last_setup_dir = NRF_USBD_COMMON_EPOUT0;

	const nrf_usbd_common_evt_t evt = {.type = NRF_USBD_COMMON_EVT_RESET};

	m_event_handler(&evt);
}

static void ev_started_handler(void)
{
#if NRF_USBD_COMMON_STARTED_EV_ENABLE
	/* Handler not used by the stack.
	 * May be used for debugging.
	 */
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
	const nrf_usbd_common_ep_t ep = NRF_USBD_COMMON_EPIN0;

	LOG_DBG("USB event: DMA ready IN0");
	/* DMA finished, track if total bytes transferred is even or odd */
	m_dma_odd ^= nrf_usbd_ep_amount_get(NRF_USBD, ep) & 1;
	usbd_dma_pending_clear();
	k_sem_give(&dma_available);

	nrf_usbd_int_enable(NRF_USBD, NRF_USBD_INT_EP0SETUP_MASK);

	usbd_ep_state_t *p_state = ep_state_access(ep);

	if (p_state->status == NRF_USBD_COMMON_EP_ABORTED) {
		/* Clear transfer information just in case */
		(void)(atomic_and(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
	} else if (p_state->handler.feeder == NULL) {
		(void)(atomic_and(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
	} else {
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
static inline void nrf_usbd_epin_dma_handler(nrf_usbd_common_ep_t ep)
{
	LOG_DBG("USB event: DMA ready IN: %x", ep);
	__ASSERT_NO_MSG(NRF_USBD_EPIN_CHECK(ep));
	__ASSERT_NO_MSG(!NRF_USBD_EPISO_CHECK(ep));
	__ASSERT_NO_MSG(NRF_USBD_EP_NR_GET(ep) > 0);
	/* DMA finished, track if total bytes transferred is even or odd */
	m_dma_odd ^= nrf_usbd_ep_amount_get(NRF_USBD, ep) & 1;
	usbd_dma_pending_clear();
	k_sem_give(&dma_available);

	usbd_ep_state_t *p_state = ep_state_access(ep);

	if (p_state->status == NRF_USBD_COMMON_EP_ABORTED) {
		/* Clear transfer information just in case */
		(void)(atomic_and(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
	} else if (p_state->handler.feeder == NULL) {
		(void)(atomic_and(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
	} else {
		/* Nothing to do */
	}
}

/**
 * @brief Handler for EasyDMA event from in isochronous endpoint.
 */
static inline void nrf_usbd_epiniso_dma_handler(nrf_usbd_common_ep_t ep)
{
	if (NRF_USBD_COMMON_ISO_DEBUG) {
		LOG_DBG("USB event: DMA ready ISOIN: %x", ep);
	}
	__ASSERT_NO_MSG(NRF_USBD_EPIN_CHECK(ep));
	__ASSERT_NO_MSG(NRF_USBD_EPISO_CHECK(ep));
	/* DMA finished, track if total bytes transferred is even or odd */
	m_dma_odd ^= nrf_usbd_ep_amount_get(NRF_USBD, ep) & 1;
	usbd_dma_pending_clear();
	k_sem_give(&dma_available);

	usbd_ep_state_t *p_state = ep_state_access(ep);

	if (p_state->status == NRF_USBD_COMMON_EP_ABORTED) {
		/* Clear transfer information just in case */
		(void)(atomic_and(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
	} else if (p_state->handler.feeder == NULL) {
		(void)(atomic_and(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
		/* Send event to the user - for an ISO IN endpoint, the whole transfer is finished
		 * in this moment
		 */
		NRF_USBD_COMMON_EP_TRANSFER_EVENT(evt, ep, NRF_USBD_COMMON_EP_OK);
		m_event_handler(&evt);
	} else {
		/* Nothing to do */
	}
}

/**
 * @brief Handler for EasyDMA event for OUT endpoint 0.
 *
 * EP0 OUT have to be cleared automatically in special way - only in the middle of the transfer.
 * It cannot be cleared when required transfer is finished because it means the same that accepting
 * the comment.
 */
static inline void nrf_usbd_ep0out_dma_handler(void)
{
	const nrf_usbd_common_ep_t ep = NRF_USBD_COMMON_EPOUT0;

	LOG_DBG("USB event: DMA ready OUT0");
	/* DMA finished, track if total bytes transferred is even or odd */
	m_dma_odd ^= nrf_usbd_ep_amount_get(NRF_USBD, ep) & 1;
	usbd_dma_pending_clear();
	k_sem_give(&dma_available);

	nrf_usbd_int_enable(NRF_USBD, NRF_USBD_INT_EP0SETUP_MASK);

	usbd_ep_state_t *p_state = ep_state_access(ep);

	if (p_state->status == NRF_USBD_COMMON_EP_ABORTED) {
		/* Clear transfer information just in case */
		(void)(atomic_and(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
	} else if (p_state->handler.consumer == NULL) {
		(void)(atomic_and(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
		/* Send event to the user - for an OUT endpoint, the whole transfer is finished in
		 * this moment
		 */
		NRF_USBD_COMMON_EP_TRANSFER_EVENT(evt, ep, NRF_USBD_COMMON_EP_OK);
		m_event_handler(&evt);
	} else {
		nrf_usbd_common_setup_data_clear();
	}
}

/**
 * @brief Handler for EasyDMA event from endpoinpoint that requires clearing.
 *
 * This handler would be called when EasyDMA transfer for OUT endpoint has been finished.
 *
 * @param[in] ep Endpoint number.
 */
static inline void nrf_usbd_epout_dma_handler(nrf_usbd_common_ep_t ep)
{
	LOG_DBG("DMA ready OUT: %x", ep);
	__ASSERT_NO_MSG(NRF_USBD_EPOUT_CHECK(ep));
	__ASSERT_NO_MSG(!NRF_USBD_EPISO_CHECK(ep));
	__ASSERT_NO_MSG(NRF_USBD_EP_NR_GET(ep) > 0);
	/* DMA finished, track if total bytes transferred is even or odd */
	m_dma_odd ^= nrf_usbd_ep_amount_get(NRF_USBD, ep) & 1;
	usbd_dma_pending_clear();
	k_sem_give(&dma_available);

	usbd_ep_state_t *p_state = ep_state_access(ep);

	if (p_state->status == NRF_USBD_COMMON_EP_ABORTED) {
		/* Clear transfer information just in case */
		(void)(atomic_and(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
	} else if (p_state->handler.consumer == NULL) {
		(void)(atomic_and(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
		/* Send event to the user - for an OUT endpoint, the whole transfer is finished in
		 * this moment
		 */
		NRF_USBD_COMMON_EP_TRANSFER_EVENT(evt, ep, NRF_USBD_COMMON_EP_OK);
		m_event_handler(&evt);
	} else {
		/* Nothing to do */
	}
}

/**
 * @brief Handler for EasyDMA event from out isochronous endpoint.
 */
static inline void nrf_usbd_epoutiso_dma_handler(nrf_usbd_common_ep_t ep)
{
	if (NRF_USBD_COMMON_ISO_DEBUG) {
		LOG_DBG("DMA ready ISOOUT: %x", ep);
	}
	__ASSERT_NO_MSG(NRF_USBD_EPISO_CHECK(ep));
	/* DMA finished, track if total bytes transferred is even or odd */
	m_dma_odd ^= nrf_usbd_ep_amount_get(NRF_USBD, ep) & 1;
	usbd_dma_pending_clear();
	k_sem_give(&dma_available);

	usbd_ep_state_t *p_state = ep_state_access(ep);

	if (p_state->status == NRF_USBD_COMMON_EP_ABORTED) {
		/* Nothing to do - just ignore */
	} else if (p_state->handler.consumer == NULL) {
		(void)(atomic_and(&m_ep_dma_waiting, ~(1U << ep2bit(ep))));
		/* Send event to the user - for an OUT endpoint, the whole transfer is finished in
		 * this moment
		 */
		NRF_USBD_COMMON_EP_TRANSFER_EVENT(evt, ep, NRF_USBD_COMMON_EP_OK);
		m_event_handler(&evt);
	} else {
		/* Nothing to do */
	}
}

static void ev_dma_epin0_handler(void)
{
	nrf_usbd_ep0in_dma_handler();
}
static void ev_dma_epin1_handler(void)
{
	nrf_usbd_epin_dma_handler(NRF_USBD_COMMON_EPIN1);
}
static void ev_dma_epin2_handler(void)
{
	nrf_usbd_epin_dma_handler(NRF_USBD_COMMON_EPIN2);
}
static void ev_dma_epin3_handler(void)
{
	nrf_usbd_epin_dma_handler(NRF_USBD_COMMON_EPIN3);
}
static void ev_dma_epin4_handler(void)
{
	nrf_usbd_epin_dma_handler(NRF_USBD_COMMON_EPIN4);
}
static void ev_dma_epin5_handler(void)
{
	nrf_usbd_epin_dma_handler(NRF_USBD_COMMON_EPIN5);
}
static void ev_dma_epin6_handler(void)
{
	nrf_usbd_epin_dma_handler(NRF_USBD_COMMON_EPIN6);
}
static void ev_dma_epin7_handler(void)
{
	nrf_usbd_epin_dma_handler(NRF_USBD_COMMON_EPIN7);
}
static void ev_dma_epin8_handler(void)
{
	nrf_usbd_epiniso_dma_handler(NRF_USBD_COMMON_EPIN8);
}

static void ev_dma_epout0_handler(void)
{
	nrf_usbd_ep0out_dma_handler();
}
static void ev_dma_epout1_handler(void)
{
	nrf_usbd_epout_dma_handler(NRF_USBD_COMMON_EPOUT1);
}
static void ev_dma_epout2_handler(void)
{
	nrf_usbd_epout_dma_handler(NRF_USBD_COMMON_EPOUT2);
}
static void ev_dma_epout3_handler(void)
{
	nrf_usbd_epout_dma_handler(NRF_USBD_COMMON_EPOUT3);
}
static void ev_dma_epout4_handler(void)
{
	nrf_usbd_epout_dma_handler(NRF_USBD_COMMON_EPOUT4);
}
static void ev_dma_epout5_handler(void)
{
	nrf_usbd_epout_dma_handler(NRF_USBD_COMMON_EPOUT5);
}
static void ev_dma_epout6_handler(void)
{
	nrf_usbd_epout_dma_handler(NRF_USBD_COMMON_EPOUT6);
}
static void ev_dma_epout7_handler(void)
{
	nrf_usbd_epout_dma_handler(NRF_USBD_COMMON_EPOUT7);
}
static void ev_dma_epout8_handler(void)
{
	nrf_usbd_epoutiso_dma_handler(NRF_USBD_COMMON_EPOUT8);
}

static void ev_sof_handler(void)
{
	nrf_usbd_common_evt_t evt = {
		NRF_USBD_COMMON_EVT_SOF,
		.data = {.sof = {.framecnt = (uint16_t)nrf_usbd_framecntr_get(NRF_USBD)}}};

	/* Process isochronous endpoints */
	uint32_t iso_ready_mask = (1U << ep2bit(NRF_USBD_COMMON_EPIN8));

	if (nrf_usbd_episoout_size_get(NRF_USBD, NRF_USBD_COMMON_EPOUT8) !=
	    NRF_USBD_EPISOOUT_NO_DATA) {
		iso_ready_mask |= (1U << ep2bit(NRF_USBD_COMMON_EPOUT8));
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
static void usbd_ep_data_handler(nrf_usbd_common_ep_t ep, uint8_t bitpos)
{
	LOG_DBG("USBD event: EndpointData: %x", ep);
	/* Mark endpoint ready for next DMA access */
	m_ep_ready |= (1U << bitpos);

	if (NRF_USBD_EPIN_CHECK(ep)) {
		/* IN endpoint (Device -> Host) */

		/* Secure against the race condition that occurs when an IN transfer is interrupted
		 * by an OUT transaction, which in turn is interrupted by a process with higher
		 * priority. If the IN events ENDEPIN and EPDATA arrive during that high priority
		 * process, the OUT handler might call usbd_ep_data_handler without calling
		 * nrf_usbd_epin_dma_handler (or nrf_usbd_ep0in_dma_handler) for the IN transaction.
		 */
		if (nrf_usbd_event_get_and_clear(NRF_USBD, nrf_usbd_common_ep_to_endevent(ep))) {
			if (ep != NRF_USBD_COMMON_EPIN0) {
				nrf_usbd_epin_dma_handler(ep);
			} else {
				nrf_usbd_ep0in_dma_handler();
			}
		}

		if (0 == (m_ep_dma_waiting & (1U << bitpos))) {
			LOG_DBG("USBD event: EndpointData: In finished");
			/* No more data to be send - transmission finished */
			NRF_USBD_COMMON_EP_TRANSFER_EVENT(evt, ep, NRF_USBD_COMMON_EP_OK);
			m_event_handler(&evt);
		}
	} else {
		/* OUT endpoint (Host -> Device) */
		if (0 == (m_ep_dma_waiting & (1U << bitpos))) {
			LOG_DBG("USBD event: EndpointData: Out waiting");
			/* No buffer prepared - send event to the application */
			NRF_USBD_COMMON_EP_TRANSFER_EVENT(evt, ep, NRF_USBD_COMMON_EP_WAITING);
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
	LOG_DBG("USBD event: Setup (rt:%.2x r:%.2x v:%.4x i:%.4x l:%u )",
		       nrf_usbd_setup_bmrequesttype_get(NRF_USBD),
		       nrf_usbd_setup_brequest_get(NRF_USBD), nrf_usbd_setup_wvalue_get(NRF_USBD),
		       nrf_usbd_setup_windex_get(NRF_USBD), nrf_usbd_setup_wlength_get(NRF_USBD));
	uint8_t bmRequestType = nrf_usbd_setup_bmrequesttype_get(NRF_USBD);

	m_last_setup_dir =
		((bmRequestType & USBD_BMREQUESTTYPE_DIRECTION_Msk) ==
		 (USBD_BMREQUESTTYPE_DIRECTION_HostToDevice << USBD_BMREQUESTTYPE_DIRECTION_Pos))
			? NRF_USBD_COMMON_EPOUT0
			: NRF_USBD_COMMON_EPIN0;

	(void)(atomic_and(&m_ep_dma_waiting, ~((1U << ep2bit(NRF_USBD_COMMON_EPOUT0)) |
					       (1U << ep2bit(NRF_USBD_COMMON_EPIN0)))));
	m_ep_ready &= ~(1U << ep2bit(NRF_USBD_COMMON_EPOUT0));
	m_ep_ready |= 1U << ep2bit(NRF_USBD_COMMON_EPIN0);

	const nrf_usbd_common_evt_t evt = {.type = NRF_USBD_COMMON_EVT_SETUP};

	m_event_handler(&evt);
}

static void ev_usbevent_handler(void)
{
	uint32_t event = nrf_usbd_eventcause_get_and_clear(NRF_USBD);

	if (event & NRF_USBD_EVENTCAUSE_ISOOUTCRC_MASK) {
		LOG_DBG("USBD event: ISOOUTCRC");
		/* Currently no support */
	}
	if (event & NRF_USBD_EVENTCAUSE_SUSPEND_MASK) {
		LOG_DBG("USBD event: SUSPEND");
		m_bus_suspend = true;
		const nrf_usbd_common_evt_t evt = {.type = NRF_USBD_COMMON_EVT_SUSPEND};

		m_event_handler(&evt);
	}
	if (event & NRF_USBD_EVENTCAUSE_RESUME_MASK) {
		LOG_DBG("USBD event: RESUME");
		m_bus_suspend = false;
		const nrf_usbd_common_evt_t evt = {.type = NRF_USBD_COMMON_EVT_RESUME};

		m_event_handler(&evt);
	}
	if (event & NRF_USBD_EVENTCAUSE_WUREQ_MASK) {
		LOG_DBG("USBD event: WUREQ (%s)", m_bus_suspend ? "In Suspend" : "Active");
		if (m_bus_suspend) {
			__ASSERT_NO_MSG(!nrf_usbd_lowpower_check(NRF_USBD));
			m_bus_suspend = false;

			nrf_usbd_dpdmvalue_set(NRF_USBD, NRF_USBD_DPDMVALUE_RESUME);
			nrf_usbd_task_trigger(NRF_USBD, NRF_USBD_TASK_DRIVEDPDM);

			const nrf_usbd_common_evt_t evt = {.type = NRF_USBD_COMMON_EVT_WUREQ};

			m_event_handler(&evt);
		}
	}
}

static void ev_epdata_handler(void)
{
	/* Get all endpoints that have acknowledged transfer */
	uint32_t dataepstatus = nrf_usbd_epdatastatus_get_and_clear(NRF_USBD);

	LOG_DBG("USBD event: EndpointEPStatus: %x", dataepstatus);

	/* All finished endpoint have to be marked as busy */
	while (dataepstatus) {
		uint8_t bitpos = NRF_CTZ(dataepstatus);
		nrf_usbd_common_ep_t ep = bit2ep(bitpos);

		dataepstatus &= ~(1UL << bitpos);

		(void)(usbd_ep_data_handler(ep, bitpos));
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
	return NRF_CTZ(req);
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
static inline size_t usbd_ep_iso_capacity(nrf_usbd_common_ep_t ep)
{
	(void)ep;
	nrf_usbd_isosplit_t split = nrf_usbd_isosplit_get(NRF_USBD);

	if (split == NRF_USBD_ISOSPLIT_HALF) {
		return NRF_USBD_COMMON_ISOSIZE / 2;
	}
	return NRF_USBD_COMMON_ISOSIZE;
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
	if ((m_ep_dma_waiting & m_ep_ready) &&
	    (k_sem_take(&dma_available, K_NO_WAIT) == 0)) {
		uint32_t req;

		while (0 != (req = m_ep_dma_waiting & m_ep_ready)) {
			uint8_t pos;

			if (NRFX_USBD_CONFIG_DMASCHEDULER_ISO_BOOST &&
			    ((req & USBD_EPISO_BIT_MASK) != 0)) {
				pos = usbd_dma_scheduler_algorithm(req & USBD_EPISO_BIT_MASK);
			} else {
				pos = usbd_dma_scheduler_algorithm(req);
			}
			nrf_usbd_common_ep_t ep = bit2ep(pos);
			usbd_ep_state_t *p_state = ep_state_access(ep);

			nrf_usbd_common_ep_transfer_t transfer;
			bool continue_transfer;

			BUILD_ASSERT(offsetof(usbd_ep_state_t, handler.feeder) ==
				     offsetof(usbd_ep_state_t, handler.consumer),
				     "feeder and consumer must be in union");
			__ASSERT_NO_MSG((p_state->handler.feeder) != NULL);

			if (NRF_USBD_EPIN_CHECK(ep)) {
				/* Device -> Host */
				continue_transfer = p_state->handler.feeder(
					&transfer, p_state->p_context, p_state->max_packet_size);

				if (!continue_transfer) {
					p_state->handler.feeder = NULL;
				}
			} else {
				/* Host -> Device */
				const size_t rx_size = nrf_usbd_common_epout_size_get(ep);

				continue_transfer = p_state->handler.consumer(
					&transfer, p_state->p_context, p_state->max_packet_size,
					rx_size);

				if (transfer.p_data.rx == NULL) {
					/* Dropping transfer - allow processing */
					__ASSERT_NO_MSG(transfer.size == 0);
				} else if (transfer.size < rx_size) {
					LOG_DBG("Endpoint %x overload (r: %u, e: %u)", ep,
						       rx_size, transfer.size);
					p_state->status = NRF_USBD_COMMON_EP_OVERLOAD;
					(void)(atomic_and(&m_ep_dma_waiting, ~(1U << pos)));
					NRF_USBD_COMMON_EP_TRANSFER_EVENT(evt, ep,
						NRF_USBD_COMMON_EP_OVERLOAD);
					m_event_handler(&evt);
					/* This endpoint will not be transmitted now, repeat the
					 * loop
					 */
					continue;
				} else {
					/* Nothing to do - only check integrity if assertions are
					 * enabled
					 */
					__ASSERT_NO_MSG(transfer.size == rx_size);
				}

				if (!continue_transfer) {
					p_state->handler.consumer = NULL;
				}
			}

			usbd_dma_pending_set();
			m_ep_ready &= ~(1U << pos);
			if (NRF_USBD_COMMON_ISO_DEBUG || (!NRF_USBD_EPISO_CHECK(ep))) {
				LOG_DBG("USB DMA process: Starting transfer on EP: %x, size: %u",
					ep, transfer.size);
			}
			/* Update number of currently transferred bytes */
			p_state->transfer_cnt += transfer.size;
			/* Start transfer to the endpoint buffer */
			nrf_usbd_ep_easydma_set(NRF_USBD, ep, transfer.p_data.addr,
						(uint32_t)transfer.size);

			usbd_dma_start(ep);

			/* Do not process SETUP packet until EP0 DMA completes.
			 * DMA transfer itself will always complete regardless
			 * of host actions and this action is solely to prevent
			 * the need to abort active DMA.
			 */
			if ((ep == NRF_USBD_COMMON_EPOUT0) || (ep == NRF_USBD_COMMON_EPIN0)) {
				nrf_usbd_int_disable(NRF_USBD, NRF_USBD_INT_EP0SETUP_MASK);
			}

			/* Transfer started - exit the loop */
			return;
		}
		k_sem_give(&dma_available);
	} else {
		if (NRF_USBD_COMMON_DMAREQ_PROCESS_DEBUG) {
			LOG_DBG("USB DMA process - EasyDMA busy");
		}
	}
}

/**
 * @brief Wait for a specified eventcause and clear it afterwards.
 */
static inline void usbd_eventcause_wait_and_clear(nrf_usbd_eventcause_mask_t eventcause)
{
	while (0 == (eventcause & nrf_usbd_eventcause_get(NRF_USBD))) {
		/* Empty loop */
	}
	nrf_usbd_eventcause_clear(NRF_USBD, eventcause);
}

/**
 * @brief Begin errata 171.
 */
static inline void usbd_errata_171_begin(void)
{
	unsigned int irq_lock_key = irq_lock();

	if (*((volatile uint32_t *)(0x4006EC00)) == 0x00000000) {
		*((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
		*((volatile uint32_t *)(0x4006EC14)) = 0x000000C0;
		*((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
	} else {
		*((volatile uint32_t *)(0x4006EC14)) = 0x000000C0;
	}

	irq_unlock(irq_lock_key);
}

/**
 * @brief End errata 171.
 */
static inline void usbd_errata_171_end(void)
{
	unsigned int irq_lock_key = irq_lock();

	if (*((volatile uint32_t *)(0x4006EC00)) == 0x00000000) {
		*((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
		*((volatile uint32_t *)(0x4006EC14)) = 0x00000000;
		*((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
	} else {
		*((volatile uint32_t *)(0x4006EC14)) = 0x00000000;
	}

	irq_unlock(irq_lock_key);
}

/**
 * @brief Begin erratas 187 and 211.
 */
static inline void usbd_errata_187_211_begin(void)
{
	unsigned int irq_lock_key = irq_lock();

	if (*((volatile uint32_t *)(0x4006EC00)) == 0x00000000) {
		*((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
		*((volatile uint32_t *)(0x4006ED14)) = 0x00000003;
		*((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
	} else {
		*((volatile uint32_t *)(0x4006ED14)) = 0x00000003;
	}

	irq_unlock(irq_lock_key);
}

/**
 * @brief End erratas 187 and 211.
 */
static inline void usbd_errata_187_211_end(void)
{
	unsigned int irq_lock_key = irq_lock();

	if (*((volatile uint32_t *)(0x4006EC00)) == 0x00000000) {
		*((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
		*((volatile uint32_t *)(0x4006ED14)) = 0x00000000;
		*((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
	} else {
		*((volatile uint32_t *)(0x4006ED14)) = 0x00000000;
	}

	irq_unlock(irq_lock_key);
}

/**
 * @brief Enable USBD peripheral.
 */
static void usbd_enable(void)
{
	if (nrf_usbd_common_errata_187()) {
		usbd_errata_187_211_begin();
	}

	if (nrf_usbd_common_errata_171()) {
		usbd_errata_171_begin();
	}

	/* Enable the peripheral */
	nrf_usbd_enable(NRF_USBD);

	/* Waiting for peripheral to enable, this should take a few us */
	usbd_eventcause_wait_and_clear(NRF_USBD_EVENTCAUSE_READY_MASK);

	if (nrf_usbd_common_errata_171()) {
		usbd_errata_171_end();
	}

	if (nrf_usbd_common_errata_187()) {
		usbd_errata_187_211_end();
	}
}
/** @} */

/**
 * @brief USBD interrupt service routines.
 *
 */
static const nrfx_irq_handler_t m_isr[] = {[USBD_INTEN_USBRESET_Pos] = ev_usbreset_handler,
					   [USBD_INTEN_STARTED_Pos] = ev_started_handler,
					   [USBD_INTEN_ENDEPIN0_Pos] = ev_dma_epin0_handler,
					   [USBD_INTEN_ENDEPIN1_Pos] = ev_dma_epin1_handler,
					   [USBD_INTEN_ENDEPIN2_Pos] = ev_dma_epin2_handler,
					   [USBD_INTEN_ENDEPIN3_Pos] = ev_dma_epin3_handler,
					   [USBD_INTEN_ENDEPIN4_Pos] = ev_dma_epin4_handler,
					   [USBD_INTEN_ENDEPIN5_Pos] = ev_dma_epin5_handler,
					   [USBD_INTEN_ENDEPIN6_Pos] = ev_dma_epin6_handler,
					   [USBD_INTEN_ENDEPIN7_Pos] = ev_dma_epin7_handler,
					   [USBD_INTEN_EP0DATADONE_Pos] = ev_setup_data_handler,
					   [USBD_INTEN_ENDISOIN_Pos] = ev_dma_epin8_handler,
					   [USBD_INTEN_ENDEPOUT0_Pos] = ev_dma_epout0_handler,
					   [USBD_INTEN_ENDEPOUT1_Pos] = ev_dma_epout1_handler,
					   [USBD_INTEN_ENDEPOUT2_Pos] = ev_dma_epout2_handler,
					   [USBD_INTEN_ENDEPOUT3_Pos] = ev_dma_epout3_handler,
					   [USBD_INTEN_ENDEPOUT4_Pos] = ev_dma_epout4_handler,
					   [USBD_INTEN_ENDEPOUT5_Pos] = ev_dma_epout5_handler,
					   [USBD_INTEN_ENDEPOUT6_Pos] = ev_dma_epout6_handler,
					   [USBD_INTEN_ENDEPOUT7_Pos] = ev_dma_epout7_handler,
					   [USBD_INTEN_ENDISOOUT_Pos] = ev_dma_epout8_handler,
					   [USBD_INTEN_SOF_Pos] = ev_sof_handler,
					   [USBD_INTEN_USBEVENT_Pos] = ev_usbevent_handler,
					   [USBD_INTEN_EP0SETUP_Pos] = ev_setup_handler,
					   [USBD_INTEN_EPDATA_Pos] = ev_epdata_handler};

/**
 * @name Interrupt handlers
 *
 * @{
 */
void nrf_usbd_common_irq_handler(void)
{
	const uint32_t enabled = nrf_usbd_int_enable_get(NRF_USBD);
	uint32_t to_process = enabled & ~NRF_USBD_INT_EP0SETUP_MASK;
	uint32_t active = 0;

	/* Check all enabled interrupts */
	while (to_process) {
		uint8_t event_nr = NRF_CTZ(to_process);

		if (nrf_usbd_event_get_and_clear(
			    NRF_USBD, (nrf_usbd_event_t)nrfx_bitpos_to_event(event_nr))) {
			active |= 1UL << event_nr;
		}
		to_process &= ~(1UL << event_nr);
	}

	/* Process the active interrupts */
	while (active) {
		uint8_t event_nr = NRF_CTZ(active);

		m_isr[event_nr]();
		active &= ~(1UL << event_nr);
	}
	usbd_dmareq_process();

	/* Handle SETUP only if there is no active DMA on EP0 */
	if (nrf_usbd_int_enable_check(NRF_USBD, NRF_USBD_INT_EP0SETUP_MASK) &&
	    nrf_usbd_event_get_and_clear(NRF_USBD, NRF_USBD_EVENT_EP0SETUP)) {
		m_isr[USBD_INTEN_EP0SETUP_Pos]();
	}
}

/** @} */
/** @} */

nrfx_err_t nrf_usbd_common_init(nrf_usbd_common_event_handler_t event_handler)
{
	__ASSERT_NO_MSG(event_handler);

	if (m_drv_state != NRFX_DRV_STATE_UNINITIALIZED) {
		return NRFX_ERROR_INVALID_STATE;
	}

	m_event_handler = event_handler;
	m_drv_state = NRFX_DRV_STATE_INITIALIZED;

	uint8_t n;

	for (n = 0; n < NRF_USBD_EPIN_CNT; ++n) {
		nrf_usbd_common_ep_t ep = NRF_USBD_COMMON_EPIN(n);

		nrf_usbd_common_ep_max_packet_size_set(ep, NRF_USBD_EPISO_CHECK(ep) ?
			(NRF_USBD_COMMON_ISOSIZE / 2) : NRF_USBD_COMMON_EPSIZE);
		usbd_ep_state_t *p_state = ep_state_access(ep);

		p_state->status = NRF_USBD_COMMON_EP_OK;
		p_state->handler.feeder = NULL;
		p_state->transfer_cnt = 0;
	}
	for (n = 0; n < NRF_USBD_EPOUT_CNT; ++n) {
		nrf_usbd_common_ep_t ep = NRF_USBD_COMMON_EPOUT(n);

		nrf_usbd_common_ep_max_packet_size_set(ep, NRF_USBD_EPISO_CHECK(ep) ?
			(NRF_USBD_COMMON_ISOSIZE / 2) : NRF_USBD_COMMON_EPSIZE);
		usbd_ep_state_t *p_state = ep_state_access(ep);

		p_state->status = NRF_USBD_COMMON_EP_OK;
		p_state->handler.consumer = NULL;
		p_state->transfer_cnt = 0;
	}

	return NRFX_SUCCESS;
}

void nrf_usbd_common_uninit(void)
{
	__ASSERT_NO_MSG(m_drv_state == NRFX_DRV_STATE_INITIALIZED);

	m_event_handler = NULL;
	m_drv_state = NRFX_DRV_STATE_UNINITIALIZED;
}

void nrf_usbd_common_enable(void)
{
	__ASSERT_NO_MSG(m_drv_state == NRFX_DRV_STATE_INITIALIZED);

	/* Prepare for READY event receiving */
	nrf_usbd_eventcause_clear(NRF_USBD, NRF_USBD_EVENTCAUSE_READY_MASK);

	usbd_enable();

	if (nrf_usbd_common_errata_223() && m_first_enable) {
		nrf_usbd_disable(NRF_USBD);

		usbd_enable();

		m_first_enable = false;
	}

#if NRF_USBD_COMMON_USE_WORKAROUND_FOR_ANOMALY_211
	if (nrf_usbd_common_errata_187() || nrf_usbd_common_errata_211())
#else
	if (nrf_usbd_common_errata_187())
#endif
	{
		usbd_errata_187_211_begin();
	}

	if (nrf_usbd_common_errata_166()) {
		*((volatile uint32_t *)((uint32_t)(NRF_USBD) + 0x800)) = 0x7E3;
		*((volatile uint32_t *)((uint32_t)(NRF_USBD) + 0x804)) = 0x40;
		__ISB();
		__DSB();
	}

	nrf_usbd_isosplit_set(NRF_USBD, NRF_USBD_ISOSPLIT_HALF);

	if (IS_ENABLED(CONFIG_NRF_USBD_ISO_IN_ZLP)) {
		nrf_usbd_common_isoinconfig_set(NRF_USBD_ISOINCONFIG_ZERODATA);
	} else {
		nrf_usbd_common_isoinconfig_set(NRF_USBD_ISOINCONFIG_NORESP);
	}

	m_ep_ready = (((1U << NRF_USBD_EPIN_CNT) - 1U) << NRF_USBD_COMMON_EPIN_BITPOS_0);
	m_ep_dma_waiting = 0;
	m_dma_odd = 0;
	__ASSERT_NO_MSG(k_sem_count_get(&dma_available) == 1);
	usbd_dma_pending_clear();
	m_last_setup_dir = NRF_USBD_COMMON_EPOUT0;

	m_drv_state = NRFX_DRV_STATE_POWERED_ON;

#if NRF_USBD_COMMON_USE_WORKAROUND_FOR_ANOMALY_211
	if (nrf_usbd_common_errata_187() && !nrf_usbd_common_errata_211())
#else
	if (nrf_usbd_common_errata_187())
#endif
	{
		usbd_errata_187_211_end();
	}
}

void nrf_usbd_common_disable(void)
{
	__ASSERT_NO_MSG(m_drv_state != NRFX_DRV_STATE_UNINITIALIZED);

	/* Make sure DMA is not active */
	k_sem_take(&dma_available, K_FOREVER);

	/* Stop just in case */
	nrf_usbd_common_stop();

	/* Disable all parts */
	nrf_usbd_int_disable(NRF_USBD, nrf_usbd_int_enable_get(NRF_USBD));
	if (m_dma_odd) {
		/* Prevent invalid bus request after next USBD enable by ensuring
		 * that total number of bytes transferred by DMA is even.
		 */
		nrf_usbd_event_clear(NRF_USBD, NRF_USBD_EVENT_ENDEPIN0);
		nrf_usbd_ep_easydma_set(NRF_USBD, NRF_USBD_COMMON_EPIN0, (uint32_t)&m_dma_odd, 1);
		usbd_dma_start(NRF_USBD_COMMON_EPIN0);
		while (!nrf_usbd_event_get_and_clear(NRF_USBD, NRF_USBD_EVENT_ENDEPIN0)) {
		}
		m_dma_odd = 0;
	}
	nrf_usbd_disable(NRF_USBD);
	usbd_dma_pending_clear();
	k_sem_give(&dma_available);
	m_drv_state = NRFX_DRV_STATE_INITIALIZED;

#if NRF_USBD_COMMON_USE_WORKAROUND_FOR_ANOMALY_211
	if (nrf_usbd_common_errata_211()) {
		usbd_errata_187_211_end();
	}
#endif
}

void nrf_usbd_common_start(bool enable_sof)
{
	__ASSERT_NO_MSG(m_drv_state == NRFX_DRV_STATE_POWERED_ON);
	m_bus_suspend = false;

	uint32_t ints_to_enable = NRF_USBD_INT_USBRESET_MASK | NRF_USBD_INT_STARTED_MASK |
				  NRF_USBD_INT_ENDEPIN0_MASK | NRF_USBD_INT_EP0DATADONE_MASK |
				  NRF_USBD_INT_ENDEPOUT0_MASK | NRF_USBD_INT_USBEVENT_MASK |
				  NRF_USBD_INT_EP0SETUP_MASK | NRF_USBD_INT_DATAEP_MASK;

	if (enable_sof) {
		ints_to_enable |= NRF_USBD_INT_SOF_MASK;
	}

	/* Enable all required interrupts */
	nrf_usbd_int_enable(NRF_USBD, ints_to_enable);

	/* Enable interrupt globally */
	irq_enable(USBD_IRQn);

	/* Enable pullups */
	nrf_usbd_pullup_enable(NRF_USBD);
}

static void nrf_usbd_common_stop(void)
{
	__ASSERT_NO_MSG(m_drv_state == NRFX_DRV_STATE_POWERED_ON);

	/* Clear interrupt */
	NVIC_ClearPendingIRQ(USBD_IRQn);

	if (irq_is_enabled(USBD_IRQn)) {
		/* Abort transfers */
		usbd_ep_abort_all();

		/* Disable pullups */
		nrf_usbd_pullup_disable(NRF_USBD);

		/* Disable interrupt globally */
		irq_disable(USBD_IRQn);

		/* Disable all interrupts */
		nrf_usbd_int_disable(NRF_USBD, ~0U);
	}
}

bool nrf_usbd_common_is_initialized(void)
{
	return (m_drv_state >= NRFX_DRV_STATE_INITIALIZED);
}

bool nrf_usbd_common_is_enabled(void)
{
	return (m_drv_state >= NRFX_DRV_STATE_POWERED_ON);
}

bool nrf_usbd_common_is_started(void)
{
	return (nrf_usbd_common_is_enabled() && irq_is_enabled(USBD_IRQn));
}

bool nrf_usbd_common_suspend(void)
{
	bool suspended = false;
	unsigned int irq_lock_key = irq_lock();

	if (m_bus_suspend) {
		if (!(nrf_usbd_eventcause_get(NRF_USBD) & NRF_USBD_EVENTCAUSE_RESUME_MASK)) {
			nrf_usbd_lowpower_enable(NRF_USBD);
			if (nrf_usbd_eventcause_get(NRF_USBD) & NRF_USBD_EVENTCAUSE_RESUME_MASK) {
				nrf_usbd_lowpower_disable(NRF_USBD);
			} else {
				suspended = true;
			}
		}
	}

	irq_unlock(irq_lock_key);

	return suspended;
}

bool nrf_usbd_common_wakeup_req(void)
{
	bool started = false;
	unsigned int irq_lock_key = irq_lock();

	if (m_bus_suspend && nrf_usbd_lowpower_check(NRF_USBD)) {
		nrf_usbd_lowpower_disable(NRF_USBD);
		started = true;

		if (nrf_usbd_common_errata_171()) {
			if (*((volatile uint32_t *)(0x4006EC00)) == 0x00000000) {
				*((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
				*((volatile uint32_t *)(0x4006EC14)) = 0x000000C0;
				*((volatile uint32_t *)(0x4006EC00)) = 0x00009375;
			} else {
				*((volatile uint32_t *)(0x4006EC14)) = 0x000000C0;
			}
		}
	}

	irq_unlock(irq_lock_key);

	return started;
}

bool nrf_usbd_common_suspend_check(void)
{
	return nrf_usbd_lowpower_check(NRF_USBD);
}

void nrf_usbd_common_suspend_irq_config(void)
{
	nrf_usbd_int_disable(NRF_USBD, m_irq_disabled_in_suspend);
}

void nrf_usbd_common_active_irq_config(void)
{
	nrf_usbd_int_enable(NRF_USBD, m_irq_disabled_in_suspend);
}

bool nrf_usbd_common_bus_suspend_check(void)
{
	return m_bus_suspend;
}

void nrf_usbd_common_force_bus_wakeup(void)
{
	m_bus_suspend = false;
}

void nrf_usbd_common_ep_max_packet_size_set(nrf_usbd_common_ep_t ep, uint16_t size)
{
	/* Only the power of 2 size allowed for Control Endpoints */
	__ASSERT_NO_MSG((((size & (size - 1)) == 0) || (NRF_USBD_EP_NR_GET(ep) != 0)));
	/* Only non zero size allowed for Control Endpoints */
	__ASSERT_NO_MSG((size != 0) || (NRF_USBD_EP_NR_GET(ep) != 0));
	/* Packet size cannot be higher than maximum buffer size */
	__ASSERT_NO_MSG((NRF_USBD_EPISO_CHECK(ep) && (size <= usbd_ep_iso_capacity(ep))) ||
		    (!NRF_USBD_EPISO_CHECK(ep) && (size <= NRF_USBD_COMMON_EPSIZE)));

	usbd_ep_state_t *p_state = ep_state_access(ep);

	p_state->max_packet_size = size;
}

uint16_t nrf_usbd_common_ep_max_packet_size_get(nrf_usbd_common_ep_t ep)
{
	usbd_ep_state_t const *p_state = ep_state_access(ep);

	return p_state->max_packet_size;
}

bool nrf_usbd_common_ep_enable_check(nrf_usbd_common_ep_t ep)
{
	return nrf_usbd_ep_enable_check(NRF_USBD, ep_to_hal(ep));
}

void nrf_usbd_common_ep_enable(nrf_usbd_common_ep_t ep)
{
	nrf_usbd_int_enable(NRF_USBD, nrf_usbd_common_ep_to_int(ep));

	if (nrf_usbd_ep_enable_check(NRF_USBD, ep)) {
		return;
	}
	nrf_usbd_ep_enable(NRF_USBD, ep_to_hal(ep));
	if ((NRF_USBD_EP_NR_GET(ep) != 0) && NRF_USBD_EPOUT_CHECK(ep) &&
	    !NRF_USBD_EPISO_CHECK(ep)) {
		unsigned int irq_lock_key = irq_lock();

		nrf_usbd_common_transfer_out_drop(ep);
		m_ep_dma_waiting &= ~(1U << ep2bit(ep));

		irq_unlock(irq_lock_key);
	}
}

void nrf_usbd_common_ep_disable(nrf_usbd_common_ep_t ep)
{
	/* Only disable endpoint if there is no active DMA */
	k_sem_take(&dma_available, K_FOREVER);
	usbd_ep_abort(ep);
	nrf_usbd_ep_disable(NRF_USBD, ep_to_hal(ep));
	nrf_usbd_int_disable(NRF_USBD, nrf_usbd_common_ep_to_int(ep));
	k_sem_give(&dma_available);

	/* This function was holding DMA semaphore and could potentially prevent
	 * next DMA from executing. Fire IRQ handler to check if any DMA needs
	 * to be started.
	 */
	usbd_int_rise();
}

void nrf_usbd_common_ep_default_config(void)
{
	nrf_usbd_int_disable(NRF_USBD,
			     NRF_USBD_INT_ENDEPIN1_MASK | NRF_USBD_INT_ENDEPIN2_MASK |
				     NRF_USBD_INT_ENDEPIN3_MASK | NRF_USBD_INT_ENDEPIN4_MASK |
				     NRF_USBD_INT_ENDEPIN5_MASK | NRF_USBD_INT_ENDEPIN6_MASK |
				     NRF_USBD_INT_ENDEPIN7_MASK | NRF_USBD_INT_ENDISOIN0_MASK |
				     NRF_USBD_INT_ENDEPOUT1_MASK | NRF_USBD_INT_ENDEPOUT2_MASK |
				     NRF_USBD_INT_ENDEPOUT3_MASK | NRF_USBD_INT_ENDEPOUT4_MASK |
				     NRF_USBD_INT_ENDEPOUT5_MASK | NRF_USBD_INT_ENDEPOUT6_MASK |
				     NRF_USBD_INT_ENDEPOUT7_MASK | NRF_USBD_INT_ENDISOOUT0_MASK);
	nrf_usbd_int_enable(NRF_USBD, NRF_USBD_INT_ENDEPIN0_MASK | NRF_USBD_INT_ENDEPOUT0_MASK);
	nrf_usbd_ep_default_config(NRF_USBD);
}

nrfx_err_t nrf_usbd_common_ep_transfer(nrf_usbd_common_ep_t ep,
				       nrf_usbd_common_transfer_t const *p_transfer)
{
	nrfx_err_t ret;
	const uint8_t ep_bitpos = ep2bit(ep);
	unsigned int irq_lock_key = irq_lock();

	__ASSERT_NO_MSG(p_transfer != NULL);

	/* Setup data transaction can go only in one direction at a time */
	if ((NRF_USBD_EP_NR_GET(ep) == 0) && (ep != m_last_setup_dir)) {
		ret = NRFX_ERROR_INVALID_ADDR;
		if (NRF_USBD_COMMON_FAILED_TRANSFERS_DEBUG &&
		    (NRF_USBD_COMMON_ISO_DEBUG || (!NRF_USBD_EPISO_CHECK(ep)))) {
			LOG_DBG("Transfer failed: Invalid EPr\n");
		}
	} else if ((m_ep_dma_waiting | ((~m_ep_ready) & NRF_USBD_COMMON_EPIN_BIT_MASK)) &
		   (1U << ep_bitpos)) {
		/* IN (Device -> Host) transfer has to be transmitted out to allow new transmission
		 */
		ret = NRFX_ERROR_BUSY;
		if (NRF_USBD_COMMON_FAILED_TRANSFERS_DEBUG) {
			LOG_DBG("Transfer failed: EP is busy");
		}
	} else {
		usbd_ep_state_t *p_state = ep_state_access(ep);
		/* Prepare transfer context and handler description */
		nrf_usbd_common_transfer_t *p_context;

		if (NRF_USBD_EPIN_CHECK(ep)) {
			p_context = m_ep_feeder_state + NRF_USBD_EP_NR_GET(ep);
			if (nrfx_is_in_ram(p_transfer->p_data.tx)) {
				/* RAM */
				if (0 == (p_transfer->flags & NRF_USBD_COMMON_TRANSFER_ZLP_FLAG)) {
					p_state->handler.feeder = nrf_usbd_common_feeder_ram;
					if (NRF_USBD_COMMON_ISO_DEBUG ||
					    (!NRF_USBD_EPISO_CHECK(ep))) {
						LOG_DBG("Transfer called on endpoint %x, "
							"size: %u, mode: "
							"RAM",
							ep, p_transfer->size);
					}
				} else {
					p_state->handler.feeder = nrf_usbd_common_feeder_ram_zlp;
					if (NRF_USBD_COMMON_ISO_DEBUG ||
					    (!NRF_USBD_EPISO_CHECK(ep))) {
						LOG_DBG("Transfer called on endpoint %x, "
							"size: %u, mode: "
							"RAM_ZLP",
							ep, p_transfer->size);
					}
				}
			} else {
				/* Flash */
				if (0 == (p_transfer->flags & NRF_USBD_COMMON_TRANSFER_ZLP_FLAG)) {
					p_state->handler.feeder = nrf_usbd_common_feeder_flash;
					if (NRF_USBD_COMMON_ISO_DEBUG ||
					    (!NRF_USBD_EPISO_CHECK(ep))) {
						LOG_DBG("Transfer called on endpoint %x, "
							"size: %u, mode: "
							"FLASH",
							ep, p_transfer->size);
					}
				} else {
					p_state->handler.feeder = nrf_usbd_common_feeder_flash_zlp;
					if (NRF_USBD_COMMON_ISO_DEBUG ||
					    (!NRF_USBD_EPISO_CHECK(ep))) {
						LOG_DBG("Transfer called on endpoint %x, "
							"size: %u, mode: "
							"FLASH_ZLP",
							ep, p_transfer->size);
					}
				}
			}
		} else {
			p_context = m_ep_consumer_state + NRF_USBD_EP_NR_GET(ep);
			__ASSERT_NO_MSG((p_transfer->p_data.rx == NULL) ||
				    (nrfx_is_in_ram(p_transfer->p_data.rx)));
			p_state->handler.consumer = nrf_usbd_common_consumer;
		}
		*p_context = *p_transfer;
		p_state->p_context = p_context;

		p_state->transfer_cnt = 0;
		p_state->status = NRF_USBD_COMMON_EP_OK;
		m_ep_dma_waiting |= 1U << ep_bitpos;
		ret = NRFX_SUCCESS;
		usbd_int_rise();
	}

	irq_unlock(irq_lock_key);

	return ret;
}

nrfx_err_t nrf_usbd_common_ep_handled_transfer(nrf_usbd_common_ep_t ep,
					 nrf_usbd_common_handler_desc_t const *p_handler)
{
	nrfx_err_t ret;
	const uint8_t ep_bitpos = ep2bit(ep);
	unsigned int irq_lock_key = irq_lock();

	__ASSERT_NO_MSG(p_handler != NULL);

	/* Setup data transaction can go only in one direction at a time */
	if ((NRF_USBD_EP_NR_GET(ep) == 0) && (ep != m_last_setup_dir)) {
		ret = NRFX_ERROR_INVALID_ADDR;
		if (NRF_USBD_COMMON_FAILED_TRANSFERS_DEBUG &&
		    (NRF_USBD_COMMON_ISO_DEBUG || (!NRF_USBD_EPISO_CHECK(ep)))) {
			LOG_DBG("Transfer failed: Invalid EP");
		}
	} else if ((m_ep_dma_waiting | ((~m_ep_ready) & NRF_USBD_COMMON_EPIN_BIT_MASK)) &
		   (1U << ep_bitpos)) {
		/* IN (Device -> Host) transfer has to be transmitted out to allow a new
		 * transmission
		 */
		ret = NRFX_ERROR_BUSY;
		if (NRF_USBD_COMMON_FAILED_TRANSFERS_DEBUG &&
		    (NRF_USBD_COMMON_ISO_DEBUG || (!NRF_USBD_EPISO_CHECK(ep)))) {
			LOG_DBG("Transfer failed: EP is busy");
		}
	} else {
		/* Transfer can be configured now */
		usbd_ep_state_t *p_state = ep_state_access(ep);

		p_state->transfer_cnt = 0;
		p_state->handler = p_handler->handler;
		p_state->p_context = p_handler->p_context;
		p_state->status = NRF_USBD_COMMON_EP_OK;
		m_ep_dma_waiting |= 1U << ep_bitpos;

		ret = NRFX_SUCCESS;
		if (NRF_USBD_COMMON_ISO_DEBUG || (!NRF_USBD_EPISO_CHECK(ep))) {
			LOG_DBG("Transfer called on endpoint %x, mode: Handler", ep);
		}
		usbd_int_rise();
	}

	irq_unlock(irq_lock_key);

	return ret;
}

void *nrf_usbd_common_feeder_buffer_get(void)
{
	return m_tx_buffer;
}

nrf_usbd_common_ep_status_t nrf_usbd_common_ep_status_get(nrf_usbd_common_ep_t ep, size_t *p_size)
{
	nrf_usbd_common_ep_status_t ret;
	usbd_ep_state_t const *p_state = ep_state_access(ep);
	unsigned int irq_lock_key = irq_lock();

	*p_size = p_state->transfer_cnt;
	ret = (p_state->handler.consumer == NULL) ? p_state->status : NRF_USBD_COMMON_EP_BUSY;

	irq_unlock(irq_lock_key);

	return ret;
}

size_t nrf_usbd_common_epout_size_get(nrf_usbd_common_ep_t ep)
{
	return nrf_usbd_epout_size_get(NRF_USBD, ep_to_hal(ep));
}

bool nrf_usbd_common_ep_is_busy(nrf_usbd_common_ep_t ep)
{
	return (0 != ((m_ep_dma_waiting | ((~m_ep_ready) & NRF_USBD_COMMON_EPIN_BIT_MASK)) &
		      (1U << ep2bit(ep))));
}

void nrf_usbd_common_ep_stall(nrf_usbd_common_ep_t ep)
{
	LOG_DBG("USB: EP %x stalled.", ep);
	nrf_usbd_ep_stall(NRF_USBD, ep_to_hal(ep));
}

void nrf_usbd_common_ep_stall_clear(nrf_usbd_common_ep_t ep)
{
	if (NRF_USBD_EPOUT_CHECK(ep) && nrf_usbd_common_ep_stall_check(ep)) {
		nrf_usbd_common_transfer_out_drop(ep);
	}
	nrf_usbd_ep_unstall(NRF_USBD, ep_to_hal(ep));
}

bool nrf_usbd_common_ep_stall_check(nrf_usbd_common_ep_t ep)
{
	return nrf_usbd_ep_is_stall(NRF_USBD, ep_to_hal(ep));
}

void nrf_usbd_common_ep_dtoggle_clear(nrf_usbd_common_ep_t ep)
{
	nrf_usbd_dtoggle_set(NRF_USBD, ep, NRF_USBD_DTOGGLE_DATA0);
}

void nrf_usbd_common_setup_get(nrf_usbd_common_setup_t *p_setup)
{
	memset(p_setup, 0, sizeof(nrf_usbd_common_setup_t));
	p_setup->bmRequestType = nrf_usbd_setup_bmrequesttype_get(NRF_USBD);
	p_setup->bRequest = nrf_usbd_setup_brequest_get(NRF_USBD);
	p_setup->wValue = nrf_usbd_setup_wvalue_get(NRF_USBD);
	p_setup->wIndex = nrf_usbd_setup_windex_get(NRF_USBD);
	p_setup->wLength = nrf_usbd_setup_wlength_get(NRF_USBD);
}

void nrf_usbd_common_setup_data_clear(void)
{
	nrf_usbd_task_trigger(NRF_USBD, NRF_USBD_TASK_EP0RCVOUT);
}

void nrf_usbd_common_setup_clear(void)
{
	LOG_DBG(">> ep0status >>");
	nrf_usbd_task_trigger(NRF_USBD, NRF_USBD_TASK_EP0STATUS);
}

void nrf_usbd_common_setup_stall(void)
{
	LOG_DBG("Setup stalled.");
	nrf_usbd_task_trigger(NRF_USBD, NRF_USBD_TASK_EP0STALL);
}

nrf_usbd_common_ep_t nrf_usbd_common_last_setup_dir_get(void)
{
	return m_last_setup_dir;
}

void nrf_usbd_common_transfer_out_drop(nrf_usbd_common_ep_t ep)
{
	unsigned int irq_lock_key = irq_lock();

	__ASSERT_NO_MSG(NRF_USBD_EPOUT_CHECK(ep));

	m_ep_ready &= ~(1U << ep2bit(ep));
	if (!NRF_USBD_EPISO_CHECK(ep)) {
		nrf_usbd_epout_clear(NRF_USBD, ep);
	}

	irq_unlock(irq_lock_key);
}

/** @endcond */
