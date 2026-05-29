/** @file
 *  @brief Bluetooth HCI Event utilities.
 *
 *  Copyright (c) 2026 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_BLUETOOTH_HCI_UTIL_HCI_EVT_H_
#define ZEPHYR_DRIVERS_BLUETOOTH_HCI_UTIL_HCI_EVT_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/net_buf.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Per-HCI-device state for chain-aware extended advertising discard tracking. */
struct hci_ext_adv_discard_ctx {
	bt_addr_le_t addr;
	uint8_t sid;
	bool active;
};

/**
 * @brief Process an HCI event that may be an LE Extended Advertising Report.
 *
 * If the event is an LE Extended Advertising Report, perform chain-aware
 * discard handling on each individual report in the event.
 * For every report:
 *
 *  - If the advertiser (addr, SID) matches a tracked discarded chain in @p ctx,
 *    the report is dropped. When the report has a non-PARTIAL data status the
 *    chain tracker is cleared.
 *  - Otherwise the report is kept.
 *
 * The function then produces the output buffer:
 *
 *  - All reports kept: the event is copied into a new event buffer.
 *  - All reports dropped: @c *out is left as @c NULL; the caller must treat the
 *    event as consumed.
 *  - Subset of reports kept: the event is repacked into a new event buffer with
 *    the remaining reports and an adjusted length.
 *
 * If no event buffer is available, all kept reports are dropped and any PARTIAL
 * chains among them are registered in @p ctx so their subsequent fragments are
 * dropped. Registering a new PARTIAL chain while another is active evicts the
 * previous one (and logs a warning) and the evicted chain's remaining fragments
 * may leak into the host. This eviction is a fail-safe that prevents the tracker
 * from getting permanently stuck on a chain whose terminator never arrives.
 *
 * Call this before the driver's normal HCI event receive path. If this returns
 * @c false, the event is either invalid or not an LE Extended Advertising Report;
 * use the normal event allocation path.

 * @note The implementation assumes that the Controller only passes one partial extended
 *       advertising report at a time.
 *
 * @param[in,out] ctx  Per-HCI-device discard tracking state.
 * @param[in]     data Raw HCI event starting at the HCI event header.
 * @param[in]     len  Length of @p data in octets.
 * @param[out]    out  Must point to storage holding @c NULL on entry. When the
 *                     function returns @c true, @c *out is either @c NULL
 *                     (event consumed internally) or a
 *                     buffer ready for @ref bt_hci_recv. Unmodified when the
 *                     function returns @c false.
 *
 * @retval true  LE Extended Advertising Report path handled; see @p out.
 * @retval false Not an LE Extended Advertising Report; use normal event processing.
 */
bool hci_ext_adv_report_process(struct hci_ext_adv_discard_ctx *ctx, const uint8_t *data,
				size_t len, struct net_buf **out);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_BLUETOOTH_HCI_UTIL_HCI_EVT_H_ */
