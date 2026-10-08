/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* The radio model takes one Tx or Rx at a time, at an absolute time of its
 * 1MHz counter, which is also the ticker counter, so the roles schedule radio
 * operations as a timer triggered radio would and chain the next one from the
 * end callback in the radio ISR. The model has no ramp-up or Tx/Rx chain
 * delay: the times here and in the events are on air times.
 */

#include "bs_2g4_radio_if.h"

/* An operation that is stopped, or that is requested too late to start, ends
 * with BSR_STATUS_ABORTED, so that a role ends its event in one place.
 */
typedef void (*lll_radio_cb_t)(const struct bsr_evt *evt, void *param);

void lll_radio_init(void);
void lll_radio_isr(void);

/* The PDU is read when the transmission starts, as a radio with DMA would,
 * so that an AuxPtr offset filled in after the request is still sent.
 */
void lll_radio_tx(const struct bsr_pkt_cfg *cfg, uint32_t at, const void *pdu,
		  lll_radio_cb_t cb, void *param);

/* With a window_us that is not 0, the Rx ends without a PDU unless an access
 * address has been received within window_us from start. buf needs room for
 * the PDU header and cfg->max_len payload octets.
 */
void lll_radio_rx(const struct bsr_pkt_cfg *cfg, uint32_t start, uint32_t window_us,
		  void *buf, lll_radio_cb_t cb, void *param);

/* cb runs from the radio ISR, as the end of the stopped operation would, so
 * that a role ends its event in the same context whether or not it was
 * stopped.
 */
void lll_radio_stop(lll_radio_cb_t cb, void *param);

/* For an event that has already ended, where no callback is wanted */
void lll_radio_abort(void);

static inline uint32_t lll_radio_now(void)
{
	return bsr_cntr_get();
}

/* Legacy PDUs (PHY_LEGACY) are on the 1M PHY */
static inline uint8_t lll_radio_phy(uint8_t phy)
{
	return (phy == PHY_2M) ? BSR_PHY_2M : BSR_PHY_1M;
}

/* The response to a PDU may start up to the active clock jitter early or late
 * (Core Spec Vol 6, Part B, Section 4.2.1), and the peer may be up to the
 * range delay away.
 */
static inline uint32_t lll_radio_tifs_rx_start(uint32_t tx_end_us, uint16_t tifs_us)
{
	return tx_end_us + tifs_us - EVENT_CLOCK_JITTER_US;
}

static inline uint32_t lll_radio_tifs_rx_window(uint8_t phy)
{
	return (EVENT_CLOCK_JITTER_US << 1) + RANGE_DELAY_US + addr_us_get(phy);
}
