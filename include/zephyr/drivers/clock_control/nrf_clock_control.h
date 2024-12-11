/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NRF_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NRF_CLOCK_CONTROL_H_

#include <zephyr/device.h>
#ifdef NRF_CLOCK
#include <hal/nrf_clock.h>
#endif
#include <zephyr/sys/onoff.h>
#include <zephyr/drivers/clock_control.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_CLOCK_CONTROL_NRF)

/** @brief Clocks handled by the CLOCK peripheral.
 *
 * Enum shall be used as a sys argument in clock_control API.
 */
enum clock_control_nrf_type {
	CLOCK_CONTROL_NRF_TYPE_HFCLK,
	CLOCK_CONTROL_NRF_TYPE_LFCLK,
#if NRF_CLOCK_HAS_HFCLK192M
	CLOCK_CONTROL_NRF_TYPE_HFCLK192M,
#endif
#if NRF_CLOCK_HAS_HFCLKAUDIO
	CLOCK_CONTROL_NRF_TYPE_HFCLKAUDIO,
#endif
	CLOCK_CONTROL_NRF_TYPE_COUNT
};

/* Define can be used with clock control API instead of enum directly to
 * increase code readability.
 */
#define CLOCK_CONTROL_NRF_SUBSYS_HF \
	((clock_control_subsys_t)CLOCK_CONTROL_NRF_TYPE_HFCLK)
#define CLOCK_CONTROL_NRF_SUBSYS_LF \
	((clock_control_subsys_t)CLOCK_CONTROL_NRF_TYPE_LFCLK)
#define CLOCK_CONTROL_NRF_SUBSYS_HF192M \
	((clock_control_subsys_t)CLOCK_CONTROL_NRF_TYPE_HFCLK192M)
#define CLOCK_CONTROL_NRF_SUBSYS_HFAUDIO \
	((clock_control_subsys_t)CLOCK_CONTROL_NRF_TYPE_HFCLKAUDIO)

/** @brief LF clock start modes. */
enum nrf_lfclk_start_mode {
	CLOCK_CONTROL_NRF_LF_START_NOWAIT,
	CLOCK_CONTROL_NRF_LF_START_AVAILABLE,
	CLOCK_CONTROL_NRF_LF_START_STABLE,
};

/* Define 32KHz clock source */
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC
#define CLOCK_CONTROL_NRF_K32SRC NRF_CLOCK_LFCLK_RC
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_XTAL
#define CLOCK_CONTROL_NRF_K32SRC NRF_CLOCK_LFCLK_XTAL
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_SYNTH
#define CLOCK_CONTROL_NRF_K32SRC NRF_CLOCK_LFCLK_SYNTH
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_EXT_LOW_SWING
#define CLOCK_CONTROL_NRF_K32SRC NRF_CLOCK_LFCLK_XTAL_LOW_SWING
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_EXT_FULL_SWING
#define CLOCK_CONTROL_NRF_K32SRC NRF_CLOCK_LFCLK_XTAL_FULL_SWING
#endif

/* Define 32KHz clock accuracy */
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_500PPM
#define CLOCK_CONTROL_NRF_K32SRC_ACCURACY 0
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_250PPM
#define CLOCK_CONTROL_NRF_K32SRC_ACCURACY 1
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_150PPM
#define CLOCK_CONTROL_NRF_K32SRC_ACCURACY 2
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_100PPM
#define CLOCK_CONTROL_NRF_K32SRC_ACCURACY 3
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_75PPM
#define CLOCK_CONTROL_NRF_K32SRC_ACCURACY 4
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_50PPM
#define CLOCK_CONTROL_NRF_K32SRC_ACCURACY 5
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_30PPM
#define CLOCK_CONTROL_NRF_K32SRC_ACCURACY 6
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF_K32SRC_20PPM
#define CLOCK_CONTROL_NRF_K32SRC_ACCURACY 7
#endif

/** @brief Force LF clock calibration. */
void z_nrf_clock_calibration_force_start(void);

/** @brief Return number of calibrations performed.
 *
 * Valid when @kconfig{CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_DEBUG} is set.
 *
 * @return Number of calibrations or -1 if feature is disabled.
 */
int z_nrf_clock_calibration_count(void);

/** @brief Return number of attempts when calibration was skipped.
 *
 * Valid when @kconfig{CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_DEBUG} is set.
 *
 * @return Number of calibrations or -1 if feature is disabled.
 */
