/*
 * Copyright (c) 2023 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief mDNS responder API
 *
 * This file contains the mDNS responder API. These APIs are used by the
 * to register mDNS records.
 */

#ifndef ZEPHYR_INCLUDE_NET_MDNS_RESPONDER_H_
#define ZEPHYR_INCLUDE_NET_MDNS_RESPONDER_H_

#include <stddef.h>
#include <zephyr/net/dns_sd.h>

/**
 * @brief Register continuous memory of @ref dns_sd_rec records.
 *
 * mDNS responder will start with iteration over mDNS records registered using
 * @ref DNS_SD_REGISTER_SERVICE (if any) and then go over external records.
 *
 * @param records A pointer to an array of mDNS records. It is stored internally
 *                without copying the content so it must be kept valid. It can
 *                be set to NULL, e.g. before freeing the memory block.
 * @param count The number of elements
 * @return 0 for OK; -EINVAL for invalid parameters.
 */
int mdns_responder_set_ext_records(const struct dns_sd_rec *records, size_t count);

#endif /* ZEPHYR_INCLUDE_NET_MDNS_RESPONDER_H_ */
