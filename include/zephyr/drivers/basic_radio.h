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

/**
 * @brief Frequency unit of the API in Hz.
 *
 * Every carrier frequency is a uint32_t in units of 10 Hz, which covers up
 * to 42.9 GHz.
 */
#define BASIC_RADIO_FREQ_UNIT_HZ 10U

/** @brief Optional radio features, in basic_radio_caps::flags. */
enum basic_radio_capability {
	/** Clear channel assessment. */
	BASIC_RADIO_CAP_CCA           = BIT(1),
	/** Scheduled start of TX, RX and tone operations. */
	BASIC_RADIO_CAP_SCHED         = BIT(2),
	/** Unmodulated tone, see basic_radio_tone_async(). */
	BASIC_RADIO_CAP_TONE          = BIT(3),
	/** BASIC_RADIO_STOP_GRACEFUL. */
	BASIC_RADIO_CAP_STOP_GRACEFUL = BIT(4),
};

/**
 * @brief Radio capabilities with the active PHY.
 *
 * Frequencies are in units of @ref BASIC_RADIO_FREQ_UNIT_HZ.
 */
struct basic_radio_caps {
	/** Bitmask of @ref basic_radio_capability values. */
	uint32_t flags;
	/** Lowest carrier frequency. */
	uint32_t freq_min;
	/** Highest carrier frequency. */
	uint32_t freq_max;
	/** Synthesizer tuning step in millihertz. */
	uint32_t freq_resolution_millihz;
	/**
	 * Minimum time in microseconds from the end of one operation to the
	 * scheduled start of the next one on another frequency.
	 */
	uint32_t retune_time_us;
	/** Maximum lead time of a scheduled start in microseconds. */
	uint32_t sched_horizon_us;
	/** Lowest TX power in dBm. */
	int8_t tx_power_min_dbm;
	/** Highest TX power in dBm. */
	int8_t tx_power_max_dbm;
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
	uint64_t sync_word;               /**< Sync pattern, LSB-aligned. */
	const void *phy;                  /**< Backend PHY object, or NULL to keep the active PHY. */
	uint32_t frequency;               /**< Carrier frequency, 10 Hz units. */
	uint8_t  max_packet_len;          /**< Maximum payload length in bytes. */
	int8_t   tx_power;                /**< TX power in dBm. */
};

/** @brief Per-packet RX metadata. */
struct basic_radio_rx_info {
	uint64_t timestamp_us;            /**< Time of the packet on the radio time base; 0 if unsupported. */
	int8_t   rssi;                    /**< Received signal strength in dBm. */
	bool     crc_ok;
};

/* --- Layer 2: per-operation adverb flags ---------------------------------- */

/** @brief TX adverbs passed to basic_radio_send().
 *
 *  Pass NULL for params (or set flags = 0) to use Layer 1 defaults from the
 *  last basic_radio_config() call.  Each BIT opts into exactly one override;
 *  unknown bits cause basic_radio_send() to return -ENOTSUP.
 */
#define BASIC_RADIO_TX_FLAG_SCHED_ABS  BIT(0) /*< Schedule TX at abs_start_time_us.  Default: immediate. */
#define BASIC_RADIO_TX_FLAG_CHANNEL    BIT(1) /*< Use frequency instead of Layer 1 default. */
#define BASIC_RADIO_TX_FLAG_POWER      BIT(2) /*< Use tx_power_dbm instead of Layer 1 default. */
#define BASIC_RADIO_TX_FLAG_SYNC_WORD  BIT(3) /*< Use sync_word instead of Layer 1 default. */

struct basic_radio_tx_params {
	/* Start time on the radio time base. Used only when BASIC_RADIO_TX_FLAG_SCHED_ABS is set. */
	uint64_t abs_start_time_us;
	/* Sync word. Used only when BASIC_RADIO_TX_FLAG_SYNC_WORD is set. */
	uint64_t sync_word;
	uint32_t flags;
	/* Carrier frequency, 10 Hz units. Used only when BASIC_RADIO_TX_FLAG_CHANNEL is set. */
	uint32_t frequency;
	/* TX power in dBm. Used only when BASIC_RADIO_TX_FLAG_POWER is set. */
	int8_t   tx_power_dbm;
};

/** @brief RX adverbs passed to basic_radio_recv().
 *
 *  Pass NULL for params (or set flags = 0) to use Layer 1 defaults from the
 *  last basic_radio_config() call.  Unknown bits cause basic_radio_recv() to return
 *  -ENOTSUP.
 */