int z_nrf_clock_calibration_skips_count(void);


/** @brief Returns information if LF clock calibration is in progress.
 *
 * @return True if calibration is in progress, false otherwise.
 */
bool z_nrf_clock_calibration_is_in_progress(void);

/** @brief Get onoff service for given clock subsystem.
 *
 * @param sys Subsystem.
 *
 * @return Service handler or NULL.
 */
struct onoff_manager *z_nrf_clock_control_get_onoff(clock_control_subsys_t sys);

/** @brief Permanently enable low frequency clock.
 *
 * Low frequency clock is usually enabled during application lifetime because
 * of long startup time and low power consumption. Multiple modules can request
 * it but never release.
 *
 * @param start_mode Specify if function should block until clock is available.
 */
void z_nrf_clock_control_lf_on(enum nrf_lfclk_start_mode start_mode);

/** @brief Request high frequency clock from Bluetooth Controller.
 *
 * Function is optimized for Bluetooth Controller which turns HF clock before
 * each radio activity and has hard timing requirements but does not require
 * any confirmation when clock is ready because it assumes that request is
 * performed long enough before radio activity. Clock is released immediately
 * after radio activity.
 *
 * Function does not perform any validation. It is the caller responsibility to
 * ensure that every z_nrf_clock_bt_ctlr_hf_request matches
 * z_nrf_clock_bt_ctlr_hf_release call.
 */
void z_nrf_clock_bt_ctlr_hf_request(void);

/** @brief Release high frequency clock from Bluetooth Controller.
 *
 * See z_nrf_clock_bt_ctlr_hf_request for details.
 */
void z_nrf_clock_bt_ctlr_hf_release(void);

#endif /* defined(CONFIG_CLOCK_CONTROL_NRF) */


#if defined(CONFIG_CLOCK_CONTROL_NRF2)

/* Specifies to use the maximum available frequency for a given clock. */
#define NRF_CLOCK_CONTROL_FREQUENCY_MAX UINT32_MAX

/* Specifies to use the maximum available accuracy for a given clock. */
#define NRF_CLOCK_CONTROL_ACCURACY_MAX      1
/* Specifies the required clock accuracy in parts-per-million. */
#define NRF_CLOCK_CONTROL_ACCURACY_PPM(ppm) (ppm)

/* Specifies that high precision of the clock is required. */
#define NRF_CLOCK_CONTROL_PRECISION_HIGH    1
/* Specifies that default precision of the clock is sufficient. */
#define NRF_CLOCK_CONTROL_PRECISION_DEFAULT 0

struct nrf_clock_spec {
	uint32_t frequency;
	uint16_t accuracy : 15;
	uint16_t precision : 1;
};

__subsystem struct nrf_clock_control_driver_api {
	struct clock_control_driver_api std_api;

	int (*request)(const struct device *dev,
		       const struct nrf_clock_spec *spec,
		       struct onoff_client *cli);
	int (*release)(const struct device *dev,
		       const struct nrf_clock_spec *spec);
	int (*cancel_or_release)(const struct device *dev,
				 const struct nrf_clock_spec *spec,
				 struct onoff_client *cli);
};

/**
 * @brief Request a reservation to use a given clock with specified attributes.
 *
 * The return value indicates the success or failure of an attempt to initiate
 * an operation to request the clock be made available. If initiation of the
 * operation succeeds, the result of the request operation is provided through
 * the configured client notification method, possibly before this call returns.
 *
 * Note that the call to this function may succeed in a case where the actual
 * request fails. Always check the operation completion result.
 *
 * @param dev pointer to the clock device structure.
 * @param spec specification of minimal acceptable attributes, like frequency,
 *             accuracy, and precision, required for the clock.
 *             Value of 0 has the meaning of "default" and can be passed
 *             instead of a given attribute if there is no strict requirement
 *             in this regard. If there is no specific requirement for any of
 *             the attributes, this parameter can be NULL.
 * @param cli pointer to client state providing instructions on synchronous
 *            expectations and how to notify the client when the request
 *            completes. Behavior is undefined if client passes a pointer
 *            object associated with an incomplete service operation.
 *
 * @retval non-negative the observed state of the on-off service associated
 *                      with the clock machine at the time the request was
 *                      processed (see onoff_request()), if successful.
 * @retval -EIO if service has recorded an error.
 * @retval -EINVAL if the function parameters are invalid or the clock
 *                 attributes cannot be provided (e.g. the requested accuracy
 *                 is unavailable).
 * @retval -EAGAIN if the reference count would overflow.
 */
