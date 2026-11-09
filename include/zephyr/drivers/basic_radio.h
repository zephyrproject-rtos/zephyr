/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public Basic Radio driver API
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_BASIC_RADIO_H_
#define ZEPHYR_INCLUDE_DRIVERS_BASIC_RADIO_H_

/**
 * @brief Basic Radio APIs
 * @defgroup basic_radio_api Basic Radio APIs
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Optional radio features. Bitmask returned by basic_radio_get_caps(). */
enum basic_radio_capability {
	BASIC_RADIO_CAP_CCA         = BIT(1),
	/* others to be added */
};

/**
 * @brief Runtime radio configuration.
 *
 * phy points to a backend-specific PHY object that fully defines the
 * waveform and frame format. Its type is declared by the backend header
 * (for example struct basic_radio_cc23x0_phy in
 * <zephyr/drivers/basic_radio/ti/cc23x0.h>) and is opaque to this API.
 * NULL keeps the active PHY.
 *
 * frequency, sync_word, max_packet_len and tx_power are the common runtime
 * parameters every backend applies on top of the PHY.
 */
struct basic_radio_config {
	const void *phy;                  /**< Backend PHY object, or NULL to keep the active PHY. */
	uint32_t frequency;               /**< Carrier frequency in Hz. */
	uint32_t sync_word;               /**< Sync pattern, LSB-aligned. */
	uint8_t  max_packet_len;          /**< Maximum payload length in bytes. */
	int8_t   tx_power;                /**< TX power in dBm. */
};

/** @brief Per-packet RX metadata. */
struct basic_radio_rx_info {
	int8_t   rssi;                    /**< Received signal strength in dBm. */
	uint32_t timestamp;               /**< Driver-defined ticks; 0 if unsupported. */
	bool     crc_ok;
};

/* --- Layer 2: per-operation adverb flags ---------------------------------- */

/** @brief TX adverbs passed to basic_radio_send().
 *
 *  Pass NULL for params (or set flags = 0) to use Layer 1 defaults from the
 *  last basic_radio_config() call.  Each BIT opts into exactly one override;
 *  unknown bits cause basic_radio_send() to return -ENOTSUP.
 */
#define BASIC_RADIO_TX_FLAG_SCHED_ABS  BIT(0) /*< Schedule TX at abs_start_time.  Default: immediate. */
#define BASIC_RADIO_TX_FLAG_CHANNEL    BIT(1) /*< Use frequency_hz instead of Layer 1 default. */
#define BASIC_RADIO_TX_FLAG_POWER      BIT(2) /*< Use tx_power_dbm instead of Layer 1 default. */
#define BASIC_RADIO_TX_FLAG_SYNC_WORD  BIT(3) /*< Use sync_word instead of Layer 1 default. */

struct basic_radio_tx_params {
	uint32_t flags;
	/* Absolute radio-timer ticks. Used only when BASIC_RADIO_TX_FLAG_SCHED_ABS is set. */
	uint32_t abs_start_time;
	/* Carrier frequency in Hz. Used only when BASIC_RADIO_TX_FLAG_CHANNEL is set. */
	uint32_t frequency_hz;
	/* TX power in dBm. Used only when BASIC_RADIO_TX_FLAG_POWER is set. */
	int8_t   tx_power_dbm;
	/* Sync word. Used only when BASIC_RADIO_TX_FLAG_SYNC_WORD is set. */
	uint32_t sync_word;
};

/** @brief RX adverbs passed to basic_radio_recv().
 *
 *  Pass NULL for params (or set flags = 0) to use Layer 1 defaults from the
 *  last basic_radio_config() call.  Unknown bits cause basic_radio_recv() to return
 *  -ENOTSUP.
 */
#define BASIC_RADIO_RX_FLAG_SCHED_ABS     BIT(0) /**< Start RX at abs_start_time.  Default: immediate. */
#define BASIC_RADIO_RX_FLAG_ONE_SHOT      BIT(1) /**< Stop after first packet.  Default: continuous. */
#define BASIC_RADIO_RX_FLAG_DISCARD_PAYL  BIT(2) /**< Discard payload bytes (PER test).  Default: keep. */
#define BASIC_RADIO_RX_FLAG_CHANNEL       BIT(3) /**< Use frequency_hz instead of Layer 1 default. */
#define BASIC_RADIO_RX_FLAG_SYNC_WORD     BIT(4) /**< Use sync_word instead of Layer 1 default. */
#define BASIC_RADIO_RX_FLAG_MAX_LEN       BIT(5) /**< Cap received payload at max_packet_len bytes. */