#define BASIC_RADIO_RX_FLAG_SCHED_ABS     BIT(0) /**< Start RX at abs_start_time_us.  Default: immediate. */
#define BASIC_RADIO_RX_FLAG_ONE_SHOT      BIT(1) /**< Stop after first packet.  Default: continuous. */
#define BASIC_RADIO_RX_FLAG_DISCARD_PAYL  BIT(2) /**< Discard payload bytes (PER test).  Default: keep. */
#define BASIC_RADIO_RX_FLAG_CHANNEL       BIT(3) /**< Use frequency instead of Layer 1 default. */
#define BASIC_RADIO_RX_FLAG_SYNC_WORD     BIT(4) /**< Use sync_word instead of Layer 1 default. */
#define BASIC_RADIO_RX_FLAG_MAX_LEN       BIT(5) /**< Cap received payload at max_packet_len bytes. */

struct basic_radio_rx_params {
	/** Start time on the radio time base. Used only when BASIC_RADIO_RX_FLAG_SCHED_ABS is set. */
	uint64_t abs_start_time_us;
	/** Sync word. Used only when BASIC_RADIO_RX_FLAG_SYNC_WORD is set. */
	uint64_t sync_word;
	uint32_t flags;
	/** Carrier frequency, 10 Hz units. Used only when BASIC_RADIO_RX_FLAG_CHANNEL is set. */
	uint32_t frequency;
	/** Maximum payload length cap in bytes. Used only when BASIC_RADIO_RX_FLAG_MAX_LEN is set. */
	uint16_t max_packet_len;
};

/** @brief Tone adverbs passed to basic_radio_tone_async(). */
#define BASIC_RADIO_TONE_FLAG_SCHED_ABS   BIT(0) /**< Start at abs_start_time_us.  Default: immediate. */

/** @brief Unmodulated tone operation. */
struct basic_radio_tone_params {
	/** Start time on the radio time base. Used only when BASIC_RADIO_TONE_FLAG_SCHED_ABS is set. */
	uint64_t abs_start_time_us;
	uint32_t flags;
	/** Tone frequency, 10 Hz units. */
	uint32_t frequency;
	/** Tone duration in microseconds from the start; 0: until basic_radio_stop(). */
	uint32_t duration_us;
	/** TX power in dBm. */
	int8_t   tx_power_dbm;
};

/**
 * @brief How basic_radio_stop() ends the active operation.
 *
 * Every backend supports BASIC_RADIO_STOP_ABORT.
 */
enum basic_radio_stop_mode {
	/** Cancel the operation only if it has not started on air yet. */
	BASIC_RADIO_STOP_DESCHEDULE,
	/** Finish the packet in progress, then end the operation. */
	BASIC_RADIO_STOP_GRACEFUL,
	/** End the operation as soon as possible, scheduled or running. */
	BASIC_RADIO_STOP_ABORT,
};

/** @brief Event types reported to a @ref basic_radio_callback_t. */
enum basic_radio_evt_type {
	/** Final event of a TX operation. */
	BASIC_RADIO_EVT_TX_DONE,
	/** One packet received. Continuous RX reports one event per packet. */
	BASIC_RADIO_EVT_RX_PKT,
	/** Final event of an RX operation. */
	BASIC_RADIO_EVT_RX_DONE,
	/** Final event of a tone operation. */
	BASIC_RADIO_EVT_TONE_DONE,
};

/** @brief Event of an asynchronous operation. */
struct basic_radio_evt {
	enum basic_radio_evt_type type;
	/**
	 * Final events: 0 if the operation ended normally, -ECANCELED if it
	 * was ended by basic_radio_stop(), -ETIME if the scheduled start was
	 * missed, -EIO on a radio error.
	 * Always 0 for BASIC_RADIO_EVT_RX_PKT.
	 */
	int status;
	/**
	 * Payload. BASIC_RADIO_EVT_RX_PKT only, valid during the callback only.
	 * NULL with BASIC_RADIO_RX_FLAG_DISCARD_PAYL.
	 */
	const uint8_t *data;
	/** Payload length in bytes. BASIC_RADIO_EVT_RX_PKT only, 0 if @p data is NULL. */
	uint16_t len;
	/** Packet metadata. BASIC_RADIO_EVT_RX_PKT only. */
	struct basic_radio_rx_info info;
};

/**
 * @brief Event callback of an asynchronous operation.
 *
 * Runs in interrupt context and must not block. It may start the next
 * operation or call basic_radio_stop().
 *
 * @param dev        Radio device.
 * @param evt        Event, valid during the callback only.
 * @param user_data  Pointer passed when the operation was started.
 */
typedef void (*basic_radio_callback_t)(const struct device *dev, const struct basic_radio_evt *evt,
				       void *user_data);

/**
 * @cond INTERNAL_HIDDEN
 *
 * Driver API - implemented by each backend, consumed by inline wrappers below.
 */
