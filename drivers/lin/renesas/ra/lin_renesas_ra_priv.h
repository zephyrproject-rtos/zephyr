/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_LIN_RENESAS_RA_LIN_RENESAS_RA_PRIV_H_
#define ZEPHYR_DRIVERS_LIN_RENESAS_RA_LIN_RENESAS_RA_PRIV_H_

#include <zephyr/drivers/lin.h>
#include <r_lin_api.h>

/**
 * @cond INTERNAL_HIDDEN
 *
 * This section contains internal definitions and structures for the Renesas RA LIN driver.
 * It is not part of the public API and should not be used directly by application code.
 */

/**
 * @brief Device states for Renesas RA LIN driver
 */
#define LIN_RENESAS_RA_STATE_IDLE        0
#define LIN_RENESAS_RA_STATE_TX_ON_GOING BIT(0)
#define LIN_RENESAS_RA_STATE_RX_ON_GOING BIT(1)
#define LIN_RENESAS_RA_HEADER_RECEIVED   BIT(2)

/**
 * @brief Configuration structure for Renesas RA LIN driver
 */
struct lin_renesas_ra_cfg {
	/* Common for LIN driver, expected to be the first object in config structure */
	struct lin_driver_config common;
	/* Pointer to private configuration */
	const void *priv;
};

/**
 * @brief Data structure for Renesas RA LIN driver
 */
struct lin_renesas_ra_data {
	/* Common for LIN driver, expected to be the first object in data structure */
	struct lin_driver_data common;
	/* HAL LIN instance */
	lin_instance_t fsp_lin_instance;
	/** In-progress transmission HAL transfer params */
	lin_transfer_params_t last_transfer_params;
	/* Semaphore to signal the completion of transmission */
	struct k_sem transmission_sem;
	/* Work item for handling timeouts */
	struct k_work_delayable timeout_work;
	/* Current device state */
	atomic_t device_state;
	/* Pointer to private data */
	void *priv;
};

/**
 * @brief Get the private config associated with the LIN device.
 *
 * @param dev Pointer to the device structure
 * @return Pointer to the private config.
 */
static inline const void *lin_renesas_ra_get_priv_config(const struct device *dev)
{
	const struct lin_renesas_ra_cfg *cfg = dev->config;

	return cfg->priv;
}

/**
 * @brief Get the private data associated with the LIN device.
 *
 * @param dev Pointer to the device structure.
 * @return Pointer to the private data.
 */
static inline void *lin_renesas_ra_get_priv_data(const struct device *dev)
{
	struct lin_renesas_ra_data *data = dev->data;

	return data->priv;
}

/* Function prototypes for internal use only */
int lin_renesas_ra_start(const struct device *dev);
int lin_renesas_ra_stop(const struct device *dev);
int lin_renesas_ra_get_config(const struct device *dev, struct lin_config *cfg);
int lin_renesas_ra_send(const struct device *dev, const struct lin_msg *msg, k_timeout_t timeout);
int lin_renesas_ra_receive(const struct device *dev, struct lin_msg *msg, k_timeout_t timeout);
int lin_renesas_ra_response(const struct device *dev, const struct lin_msg *msg,
			    k_timeout_t timeout);
int lin_renesas_ra_read(const struct device *dev, struct lin_msg *msg, k_timeout_t timeout);
int lin_renesas_ra_set_callback(const struct device *dev, lin_event_callback_t callback,
				void *user_data);
void lin_renesas_ra_callback_adapter(lin_callback_args_t *p_args);
void lin_renesas_ra_timeout_work_handler(struct k_work *work);

/** @endcond */

#endif /* ZEPHYR_DRIVERS_LIN_RENESAS_RA_LIN_RENESAS_RA_PRIV_H_ */