static inline
int nrf_clock_control_request(const struct device *dev,
			      const struct nrf_clock_spec *spec,
			      struct onoff_client *cli)
{
	const struct nrf_clock_control_driver_api *api =
		(const struct nrf_clock_control_driver_api *)dev->api;

	return api->request(dev, spec, cli);
}

/**
 * @brief Synchronously request a reservation to use a given clock with specified attributes.
 *
 * Function can only be called from thread context as it blocks until request is completed.
 * @see nrf_clock_control_request().
 *
 * @param dev pointer to the clock device structure.
 * @param spec See nrf_clock_control_request().
 * @param timeout Request timeout.
 *
 * @retval 0 if request is fulfilled.
 * @retval -EWOULDBLOCK if request is called from the interrupt context.
 * @retval negative See error codes returned by nrf_clock_control_request().
 */
int nrf_clock_control_request_sync(const struct device *dev,
				   const struct nrf_clock_spec *spec,
				   k_timeout_t timeout);

/**
 * @brief Release a reserved use of a clock.
 *
 * @param dev pointer to the clock device structure.
 * @param spec the same specification of the clock attributes that was used
 *             in the reservation request (so that the clock control module
 *             can keep track of what attributes are still requested).
 *
 * @retval non-negative the observed state of the on-off service associated
 *                      with the clock machine at the time the request was
 *                      processed (see onoff_release()), if successful.
 * @retval -EIO if service has recorded an error.
 * @retval -ENOTSUP if the service is not in a state that permits release.
 */
static inline
int nrf_clock_control_release(const struct device *dev,
			      const struct nrf_clock_spec *spec)
{
	const struct nrf_clock_control_driver_api *api =
		(const struct nrf_clock_control_driver_api *)dev->api;

	return api->release(dev, spec);
}

/**
 * @brief Safely cancel a reservation request.
 *
 * It may be that a client has issued a reservation request but needs to
 * shut down before the request has completed. This function attempts to
 * cancel the request and issues a release if cancellation fails because
 * the request was completed. This synchronously ensures that ownership
 * data reverts to the client so is available for a future request.
 *
 * @param dev pointer to the clock device structure.
 * @param spec the same specification of the clock attributes that was used
 *             in the reservation request.
 * @param cli a pointer to the same client state that was provided
 *            when the operation to be cancelled was issued.
 *
 * @retval ONOFF_STATE_TO_ON if the cancellation occurred before the transition
 *                           completed.
 * @retval ONOFF_STATE_ON if the cancellation occurred after the transition
 *                        completed.
 * @retval -EINVAL if the parameters are invalid.
 * @retval negative other errors produced by onoff_release().
 */
static inline
int nrf_clock_control_cancel_or_release(const struct device *dev,
					const struct nrf_clock_spec *spec,
					struct onoff_client *cli)
{
	const struct nrf_clock_control_driver_api *api =
		(const struct nrf_clock_control_driver_api *)dev->api;

	return api->cancel_or_release(dev, spec, cli);
}

#endif /* defined(CONFIG_CLOCK_CONTROL_NRF2) */

/** @brief Get clock frequency that is used for the given node.
 *
 * Macro checks if node has clock property and if yes then if clock has clock_frequency property
 * then it is returned. If it has supported_clock_frequency property with the list of supported
 * frequencies then the last one is returned with assumption that they are ordered and the last
 * one is the highest. If node does not have clock then 16 MHz is returned which is the default
 * frequency.
 *
 * @param node Devicetree node.
 *
 * @return Frequency of the clock that is used for the node.
 */
#define NRF_PERIPH_GET_FREQUENCY(node) \
	COND_CODE_1(DT_CLOCKS_HAS_IDX(node, 0),							\
		(COND_CODE_1(DT_NODE_HAS_PROP(DT_CLOCKS_CTLR(node), clock_frequency),		\
			     (DT_PROP(DT_CLOCKS_CTLR(node), clock_frequency)),			\
			     (DT_PROP_LAST(DT_CLOCKS_CTLR(node), supported_clock_frequency)))),	\
		(NRFX_MHZ_TO_HZ(16)))

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NRF_CLOCK_CONTROL_H_ */