__subsystem struct basic_radio_driver_api {
	/* Main Functions */
	int      (*config)(const struct device *dev, const struct basic_radio_config *cfg);
	int      (*send_async)(const struct device *dev, const uint8_t *data, uint16_t len,
			       const struct basic_radio_tx_params *params, basic_radio_callback_t cb,
			       void *user_data);
	int      (*recv_async)(const struct device *dev, const struct basic_radio_rx_params *params,
			       basic_radio_callback_t cb, void *user_data);
	int      (*tone_async)(const struct device *dev, const struct basic_radio_tone_params *params,
			       basic_radio_callback_t cb, void *user_data);
	int      (*stop)(const struct device *dev, enum basic_radio_stop_mode mode);
	uint64_t (*time_get)(const struct device *dev);
	int      (*get_caps)(const struct device *dev, struct basic_radio_caps *caps);
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
 * @brief Start a TX operation.
 *
 * The radio runs one operation at a time. The operation ends with exactly
 * one BASIC_RADIO_EVT_TX_DONE event, which may arrive before this function
 * returns. The radio is idle again when @p cb runs.
 *
 * @param dev        Radio device.
 * @param data       Payload to transmit. Copied, may be reused on return.
 * @param len        Length of @p data in bytes.
 * @param params     TX adverbs, or NULL for the configured defaults.
 * @param cb         Event callback.
 * @param user_data  Pointer passed to @p cb.
 *
 * @retval 0        Operation started.
 * @retval -EINVAL  Invalid length, parameter out of range, start time beyond
 *                  the scheduling horizon, or @p cb is NULL.
 * @retval -ENOTSUP Unsupported flag in @p params.
 * @retval -ENODEV  No active PHY.
 * @retval -EBUSY   Another operation is active.
 * @retval -ETIME   The scheduled start time has passed.
 * @retval -EIO     The radio rejected the operation.
 */
static inline int basic_radio_send_async(const struct device *dev, const uint8_t *data,
					 uint16_t len, const struct basic_radio_tx_params *params,
					 basic_radio_callback_t cb, void *user_data)
{
	const struct basic_radio_driver_api *api = dev->api;

	return api->send_async(dev, data, len, params, cb, user_data);
}

/**
 * @brief Start an RX operation.
 *
 * Each received packet is reported with BASIC_RADIO_EVT_RX_PKT. The
 * operation ends with exactly one BASIC_RADIO_EVT_RX_DONE event: after the
 * first packet with BASIC_RADIO_RX_FLAG_ONE_SHOT, otherwise only on
 * basic_radio_stop() or a radio error.
 *
 * @param dev        Radio device.
 * @param params     RX adverbs, or NULL for the configured defaults.
 * @param cb         Event callback.
 * @param user_data  Pointer passed to @p cb.
 *
 * @retval 0        Operation started.
 * @retval -EINVAL  Parameter out of range, start time beyond the scheduling
 *                  horizon, or @p cb is NULL.
 * @retval -ENOTSUP Unsupported flag in @p params.
 * @retval -ENODEV  No active PHY.
 * @retval -EBUSY   Another operation is active.
 * @retval -ETIME   The scheduled start time has passed.
 * @retval -EIO     The radio rejected the operation.
 */
static inline int basic_radio_recv_async(const struct device *dev,
					 const struct basic_radio_rx_params *params,
					 basic_radio_callback_t cb, void *user_data)
{
	const struct basic_radio_driver_api *api = dev->api;

	return api->recv_async(dev, params, cb, user_data);
}

/**
 * @brief Start an unmodulated tone.
 *
 * The operation ends with exactly one BASIC_RADIO_EVT_TONE_DONE event,
 * after @p params->duration_us or on basic_radio_stop().
 *
 * @param dev        Radio device.
 * @param params     Tone parameters.
 * @param cb         Event callback.
 * @param user_data  Pointer passed to @p cb.
 *
 * @retval 0        Operation started.
 * @retval -EINVAL  Parameter out of range, start time beyond the scheduling
 *                  horizon, or @p cb is NULL.
 * @retval -ENOTSUP Unsupported flag in @p params.
 * @retval -ENOSYS  The backend does not support tones.
 * @retval -ENODEV  No active PHY.
 * @retval -EBUSY   Another operation is active.
 * @retval -ETIME   The scheduled start time has passed.
 * @retval -EIO     The radio rejected the operation.
 */
static inline int basic_radio_tone_async(const struct device *dev,
					 const struct basic_radio_tone_params *params,
					 basic_radio_callback_t cb, void *user_data)
{
	const struct basic_radio_driver_api *api = dev->api;

	if (api->tone_async == NULL) {
		return -ENOSYS;
	}

	return api->tone_async(dev, params, cb, user_data);
}

/**
 * @brief Stop the active operation.
 *
 * Does not block and may be called from interrupt context. The final event
 * of the operation reports -ECANCELED once the stop takes effect.
 *
 * @param dev   Radio device.
 * @param mode  How to end the operation.
 *
 * @retval 0         Stop requested.
 * @retval -EALREADY No operation is active, or it has already ended.
 * @retval -EBUSY    BASIC_RADIO_STOP_DESCHEDULE and the operation has
 *                   started. It keeps running.
 * @retval -EINVAL   Invalid @p mode.
 * @retval -ENOTSUP  The backend does not support @p mode.
 */
static inline int basic_radio_stop(const struct device *dev, enum basic_radio_stop_mode mode)
{
	const struct basic_radio_driver_api *api = dev->api;

	return api->stop(dev, mode);
}

/**
 * @brief Transmit a packet, blocking until the operation ends.
 *
 * On timeout the operation is aborted. basic_radio_stop() from another
 * context ends the call with -ECANCELED.
 *
 * @param dev      Radio device.
 * @param data     Payload to transmit.
 * @param len      Length of @p data in bytes.
 * @param params   TX adverbs, or NULL for the configured defaults.
 * @param timeout  Maximum time to wait for TX completion.
 *
 * @retval 0          Packet sent.
 * @retval -ETIMEDOUT TX did not end within @p timeout.
 * @retval -ECANCELED Stopped by basic_radio_stop().
 * @retval -ETIME     The scheduled start time was missed.
 * @retval -EIO       Radio error.
 * @return Other negative errno values of basic_radio_send_async().
 */
int basic_radio_send(const struct device *dev, const uint8_t *data, uint16_t len,
		     const struct basic_radio_tx_params *params, k_timeout_t timeout);

/**
 * @brief Receive one packet, blocking until data arrives or @p timeout expires.
 *
 * The RX operation is aborted after the first packet or on timeout.
 * basic_radio_stop() from another context ends the call with -ECANCELED.
 *
 * @param dev      Radio device.
 * @param buf      Destination buffer.
 * @param buf_len  Size of @p buf in bytes. Longer payloads are truncated.
 * @param params   RX adverbs, or NULL for the configured defaults.
 * @param timeout  Maximum wait time.
 * @param info     Optional RX metadata (may be NULL).
 *
 * @return Number of bytes copied to @p buf on success.
 * @retval -EAGAIN    No packet within @p timeout.
 * @retval -ECANCELED Stopped by basic_radio_stop().
 * @retval -ETIME     The scheduled start time was missed.
 * @retval -EIO       Radio error, or RX ended without a packet.
 * @return Other negative errno values of basic_radio_recv_async().
 */
int basic_radio_recv(const struct device *dev, uint8_t *buf, uint16_t buf_len,
		     const struct basic_radio_rx_params *params, k_timeout_t timeout,
		     struct basic_radio_rx_info *info);

/**
 * @brief Read the radio time base.
 *
 * The time base is a free-running 64-bit counter in microseconds. A
 * scheduled start (abs_start_time_us) is an absolute compare against this
 * counter, independent of when the previous operation ended, so a sequence
 * of starts computed as t0 + n * period does not accumulate error. The start
 * must be in the future and at most basic_radio_caps::sched_horizon_us
 * ahead. RX timestamps use the same base.
 *
 * May be called from interrupt context.
 *
 * @param dev  Radio device.
 *
 * @return Current time in microseconds.
 */
static inline uint64_t basic_radio_time_get(const struct device *dev)
{
	const struct basic_radio_driver_api *api = dev->api;

	return api->time_get(dev);
}

/**
 * @brief Query the capabilities with the active PHY.
 *
 * @param dev   Radio device.
 * @param caps  Filled with the capabilities.
 *
 * @retval 0        Success.
 * @retval -ENODEV  No active PHY.
 */
static inline int basic_radio_get_caps(const struct device *dev, struct basic_radio_caps *caps)
{
	const struct basic_radio_driver_api *api = dev->api;

	return api->get_caps(dev, caps);
}

/**
 * @brief Transmit an unmodulated continuous wave, blocking until it ends.
 *
 * Immediate tone of basic_radio_tone_async() for test and regulatory work.
 *
 * @param dev          Radio device.
 * @param frequency    Output frequency, 10 Hz units.
 * @param tx_power     TX power in dBm.
 * @param duration_ms  Transmission duration in milliseconds, not 0.
 *
 * @retval 0          The CW transmission ended after @p duration_ms.
 * @retval -EINVAL    @p duration_ms is 0.
 * @retval -ECANCELED Stopped by basic_radio_stop().
 * @retval -ETIMEDOUT The tone did not end in time and was aborted.
 * @retval -EIO       Radio error.
 * @return Other negative errno values of basic_radio_tone_async().
 */
int basic_radio_test_cw(const struct device *dev, uint32_t frequency, int8_t tx_power,
			uint16_t duration_ms);

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
