/*
 * Copyright (c) 2020 Endian Technologies AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GSM_PPP_H_
#define GSM_PPP_H_

/** @cond INTERNAL_HIDDEN */
struct device;
/**
 * @brief  Start the GSM PPP modem.
 *
 * @param  dev: Modem device struct
 *
 * @retval 0 if success, errno.h values otherwise.
 */
int gsm_ppp_start(const struct device *dev);

/**
 * @brief  Stop the GSM PPP modem.
 *
 * @param  dev: Modem device struct
 *
 * @retval 0 if success, errno.h values otherwise.
 */
int gsm_ppp_stop(const struct device *dev);
/** @endcond */

#endif /* GSM_PPP_H_ */
