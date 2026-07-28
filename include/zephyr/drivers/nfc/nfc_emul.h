/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_NFC_NFC_EMUL_H_
#define ZEPHYR_INCLUDE_DRIVERS_NFC_NFC_EMUL_H_

#include <zephyr/device.h>
#include <zephyr/drivers/nfc.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Emulated NFC controller backend API
 * @defgroup nfc_emul Emulated NFC controller
 * @ingroup io_emulators
 * @ingroup nfc_interface
 * @{
 *
 * The emulated controller has no hardware behind it. It exists so the NFC
 * driver-class contract can be exercised on a host build.
 *
 * Which subset of @ref nfc_driver_api an instance implements is fixed at build
 * time by its @c backend devicetree property, so that the three controller
 * classes the API is designed to cover are each represented by a device that
 * really does lack the other classes' operations:
 *
 * - @c "offload" only offers @c offload_poll_start / @c offload_poll_stop /
 *   @c offload_exchange, like a controller that resolves and activates targets
 *   in firmware.
 * - @c "initiator" only offers the host-driven reader operations, including
 *   @c im_transceive.
 * - @c "target" only offers the card-emulation operations.
 *
 * Exchanges are driven by a script: the test states the frames it expects to
 * be transmitted and the responses to hand back, then calls the regular NFC
 * API and checks that the script was consumed exactly.
 */

/**
 * @brief One scripted exchange.
 *
 * A script entry is matched against a single transmit call. Set
 * @ref nfc_emul_frame.tx to NULL to accept any payload.
 */
struct nfc_emul_frame {
	/** Expected transmit payload, or NULL to accept any. */
	const uint8_t *tx;
	/** Length of @ref nfc_emul_frame.tx. */
	uint16_t tx_len;
	/**
	 * Expected number of valid bits in the last transmitted byte. Only
	 * checked when @ref nfc_emul_frame.tx is set, and compared modulo 8 so
	 * that 0 and 8 both mean a whole byte, as the hardware drivers treat
	 * them.
	 */
	uint8_t tx_last_bits;
	/** Response payload handed back to the caller. */
	const uint8_t *rx;
	/** Length of @ref nfc_emul_frame.rx. */
	uint16_t rx_len;
	/** Value the transmit call returns. Non-zero suppresses the response. */
	int ret;
};

/**
 * @brief Load the script of expected exchanges.
 *
 * Resets the script position. @p frames must stay valid until the script is
 * replaced.
 *
 * @param dev Emulated NFC device.
 * @param frames Expected exchanges, or NULL to clear the script.
 * @param count Number of entries in @p frames.
 */
void nfc_emul_load_script(const struct device *dev, const struct nfc_emul_frame *frames,
			  size_t count);

/**
 * @brief Get how many scripted exchanges have not been consumed.
 *
 * A test that ran to completion should see zero.
 *
 * @param dev Emulated NFC device.
 *
 * @return Number of remaining entries.
 */
size_t nfc_emul_script_remaining(const struct device *dev);

/**
 * @brief Number of times the RF field has been switched off.
 *
 * @param dev Emulated NFC device.
 *
 * @return Number of off transitions since the device was initialised.
 */
uint32_t nfc_emul_field_cycles(const struct device *dev);

/**
 * @brief Stage the target an offloading controller reports.
 *
 * If discovery is already running the callback registered with
 * nfc_offload_poll_start() is invoked before returning; otherwise the target is
 * reported when discovery next starts.
 *
 * @param dev Emulated NFC device.
 * @param target Target to report, or NULL to stage nothing.
 *
 * @retval 0 on success.
 * @retval -ENOTSUP if @p dev is not an offload instance.
 */
int nfc_emul_set_target(const struct device *dev, const struct nfc_target *target);

/**
 * @brief Whether the staged target has been released by an offloading caller.
 *
 * @param dev Emulated NFC device.
 */
bool nfc_emul_target_released(const struct device *dev);

/**
 * @brief Raise a target-mode event.
 *
 * Invokes the callback registered with nfc_target_start().
 *
 * @param dev Emulated NFC device.
 * @param event Event to report.
 * @param data Frame for @ref NFC_TARGET_FRAME, otherwise NULL.
 * @param len Length of @p data.
 *
 * @retval 0 on success.
 * @retval -ENOTSUP if @p dev is not a target instance.
 * @retval -EPERM if target mode has not been started.
 */
int nfc_emul_raise_target_event(const struct device *dev, enum nfc_target_event event,
				const uint8_t *data, uint16_t len);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_NFC_NFC_EMUL_H_ */