struct basic_radio_rx_params {
	uint32_t flags;
	/** Absolute radio-timer ticks. Used only when BASIC_RADIO_RX_FLAG_SCHED_ABS is set. */
	uint32_t abs_start_time;
	/** Carrier frequency in Hz. Used only when BASIC_RADIO_RX_FLAG_CHANNEL is set. */
	uint32_t frequency_hz;
	/** Sync word. Used only when BASIC_RADIO_RX_FLAG_SYNC_WORD is set. */
	uint32_t sync_word;
	/** Maximum payload length cap in bytes. Used only when BASIC_RADIO_RX_FLAG_MAX_LEN is set. */
	uint16_t max_packet_len;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * Driver API - implemented by each backend, consumed by inline wrappers below.
 */
__subsystem struct basic_radio_driver_api {
	/* Main Functions */
	int      (*config)(const struct device *dev, const struct basic_radio_config *cfg);
	int      (*send)(const struct device *dev, const uint8_t *data, uint16_t len, const struct basic_radio_tx_params *params, k_timeout_t timeout);
	int      (*recv)(const struct device *dev, uint8_t *buf, uint16_t buf_len, const struct basic_radio_rx_params *params, k_timeout_t timeout, struct basic_radio_rx_info *info);
	int      (*test_cw)(const struct device *dev, uint32_t frequency, int8_t tx_power, uint16_t duration_ms);
	uint32_t (*get_caps)(const struct device *dev);
	/* Optional Functions */
	/* Vendor-Specific */
	int      (*ext_ctl)(const struct device *dev, uint32_t cmd, void *arg, size_t arg_sz);
};
/** @endcond */

/**
 * @brief Apply a runtime radio configuration.
 *
 * Selects the PHY object in @p cfg->phy, if not NULL, and applies the
 * runtime parameters on top of it. @p cfg->phy must point to an object of
 * the type defined by the backend of @p dev. Changing to a PHY with another
 * radio configuration may reinitialize the radio. The radio must be idle.
 *
 * @param dev  Radio device.
 * @param cfg  Configuration to apply.
 *
 * @retval 0        Success.
 * @retval -EBUSY   Radio is not idle.
 * @retval -EINVAL  A parameter is out of range for the selected PHY, or
 *                  @p cfg->phy is NULL and no PHY is active.
 * @retval -ENOTSUP The backend cannot drive the PHY in @p cfg->phy.
 * @retval -EIO     Radio reconfiguration failed.
 */
static inline int basic_radio_config(const struct device *dev, const struct basic_radio_config *cfg)
{
	const struct basic_radio_driver_api *api = dev->api;

	return api->config(dev, cfg);
}

/**
 * @brief Transmit a packet, blocking until the frame is on the air.
 *
 * @param dev      Radio device.
 * @param data     Payload to transmit.
 * @param len      Length of @p data in bytes.
 * @param timeout  Maximum time to wait for TX completion.
 *
 * @return 0 on success, negative errno on failure.
 */
static inline int basic_radio_send(const struct device *dev, const uint8_t *data, uint16_t len, const struct basic_radio_tx_params *params, k_timeout_t timeout)
{
	const struct basic_radio_driver_api *api = dev->api;
	return api->send(dev, data, len, params, timeout);
}

/**
 * @brief Receive one packet, blocking until data arrives or @p timeout expires.
 *
 * @param dev      Radio device.
 * @param buf      Destination buffer.
 * @param buf_len  Size of @p buf in bytes.
 * @param timeout  Maximum wait time.
 * @param info     Optional RX metadata (may be NULL).
 *
 * @return Number of bytes received on success, negative errno on failure.
 */
static inline int basic_radio_recv(const struct device *dev, uint8_t *buf, uint16_t buf_len,
			       const struct basic_radio_rx_params *params, k_timeout_t timeout,
			       struct basic_radio_rx_info *info)
{
	const struct basic_radio_driver_api *api = dev->api;

	return api->recv(dev, buf, buf_len, params, timeout, info);
}

/**
 * @brief Query supported optional features.
 *
 * @return Bitmask of @ref basic_radio_capability values.
 */
static inline uint32_t basic_radio_get_caps(const struct device *dev)
{
	const struct basic_radio_driver_api *api = dev->api;

	return api->get_caps(dev);
}

/**
 * @brief Transmit an unmodulated continuous wave for test / regulatory work.
 *
 * @param dev          Radio device.
 * @param frequency    Output frequency in Hz.
 * @param tx_power     TX power in dBm.
 * @param duration_ms  Transmission duration in milliseconds.
 *
 * @retval 0        The CW transmission was scheduled.
 * @retval -ENOSYS  The backend does not support CW test mode.
 */
static inline int basic_radio_test_cw(const struct device *dev, uint32_t frequency, int8_t tx_power, uint16_t duration_ms)
{
	const struct basic_radio_driver_api *api = dev->api;

	if (api->test_cw == NULL)
	{
		return -ENOSYS;
	}
	return api->test_cw(dev, frequency, tx_power, duration_ms);
}

/**
 * @brief Vendor-specific escape hatch. @c cmd space is per-backend.
 *
 * @retval 0        Command handled.
 * @retval -ENOSYS  Backend does not implement ext_ctl.
 * @retval -ENOTSUP Backend does not recognize @p cmd.
 */
static inline int basic_radio_ext_ctl(const struct device *dev, uint32_t cmd, void *arg, size_t arg_sz)
{
	const struct basic_radio_driver_api *api = dev->api;

	if (api->ext_ctl == NULL) {
		return -ENOSYS;
	}
	return api->ext_ctl(dev, cmd, arg, arg_sz);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_BASIC_RADIO_H_ */
