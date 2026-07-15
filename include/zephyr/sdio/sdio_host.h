/*
 * Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SDIO host-role API
 *
 * Host-role bindings for the role-neutral SDIO core. An @ref sdio_dev created
 * here drives the bus through a Zephyr SD host controller (@ref sdhc_interface) and can
 * be used directly with the function I/O helpers in @ref sdio_core.h.
 *
 * A compatibility bridge is also provided so that an SDIO device already
 * enumerated by the SD-card stack (@ref sd_card) can be wrapped in a
 * role-neutral @ref sdio_dev without re-running enumeration.
 */

#ifndef ZEPHYR_INCLUDE_SDIO_SDIO_HOST_H_
#define ZEPHYR_INCLUDE_SDIO_SDIO_HOST_H_

#include <zephyr/device.h>
#include <zephyr/sdio/sdio_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SDIO host-role abstraction
 * @defgroup sdio_host SDIO host-role abstraction
 * @ingroup sdio_core
 * @{
 */

/* Forward declaration to avoid a hard dependency on the SD-card subsystem. */
struct sd_card;

/**
 * @brief Initialize a host-role SDIO endpoint over an SD host controller.
 *
 * Binds @p dev to the supplied @p sdhc controller and installs the host
 * transport backend. The caller is responsible for having powered and
 * enumerated the bus (for the card-centered flow use @ref sdio_host_bind_card
 * instead).
 *
 * @param dev  endpoint to initialize
 * @param sdhc SD host controller device
 * @param caps initial bus capability flags (see @ref sdio_caps)
 * @retval 0 on success
 * @retval -EINVAL invalid argument
 */
int sdio_host_init(struct sdio_dev *dev, const struct device *sdhc,
		   uint32_t caps);

/**
 * @brief Wrap an enumerated SD-card SDIO device as a role-neutral endpoint.
 *
 * Compatibility bridge between the legacy SD-card centered SDIO API and the
 * role-neutral core. The @p card must already have been initialized with
 * @c sd_init() and be of type @c CARD_SDIO or @c CARD_COMBO. The resulting
 * @p dev shares the controller and negotiated capabilities of the card and may
 * be used with the @ref sdio_core function I/O helpers.
 *
 * @param dev  endpoint to initialize
 * @param card initialized SDIO/combo SD card
 * @retval 0 on success
 * @retval -EINVAL invalid argument
 * @retval -ENOTSUP card is not an SDIO/combo card
 */
int sdio_host_bind_card(struct sdio_dev *dev, struct sd_card *card);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SDIO_SDIO_HOST_H_ */
